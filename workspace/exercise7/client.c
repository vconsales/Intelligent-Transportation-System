/*
 * Transmitter.c
 *
 *  Created on: Apr 25, 2018
 *      Author: user
 */
 
#include "stdio.h"
#include "contiki.h"
#include "sys/etimer.h"
#include "net/rime/rime.h"
#include "dev/button-sensor.h"

#define MAX_RETRANSMISSIONS 5
 
 
PROCESS(runicast_process, "runicast process - transmitter");
 
AUTOSTART_PROCESSES(&runicast_process);
 
static void recv_runicast(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno){
  static char buf[PACKETBUF_SIZE];
  packetbuf_copyto(buf);
  printf("runicast message received from %d.%d, msg:%s\n", from->u8[0], from->u8[1], buf);
}
 
 
static void sent_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
  printf("runicast message sent to %d.%d, retransmissions %d\n", to->u8[0], to->u8[1], retransmissions);
}
 
 
static void timedout_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
  printf("runicast message timed out when sending to %d.%d, retransmissions %d\n", to->u8[0], to->u8[1], retransmissions);
}
 
 
static const struct runicast_callbacks runicast_calls = {recv_runicast, sent_runicast, timedout_runicast};
static struct runicast_conn runicast;
 
 
PROCESS_THREAD(runicast_process, ev, data)
{
  PROCESS_EXITHANDLER(runicast_close(&runicast));
 
  PROCESS_BEGIN();
 

  SENSORS_ACTIVATE(button_sensor);
    
  runicast_open(&runicast, 1024, &runicast_calls);
  static linkaddr_t recv;
  recv.u8[0] = 20;
  recv.u8[1] = 0;


  while(1) {
    PROCESS_WAIT_EVENT();
  
    if(ev == sensors_event && data == &button_sensor){
      if(!runicast_is_transmitting(&runicast)) {
        packetbuf_copyfrom("REQ TEMP", 9);
        printf("%u.%u: sending runicast to address %u.%u\n", linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1], recv.u8[0], recv.u8[1]);
   
        runicast_send(&runicast, &recv, MAX_RETRANSMISSIONS);
      }
    }
  }
  PROCESS_END();
}
