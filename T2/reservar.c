#define _XOPEN_SOURCE 500

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include "reservar.h"


#define NUM_PARKING_SPOTS 10

pthread_mutex_t mutex;
pthread_cond_t cond;

int parking_spots[NUM_PARKING_SPOTS];

int display = 0;
int ticket_dist = 0;


void initReservar() {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    for (int i = 0; i < NUM_PARKING_SPOTS; i++) {
        parking_spots[i] = 0;
    }
}

void cleanReservar() {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

int reservar(int k) {
    pthread_mutex_lock(&mutex);

    int i = 0;
    int j = 0;
    int my_num = ticket_dist++;

    while (my_num != display) {
        pthread_cond_wait(&cond, &mutex);
    }

    while (true) {
        for (i = 0; i <= NUM_PARKING_SPOTS - k; i++) {
            for (j = i; j < i + k; j++) {
                if (parking_spots[j] == 1)
                    break;
            }
            if (j == i + k) {
                break;
            }
        }
        if (j == i + k) {
            break;
        }
        pthread_cond_wait(&cond, &mutex);
    }

    for (j = i; j < i + k; j++) {
        parking_spots[j] = 1;
    }

    display++;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
    return i;
}

void liberar(int e, int k) {
    pthread_mutex_lock(&mutex);
    for (int i = e; i < e + k; i++) {
        parking_spots[i] = 0;
    }
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
}
