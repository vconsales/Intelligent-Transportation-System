/*
 * Broadcast.c
 *
 *  Created on: Apr 25, 2018
 *      Author: user
 */
 
#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "stdio.h"
 
PROCESS(broadcast_process, "Broadcast process");
 
AUTOSTART_PROCESSES(&broadcast_process);
 
 
PROCESS_THREAD(broadcast_process, ev, data){
 
  static struct etimer et;
 
  PROCESS_BEGIN();
 
  while(1) {
 
    /* Delay 4-8 seconds */
    etimer_set(&et, CLOCK_SECOND*4 + random_rand()%(CLOCK_SECOND*4));

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    printf("my address: %d.%d\n",linkaddr_node_addr.u8[1],linkaddr_node_addr.u8[0]);
  }

  PROCESS_END();
}
