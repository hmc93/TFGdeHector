/**
 * @brief Programa principal del reproductor de audio codificado en MP3. Se conecta
 * a una red wifi y recibe paquetes a traves de la conexion a una direccion multicast.
 * Los paquetes recibidos son almacenados en un buffer de recepcion para luego ser enviados
 * a un modulo capaz de descodificarlos y reproducirlos.
 *
 * @author Hector Miguel Carretero Carmona <hector@b105.upm.es>
 * @company B105 - Electronic Systems Lab
 */

#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "esp/gpio.h"
#include "esp8266.h"

#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/udp.h"
#include "esp/spi.h"

#include "global_def.h"
#include "mcast_recv.h"
#include "mp3_decoder.h"

#include "port.c"

#include "wifi_config.h"

struct communication_data {
	QueueHandle_t bufferQueue_p;
};

static void recv_task(void *pvParameters);
static void decoder_task(void *pvParameters);
static void wifi_task(void *pvParameters);

static int wifi_alive = 0;
static int deco_ready = 0;
static int espera = 0;


/**
 *  @brief  Funcion que, una vez configurado el wifi, permite al resto de
 *  tareas continuar con su funcionamiento
 *
 *  @return void
 */

void on_wifi_ready() {
	printf("WiFi conectado!!\n");
    gpio_write(2, 1);
	wifi_alive = 1;
}
/**
 *  @brief  Funcion activada cuando el pin DREQ cambia de valor. Provoca la pausa del envio
 *  de paquetes hacia el modulo descodificador
 *
 *  @return void
 */
void my_intr_handler(uint8_t gpio_num) {
		espera = !espera;
}
/**
 *  @brief  Tarea que se encarga de recibir el audio recibido a traves de
 *  la conexion multicast y de almacenarlo en el buffer de recepcion
 *
 *  @param pvParameters	Parámetro que se le pasa al crear la tarea
 *  @return void
 */
static void recv_task(void *pvParameters)
{
	struct communication_data* comm_data = (struct communication_data*)pvParameters;
	struct mcast_data* audio_mcast;

	while((wifi_alive==0) || (deco_ready==0))
	{
		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}

	audio_mcast = connect_mcast(comm_data->bufferQueue_p, MCAST_ADDRESS, MCAST_PORT);
	printf("#MCAST: Audio reception ready!\n");

	while((wifi_alive==1) && (deco_ready==1))
	{
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}

	disconnect_udp(audio_mcast);
}
/**
 *  @brief Funcion que se encarga de configurar el Vs1003 y de enviarle
 *  los paquetes de audio codificado recibidos a través del SPI
 *
 *  @param pvParameters	Parámetro que se le pasa al crear la tarea
 *  @return void
 */
static void decoder_task(void *pvParameters)
{
	struct communication_data* comm_data = (struct communication_data*)pvParameters;
	struct pbuf* p = (struct pbuf*)malloc(sizeof(struct pbuf));
	int i;
	const int xdcs = MP3_XDCS;
	while(wifi_alive == 0)
	{
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
	/*** Configurar SPI y modulo descodificador ***/
	configureMp3();
	printf("#PLAYER: Decoder configured!\n");
	deco_ready = 1;
	/*** Interrupcion para la lectura de DREQ ***/
	gpio_set_interrupt(MP3_DREQ, GPIO_INTTYPE_EDGE_ANY, my_intr_handler);
	gpio_write(xdcs, 0);
	while ((wifi_alive == 1) && (deco_ready == 1))
	{
			if( xQueueReceive( (comm_data->bufferQueue_p), p, 0 ) == pdPASS )
			{
				for (i = 0; i < (p->len); i++){
						while (espera){
							vTaskDelay(10 / portTICK_PERIOD_MS);
						}

					spi_transfer_8(SPI_BUS, *(((uint8_t*)p->payload+i)));
				}
			}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

/**
 *  @brief  Tarea que se encarga de desplegar la herramienta esp-wifi-config,
 *  que configura el módulo en modo AP para que el usuario elija la red wifi
 *  a la que quiere que el modulo se conecte
 *
 *  @param pvParameters	Parámetro que se le pasa al crear la tarea
 *  @return void
 */
static void wifi_task(void *pvParameters)
{
	wifi_config_init("esp8266-AP", "esppassword", on_wifi_ready);
	while (1){
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}

/**
 *  @brief  Función principal del programa
 *
 *  Inicializa las tareas y el buffer de recepción
 *
 */
void user_init(void)
{
	TaskHandle_t *wifiTask_h = (TaskHandle_t*)malloc(sizeof(TaskHandle_t));
	TaskHandle_t *recvTask_h = (TaskHandle_t*)malloc(sizeof(TaskHandle_t));
	TaskHandle_t *decoderTask_h = (TaskHandle_t*)malloc(sizeof(TaskHandle_t));
	struct communication_data* data_m = (struct communication_data*)malloc(sizeof(struct communication_data));

	data_m->bufferQueue_p = xQueueCreate(1000, sizeof(struct pbuf));

	uart_set_baud(0, 115200);
	printf("SDK version:%s\n", sdk_system_get_sdk_version());

	xTaskCreate(&wifi_task, "wifiTask", 1024, NULL, 1, wifiTask_h);
	xTaskCreate(&recv_task, "recvTask", 1024, data_m, 4, recvTask_h);
	xTaskCreate(&decoder_task, "decoderTask", 1024, data_m, 5, decoderTask_h);
}
