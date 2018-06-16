#include "contiki.h"
#include "stdio.h"
#include "sys/etimer.h"
#include "dev/serial-line.h"
#include "dev/button-sensor.h"
#include "dev/light-sensor.h"
#include "dev/sht11/sht11-sensor.h"

PROCESS(Process_1, "exercise31");

AUTOSTART_PROCESSES(&Process_1);

int get_avg(int *arr) {
	int sum=0, i;
	for(i=0; i<5; i++)
		sum += arr[i];

	return sum/5;
}

PROCESS_THREAD(Process_1, ev, data) {
	enum sel_sensor{HUMIDITY, LIGHT};
	static enum sel_sensor mode = HUMIDITY;

	static struct etimer et;

	static int light_values_index = 0;
	static int humidity_values_index = 0;
	static int light_values_count = 0;
	static int humidity_values_count = 0;
	static int light_values[5];
	static int humidity_values[5];

	PROCESS_BEGIN();

	SENSORS_ACTIVATE(button_sensor);
	SENSORS_ACTIVATE(sht11_sensor);

	etimer_set(&et, CLOCK_SECOND*5);

	printf("Starting exercise31.../n");

	while(1) {
		PROCESS_WAIT_EVENT_UNTIL((ev == sensors_event && data == &button_sensor) || etimer_expired(&et));
		if(ev == sensors_event && data == &button_sensor)
		{
			if(mode == HUMIDITY)
			{
				printf("Switching to Light\n");
				mode = LIGHT;
				SENSORS_ACTIVATE(light_sensor);
				SENSORS_DEACTIVATE(sht11_sensor);
			}
			else
			{
				printf("Switching to Humidity\n");
				mode = HUMIDITY;
				SENSORS_ACTIVATE(sht11_sensor);
				SENSORS_DEACTIVATE(light_sensor);
			}

		}
		else
		{
			if(mode == LIGHT)
			{
				light_values[light_values_index] = light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC);
				light_values_index = (light_values_index+1)%5;
				if(light_values_count < 5)
				{
					light_values_count++;
					printf("Light Average: Not enough values\n");
				}
				else
					printf("Light Average: %d\n", get_avg(light_values));
			}
			else if(mode == HUMIDITY)
			{
				humidity_values[humidity_values_index] = sht11_sensor.value(SHT11_SENSOR_HUMIDITY)/41;
				humidity_values_index = (humidity_values_index+1)%5;
				if(humidity_values_count < 5)
				{
					humidity_values_count++;
					printf("Humidity Average: Not enough values\n");
				}
				else
					printf("Humidity Average: %d\n", get_avg(humidity_values));
			}

			etimer_reset(&et);
		}
	}

	PROCESS_END();
}
