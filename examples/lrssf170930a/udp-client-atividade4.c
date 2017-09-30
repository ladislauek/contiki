#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ip/resolv.h"
#include "dev/leds.h"

#include <string.h>
#include <stdbool.h>

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

#define SEND_INTERVAL       5 * CLOCK_SECOND
//#define SEND_INTERVAL       15 * CLOCK_SECOND
#define MAX_PAYLOAD_LEN		40
#define CONN_PORT     8802
#define MDNS 0

#define LED_TOGGLE_REQUEST (0x79)
#define LED_SET_STATE (0x7A)
#define LED_GET_STATE (0x7B)
#define LED_STATE (0x7C)

static char buf[MAX_PAYLOAD_LEN];

static struct uip_udp_conn *client_conn;

#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[UIP_LLH_LEN + UIP_IPH_LEN])
#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client process");
AUTOSTART_PROCESSES(&resolv_process,&udp_client_process);
/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
    char *dados;
    int ledR = 0;
    int ledG = 0;
    if(uip_newdata()) {
        dados = uip_appdata;
        dados[uip_datalen()] = '\0';
        printf("Response from the server: '%s' (%d bytes)\n", dados, uip_datalen());
        if ( dados[0] == LED_SET_STATE ) {
            ledG = dados[1]&0x1;      // ???
            ledR = (dados[1]&0x2)>>1; // ???
            printf("Dados = %d %d\n", ledR, ledG);
            if ( ledR == 1 ) {
                leds_on(LEDS_RED);
            } else if ( ledR == 0 ) {
                leds_off(LEDS_RED);
            }
            if ( ledG == 1 ) {
                leds_on(LEDS_GREEN);
            } else if ( ledG == 0 ) {
                leds_off(LEDS_GREEN);
            }
            //printf("0x%02X \n", dados[1]);
        }
    }
}
/*---------------------------------------------------------------------------*/
static void
timeout_handler(void)
{
    char payload = LED_TOGGLE_REQUEST;

    buf[0] = payload;
    if(uip_ds6_get_global(ADDR_PREFERRED) == NULL) {
      PRINTF("Aguardando auto-configuracao de IP\n");
      return;
    }
    uip_udp_packet_send(client_conn, buf, sizeof(payload));

    PRINTF("Cliente para [");
    PRINT6ADDR(&client_conn->ripaddr);
    PRINTF( "]:%u\n", UIP_HTONS(client_conn->rport) );

}
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("Client IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
#if UIP_CONF_ROUTER
static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;

  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
}
#endif /* UIP_CONF_ROUTER */
/*---------------------------------------------------------------------------*/

#if MDNS

static resolv_status_t
set_connection_address(uip_ipaddr_t *ipaddr)
{
#ifndef UDP_CONNECTION_ADDR
#if RESOLV_CONF_SUPPORTS_MDNS
#define UDP_CONNECTION_ADDR       contiki-udp-server.local
#elif UIP_CONF_ROUTER
#define UDP_CONNECTION_ADDR       fd00:0:0:0:0212:7404:0004:0404
#else
#define UDP_CONNECTION_ADDR       fe80:0:0:0:6466:6666:6666:6666
#endif
#endif /* !UDP_CONNECTION_ADDR */

#define _QUOTEME(x) #x
#define QUOTEME(x) _QUOTEME(x)

    resolv_status_t status = RESOLV_STATUS_ERROR;

    if(uiplib_ipaddrconv(QUOTEME(UDP_CONNECTION_ADDR), ipaddr) == 0) {
        uip_ipaddr_t *resolved_addr = NULL;
        status = resolv_lookup(QUOTEME(UDP_CONNECTION_ADDR),&resolved_addr);
        if(status == RESOLV_STATUS_UNCACHED || status == RESOLV_STATUS_EXPIRED) {
            PRINTF("Attempting to look up %s\n",QUOTEME(UDP_CONNECTION_ADDR));
            resolv_query(QUOTEME(UDP_CONNECTION_ADDR));
            status = RESOLV_STATUS_RESOLVING;
        } else if(status == RESOLV_STATUS_CACHED && resolved_addr != NULL) {
            PRINTF("Lookup of \"%s\" succeded!\n",QUOTEME(UDP_CONNECTION_ADDR));
        } else if(status == RESOLV_STATUS_RESOLVING) {
            PRINTF("Still looking up \"%s\"...\n",QUOTEME(UDP_CONNECTION_ADDR));
        } else {
            PRINTF("Lookup of \"%s\" failed. status = %d\n",QUOTEME(UDP_CONNECTION_ADDR),status);
        }
        if(resolved_addr)
            uip_ipaddr_copy(ipaddr, resolved_addr);
    } else {
        status = RESOLV_STATUS_CACHED;
    }

    return status;
}
#endif

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer et;
  uip_ipaddr_t ipaddr;

  PROCESS_BEGIN();
  PRINTF("UDP client process started\n");

#if UIP_CONF_ROUTER
  set_global_address();
#endif

  print_local_addresses();

#if MDNS
  static resolv_status_t status = RESOLV_STATUS_UNCACHED;
  while(status != RESOLV_STATUS_CACHED) {
      status = set_connection_address(&ipaddr);

      if(status == RESOLV_STATUS_RESOLVING) {
          PROCESS_WAIT_EVENT_UNTIL(ev == resolv_event_found);
      } else if(status != RESOLV_STATUS_CACHED) {
          PRINTF("Can't get connection address.\n");
          PROCESS_YIELD();
      }
  }
#else
  //c_onfigures the destination IPv6 address
  uip_ip6addr(&ipaddr, 0xfd00, 0, 0, 0, 0x212, 0x4b00, 0x791, 0xb681);
  //uip_ip6addr(&ipaddr, 0xfe80, 0, 0, 0, 0x215, 0x2000, 0x0002, 0x2145);
#endif
  /* new connection with remote host */
  client_conn = udp_new(&ipaddr, UIP_HTONS(CONN_PORT), NULL);
  udp_bind(client_conn, UIP_HTONS(CONN_PORT));

  PRINT6ADDR(&client_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n",
	UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));

  etimer_set(&et, SEND_INTERVAL);
  while(1) {
    PROCESS_YIELD();
    if(etimer_expired(&et)) {
      timeout_handler();
      etimer_restart(&et);
    } else if(ev == tcpip_event) {
      tcpip_handler();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
