#!/bin/bash

# Defina os gráficos e o número de threads para cada teste
testes=(
    "grafo1.edgelist 2"
    "grafo2.edgelist 4"
    "grafo3.edgelist 6"
    "grafo4.edgelist 8"
)

# Função para rodar os testes
executar_teste() {
    local grafo=$1
    local threads=$2
    echo "Executando teste com grafo $grafo e $threads threads..."
    ./OpenMPGrafo $grafo $threads
}

# Testes com diferentes configurações de grafos e threads
for teste in "${testes[@]}"; do
    # Separar os parâmetros
    grafo=$(echo $teste | cut -d ' ' -f 1)
    threads=$(echo $teste | cut -d ' ' -f 2)
    
    # Executar o teste
    executar_teste $grafo $threads
    echo "---------------------------"
done

# Teste de comparação com diferentes números de threads para o grafo2.edgelist
for threads in 2 4 8; do
    echo "Executando comparação com $threads threads..."
    ./OpenMPGrafo grafo2.edgelist $threads
    echo "---------------------------"
done
