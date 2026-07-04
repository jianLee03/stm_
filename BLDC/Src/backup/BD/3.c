//If the communication period is 50ms, the required Baudrate is '57600bps'

#define MOTOR_CONTROLLER_MACHINE_ID   183
#define USER_MACHINE_ID               184
#define ID                            1
#define PID_PNT_VEL_CMD               207
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

int Byte2Int(BYTE byLow, BYTE byHigh)
{
	return (byLow | (int)byHigh<<8);
}

long Byte2Long(BYTE byData1, BYTE byData2, BYTE byData3, BYTE byData4)
{
	return((long)byData1 | (long)byData2<<8 | (long)byData3<<16 | 
		(long)byData4<<24);
}

IByte Int2Byte(int nIn)
{
	IByte Ret;

	Ret.byLow = nIn & 0xff;
	Ret.byHigh = nIn>>8 & 0xff;
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

int PutPNTVelCmd(int nLeftRPM, nRightRPM)
{
	BYTE byD[MAX_PACKET_SIZE];
	BYTE byChkSum, byDataNum;
	IByte iData;

    byDataNum = 7;
		
	byD[0]  = MOTOR_CONTROLLER_MACHINE_ID;      //RMID
	byD[1]  = USER_MACHINE_ID;                  //TMID
	byD[2]  = ID;
	byD[3]  = PID_PNT_VEL_CMD;
	byD[4]  = byDataNum;
	byD[5]  = ENABLE;
	iData   = Int2Byte(nLeftRPM);
	byD[6]  = iData.byLow;
	byD[7]  = iData.byHigh;
	byD[8]  = ENABLE;
	iData   = Int2Byte(nRightRPM);
	byD[9]  = iData.byLow;
	byD[10] = iData.byHigh;

	byD[11] = RETURN_PNT_MAIN_DATA;

	for(int i = 0 ; i < 12; i++) byChkSum += byD[i];
	byD[12] = ~(byChkSum) + 1;
    	SendData(byD); // Created for user firmware

	return 1;
}
int Main50msProc()
{
    PutPNTVel(RequiredLeftMotRPM, RequiredRightMotRPM);
    return 1;
}

int main(void)
{
	while(1) 
    {
        Main50msProc();  //Using a timer
        if(ReceivePacketOK) ReceivePacketAnalyzing(ReceivePacket); // Created for user firmware
	}
	return 1;
}