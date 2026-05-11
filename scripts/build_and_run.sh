#!/bin/bash
# =============================================================
# build_and_run.sh – Compilar y ejecutar todos los módulos
# Centro de Paquetería – Sistemas Operativos UDLAP
# =============================================================
set -e

RESET="\033[0m"; VERDE="\033[32m"; CYAN="\033[36m"; AMARILLO="\033[33m"
NEGRITA="\033[1m"

cd "$(dirname "$0")/.."          # posicionarse en raíz del proyecto
SRC=src
BIN=bin
LOGS=results/logs

mkdir -p $BIN $LOGS

echo -e "${NEGRITA}${CYAN}╔════════════════════════════════════════════╗"
echo -e "║   Centro de Paquetería – Compilando...     ║"
echo -e "╚════════════════════════════════════════════╝${RESET}\n"

# ── Compilar ──────────────────────────────────────────────
compile() {
    local name=$1 flags=$2
    echo -e "${AMARILLO}Compilando $name...${RESET}"
    gcc -Wall -Wextra -O2 -o $BIN/$name $SRC/${name}.c $flags
    echo -e "${VERDE}  ✓ $BIN/$name${RESET}"
}

compile modulo1_productor_consumidor "-lpthread -lrt"
compile modulo2_condicion_carrera    "-lpthread"
compile modulo3_deadlock             "-lpthread"
compile modulo4_metricas             "-lpthread -lm"

echo -e "\n${VERDE}${NEGRITA}Compilación exitosa.${RESET}\n"

# ── Ejecutar módulos y guardar logs ───────────────────────
run_module() {
    local num=$1 name=$2
    echo -e "${CYAN}${NEGRITA}══ Módulo $num ══════════════════════════════${RESET}"
    $BIN/$name 2>&1 | tee $LOGS/modulo${num}.log
    echo ""
}

run_module 1 modulo1_productor_consumidor
run_module 2 modulo2_condicion_carrera
run_module 3 modulo3_deadlock
run_module 4 modulo4_metricas

echo -e "${VERDE}${NEGRITA}Todos los módulos ejecutados."
echo -e "Logs guardados en $LOGS/${RESET}"
