#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#include "disk.h"
#include "pss.h"

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int ready;
    pthread_cond_t w;
} Request;

int busy = 0;

PriQueue *lesserQueue;
PriQueue *greaterQueue;

int current_track;

void iniDisk(void) {
    // initiate the variables as priority queues
    lesserQueue = makePriQueue();
    greaterQueue = makePriQueue();
}

void cleanDisk(void) {
    // destroy the priority queues
    destroyPriQueue(lesserQueue);
    destroyPriQueue(greaterQueue);
}

void requestDisk(int track) {
    pthread_mutex_lock(&m);
    if (!busy) {
        busy = 1;
        current_track = track;
    } else {
        // current track is equal to the head of the greater priority queue
        Request req = {0, PTHREAD_COND_INITIALIZER};
        // the new track will be added to the priority queue
        // depending if its greater or lesser than the head of the disk
        if (current_track <= track) {
            priPut(greaterQueue, &req, track);
        } else {
            priPut(lesserQueue, &req, track);
        }
        while (!req.ready) {
            pthread_cond_wait(&req.w, &m);
        }
    }
    pthread_mutex_unlock(&m);
}

void releaseDisk() {
    pthread_mutex_lock(&m);
    // if the greater queue is not empty, we unqueue the head and update the current track, making a request
    if (!emptyPriQueue(greaterQueue)) {
        current_track = priBest(greaterQueue);
        Request *req = priGet(greaterQueue);
        req->ready = 1;
        pthread_cond_signal(&req->w);
    } else {
        // when the greater queue is empty, we swap the queues so now the lesser queue becomes the greater queue
        // the tempqueue helps us make this change
        PriQueue *temp = lesserQueue;
        lesserQueue = greaterQueue;
        greaterQueue = temp;
        // we check if this new queue is empty, if its not then we make a new request, updating the head of the queue first
        if (!emptyPriQueue(greaterQueue)) {
            current_track = priBest(greaterQueue);
            Request *req = priGet(greaterQueue);
            req->ready = 1;
            pthread_cond_signal(&req->w);
        }
        // if this swapped queue is also empty, then the whole disk is empty
        else {
            busy = 0;
        }
    }
    pthread_mutex_unlock(&m);
}