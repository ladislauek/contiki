#include "contiki.h"
#include "sys/etimer.h"
#include "sys/ctimer.h"
#include "dev/leds.h"
#include "dev/watchdog.h"
#include "random.h"
#include "button-sensor.h"
#include "board-peripherals.h"

#include "dev/adc-sensor.h"
#include "lib/sensors.h"

#include "ti-lib.h"

#include <stdio.h>
#include <stdint.h>

static struct etimer et_adc;

/*---------------------------------------------------------------------------*/
PROCESS(adc_process, "read button process");
AUTOSTART_PROCESSES(&adc_process);

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(adc_process, ev, data)
{

    PROCESS_BEGIN();

    etimer_set(&et_adc, 2*CLOCK_SECOND);

    static struct sensors_sensor *sensor;
    sensor = sensors_find(ADC_SENSOR);

    while(1){
        PROCESS_WAIT_EVENT();
        if(ev == PROCESS_EVENT_TIMER) {
            SENSORS_ACTIVATE(*sensor);
            sensor->configure(ADC_SENSOR_SET_CHANNEL, ADC_COMPB_IN_AUXIO0);
            printf("Valor = %d micro Volts\n", sensor->value(ADC_SENSOR_VALUE) );
            etimer_reset(&et_adc);
            SENSORS_DEACTIVATE(*sensor);
        }
    }

    PROCESS_END();

}
