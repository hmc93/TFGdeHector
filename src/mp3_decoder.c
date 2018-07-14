/**
 * @brief Funciones necesarias para la configuracion del modulo descodificador Vs1003 como modulo
 * 		  esclavo descodificador y reproductor de MP3.
 *
 * @author Hector Miguel Carretero Carmona <hector@b105.upm.es>
 * @company B105 - Electronic Systems Lab
 */

#include "FreeRTOS.h"
#include "task.h"

#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "esp/gpio.h"
#include "esp8266.h"

#include "lwip/udp.h"

#include "mp3_decoder.h"

enum {
  WAITING
};

static void Mp3WriteRegister(uint32_t addressbyte, uint32_t highbyte, uint32_t lowbyte);
/**
 * @brief Funcion que se encarga de configurar el modulo descodificador para su posterior funcionamiento
 * como esclavo controlado a traves de la conexion SPI
 */
void configureMp3(void)
{

	const int xcs = MP3_XCS;
	const int xdcs = MP3_XDCS;
	const int dreq = MP3_DREQ;
	const int xrst = MP3_XRST;

	const spi_settings_t spi_config =  {
	.mode = SPI_MODE0,
	.freq_divider = SPI_FREQ_DIV_2M,
	.msb = true,    //  MSB first
	.endianness = SPI_BIG_ENDIAN,
	.minimal_pins = true    //  only MISO, MOSI and SCK
	};

	if (!spi_set_settings(SPI_BUS, &spi_config))
		{
				printf("#ERROR: Cannot init SPI\n");
		}

	gpio_enable(xcs, GPIO_OUTPUT);
	gpio_enable(xdcs, GPIO_OUTPUT);
	gpio_enable(xrst, GPIO_OUTPUT);
	gpio_enable(dreq, GPIO_INPUT);

	gpio_write(xrst, 0);
	gpio_write(xcs, 1);
	gpio_write(xdcs, 1);
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	gpio_write(xrst, 1);
	vTaskDelay(100 / portTICK_PERIOD_MS);


	Mp3WriteRegister(SCI_MODE, 0x08, 0x0C);
	Mp3WriteRegister(SCI_VOL, 0x00, 0x00);
	Mp3WriteRegister(SCI_CLOCKF, 0xB8, 0x00);
	Mp3WriteRegister(SCI_AUDATA, 0xAC, 0x45);
	//Mp3WriteRegister(SCI_CLOCKF, 0xE0, 0x00); Configuracion alternativa
	return;
}

/**
 * @brief Funcion que se encarga el envio de comandos de escritura al modulo descodificador
 *
 * @params addressbyte: direccion de escritura
 * 		   highbyte: byte mas significativo
 * 		   lowbyte: byte menos significativo
 */
static void Mp3WriteRegister(uint32_t addressbyte, uint32_t highbyte, uint32_t lowbyte)
{
	const int xcs = MP3_XCS;
	const int dreq = MP3_DREQ;
	uint32_t addressbyte_c;
	uint32_t highbyte_c;
	uint32_t command = 0x00000000;
	addressbyte_c = addressbyte << 16;
	highbyte_c = highbyte << 8;
	command = ((0xFF000000 & 0x02000000) | (0x00FF0000 & addressbyte_c) | (0x0000FF00 & highbyte_c) | (0x000000FF & lowbyte));
	while(!gpio_read(dreq)){
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
	gpio_write(xcs, 0);
	spi_transfer_32(SPI_BUS,command);
	gpio_write(xcs, 1);
	while(!gpio_read(dreq)){
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}
