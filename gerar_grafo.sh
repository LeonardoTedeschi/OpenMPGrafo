#!/bin/bash

# Função para gerar grafo
gerar_grafo() {
    local arquivo=$1
    local num_vertices=$2
    local num_arestas=$3
    local max_vertices=$((num_vertices - 1))

    # Gerando as arestas
    for ((i = 0; i < num_arestas; i++)); do
        while true; do
            # Escolher dois vértices aleatórios
            v1=$((RANDOM % num_vertices))
            v2=$((RANDOM % num_vertices))

            # Evitar laços (vértice igual a ele mesmo) e duplicação de aresta
            if [[ $v1 -ne $v2 ]]; then
                echo "$v1 $v2" >> $arquivo
                break
            fi
        done
    done
}

# Verifica se os parâmetros foram passados
if [ $# -ne 3 ]; then
    echo "Uso: $0 <arquivo_saida> <num_vertices> <num_arestas>"
    exit 1
fi

# Chama a função para gerar o grafo
gerar_grafo $1 $2 $3
echo "Grafo gerado no arquivo $1"
