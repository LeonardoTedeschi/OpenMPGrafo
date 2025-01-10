#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

// Estrutura para armazenar os resultados
typedef struct {
    int i, j, k;
} Resultado;

// Buffer local de cada thread
typedef struct {
    Resultado *resultados;
    int contador;
    int capacidade;
} BufferThread;

// Argumentos para as threads
typedef struct {
    int thread_id;
    int num_threads;
    int num_vertices;
    int **adjacencia;
    int *graus;
    pthread_mutex_t *mutex_arquivo;
    FILE *arquivo_saida;
} ArgumentosThread;

// Função para inicializar uma lista de adjacência
int **inicializar_lista_adjacencia(int num_vertices) {
    int **lista = (int **)malloc(num_vertices * sizeof(int *));
    for (int i = 0; i < num_vertices; i++) {
        lista[i] = NULL;
    }
    return lista;
}

// Função para adicionar um vizinho à lista de adjacência
void adicionar_vizinho(int **adjacencia, int *graus, int vertice, int vizinho) {
    graus[vertice]++;
    adjacencia[vertice] = realloc(adjacencia[vertice], graus[vertice] * sizeof(int));
    adjacencia[vertice][graus[vertice] - 1] = vizinho;
}

// Função para encontrar o número de vizinhos comuns
int encontrar_vizinhos_comuns(int *v1, int tam1, int *v2, int tam2) {
    int i = 0, j = 0, comuns = 0;
    while (i < tam1 && j < tam2) {
        if (v1[i] < v2[j]) {
            i++;
        } else if (v1[i] > v2[j]) {
            j++;
        } else {
            comuns++;
            i++;
            j++;
        }
    }
    return comuns;
}

// Função para adicionar resultados no buffer
void adicionar_resultado(BufferThread *buffer, int i, int j, int k) {
    if (buffer->contador == buffer->capacidade) {
        buffer->capacidade *= 2;
        buffer->resultados = realloc(buffer->resultados, buffer->capacidade * sizeof(Resultado));
    }
    buffer->resultados[buffer->contador++] = (Resultado){i, j, k};
}

// Função executada por cada thread
void *processar_pares(void *args) {
    ArgumentosThread *argumentos = (ArgumentosThread *)args;

    // Inicializa o buffer local
    BufferThread buffer = {.resultados = malloc(100 * sizeof(Resultado)),
                           .contador = 0,
                           .capacidade = 100};

    struct timeval inicio_thread, fim_thread;
    gettimeofday(&inicio_thread, NULL);

    // Processa os pares de vértices
    for (int i = argumentos->thread_id; i < argumentos->num_vertices; i += argumentos->num_threads) {
        for (int j = i + 1; j < argumentos->num_vertices; j++) {
            int comuns = encontrar_vizinhos_comuns(argumentos->adjacencia[i], argumentos->graus[i],
                                                   argumentos->adjacencia[j], argumentos->graus[j]);
            if (comuns > 0) {
                adicionar_resultado(&buffer, i, j, comuns);
            }
        }
    }

    gettimeofday(&fim_thread, NULL);
    double tempo_thread = (fim_thread.tv_sec - inicio_thread.tv_sec) +
                          (fim_thread.tv_usec - inicio_thread.tv_usec) / 1e6;

    // Escreve os resultados no arquivo de saída (sequencial)
    pthread_mutex_lock(argumentos->mutex_arquivo);
    for (int i = 0; i < buffer.contador; i++) {
        fprintf(argumentos->arquivo_saida, "%d %d %d\n",
                buffer.resultados[i].i, buffer.resultados[i].j, buffer.resultados[i].k);
    }
    printf("Thread %d completou em %.6f segundos.\n", argumentos->thread_id, tempo_thread);
    pthread_mutex_unlock(argumentos->mutex_arquivo);

    free(buffer.resultados);
    return (void *)(size_t)tempo_thread;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <arquivo_edgelist> <numero_threads>\n", argv[0]);
        return 1;
    }

    // Obtém o número de threads
    int num_threads = atoi(argv[2]);
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_threads > num_cores) {
        num_threads = num_cores;
        printf("Ajustando o número de threads para %d (núcleos disponíveis).\n", num_threads);
    }

    struct timeval inicio_total, fim_total, inicio_sequencial, fim_sequencial;
    gettimeofday(&inicio_total, NULL);

    // Abre o arquivo de entrada
    FILE *arquivo_entrada = fopen(argv[1], "r");
    if (!arquivo_entrada) {
        perror("Erro ao abrir o arquivo de entrada");
        return 1;
    }

    // Determina o número de vértices e carrega as arestas
    int num_vertices = 0, u, v;
    while (fscanf(arquivo_entrada, "%d %d", &u, &v) == 2) {
        if (u > num_vertices) num_vertices = u;
        if (v > num_vertices) num_vertices = v;
    }
    num_vertices++; // Para incluir o vértice de índice máximo
    rewind(arquivo_entrada);

    int **adjacencia = inicializar_lista_adjacencia(num_vertices);
    int *graus = calloc(num_vertices, sizeof(int));

    while (fscanf(arquivo_entrada, "%d %d", &u, &v) == 2) {
        adicionar_vizinho(adjacencia, graus, u, v);
        adicionar_vizinho(adjacencia, graus, v, u);
    }
    fclose(arquivo_entrada);

    // Ordena as listas de adjacência para interseção eficiente
    for (int i = 0; i < num_vertices; i++) {
        if (graus[i] > 1) {
            qsort(adjacencia[i], graus[i], sizeof(int), (int (*)(const void *, const void *))strcmp);
        }
    }

    // Prepara o arquivo de saída
    char arquivo_saida_nome[256];
    snprintf(arquivo_saida_nome, sizeof(arquivo_saida_nome), "%.*s.cng",
             (int)(strrchr(argv[1], '.') - argv[1]), argv[1]);
    FILE *arquivo_saida = fopen(arquivo_saida_nome, "w");
    if (!arquivo_saida) {
        perror("Erro ao abrir o arquivo de saída");
        return 1;
    }

    // Inicia a parte paralela
    pthread_t threads[num_threads];
    ArgumentosThread argumentos[num_threads];
    pthread_mutex_t mutex_arquivo = PTHREAD_MUTEX_INITIALIZER;

    gettimeofday(&inicio_sequencial, NULL);
    for (int i = 0; i < num_threads; i++) {
        argumentos[i] = (ArgumentosThread){
            .thread_id = i,
            .num_threads = num_threads,
            .num_vertices = num_vertices,
            .adjacencia = adjacencia,
            .graus = graus,
            .mutex_arquivo = &mutex_arquivo,
            .arquivo_saida = arquivo_saida,
        };
        pthread_create(&threads[i], NULL, processar_pares, &argumentos[i]);
    }
    gettimeofday(&fim_sequencial, NULL);

    double tempo_sequencial = (fim_sequencial.tv_sec - inicio_sequencial.tv_sec) +
                              (fim_sequencial.tv_usec - inicio_sequencial.tv_usec) / 1e6;

    // Aguarda todas as threads
    double tempo_total_threads = 0;
    for (int i = 0; i < num_threads; i++) {
        void *retorno;
        pthread_join(threads[i], &retorno);
        tempo_total_threads += (double)(size_t)retorno;
    }

    fclose(arquivo_saida);

    gettimeofday(&fim_total, NULL);
    double tempo_total = (fim_total.tv_sec - inicio_total.tv_sec) +
                         (fim_total.tv_usec - inicio_total.tv_usec) / 1e6;

    // Relatórios
    printf("Tempo total: %.6f segundos.\n", tempo_total);
    printf("Tempo da parte sequencial: %.6f segundos.\n", tempo_sequencial);
    printf("Tempo total das threads: %.6f segundos.\n", tempo_total_threads);

    // Libera memória
    for (int i = 0; i < num_vertices; i++) {
        free(adjacencia[i]);
    }
    free(adjacencia);
    free(graus);

    return 0;
}
