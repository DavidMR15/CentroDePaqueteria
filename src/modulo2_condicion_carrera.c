// Modulo 2 - Condicion de Carrera 

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_TRABAJADORES  8
#define DESPACHOS_POR_W   10
#define ESPERADO          (NUM_TRABAJADORES * DESPACHOS_POR_W)

// Sin mutex 
static long paquetes_despachados_sm = 0;

void *trabajador_sm(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < DESPACHOS_POR_W; i++) {

        long local = paquetes_despachados_sm; 
        usleep(1);                            
        paquetes_despachados_sm = local + 1;  

        long registro_actual = paquetes_despachados_sm;
        int  colision        = (registro_actual != local + 1);

        if (colision)
            printf("  [TRABAJADOR %d] COLISION   | "
                   "Leyo: %ld | Esperaba escribir: %ld | "
                   "Total real: %ld  <-- paquete PERDIDO\n",
                   id, local, local + 1, registro_actual);
        else
            printf("  [TRABAJADOR %d] Despachado | "
                   "Leyo: %ld | Escribio: %ld | "
                   "Total ahora: %ld\n",
                   id, local, local + 1, registro_actual);
    }
    return NULL;
}

long experimento_sin_mutex(void) {
    paquetes_despachados_sm = 0;
    pthread_t hilos[NUM_TRABAJADORES];
    int       ids[NUM_TRABAJADORES];
    for (int i = 0; i < NUM_TRABAJADORES; i++) {
        ids[i] = i + 1;
        pthread_create(&hilos[i], NULL, trabajador_sm, &ids[i]);
    }
    for (int i = 0; i < NUM_TRABAJADORES; i++)
        pthread_join(hilos[i], NULL);
    return paquetes_despachados_sm;
}

// Con mutex 
static long            paquetes_despachados_cm = 0;
static pthread_mutex_t mutex_despacho = PTHREAD_MUTEX_INITIALIZER;

void *trabajador_cm(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < DESPACHOS_POR_W; i++) {
        pthread_mutex_lock(&mutex_despacho);

        long local = paquetes_despachados_cm; 
        usleep(1);
        paquetes_despachados_cm = local + 1;  

        long registro_actual = paquetes_despachados_cm;
        pthread_mutex_unlock(&mutex_despacho);

        printf("  [TRABAJADOR %d] Despachado | "
               "Leyo: %2ld | Escribio: %2ld | "
               "Total ahora: %2ld\n",
               id, local, local + 1, registro_actual);
    }
    return NULL;
}

long experimento_con_mutex(void) {
    paquetes_despachados_cm = 0;
    pthread_t hilos[NUM_TRABAJADORES];
    int       ids[NUM_TRABAJADORES];
    for (int i = 0; i < NUM_TRABAJADORES; i++) {
        ids[i] = i + 1;
        pthread_create(&hilos[i], NULL, trabajador_cm, &ids[i]);
    }
    for (int i = 0; i < NUM_TRABAJADORES; i++)
        pthread_join(hilos[i], NULL);
    return paquetes_despachados_cm;
}

// Main 
int main(void) {
    printf("Modulo 2 - Sistema de Paqueteria: Condicion de Carrera \n");
    printf("Trabajadores: %d | Despachos/trabajador: %d | Esperado: %d\n\n",
           NUM_TRABAJADORES, DESPACHOS_POR_W, ESPERADO);

    long sin_mx[2], con_mx[2];

    for (int r = 0; r < 2; r++) {
        printf("\n");
        printf(" Ronda %d  SIN mutex  (registro sin proteccion)\n", r + 1);
        printf("\n");
        sin_mx[r] = experimento_sin_mutex();

        long perdidos = ESPERADO - sin_mx[r];
        printf("\n Paquetes registrados: %ld / %d", sin_mx[r], ESPERADO);
        if (perdidos > 0)
            printf("%ld paquetes perdidos \n\n", perdidos);
        else
            printf("  (carrera no visible esta ronda)\n\n");

        printf("\n");
        printf(" Ronda %d  CON mutex  (registro protegido)\n", r + 1);
        printf("\n");
        con_mx[r] = experimento_con_mutex();
        printf("\n Paquetes registrados: %ld / %d  OK\n\n", con_mx[r], ESPERADO);
    }

    printf("\n");
    printf("| Ronda |   Sin mutex    |  Con mutex\n");
    printf("\n");
    for (int r = 0; r < 2; r++) {
        long perdidos = ESPERADO - sin_mx[r];
        printf("|   %d  | %2ld / %d  %-15s| %2ld / %d  OK              ║\n",
               r + 1,
               sin_mx[r], ESPERADO, perdidos > 0 ? "Perdidos" : "(sin perdida)",
               con_mx[r], ESPERADO);
    }
    printf("\n");

    pthread_mutex_destroy(&mutex_despacho);
    return 0;
}
