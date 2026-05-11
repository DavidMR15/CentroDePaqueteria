//MÓDULO 1: Productor-Consumidor con Semáforos y Mutex

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

//paquete
typedef struct {
    int    id;
    char   destino[32];
    float  peso_kg;
    char   estado[16];
    time_t timestamp;
} Paquete;

//Parametros por default
static int BUFFER_SIZE      = 10;
static int NUM_CONSUMIDORES =  3;
static int TOTAL_PAQUETES   = 20;

//Recursos a proteger
static Paquete *cola  = NULL;
static int      head  = 0;
static int      tail  = 0;
static int      count = 0;

//Sincronización
static pthread_mutex_t mutex_cola;
static sem_t           sem_vacios;
static sem_t           sem_llenos;

//Contadores
static int paquetes_producidos = 0;
static int paquetes_consumidos = 0;

static void timestamp(char *buf, size_t n) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, n, "%H:%M:%S", tm);
}

//productor
void *productor(void *arg) {
    (void)arg;
    char ts[16];
    const char *destinos[] = {
        "Puebla", "CDMX", "Guadalajara", "Monterrey", "Oaxaca"
    };

    for (int i = 1; i <= TOTAL_PAQUETES; i++) {
        Paquete p;
        p.id      = i;
        p.peso_kg = 0.5f + (rand() % 200) / 10.0f;
        p.timestamp = time(NULL);
        strncpy(p.destino, destinos[rand() % 5], sizeof(p.destino) - 1);
        p.destino[sizeof(p.destino) - 1] = '\0';
        strncpy(p.estado, "EN_COLA", sizeof(p.estado) - 1);
        p.estado[sizeof(p.estado) - 1] = '\0';

        sem_wait(&sem_vacios);            
        pthread_mutex_lock(&mutex_cola); 

        cola[tail] = p;
        tail = (tail + 1) % BUFFER_SIZE;
        count++;
        paquetes_producidos++;

        timestamp(ts, sizeof(ts));
        printf("[%s] PRODUCTOR  Paquete #%d | Destino: %s | "
               "Peso: %.1f kg | Cola: %d/%d\n",
               ts, p.id, p.destino, p.peso_kg, count, BUFFER_SIZE);

        pthread_mutex_unlock(&mutex_cola); 
        sem_post(&sem_llenos);            

        usleep((100 + rand() % 300) * 1000);
    }

    printf("\n[PRODUCTOR] Todos los paquetes han sido generados.\n");

    for (int i = 0; i < NUM_CONSUMIDORES; i++)
        sem_post(&sem_llenos);

    return NULL;
}

// Consumidor
void *consumidor(void *arg) {
    int id_trabajador = *((int *)arg);
    char ts[16];

    while (1) {
        sem_wait(&sem_llenos);        
        pthread_mutex_lock(&mutex_cola);

        if (paquetes_consumidos >= TOTAL_PAQUETES) {
            pthread_mutex_unlock(&mutex_cola);
            sem_post(&sem_llenos);
            break;
        }

        Paquete p = cola[head];
        head = (head + 1) % BUFFER_SIZE;
        count--;
        paquetes_consumidos++;
        strncpy(p.estado, "PROCESANDO", sizeof(p.estado) - 1);
        p.estado[sizeof(p.estado) - 1] = '\0';

        timestamp(ts, sizeof(ts));
        printf("[%s] TRABAJADOR %d Procesando #%02d | Destino: %-12s | "
               "Cola restante: %d\n",
               ts, id_trabajador, p.id, p.destino, count);

        pthread_mutex_unlock(&mutex_cola);
        sem_post(&sem_vacios);        

        usleep((200 + rand() % 500) * 1000);

        timestamp(ts, sizeof(ts));
        printf("[%s] TRABAJADOR %d Entregado  #%02d | Destino: %-12s\n",
               ts, id_trabajador, p.id, p.destino);
    }

    printf("[TRABAJADOR %d] Turno terminado.\n", id_trabajador);
    return NULL;
}

int main(int argc, char *argv[]) {

    if (argc == 4) {
        int b = atoi(argv[1]);
        int w = atoi(argv[2]);
        int p = atoi(argv[3]);

        if (b < 1 || w < 1 || p < 1) {
            fprintf(stderr, "Error: los tres argumentos deben ser enteros positivos.\n");
            fprintf(stderr, "Uso:     %s <buffer> <trabajadores> <paquetes>\n", argv[0]);
            fprintf(stderr, "Ejemplo: %s 10 4 60\n", argv[0]);
            return 1;
        }
        //porque 64 PREGUNTA
        if (w > 64) {
            fprintf(stderr, "Error: número máximo de trabajadores: 64.\n");
            return 1;
        }

        BUFFER_SIZE      = b;
        NUM_CONSUMIDORES = w;
        TOTAL_PAQUETES   = p;

    } else if (argc != 1) {
        fprintf(stderr, "Uso:     %s <buffer> <trabajadores> <paquetes>\n", argv[0]);
        fprintf(stderr, "         %s  (sin argumentos usa valores por defecto: 10 3 20)\n", argv[0]);
        fprintf(stderr, "Ejemplo: %s 10 4 60\n", argv[0]);
        return 1;
    }
    srand((unsigned)time(NULL));

    cola = malloc((size_t)BUFFER_SIZE * sizeof(Paquete));
    if (!cola) {
        fprintf(stderr, "Error: no se pudo reservar memoria para la cola.\n");
        return 1;
    }


    printf("MÓDULO 1 Productor-Consumidor\n");
    printf("Buffer      : %d\n", BUFFER_SIZE);
    printf("Trabajadores: %d\n", NUM_CONSUMIDORES);
    printf("Paquetes    : %d\n", TOTAL_PAQUETES);

    pthread_mutex_init(&mutex_cola, NULL);
    sem_init(&sem_vacios, 0, (unsigned)BUFFER_SIZE);
    sem_init(&sem_llenos, 0, 0);

    pthread_t  hilo_productor;
    pthread_t *hilos_consumidor = malloc((size_t)NUM_CONSUMIDORES * sizeof(pthread_t));
    int       *ids              = malloc((size_t)NUM_CONSUMIDORES * sizeof(int));

    if (!hilos_consumidor || !ids) {
        fprintf(stderr, "Error: memoria insuficiente para los hilos.\n");
        free(cola);
        return 1;
    }

    pthread_create(&hilo_productor, NULL, productor, NULL);
    for (int i = 0; i < NUM_CONSUMIDORES; i++) {
        ids[i] = i + 1;
        pthread_create(&hilos_consumidor[i], NULL, consumidor, &ids[i]);
    }

    pthread_join(hilo_productor, NULL);
    for (int i = 0; i < NUM_CONSUMIDORES; i++)
        pthread_join(hilos_consumidor[i], NULL);

    printf("Paquetes producidos : %d\n", paquetes_producidos);
    printf("Paquetes consumidos : %d\n", paquetes_consumidos);
    printf("Paquetes en cola    : %d\n", count);

    pthread_mutex_destroy(&mutex_cola);
    sem_destroy(&sem_vacios);
    sem_destroy(&sem_llenos);
    free(cola);
    free(hilos_consumidor);
    free(ids);

    return 0;
}