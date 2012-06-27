#define F_CPU 8000000UL

#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rfm12.h"
#include "crc.h"
#include <util/delay.h>

#define DEVICE_ADDRESS	1
#define TARGET_ADDRESS	2

volatile uint16_t status;
volatile uint8_t rfm_mode = 0;

void rfm_init(uint8_t mode)
{
	SPI_DDR |= 1<<CS_PIN | 1<<MOSI_PIN | 1<<SCK_PIN | 1<<RFM_RES;
	SPI_DDR &= ~(1<<MISO_PIN);
	
	RFM_PORT |= 1<<CS_PIN;
	RFM_PORT &= ~(1<<SCK_PIN | 1<<RFM_RES);

	SPCR |= 1<<SPE | 1<<MSTR;
	
	rfm_reset_module();

	rfm_command(0x80D8);//y	//TX-Register enable, band: 433MHz, load capacitor: 12pF
	rfm_command(0xA620);//y	//Frequency: 433.92MHz
	rfm_command(0xC623);//y	//Data Rate: 9600bit/s --> R=35, cs=0
//	rfm_command(0xC602);//y	//Data Rate: 115200bit/s --> R=2, cs=0
//	rfm_command(0xC605);//y	//Data Rate: 57600bit/s --> R=5, cs=0
	rfm_command(0xC477);//??	//AFC Command
	
	rfm_set_mode(mode);
	rfm_status();
}

void rfm_set_mode(uint8_t mode)
{
	if((mode == TX_MODE) && (rfm_mode != TX_MODE))
	{
		rfm_command(0x8221);//y	//enable transmitter, disable clk-pin
		rfm_command(0x9850);//yy	//frequence deviation: 90 kHz, relative output power: 0dB

		rfm_mode = TX_MODE;
	}
	else if((mode == RX_MODE) && (rfm_mode != RX_MODE))
	{
		rfm_command(0x8281);//y	//enable receiver, disable clk-pin
		rfm_command(0x9761);//??	//VDI output, always on, bandwidth: 270kHz, LNA-gain:0dBm, DRSSI threshold: -97dBm
		rfm_command(0xC2EF);//y	//enable clock recovery auto lock control, filter: digital
		rfm_command(0xCA81);//yy	//FIFO interrupt level: 8, length of synchron pattern: 1, fill start condition: sync pattern
		rfm_command(0xCA83);//yy	//FIFO enable FIFO, hi sensitivity reset mode
	
		rfm_mode = RX_MODE;
	}
}

void rfm_reset_module(void)
{
	RFM_PORT &= ~(1<<RFM_RES);
	_delay_ms(100);
	RFM_PORT |= 1<<RFM_RES;
	_delay_ms(100);
}

void rfm_reset_fifo(void)
{
	rfm_command(0xCA81);
	rfm_command(0xCA83);
}

void rfm_command(uint16_t command)
{
	uint8_t dummy;
	
	RFM_PORT &= ~(1<<CS_PIN);
	
	SPDR = (uint8_t)(command>>8);
	
	while((SPSR & (1<<SPIF)) == 0)
	{
		;
	}
	
	dummy = SPDR;
	
	SPDR = (uint8_t)(command);
	
	while((SPSR & (1<<SPIF)) == 0)
	{
		;
	}
	
	RFM_PORT |= 1<<CS_PIN;
	
	dummy = SPDR;
}

uint16_t rfm_status(void)
{
	uint8_t byte1, byte2;
	uint16_t out;
	
	RFM_PORT &= ~(1<<CS_PIN);
	
	SPDR = 0x00;
	
	while((SPSR & (1<<SPIF)) == 0)
	{
		;
	}
	
	byte1 = SPDR;
	
	SPDR = 0x00;
	
	while((SPSR & (1<<SPIF)) == 0)
	{
		;
	}
	
	RFM_PORT |= 1<<CS_PIN;
	
	byte2 = SPDR;
	
	out = 0;
	
	out |= (uint16_t)(byte1<<8);
	out |= (uint16_t)(byte2);

	return out;
}

void rfm_send_byte_blocking(uint8_t byte)
{
	rfm_set_mode(TX_MODE);
	
	do
	{
		status = rfm_status();
	}while((status & 0x8000) == 0x0000);
	
	RFM_PORT &= ~(1<<CS_PIN);
	
	SPDR = 0xB8;
	
	while((SPSR & (1<<SPIF)) == 0)
	{
		;
	}
	
	SPDR = byte;
	
	while((SPSR & (1<<SPIF)) == 0)
	{
		;
	}
	
	RFM_PORT |= 1<<CS_PIN;
}

uint8_t rfm_recv_byte_blocking(void)
{
	uint8_t out;
	
	rfm_set_mode(RX_MODE);
		
	do
	{
		status = rfm_status();
	}while(((status & 0x8000) == 0x0000));
	
	RFM_PORT &= ~(1<<CS_PIN);
	
	SPDR = 0xB0;
	
	while((SPSR & (1<<SPIF)) == 0)
	{
		;
	}
	
	SPDR = 0x00;
	
	while((SPSR & (1<<SPIF)) == 0)
	{
		;
	}
	
	RFM_PORT |= 1<<CS_PIN;
	
	out = SPDR;
	
	return out;
}

void rfm_send_bytes_blocking(uint8_t* data, uint8_t length)
{
	uint8_t i;
	
	for(i=0; i<length; i++)
	{
		rfm_send_byte_blocking(*(data+i));
	}
}

void rfm_recv_bytes_blocking(uint8_t* data, uint8_t length)
{
	uint8_t i;
	
	for(i=0; i<length; i++)
	{
		*(data+i) = rfm_recv_byte_blocking();
	}
}

void rfm_send_init_transmission(void)
{
	uint8_t i;
	
	rfm_set_mode(TX_MODE);
	
	//Preamble zum sync
	for(i=0; i<3; i++)
	{
		rfm_send_byte_blocking(0xAA);
	}
		
	//sende FIFO pattern 0x2DD4
	rfm_send_byte_blocking(0x2D);
	rfm_send_byte_blocking(0xD4);
}

//Data-Package Methoden
uint8_t sendPackageBlocking(struct DataPackage* pkg)
{
	uint8_t i;
	uint8_t *ptr;
	struct AckPackage* ack;
	
	rfm_send_init_transmission();
	
	ptr = (uint8_t*)pkg;
	
	rfm_send_bytes_blocking(ptr, pkg->pkgSize);
	rfm_send_bytes_blocking(0x00, 2);

	ack = receiveAckPackageBlocking();
	
	if((pkg->pkgNr+1 == ack->pkgNr) && (pkg->scrAddr == ack->dstAddr) && (pkg->dstAddr == ack->scrAddr) && (ack->pkgType == ACK_PKG_TYPE))
	{
		i = ack->pkgNr;
	}
	else
	{
		i = 0;
	}
	
	deleteAckPackage(ack);
	
	return i;
}

struct DataPackage* receivePackageBlocking(void)
{
	uint8_t* ptr;
	uint8_t i;
	struct DataPackage* pkg;
	
	pkg = malloc(sizeof(struct DataPackage));
	memset(pkg, 0x00, sizeof(struct DataPackage));
	ptr = (uint8_t*)pkg;
	
	rfm_set_mode(RX_MODE);
	
	rfm_recv_bytes_blocking(ptr, DATA_PKG_HEAD_SIZE);
	
	if(pkg->pkgSize > DATA_PKG_MAX_SIZE)
	{
		pkg->pkgSize = 0;
	}
	
	rfm_recv_bytes_blocking(ptr+DATA_PKG_HEAD_SIZE, pkg->pkgSize);
	
	rfm_reset_fifo();

	sendAckPackageBlocking(pkg);
	
	return pkg;
}

struct DataPackage* createDataPackage(uint8_t scrAddr, uint8_t dstAddr, uint8_t pkgNr, uint8_t *pkgData, uint8_t dataSize)
{
	uint8_t i;
	struct DataPackage* pkg;
	
	pkg = malloc(sizeof(struct DataPackage));
	
	pkg->scrAddr = scrAddr;
	pkg->dstAddr = dstAddr;
	pkg->pkgType = DATA_PKG_TYPE;
	pkg->pkgSize = dataSize+DATA_PKG_HEAD_SIZE;
	pkg->pkgNr = pkgNr;
	
	if(dataSize > DATA_PKG_MAX_SIZE)
	{
		dataSize = 0;
	}
	
	for(i=0; i<dataSize; i++)
	{
		pkg->pkgData[i] = *(pkgData+i);
	}	

	pkg->crc = crc8((uint8_t*)pkg+1, pkg->pkgSize-1, POLYNOM);
	
	return pkg;
}

void deleteDataPackage(struct DataPackage* pkg)
{
	free(pkg);
}

//ACK-Package Methoden
void sendAckPackageBlocking(struct DataPackage* pkg)
{
	struct AckPackage* ack;
	uint8_t* ptr;
	uint8_t i;
	
	if(checkCrc8((uint8_t*)pkg+1, pkg->pkgSize-1, pkg->crc, POLYNOM) == 0)
	{
		ack = createAckPackage(pkg->dstAddr, pkg->scrAddr, pkg->pkgNr+1);
	}
	else
	{
		ack = createAckPackage(pkg->dstAddr, pkg->scrAddr, pkg->pkgNr);
		pkg->pkgSize = 0;
	}
	
	ptr = (uint8_t*)ack;
	
	rfm_send_init_transmission();
	
	rfm_send_bytes_blocking(ptr, ACK_PKG_LENGTH);
	rfm_send_bytes_blocking(0x00, 2);
	
	deleteAckPackage(ack);
}

struct AckPackage* receiveAckPackageBlocking()
{
	uint8_t i;
	uint8_t *ptr;
	struct AckPackage* ack;
	
	ack = malloc(sizeof(struct AckPackage));
	memset(ack, 0x00, sizeof(struct AckPackage));
	
	ptr = (uint8_t*)ack;
	
	rfm_set_mode(RX_MODE);
	
	rfm_recv_bytes_blocking(ptr, ACK_PKG_LENGTH);
	
	rfm_reset_fifo();
	
	return ack;
}

struct AckPackage* createAckPackage(uint8_t scrAddr, uint8_t dstAddr, uint8_t pkgNr)
{
	struct AckPackage* pkg;
	
	pkg = malloc(sizeof(struct DataPackage));
	
	pkg->scrAddr = scrAddr;
	pkg->dstAddr = dstAddr;
	pkg->pkgType = ACK_PKG_TYPE;
	pkg->pkgNr = pkgNr;
	
	return pkg;
}

void deleteAckPackage(struct AckPackage* pkg)
{
	free(pkg);
}