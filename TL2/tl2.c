#include "contiki.h"
#include "stdio.h"
#include "sys/etimer.h"
#include "string.h"
#include "dev/serial-line.h"
#include "dev/button-sensor.h"
#include "dev/light-sensor.h"
#include "dev/sht11/sht11-sensor.h"
#include "dev/leds.h"
#include "dev/serial-line.h"
#include "net/rime/rime.h"
#include "../commons.h"

//semaforo nella strada di emergenza

PROCESS(Process_1, "traffic_scheduler2");
AUTOSTART_PROCESSES(&Process_1);

//non serve
/*static void recv_runicast(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno){
	static char buf[PACKETBUF_SIZE];
	packetbuf_copyto(buf);
	printf("runicast message received from %d.%d, msg:%s\n", from->u8[0], from->u8[1], buf);
}*/
 
 
static void sent_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
	printf("runicast message sent to %d.%d, retransmissions %d\n", to->u8[0], to->u8[1], retransmissions);
}
 
 
static void timedout_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
	printf("runicast message timed out when sending to %d.%d, retransmissions %d\n", to->u8[0], to->u8[1], retransmissions);
}

static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  	printf("broadcast message received from %d.%d: '%s'\n", from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
  	process_post(&Process_1, PROCESS_EVENT_MSG, packetbuf_dataptr());
}
 
static void broadcast_sent(struct broadcast_conn *c, int status, int num_tx)
{ 
	printf("broadcast message sent. Status %d. For this packet, this is transmission number %d\n", status, num_tx);
}
 
 
static const struct broadcast_callbacks broadcast_call = {broadcast_recv, broadcast_sent}; 
static struct broadcast_conn broadcast;
 
 
static const struct runicast_callbacks runicast_calls = {NULL, sent_runicast, timedout_runicast};
static struct runicast_conn runicast;

void detect_intersection_msg(char *data, unsigned char *intersection_state_new)
{
	if( strcmp(data, "EMERG-MAIN") == 0 ){
		*intersection_state_new |= EMER_MAIN;
	} else if( strcmp(data, "EMERG-SECO") == 0 ) {
		*intersection_state_new |= EMER_SECO;
	} else if( strcmp(data, "NORMA-MAIN") == 0 ) {
		*intersection_state_new |= NORM_MAIN;
	} else if( strcmp(data, "NORMA-SECO") == 0 ) {
		*intersection_state_new |= NORM_SECO;
	}
}

PROCESS_THREAD(Process_1, ev, data) {
	static struct etimer et;
	static unsigned char intersection_state_curr= 0x00;
	static unsigned char intersection_state_new = 0x00;

	PROCESS_EXITHANDLER(broadcast_close(&broadcast));
	PROCESS_EXITHANDLER(runicast_close(&runicast));

	PROCESS_BEGIN();
	
	static linkaddr_t recv;
	recv.u8[0] = TL2_ADDR;
	recv.u8[1] = 0;

	broadcast_open(&broadcast, BROADCAST_PORT, &broadcast_call);
	runicast_open(&runicast, TL2_TO_GI_PORT, &runicast_calls);

	printf("Starting traffic schedule TL2...\n");
	leds_on(LEDS_GREEN);
	leds_on(LEDS_RED);
	etimer_set(&et, CLOCK_SECOND*1);

	while(1) {
		PROCESS_WAIT_EVENT();
		if( ev == PROCESS_EVENT_MSG ){
			detect_intersection_msg(data,&intersection_state_new);	
 
			//printf("intersection_state_new:%x\n",intersection_state_new);

			// nessuna macchina attraversa l'incrocio e passa 
			// una macchina sulla strada principale
			if( intersection_state_curr == 0x00 ) {
				if( intersection_state_new & (EMER_SECO | NORM_SECO) ){
					leds_off(LEDS_RED);
					leds_on(LEDS_GREEN);
					etimer_set(&et,CLOCK_SECOND*CROSS_PERIOD);
					intersection_state_curr = intersection_state_new & (EMER_SECO | NORM_SECO);
				} else if( intersection_state_new & (EMER_MAIN | NORM_MAIN) ) {
					leds_on(LEDS_RED);
					leds_off(LEDS_GREEN);
					etimer_set(&et,CLOCK_SECOND*CROSS_PERIOD);
					intersection_state_curr = intersection_state_new & (EMER_MAIN | NORM_MAIN);
				}
			}
			//printf("intersection_state_curr:%x\n",intersection_state_curr);
		} else if( etimer_expired(&et) ){
			printf("expired intersection_state_curr:%x\n",intersection_state_curr);
			if( intersection_state_curr == 0x00 ){
				leds_toggle(LEDS_GREEN | LEDS_RED );
				etimer_set(&et,CLOCK_SECOND*TOGGLE_PERIOD);
				//printf("nessun veicolo\n");
			} else {
				// la macchina è stata schedulata
				//printf("macchina schedulata\n");
				intersection_state_new &= ~intersection_state_curr; 
				intersection_state_curr = intersection_state_new;
				leds_off(LEDS_RED | LEDS_GREEN);
				
				if( intersection_state_new & EMER_MAIN ) {
					leds_on(LEDS_RED);
					leds_off(LEDS_GREEN);
					etimer_set(&et,CLOCK_SECOND*CROSS_PERIOD);
					intersection_state_curr = intersection_state_new & EMER_MAIN;
				} else if( intersection_state_new & EMER_SECO ){
					leds_off(LEDS_RED);
					leds_on(LEDS_GREEN);
					etimer_set(&et,CLOCK_SECOND*CROSS_PERIOD);
					intersection_state_curr = intersection_state_new & EMER_SECO;
				} else if( intersection_state_new & NORM_MAIN ){
					leds_on(LEDS_RED);
					leds_off(LEDS_GREEN);
					etimer_set(&et,CLOCK_SECOND*CROSS_PERIOD);
					intersection_state_curr = intersection_state_new & NORM_MAIN ;
				} else if( intersection_state_new & NORM_SECO ){
					leds_off(LEDS_RED);
					leds_on(LEDS_GREEN);
					etimer_set(&et,CLOCK_SECOND*CROSS_PERIOD);
					intersection_state_curr = intersection_state_new & NORM_SECO;
				} else {
					etimer_set(&et,CLOCK_SECOND*TOGGLE_PERIOD);
				}
			}
			
		}
	}

	PROCESS_END();
}