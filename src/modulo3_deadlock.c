/*
 * MÓDULO 3: Detección y Prevención de Deadlock
 * Centro de Paquetería - Sistemas Operativos
 * UDLAP - Primavera 2026
 *
 * Descripción:
 *   Demuestra un deadlock clásico entre dos hilos que adquieren
 *   dos mutex en orden inverso, luego muestra cómo prevenirlo
 *   imponiendo un orden global de adquisición.
 *
 *   También implementa un watchdog que detecta si el sistema
 *   lleva más de TIMEOUT segundos sin avanzar.
 *
 * Compilar: gcc -o modulo3 modulo3_deadlock.c -lpthread
 * Ejecutar: ./modulo3
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdatomic.h>

/* ─── Colores ANSI ────────────────────────────────────── */
#define RESET    "\033[0m"
#define ROJO     "\033[31m"
#define VERDE    "\033[32m"
#define AMARILLO "\033[33m"
#define CYAN     "\033[36m"
#define BLANCO   "\033[37m"
#define NEGRITA  "\033[1m"

/* ─── Recursos compartidos (como si fueran escáneres/básculas) ── */
static pthread_mutex_t recurso_A;   /* p.ej. Báscula        */
static pthread_mutex_t recurso_B;   /* p.ej. Escáner QR     */

/* ─── Contador de paquetes procesados (para watchdog) ─────────── */
static atomic_int paquetes_ok = 0;
static atomic_int deadlock_detectado = 0;

#define TIMEOUT_SEG 2    /* segundos antes de declarar deadlock */

/* ══════════════════════════════════════════════════════
 *  EXPERIMENTO A – DEADLOCK INTENCIONAL
 *  Hilo 1: adquiere A → luego B
 *  Hilo 2: adquiere B → luego A
 *  → Espera circular garantizada.
 * ══════════════════════════════════════════════════════ */
void *hilo_deadlock_1(void *arg) {
    (void)arg;
    printf(ROJO "  [Hilo-1] Adquiriendo Báscula (A)...\n" RESET);
    pthread_mutex_lock(&recurso_A);
    printf(ROJO "  [Hilo-1] Báscula (A) adquirida. Esperando Escáner (B)...\n" RESET);
    sleep(1);   /* dar tiempo al hilo 2 de tomar B */
    pthread_mutex_lock(&recurso_B);   /* ← bloqueado aquí para siempre */
    printf(ROJO "  [Hilo-1] Escáner (B) adquirido. Procesando...\n" RESET);
    pthread_mutex_unlock(&recurso_B);
    pthread_mutex_unlock(&recurso_A);
    return NULL;
}

void *hilo_deadlock_2(void *arg) {
    (void)arg;
    printf(ROJO "  [Hilo-2] Adquiriendo Escáner (B)...\n" RESET);
    pthread_mutex_lock(&recurso_B);
    printf(ROJO "  [Hilo-2] Escáner (B) adquirido. Esperando Báscula (A)...\n" RESET);
    sleep(1);
    pthread_mutex_lock(&recurso_A);   /* ← bloqueado aquí para siempre */
    printf(ROJO "  [Hilo-2] Báscula (A) adquirida. Procesando...\n" RESET);
    pthread_mutex_unlock(&recurso_A);
    pthread_mutex_unlock(&recurso_B);
    return NULL;
}

/* ══════════════════════════════════════════════════════
 *  WATCHDOG – detecta si el sistema se congela
 * ══════════════════════════════════════════════════════ */
void *watchdog(void *arg) {
    (void)arg;
    int ultimo = atomic_load(&paquetes_ok);
    sleep(TIMEOUT_SEG);
    int actual = atomic_load(&paquetes_ok);

    if (actual == ultimo) {
        atomic_store(&deadlock_detectado, 1);
        printf(AMARILLO "\n  ⚠  WATCHDOG: Sin progreso por %d segundos.\n"
               "      → Posible DEADLOCK detectado.\n" RESET, TIMEOUT_SEG);
    }
    return NULL;
}

/* ══════════════════════════════════════════════════════
 *  EXPERIMENTO B – PREVENCIÓN (orden global de locks)
 *  Ambos hilos adquieren siempre A → B.
 *  No puede haber espera circular.
 * ══════════════════════════════════════════════════════ */
void *hilo_seguro_1(void *arg) {
    (void)arg;
    /* Orden siempre: A primero, luego B */
    printf(VERDE "  [Hilo-1] Adquiriendo Báscula (A)...\n" RESET);
    pthread_mutex_lock(&recurso_A);
    printf(VERDE "  [Hilo-1] Adquiriendo Escáner (B)...\n" RESET);
    pthread_mutex_lock(&recurso_B);

    printf(VERDE "  [Hilo-1] Ambos recursos. Procesando paquete.\n" RESET);
    usleep(300000);
    atomic_fetch_add(&paquetes_ok, 1);

    pthread_mutex_unlock(&recurso_B);
    pthread_mutex_unlock(&recurso_A);
    printf(VERDE "  [Hilo-1] Recursos liberados.\n" RESET);
    return NULL;
}

void *hilo_seguro_2(void *arg) {
    (void)arg;
    /* Mismo orden: A primero, luego B */
    printf(VERDE "  [Hilo-2] Adquiriendo Báscula (A)...\n" RESET);
    pthread_mutex_lock(&recurso_A);
    printf(VERDE "  [Hilo-2] Adquiriendo Escáner (B)...\n" RESET);
    pthread_mutex_lock(&recurso_B);

    printf(VERDE "  [Hilo-2] Ambos recursos. Procesando paquete.\n" RESET);
    usleep(300000);
    atomic_fetch_add(&paquetes_ok, 1);

    pthread_mutex_unlock(&recurso_B);
    pthread_mutex_unlock(&recurso_A);
    printf(VERDE "  [Hilo-2] Recursos liberados.\n" RESET);
    return NULL;
}

/* ══════════════════════════════════════════════════════
 *  MAIN
 * ══════════════════════════════════════════════════════ */
int main(void) {
    printf(BLANCO NEGRITA
           "╔══════════════════════════════════════════════════╗\n"
           "║   MÓDULO 3 – Detección y Prevención de Deadlock ║\n"
           "╚══════════════════════════════════════════════════╝\n\n"
           RESET);

    pthread_mutex_init(&recurso_A, NULL);
    pthread_mutex_init(&recurso_B, NULL);

    /* ── PARTE 1: demostrar deadlock ── */
    printf(NEGRITA "── PARTE 1: DEADLOCK INTENCIONAL ──────────────────\n" RESET);
    printf("  Hilo-1 toma A→B  │  Hilo-2 toma B→A  (orden inverso)\n\n");

    pthread_t wd, h1, h2;
    pthread_create(&wd, NULL, watchdog, NULL);
    pthread_create(&h1, NULL, hilo_deadlock_1, NULL);
    pthread_create(&h2, NULL, hilo_deadlock_2, NULL);

    pthread_join(wd, NULL);

    if (atomic_load(&deadlock_detectado)) {
        /* Cancelar hilos bloqueados */
        pthread_cancel(h1);
        pthread_cancel(h2);
        pthread_join(h1, NULL);
        pthread_join(h2, NULL);
        printf(ROJO "  → Hilos cancelados por el watchdog.\n\n" RESET);
    } else {
        pthread_join(h1, NULL);
        pthread_join(h2, NULL);
    }

    /* Reiniciar mutexes (los hilos cancelados los dejaron tomados) */
    pthread_mutex_destroy(&recurso_A);
    pthread_mutex_destroy(&recurso_B);
    pthread_mutex_init(&recurso_A, NULL);
    pthread_mutex_init(&recurso_B, NULL);
    atomic_store(&paquetes_ok, 0);

    /* ── PARTE 2: prevención con orden global ── */
    printf(NEGRITA "── PARTE 2: PREVENCIÓN (orden global A → B) ───────\n" RESET);
    printf("  Ambos hilos adquieren siempre A primero, luego B.\n\n");

    pthread_create(&h1, NULL, hilo_seguro_1, NULL);
    pthread_create(&h2, NULL, hilo_seguro_2, NULL);
    pthread_join(h1, NULL);
    pthread_join(h2, NULL);

    printf(VERDE "\n  ✓ Paquetes procesados sin deadlock: %d\n" RESET,
           atomic_load(&paquetes_ok));

    /* ── Análisis ── */
    printf(CYAN "\n[ANÁLISIS]\n" RESET);
    printf("  Deadlock ocurre cuando:\n");
    printf("    1) Exclusión mutua  – recursos no compartibles.\n");
    printf("    2) Hold & wait      – hilo retiene recurso y espera otro.\n");
    printf("    3) No apropiación   – no se le quita el recurso al hilo.\n");
    printf("    4) Espera circular  – ciclo de dependencias entre hilos.\n\n");
    printf("  Prevención: imponer orden global rompe la condición 4.\n");
    printf("  Si todos adquieren A antes de B, nunca habrá ciclo.\n");

    pthread_mutex_destroy(&recurso_A);
    pthread_mutex_destroy(&recurso_B);
    return 0;
}
