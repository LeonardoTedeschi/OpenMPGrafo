#!/bin/bash

# Certifique-se de que o executável Treads está disponível
if [ ! -f ./Treads ]; then
    echo "Erro: Arquivo executável 'Treads' não encontrado."
    echo "Compile o programa com: gcc Tread.c -o Treads -lpthread"
    exit 1
fi

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

    # Verifique se o arquivo do grafo existe
    if [ ! -f "$grafo" ]; then
        echo "Erro: Arquivo de grafo '$grafo' não encontrado."
        return
    fi

    echo "Executando teste com grafo $grafo e $threads threads..."

    # Rodar a versão com Treads (sem OpenMP) e capturar o tempo de execução
    ./Treads "$grafo" "$threads"
}

# Testes com diferentes configurações de grafos e threads
for teste in "${testes[@]}"; do
    # Separar os parâmetros
    grafo=$(echo $teste | cut -d ' ' -f 1)
    threads=$(echo $teste | cut -d ' ' -f 2)

    # Executar o teste
    executar_teste "$grafo" "$threads"
    echo "---------------------------"
done

# Teste de comparação com diferentes números de threads para o grafo2.edgelist
for threads in 2 4 8; do
    echo "Executando comparação com $threads threads..."
    executar_teste "grafo2.edgelist" "$threads"
    echo "---------------------------"
done
