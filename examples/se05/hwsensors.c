/*
 * Copyright (c) 2014, Texas Instruments Incorporated - http://www.ti.com/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/

#include "contiki.h"
#include "sys/etimer.h"
#include "sys/ctimer.h"
#include "dev/leds.h"
#include "dev/watchdog.h"
#include "random.h"
#include "button-sensor.h"
#include "board-peripherals.h"

#include "ti-lib.h"

#include <stdio.h>
#include <stdint.h>

static struct etimer et_led;

/*---------------------------------------------------------------------------*/
PROCESS(led_process, "read button process");
AUTOSTART_PROCESSES(&led_process);

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(led_process, ev, data)
{
    PROCESS_BEGIN();
    etimer_set(&et_led, CLOCK_SECOND);

    static int contador = 0;

    IOCPinTypeGpioOutput(IOID_29);
    IOCPinTypeGpioOutput(IOID_30);

    while(1){
        //PROCESS_YIELD();
        PROCESS_WAIT_EVENT();
        if(ev == PROCESS_EVENT_TIMER) {
            GPIO_writeDio(IOID_29, contador&0x1);
            GPIO_writeDio(IOID_30, (contador&0x2)>>1);
            etimer_reset(&et_led);
            printf("Variaveis = %2d %d %d\n", contador, (contador&0x2)>>1, contador&0x1);
            //printf("Bin = %2d\n", contador&0x1);
            contador++;
        }
/*
        if(ev == sensors_event){
            if(data == &button_left_sensor){
                printf("Left Button!\n");
                leds_toggle(LEDS_RED);
            }
            else if(data == &button_right_sensor){
                leds_toggle(LEDS_GREEN);
                printf("Right Button!\n");
            }
        }
*/
    }

    //return 0;

    PROCESS_END();

}
