#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

static pthread_mutex_t recurso_A;
static pthread_mutex_t recurso_B;

void *hilo_1(void *arg) {
    (void)arg;
    printf("[Hilo-1] Tomando Recurso A...\n");
    pthread_mutex_lock(&recurso_A);
    printf("[Hilo-1] Recurso A tomado. Esperando Recurso B...\n");
    sleep(1);
    printf("[Hilo-1] Intentando tomar Recurso B... <-- BLOQUEADO\n");
    pthread_mutex_lock(&recurso_B);
    pthread_mutex_unlock(&recurso_B);
    pthread_mutex_unlock(&recurso_A);
    return NULL;
}

void *hilo_2(void *arg) {
    (void)arg;
    printf("[Hilo-2] Tomando Recurso B...\n");
    pthread_mutex_lock(&recurso_B);
    printf("[Hilo-2] Recurso B tomado. Esperando Recurso A...\n");
    sleep(1);
    printf("[Hilo-2] Intentando tomar Recurso A... <-- BLOQUEADO\n");
    pthread_mutex_lock(&recurso_A);
    pthread_mutex_unlock(&recurso_A);
    pthread_mutex_unlock(&recurso_B);
    return NULL;
}

void *hilo_seguro_1(void *arg) {
    (void)arg;
    printf("[Hilo-1] Tomando Recurso A...\n");
    pthread_mutex_lock(&recurso_A);
    printf("[Hilo-1] Tomando Recurso B...\n");
    pthread_mutex_lock(&recurso_B);
    printf("[Hilo-1] Procesando.\n");
    usleep(300000);
    pthread_mutex_unlock(&recurso_B);
    pthread_mutex_unlock(&recurso_A);
    printf("[Hilo-1] Listo.\n");
    return NULL;
}

void *hilo_seguro_2(void *arg) {
    (void)arg;
    printf("[Hilo-2] Tomando Recurso A...\n");
    pthread_mutex_lock(&recurso_A);
    printf("[Hilo-2] Tomando Recurso B...\n");
    pthread_mutex_lock(&recurso_B);
    printf("[Hilo-2] Procesando.\n");
    usleep(300000);
    pthread_mutex_unlock(&recurso_B);
    pthread_mutex_unlock(&recurso_A);
    printf("[Hilo-2] Listo.\n");
    return NULL;
}

int main(void) {
    pthread_t h1, h2;

    pthread_mutex_init(&recurso_A, NULL);
    pthread_mutex_init(&recurso_B, NULL);

    printf("Parte 1: Deadlock\n");
    printf("Hilo-1: A->B  |  Hilo-2: B->A\n\n");

    pthread_create(&h1, NULL, hilo_1, NULL);
    pthread_create(&h2, NULL, hilo_2, NULL);

    sleep(3);
    printf("\n3 segundos congelado: DEADLOCK\n");
    printf("Cancelando hilos...\n\n");

    pthread_cancel(h1);
    pthread_cancel(h2);
    pthread_join(h1, NULL);
    pthread_join(h2, NULL);

    pthread_mutex_destroy(&recurso_A);
    pthread_mutex_destroy(&recurso_B);
    pthread_mutex_init(&recurso_A, NULL);
    pthread_mutex_init(&recurso_B, NULL);

    printf("Parte 2: Prevencion (orden global A->B) \n");
    printf("Ambos hilos toman siempre A primero, luego B.\n\n");

    pthread_create(&h1, NULL, hilo_seguro_1, NULL);
    pthread_create(&h2, NULL, hilo_seguro_2, NULL);
    pthread_join(h1, NULL);
    pthread_join(h2, NULL);

    printf("\nAmbos procesados sin deadlock.\n");

    pthread_mutex_destroy(&recurso_A);
    pthread_mutex_destroy(&recurso_B);
    return 0;
}
