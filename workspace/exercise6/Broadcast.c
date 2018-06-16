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
 
 
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from){
 
  printf("broadcast message received from %d.%d: '%s'\n", from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
}
 
static void broadcast_sent(struct broadcast_conn *c, int status, int num_tx){
 
  printf("broadcast message sent. Status %d. For this packet, this is transmission number %d\n", status, num_tx);
}
 
static const struct broadcast_callbacks broadcast_call = {broadcast_recv, broadcast_sent}; //Be careful to the order
static struct broadcast_conn broadcast;
 
 
 
PROCESS_THREAD(broadcast_process, ev, data){
 
  static struct etimer et;
 
  PROCESS_EXITHANDLER(broadcast_close(&broadcast));
 
  PROCESS_BEGIN();
 
  broadcast_open(&broadcast, 129, &broadcast_call);
 
  while(1) {
 
    /* Delay 4-8 seconds */
    etimer_set(&et, CLOCK_SECOND*4 + random_rand()%(CLOCK_SECOND*4));

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    packetbuf_copyfrom("New message", 12);
    broadcast_send(&broadcast);
  }

  PROCESS_END();
}
