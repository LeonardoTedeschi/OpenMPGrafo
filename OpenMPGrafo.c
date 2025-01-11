#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

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

// Função para calcular a interseção entre duas listas de adjacência
int calcular_vizinhos_comuns(int *lista_u, int grau_u, int *lista_v, int grau_v) {
    int i = 0, j = 0, contador = 0;
    while (i < grau_u && j < grau_v) {
        if (lista_u[i] < lista_v[j]) {
            i++;
        } else if (lista_u[i] > lista_v[j]) {
            j++;
        } else {
            contador++;
            i++;
            j++;
        }
    }
    return contador;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <arquivo.edgelist> <numero_de_threads>\n", argv[0]);
        return 1;
    }

    char *arquivo_entrada = argv[1];
    int num_threads = atoi(argv[2]);
    if (num_threads <= 0) {
        printf("Número de threads inválido: %d\n", num_threads);
        return 1;
    }

    char arquivo_saida[256];
    strncpy(arquivo_saida, arquivo_entrada, strlen(arquivo_entrada) - strlen(strrchr(arquivo_entrada, '.')));
    strcat(arquivo_saida, ".cng");

    FILE *arquivo = fopen(arquivo_entrada, "r");
    if (!arquivo) {
        perror("Erro ao abrir o arquivo de entrada");
        return 1;
    }

    // Determinar o número máximo de vértices
    int num_vertices = 0, u, v;
    while (fscanf(arquivo, "%d %d", &u, &v) != EOF) {
        if (u > num_vertices) num_vertices = u;
        if (v > num_vertices) num_vertices = v;
    }
    num_vertices++;
    rewind(arquivo);

    // Inicializar listas de adjacência
    int **adjacencia = inicializar_lista_adjacencia(num_vertices);
    int *graus = (int *)calloc(num_vertices, sizeof(int));

    // Ler arestas do arquivo
    while (fscanf(arquivo, "%d %d", &u, &v) != EOF) {
        adicionar_vizinho(adjacencia, graus, u, v);
        adicionar_vizinho(adjacencia, graus, v, u);
    }
    fclose(arquivo);

    // Buffers para resultados por thread
    int **buffer_resultados = (int **)malloc(num_threads * sizeof(int *));
    int *tamanho_buffer = (int *)calloc(num_threads, sizeof(int));

    omp_set_num_threads(num_threads);
    double *tempos_threads = (double *)calloc(num_threads, sizeof(double));

    // Parte paralela: cálculo de vizinhos comuns
    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        int capacidade_inicial = 100000;  // Capacidade inicial do buffer local
        buffer_resultados[thread_id] = (int *)malloc(3 * capacidade_inicial * sizeof(int));
        int tamanho_local = 0;

        double inicio_tempo_thread = omp_get_wtime();

        #pragma omp for schedule(dynamic, 100)
        for (int i = 0; i < num_vertices; i++) {
            for (int j = i + 1; j < num_vertices; j++) {
                int comuns = calcular_vizinhos_comuns(adjacencia[i], graus[i], adjacencia[j], graus[j]);
                if (comuns > 0) {
                    if (tamanho_local >= capacidade_inicial) {
                        capacidade_inicial *= 2;
                        buffer_resultados[thread_id] = realloc(buffer_resultados[thread_id], 3 * capacidade_inicial * sizeof(int));
                    }
                    buffer_resultados[thread_id][tamanho_local * 3] = i;
                    buffer_resultados[thread_id][tamanho_local * 3 + 1] = j;
                    buffer_resultados[thread_id][tamanho_local * 3 + 2] = comuns;
                    tamanho_local++;
                }
            }
        }

        tempos_threads[thread_id] = omp_get_wtime() - inicio_tempo_thread;
        tamanho_buffer[thread_id] = tamanho_local;
    }

    double tempo_total_inicio = omp_get_wtime();

    // Parte sequencial: Escrever os resultados no arquivo
    FILE *saida = fopen(arquivo_saida, "w");
    for (int t = 0; t < num_threads; t++) {
        for (int j = 0; j < tamanho_buffer[t]; j++) {
            fprintf(saida, "%d %d %d\n", buffer_resultados[t][j * 3], buffer_resultados[t][j * 3 + 1], buffer_resultados[t][j * 3 + 2]);
        }
        free(buffer_resultados[t]);
    }
    fclose(saida);

    // Liberar memória
    free(buffer_resultados);
    free(tamanho_buffer);
    for (int i = 0; i < num_vertices; i++) free(adjacencia[i]);
    free(adjacencia);
    free(graus);

    // Calcular e exibir tempos
    double tempo_paralelo_total = 0.0;
    double tempo_paralelo_max = 0.0;
    for (int i = 0; i < num_threads; i++) {
        if (tempos_threads[i] > tempo_paralelo_max) {
            tempo_paralelo_max = tempos_threads[i];
        }
        printf("Tempo da thread %d: %.6f segundos\n", i, tempos_threads[i]);
    }

    double tempo_total_fim = omp_get_wtime();
    double tempo_sequencial_total = tempo_total_fim - tempo_total_inicio;
    printf("Tempo total da parte paralela (mais lenta): %.6f segundos\n", tempo_paralelo_max);
    printf("Tempo total da parte sequencial: %.6f segundos\n", tempo_sequencial_total);
    printf("Tempo total de execução: %.6f segundos\n", tempo_paralelo_max + tempo_sequencial_total);

    free(tempos_threads);

    printf("Arquivo gerado com sucesso: %s\n", arquivo_saida);

    // Criar arquivo CSV para gráficos
    FILE *csv_saida = fopen("resultados.csv", "w");
    fprintf(csv_saida, "Thread,Tempo (segundos)\n");
    for (int i = 0; i < num_threads; i++) {
        fprintf(csv_saida, "%d,%.6f\n", i, tempos_threads[i]);
    }
    fprintf(csv_saida, "Tempo total da parte paralela,%.6f\n", tempo_paralelo_max);
    fprintf(csv_saida, "Tempo total da parte sequencial,%.6f\n", tempo_sequencial_total);
    fprintf(csv_saida, "Tempo total de execução,%.6f\n", tempo_paralelo_max + tempo_sequencial_total);
    fclose(csv_saida);

    return 0;
}
