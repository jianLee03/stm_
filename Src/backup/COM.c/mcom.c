//////////////////////////
// 	com.c

#include "global.h"
#include "sysio.h"
#include "main.h"
#include "default.h"
#include "mytimer.h"
#include "definitions.h"
#include "..\mdui\eprom.h"
#include "com.h"
#include "..\mdui\ctrl.h"
#include "..\mdui\uss.h"
#include "futaba.h"
#include "jcom.h"


Comm Com[COM_NUM];	// Set the global data 

void ResetComBuf(BYTE byPort)
{
	BYTE i;
 
	for(i=0; i<MAX_PACKET_SIZE; i++) {
		Com[byPort].byBuf[i] = 0;
	}
	Com[byPort].byStep = 0;
	Com[byPort].nInIdleTime = Com[byPort].nModbusIdle; 
}

///////////////////////////////////////////////////////////
// Replace the byBuf[i] data to byInBuf[i] for the data stability, to protect change the byInBuf data
void SetComInBuf(BYTE byPacketNum, BYTE byPort)
{
	BYTE i=0;

        for(i=0; i<byPacketNum; i++) {
		Com[byPort].byInBuf[i] = Com[byPort].byBuf[i];
	}
	Com[byPort].nInPacketNum = byPacketNum;
	Com[byPort].fgInPacketOK = 1;
        Com[byPort].fgInIdleLine = 1;
        Com[byPort].byStep = 0;
	Com[byPort].nInIdleTime = Com[byPort].nModbusIdle; 
}

void GetRcvChar(BYTE i, BYTE byIn)
{
	static BYTE byDataNum[COM_NUM], byChkSum[COM_NUM];
	static BYTE byMaxDataNum[COM_NUM];

	if(Com[i].fgInIdleLine) {	// First packet byte input
                if(byIn==MY_MID || byIn==MID_BLDC_CTR || byIn==MID_ALL || byIn==MID_REMOCON || byIn==MID_MMI) {
			Com[i].byStep = 1;
			Com[i].nPacketNum = 0;
			if(i==PORT_RC) Com[i].nInIdleTime = RF_IDLE_TIME; 
			else Com[i].nInIdleTime = Com[i].nNormalInIdle; 
		}
		else return;
	}

	//////////////////////////////////////
	Com[i].fgInIdleLine = 0;
	Cnt.cwComInIdle[i] = 0;

	if(i==PORT_RS485 || i==PORT_RS485B) {
		Com[i].fgOutIdleLine = 0;
		Cnt.cwComOutIdle[i] = 0;
	} 

	if(Com[i].nPacketNum>=MAX_PACKET_SIZE) {
		ResetComBuf(i);
		return;
	}

	switch(Com[i].byStep) {
	case 1: 
		byChkSum[i] = byIn;	
		Com[i].byBuf[Com[i].nPacketNum++] = byIn;
		Com[i].byStep++;
		break;
	case 2:
		byChkSum[i] += byIn;
		Com[i].byBuf[Com[i].nPacketNum++] = byIn;
		Com[i].byStep++;
		break;
	case 3:
		// ID	
		byChkSum[i] += byIn;
		Com[i].byBuf[Com[i].nPacketNum++] = byIn;
		Com[i].byStep++;
		break;
	case 4:	// PID
		byChkSum[i] += byIn;
		Com[i].byBuf[Com[i].nPacketNum++] = byIn;
		Com[i].byStep++;
		break;
	case 5:	// Data num
		byMaxDataNum[i] = byIn;
		if(byMaxDataNum[i]>=MAX_DATA_SIZE) {
			ResetComBuf(i);
			break;
		}
		byDataNum[i] = 0;
		byChkSum[i] += byIn;
		Com[i].byBuf[Com[i].nPacketNum++] = byIn;

		if(byMaxDataNum[i]) Com[i].byStep++;
		else Com[i].byStep = 0;
		break;
	case 6:
		if(++byDataNum[i]>=MAX_DATA_SIZE) {
			ResetComBuf(i);
			break;
		}
		byChkSum[i] += byIn;
		Com[i].byBuf[Com[i].nPacketNum++] = byIn;
		if(byDataNum[i]>=byMaxDataNum[i]) Com[i].byStep++;
		break;
	case 7:	// ChkSum.
		Com[i].byBuf[Com[i].nPacketNum++] = byIn;
		byChkSum[i] += byIn;
		if(byChkSum[i]==0) {
			SetComInBuf(Com[i].nPacketNum, i);
			byDataNum[i] = 0;
		}
		Com[i].byStep = 0;
		Com[i].fgInIdleLine = 1;
		Com[i].fgOutIdleLine = 0;
		Cnt.cwComOutIdle[i] = 0;
		Com[i].nInIdleTime = Com[i].nModbusIdle; 
		break;
	default : 
		break;
	}
}

// RS485
int16_t SendRingByte1(void)
{
	static BYTE fgEndByte;

	if(U1STAbits.TRMT==0) return 0;	// Not empty output buffer.
	if(fgEndByte) {
		Com[COM1].fgSndOK = 1;
		fgEndByte = 0;	// Last byte outted
	}
	if(Com[COM1].cnOutRing<=0) return 0;// there is no out data.

	Com[COM1].fgSndOK = 0;
	Cnt.cwComOutIdle[COM1] = 0;

	U1TXREG = Com[COM1].byOutBuf[Com[COM1].cnOutGet];
	if(++Com[COM1].cnOutGet>=MAX_PACKET_SIZE) Com[COM1].cnOutGet = 0;

	if(Com[COM1].cnOutRing==1) fgEndByte = 1;
	Com[COM1].cnOutRing--;
	return 1;
}	

// TTL232
int16_t SendRingByte2(void)
{
	if(U2STAbits.TRMT==0) return 0;	// Not empty output buffer.
	if(Com[COM2].cnOutRing<=0) return 0;// there is no out data.

	U2TXREG = Com[COM2].byOutBuf[Com[COM2].cnOutGet];
	if(++Com[COM2].cnOutGet>=MAX_PACKET_SIZE) Com[COM2].cnOutGet = 0;

	Com[COM2].cnOutRing--;
	Cnt.cwComOutIdle[COM2] = 0;
	return 1;
}	

// RS232,
int16_t SendRingByte3(void)
{
	if(U3STAbits.TRMT==0) return 0;	// Not empty output buffer.
	if(Com[COM3].cnOutRing<=0) return 0;// there is no out data.

	U3TXREG = Com[COM3].byOutBuf[Com[COM3].cnOutGet];
	if(++Com[COM3].cnOutGet>=MAX_PACKET_SIZE) Com[COM3].cnOutGet = 0;

	Com[COM3].cnOutRing--;
	Cnt.cwComOutIdle[COM3] = 0;
	return 1;
}	

////////////////
// RS485B
int16_t SendRingByte4(void)
{
	static BYTE fgEndByte;

	if(U4STAbits.TRMT==0) return 0;	// Not empty output buffer.
	if(fgEndByte) {
		Com[COM4].fgSndOK = 1;
		fgEndByte = 0;	// Last byte out
	}
	if(Com[COM4].cnOutRing<=0) return 0;// there is no out data.

	Com[COM4].fgSndOK = 0;
	Cnt.cwComOutIdle[COM4] = 0;

	U4TXREG = Com[COM4].byOutBuf[Com[COM4].cnOutGet];
	if(++Com[COM4].cnOutGet>=MAX_PACKET_SIZE) Com[COM4].cnOutGet = 0;

	if(Com[COM4].cnOutRing==1) fgEndByte = 1;
	Com[COM4].cnOutRing--;
	return 1;
}	

int16_t PutByte(BYTE i, BYTE byIn)
{
	Com[i].byOutBuf[Com[i].cnOutPut] = byIn;
        if(++Com[i].cnOutPut>=MAX_PACKET_SIZE) Com[i].cnOutPut = 0;
        Com[i].cnOutRing++;
	return 1;
}

// Send the input data array to UART.
// i : COM_NUM
int16_t SendPacket(BYTE i, BYTE *byData, int16_t nLen)
{
	int16_t k;

	Com[i].fgSndOK = 0;
	Com[i].fgOutIdleLine = 0;// OutIdleLineSetting.
	Cnt.cwComOutIdle[i] = 0;
	for(k=0; k<nLen; k++) PutByte(i, *(byData+k));
	return 1;
}

void InitSerialPort1(BYTE bySetBaud)		//Initialize UART2 Module
{
	int32_t lBaud=0;

	if(bySetBaud==SET_BAUD_9600) {
		U1MODE = 0x8;
		U1BRG = 1562;
		lBaud = 9600;
	}
	else if(bySetBaud==SET_BAUD_38400) {
		U1MODE = 0x8;
		U1BRG = 390;
		lBaud = 38400;
	}
	else if(bySetBaud==SET_BAUD_57600) {
		U1MODE = 0x0;
		U1BRG = 64;
		lBaud = 57600;
	}
	else if(bySetBaud==SET_BAUD_115200) {
		U1MODE = 0x8;
		U1BRG = 129;
		lBaud = 115200;
	}
	else {
		// 19200bps
		U1MODE = 0x8;
		U1BRG = 780;
		lBaud = 19200;
	}

   	U1STASET = (_U1STA_UTXEN_MASK | _U1STA_URXEN_MASK);	
	U1MODESET = _U1MODE_ON_MASK;

	Com[COM1].nModbusIdle = MODBUS_IDLE_NUM/lBaud;	// 6char time(at 40khz)
	Com[COM1].nNormalInIdle = Com[COM1].nModbusIdle + (Com[COM1].nModbusIdle/2);
	if(Com[COM1].nNormalInIdle<MIN_IDLE_TIME) Com[COM1].nNormalInIdle = MIN_IDLE_TIME; 

	Com[COM1].nInIdleTime = Com[COM1].nNormalInIdle; 
	Com[COM1].nOutIdleTime =Com[COM1].nInIdleTime + (Com[COM1].nModbusIdle/2); 

	Drive485(_OFF);
	Com[COM1].fgOutIdleLine = Com[COM1].fgInIdleLine = 1;
}

// TTL232
void InitSerialPort2(BYTE bySetBaud)		//Initialize UART2 Module
{
	int32_t lBaud=0;

	if(bySetBaud==SET_BAUD_9600) {
		U2MODE = 0x8;
		U2BRG = 1562;
		lBaud = 9600;
	}
	else if(bySetBaud==SET_BAUD_38400) {
		U2MODE = 0x8;
		U2BRG = 390;
		lBaud = 38400;
	}
	else if(bySetBaud==SET_BAUD_57600) {
		U2MODE = 0x0;
		U2BRG = 64;
		lBaud = 57600;
	}
	else if(bySetBaud==SET_BAUD_115200) {
		U2MODE = 0x8;
		U2BRG = 129;
		lBaud = 115200;
	}
	else {
		U2MODE = 0x8;
		U2BRG = 780;
		lBaud = 19200;
	}

   	U2STASET = (_U2STA_UTXEN_MASK | _U2STA_URXEN_MASK);	
	U2MODESET = _U2MODE_ON_MASK;


	Com[COM2].nModbusIdle = MODBUS_IDLE_NUM/lBaud;	// 4char time(at 40khz)
	Com[COM2].nNormalInIdle = Com[COM2].nModbusIdle + (Com[COM2].nModbusIdle/2);
	if(Com[COM2].nNormalInIdle<MIN_IDLE_TIME) Com[COM2].nNormalInIdle = MIN_IDLE_TIME; 

	Com[COM2].nInIdleTime = Com[COM2].nNormalInIdle; 
	Com[COM2].nOutIdleTime = Com[COM2].nInIdleTime + (Com[COM2].nModbusIdle/2);
	Com[COM2].fgOutIdleLine = Com[COM2].fgInIdleLine = 1;
}

//////////////////////////////////////////////////
// RS232 
void InitSerialPort3(BYTE bySetBaud)		//Initialize UART2 Module
{
	int32_t lBaud=0;

	if(bySetBaud==SET_BAUD_9600) {
		U3MODE = 0x8;
		U3BRG = 1562;
		lBaud = 9600;
	}
	else if(bySetBaud==SET_BAUD_38400) {
		U3MODE = 0x8;
		U3BRG = 390;
		lBaud = 38400;
	}
	else if(bySetBaud==SET_BAUD_57600) {
		U3MODE = 0x0;
		U3BRG = 64;
		lBaud = 57600;
	}
	else if(bySetBaud==SET_BAUD_115200) {
		U3MODE = 0x8;
		U3BRG = 129;
		lBaud = 115200;
	}
	else {
		U3MODE = 0x8;
		U3BRG = 780;
		lBaud = 19200;
	}

   	U3STASET = (_U3STA_UTXEN_MASK | _U3STA_URXEN_MASK);	
	U3MODESET = _U3MODE_ON_MASK;

	Com[COM3].nModbusIdle = MODBUS_IDLE_NUM/lBaud;	// 10char time(for 10K sampling)
	Com[COM3].nNormalInIdle = Com[COM3].nModbusIdle + (Com[COM3].nModbusIdle/2);
	if(Com[COM3].nNormalInIdle<MIN_IDLE_TIME) Com[COM3].nNormalInIdle = MIN_IDLE_TIME; 

	Com[COM3].nInIdleTime = Com[COM3].nNormalInIdle; 
	Com[COM3].nOutIdleTime = Com[COM3].nInIdleTime + (Com[COM3].nModbusIdle/2);
	Com[COM3].fgOutIdleLine = Com[COM3].fgInIdleLine = 1;
}

void InitSerialPort4(BYTE bySetBaud)		//Initialize UART2 Module
{
	int32_t lBaud=0;

	if(bySetBaud==SET_BAUD_9600) {
		U4MODE = 0x8;
		U4BRG = 1562;
		lBaud = 9600;
	}
	else if(bySetBaud==SET_BAUD_38400) {
		U4MODE = 0x8;
		U4BRG = 390;
		lBaud = 38400;
	}
	else if(bySetBaud==SET_BAUD_57600) {
		U4MODE = 0x0;
		U4BRG = 64;
		lBaud = 57600;
	}
	else if(bySetBaud==SET_BAUD_115200) {
		U4MODE = 0x8;
		U4BRG = 129;
		lBaud = 115200;
	}
	else {
		// 19200bps
		U4MODE = 0x8;
		U4BRG = 780;
		lBaud = 19200;
	}

   	U4STASET = (_U4STA_UTXEN_MASK | _U4STA_URXEN_MASK);	
	U4MODESET = _U4MODE_ON_MASK;

	Com[COM4].nModbusIdle = MODBUS_IDLE_NUM/lBaud;	// 6char time(at 40khz)
	Com[COM4].nNormalInIdle = Com[COM4].nModbusIdle + (Com[COM4].nModbusIdle/2);
	if(Com[COM4].nNormalInIdle<MIN_IDLE_TIME) Com[COM4].nNormalInIdle = MIN_IDLE_TIME; 

	Com[COM4].nInIdleTime = Com[COM4].nNormalInIdle; 
	Com[COM4].nOutIdleTime = Com[COM4].nInIdleTime + (Com[COM4].nModbusIdle/2);

	Drive485B(_OFF);
	Com[COM4].fgOutIdleLine = Com[COM4].fgInIdleLine = 1;
}

///////////////////////////
// TTL232B
void InitSerialPort5(BYTE bySetBaud)		//Initialize UART2 Module
{
	int32_t lBaud=0;

	if(bySetBaud==SET_BAUD_9600) {
		U5MODE = 0x8;
		U5BRG = 1562;
		lBaud = 9600;
	}
	else if(bySetBaud==SET_BAUD_38400) {
		U5MODE = 0x8;
		U5BRG = 390;
		lBaud = 38400;
	}
	else if(bySetBaud==SET_BAUD_57600) {
		U5MODE = 0x0;
		U5BRG = 64;
		lBaud = 57600;
	}
	else if(bySetBaud==SET_BAUD_115200) {
		U5MODE = 0x8;
		U5BRG = 129;
		lBaud = 115200;
	}
	else {
		// 19200bps
		U5MODE = 0x8;
		U5BRG = 780;
		lBaud = 19200;
	}

   	U5STASET = (_U5STA_UTXEN_MASK | _U5STA_URXEN_MASK);	
	U5MODESET = _U5MODE_ON_MASK;

	Com[COM5].nModbusIdle = MODBUS_IDLE_NUM/lBaud;	// 4char time(at 40khz)
	Com[COM5].nNormalInIdle = Com[COM5].nModbusIdle + (Com[COM5].nModbusIdle/2);
	if(Com[COM5].nNormalInIdle<MIN_IDLE_TIME) Com[COM5].nNormalInIdle = MIN_IDLE_TIME; 

	Com[COM5].nInIdleTime = Com[COM5].nNormalInIdle; 
	Com[COM5].nOutIdleTime = Com[COM5].nInIdleTime + (Com[COM5].nModbusIdle/2); 
	Com[COM5].fgOutIdleLine = Com[COM5].fgInIdleLine = 1;
}

// 100,000bps
// 1start bit, 8data bits, even parity, stop bit:2
void InitSerialPortOnFutabaSBUS(void)
{
	int32_t lBaud=0;

   	U5MODE = 0xb;	// 0000 1011 =>8data bits, ever parity, stop bits(2)

    	// Enable UART5 Receiver, Transmitter and TX Interrupt selection 
    	//U5STASET = (_U5STA_UTXEN_MASK | _U5STA_URXEN_MASK | _U5STA_UTXISEL1_MASK );
    	U5STASET = (_U5STA_UTXEN_MASK | _U5STA_URXEN_MASK); 

    	// BAUD Rate register Setup 
    	U5BRG = 129;

	lBaud = 100000;

	U5MODESET = _U5MODE_ON_MASK;

	Com[COM5].nModbusIdle = MODBUS_IDLE_NUM/lBaud;	// 4char time(at 40khz)
	Com[COM5].nNormalInIdle = Com[COM5].nModbusIdle + (Com[COM5].nModbusIdle/4);
	if(Com[COM5].nNormalInIdle<MIN_IDLE_TIME) Com[COM5].nNormalInIdle = MIN_IDLE_TIME; 

	Com[COM5].nInIdleTime = Com[COM5].nNormalInIdle; 
	Com[COM5].nOutIdleTime = Com[COM5].nInIdleTime + (Com[COM5].nModbusIdle/2); 

	Com[COM5].fgOutIdleLine = Com[COM5].fgInIdleLine = 1;
	return;
}
//
//
//
/////////////////////////////////////////////////// END
