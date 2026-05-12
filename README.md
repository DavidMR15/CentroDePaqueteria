# Centro de Paquetería – Proyecto Final Sistemas Operativos

David Miguel Medina Raymundo (183112) · María Fernanda Morales Hernández (183604)

---

## Descripción

Simulación de un centro de distribución de paquetes que demuestra cuatro
fenómenos clave de Sistemas Operativos mediante hilos en C.

---

## Dependencias

```
gcc
libpthread   (incluida en glibc)
libm         (incluida en glibc)
librt        (incluida en glibc)
```

En Ubuntu/Debian:

```bash
sudo apt update && sudo apt install -y build-essential
```

---

## Estructura del repositorio

```
project/
├── src/
│   ├── modulo1_productor_consumidor.c
│   ├── modulo2_condicion_carrera.c
│   └── modulo3_metricas.c
├── scripts/
│   └── build_and_run.sh
├── results/
│   ├── logs/         
│   ├── tables/       
│   └── screenshots/  
└── README.md
```

---

## Compilación e instrucciones de ejecución

### Opción A – Script automático (recomendado)

```bash
chmod +x scripts/build_and_run.sh
./scripts/build_and_run.sh
```

Compila todos los módulos y los ejecuta en secuencia.
Los logs quedan en `results/logs/moduloN.log`.

### Opción B – Compilar y ejecutar por módulo

```bash
mkdir -p bin

# Módulo 1 – Productor-Consumidor
gcc -Wall -O2 -o bin/modulo1 src/modulo1_productor_consumidor.c -lpthread -lrt
./bin/modulo1

# Módulo 2 – Condición de carrera
gcc -Wall -O2 -o bin/modulo2 src/modulo2_condicion_carrera.c -lpthread
./bin/modulo2

# Módulo 3 – Métricas de CPU
gcc -Wall -O2 -o bin/modulo3 src/modulo3_metricas.c -lpthread -lm
./bin/modulo4
```

---

## Módulos implementados

| # | Módulo | Concepto de SO |
|---|--------|----------------|
| 1 | Productor-Consumidor | Hilos, semáforos, mutex, buffer circular |
| 2 | Condición de carrera | Race condition, exclusión mutua |
| 3 | Métricas de CPU | Turnaround, tiempo de espera, throughput |

---

## Resultados esperados

### Módulo 1

- El productor inserta exactamente `TOTAL_PAQUETES` paquetes.
- Los trabajadores los consumen sin pérdidas ni duplicados.
- El resumen final muestra `producidos == consumidos`.

### Módulo 2

- Sin mutex: el contador final es **menor** que `NUM_HILOS × INCREMENTOS`.
- Con mutex: el contador final es **exactamente** `NUM_HILOS × INCREMENTOS`.

### Módulo 3

Tabla de throughput esperada (aproximada):

| Workers | Buffer | Turnaround (ms) | Espera (ms) | Throughput (p/s) |
|---------|--------|-----------------|-------------|------------------|
| 1       | 5      | ~350            | ~250        | ~3.0             |
| 2       | 5      | ~250            | ~150        | ~5.0             |
| 4       | 5      | ~180            | ~80         | ~7.5             |
| 8       | 5      | ~160            | ~60         | ~8.5             |
| 1       | 10     | ~350            | ~250        | ~3.0             |
| 4       | 10     | ~170            | ~70         | ~8.0             |


