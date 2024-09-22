#ifndef _MMC_H__
#define _MMC_H__

//SPI configuration (Platform dependent)
#define SPI_PORT	PORTB
#define SPI_DDR		DDRB
#define DD_MOSI		5 //3
#define DD_SCK		7 //5
#define DD_SS		4 //2

// Defines for SD card SPI access
#define SD_CS_PORT   PORTB
#define SD_CS_DDR    DDRB
#define SD_CS_PIN    4 //2

// Port Controls  (Platform dependent)
#define SELECT()	SD_CS_PORT &= ~(1<<SD_CS_PIN)      /* MMC CS = L */
#define DESELECT()	SD_CS_PORT |=  (1<<SD_CS_PIN)      /* MMC CS = H */ 


/*-----------------------------------------------------------------------*/
/* SPI interace                                                          */
/*-----------------------------------------------------------------------*/
void init_spi (void)
{
	DESELECT();												// set default CS pin	
	SPI_DDR    = (1<<DD_MOSI) | (1<<DD_SCK)| (1<<DD_SS);	// set spi direction
//	SPI_DDR    = (1<<DD_MOSI) | (1<<DD_SCK)| (1<<DD_SS);	// set spi direction
//	SD_CS_DDR |= (1<<SD_CS_PIN);          					// Turns on CS pin as output
    if (!(SPI_DDR & (1<<DD_SS)))							// if SS is input
    	SPI_PORT |= (1<<DD_SS);								//   set internal pull up SS for spi master mode

	SPCR	   = (1<<SPE)|(1<<MSTR); 						// Initialize SPI port (Mode 0)
    SPSR	   = (1<<SPI2X);								// SPI double speed settings
}

BYTE rcv_spi (void)
{
	SPDR = 0xFF;
	loop_until_bit_is_set(SPSR, SPIF);
	return SPDR;
}

/* Alternative macro to receive data fast */
#define rcv_spi_m(dst)	SPDR=0xFF; loop_until_bit_is_set(SPSR,SPIF); *(dst)=SPDR

#define xmit_spi(dat) 	SPDR=(dat); loop_until_bit_is_set(SPSR,SPIF)

#endif
