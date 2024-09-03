#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "pss.h"
#include "bolsa.h"
#include "spinlocks.h"

// Declare aca sus variables globales
typedef enum {PEND, RECHAZ, ADJUD} Resol;

int mutex = OPEN;
int best_price;
char *seller_name = NULL;
char *buyer_name = NULL;

Resol *global_r;
int *global_w;

int vendo(int precio, char *vendedor, char *comprador) {
  //comenzamos la zona critica
    spinLock(&mutex);
    //creamos la resolucion local
    Resol r = PEND;
    //ajustamos el precio de ser necesario
    if(best_price == 0 || precio < best_price){
      
      if(global_r != NULL){
        *global_r = RECHAZ;
        spinUnlock(global_w);
      }
      //ajustamos las variables globales
      best_price = precio;
      seller_name = vendedor;
      buyer_name = comprador;
      global_r = &r;

    }
    else {
      spinUnlock(&mutex);
      return 0;
    }
    //utilizamos spinlocks para esperar
    int w = CLOSED;
    global_w = &w;
    while(r == PEND){
      //esperamos
      spinUnlock(&mutex);
      spinLock(&w);
    }
    //devolvemos la resolucion de la venta
    return r == ADJUD;
}

int compro(char *comprador, char *vendedor) {
  spinLock(&mutex);
  //en caso de haber un vendedor el mejor precio será distinto de 0
  if(best_price == 0){
    spinUnlock(&mutex);
    return 0;
  }
  else{
      int paid;
      //cambiamos las variables globales una vez realizada la venta
      paid = best_price;
      strcpy(vendedor, seller_name);
      strcpy(buyer_name, comprador);
        
      *global_r = ADJUD;
      spinUnlock(global_w);
      //reiniciamos el mejor precio
      best_price = 0;
      global_r = NULL;
      spinUnlock(&mutex);
      return paid;
  }
} 
  

