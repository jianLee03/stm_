// com.h
// header for com.c
// 2008.10.25. first born 

#define COM1			0
#define COM2			1
#define COM3			2
#define COM4			3
#define COM5			4
#define COM6			5
#define COM_NUM			6

#define SET_BAUD_9600		1
#define SET_BAUD_19200		2
#define SET_BAUD_38400		3
#define SET_BAUD_57600		4
#define SET_BAUD_115200		5

// MAX data 15EA, etc 6EA->21EA
#define MAX_DATA_SIZE		30
#define MAX_PACKET_SIZE		(MAX_DATA_SIZE+6)	

// 10cnt=>1ms(10Khz)
#define RF_IDLE_TIME		150	// 20ms(For Bluetooth)->10KHz counting
#define MIN_IDLE_TIME		5	// more than 500us
#define MAX_IDLE_TIME		33	// 320000/9600
#define MODBUS_IDLE_NUM		320000	// for 4char. to split modbus packet

// About RS232 serial communication
typedef struct {
	BYTE byBuf[MAX_PACKET_SIZE+2];
	BYTE byInBuf[MAX_PACKET_SIZE+2];
	int16_t nPacketNum, nInPacketNum;
	BYTE fgInPacketOK;

	BYTE fgInIdleLine, fgOutIdleLine;
	BYTE byStep;
	BYTE fgSndOK;

	BYTE fgIsModbusConnected;
	BYTE fgIsMyComConnected; 
	//////////////////////// for TxD control
	BYTE byOutBuf[MAX_PACKET_SIZE+2];
	int16_t cnOutRing, cnOutPut, cnOutGet;

	int16_t nInIdleTime, nOutIdleTime;
	int16_t nModbusIdle, nNormalInIdle;
	BYTE fgComErr;
} Comm;
extern Comm Com[COM_NUM];

extern int16_t PutByte(BYTE i, BYTE byIn);
extern int16_t SendRingByte1(void);
extern int16_t SendRingByte2(void);
extern int16_t SendRingByte3(void);
extern int16_t SendRingByte4(void);

extern void GetUSSChar(BYTE byIn, BYTE byPort);
extern int16_t SendPacket(BYTE byPort, BYTE *byData, int16_t nLen);
extern void GetRcvChar(BYTE byComPort, BYTE byIn);

extern void InitSerialPort1(BYTE bySetBaud);	//Initialize UART1 Module
extern void InitSerialPort2(BYTE bySetBaud);	//Initialize UART2 Module
extern void InitSerialPort3(BYTE bySetBaud);	//Initialize UART3 Module
extern void InitSerialPort4(BYTE bySetBaud);	//Initialize UART4 Module
extern void InitSerialPort5(BYTE bySetBaud);	//Initialize UART5 Module
extern void InitSerialPortOnFutabaSBUS(void);



////////////////////////////



