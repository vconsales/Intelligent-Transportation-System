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

PROCESS(Process_1, "ground_sensor1");
AUTOSTART_PROCESSES(&Process_1);

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

static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  	printf("broadcast message received from %d.%d: '%s'\n", from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
}
 
static void broadcast_sent(struct broadcast_conn *c, int status, int num_tx)
{ 
	printf("broadcast message sent. Status %d. For this packet, this is transmission number %d\n", status, num_tx);
}
 
static const struct broadcast_callbacks broadcast_call = {broadcast_recv, broadcast_sent}; 
static struct broadcast_conn broadcast;
 
 
static const struct runicast_callbacks runicast_calls = {recv_runicast, sent_runicast, timedout_runicast};
static struct runicast_conn runicast;


PROCESS_THREAD(Process_1, ev, data) {
	static struct etimer et;
	char message[12];
	PROCESS_BEGIN();
	SENSORS_ACTIVATE(button_sensor);

	broadcast_open(&broadcast, BROADCAST_PORT, &broadcast_call);
	// chiudere le connessioni quando si chiude il firmware
	PROCESS_EXITHANDLER(broadcast_close(&broadcast));

	while(1) {
		PROCESS_WAIT_EVENT_UNTIL((ev == sensors_event && data == &button_sensor) );
		if(ev == sensors_event && data == &button_sensor)
		{
			etimer_set(&et, CLOCK_SECOND*0.5);
			PROCESS_WAIT_EVENT();
			if(ev == sensors_event && data == &button_sensor){
				// second press, emergency veichle
				sprintf(message, "EMERG-MAIN");
			} else {
				// timer scaduto
				sprintf(message, "NORMA-MAIN");
			}
			packetbuf_copyfrom(message, strlen(message)+1);
			broadcast_send(&broadcast);
		}
	}
	PROCESS_END();
}
