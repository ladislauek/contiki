/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         A very simple Contiki application showing how Contiki programs look
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"

#include <stdio.h> /* For printf() */
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "sys/etimer.h"

/*---------------------------------------------------------------------------*/
PROCESS(lab_2_process, "lab_2_process");
AUTOSTART_PROCESSES(&lab_2_process);
/*---------------------------------------------------------------------------*/

static struct etimer et;
static int led;

static void reiniciarLED() {
    led = random_rand() % 2;
    if (led == 0) {
        leds_on(LEDS_RED);
        leds_off(LEDS_GREEN);
    } else {
        leds_off(LEDS_RED);
        leds_on(LEDS_GREEN);
    }
}

PROCESS_THREAD(lab_2_process, ev, data)
{
  PROCESS_BEGIN();
  SENSORS_ACTIVATE(button_sensor);
  static int num, pontos = 0;
  random_init(clock_time());

  etimer_set(&et, CLOCK_SECOND * 0.5);
  PROCESS_YIELD();


  reiniciarLED();
  etimer_set(&et, CLOCK_SECOND * 3);

  for (num=0; num < 10; num++) {
    PROCESS_YIELD();
    if (ev == PROCESS_EVENT_TIMER)
    {
        printf("Demorou!\n");
    } else if (ev == sensors_event) {
      if (data == &button_left_sensor) {
        printf("Botao 1 pressionado.\n");
        if (led==0) {
            pontos++;
            printf("OK!\n");
        } else {
            printf("Botao errado!\n");
        }
      } else if (data == &button_right_sensor) {
        printf("Botao 2 pressionado.\n");
        if (led==1) {
            pontos++;
            printf("OK!\n");
        } else {
            printf("Botao errado!\n");
        }
      }
    }
    etimer_restart(&et);
    reiniciarLED();
  }
  printf("Sua pontuacao = %d\n", pontos);
  SENSORS_DEACTIVATE(button_sensor);
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
