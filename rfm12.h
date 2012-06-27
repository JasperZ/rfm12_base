//ATMEGA32
/*
#define RFM_PORT	PORTB
#define	SPI_DDR		DDRB
#define	CS_PIN		PB4
#define	MOSI_PIN	PB5
#define	MISO_PIN	PB6
#define	SCK_PIN		PB7
#define	RFM_RES		PB3
*/
//ATMEGA8
#define RFM_PORT	PORTB
#define	SPI_DDR		DDRB
#define	CS_PIN		PB2
#define	MOSI_PIN	PB3
#define	MISO_PIN	PB4
#define	SCK_PIN		PB5
#define	RFM_RES		PB0

#define	RX_MODE		1
#define	TX_MODE		2

//DATA Package Defines
#define DATA_PKG_TYPE	0xAA
#define DATA_PKG_MAX_SIZE	250U
#define DATA_PKG_HEAD_SIZE	6U
#define DATA_PKG_MAX_BODY_SIZE	(DATA_PKG_MAX_SIZE-DATA_PKG_HEAD_SIZE)*1U

//ACK Package Defines
#define ACK_PKG_TYPE	0x55
#define ACK_PKG_LENGTH	4U

//CRC Defines
#define POLYNOM 211U

extern volatile uint16_t status;
extern volatile uint8_t rfm_mode;

typedef struct DataPackage
{
	uint8_t crc;
	uint8_t dstAddr;
	uint8_t scrAddr;
	uint8_t pkgType;
	uint8_t pkgNr;
	uint8_t pkgSize;
	uint8_t pkgData[DATA_PKG_MAX_BODY_SIZE];
};

typedef struct AckPackage
{
	uint8_t dstAddr;
	uint8_t scrAddr;
	uint8_t pkgType;
	uint8_t pkgNr;
};

extern void rfm_init(uint8_t mode);
extern void rfm_set_mode(uint8_t mode);
extern void rfm_reset_module(void);
extern void rfm_reset_fifo(void);
extern void rfm_command(uint16_t command);
extern uint16_t rfm_status(void);
extern void rfm_send_byte_blocking(uint8_t byte);
extern uint8_t rfm_recv_byte_blocking(void);
extern uint8_t sendPackageBlocking(struct DataPackage* pkg);
extern struct DataPackage* receivePackageBlocking(void);
extern struct DataPackage* createDataPackage(uint8_t scrAddr, uint8_t dstAddr, uint8_t pkgNr, uint8_t *pkgData, uint8_t dataSize);
extern void deleteDataPackage(struct DataPackage* pkg);
extern struct AckPackage* createAckPackage(uint8_t scrAddr, uint8_t dstAddr, uint8_t pkgNr);
extern void deleteAckPackage(struct AckPackage* pkg);
extern void sendAckPackageBlocking(struct DataPackage* pkg);
extern struct AckPackage* receiveAckPackageBlocking();
