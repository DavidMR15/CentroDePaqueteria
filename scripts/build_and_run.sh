#!/bin/bash
# build_and_run.sh 

set -e


cd "$(dirname "$0")/.."
SRC=src
BIN=bin
LOGS=results/logs

mkdir -p $BIN $LOGS

echo "=============================================="
echo "   Centro de Paquetería – Compilando...   "
echo "=============================================="


compile() {
    local name=$1 flags=$2
    echo "Compilando $name..."
    gcc -Wall -Wextra -O2 -o $BIN/$name $SRC/${name}.c $flags
    echo "  -> Listo: $BIN/$name"
}


compile modulo1_productor_consumidor "-lpthread -lrt"
compile modulo2_condicion_carrera "-lpthread"
compile modulo3_metricas "-lpthread -lm"

echo -e "\nCompilacion exitosa.\n"


run_module() {
    local num=$1 name=$2
    echo "== Modulo $num =============================="
    $BIN/$name 2>&1 | tee $LOGS/modulo${num}.log
    echo ""
}


run_module 1 modulo1_productor_consumidor
run_module 2 modulo2_condicion_carrera
run_module 3 modulo3_metricas

echo "Todos los modulos ejecutados."
echo "Logs guardados en $LOGS/"
