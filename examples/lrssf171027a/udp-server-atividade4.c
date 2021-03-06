#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/rpl/rpl.h"
#include "dev/leds.h"
//#include "button-sensor.h"
//#include "board-peripherals.h"
#include "lpm.h"
#include "ti-lib.h"

#include <string.h>

#define AJUSTAR_CARGA (0x79)
#define ESTADO_CARGA (0x7A)
//#define LED_GET_STATE (0x7B)
//#define LED_STATE (0x7C)

#define EVENTO_CLIENTE (44)

#define CONN_PORT (8802)

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[UIP_LLH_LEN + UIP_IPH_LEN])

#define MAX_PAYLOAD_LEN 120

static struct uip_udp_conn *server_conn;

PROCESS(pwm_process, "PWM process");
PROCESS(udp_server_process, "UDP server process");
AUTOSTART_PROCESSES(&resolv_process,&udp_server_process,&pwm_process);

//uint8_t ledCounter=0;

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

static void
tcpip_handler(void)
{

    char buf[MAX_PAYLOAD_LEN];
    char* msg = (char*)uip_appdata;
    int i;

    if(uip_newdata()) {
        ((char *)uip_appdata)[uip_datalen()] = 0;
        PRINTF("Server received: '%s' from ", msg);
        PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
        PRINTF("\n");

        //uip_ipaddr_copy(&server_conn->ripaddr, &UIP_IP_BUF->srcipaddr);
        //PRINTF("Responding with message: ");
        //sprintf(buf, "Hello from the server! (%d)", ++seq_id);
        //PRINTF("%s\n", buf);

        switch (msg[0])
        {
            case AJUSTAR_CARGA:
            {
                PRINTF("AJUSTAR_CARGA\n");
                uip_ipaddr_copy(&server_conn->ripaddr, &UIP_IP_BUF->srcipaddr);
                server_conn->rport = UIP_UDP_BUF->destport;
                process_post(&pwm_process, EVENTO_CLIENTE, (void*)(msg[1]));
                buf[0] = ESTADO_CARGA;
                buf[1] = msg[1];
                //buf[1] = (ledCounter++)&0x03;
                uip_udp_packet_send(server_conn, buf, 2);
                PRINTF("Enviando ESTADO_CARGA para [");
                PRINT6ADDR(&server_conn->ripaddr);
                PRINTF("]:%u\n", UIP_HTONS(server_conn->rport));
                /* Restore server connection to allow data from any node */
                uip_create_unspecified(&server_conn->ripaddr);
                server_conn->rport = 0;
                break;
            }
    /*
            case LED_STATE:
            {
                if (msg[1]&LEDS_GREEN) {
                    leds_on(LEDS_GREEN);
                } else {
                    leds_off(LEDS_GREEN);
                }
                if (msg[1]&LEDS_RED) {
                    leds_on(LEDS_RED);
                } else {
                    leds_off(LEDS_RED);
                }
                PRINTF("LED_STATE: %s %s\n",(msg[1]&LEDS_GREEN)?" (G) ":"  G  ",(msg[1]&LEDS_RED)?" (R) ":"  R  ");
                break;
            }
    */
            default:
            {
                PRINTF("Comando Invalido: ");
                for(i=0;i<uip_datalen();i++)
                {
                    PRINTF("0x%02X ",msg[i]);
                }
                PRINTF("\n");
                break;
            }
        }
    }

    uip_udp_packet_send(server_conn, buf, strlen(buf));
    /* Restore server connection to allow data from any node */
    memset(&server_conn->ripaddr, 0, sizeof(server_conn->ripaddr));
    return;

}

static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("Server IPv6 addresses: \n");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
    }
  }
}

PROCESS_THREAD(pwm_process, ev, data)
{
    PROCESS_BEGIN();
    static int16_t current_duty = 10;
    static int16_t loadvalue;
    static double ticks = 0;
    loadvalue = pwminit(5000);
    while(1) {
        PROCESS_WAIT_EVENT();
        if (ev==EVENTO_CLIENTE) {
            struct process *dataProc = (struct process*)data;
            //printf("Recebido valor = %d\n", dataProc);
            current_duty = dataProc;
/*
            if ( dataProc == 1 && current_duty < 100 ) {
                current_duty = current_duty + 10;
                printf("Aumentou...\n");
            } else if ( dataProc == -1 && current_duty > 10 ) {
                current_duty = current_duty - 10;
                printf("Diminuiu...\n");
            }
*/
            ticks = (current_duty*loadvalue)/100;
            printf("current_duty = %d\n", current_duty);
        }
        ti_lib_timer_match_set(GPT0_BASE, TIMER_A, loadvalue - ticks);
    }
    PROCESS_END();
}

PROCESS_THREAD(udp_server_process, ev, data)
{

#if UIP_CONF_ROUTER
  uip_ipaddr_t ipaddr;
  rpl_dag_t *dag;
#endif /* UIP_CONF_ROUTER */

  PROCESS_BEGIN();
  PRINTF("UDP server started\n");

#if RESOLV_CONF_SUPPORTS_MDNS
  resolv_set_hostname("contiki-udp-server");
  PRINTF("Setting hostname to contiki-udp-server\n");
#endif

#if UIP_CONF_ROUTER
  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
#endif /* UIP_CONF_ROUTER */

  print_local_addresses();

//#if 1 // UIP_CONF_ROUTER
#if 0 // UIP_CONF_ROUTER >>> Prof Ohara
  dag = rpl_set_root(RPL_DEFAULT_INSTANCE,&uip_ds6_get_global(ADDR_PREFERRED)->ipaddr);
  if(dag != NULL) {
    uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
    rpl_set_prefix(dag, &ipaddr, 64);
    PRINTF("Created a new RPL dag with ID: ");
    PRINT6ADDR(&dag->dag_id);
    PRINTF("\n");
  }
#endif

  server_conn = udp_new(NULL, UIP_HTONS(CONN_PORT), NULL);
  udp_bind(server_conn, UIP_HTONS(CONN_PORT));

  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      tcpip_handler();
    }
  }

  PROCESS_END();

}
