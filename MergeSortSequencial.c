#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h> // Adicionado para utilizarmos a função pow() na fórmula

/**
 * Método responsável por criar e preencher um vetor dinâmico com números inteiros aleatórios.
 * @param tamanho O tamanho do vetor a ser gerado.
 * @return Um ponteiro para o vetor preenchido com valores aleatórios.
 */
int* preencherVetorAleatorio(int tamanho) {
    // Aloca memória dinamicamente para o vetor de acordo com o tamanho solicitado
    int* vetor = (int*) malloc(tamanho * sizeof(int));
    
    // Boa prática: verifica se o sistema operacional conseguiu alocar a memória
    if (vetor == NULL) {
        printf("Erro fatal: Nao foi possivel alocar memoria.\n");
        exit(1);
    }

    // Preenche o vetor com valores aleatórios entre 0 e 999
    for (int i = 0; i < tamanho; i++) {
        vetor[i] = rand() % 1000;
    }

    return vetor;
}

/**
 * Método que combina (funde) dois subvetores ordenados de forma sequencial.
 * Em C, é fundamental libertar a memória dos vetores temporários no final.
 * @param vetor O vetor principal.
 * @param esquerda O índice inicial do primeiro subvetor.
 * @param meio O índice final do primeiro subvetor.
 * @param direita O índice final do segundo subvetor.
 */
void merge(int vetor[], int esquerda, int meio, int direita) {
    int n1 = meio - esquerda + 1;
    int n2 = direita - meio;

    // Aloca memória dinamicamente para os vetores temporários
    int* vetorEsquerda = (int*) malloc(n1 * sizeof(int));
    int* vetorDireita = (int*) malloc(n2 * sizeof(int));

    // Copia os dados originais para os vetores temporários
    for (int i = 0; i < n1; i++)
        vetorEsquerda[i] = vetor[esquerda + i];
    for (int j = 0; j < n2; j++)
        vetorDireita[j] = vetor[meio + 1 + j];

    int i = 0, j = 0, k = esquerda;
    
    // Compara e insere ordenadamente de volta no vetor original
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

    // Copia os elementos restantes do vetorEsquerda, se houver
    while (i < n1) {
        vetor[k] = vetorEsquerda[i];
        i++;
        k++;
    }

    // Copia os elementos restantes do vetorDireita, se houver
    while (j < n2) {
        vetor[k] = vetorDireita[j];
        j++;
        k++;
    }

    // LIBERAÇÃO DE MEMÓRIA: Passo essencial na linguagem C
    free(vetorEsquerda);
    free(vetorDireita);
}

/**
 * Método principal do MergeSort que divide o vetor recursivamente.
 */
void mergeSort(int vetor[], int esquerda, int direita) {
    if (esquerda < direita) {
        // Evita overflow na soma
        int meio = esquerda + (direita - esquerda) / 2;

        mergeSort(vetor, esquerda, meio);
        mergeSort(vetor, meio + 1, direita);

        merge(vetor, esquerda, meio, direita);
    }
}

int main() {
    // Inicializa a semente geradora de números aleatórios com o tempo atual do sistema
    srand(time(NULL));
    
    printf("Iniciando testes de desempenho do MergeSort Sequencial em C...\n\n");
    
    // Abre o arquivo de log para anexar (append), preservando execuções anteriores
    FILE *logFile = fopen("log_mergesort_sequencial_c.txt", "a");
    if (logFile == NULL) {
        printf("Erro fatal: Nao foi possivel abrir ou criar o arquivo de log.\n");
        return 1;
    }

    fprintf(logFile, "--- Novo Teste de Desempenho (MergeSort Sequencial em C) ---\n");

    // Laço para variar o expoente n de 0 até 6, calculando 2^n * 1000
    for (int n = 0; n <= 25; n++) {
        // Calcula o tamanho dinâmico do vetor
        int tamanho = (int)(pow(2, n) * 1000);
        
        // Instancia e preenche o vetor chamando a nossa fábrica de dados
        int* meuVetor = preencherVetorAleatorio(tamanho);
        
        // Marca o tempo de início capturando os ciclos do processador
        clock_t tempoInicial = clock();
        
        // Chama o algoritmo de ordenação para todo o vetor
        mergeSort(meuVetor, 0, tamanho - 1);
        
        // Marca o tempo de encerramento do processamento
        clock_t tempoFinal = clock();
        
        // Calcula a diferença, converte para segundos e depois para milissegundos
        double tempoExecucao = ((double)(tempoFinal - tempoInicial)) / CLOCKS_PER_SEC * 1000.0;
        
        // Exibe o resultado da carga atual no console para acompanhamento
        printf("%d elements => %.0f ms\n", tamanho, tempoExecucao);
        
        // Grava o mesmo resultado formatado no nosso arquivo de log
        fprintf(logFile, "%d elements => %.0f ms\n", tamanho, tempoExecucao);
        
        // Extremamente importante em C: libera a memória alocada dinamicamente nesta iteração
        // Se isso não for feito, os testes maiores travariam o sistema por falta de RAM
        free(meuVetor);
    }
    
    printf("\nResultados salvos com sucesso no arquivo 'log_mergesort_sequencial_c.txt'.\n");
    
    // Fecha a conexão com o arquivo gravado no disco rígido
    fclose(logFile);
    
    return 0;
}