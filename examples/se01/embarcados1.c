#include "contiki.h"
#include <stdio.h>
static struct etimer et_se;

PROCESS(embarcados_process, "Embarcados process");
AUTOSTART_PROCESSES(&embarcados_process);

PROCESS_THREAD(embarcados_process, ev, data)
{

    PROCESS_BEGIN();
    etimer_set(&et_se, 4*CLOCK_SECOND);

    while(1) {
        PROCESS_WAIT_EVENT();
        if(ev == PROCESS_EVENT_TIMER) {
            printf("Sistemas Embarcados!\n");
            etimer_reset(&et_se);
        }
    }
    PROCESS_END();

}
