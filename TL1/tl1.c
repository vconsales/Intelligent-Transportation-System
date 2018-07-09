#include "../commons.h"

static int battery_lvl = MAX_BATTERY;
PROCESS(Process_1, "traffic_scheduler1");
AUTOSTART_PROCESSES(&Process_1);

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
 
static const struct broadcast_callbacks broadcast_call = {broadcast_recv, NULL}; 
static struct broadcast_conn broadcast;
 
 
static const struct runicast_callbacks runicast_calls = {NULL, sent_runicast, timedout_runicast};
static struct runicast_conn runicast;

static void close_all()
{
	broadcast_close(&broadcast);
	runicast_close(&runicast);

}

PROCESS_THREAD(Process_1, ev, data) {
	static struct etimer leds_timer;
	static struct etimer sense_timer;
	static unsigned char intersection_state_curr= 0x00;
	static unsigned char intersection_state_new = 0x00;
	
	PROCESS_EXITHANDLER(close_all());
	PROCESS_BEGIN();
	printf("Starting traffic schedule TL2...\n");

	static linkaddr_t my_addr;
	my_addr.u8[0] = TL1_ADDR;
	my_addr.u8[1] = 0;
	linkaddr_set_node_addr (&my_addr);

	static linkaddr_t recv;
	recv.u8[0] = G1_ADDR;
	recv.u8[1] = 0;

	broadcast_open(&broadcast, BROADCAST_PORT, &broadcast_call);
	runicast_open(&runicast, TL1_TO_G1_PORT, &runicast_calls);
	
	SENSORS_ACTIVATE(button_sensor);
	leds_on(LEDS_GREEN);
	leds_on(LEDS_RED);

	etimer_set(&leds_timer, CLOCK_SECOND*TOGGLE_PERIOD);
	etimer_set(&sense_timer, CLOCK_SECOND*SENSE_PERIOD_H);

	while(1) {
		PROCESS_WAIT_EVENT();
		if( ev == PROCESS_EVENT_MSG ){
			detect_intersection_msg(data,&intersection_state_new);	
 
 			//printf("intersection_state_new:%x\n",intersection_state_new);

			// nessuna macchina attraversa l'incrocio e passa 
			// una macchina sulla strada principale
			if( intersection_state_curr == 0x00 ) {
				if( intersection_state_new & (EMER_MAIN | NORM_MAIN) ) {
					leds_off(LEDS_RED);
					leds_on(LEDS_GREEN);
					etimer_set(&leds_timer,CLOCK_SECOND*CROSS_PERIOD);
					intersection_state_curr = intersection_state_new & (EMER_MAIN | NORM_MAIN);
				} else if( intersection_state_new & (EMER_SECO | NORM_SECO) ){
					leds_on(LEDS_RED);
					leds_off(LEDS_GREEN);
					etimer_set(&leds_timer,CLOCK_SECOND*CROSS_PERIOD);
					intersection_state_curr = intersection_state_new & (EMER_SECO | NORM_SECO);
				} 
			}
			//printf("intersection_state_curr:%x\n",intersection_state_curr);
		} else if( etimer_expired(&leds_timer) ){
			//printf("expired intersection_state_curr:%x\n",intersection_state_curr);
			if( intersection_state_curr == 0x00 ){
				leds_toggle(LEDS_GREEN | LEDS_RED );
				
				if( battery_lvl <= LOW_BATTERY )
					leds_toggle(LEDS_BLUE);
				else
					leds_off(LEDS_BLUE);

				etimer_set(&leds_timer,CLOCK_SECOND*TOGGLE_PERIOD);
				battery_lvl -= LEDS_DRAIN;
			} else {
				// la macchina è stata schedulata
				//printf("macchina schedulata\n");
				intersection_state_new &= ~intersection_state_curr; 
				intersection_state_curr = intersection_state_new;
				leds_off(LEDS_RED | LEDS_GREEN);
				
				if( intersection_state_new & EMER_MAIN ) {
					leds_off(LEDS_RED);
					leds_on(LEDS_GREEN);
					battery_lvl -= LEDS_DRAIN;
					etimer_set(&leds_timer,CLOCK_SECOND*CROSS_PERIOD);
					intersection_state_curr = intersection_state_new & EMER_MAIN;
				} else if( intersection_state_new & EMER_SECO ){
					leds_on(LEDS_RED);
					leds_off(LEDS_GREEN);
					battery_lvl -= LEDS_DRAIN;
					etimer_set(&leds_timer,CLOCK_SECOND*CROSS_PERIOD);
					intersection_state_curr = intersection_state_new & EMER_SECO;
				} else if( intersection_state_new & NORM_MAIN ){
					leds_off(LEDS_RED);
					leds_on(LEDS_GREEN);
					battery_lvl -= LEDS_DRAIN;
					etimer_set(&leds_timer,CLOCK_SECOND*CROSS_PERIOD);
					intersection_state_curr = intersection_state_new & NORM_MAIN ;
				} else if( intersection_state_new & NORM_SECO ){
					leds_on(LEDS_RED);
					leds_off(LEDS_GREEN);
					battery_lvl -= LEDS_DRAIN;
					etimer_set(&leds_timer,CLOCK_SECOND*CROSS_PERIOD);
					intersection_state_curr = intersection_state_new & NORM_SECO;
				} else {
					etimer_set(&leds_timer,CLOCK_SECOND*TOGGLE_PERIOD);
				}
			}
		} else if( ev == sensors_event && data == &button_sensor ) {
			battery_lvl = MAX_BATTERY;
		} else if( etimer_expired(&sense_timer) ) {
			//printf("sono TL1, mando temp\n");
			do_sense(&runicast, &recv, &battery_lvl);

			// if battery lvl is below the energy consumed by sense process 
			// the task 2 is disabled
			set_sense_timer(&sense_timer, battery_lvl);
		}
	}

	PROCESS_END();
}