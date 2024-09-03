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

void f(nThread th){
  nth_delQueue(th->ptr, th);
  th->ptr = NULL;
}

// solicitud de ingreso del lector
int nEnterRead(nRWLock *rwl, int timeout) {
    START_CRITICAL
    // La solicitud queda pendiente si hay un escritor trabajando o la cola de lectores no está vacía
    // PD(T4): Me costo mucho darme cuenta que mi codigo no terminaba de funcionar porque 
    // no se leer y estaba chequeando la segunda condicion en la lista de lectores en vez de escritores
    if (nth_queueLength(rwl -> writersQ)>0 || rwl ->writing) {
        nThread thisTh = nSelf();
        nth_putBack(rwl->readersQ, thisTh);
        suspend(WAIT_RWLOCK);
        schedule();
    }
    // De lo contrario la solicitud se atiende de inmediato
    rwl->readers++;
    
    return 1;

}

// solicitud de ingreso del escritor
int nEnterWrite(nRWLock *rwl, int timeout) {
    START_CRITICAL
    // Si hay al menos un lector leyendo o un escritor escribiendo, la solicitud queda pendiente
    if (rwl->readers > 0 || rwl->writing) {
        nThread thisTh = nSelf();
        nth_putBack(rwl->writersQ, thisTh);
        if(timeout > 0){
          NthQueue *w = rwl -> writersQ;
          thisTh -> ptr  = w;
          suspend(WAIT_RWLOCK_TIMEOUT);
          nth_programTimer(timeout * 1000000LL, f);
        }
        else{
          suspend(WAIT_RWLOCK);
        }
        schedule();
        if(timeout > 0 && thisTh -> ptr == NULL){
          END_CRITICAL
          return 0;
        }
    }
    // De lo contrario la solicitud se atiende de inmediato
    rwl->writing = 1;
  
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
        //cambiamos inmediatamente el estado writing para evitar que entre un lector cuando no deba
        rwl->writing = 1;
        //verificamos el estado del thread
        if(w->status == WAIT_RWLOCK || w -> status == WAIT_RWLOCK_TIMEOUT){
          //si el estado continua en timeout, cancelamos el thread
          if(w->status == WAIT_RWLOCK_TIMEOUT){
            nth_cancelThread(w);
          }
        }
        rwl->writing=1; 
        setReady(w);
        schedule();
    }
    END_CRITICAL
}

// salida de un escritor
void nExitWrite(nRWLock *rwl) {
    START_CRITICAL
    
    // Si hay solicitudes de lectores pendientes, se aceptan todos los lectores
    if (nth_queueLength(rwl->readersQ)>0) {
        while (nth_queueLength(rwl->readersQ)>0) {
            nThread r = nth_getFront(rwl->readersQ);
            rwl->readers++;
            setReady(r);
        }
        schedule();
    }
    // Si no hay solicitudes de lectores pendientes y si hay solicitudes de escritores pendientes
    // se atiende la del lector que lleva más tiempo esperando
    else if (nth_queueLength(rwl -> writersQ) > 0) {
        nThread w = nth_getFront(rwl->writersQ);
        //cambiamos inmediatamente el estado writing a ocupado
        rwl -> writing = 1;
        //checamos el estado del thread
        if(w->status == WAIT_RWLOCK || w -> status == WAIT_RWLOCK_TIMEOUT){
          //cancelamos de ser necesario
          if(w->status == WAIT_RWLOCK_TIMEOUT){
            nth_cancelThread(w);
          }
        } 
        setReady(w);
        schedule();
    }
    rwl->writing = 0;
    END_CRITICAL
}