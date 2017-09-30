#include "contiki.h"
#include <stdio.h>
#include "dev/leds.h"

#define LED_PING_EVENT (44)
#define LED_PONG_EVENT (45)

static struct etimer et_blink, et_10;

PROCESS(blink_process, "Processo LED verde");
PROCESS(proc3_process, "Processo LED vermelho");
PROCESS(pong_process, "Processo Pong");
AUTOSTART_PROCESSES(&blink_process, &proc3_process, &pong_process);

PROCESS_THREAD(blink_process, ev, data)
{

    PROCESS_BEGIN();
    etimer_set(&et_blink, 2*CLOCK_SECOND);

    while(1) {
        PROCESS_WAIT_EVENT();
        if(ev == PROCESS_EVENT_TIMER) {
            printf("LED verde!\n");
            leds_toggle(LEDS_GREEN);
            process_post(&pong_process, LED_PING_EVENT, (void*)(&blink_process));
            etimer_reset(&et_blink);
        }
        if(ev == LED_PONG_EVENT) {
            printf("blink: Recebido Pong!\n");
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
            process_post(&pong_process, LED_PING_EVENT, (void*)(&proc3_process));
            etimer_reset(&et_10);
        }
        if(ev == LED_PONG_EVENT) {
            printf("proc3: Recebido Pong!\n");
        }
    }
    PROCESS_END();

}

PROCESS_THREAD(pong_process, ev, data)
{

    PROCESS_BEGIN();

    while(1) {
        PROCESS_WAIT_EVENT();
        if(ev == LED_PING_EVENT) {
            struct process *dataProc = (struct process*)data;
            process_post((struct process*)data, LED_PONG_EVENT, NULL);
            printf("Pong: Recebido ping de %s\n", dataProc->name);
            //printf("Pong enviado para %s\n", dataProc->name);
        }
    }

    PROCESS_END();

}
