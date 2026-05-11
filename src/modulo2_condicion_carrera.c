/*
 * MÓDULO 2: Condición de Carrera – Antes y Después de Sincronización
 * Centro de Paquetería - Sistemas Operativos
 * UDLAP - Primavera 2026
 *
 * Descripción:
 *   Demuestra qué ocurre cuando múltiples hilos acceden a un contador
 *   compartido SIN protección (condición de carrera) y CON mutex.
 *   El experimento se corre dos veces y se comparan los resultados.
 *
 * Compilar: gcc -o modulo2 modulo2_condicion_carrera.c -lpthread
 * Ejecutar: ./modulo2
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

/* ─── Configuración ─────────────────────────────────── */
#define NUM_HILOS       8       /* trabajadores concurrentes */
#define INCREMENTOS  50000      /* cada hilo incrementa N veces */
#define ESPERADO    (NUM_HILOS * INCREMENTOS)

/* ─── Colores ANSI ────────────────────────────────────── */
#define RESET    "\033[0m"
#define ROJO     "\033[31m"
#define VERDE    "\033[32m"
#define AMARILLO "\033[33m"
#define CYAN     "\033[36m"
#define BLANCO   "\033[37m"
#define NEGRITA  "\033[1m"

/* ══════════════════════════════════════════════════════
 *  EXPERIMENTO A – SIN protección (condición de carrera)
 * ══════════════════════════════════════════════════════ */
static long contador_inseguro = 0;

void *trabajador_inseguro(void *arg) {
    (void)arg;
    for (int i = 0; i < INCREMENTOS; i++) {
        /*
         * SECCIÓN CRÍTICA SIN PROTECCIÓN:
         * La operación contador++ no es atómica. Se descompone en:
         *   1) leer  contador  → registro
         *   2) sumar 1 al registro
         *   3) escribir registro → contador
         * Si dos hilos leen el mismo valor antes de que el otro escriba,
         * se pierde un incremento → condición de carrera.
         */
        contador_inseguro++;
    }
    return NULL;
}

long experimento_sin_mutex(void) {
    contador_inseguro = 0;
    pthread_t hilos[NUM_HILOS];

    for (int i = 0; i < NUM_HILOS; i++)
        pthread_create(&hilos[i], NULL, trabajador_inseguro, NULL);
    for (int i = 0; i < NUM_HILOS; i++)
        pthread_join(hilos[i], NULL);

    return contador_inseguro;
}

/* ══════════════════════════════════════════════════════
 *  EXPERIMENTO B – CON mutex (sincronizado)
 * ══════════════════════════════════════════════════════ */
static long contador_seguro = 0;
static pthread_mutex_t mutex_contador = PTHREAD_MUTEX_INITIALIZER;

void *trabajador_seguro(void *arg) {
    (void)arg;
    for (int i = 0; i < INCREMENTOS; i++) {
        pthread_mutex_lock(&mutex_contador);
        contador_seguro++;          /* sección crítica protegida */
        pthread_mutex_unlock(&mutex_contador);
    }
    return NULL;
}

long experimento_con_mutex(void) {
    contador_seguro = 0;
    pthread_t hilos[NUM_HILOS];

    for (int i = 0; i < NUM_HILOS; i++)
        pthread_create(&hilos[i], NULL, trabajador_seguro, NULL);
    for (int i = 0; i < NUM_HILOS; i++)
        pthread_join(hilos[i], NULL);

    return contador_seguro;
}

/* ══════════════════════════════════════════════════════
 *  MAIN – Correr ambos experimentos varias veces
 * ══════════════════════════════════════════════════════ */
int main(void) {
    printf(BLANCO NEGRITA
           "╔══════════════════════════════════════════════════════════╗\n"
           "║   MÓDULO 2 – Condición de Carrera: Antes / Después      ║\n"
           "║   Hilos: %d  │  Incrementos/hilo: %d  │  Esperado: %d  ║\n"
           "╚══════════════════════════════════════════════════════════╝\n\n"
           RESET,
           NUM_HILOS, INCREMENTOS, ESPERADO);

    const int RONDAS = 5;
    long total_perdido = 0;

    /* ── Tabla de resultados ── */
    printf("%-6s %-18s %-18s %-12s %-10s\n",
           "Ronda", "Sin mutex", "Con mutex", "Pérdida", "Correcto?");
    printf("──────────────────────────────────────────────────────────\n");

    for (int r = 1; r <= RONDAS; r++) {
        /* Experimento SIN mutex */
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        long sin_mx = experimento_sin_mutex();
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double ms_sin = (t1.tv_sec - t0.tv_sec)*1e3 +
                        (t1.tv_nsec - t0.tv_nsec)/1e6;

        /* Experimento CON mutex */
        clock_gettime(CLOCK_MONOTONIC, &t0);
        long con_mx = experimento_con_mutex();
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double ms_con = (t1.tv_sec - t0.tv_sec)*1e3 +
                        (t1.tv_nsec - t0.tv_nsec)/1e6;

        long perdido = ESPERADO - sin_mx;
        total_perdido += perdido;

        printf("%-6d " ROJO "%-18ld" RESET " " VERDE "%-18ld" RESET
               " " AMARILLO "%-12ld" RESET " %s\n",
               r, sin_mx, con_mx, perdido,
               (con_mx == ESPERADO) ? VERDE "✓ Sí" RESET : ROJO "✗ No" RESET);

        printf("       (%.1f ms)           (%.1f ms)\n", ms_sin, ms_con);
    }

    printf("──────────────────────────────────────────────────────────\n");
    printf(NEGRITA "Pérdida promedio por ronda: %ld incrementos\n" RESET,
           total_perdido / RONDAS);

    /* ── Análisis ── */
    printf(CYAN "\n[ANÁLISIS]\n" RESET);
    printf("  • Sin mutex: los hilos leen y escriben el contador de forma\n");
    printf("    intercalada. El valor final es MENOR al esperado porque\n");
    printf("    múltiples hilos leen el mismo valor antes de que otro lo\n");
    printf("    escriba (condición de carrera).\n\n");
    printf("  • Con mutex: solo un hilo modifica el contador a la vez.\n");
    printf("    El resultado siempre es exactamente %d.\n\n", ESPERADO);
    printf("  • Costo de sincronización: el mutex introduce overhead (ms\n");
    printf("    adicionales) pero garantiza la corrección del resultado.\n");

    pthread_mutex_destroy(&mutex_contador);
    return 0;
}
