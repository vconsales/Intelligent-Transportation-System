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

PROCESS(Process_1, "exercise5");

AUTOSTART_PROCESSES(&Process_1);

PROCESS_THREAD(Process_1, ev, data) {
	static struct etimer et5;
	static struct etimer et0_5;
	static int n = 0;
	static int m = 0;
	PROCESS_BEGIN();

	etimer_set(&et5, CLOCK_SECOND*5);
	//etimer_set(&et5, CLOCK_SECOND*0.5);

	printf("Starting exercise5...\n");
	leds_off(LEDS_GREEN);

	while(1) {
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et5));
		if( etimer_expired(&et5) ){
			leds_toggle(LEDS_GREEN);
			etimer_set(&et0_5, CLOCK_SECOND*0.5);
			SENSORS_ACTIVATE(button_sensor);
			n++;
			PROCESS_WAIT_EVENT_UNTIL((ev == sensors_event && data == &button_sensor) || etimer_expired(&et0_5) );
			if( ev == sensors_event && data == &button_sensor ) {
				m++;
				printf("Your score is %d of %d\n",m,n);
			} else {
				printf("Your score is %d of %d\n",m,n);
			}
			leds_toggle(LEDS_GREEN);	
			SENSORS_DEACTIVATE(button_sensor);
		}
		etimer_reset(&et5);
	}

	PROCESS_END();
}


