#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

static pthread_mutex_t recurso_A;
static pthread_mutex_t recurso_B;

/* DEADLOCK: Hilo-1 toma A→B, Hilo-2 toma B→A */
void *hilo_1(void *arg) {
    (void)arg;
    printf("[Hilo-1] Tomando Bascula (A)...\n");
    pthread_mutex_lock(&recurso_A);
    printf("[Hilo-1] Bascula (A) tomada. Esperando Escaner (B)...\n");
    sleep(1);
    printf("[Hilo-1] Intentando tomar Escaner (B)... <-- BLOQUEADO\n");
    pthread_mutex_lock(&recurso_B);   /* nunca llega aqui */
    pthread_mutex_unlock(&recurso_B);
    pthread_mutex_unlock(&recurso_A);
    return NULL;
}

void *hilo_2(void *arg) {
    (void)arg;
    printf("[Hilo-2] Tomando Escaner (B)...\n");
    pthread_mutex_lock(&recurso_B);
    printf("[Hilo-2] Escaner (B) tomado. Esperando Bascula (A)...\n");
    sleep(1);
    printf("[Hilo-2] Intentando tomar Bascula (A)... <-- BLOQUEADO\n");
    pthread_mutex_lock(&recurso_A);   /* nunca llega aqui */
    pthread_mutex_unlock(&recurso_A);
    pthread_mutex_unlock(&recurso_B);
    return NULL;
}

/* PREVENCION: ambos toman siempre A→B */
void *hilo_seguro_1(void *arg) {
    (void)arg;
    printf("[Hilo-1] Tomando Bascula (A)...\n");
    pthread_mutex_lock(&recurso_A);
    printf("[Hilo-1] Tomando Escaner (B)...\n");
    pthread_mutex_lock(&recurso_B);
    printf("[Hilo-1] Procesando paquete.\n");
    usleep(300000);
    pthread_mutex_unlock(&recurso_B);
    pthread_mutex_unlock(&recurso_A);
    printf("[Hilo-1] Listo.\n");
    return NULL;
}

void *hilo_seguro_2(void *arg) {
    (void)arg;
    printf("[Hilo-2] Tomando Bascula (A)...\n");
    pthread_mutex_lock(&recurso_A);
    printf("[Hilo-2] Tomando Escaner (B)...\n");
    pthread_mutex_lock(&recurso_B);
    printf("[Hilo-2] Procesando paquete.\n");
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

    /* --- PARTE 1: Deadlock --- */
    printf("=== PARTE 1: DEADLOCK (orden inverso) ===\n");
    printf("Hilo-1: A -> B  |  Hilo-2: B -> A\n\n");

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    pthread_create(&h1, NULL, hilo_1, NULL);
    pthread_create(&h2, NULL, hilo_2, NULL);

    sleep(3);
    printf("\n*** El programa lleva 3 segundos congelado: DEADLOCK ***\n");
    printf("*** Cancelando hilos...\n\n");
    pthread_cancel(h1);
    pthread_cancel(h2);
    pthread_join(h1, NULL);
    pthread_join(h2, NULL);

    pthread_mutex_destroy(&recurso_A);
    pthread_mutex_destroy(&recurso_B);
    pthread_mutex_init(&recurso_A, NULL);
    pthread_mutex_init(&recurso_B, NULL);

    /* --- PARTE 2: Prevencion --- */
    printf("=== PARTE 2: PREVENCION (orden global A->B) ===\n");
    printf("Ambos hilos toman siempre A primero, luego B.\n\n");

    pthread_create(&h1, NULL, hilo_seguro_1, NULL);
    pthread_create(&h2, NULL, hilo_seguro_2, NULL);
    pthread_join(h1, NULL);
    pthread_join(h2, NULL);

    printf("\nAmbos paquetes procesados sin deadlock.\n");

    pthread_mutex_destroy(&recurso_A);
    pthread_mutex_destroy(&recurso_B);
    return 0;
}
