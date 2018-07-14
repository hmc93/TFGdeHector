/**
 * @brief Definiciones necesarias para la configuracion y el uso del Vs1003 como modulo descodificador y reproductor
 *
 * @author Hector Miguel Carretero Carmona <hector@b105.upm.es>
 * @company B105 - Electronic Systems Lab
 */
#ifndef MP3_DECODER_H
#define MP3_DECODER_H

#include "esp/spi.h"
#include "queue.h"
#include "semphr.h"


//VS10003 Registros SCI
#define SCI_MODE 0x00
#define SCI_STATUS 0x01
#define SCI_BASS 0x02
#define SCI_CLOCKF 0x03
#define SCI_DECODE_TIME 0x04
#define SCI_AUDATA 0x05
#define SCI_WRAM 0x06
#define SCI_WRAMADDR 0x07
#define SCI_HDAT0 0x08
#define SCI_HDAT1 0x09
#define SCI_AIADDR 0x0A
#define SCI_VOL 0x0B
#define SCI_AICTRL0 0x0C
#define SCI_AICTRL1 0x0D
#define SCI_AICTRL2 0x0E
#define SCI_AICTRL3 0x0F

//Bus SPI y pines necesarios para el control del Vs1003
#define SPI_BUS 	1
#define MP3_XCS 	16
#define MP3_XDCS 	4
#define MP3_DREQ 	5
#define MP3_XRST 	0
#define AUX			2

void configureMp3(void);

#endif
