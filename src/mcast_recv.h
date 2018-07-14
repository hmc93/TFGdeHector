/**
* @brief Cabecera - Implementación del receptor UDP multicast.
*
* @file mcast_recv.h
* @version 3.0
* @author David González Filoso <dgfiloso@b105.upm.es>
* @company B105 - Electronic Systems Lab
*/

#ifndef MCAST_RECV_H
#define MCAST_RECV_H

#include "semphr.h"
#include "queue.h"

struct mcast_data {
  struct udp_pcb* mcast_conn;		//!< Socket del servidor
  struct ip_info* local_ip;			//!< Dirección IP del módulo
  ip_addr_t* mcast_addr;			//!< Dirección IP del grupo multicast
  QueueHandle_t bufferQueue;		//!< Cola para comunicar los datos entre tareas
};

//	Conexión con el grupo multicast UDP
struct mcast_data* connect_mcast(QueueHandle_t bufferQueue_p, const char* mcast_ip, int mcast_port);

//	Desconexión con el grupo multicast UDP
void disconnect_udp(struct mcast_data* data_m);

#endif /* MCAST_RECV_H */
