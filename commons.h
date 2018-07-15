#include "contiki.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/etimer.h"
#include "string.h"
#include "dev/serial-line.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/light-sensor.h"
#include "dev/sht11/sht11-sensor.h"
#include "net/rime/rime.h"

//#define DEBUG

#define MAX_RETRANSMISSIONS 5
#define BROADCAST_PORT 559

#define G2_TO_G1_PORT  600
#define TL1_TO_G1_PORT 700
#define TL2_TO_G1_PORT 800
#define G1_TO_RTD_PORT 900

#define NORM_SECO 0x01
#define NORM_MAIN 0x02
#define EMER_SECO 0x04
#define EMER_MAIN 0x08 

#define MAX_BATTERY 100
#define MED_BATTERY 50
#define LOW_BATTERY 20
#define LEDS_DRAIN  5
#define SENSE_DRAIN 10

#define CROSS_PERIOD 5
#define TOGGLE_PERIOD 1
#define SECOND_PRESS 0.5
#define SENSE_PERIOD_H 5
#define SENSE_PERIOD_M 10
#define SENSE_PERIOD_L 20

#define TL1_ADDR 1
#define TL2_ADDR 2
#define G1_ADDR  3
#define G2_ADDR  4

#define V_TL1_ADDR 0
#define V_TL2_ADDR 1
#define V_G2_ADDR  2

#define NO_REC_SENS  0x00
#define REC_SENS_TL1 0x01
#define REC_SENS_TL2 0x02
#define REC_SENS_G2  0x04

//custom event to signal that all sensed data was received
#define ALL_SENS_REC 10


typedef struct sensed_data_t {
	int temperature;
	int humidity;
} sensed_data;


int virtual_address(const linkaddr_t *addr)
{
	if( addr->u8[0] == TL1_ADDR && addr->u8[1] == 0 )
		return V_TL1_ADDR;
	else if( addr->u8[0] == TL2_ADDR && addr->u8[1] == 0 ) 
		return V_TL2_ADDR;
	else if( addr->u8[0] == G2_ADDR && addr->u8[1] == 0 ) 
		return V_G2_ADDR;
	else
		return -1;
}

void set_sense_timer(struct etimer *sense_timer, int battery_lvl){
	int sense_period = SENSE_PERIOD_H;

	if( battery_lvl > SENSE_DRAIN ){
		if(battery_lvl <= MED_BATTERY && battery_lvl > LOW_BATTERY )
			sense_period = SENSE_PERIOD_M;
		else if ( battery_lvl <= LOW_BATTERY)
			sense_period = SENSE_PERIOD_L;
		else
			sense_period = SENSE_PERIOD_H;

		etimer_set(sense_timer,CLOCK_SECOND*sense_period);
	}
}

void drain_battery(int *battery_lvl, int d)
{
	*battery_lvl -= d;
	if( *battery_lvl < 0 )
		*battery_lvl = 0;
	#ifdef DEBUG
	printf("battery_lvl:%d\n", *battery_lvl);
	#endif
}

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

void do_sense(struct runicast_conn *runicast, linkaddr_t *recv, int *battery_lvl)
{
	static sensed_data s;

	SENSORS_ACTIVATE(sht11_sensor);
	s.humidity = sht11_sensor.value(SHT11_SENSOR_HUMIDITY)/41;
	s.temperature = (sht11_sensor.value(SHT11_SENSOR_TEMP)/10-396)/10;
	SENSORS_DEACTIVATE(sht11_sensor);

	if(!runicast_is_transmitting(runicast)) {
		packetbuf_copyfrom(&s,sizeof(sensed_data));
		#ifdef DEBUG
		printf("%u.%u: sending runicast to address %u.%u\n", linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1], recv->u8[0], recv->u8[1]);
		#endif
		drain_battery(battery_lvl, SENSE_DRAIN);
		runicast_send(runicast, recv, MAX_RETRANSMISSIONS);
	}
}