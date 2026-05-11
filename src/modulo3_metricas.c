// Módulo 3

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define TOTAL_PAQUETES  20      
#define PROCESO_MS      30      
#define LLEGADA_MS      20      

typedef struct {
    int             id;
    struct timespec t_llegada;
    struct timespec t_inicio_proc;
    struct timespec t_fin_proc;
} PaqueteMetrica;


PaqueteMetrica historial[TOTAL_PAQUETES];

typedef struct {
    int              buffer_size;
    int              num_workers;
    PaqueteMetrica  *cola;
    int              head, tail;
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
    return (double)(fin->tv_sec - ini->tv_sec) * 1000.0 +
           (double)(fin->tv_nsec - ini->tv_nsec) / 1000000.0;
}

void *prod_metrica(void *arg) {
    ExperimentoCtx *ctx = (ExperimentoCtx *)arg;

    for (int i = 0; i < TOTAL_PAQUETES; i++) {
        sem_wait(&ctx->sem_vacios);
        pthread_mutex_lock(&ctx->mutex);

        PaqueteMetrica p;
        p.id = i; 
        clock_gettime(CLOCK_MONOTONIC, &p.t_llegada);

        ctx->cola[ctx->tail] = p;
        ctx->tail = (ctx->tail + 1) % ctx->buffer_size;
        ctx->producidos++;

        pthread_mutex_unlock(&ctx->mutex);
        sem_post(&ctx->sem_llenos);

        usleep(LLEGADA_MS * 1000);
    }

   
    for (int i = 0; i < ctx->num_workers; i++)
        sem_post(&ctx->sem_llenos);

    return NULL;
}

void *cons_metrica(void *arg) {
    ExperimentoCtx *ctx = (ExperimentoCtx *)arg;

    while (1) {
        sem_wait(&ctx->sem_llenos);
        pthread_mutex_lock(&ctx->mutex);

        if (ctx->consumidos >= TOTAL_PAQUETES) {
            pthread_mutex_unlock(&ctx->mutex);
            break;
        }

       
        PaqueteMetrica paq = ctx->cola[ctx->head];
        ctx->head = (ctx->head + 1) % ctx->buffer_size;
        ctx->consumidos++;
        
        clock_gettime(CLOCK_MONOTONIC, &paq.t_inicio_proc);
        pthread_mutex_unlock(&ctx->mutex);
        sem_post(&ctx->sem_vacios);

       
        usleep(PROCESO_MS * 1000);
        
        struct timespec t_fin;
        clock_gettime(CLOCK_MONOTONIC, &t_fin);

       
        historial[paq.id].t_llegada = paq.t_llegada;
        historial[paq.id].t_inicio_proc = paq.t_inicio_proc;
        historial[paq.id].t_fin_proc = t_fin;
    }
    return NULL;
}

Resultado ejecutar_experimento(int workers, int buf_size) {
    ExperimentoCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    memset(historial, 0, sizeof(historial));
    
    ctx.buffer_size = buf_size;
    ctx.num_workers = workers;
    ctx.cola = calloc((size_t)buf_size, sizeof(PaqueteMetrica));

    pthread_mutex_init(&ctx.mutex, NULL);
    sem_init(&ctx.sem_vacios, 0, (unsigned)buf_size);
    sem_init(&ctx.sem_llenos, 0, 0);

    struct timespec t_ini, t_fin;
    clock_gettime(CLOCK_MONOTONIC, &t_ini);

    pthread_t hprod, *hcons = malloc((size_t)workers * sizeof(pthread_t));

    pthread_create(&hprod, NULL, prod_metrica, &ctx);
    for (int i = 0; i < workers; i++)
        pthread_create(&hcons[i], NULL, cons_metrica, &ctx);

    pthread_join(hprod, NULL);
    for (int i = 0; i < workers; i++)
        pthread_join(hcons[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &t_fin);

    
    double sum_ta = 0, sum_esp = 0;
    for (int i = 0; i < TOTAL_PAQUETES; i++) {
        sum_ta  += diff_ms(&historial[i].t_llegada, &historial[i].t_fin_proc);
        sum_esp += diff_ms(&historial[i].t_llegada, &historial[i].t_inicio_proc);
    }

    double dur_ms = diff_ms(&t_ini, &t_fin);

    Resultado r;
    r.workers = workers;
    r.buf_size = buf_size;
    r.turnaround_prom_ms = sum_ta / TOTAL_PAQUETES;
    r.espera_prom_ms = sum_esp / TOTAL_PAQUETES;
    r.duracion_total_ms = dur_ms;
    r.throughput_pps = TOTAL_PAQUETES / (dur_ms / 1000.0);

    free(ctx.cola);
    free(hcons);
    pthread_mutex_destroy(&ctx.mutex);
    sem_destroy(&ctx.sem_vacios);
    sem_destroy(&ctx.sem_llenos);

    return r;
}

int main(void) {
    printf("MÓDULO 3 - Métricas de CPU corregidas\n");
    printf("Paquetes: %d | Proceso: %d ms | Llegada: %d ms\n\n", 
           TOTAL_PAQUETES, PROCESO_MS, LLEGADA_MS);

    int workers_cfg[] = {1, 2, 4, 8};
    int buffer_cfg[]  = {5, 10};

    printf("%-8s %-8s %-15s %-15s %-15s %-10s\n", 
           "Workers", "Buffer", "Turnaround(ms)", "Espera(ms)", "Throughput", "Total(ms)");
    printf("\n");

    for (int b = 0; b < 2; b++) {
        for (int w = 0; w < 4; w++) {
            Resultado r = ejecutar_experimento(workers_cfg[w], buffer_cfg[b]);
            printf("%-8d %-8d %-15.1f %-15.1f %-15.2f %-10.0f\n",
                   r.workers, r.buf_size, r.turnaround_prom_ms, 
                   r.espera_prom_ms, r.throughput_pps, r.duracion_total_ms);
        }
        printf("\n");
    }
    return 0;
}

