#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h> // Adicionado para utilizarmos a função pow() na fórmula
#include <omp.h> // Inclusão obrigatória para utilizarmos as diretivas do OpenMP

// Definição do limite de corte para o controle de granularidade
#define LIMITE_GRANULARIDADE 10000

/**
 * Método responsável por criar e preencher um vetor dinâmico com números inteiros aleatórios.
 * Mantido de forma sequencial para evitar "data race" na função rand() padrão do C.
 * @param tamanho O tamanho do vetor a ser gerado.
 * @return Um ponteiro para o vetor preenchido com valores aleatórios.
 */
int* preencherVetorAleatorio(int tamanho) {
    int* vetor = (int*) malloc(tamanho * sizeof(int));
    
    if (vetor == NULL) {
        printf("Erro fatal: Nao foi possivel alocar memoria.\n");
        exit(1);
    }

    for (int i = 0; i < tamanho; i++) {
        vetor[i] = rand() % 1000;
    }

    return vetor;
}

/**
 * Método que combina (funde) dois subvetores ordenados de forma sequencial.
 * A fase de conquista (merge) permanece sequencial para cada conjunto de subvetores.
 * @param vetor O vetor principal.
 * @param esquerda O índice inicial do primeiro subvetor.
 * @param meio O índice final do primeiro subvetor.
 * @param direita O índice final do segundo subvetor.
 */
void merge(int vetor[], int esquerda, int meio, int direita) {
    int n1 = meio - esquerda + 1;
    int n2 = direita - meio;

    int* vetorEsquerda = (int*) malloc(n1 * sizeof(int));
    int* vetorDireita = (int*) malloc(n2 * sizeof(int));

    for (int i = 0; i < n1; i++)
        vetorEsquerda[i] = vetor[esquerda + i];
    for (int j = 0; j < n2; j++)
        vetorDireita[j] = vetor[meio + 1 + j];

    int i = 0, j = 0, k = esquerda;
    
    while (i < n1 && j < n2) {
        if (vetorEsquerda[i] <= vetorDireita[j]) {
            vetor[k] = vetorEsquerda[i];
            i++;
        } else {
            vetor[k] = vetorDireita[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        vetor[k] = vetorEsquerda[i];
        i++;
        k++;
    }

    while (j < n2) {
        vetor[k] = vetorDireita[j];
        j++;
        k++;
    }

    free(vetorEsquerda);
    free(vetorDireita);
}

/**
 * Método principal do MergeSort paralelizado com OpenMP Tasks e controle de granularidade.
 */
void mergeSort(int vetor[], int esquerda, int direita) {
    if (esquerda < direita) {
        int meio = esquerda + (direita - excitement) / 2;
        int meio = esquerda + (direita - esquerda) / 2;

        // Se o tamanho do subvetor atual for menor que o limite estabelecido,
        // realiza as chamadas recursivas de forma sequencial na mesma thread.
        if ((direita - esquerda) < LIMITE_GRANULARIDADE) {
            mergeSort(vetor, esquerda, meio);
            mergeSort(vetor, meio + 1, direita);
        } else {
            // Cria uma tarefa paralela independente para ordenar a primeira metade
            // A cláusula shared(vetor) garante que todas as threads manipulem a mesma memória principal
            #pragma omp task shared(vetor)
            mergeSort(vetor, esquerda, meio);

            // Cria uma tarefa paralela independente para ordenar a segunda metade
            #pragma omp task shared(vetor)
            mergeSort(vetor, meio + 1, direita);

            // Barreira de sincronização: aguarda rigorosamente que as duas metades acima terminem
            #pragma omp taskwait
        }

        // Após as duas metades estarem ordenadas pelas threads, o código as funde
        merge(vetor, esquerda, meio, direita);
    }
}

int main() {
    // Configura o número máximo de threads do OpenMP para 8
    omp_set_num_threads(8);
    
    srand(time(NULL));
    printf("Iniciando testes de desempenho do MergeSort Paralelo com OpenMP...\n\n");
    
    // Abre o arquivo de log para anexar (append), preservando execuções anteriores
    FILE *logFile = fopen("log_mergesort_paralelo_c.txt", "a");
    if (logFile == NULL) {
        printf("Erro fatal: Nao foi possivel abrir ou criar o arquivo de log.\n");
        return 1;
    }

    fprintf(logFile, "--- Novo Teste de Desempenho (MergeSort Paralelo em C) ---\n");

    // Laço para variar o expoente n de 0 até 25, calculando 2^n * 1000
    for (int n = 0; n <= 25; n++) {
        // Calcula o tamanho dinâmico do vetor
        int tamanho = (int)(pow(2, n) * 1000);
        
        // Instancia e preenche o vetor chamando a nossa fábrica de dados
        int* meuVetor = preencherVetorAleatorio(tamanho);
        
        // Marca o tempo real de início utilizando a métrica correta do OpenMP
        double tempoInicial = omp_get_wtime();
        
        // O bloco parallel cria a nossa equipe de 8 threads
        #pragma omp parallel
        {
            // A diretiva single garante que a PRIMEIRA chamada do MergeSort seja feita
            // por apenas UMA thread mestre.
            #pragma omp single
            {
                mergeSort(meuVetor, 0, tamanho - 1);
            }
        }
        
        // Marca o tempo de encerramento do processamento em tempo real
        double tempoFinal = omp_get_wtime();
        
        // Calcula a diferença em segundos e converte para milissegundos
        double tempoExecucao = (tempoFinal - tempoInicial) * 1000.0;
        
        // Exibe o resultado da carga atual no console para acompanhamento
        printf("%d elements => %.0f ms\n", tamanho, tempoExecucao);
        
        // Grava o mesmo resultado formatado no nosso arquivo de log
        fprintf(logFile, "%d elements => %.0f ms\n", tamanho, tempoExecucao);
        
        // Libera a memória alocada dinamicamente nesta iteração
        free(meuVetor);
    }
    
    printf("\nResultados salvos com sucesso no arquivo 'log_mergesort_paralelo_c.txt'.\n");
    
    // Fecha a conexão com o arquivo gravado no disco rígido
    fclose(logFile);
    
    return 0;
}
