#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <pthread.h>

#include "maleta.h"

int P = 8;

typedef struct {
    double *w;
    double *v;
    int *z;
    int n;
    double maxW;
    int k;
    double res;

} Args;

void *thread(void *p){
    Args *args = (Args *)p;
    double *w = args->w;
    double *v = args->v;
    int *z = args->z;
    int n = args->n;
    double maxW = args->maxW;
    int k = args->k;

    args->res = llenarMaletaSec(w, v, z, n, maxW, k);

    return NULL;
}

double llenarMaletaPar(double w[], double v[], int z[], int n,
                       double maxW, int k) {
    pthread_t pid[P];
    Args args[P];

    for(int i = 0; i < P; i++){
        args[i].w = w;
        args[i].v = v;
        args[i].z = malloc(n * sizeof(int)); 
        args[i].n = n;
        args[i].maxW = maxW;
        args[i].k = k/8;
        
        pthread_create(&pid[i], NULL, thread, &args[i]);
    }

    double best = 0;
    for(int i = 0; i < P; i++){
        pthread_join(pid[i], NULL);
        if(args[i].res > best){
            best = args[i].res;
            for(int j = 0; j < n; j++){
                z[j] = args[i].z[j];
            }
        }
    }

    for(int i = 0; i < P; i++){
        free(args[i].z); // Liberar memoria para z
    }

    return best;
}
