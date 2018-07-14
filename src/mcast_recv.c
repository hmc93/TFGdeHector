/**
* @brief Código - Implementación del receptor UDP multicast.
*
* Permite crear conexiones a grupos multicast.
*
* @author David González Filoso <dgfiloso@b105.upm.es>
* @company B105 - Electronic Systems Lab
*/
#include <string.h>
#include <stdio.h>

#include "espressif/esp_common.h"
#include <espressif/esp_wifi.h>
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/inet.h"
#include "lwip/ip_addr.h"
#include "lwip/udp.h"
#include <lwip/igmp.h>
#include <lwip/netif.h>

#include "mcast_recv.h"
#include "global_def.h"

/*	Prototipos funciones privadas *****************************************/
static void mcast_audio(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
/*	Funciones públicas *****************************************************/

/**typedef void (*udp_recv_fn)(void *arg, struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port);
 * @brief Conexión con un grupo multicast UDP.
 *
 * Crea una conexión y la conecta al grupo multicast indicado.
 *
 * @param bufferQueue_p Cola que usaremos para comunicar los datos de audio
 * @param mcast_ip IP del grupo multicast
 * @param mcast_port Puerto del grupo multicast
 * @return struct mcast_data* Estructura con los datos necesarios por el resto de funciones
 */
struct mcast_data* connect_mcast(QueueHandle_t bufferQueue_p, const char* mcast_ip, int mcast_port)
{
	struct netif* nfp = netif_list;
	struct mcast_data* data_m = (struct mcast_data*)malloc(sizeof(struct mcast_data));
	data_m->mcast_conn = udp_new();
	data_m->local_ip = (struct ip_info*)malloc(sizeof(struct ip_info));
	data_m->mcast_addr = (ip_addr_t*)malloc(sizeof(ip_addr_t));
	data_m->bufferQueue = bufferQueue_p;
	err_t err;

	//	Pasamos la dirección IP de char a estructura
	if ((err=ipaddr_aton(mcast_ip, data_m->mcast_addr)) == 0)
	{
		printf("#MCAST: cannot aton mcast addr\n");
		return NULL;
	}

	//	Obtener IP del dispositivo
	if (!sdk_wifi_get_ip_info(STATION_IF, data_m->local_ip))
	{
		printf("#MCAST: no IP local addr\n");
		return NULL;
	}

	//	Comenzar IGMP en la netif de nuestra interfaz
	while (nfp!=NULL) {
		if ( ip_addr_cmp(&(data_m->local_ip->ip), &(nfp->ip_addr)) ) {
			if (!(nfp->flags & NETIF_FLAG_IGMP)) {
				nfp->flags |= NETIF_FLAG_IGMP;
				err = igmp_start(nfp);
				if (err != ERR_OK) {
					printf("#MCAST: igmp_start on %c%c failed %d\n",nfp->name[0], nfp->name[1],err);
					return NULL;
				}
			}
		}
		nfp = nfp->next;
	}

	//	Comprobamos que se ha creado correctamente el socket UDP
	if (!(data_m->mcast_conn))
	{
		printf("#MCAST: udp_new failed\n");
		return NULL;
	}

	//	Unimos el socket al grupo multicast
	if ((err=igmp_joingroup(&(data_m->local_ip->ip), data_m->mcast_addr)) != ERR_OK)
	{
		printf("#MCAST: igmp_join failed %d\n",err);
		return NULL;
	}

	//	Enlazamos el socket con un puerto
	if ((err=udp_bind(data_m->mcast_conn, IP_ADDR_ANY, mcast_port)) != ERR_OK)
	{
		printf("#MCAST: udp_bind failed %d\n",err);
		return NULL;
	}

	udp_recv(data_m->mcast_conn, mcast_audio, data_m);
	printf("#MCAST: Callback function configured\n");

	return data_m;
}

/**
 *  @brief  Desconectamos el servidor UDP conectado al grupo multicast.
 *
 *  @param data_m Datos globales que debemos liberar y cerrar
 *  @return void
 */
void disconnect_udp(struct mcast_data* data_m)
{
  udp_remove(data_m->mcast_conn);
  printf("#MCAST: Server disconnected\n");
  free(data_m->local_ip);
  free(data_m->mcast_addr);
  vQueueDelete(data_m->bufferQueue);
  free(data_m);
}

/* Funciones privadas *********************************************************************/
static void mcast_audio(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	struct mcast_data* data_m = (struct mcast_data*)arg;

	if( xQueueSendToBack( (data_m->bufferQueue), p, 0 ) != pdPASS )
	{
		printf("#UDP: Cannot send %d bytes to the player\n", (p->len));
	}

	pbuf_free(p);
}



