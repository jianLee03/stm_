//If the communication period is 50ms, the required Baudrate is '57600bps'

//Header and ID Number
#define MOTOR_CONTROLLER_MACHINE_ID   183
#define USER_MACHINE_ID               184
#define ID                            1

//////////////////////////////////////////

//Parameter ID
#define PID_REQ_PID_DATA              4
#define PID_VEL_CMD                   130
#define PID_MAIN_DATA                 193

//////////////////////////////////////////

#define MAX_PACKET_SIZE               22 

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
    BYTE byRMID, byTMID, byID, byPID, byDataNum, byDataSum, byControlType;
    BYTE byMotorStatus, byBrakeOutput, byTemperature, byMotorStatus2;
    int nNowMotorRPM, nCurrent, nReferenceRPM, nControlOutput; 
    long lMotorPosition; 
    
    if(byDataSum == 0)
    {
        // According to PID 193 communication specification

        byRMID         = byInData[0];      //184 <- user machine id
        byTMID         = byInData[1];      //183 <- motor controller machine id
        byID           = byInData[2];      //1   <- id
        byPID          = byInData[3];      //193 <- PID  
        byDataNum      = byInData[4];      //17  <- bytes

        nNowMotorRPM   = Byte2Int(byInData[5], byInData[6]);
        nCurrent       = Byte2Int(byInData[7], byInData[8]);
        byControlType  = byInData[9];
        nReferenceRPM  = Byte2Int(byInData[10], byInData[11]);  
        nControlOutput = Byte2Int(byInData[12], byInData[13]);
        byControlType  = byInData[14];
        lMotorPosition = Byte2Long(byInData[15], byInData[16], byInData[17], byInData[18]);   
        byBrakeOutput  = byInData[19];
        byTemperature  = byInData[20];
        byMotorStatus2 = byInData[21];
    }
    return 1;
}

int PutReqPIDData(int nReqPID)
{
	BYTE byD[MAX_PACKET_SIZE];
	BYTE byChkSum, byDataNum;
	IByte iData;
    int i, j;

    byDataNumber = 1;
		
	byD[i++]  = MOTOR_CONTROLLER_MACHINE_ID;      //RMID
	byD[i++]  = USER_MACHINE_ID;                  //TMID
	byD[i++]  = ID;                               //ID
	byD[i++]  = PID_REQ_PID_DATA;                 //PID
    byD[i++]  = byDataNumber;
	byD[i++]  = nReqPID;

	for(j = 0 ; j <= i; j++) byChkSum += byD[j];
	byD[i] = ~(byChkSum) + 1;
    
    SendData(byD); // Created for user firmware

	return 1;
}

int PutVelCmd(int nRPM)
{
	BYTE byD[MAX_PACKET_SIZE];
	BYTE byChkSum, byDataNum;
	IByte iData;
    int i, j;

    byDataNumber = 2;
		
	byD[i++]  = MOTOR_CONTROLLER_MACHINE_ID;      //RMID
	byD[i++]  = USER_MACHINE_ID;                  //TMID
	byD[i++]  = ID;                               //ID
	byD[i++]  = PID_VEL_CMD;                      //PID
    byD[i++]  = byDataNumber;

	iData     = Int2Byte(nRPM);
	byD[i++]  = iData.byLow;                      //LSB
	byD[i++]  = iData.byHigh;                     //MSB

	for(j = 0 ; j <= i; j++) byChkSum += byD[j];
	byD[i] = ~(byChkSum) + 1;
    
    SendData(byD); // Created for user firmware

	return 1;
}

int Main25msProc()
{
    BYTE byToggle;

    byToggle ^= 1;
    if(byToggle == 0) PutVelCmd(RequiredMotRPM);
    else if(byToggle == 1) PutReqPIDData(PID_MAIN_DATA);

    return 1;
}

int main(void)
{
	while(1) 
    {
        Main25msProc();  //Using a timer
        if(ReceivePacketOK) ReceivePacketAnalyzing(ReceivePacket); // Created for user firmware
	}

	return 1;
}