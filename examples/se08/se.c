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

#include "lpm.h"

#include "ti-lib.h"

#include <stdio.h>
#include <stdint.h>

#define LIGHT_EVENT (44)

static struct etimer et_adc;

uint8_t pwm_request_max_pm(void) {
    return LPM_MODE_DEEP_SLEEP;
}

void sleep_enter(void) {
    leds_on(LEDS_RED);
}

void sleep_leave(void) {
    leds_off(LEDS_RED);
}

LPM_MODULE(pwmdrive_module, pwm_request_max_pm, sleep_enter, sleep_leave, LPM_DOMAIN_PERIPH);

int16_t pwminit(int32_t freq) {

    uint32_t load = 0;

    ti_lib_ioc_pin_type_gpio_output(IOID_21);
    leds_off(LEDS_RED);

    /* Enable GPT0 clocks under active mode */
    //ti_lib_prcm_peripheral_run_enable(PRCM_PERIPH_TIMER0);
    //ti_lib_prcm_load_set();
    //while(!ti_lib_prcm_load_get());

    /* Enable GPT0 clocks under active, sleep, deep sleep */
    ti_lib_prcm_peripheral_run_enable(PRCM_PERIPH_TIMER0);
    ti_lib_prcm_peripheral_sleep_enable(PRCM_PERIPH_TIMER0);
    ti_lib_prcm_peripheral_deep_sleep_enable(PRCM_PERIPH_TIMER0);
    ti_lib_prcm_load_set();
    while(!ti_lib_prcm_load_get());

    /* Register with LPM. This will keep the PERIPH PD powered on
    * during deep sleep, allowing the pwm to keep working while the chip is
    * being power-cycled */
    lpm_register_module(&pwmdrive_module);

    /* Drive the I/O ID with GPT0 / Timer A */
    ti_lib_ioc_port_configure_set(IOID_21, IOC_PORT_MCU_PORT_EVENT0, IOC_STD_OUTPUT);

    /* GPT0 / Timer A: PWM, Interrupt Enable */
    ti_lib_timer_configure(GPT0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PWM | TIMER_CFG_B_PWM);

    /* Stop the timers */
    ti_lib_timer_disable(GPT0_BASE, TIMER_A);
    ti_lib_timer_disable(GPT0_BASE, TIMER_B);

    if(freq > 0) {
        load = (GET_MCU_CLOCK / freq);
        ti_lib_timer_load_set(GPT0_BASE, TIMER_A, load);
        ti_lib_timer_match_set(GPT0_BASE, TIMER_A, load-1);
        /* Start */
        ti_lib_timer_enable(GPT0_BASE, TIMER_A);
    }
    return load;
}

PROCESS(pwm_process, "button process");
PROCESS(adc_process, "sensor process");
AUTOSTART_PROCESSES(&pwm_process, &adc_process);

PROCESS_THREAD(pwm_process, ev, data)
{
    PROCESS_BEGIN();
    static int16_t current_duty = 10;
    static int16_t loadvalue;
    static double ticks = 0;
    loadvalue = pwminit(5000);
    while(1) {
        PROCESS_WAIT_EVENT();
        if (ev==LIGHT_EVENT) {
            struct process *dataProc = (struct process*)data;
            //printf("Recebido valor = %d\n", dataProc);
            if ( dataProc == 1 && current_duty < 100 ) {
                current_duty = current_duty + 10;
                printf("Aumentou...\n");
            } else if ( dataProc == -1 && current_duty > 10 ) {
                current_duty = current_duty - 10;
                printf("Diminuiu...\n");
            }
            ticks = (current_duty*loadvalue)/100;
            printf("current_duty = %d\n", current_duty);
        }
        ti_lib_timer_match_set(GPT0_BASE, TIMER_A, loadvalue - ticks);
        /*
        PROCESS_YIELD();
        if (ev == sensors_event) {
            if ( data == &button_left_sensor ) {
                current_duty = current_duty + 10;
                printf("Aumentando...\n");
            } else if ( data == &button_right_sensor ) {
                current_duty = current_duty - 10;
                printf("Diminuindo...\n");
            }
            ticks = (current_duty * loadvalue) / 100;
            printf("CD = %d\n", current_duty);
        }
        ti_lib_timer_match_set(GPT0_BASE, TIMER_A, loadvalue - ticks);
        */
    }
    PROCESS_END();
}

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
            if ( sensor->value(ADC_SENSOR_VALUE) > 3000000 ) {
                process_post(&pwm_process, LIGHT_EVENT, (void*)(1));
            } else if ( sensor->value(ADC_SENSOR_VALUE) < 10000 ) {
                process_post(&pwm_process, LIGHT_EVENT, (void*)(-1));
            }
            //process_post(&pwm_process, LIGHT_EVENT, (void*)(&adc_process));
            etimer_reset(&et_adc);
            SENSORS_DEACTIVATE(*sensor);
        }
    }
    PROCESS_END();
}
