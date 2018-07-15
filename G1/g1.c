#define G1
#include "../commons.h"

static int avg_humidity;
static int avg_temperature;
static unsigned char state_rec_data = NO_REC_SENS;
static sensed_data rec_s[3];
static char *emergency_msg = NULL;

PROCESS(Process_1, "ground_sensor1");
PROCESS(Process_2, "rtd_emergency");
AUTOSTART_PROCESSES(&Process_1, &Process_2);

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
	#ifdef DEBUG
	printf("v=%d temp=%d hum=%d state_rec_data:%x\n",v,s->temperature,s->humidity,state_rec_data);
	#endif
}

static void recv_runicast(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno){
	#ifdef DEBUG
	printf("runicast message received from %d.%d\n", from->u8[0], from->u8[1]);
	#endif

	sensed_data* s = (sensed_data*)packetbuf_dataptr();
	insert_sensed_data(from,s);

	if( state_rec_data == 0x07 ){
		process_post(&Process_1,ALL_SENS_REC,NULL);
	}
}
 
 
static void sent_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
	#ifdef DEBUG
	printf("runicast message sent to %d.%d, retransmissions %d\n", to->u8[0], to->u8[1], retransmissions);
	#endif
}
 
static void timedout_runicast(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
	printf("runicast message timed out when sending to %d.%d, retransmissions %d\n", to->u8[0], to->u8[1], retransmissions);
}
 
static void broadcast_sent(struct broadcast_conn *c, int status, int num_tx)
{ 
	#ifdef DEBUG
	printf("broadcast message sent. Status %d. For this packet, this is transmission number %d\n", status, num_tx);
	#endif
}
 
static const struct broadcast_callbacks broadcast_call = {NULL, broadcast_sent}; 
static struct broadcast_conn broadcast;

 
static const struct runicast_callbacks runicast_calls = {recv_runicast, sent_runicast, timedout_runicast};
static struct runicast_conn runicast[3];


static void display_RTD(){
	char *str = emergency_msg;
	if( str == NULL )
		str = "";
	printf("%s AVG TEMP=%d HUM=%d\n",str,avg_temperature,avg_humidity);
			
}

static void close_all() {
	int i;
	for(i=0; i<3; i++)
		runicast_close(&runicast[i]);
}

PROCESS_THREAD(Process_1, ev, data) {
	static struct etimer second_press_timer;
	static char message[12];
	
	PROCESS_EXITHANDLER(close_all());
	PROCESS_BEGIN();
	static int i;

	static linkaddr_t my_addr;
	my_addr.u8[0] = G1_ADDR;
	my_addr.u8[1] = 0;
	linkaddr_set_node_addr(&my_addr); 

	runicast_open(&(runicast[0]), G2_TO_G1_PORT, &runicast_calls);
	runicast_open(&(runicast[1]), TL1_TO_G1_PORT, &runicast_calls);
	runicast_open(&(runicast[2]), TL2_TO_G1_PORT, &runicast_calls);
	broadcast_open(&broadcast, BROADCAST_PORT, &broadcast_call);

	SENSORS_ACTIVATE(button_sensor);

	while(1) {
		PROCESS_WAIT_EVENT();

		if(ev == sensors_event && data == &button_sensor) { 
			etimer_set(&second_press_timer, CLOCK_SECOND*SECOND_PRESS);
			PROCESS_WAIT_EVENT();
			if(ev == sensors_event && data == &button_sensor){
				// second press, emergency veichle
				//printf("seconda pressione\n");
				sprintf(message, "EMERG-MAIN");
				etimer_stop(&second_press_timer);
			} else {
				// timer scaduto
				sprintf(message, "NORMA-MAIN");
			}
			packetbuf_copyfrom(message, strlen(message)+1);
			broadcast_send(&broadcast);
		} else if( ev == ALL_SENS_REC) {
			//printf("ricevuti da tutti \n");
			SENSORS_ACTIVATE(sht11_sensor);
			avg_humidity = sht11_sensor.value(SHT11_SENSOR_HUMIDITY)/41;
			avg_temperature = (sht11_sensor.value(SHT11_SENSOR_TEMP)/10-396)/10;
			SENSORS_DEACTIVATE(sht11_sensor);

			for(i=0; i<3; i++){
				avg_humidity += rec_s[i].humidity;
				avg_temperature += rec_s[i].temperature;
			}

			avg_humidity /= 4;
			avg_temperature /= 4;

			display_RTD();
			state_rec_data = NO_REC_SENS;
		}
	}
	PROCESS_END();
}

PROCESS_THREAD(Process_2, ev, data)
{
	static int correct_pass = 0;
	PROCESS_BEGIN();
	serial_line_init();

	while(1) {
		while( correct_pass == 0 ){
			printf("enter password: \n");
			PROCESS_WAIT_EVENT_UNTIL( ev == serial_line_event_message );
			if( strcmp((char *)data,"NES") == 0 ){
				printf("Correct password\n");
				correct_pass = 1;
			} else 
				printf("Incorrect password!\n");
		}
		
		printf("Type emergency warning: \n");
		PROCESS_WAIT_EVENT_UNTIL( ev == serial_line_event_message );
		char *str_msg = (char*)data;
		
		if( emergency_msg != NULL )
			free(emergency_msg);
		emergency_msg = malloc(sizeof(char) * (strlen(str_msg)+1));
		strcpy(emergency_msg, str_msg);
		correct_pass = 0;
	}
	PROCESS_END();
}