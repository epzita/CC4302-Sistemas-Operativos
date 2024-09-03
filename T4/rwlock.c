#define _XOPEN_SOURCE 500

#include "nthread-impl.h"
#include "rwlock.h"

struct rwlock {
    // colas NthQueue para el manejo de lectores/escritores
    NthQueue *readersQ;
    NthQueue *writersQ;
    // cantidad de lectores y escritores
    int readers;
    // estado del diccionario
    int writing;
};

nRWLock *nMakeRWLock() {
    // creación del controlador
    nRWLock *rwl = malloc(sizeof(nRWLock));
    rwl->readersQ = nth_makeQueue();
    rwl->writersQ = nth_makeQueue();
    rwl->readers = 0;
    // no usaremos int writers, ya que solo nos interesa la cola de solicitudes
    // y el estado writing
    rwl->writing = 0;
    return rwl;
}

void nDestroyRWLock(nRWLock *rwl) {
    // destrucción del controlador
    nth_destroyQueue(rwl->readersQ);
    nth_destroyQueue(rwl->writersQ);
    // no olvidamos liberar la memoria pedida con malloc
    free(rwl);
}

// solicitud de ingreso del lector
int nEnterRead(nRWLock *rwl, int timeout) {
    START_CRITICAL
    if(nth_queueLength(rwl -> writersQ) == 0 && rwl -> writing == 0){
        rwl -> readers++;
    }
    // La solicitud queda pendiente si hay un escritor trabajando o la cola de lectores no está vacía
    // PD: Me costo mucho darme cuenta que mi codigo no terminaba de funcionar porque
    // no se leer y estaba chequeando la segunda condicion en la lista de lectores en vez de escritores
    else {
        nThread thisTh = nSelf();
        nth_putBack(rwl->readersQ, thisTh);
        suspend(WAIT_RWLOCK);
        schedule();
    }
    END_CRITICAL
    return 1;

}

// solicitud de ingreso del escritor
int nEnterWrite(nRWLock *rwl, int timeout) {
    START_CRITICAL
    if (rwl -> readers == 0 && rwl -> writing == 0){
        rwl-> writing = 1;
    }
    // Si hay al menos un lector leyendo o un escritor escribiendo, la solicitud queda pendiente
    else {
        nThread thisTh = nSelf();
        nth_putBack(rwl->writersQ, thisTh);
        suspend(WAIT_RWLOCK);
        schedule();
    }
    END_CRITICAL
    return 1;
}

// salida de un lector
void nExitRead(nRWLock *rwl) {
    START_CRITICAL
    rwl->readers--;
    // si no hay lectores pendientes y hay escritores esperando
    // se acepta el escritor que lleva más tiempo esperando
    if (rwl->readers == 0 && nth_queueLength(rwl->writersQ)>0) {
        nThread w = nth_getFront(rwl->writersQ);
        rwl->writing = 1;
        setReady(w);
        schedule();
    }
    
    END_CRITICAL
}

// salida de un escritor
void nExitWrite(nRWLock *rwl) {
    START_CRITICAL
    rwl->writing = 0;
    // Si hay solicitudes de lectores pendientes, se aceptan todos los lectores
    if (nth_queueLength(rwl->readersQ)>0) {
        while (nth_queueLength(rwl->readersQ)>0) {
            nThread r = nth_getFront(rwl->readersQ);
            rwl -> readers++;
            setReady(r);
        }
        schedule();
        
    }
    // Si no hay solicitudes de lectores pendientes y si hay solicitudes de escritores pendientes
    // se atiende la del lector que lleva más tiempo esperando
    else if (nth_queueLength(rwl -> writersQ) > 0) {
        nThread w = nth_getFront(rwl->writersQ);
        rwl->writing = 1;
        setReady(w);
        schedule();
    }
    
    END_CRITICAL
}