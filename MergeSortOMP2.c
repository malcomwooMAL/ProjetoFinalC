#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h> 
#include <omp.h> 

// Definição do Threshold Otimizado (Ponto de Corte de Alta Performance).
// Subvetores menores que 100.000 elementos não gerarão o overhead de novas tasks.
#define THRESHOLD 100000 

/**
 * Método responsável por criar e preencher um vetor dinâmico com números inteiros aleatórios.
 * @param tamanho O tamanho do vetor a ser gerado.
 * @return Um ponteiro para o vetor preenchido com valores aleatórios.
 */
int* preencherVetorAleatorio(int tamanho) {
    int* vetor = (int*) malloc(tamanho * sizeof(int));
    
    if (vetor == NULL) {
        printf("Erro fatal: Nao foi possivel alocar memoria para o vetor principal.\n");
        exit(1);
    }

    for (int i = 0; i < tamanho; i++) {
        vetor[i] = rand() % 1000;
    }

    return vetor;
}

/**
 * Método otimizado que combina dois subvetores utilizando um espaço de memória pré-alocado.
 * ELIMINADO: Chamadas a malloc() e free() foram completamente extirpadas deste escopo.
 * @param vetor O vetor principal contendo os dados.
 * @param vetorTemp O vetor auxiliar pré-alocado que serve de área de trânsito.
 * @param esquerda O índice inicial do primeiro subvetor.
 * @param meio O índice final do primeiro subvetor.
 * @param direita O índice final do segundo subvetor.
 */
void mergeOtimizado(int vetor[], int vetorTemp[], int esquerda, int meio, int direita) {
    // Copia os dados do fragmento atual para o vetor temporário correspondente
    for (int i = esquerda; i <= direita; i++) {
        vetorTemp[i] = vetor[i];
    }

    int i = esquerda;      // Índice inicial do primeiro subvetor (metade esquerda)
    int j = meio + 1;      // Índice inicial do segundo subvetor (metade direita)
    int k = esquerda;      // Índice de posicionamento no vetor principal

    // Intercala os elementos de forma ordenada de volta no vetor original
    while (i <= meio && j <= direita) {
        if (vetorTemp[i] <= vetorTemp[j]) {
            vetor[k] = vetorTemp[i];
            i++;
        } else {
            vetor[k] = vetorTemp[j];
            j++;
        }
        k++;
    }

    // Copia os elementos restantes da metade esquerda, se houver
    while (i <= meio) {
        vetor[k] = vetorTemp[i];
        i++;
        k++;
    }
    
    // Nota arquitetural: Os elementos restantes da metade direita nao precisam ser copiados,
    // pois ja ocupam a posicao correta no final do vetor principal.
}

/**
 * Método auxiliar puramente sequencial que utiliza a memória pré-alocada.
 * Invocado automaticamente quando o tamanho do bloco cai abaixo do THRESHOLD otimizado.
 */
void mergeSortSequencialOtimizado(int vetor[], int vetorTemp[], int esquerda, int direita) {
    if (esquerda < direita) {
        int meio = esquerda + (direita - esquerda) / 2;

        mergeSortSequencialOtimizado(vetor, vetorTemp, esquerda, meio);
        mergeSortSequencialOtimizado(vetor, vetorTemp, meio + 1, direita);

        mergeOtimizado(vetor, vetorTemp, esquerda, meio, direita);
    }
}

/**
 * Método principal do MergeSort paralelizado com OpenMP Tasks, Pre-alocação e Granularidade Fina.
 */
void mergeSortOtimizado(int vetor[], int vetorTemp[], int esquerda, int direita) {
    // Avalia o novo Ponto de Corte de alta granularidade
    if ((direita - esquerda) < THRESHOLD) {
        mergeSortSequencialOtimizado(vetor, vetorTemp, esquerda, direita);
        return; 
    }

    if (esquerda < direita) {
        int meio = esquerda + (direita - esquerda) / 2;

        // Cria uma tarefa paralela independente para a metade esquerda
        // Ambas as threads compartilham de forma segura o vetor principal e o vetor temporario
        #pragma omp task shared(vetor, vetorTemp) firstprivate(esquerda, meio)
        mergeSortOtimizado(vetor, vetorTemp, esquerda, meio);

        // Cria uma tarefa paralela independente para a metade direita
        #pragma omp task shared(vetor, vetorTemp) firstprivate(meio, direita)
        mergeSortOtimizado(vetor, vetorTemp, meio + 1, direita);

        // Barreira de sincronização: aguarda a conclusão das duas tarefas de divisão acima
        #pragma omp taskwait

        // Realiza a fusão dos dados ordenados utilizando a memória de trânsito compartilhada
        mergeOtimizado(vetor, vetorTemp, esquerda, meio, direita);
    }
}

int main() {
    // Configura o pool para 8 threads físicas, pareando com o ambiente de hardware
    omp_set_num_threads(8);
    
    srand(time(NULL));
    printf("Iniciando testes do MergeSort Paralelo Altamente Otimizado em C...\n\n");
    
    FILE *logFile = fopen("log_mergesort_paralelo_otimizado_c.txt", "a");
    if (logFile == NULL) {
        printf("Erro fatal: Nao foi possivel abrir ou criar o arquivo de log.\n");
        return 1;
    }

    fprintf(logFile, "--- Novo Teste de Desempenho (MergeSort Paralelo Otimizado em C) ---\n");

    // Laço parametrizado até o teto físico seguro estabelecido para o hardware da máquina (n = 20)
    for (int n = 0; n <= 20; n++) {
        int tamanho = (int)(pow(2, n) * 1000);
        
        // Instancia o vetor principal com valores aleatorios
        int* meuVetor = preencherVetorAleatorio(tamanho);
        
        // PRÉ-ALOCAÇÃO GLOBAL: Aloca o vetor de trânsito uma única vez para esta iteração
        int* meuVetorTemp = (int*) malloc(tamanho * sizeof(int));
        if (meuVetorTemp == NULL) {
            printf("Erro fatal: Memoria RAM insuficiente para o vetor auxiliar na iteracao n=%d.\n", n);
            free(meuVetor);
            break;
        }
        
        double tempoInicial = omp_get_wtime();
        
        // Abre a região paralela do OpenMP
        #pragma omp parallel
        {
            // Garante que apenas a thread mestre faça o disparo inicial do algoritmo
            #pragma omp single
            {
                mergeSortOtimizado(meuVetor, meuVetorTemp, 0, tamanho - 1);
            }
        }
        
        double tempoFinal = omp_get_wtime();
        double tempoExecucao = (tempoFinal - tempoInicial) * 1000.0;
        
        printf("%d elements => %.0f ms\n", tamanho, tempoExecucao);
        fprintf(logFile, "%d elements => %.0f ms\n", tamanho, tempoExecucao);
        
        // Liberação cirúrgica das duas grandes estruturas de memória RAM
        free(meuVetor);
        free(meuVetorTemp);
    }
    
    printf("\nResultados salvos com sucesso no arquivo 'log_mergesort_paralelo_otimizado_c.txt'.\n");
    fclose(logFile);
    
    return 0;
}