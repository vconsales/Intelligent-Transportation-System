#define G1
#include "../commons.h"

static unsigned char state_rec_data = NO_REC_SENS;
static sensed_data rec_s[3];


PROCESS(Process_1, "ground_sensor1");
//PROCESS(Process_2, "sensing_process");
//AUTOSTART_PROCESSES(&Process_1, &Process_2);
AUTOSTART_PROCESSES(&Process_1);


static void insert_sensed_data(const linkaddr_t *from, sensed_data *s)
{
	int v = virtual_address(from);
	if( v == -1 )
		return;

	rec_s[v].temperature = s->temperature;
	rec_s[v].humidity = s->humidity;

	switch(v){
		case V_TL1_ADDR:
			state_rec_data |= REC_SENS_TL1;
			break;
		case V_TL2_ADDR:
			state_rec_data |= REC_SENS_TL2;
			break;
		case V_G2_ADDR:
			state_rec_data |= REC_SENS_G2;
			break;
	}
	printf("v=%d temp=%d hum=%d state_rec_data:%x\n",v,s->temperature,s->humidity,state_rec_data);
}

static void recv_runicast(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno){
	//printf("runicast message received from %d.%d, msg:%s\n", from->u8[0], from->u8[1], packetbuf_dataptr());
	printf("runicast message received from %d.%d\n", from->u8[0], from->u8[1]);

	sensed_data* s = (sensed_data*)packetbuf_dataptr();
	insert_sensed_data(from,s);

	if( state_rec_data == 0x07 ){
		process_post(&Process_1,ALL_SENS_REC,NULL);
	}
}
 
 
static void sent_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
	printf("runicast message sent to %d.%d, retransmissions %d\n", to->u8[0], to->u8[1], retransmissions);
}
 
 
static void timedout_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
	printf("runicast message timed out when sending to %d.%d, retransmissions %d\n", to->u8[0], to->u8[1], retransmissions);
}

//non serve
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
static struct runicast_conn runicast[3];


static void close_all()
{
	int i;
	for(i=0; i<3; i++)
		runicast_close(&runicast[i]);
}


PROCESS_THREAD(Process_1, ev, data) {
	static struct etimer second_press_timer;
	static char message[12];
	
	PROCESS_BEGIN();
	static int humidity;
	static int temperature;
	static int i;

	static linkaddr_t my_addr;
	my_addr.u8[0] = G1_ADDR;
	my_addr.u8[1] = 0;
	linkaddr_set_node_addr(&my_addr); 

	runicast_open(&(runicast[0]), G2_TO_G1_PORT, &runicast_calls);
	runicast_open(&(runicast[1]), TL1_TO_G1_PORT, &runicast_calls);
	runicast_open(&(runicast[2]), TL2_TO_G1_PORT, &runicast_calls);

	printf("my address: %d.%d\n",linkaddr_node_addr.u8[0],linkaddr_node_addr.u8[1]);

	SENSORS_ACTIVATE(button_sensor);

	broadcast_open(&broadcast, BROADCAST_PORT, &broadcast_call);
	// chiudere le connessioni quando si chiude il firmware
	PROCESS_EXITHANDLER(broadcast_close(&broadcast));

	while(1) {
		//PROCESS_WAIT_EVENT_UNTIL((ev == sensors_event && data == &button_sensor) );
		PROCESS_WAIT_EVENT();

		if(ev == sensors_event && data == &button_sensor) { // questo if è inutile
			etimer_set(&second_press_timer, CLOCK_SECOND*SECOND_PRESS);
			PROCESS_WAIT_EVENT();
			if(ev == sensors_event && data == &button_sensor){
				// second press, emergency veichle
				printf("seconda pressione\n");
				sprintf(message, "EMERG-MAIN");
				etimer_stop(&second_press_timer);
			} else {
				// timer scaduto
				sprintf(message, "NORMA-MAIN");
			}
			packetbuf_copyfrom(message, strlen(message)+1);
			broadcast_send(&broadcast);
		} else if( ev == ALL_SENS_REC) {
			printf("ricevuti da tutti \n");
			SENSORS_ACTIVATE(sht11_sensor);
			humidity = sht11_sensor.value(SHT11_SENSOR_HUMIDITY)/41;
			temperature = (sht11_sensor.value(SHT11_SENSOR_TEMP)/10-396)/10;
			SENSORS_DEACTIVATE(sht11_sensor);

			for(i=0; i<3; i++){
				humidity += rec_s[i].humidity;
				temperature += rec_s[i].temperature;
			}

			humidity /= 4;
			temperature /= 4;

			printf("AVG TEMP=%d HUM=%d\n",temperature,humidity);
			state_rec_data = NO_REC_SENS;
		}
	}
	PROCESS_END();
}

PROCESS_THREAD(Process_2, ev, data)
{
	PROCESS_BEGIN();
	SENSORS_ACTIVATE(button_sensor);
	uart1_set_input(serial_line_input_byte);
	serial_line_init();

	while(1) {
		PROCESS_WAIT_EVENT_UNTIL((ev == sensors_event && data == &button_sensor) );
		if(ev == sensors_event && data == &button_sensor)
		{
			while(1) {
				printf("write password: ");
			
				PROCESS_WAIT_EVENT_UNTIL( ev == serial_line_event_message );
				printf("password typed: %s\n",(char *)data);
				if( strcmp((char *)data,"user") == 0 ){
					leds_off(LEDS_RED);
					if(tempe_values_count < 5)
					{
						printf("Temperature Average: Not enough values\n");
					}
					else
						printf("Temperature Average: %d\n", get_avg(tempe_values));
					break;
				} else {
					leds_on(LEDS_RED);		
				}
			}
		}
	}
	PROCESS_END();
}
