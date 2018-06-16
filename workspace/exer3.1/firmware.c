/*
 * firmware.c
 *
 *  Created on: Apr 6, 2018
 *      Author: user
 */


#include "contiki.h"
#include "dev/light-sensor.h"
#include "dev/button-sensor.h"
#include "dev/sht11/sht11-sensor.h"
#include "sys/etimer.h"
#include "stdio.h"

PROCESS(Process_1, "Process 1");
PROCESS(Process_2, "Process 2");

AUTOSTART_PROCESSES(&Process_1, &Process_2);

unsigned int light_samples = 0;
unsigned int humidity_samples = 0;
int light_values[5];
int humidity_values[5];
int mode = 0;


PROCESS_THREAD(Process_1, ev, data){
    PROCESS_BEGIN();
    while(1){
    	PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
    	if( mode == 0 )
    		mode = 1;
    	else
    		mode = 0;

    }
    PROCESS_END();
}

PROCESS_THREAD(Process_2, ev, data){
    static struct etimer five_timer;
    etimer_set(&five_timer, CLOCK_SECOND*5);
    int i;
	PROCESS_BEGIN();
	while(1){
		printf("processo 2\n");
		PROCESS_WAIT_EVENT();
		if( etimer_expired(&five_timer) ){
			if( mode == 0 ){
				light_values[light_samples%5] = light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC);
				light_samples++;
				if( light_samples < 5 ){
					printf("Not enough light values\n");
				}else {
					for(i=0; i<5; i++){
						printf("%d ",light_values[i]);
						printf("\n");
					}
				}
			} else {
				humidity_values[humidity_samples%5] = sht11_sensor.value(SHT11_SENSOR_HUMIDITY);
				humidity_samples++;
				if( humidity_samples < 5 ){
					printf("Not enough humidity values\n");
				} else {
					for(i=0; i<5; i++){
						printf("%d ",humidity_values[i]);
						printf("\n");
					}
				}
			}
			etimer_reset(&five_timer);
		}		
	}
	PROCESS_END();
}
