#include "contiki.h"
#include <stdio.h>
#include "dev/leds.h"
static struct etimer et_blink, et_10;

PROCESS(blink_process, "LED verde");
PROCESS(proc3_process, "LED vermelho");
AUTOSTART_PROCESSES(&blink_process, &proc3_process);

PROCESS_THREAD(blink_process, ev, data)
{

    PROCESS_BEGIN();
    etimer_set(&et_blink, 2*CLOCK_SECOND);

    while(1) {
        PROCESS_WAIT_EVENT();
        if(ev == PROCESS_EVENT_TIMER) {
            printf("LED verde!\n");
            leds_toggle(LEDS_GREEN);
            etimer_reset(&et_blink);
        }
    }
    PROCESS_END();

}

PROCESS_THREAD(proc3_process, ev, data)
{

    PROCESS_BEGIN();
    etimer_set(&et_10, 10*CLOCK_SECOND);

    while(1) {
        PROCESS_WAIT_EVENT();
        if(ev == PROCESS_EVENT_TIMER) {
            printf("LED vermelho!\n");
            leds_toggle(LEDS_RED);
            etimer_reset(&et_10);
        }
    }
    PROCESS_END();

}
