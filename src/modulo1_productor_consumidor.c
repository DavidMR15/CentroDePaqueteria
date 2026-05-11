/*
 * MÓDULO 1: Productor-Consumidor con Semáforos y Mutex
 * Centro de Paquetería - Sistemas Operativos
 * UDLAP - Primavera 2026
 *
 * Descripción:
 *   Simula un centro de distribución donde un hilo productor genera paquetes
 *   y múltiples hilos consumidores (trabajadores) los procesan.
 *   Se usan semáforos POSIX y mutex para sincronización.
 *
 * Compilar: gcc -o modulo1 modulo1_productor_consumidor.c -lpthread -lrt
 * Ejecutar: ./modulo1
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

/* ─── Configuración ─────────────────────────────────── */
#define BUFFER_SIZE      10      /* capacidad máxima del buffer (cola) */
#define NUM_CONSUMIDORES  3      /* número de trabajadores */
#define TOTAL_PAQUETES   20      /* paquetes que generará el productor */

/* ─── Estructura de un paquete ───────────────────────── */
typedef struct {
    int    id;
    char   destino[32];
    float  peso_kg;
    char   estado[16];   /* "EN_COLA", "PROCESANDO", "ENTREGADO" */
    time_t timestamp;
} Paquete;

/* ─── Recursos compartidos ────────────────────────────── */
static Paquete cola[BUFFER_SIZE];
static int     head = 0;          /* índice de extracción */
static int     tail = 0;          /* índice de inserción  */
static int     count = 0;         /* paquetes en la cola  */

/* ─── Primitivas de sincronización ───────────────────── */
static pthread_mutex_t mutex_cola;   /* exclusión mutua sobre la cola    */
static sem_t           sem_vacios;   /* espacios disponibles en el buffer */
static sem_t           sem_llenos;   /* paquetes disponibles para consumir */

/* ─── Contadores globales (para métricas) ─────────────── */
static int paquetes_producidos = 0;
static int paquetes_consumidos = 0;



/* ─── Función auxiliar: timestamp legible ─────────────── */
static void timestamp(char *buf, size_t n) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, n, "%H:%M:%S", tm);
}

/* ══════════════════════════════════════════════════════
 *  PRODUCTOR
 *  Genera TOTAL_PAQUETES paquetes y los inserta en la cola.
 * ══════════════════════════════════════════════════════ */
void *productor(void *arg) {
    (void)arg;
    char ts[16];
    const char *destinos[] = {
        "Puebla", "CDMX", "Guadalajara", "Monterrey", "Oaxaca"
    };

    for (int i = 1; i <= TOTAL_PAQUETES; i++) {
        /* ── Preparar paquete antes de entrar a sección crítica ── */
        Paquete p;
        p.id        = i;
        p.peso_kg   = 0.5f + (rand() % 200) / 10.0f;
        p.timestamp = time(NULL);
        strncpy(p.destino, destinos[rand() % 5], sizeof(p.destino) - 1);
        strncpy(p.estado, "EN_COLA", sizeof(p.estado) - 1);

        /* ── Esperar espacio disponible en el buffer ── */
        sem_wait(&sem_vacios);

        /* ── Sección crítica: insertar en la cola ── */
        pthread_mutex_lock(&mutex_cola);

        cola[tail] = p;
        tail = (tail + 1) % BUFFER_SIZE;
        count++;
        paquetes_producidos++;

        timestamp(ts, sizeof(ts));
        printf("[%s] PRODUCTOR  → Paquete #%02d | Destino: %-12s | Peso: %.1f kg | Cola: %d/%d\n",
               ts, p.id, p.destino, p.peso_kg, count, BUFFER_SIZE);

        pthread_mutex_unlock(&mutex_cola);

        /* ── Señalar que hay un paquete disponible ── */
        sem_post(&sem_llenos);

        /* Simula tiempo entre llegadas de paquetes */
        usleep((100 + rand() % 300) * 1000);
    }

    printf("\n[PRODUCTOR] Todos los paquetes han sido generados.\n");

    /*
     * FIX: Enviar NUM_CONSUMIDORES señales "fantasma" a sem_llenos.
     *
     * Sin esto ocurre deadlock: los consumidores quedan bloqueados para
     * siempre en sem_wait(&sem_llenos) porque el semáforo ya llegó a 0
     * y nadie vuelve a hacer sem_post.  Con estas señales cada consumidor
     * despierta, entra a la sección crítica, detecta que
     * paquetes_consumidos >= TOTAL_PAQUETES y sale del loop.
     * El propio sem_post(&sem_llenos) dentro del if propaga la señal
     * al siguiente consumidor en cadena.
     */
    for (int i = 0; i < NUM_CONSUMIDORES; i++)
        sem_post(&sem_llenos);

    return NULL;
}

/* ══════════════════════════════════════════════════════
 *  CONSUMIDOR (TRABAJADOR)
 *  Extrae paquetes de la cola y los "procesa".
 * ══════════════════════════════════════════════════════ */
void *consumidor(void *arg) {
    int id_trabajador = *((int *)arg);
    char ts[16];

    while (1) {
        /* ── Esperar paquete disponible ── */
        sem_wait(&sem_llenos);

        /* ── Sección crítica: extraer de la cola ── */
        pthread_mutex_lock(&mutex_cola);

        /* Condición de parada: productor terminó y cola vacía */
        if (paquetes_consumidos >= TOTAL_PAQUETES) {
            pthread_mutex_unlock(&mutex_cola);
            sem_post(&sem_llenos);   /* propagar señal al siguiente consumidor */
            break;
        }

        Paquete p = cola[head];
        head = (head + 1) % BUFFER_SIZE;
        count--;
        paquetes_consumidos++;
        strncpy(p.estado, "PROCESANDO", sizeof(p.estado) - 1);

        timestamp(ts, sizeof(ts));
        printf("[%s] TRABAJADOR %d → Procesando #%02d | Destino: %-12s | Cola restante: %d\n",
               ts, id_trabajador, p.id, p.destino, count);

        pthread_mutex_unlock(&mutex_cola);

        /* ── Señalar que se liberó un espacio ── */
        sem_post(&sem_vacios);

        /* Simula tiempo de procesamiento */
        usleep((200 + rand() % 500) * 1000);

        timestamp(ts, sizeof(ts));
        printf("[%s] TRABAJADOR %d ✓ Entregado  #%02d | Destino: %-12s\n",
               ts, id_trabajador, p.id, p.destino);
    }

    printf("[TRABAJADOR %d] Turno terminado.\n", id_trabajador);
    return NULL;
}

/* ══════════════════════════════════════════════════════
 *  MAIN
 * ══════════════════════════════════════════════════════ */
int main(void) {
    srand((unsigned)time(NULL));

    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║   MÓDULO 1 – Productor-Consumidor (mutex+sem)    ║\n");
    printf("║   Buffer: %d  │  Trabajadores: %d  │  Paquetes: %d  ║\n",
           BUFFER_SIZE, NUM_CONSUMIDORES, TOTAL_PAQUETES);
    printf("╚══════════════════════════════════════════════════╝\n\n");

    /* ── Inicializar primitivas de sincronización ── */
    pthread_mutex_init(&mutex_cola, NULL);
    sem_init(&sem_vacios, 0, BUFFER_SIZE);   /* buffer vacío al inicio */
    sem_init(&sem_llenos, 0, 0);             /* sin paquetes al inicio */

    /* ── Crear hilos ── */
    pthread_t hilo_productor;
    pthread_t hilos_consumidor[NUM_CONSUMIDORES];
    int ids[NUM_CONSUMIDORES];

    pthread_create(&hilo_productor, NULL, productor, NULL);

    for (int i = 0; i < NUM_CONSUMIDORES; i++) {
        ids[i] = i + 1;
        pthread_create(&hilos_consumidor[i], NULL, consumidor, &ids[i]);
    }

    /* ── Esperar a que todos terminen ── */
    pthread_join(hilo_productor, NULL);
    for (int i = 0; i < NUM_CONSUMIDORES; i++)
        pthread_join(hilos_consumidor[i], NULL);

    /* ── Resumen ── */
    printf("\n╔══════════════════════════════════════════════════╗\n");
    printf("║                    RESUMEN FINAL                ║\n");
    printf("╠══════════════════════════════════════════════════╣\n");
    printf("║  Paquetes producidos : %-26d║\n", paquetes_producidos);
    printf("║  Paquetes consumidos : %-26d║\n", paquetes_consumidos);
    printf("║  Paquetes en cola    : %-26d║\n", count);
    printf("╚══════════════════════════════════════════════════╝\n");

    /* ── Limpiar recursos ── */
    pthread_mutex_destroy(&mutex_cola);
    sem_destroy(&sem_vacios);
    sem_destroy(&sem_llenos);

    return 0;
}
