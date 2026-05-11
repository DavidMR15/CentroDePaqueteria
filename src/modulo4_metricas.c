/*
 * MÓDULO 4: Métricas de CPU – Turnaround, Espera y Throughput
 * Centro de Paquetería - Sistemas Operativos
 * UDLAP - Primavera 2026
 *
 * Descripción:
 *   Mide el rendimiento del sistema de paquetería bajo distintas
 *   condiciones: número de trabajadores (1, 2, 4, 8) y tamaño
 *   de buffer (2, 5, 10). Reporta:
 *     - Tiempo de turnaround promedio (llegada → entrega)
 *     - Tiempo de espera promedio (en cola)
 *     - Throughput (paquetes/segundo)
 *
 * Compilar: gcc -o modulo4 modulo4_metricas.c -lpthread -lm
 * Ejecutar: ./modulo4
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* ─── Configuración del experimento ──────────────────── */
#define TOTAL_PAQUETES  30      /* paquetes por experimento */
#define PROCESO_MS     100      /* ms que tarda procesar un paquete */
#define LLEGADA_MS      80      /* ms entre llegadas de paquetes    */

/* ─── Colores ANSI ────────────────────────────────────── */
#define RESET    "\033[0m"
#define ROJO     "\033[31m"
#define VERDE    "\033[32m"
#define AMARILLO "\033[33m"
#define CYAN     "\033[36m"
#define BLANCO   "\033[37m"
#define NEGRITA  "\033[1m"

/* ─── Estructura de paquete con métricas ──────────────── */
typedef struct {
    int    id;
    struct timespec t_llegada;     /* cuando se insertó en la cola  */
    struct timespec t_inicio_proc; /* cuando un trabajador lo tomó  */
    struct timespec t_fin_proc;    /* cuando terminó de procesarse  */
} PaqueteMetrica;

/* ─── Estado compartido del experimento ───────────────── */
typedef struct {
    int              buffer_size;
    int              num_workers;
    PaqueteMetrica  *cola;
    int              head, tail, count;
    int              producidos, consumidos;
    pthread_mutex_t  mutex;
    sem_t            sem_vacios;
    sem_t            sem_llenos;
} ExperimentoCtx;

/* ─── Resultados de un experimento ───────────────────── */
typedef struct {
    int    workers;
    int    buf_size;
    double turnaround_prom_ms;
    double espera_prom_ms;
    double throughput_pps;   /* paquetes por segundo */
    double duracion_total_ms;
} Resultado;

/* ── Diferencia en ms entre dos timespec ── */
static double diff_ms(struct timespec *ini, struct timespec *fin) {
    return (fin->tv_sec - ini->tv_sec) * 1e3 +
           (fin->tv_nsec - ini->tv_nsec) / 1e6;
}

/* ══════════════════════════════════════════════════════
 *  HILO PRODUCTOR
 * ══════════════════════════════════════════════════════ */
void *prod_metrica(void *arg) {
    ExperimentoCtx *ctx = (ExperimentoCtx *)arg;

    for (int i = 0; i < TOTAL_PAQUETES; i++) {
        sem_wait(&ctx->sem_vacios);
        pthread_mutex_lock(&ctx->mutex);

        PaqueteMetrica p;
        p.id = i + 1;
        clock_gettime(CLOCK_MONOTONIC, &p.t_llegada);
        memset(&p.t_inicio_proc, 0, sizeof(p.t_inicio_proc));
        memset(&p.t_fin_proc,    0, sizeof(p.t_fin_proc));

        ctx->cola[ctx->tail] = p;
        ctx->tail = (ctx->tail + 1) % ctx->buffer_size;
        ctx->count++;
        ctx->producidos++;

        pthread_mutex_unlock(&ctx->mutex);
        sem_post(&ctx->sem_llenos);

        usleep(LLEGADA_MS * 1000);
    }
    return NULL;
}

/* ══════════════════════════════════════════════════════
 *  HILO CONSUMIDOR
 * ══════════════════════════════════════════════════════ */
void *cons_metrica(void *arg) {
    ExperimentoCtx *ctx = (ExperimentoCtx *)arg;

    while (1) {
        sem_wait(&ctx->sem_llenos);
        pthread_mutex_lock(&ctx->mutex);

        if (ctx->consumidos >= TOTAL_PAQUETES) {
            pthread_mutex_unlock(&ctx->mutex);
            sem_post(&ctx->sem_llenos);
            break;
        }

        PaqueteMetrica *p = &ctx->cola[ctx->head];
        clock_gettime(CLOCK_MONOTONIC, &p->t_inicio_proc);
        ctx->head  = (ctx->head + 1) % ctx->buffer_size;
        ctx->count--;
        ctx->consumidos++;

        /* Guardamos índice para registrar fin después */
        int idx = (ctx->head + ctx->buffer_size - 1) % ctx->buffer_size;

        pthread_mutex_unlock(&ctx->mutex);
        sem_post(&ctx->sem_vacios);

        /* Simular procesamiento */
        usleep(PROCESO_MS * 1000);

        pthread_mutex_lock(&ctx->mutex);
        clock_gettime(CLOCK_MONOTONIC, &ctx->cola[idx].t_fin_proc);
        pthread_mutex_unlock(&ctx->mutex);
    }
    return NULL;
}

/* ══════════════════════════════════════════════════════
 *  EJECUTAR UN EXPERIMENTO
 * ══════════════════════════════════════════════════════ */
Resultado ejecutar_experimento(int workers, int buf_size) {
    ExperimentoCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.buffer_size = buf_size;
    ctx.num_workers = workers;
    ctx.cola = calloc(buf_size, sizeof(PaqueteMetrica));

    pthread_mutex_init(&ctx.mutex, NULL);
    sem_init(&ctx.sem_vacios, 0, buf_size);
    sem_init(&ctx.sem_llenos, 0, 0);

    struct timespec t_global_ini, t_global_fin;
    clock_gettime(CLOCK_MONOTONIC, &t_global_ini);

    /* Lanzar hilos */
    pthread_t hprod;
    pthread_t *hcons = malloc(workers * sizeof(pthread_t));

    pthread_create(&hprod, NULL, prod_metrica, &ctx);
    for (int i = 0; i < workers; i++)
        pthread_create(&hcons[i], NULL, cons_metrica, &ctx);

    pthread_join(hprod, NULL);
    for (int i = 0; i < workers; i++)
        pthread_join(hcons[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &t_global_fin);

    /* ── Calcular métricas ── */
    double sum_turnaround = 0, sum_espera = 0;
    int validos = 0;

    for (int i = 0; i < buf_size; i++) {
        PaqueteMetrica *p = &ctx.cola[i];
        if (p->t_fin_proc.tv_sec == 0) continue;
        sum_turnaround += diff_ms(&p->t_llegada, &p->t_fin_proc);
        sum_espera     += diff_ms(&p->t_llegada, &p->t_inicio_proc);
        validos++;
    }

    double dur_ms = diff_ms(&t_global_ini, &t_global_fin);

    Resultado r;
    r.workers          = workers;
    r.buf_size         = buf_size;
    r.turnaround_prom_ms = (validos > 0) ? sum_turnaround / validos : 0;
    r.espera_prom_ms     = (validos > 0) ? sum_espera     / validos : 0;
    r.duracion_total_ms  = dur_ms;
    r.throughput_pps     = (dur_ms > 0) ? TOTAL_PAQUETES / (dur_ms / 1000.0) : 0;

    free(ctx.cola);
    free(hcons);
    pthread_mutex_destroy(&ctx.mutex);
    sem_destroy(&ctx.sem_vacios);
    sem_destroy(&ctx.sem_llenos);

    return r;
}

/* ══════════════════════════════════════════════════════
 *  MAIN – Matriz de experimentos
 * ══════════════════════════════════════════════════════ */
int main(void) {
    printf(BLANCO NEGRITA
           "╔══════════════════════════════════════════════════════════════╗\n"
           "║   MÓDULO 4 – Métricas de CPU: Turnaround, Espera, Throughput ║\n"
           "║   Paquetes por experimento: %-4d  Proceso: %d ms  Llegada: %d ms ║\n"
           "╚══════════════════════════════════════════════════════════════╝\n\n"
           RESET, TOTAL_PAQUETES, PROCESO_MS, LLEGADA_MS);

    int configuraciones_workers[]  = {1, 2, 4, 8};
    int configuraciones_buffer[]   = {5, 10};
    int nw = 4, nb = 2;

    /* Cabecera de tabla */
    printf(NEGRITA "%-10s %-8s %-18s %-18s %-14s %-10s\n" RESET,
           "Workers", "Buffer", "Turnaround(ms)", "Espera(ms)", "Throughput(p/s)", "Total(ms)");
    printf("────────────────────────────────────────────────────────────────────\n");


    for (int b = 0; b < nb; b++) {
        for (int w = 0; w < nw; w++) {
            int workers  = configuraciones_workers[w];
            int buf_size = configuraciones_buffer[b];

            printf(CYAN "  Ejecutando: workers=%-2d buffer=%-3d ..." RESET "\r",
                   workers, buf_size);
            fflush(stdout);

            Resultado r = ejecutar_experimento(workers, buf_size);

            /* Colorear throughput: verde si > 5 p/s, rojo si < 3 */
            const char *color_tp = (r.throughput_pps >= 5.0) ? VERDE :
                                   (r.throughput_pps <= 3.0) ? ROJO : AMARILLO;

            printf("%-10d %-8d %-18.1f %-18.1f %s%-14.2f%s %-10.0f\n",
                   workers, buf_size,
                   r.turnaround_prom_ms,
                   r.espera_prom_ms,
                   color_tp, r.throughput_pps, RESET,
                   r.duracion_total_ms);
        }
        printf("────────────────────────────────────────────────────────────────────\n");
    }

    /* ── Análisis ── */
    printf(CYAN "\n[ANÁLISIS]\n" RESET);
    printf("  • Turnaround = tiempo total desde llegada del paquete hasta entrega.\n");
    printf("  • Espera     = tiempo que el paquete pasa en cola sin ser procesado.\n");
    printf("  • Throughput = paquetes completados por segundo.\n\n");
    printf("  Observaciones esperadas:\n");
    printf("  1) Más trabajadores → menor turnaround y menor tiempo de espera.\n");
    printf("  2) Con 1 trabajador, los paquetes se acumulan y la espera crece.\n");
    printf("  3) A partir de cierto número de workers el beneficio se satura\n");
    printf("     (el cuello de botella pasa a ser el productor o el buffer).\n");
    printf("  4) Buffer más grande reduce bloqueos del productor.\n");

    return 0;
}
