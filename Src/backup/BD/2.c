
//If the communication period is 50ms, the required Baudrate is '57600bps'

#define MOTOR_CONTROLLER_MACHINE_ID   183
#define USER_MACHINE_ID               184
#define ID                            1
#define PID_PNT_POSI_VEL_CMD          206
#define PID_PNT_MAIN_TOTAL_DATA_NUM   24 

#define PID_MAIN_DATA                 193

#define ENABLE                        1  
#define RETURN_PNT_MAIN_DATA          2   

typedef unsigned char 	BYTE;
typedef unsigned int 	WORD;

typedef struct {
	BYTE byLow;
	BYTE byHigh;
} IByte;

typedef struct {
	BYTE byData1;	// Low Byte
	BYTE byData2;
	BYTE byData3;
	BYTE byData4;	// High Byte
} LByte;

int Byte2Int(BYTE byLow, BYTE byHigh)
{
	return (byLow | (int)byHigh<<8);
}

long Byte2Long(BYTE byData1, BYTE byData2, BYTE byData3, BYTE byData4)
{
	return ((long)byData1 | (long)byData2<<8 | (long)byData3<<16 | 
		(long)byData4<<24);
}

IByte Int2Byte(int nIn)
{
	IByte Ret;

	Ret.byLow = nIn & 0xff;
	Ret.byHigh = nIn>>8 & 0xff;
	return Ret;
}

LByte Long2Byte(long lIn)
{
	LByte Ret;

	Ret.byData1 = lIn & 0xff;
	Ret.byData2 = lIn>>8 & 0xff;
	Ret.byData3 = lIn>>16 & 0xff;
	Ret.byData4 = lIn>>24 & 0xff;
	return Ret;
}

int ReceivePacketAnalyzing(BYTE byInData[])
{
    BYTE byRMID, byTMID, byID, byPID, byDataNum, byDataSum, byLeftMotState, byRightMotState;
    int nRefLeftRPM, nRefRightRPM 
    WORD wRefLeftCurrent, wRefRightCurrent;
    long lLeftMotPosition,lRightMotPosition; 
    
    if(byDataSum == 0)
    {
        byRMID            = byInData[0];      //184 <- user machine id
        byTMID            = byInData[1];      //183 <- motor controller machine id
        byID              = byInData[2];      //1   <- id
        byPID             = byInData[3];      //210 <- PID  
        byDataNum         = byInData[4];      //18  <- bytes

        nRefLeftRPM       = Byte2Int(byInData[5], byInData[6]);
        nRefLeftCurrent   = Byte2Int(byInData[7], byInData[8]);
        byLeftMotState    = byInData[9];
        lLeftMotPosition  = Byte2Long(byInData[10], byInData[11], byInData[12], byInData[13]);
        nRefRightRPM      = Byte2Int(byInData[14], byInData[15]);
        nRefRightCurrent  = Byte2Int(byInData[16], byInData[17]);
        byRightMotState   = byInData[18];
        lRightMotPosition = Byte2Long(byInData[19], byInData[20], byInData[21], byInData[22]);    
    }
    return 1;
}

int PutPNTPosiVelCmd(lLeftMotPosi, lLeftMotRPM, lRightMotPosi, lRightMotRPM)
{
	BYTE byD[MAX_PACKET_SIZE];
	BYTE byChkSum, byDataNum;
	IByte iData;
    LByte lData;

    byDataNum = 15;
		
	byD[0]  = MOTOR_CONTROLLER_MACHINE_ID;      //RMID
	byD[1]  = USER_MACHINE_ID;                  //TMID
	byD[2]  = ID;
	byD[3]  = PID_PNT_POSI_VEL_CMD;
	byD[4]  = byDataNum;
	byD[5]  = ENABLE;
	lData   = Long2Byte(lLeftMotPosi);
	byD[6]  = iData.byData1;
	byD[7]  = iData.byData2;
	byD[8]  = iData.byData3;
	byD[9]  = iData.byData4;
	iData   = Int2Byte(lLeftMotRPM);
	byD[10] = iData.byLow;
	byD[11] = iData.byHigh;
    byD[12] = ENABLE;
    lData   = Long2Byte(lRightMotPosi);
	byD[13] = iData.byData1;
	byD[14] = iData.byData2;
	byD[15] = iData.byData3;
	byD[16] = iData.byData4;
	iData   = Int2Byte(lRightMotRPM);
	byD[17] = iData.byLow;
	byD[18] = iData.byHigh;

	byD[19] = RETURN_PNT_MAIN_DATA;

	for(int i = 0 ; i < 20; i++) byChkSum += byD[i];
	byD[20] = ~(byChkSum) + 1;
    	SendData(byD); // Created for user firmware

	return 1;
}

int Main50msProc()
{
        PutPNTPosiVelCmd(RequiredLeftMotPosi, RequiredLeftMotRPM, 
                        RequiredRightMotPosi, RequiredRightMotRPM);
    return 1;
}

int main(void)
{
	while(1) 
    {
        Main50msProc();  //Using a timer
        if(ReceivePacketOK) ReceivePacketAnalyzing(ReceivingPacket); // Created for user firmware
	}
	return 1;
}