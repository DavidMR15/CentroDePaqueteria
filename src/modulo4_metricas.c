//MÓDULO 4: Métricas de CPU – Turnaround, Espera y Throughput

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>

//Constantes
#define TOTAL_PAQUETES  20      
#define PROCESO_MS      30      
#define LLEGADA_MS      20      

typedef struct {
    int             id;
    struct timespec t_llegada;
    struct timespec t_inicio_proc;
    struct timespec t_fin_proc;
} PaqueteMetrica;

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


typedef struct {
    int    workers;
    int    buf_size;
    double turnaround_prom_ms;
    double espera_prom_ms;
    double throughput_pps;
    double duracion_total_ms;
} Resultado;

static double diff_ms(struct timespec *ini, struct timespec *fin) {
    return (fin->tv_sec  - ini->tv_sec)  * 1e3 +
           (fin->tv_nsec - ini->tv_nsec) / 1e6;
}

//Productor
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

    for (int i = 0; i < ctx->num_workers; i++)
        sem_post(&ctx->sem_llenos);

    return NULL;
}

//Consumidor
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

        PaqueteMetrica paq = ctx->cola[ctx->head];
        clock_gettime(CLOCK_MONOTONIC, &paq.t_inicio_proc);
        ctx->head = (ctx->head + 1) % ctx->buffer_size;
        ctx->count--;
        ctx->consumidos++;

        pthread_mutex_unlock(&ctx->mutex);
        sem_post(&ctx->sem_vacios);

        usleep(PROCESO_MS * 1000);
        struct timespec t_fin;
        clock_gettime(CLOCK_MONOTONIC, &t_fin);

        pthread_mutex_lock(&ctx->mutex);
        int slot = (paq.id - 1) % ctx->buffer_size;
        ctx->cola[slot].t_llegada     = paq.t_llegada;
        ctx->cola[slot].t_inicio_proc = paq.t_inicio_proc;
        ctx->cola[slot].t_fin_proc    = t_fin;
        pthread_mutex_unlock(&ctx->mutex);
    }

    return NULL;
}

Resultado ejecutar_experimento(int workers, int buf_size) {
    ExperimentoCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.buffer_size = buf_size;
    ctx.num_workers = workers;
    ctx.cola = calloc((size_t)buf_size, sizeof(PaqueteMetrica));

    pthread_mutex_init(&ctx.mutex, NULL);
    sem_init(&ctx.sem_vacios, 0, (unsigned)buf_size);
    sem_init(&ctx.sem_llenos, 0, 0);

    struct timespec t_ini, t_fin;
    clock_gettime(CLOCK_MONOTONIC, &t_ini);

    pthread_t  hprod;
    pthread_t *hcons = malloc((size_t)workers * sizeof(pthread_t));

    pthread_create(&hprod, NULL, prod_metrica, &ctx);
    for (int i = 0; i < workers; i++)
        pthread_create(&hcons[i], NULL, cons_metrica, &ctx);

    pthread_join(hprod, NULL);
    for (int i = 0; i < workers; i++)
        pthread_join(hcons[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &t_fin);

    double sum_ta = 0, sum_esp = 0;
    int validos = 0;

    for (int i = 0; i < buf_size; i++) {
        PaqueteMetrica *p = &ctx.cola[i];
        if (p->t_fin_proc.tv_sec == 0) continue;
        sum_ta  += diff_ms(&p->t_llegada, &p->t_fin_proc);
        sum_esp += diff_ms(&p->t_llegada, &p->t_inicio_proc);
        validos++;
    }

    double dur_ms = diff_ms(&t_ini, &t_fin);

    Resultado r;
    r.workers             = workers;
    r.buf_size            = buf_size;
    r.turnaround_prom_ms  = validos > 0 ? sum_ta  / validos : 0;
    r.espera_prom_ms      = validos > 0 ? sum_esp / validos : 0;
    r.duracion_total_ms   = dur_ms;
    r.throughput_pps      = dur_ms > 0 ? TOTAL_PAQUETES / (dur_ms / 1000.0) : 0;

    free(ctx.cola);
    free(hcons);
    pthread_mutex_destroy(&ctx.mutex);
    sem_destroy(&ctx.sem_vacios);
    sem_destroy(&ctx.sem_llenos);

    return r;
}

int main(void) {
    printf("MÓDULO 4 - Métricas de CPU: Turnaround, Espera, Throughput\n");
    printf("Paquetes: %d  |  Proceso: %d ms  |  Llegada: %d ms\n", TOTAL_PAQUETES, PROCESO_MS, LLEGADA_MS);

    int workers_cfg[] = {1, 2, 4, 8};
    int buffer_cfg[]  = {5, 10};
    int nw = 4, nb = 2;

    printf("%s %-8s %s %s %s %s\n", "Workers", "Buffer", "Turnaround(ms)", "Espera(ms)", "Throughput(p/s)", "Total(ms)");

    for (int b = 0; b < nb; b++) {
        for (int w = 0; w < nw; w++) {
            int workers  = workers_cfg[w];
            int buf_size = buffer_cfg[b];

            printf("  [Ejecutando workers=%-2d buffer=%-3d ...]\n",
                   workers, buf_size);
            fflush(stdout);

            Resultado r = ejecutar_experimento(workers, buf_size);

            printf("%-10d %-8d %-18.1f %-18.1f %-16.2f %-10.0f\n",
                   workers, buf_size,
                   r.turnaround_prom_ms,
                   r.espera_prom_ms,
                   r.throughput_pps,
                   r.duracion_total_ms);
        }
    }
    return 0;
}