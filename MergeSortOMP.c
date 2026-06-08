#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

// Definição do limite de corte (Threshold) para o controle de granularidade das tarefas.
// Fragmentos menores que 10.000 elementos serão desviados para execução sequencial linear.
#define LIMITE 2000

/**
 * Método que combina (funde) dois subvetores ordenados de forma sequencial.
 * GARGALO ARQUITETURAL: Realiza alocações dinâmicas concorrentes via malloc dentro do escopo recursivo.
 * @param arr O vetor principal contendo os dados.
 * @param l O índice inicial do primeiro subvetor (metade esquerda).
 * @param m O índice correspondente ao ponto médio de divisão.
 * @param r O índice final do segundo subvetor (metade direita).
 */
void merge(int arr[], int l, int m, int r) {

    int n1 = m - l + 1;
    int n2 = r - m;

    // Alocação dinâmica síncrona de memória RAM para as estruturas auxiliares de cópia.
    // Ponto de alta contenção de locks globais na biblioteca Glibc do sistema operacional.
    int *L = malloc(n1 * sizeof(int));
    int *R = malloc(n2 * sizeof(int));

    // Transferência síncrona de dados do vetor principal para os arrays temporários de trânsito
    for (int i = 0; i < n1; i++)
        L[i] = arr[l + i];

    for (int i = 0; i < n2; i++)
        R[i] = arr[m + 1 + i];

    int i = 0, j = 0, k = l;

    // Intercalação ordenada dos dados de volta na estrutura original
    while (i < n1 && j < n2) {
        if (L[i] <= R[j])
            arr[k++] = L[i++];
        else
            arr[k++] = R[j++];
    }

    // Processamento dos elementos remanescentes da metade esquerda, se houver
    while (i < n1)
        arr[k++] = L[i++];

    // Processamento dos elementos remanescentes da metade direita, se houver
    while (j < n2)
        arr[k++] = R[j++];

    // Desalocação síncrona obrigatória de memória RAM no final de cada intercalação recursiva.
    // Nova disputa por travas exclusivas de controle de páginas no Kernel Linux.
    free(L);
    free(R);
}

/**
 * Método principal do MergeSort paralelizado com OpenMP Tasks e controle de granularidade fina.
 * @param arr O vetor contendo os elementos a serem ordenados.
 * @param l O limite indexador à esquerda do subvetor.
 * @param r O limite indexador à direita do subvetor.
 */
void mergeSortParallel(int arr[], int l, int r) {

    if (l < r) {

        int m = l + (r - l) / 2;

        // Avaliação do limite de corte para controle preventivo do overhead de tarefas
        if ((r - l) < LIMITE) {

            // Execução recursiva linear na mesma thread síncrona se o tamanho cair abaixo do limite
            mergeSortParallel(arr, l, m);
            mergeSortParallel(arr, m + 1, r);

        } else {

            // Cria uma tarefa assíncrona independente no OpenMP para processar a metade esquerda.
            // A cláusula shared(arr) compartilha de forma direta a referência do ponteiro na memória RAM.
            #pragma omp task
            mergeSortParallel(arr, l, m);

            // Cria uma tarefa assíncrona independente no OpenMP para processar a metade direita.
            #pragma omp task
            mergeSortParallel(arr, m + 1, r);

            // Barreira de sincronização local: suspende a thread pai até a conclusão de ambas as tasks filhas
            #pragma omp taskwait
        }

        // Fusão dos fragmentos ordenados utilizando a rotina clássica com malloc dinâmico síncrono
        merge(arr, l, m, r);
    }
}

int main() {

    // Parametrização explícita do pool de execução para operar com 8 threads físicas dedicadas
    omp_set_num_threads(8);

    srand(time(NULL));
    printf("Iniciando testes de desempenho do MergeSort Paralelo com OpenMP...\n\n");

    // INCORPORAÇÃO CORRIGIDA: Abertura automatizada do ficheiro de log persistente em modo append
    FILE *logFile = fopen("log_mergesort_paralelo_c.txt", "a");
    if (logFile == NULL) {
        printf("Erro fatal: Nao foi possivel abrir ou criar o arquivo de log.\n");
        return 1;
    }

    fprintf(logFile, "--- Novo Teste de Desempenho (MergeSort Paralelo em C) ---\n");

    // Inicialização do laço de estresse exponencial até o expoente limite sugerido (n=25)
    for(int exp = 0; exp <= 25; exp++) {

        // Cálculo dinâmico do tamanho volumétrico baseado na expressão exponencial do problema
        int tamanho = 1000 * (1 << exp);

        // Alocação inicial da grande matriz de dados contígua no Heap
        int *vetor = malloc(tamanho * sizeof(int));
        if (vetor == NULL) {
            printf("Erro fatal: Nao foi possivel alocar memoria para o tamanho %d.\n", tamanho);
            fprintf(logFile, "Erro fatal: Nao foi possivel alocar memoria para o tamanho %d.\n", tamanho);
            break;
        }

        // Preenchimento contíguo do vetor utilizando valores pseudoaleatórios compactados
        for(int i = 0; i < tamanho; i++)
            vetor[i] = rand() % 1000;

        // Captura do relógio de parede de alta resolução através da API nativa do OpenMP
        double inicio = omp_get_wtime();

        // Abertura da região paralela concorrente do OpenMP
        #pragma omp parallel
        {
            // Garante que apenas a thread mestre faça o disparo da tarefa raiz do MergeSort
            #pragma omp single
            {
                mergeSortParallel(vetor, 0, tamanho - 1);
            }
        }

        // Captura do tempo final após a barreira de sincronização global implicada no fechamento do bloco
        double fim = omp_get_wtime();
        double tempoExecucao = (fim - inicio) * 1000.0;

        // INCORPORAÇÃO CORRIGIDA: Escrita duplicada e simultânea no terminal e no ficheiro de log local
        printf("%d elements => %.0f ms\n", tamanho, tempoExecucao);
        fprintf(logFile, "%d elements => %.0f ms\n", tamanho, tempoExecucao);

        // Liberação de espaço do vetor contíguo principal no encerramento desta iteração
        free(vetor);
    }

    // INCORPORAÇÃO CORRIGIDA: Fechamento seguro da conexão com o descritor de ficheiro do sistema operacional
    fclose(logFile);
    printf("\nResultados salvos com sucesso no arquivo 'log_mergesort_paralelo_c.txt'.\n");

    return 0;
}