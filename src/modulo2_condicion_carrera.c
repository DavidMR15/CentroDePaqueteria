/*
 * Modulo 2 - Condicion de Carrera
 * Sistemas Operativos - UDLAP
 * Primavera 2026
 *
 * Compilar: gcc -o modulo2 modulo2_condicion_carrera.c -lpthread
 * Ejecutar: ./modulo2
 */

#include <stdio.h>
#include <pthread.h>

#define NUM_HILOS    8
#define INCREMENTOS  50000
#define ESPERADO     (NUM_HILOS * INCREMENTOS)

// --- Sin mutex ---

static long contador_inseguro = 0;

void *trabajador_inseguro(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < INCREMENTOS; i++) {
        long antes = contador_inseguro;
        contador_inseguro++;
        long despues = contador_inseguro;

        // mostrar solo algunos para no saturar la terminal
        if (i % 10000 == 0) {
            printf("  [hilo %d] leyo: %ld | escribio: %ld | contador ahora: %ld\n",
                   id, antes, antes + 1, despues);
        }
    }
    return NULL;
}

long experimento_sin_mutex(void) {
    contador_inseguro = 0;
    pthread_t hilos[NUM_HILOS];
    int ids[NUM_HILOS];

    for (int i = 0; i < NUM_HILOS; i++) {
        ids[i] = i;
        pthread_create(&hilos[i], NULL, trabajador_inseguro, &ids[i]);
    }
    for (int i = 0; i < NUM_HILOS; i++)
        pthread_join(hilos[i], NULL);

    return contador_inseguro;
}

// --- Con mutex ---

static long contador_seguro = 0;
static pthread_mutex_t mutex_contador = PTHREAD_MUTEX_INITIALIZER;

void *trabajador_seguro(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < INCREMENTOS; i++) {
        pthread_mutex_lock(&mutex_contador);
        long antes = contador_seguro;
        contador_seguro++;
        long despues = contador_seguro;

        if (i % 10000 == 0) {
            printf("  [hilo %d] leyo: %ld | escribio: %ld | contador ahora: %ld\n",
                   id, antes, antes + 1, despues);
        }
        pthread_mutex_unlock(&mutex_contador);
    }
    return NULL;
}

long experimento_con_mutex(void) {
    contador_seguro = 0;
    pthread_t hilos[NUM_HILOS];
    int ids[NUM_HILOS];

    for (int i = 0; i < NUM_HILOS; i++) {
        ids[i] = i;
        pthread_create(&hilos[i], NULL, trabajador_seguro, &ids[i]);
    }
    for (int i = 0; i < NUM_HILOS; i++)
        pthread_join(hilos[i], NULL);

    return contador_seguro;
}

// --- Main ---

int main(void) {
    printf("Modulo 2 - Condicion de Carrera: Antes / Despues\n");
    printf("Hilos: %d | Incrementos/hilo: %d | Esperado: %d\n\n",
           NUM_HILOS, INCREMENTOS, ESPERADO);

    printf("%-6s %-18s %-18s\n", "Ronda", "Sin mutex", "Con mutex");
    printf("------------------------------------------\n");

    for (int r = 1; r <= 2; r++) {
        printf("\n>>> Ronda %d - SIN mutex:\n", r);
        long sin_mx = experimento_sin_mutex();
        printf("    Resultado sin mutex: %ld (esperado: %d)\n", sin_mx, ESPERADO);

        printf("\n>>> Ronda %d - CON mutex:\n", r);
        long con_mx = experimento_con_mutex();
        printf("    Resultado con mutex: %ld (esperado: %d)\n", con_mx, ESPERADO);

        printf("\n%-6d %-18ld %-18ld\n", r, sin_mx, con_mx);
        printf("------------------------------------------\n");
    }

    pthread_mutex_destroy(&mutex_contador);
    return 0;
}
