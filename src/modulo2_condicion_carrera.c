// Modulo 2 - Condicion de Carrera

#include <stdio.h>
#include <pthread.h>

#define NUM_HILOS    8
#define INCREMENTOS  10
#define ESPERADO     (NUM_HILOS * INCREMENTOS)

// Sin mutex 

static long contador_sm = 0;

void *trabajador_sm(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < INCREMENTOS; i++) {
        long antes = contador_sm;
        contador_sm++;
        long despues = contador_sm;
        printf("  [hilo %d] leyo: %ld | escribio: %ld | contador ahora: %ld\n",
               id, antes, antes + 1, despues);
    }
    return NULL;
}

long experimento_sin_mutex(void) {
    contador_sm = 0;
    pthread_t hilos[NUM_HILOS];
    int ids[NUM_HILOS];

    for (int i = 0; i < NUM_HILOS; i++) {
        ids[i] = i;
        pthread_create(&hilos[i], NULL, trabajador_sm, &ids[i]);
    }
    for (int i = 0; i < NUM_HILOS; i++)
        pthread_join(hilos[i], NULL);

    return contador_sm;
}

// Con mutex 

static long contador_cm = 0;
static pthread_mutex_t mutex_contador = PTHREAD_MUTEX_INITIALIZER;

void *trabajador_cm(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < INCREMENTOS; i++) {
        pthread_mutex_lock(&mutex_contador);
        long antes = contador_cm;
        contador_cm++;
        long despues = contador_cm;
        printf("  [hilo %d] leyo: %ld | escribio: %ld | contador ahora: %ld\n",
               id, antes, antes + 1, despues);
        pthread_mutex_unlock(&mutex_contador);
    }
    return NULL;
}

long experimento_con_mutex(void) {
    contador_cm = 0;
    pthread_t hilos[NUM_HILOS];
    int ids[NUM_HILOS];

    for (int i = 0; i < NUM_HILOS; i++) {
        ids[i] = i;
        pthread_create(&hilos[i], NULL, trabajador_cm, &ids[i]);
    }
    for (int i = 0; i < NUM_HILOS; i++)
        pthread_join(hilos[i], NULL);

    return contador_cm;
}

// Main 

int main(void) {
    printf("Modulo 2 - Condicion de Carrera: Antes / Despues\n");
    printf("Hilos: %d | Incrementos/hilo: %d | Esperado: %d\n\n",
           NUM_HILOS, INCREMENTOS, ESPERADO);

    printf("%s %s %s\n", "Ronda", "Sin mutex", "Con mutex");
    printf("\n");

    for (int r = 1; r <= 2; r++) {
        printf("\n Ronda %d - SIN mutex:\n", r);
        long sin_mx = experimento_sin_mutex();
        printf("    Resultado sin mutex: %ld (esperado: %d)\n", sin_mx, ESPERADO);

        printf("\n Ronda %d - CON mutex:\n", r);
        long con_mx = experimento_con_mutex();
        printf("    Resultado con mutex: %ld (esperado: %d)\n", con_mx, ESPERADO);

        printf("\n%d %ld %ld\n", r, sin_mx, con_mx);
        printf("\n");
    }

    pthread_mutex_destroy(&mutex_contador);
    return 0;
}
