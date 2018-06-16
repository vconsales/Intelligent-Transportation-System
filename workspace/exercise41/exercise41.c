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

PROCESS(Process_1, "exercise41");
PROCESS(Process_2, "passw");

AUTOSTART_PROCESSES(&Process_1, &Process_2);

int get_avg(int *arr) {
	int sum=0, i;
	for(i=0; i<5; i++)
		sum += arr[i];

	return sum/5;
}

static int tempe_values_index = 0;
static int tempe_values_count = 0;
static int tempe_values[5];

PROCESS_THREAD(Process_1, ev, data) {
	static struct etimer et;

	static int light_values_index = 0;
	static int light_values_count = 0;
	static int light_values[5];

	PROCESS_BEGIN();

	SENSORS_ACTIVATE(light_sensor);
	SENSORS_ACTIVATE(sht11_sensor);

	etimer_set(&et, CLOCK_SECOND*5);

	printf("Starting exercise4.1...\n");
	leds_on(LEDS_GREEN);

	while(1) {
		PROCESS_WAIT_UNTIL(etimer_expired(&et));
		
		light_values[light_values_index] = light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC);
		light_values_index = (light_values_index+1)%5;

		tempe_values[tempe_values_index] = (sht11_sensor.value(SHT11_SENSOR_TEMP)/10-396)/10;
		tempe_values_index = (tempe_values_index+1)%5;
		tempe_values_count++;

		if(light_values_count < 5)
		{
			light_values_count++;
			printf("Light Average: Not enough values\n");
		}
		else
			printf("Light Average: %d\n", get_avg(light_values));
		
		leds_toggle(LEDS_GREEN);
		etimer_reset(&et);
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
