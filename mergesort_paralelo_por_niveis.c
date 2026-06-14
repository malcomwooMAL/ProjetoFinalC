#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <omp.h>

/**
 * Método sequencial que combina (funde) dois subvetores ordenados.
 * @param arr O vetor principal contendo os dados.
 * @param l O índice inicial do primeiro subvetor.
 * @param m O índice do ponto médio.
 * @param r O índice final do segundo subvetor.
 */
void merge(int arr[], int l, int m, int r) {
    int n1 = m - l + 1;
    int n2 = r - m;

    int *L = (int*) malloc(n1 * sizeof(int));
    int *R = (int*) malloc(n2 * sizeof(int));

    for (int i = 0; i < n1; i++) L[i] = arr[l + i];
    for (int i = 0; i < n2; i++) R[i] = arr[m + 1 + i];

    int i = 0, j = 0, k = l;

    while (i < n1 && j < n2) {
        if (L[i] <= R[j])
            arr[k++] = L[i++];
        else
            arr[k++] = R[j++];
    }

    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];

    free(L);
    free(R);
}

/**
 * Rotina puramente sequencial do Merge Sort.
 * Será acionada quando a árvore de tarefas paralelas atingir o nível 0.
 */
void mergeSortSequencial(int arr[], int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        mergeSortSequencial(arr, l, m);
        mergeSortSequencial(arr, m + 1, r);
        merge(arr, l, m, r);
    }
}

/**
 * Método principal do MergeSort Paralelo controlado por NÍVEIS de profundidade.
 * @param arr O vetor contendo os elementos.
 * @param l Índice à esquerda.
 * @param r Índice à direita.
 * @param level Nível atual de profundidade permitida para gerar threads.
 */
void mergeSortParallelLevels(int arr[], int l, int r, int level) {
    if (l < r) {
        // Se a profundidade limite foi alcançada (nível 0), 
        // cessa o paralelismo e resolve o resto sequencialmente.
        if (level == 0) {
            mergeSortSequencial(arr, l, r);
            return;
        }

        int m = l + (r - l) / 2;

        // Cria task para a metade esquerda, reduzindo 1 nível de profundidade
        #pragma omp task shared(arr)
        mergeSortParallelLevels(arr, l, m, level - 1);

        // Cria task para a metade direita, reduzindo 1 nível de profundidade
        #pragma omp task shared(arr)
        mergeSortParallelLevels(arr, m + 1, r, level - 1);

        // Aguarda as duas tasks filhas terminarem
        #pragma omp taskwait

        // Junta as metades ordenadas
        merge(arr, l, m, r);
    }
}

int main() {
    // Configura 8 threads lógicas/físicas para o OpenMP usar
    omp_set_num_threads(8);

    srand(time(NULL));
    printf("Iniciando testes: MergeSort Paralelo em C (Controle por Niveis)...\n\n");

    FILE *logFile = fopen("log_mergesort_paralelo_niveis_c.txt", "a");
    if (logFile == NULL) {
        printf("Erro fatal: Nao foi possivel criar o arquivo de log.\n");
        return 1;
    }

    fprintf(logFile, "--- Teste OpenMP: Abordagem por Niveis ---\n");

    // Mantemos a mesma lógica exponencial de estresse dos testes anteriores
    for(int exp = 0; exp <= 25; exp++) {
        
        // Uso de long long para evitar overflow precoce na variavel 'tamanho'
        long long tamanho = 1000LL * (1LL << exp);
        
        // Evita tentar alocar se sabidamente vai estourar a RAM 
        // (O limite seguro prático antes de corromper o malloc neste hardware)
        if (tamanho > 2100000000LL) {
            printf("Abortando tamanho gigante para evitar travamento severo do SO.\n");
            break;
        }

        int *vetor = (int*) malloc(tamanho * sizeof(int));
        if (vetor == NULL) {
            printf("Erro: Sem memoria para %lld elementos.\n", tamanho);
            fprintf(logFile, "Erro: Sem memoria para %lld elementos.\n", tamanho);
            break;
        }

        for(long long i = 0; i < tamanho; i++) {
            vetor[i] = rand() % 1000;
        }

        double inicio = omp_get_wtime();

        // Nível 3 gera exatamente 2^3 = 8 ramos terminais (tasks simultâneas máximas)
        int LEVEL = 3; 

        #pragma omp parallel
        {
            #pragma omp single
            {
                // Dispara a árvore informando que ela tem 3 níveis de profundidade de permissão
                mergeSortParallelLevels(vetor, 0, tamanho - 1, LEVEL);
            }
        }

        double fim = omp_get_wtime();
        double tempoExecucao = (fim - inicio) * 1000.0;

        printf("%lld elements => %.0f ms\n", tamanho, tempoExecucao);
        fprintf(logFile, "%lld elements => %.0f ms\n", tamanho, tempoExecucao);

        free(vetor);
    }

    fclose(logFile);
    printf("\nTestes concluidos.\n");

    return 0;
}