//////////////////
// cmd.c
// for the communication with the md controller

DriverStruct Drv[MAX_DRIVER_NUM];

int16_t MDPutAlarmReset(BYTE byID)
{
	PutByteData(MID_BLDC_CTR, MY_MID, byID, PID_COMMAND, CMD_ALARM_RESET, PORT_MD);
	return 1;
}

int16_t PutReqPIDData(BYTE byRMID, BYTE byTMID, BYTE byID, BYTE byPID)
{
	PutByteData(byRMID, byTMID, byID, PID_REQ_PID_DATA, byPID, PORT_MD);
	return 1;
}

int16_t MDPutTqOff(BYTE byID)
{
	PutByteData(MID_BLDC_CTR, MY_MID, byID, PID_TQ_OFF, 1, PORT_MD);
	return 1;
}

int16_t MDPutBrakeCmd(BYTE byID)
{
	PutByteData(MID_BLDC_CTR, MY_MID, byID, PID_BRAKE, 1, PORT_MD);
	return 1;
}

int16_t PutCommand(BYTE byRMID, BYTE byID, BYTE byCmd)
{
	PutByteData(byRMID, MY_MID, byID, PID_COMMAND, byCmd, PORT_MD);
	return 1;
}

//////////////// command to MD 
int16_t MDPutByteCmd(BYTE byID, BYTE byPID, BYTE byD)
{
	PutByteData(MID_BLDC_CTR, MY_MID, byID, byPID, byD, PORT_MD);
	return 1;
}

int16_t MDPutTwoByteCmd(BYTE byID, BYTE byPID, BYTE byD, BYTE byD2)
{
	PutTwoByteData(MID_BLDC_CTR, MY_MID, byID, byPID, byD, byD2, PORT_MD);
	return 1;
}

int16_t MDPutWordCmd(BYTE byID, BYTE byPID, int16_t nData)
{
	PutWordData(MID_BLDC_CTR, MY_MID, byID, byPID, nData, PORT_MD);
	return 1;
}

int16_t MDPutPosiVelCmd(BYTE byID, int32_t lPosi, int16_t nMaxVel)
{ 
	BYTE byD[MAX_PACKET_SIZE];
	IByte iData;
	LByte lData;

	lData = Long2Byte(lPosi);	// int32_t type position data.
	byD[0] = lData.byData1;
	byD[1] = lData.byData2;
	byD[2] = lData.byData3;
	byD[3] = lData.byData4;

	iData = Int2Byte(nMaxVel);
	byD[4] = iData.byLow;
	byD[5] = iData.byHigh;

	PutNByteData(MID_BLDC_CTR, MY_MID, byID, PID_POSI_VEL_CMD, byD, 6, PORT_MD);
	return 1;
}

int16_t MDPutPNTIncPosiCmd(BYTE byID, BYTE fgEN1, int32_t lPos1, int16_t nMaxRPM1,
	BYTE fgEN2, int32_t lPos2, int16_t nMaxRPM2, BYTE byReqType)
{
	BYTE byD[MAX_PACKET_SIZE];
	IByte iData;
	LByte lData;

	byD[0] = fgEN1;
	lData = Long2Byte(lPos1);	// int32_t type position data.
	byD[1] = lData.byData1;
	byD[2] = lData.byData2;
	byD[3] = lData.byData3;
	byD[4] = lData.byData4;
	iData = Int2Byte(nMaxRPM1);
	byD[5] = iData.byLow;
	byD[6] = iData.byHigh;
	byD[7] = fgEN2;
	lData = Long2Byte(lPos2);	// int32_t type position data.
	byD[8] = lData.byData1;
	byD[9] = lData.byData2;
	byD[10] = lData.byData3;
	byD[11] = lData.byData4;
	iData = Int2Byte(nMaxRPM2);
	byD[12] = iData.byLow;
	byD[13] = iData.byHigh;
	byD[14] = byReqType;

	PutNByteData(MID_BLDC_CTR, MY_MID, byID, PID_PNT_INC_POSI_VEL_CMD, byD, 15, PORT_MD);
	return 1;
}

int16_t MDPutPNTPosiCmd(BYTE byID, BYTE fgEN1, int32_t lPos1, int16_t nMaxRPM1,
	BYTE fgEN2, int32_t lPos2, int16_t nMaxRPM2, BYTE byReqType)
{
	BYTE byD[MAX_PACKET_SIZE];
	IByte iData;
	LByte lData;

	byD[0] = fgEN1;
	lData = Long2Byte(lPos1);	// int32_t type position data.
	byD[1] = lData.byData1;
	byD[2] = lData.byData2;
	byD[3] = lData.byData3;
	byD[4] = lData.byData4;
	iData = Int2Byte(nMaxRPM1);
	byD[5] = iData.byLow;
	byD[6] = iData.byHigh;
	byD[7] = fgEN2;
	lData = Long2Byte(lPos2);	// int32_t type position data.
	byD[8] = lData.byData1;
	byD[9] = lData.byData2;
	byD[10] = lData.byData3;
	byD[11] = lData.byData4;
	iData = Int2Byte(nMaxRPM2);
	byD[12] = iData.byLow;
	byD[13] = iData.byHigh;
	byD[14] = byReqType;

	PutNByteData(MID_BLDC_CTR, MY_MID, byID, PID_PNT_POSI_VEL_CMD, byD, 15, PORT_MD);
	return 1;
}

int16_t MDPutVelCmd(BYTE byID, int16_t nVel)
{
	PutWordData(MID_BLDC_CTR, MY_MID, byID, PID_VEL_CMD, nVel, PORT_MD);
	return 1;
}

int16_t MDPutSyncVelCmd(BYTE byID, int16_t nVel)
{
	IByte iData;
	BYTE byD[5];

	byD[0] = PNT_CMD_SYNC_VEL;
	iData = Int2Byte(nVel);
	byD[1] = iData.byLow;
	byD[2] = iData.byHigh;
	PutNByteData(MID_BLDC_CTR, MY_MID, byID, PID_PNT_CMD, byD, 3, PORT_MD);
	return 1;
}

int16_t MDPutOpenVelCmd(BYTE byID, int16_t nOut)
{
	PutWordData(MID_BLDC_CTR, MY_MID, byID, PID_OPEN_VEL_CMD, nOut, PORT_MD);
	return 1;
}

int16_t MDPutPosiSet2(BYTE byID, int32_t lSetPosi)
{
	BYTE byArray[MAX_PACKET_SIZE];
	LByte lData;

	lData = Long2Byte(lSetPosi);	// int32_t type position data.
	byArray[0] = lData.byData1;
	byArray[1] = lData.byData2;
	byArray[2] = lData.byData3;
	byArray[3] = lData.byData4;
	PutNByteData(MID_BLDC_CTR, MY_MID, byID, PID_POSI_SET2, byArray, 4, PORT_MD);
	return 1;
}

int16_t MDPutPosiSet(BYTE byID, int32_t lSetPosi)
{
	BYTE byArray[MAX_PACKET_SIZE];
	LByte lData;

	lData = Long2Byte(lSetPosi);	// int32_t type position data.
	byArray[0] = lData.byData1;
	byArray[1] = lData.byData2;
	byArray[2] = lData.byData3;
	byArray[3] = lData.byData4;
	PutNByteData(MID_BLDC_CTR, MY_MID, byID, PID_POSI_SET, byArray, 4, PORT_MD);
	return 1;
}

int16_t MDPutGain(BYTE byRMID, BYTE byID, int16_t nPVGain, int16_t nPGain, int16_t nIGain)
{
	BYTE byArray[MAX_PACKET_SIZE];
	IByte iData;
    
	iData = Int2Byte(nPVGain);
	byArray[0] = iData.byLow;
	byArray[1] = iData.byHigh;
	iData = Int2Byte(nPGain);
	byArray[2] = iData.byLow;
	byArray[3] = iData.byHigh;
	iData = Int2Byte(nIGain);
	byArray[4] = iData.byLow;
	byArray[5] = iData.byHigh;
	PutNByteData(byRMID, MY_MID, byID, PID_GAIN, byArray, 6, PORT_MD);
	return 1;
}

int16_t MDPutPNTBrake(BYTE byID, BYTE fgPanEn, BYTE fgTiltEn, BYTE byReqID)
{
	BYTE byD[MAX_PACKET_SIZE];

	byD[0] = fgPanEn;
	byD[1] = fgTiltEn;
	byD[2] = byReqID;
	PutNByteData(MID_BLDC_CTR, MY_MID, byID, PID_PNT_BRAKE, byD, 3, PORT_MD);
	return 1;
}

int16_t MDPutPNTTqOff(BYTE byID, BYTE fgPanEn, BYTE fgTiltEn, BYTE byReqID)
{
	BYTE byD[MAX_PACKET_SIZE];

	byD[0] = fgPanEn;
	byD[1] = fgTiltEn;
	byD[2] = byReqID;
	PutNByteData(MID_BLDC_CTR, MY_MID, byID, PID_PNT_TQ_OFF, byD, 3, PORT_MD);
	return 1;
}

int16_t MDPutTqCmd(BYTE byID, int16_t nTq)
{
	BYTE byD[MAX_PACKET_SIZE];
	IByte iData;

	iData = Int2Byte(nTq);
	byD[0] = iData.byLow;
	byD[1] = iData.byHigh;

	PutNByteData(MID_BLDC_CTR, MY_MID, byID, PID_TQ_CMD, byD, 2, PORT_MD);
	return 1;
}

int16_t MDPutPNTTqCmd(BYTE byID, BYTE fgPanEn, int16_t nPan, BYTE fgTiltEn, int16_t nTilt, BYTE byReqID)
{
	BYTE byD[MAX_PACKET_SIZE];
	IByte iData;

	byD[0] = fgPanEn;
	iData = Int2Byte(nPan);
	byD[1] = iData.byLow;
	byD[2] = iData.byHigh;

	byD[3] = fgPanEn;
	iData = Int2Byte(nTilt);
	byD[4] = iData.byLow;
	byD[5] = iData.byHigh;
	byD[6] = byReqID;

	PutNByteData(MID_BLDC_CTR, MY_MID, byID, PID_PNT_TQ_CMD, byD, 7, PORT_MD);
	return 1;
}

int16_t MDPutPNTOpenVelCmd(BYTE byID, BYTE fgPanEn, int16_t nPan, BYTE fgTiltEn, int16_t nTilt, BYTE byReqID)
{
	BYTE byD[MAX_PACKET_SIZE];
	IByte iData;

	byD[0] = fgPanEn;
	iData = Int2Byte(nPan);
	byD[1] = iData.byLow;
	byD[2] = iData.byHigh;

	byD[3] = fgPanEn;
	iData = Int2Byte(nTilt);
	byD[4] = iData.byLow;
	byD[5] = iData.byHigh;
	byD[6] = byReqID;

	PutNByteData(MID_BLDC_CTR, MY_MID, byID, PID_PNT_OPEN_VEL_CMD, byD, 7, PORT_MD);
	return 1;
}

//////////////////////////
// For RC control
int16_t MDPutPNTVelCmd(BYTE byID, BYTE byEnType1, int16_t nVel1, BYTE byEnType2, int16_t nVel2, BYTE fgReq)
{
	BYTE byD[MAX_PACKET_SIZE];
	IByte iData;

	byD[0] = byEnType1;
	iData = Int2Byte(nVel1);
	byD[1] = iData.byLow;
	byD[2] = iData.byHigh;

	byD[3] = byEnType2;
	iData = Int2Byte(nVel2);
	byD[4] = iData.byLow;
	byD[5] = iData.byHigh;
	byD[6] = fgReq;

	PutNByteData(MID_BLDC_CTR, MY_MID, byID, PID_PNT_VEL_CMD, byD, 7, PORT_MD);
	return 1;
}

int16_t GetState(BYTE i, BYTE byData)
{
	Drv[i].fgAlarm = byData & 0x01;
	Drv[i].fgOverVolt = (byData & 0x04)>>2;
	Drv[i].fgOverTemp = (byData & 0x08)>>3;
	Drv[i].fgOverLoad = (byData & 0x10)>>4;
	Drv[i].fgHallFail = (byData & 0x20)>>5;
	Drv[i].fgInvVel = (byData & 0x40)>>6;
	Drv[i].fgStall = (byData & 0x80)>>7;
	Drv[i].byCtrlStatus = byData;
	return 1;
}

int16_t GetState2(BYTE i, BYTE byData)
{
	Drv[i].fgAlarm2 = byData & 0x01;
	Drv[i].fgOverVolt2 = (byData & 0x04)>>2;
	Drv[i].fgOverTemp2 = (byData & 0x08)>>3;
	Drv[i].fgOverLoad2 = (byData & 0x10)>>4;
	Drv[i].fgHallFail2 = (byData & 0x20)>>5;
	Drv[i].fgInvVel2 = (byData & 0x40)>>6;
	Drv[i].fgStall2 = (byData & 0x80)>>7;
	Drv[i].byCtrlStatus2 = byData;
	return 1;
}

////////////////////////////////////////////////////// Received data

int16_t GetMainData(BYTE i, BYTE byData[])
{
	Drv[i].nRPM = Byte2Int(byData[0], byData[1]);
	Drv[i].nTq = Byte2Int(byData[2], byData[3]);
	Drv[i].byType = byData[4];
	Drv[i].nRefRPM = Byte2Int(byData[5], byData[6]);
	Drv[i].nOut = Byte2Int(byData[7], byData[8]);
	GetState(i, byData[9]);
	Drv[i].lPosi = Byte2Long(byData[10], byData[11], byData[12], byData[13]); 
	Drv[i].byTemp = byData[15];
	return 1;
}

int16_t GetMonitor(BYTE i, BYTE byD[], BYTE byDataNum)
{
	Drv[i].nRPM = Byte2Int(byD[0], byD[1]);
	Drv[i].nTq = Byte2Int(byD[2], byD[3]);
	Drv[i].nOut = Byte2Int(byD[4], byD[5]);
	GetState(i, byD[6]);
	Drv[i].lPosi = Byte2Long(byD[7], byD[8], byD[9], byD[10]); 
	return 1;
}

int16_t GetPNTMonitor(BYTE i, BYTE byD[])
{
	// MOT1
	Drv[i].nRPM = Byte2Int(byD[0], byD[1]);
	GetState(i, byD[2]);
	Drv[i].lPosi = Byte2Long(byD[3], byD[4], byD[5], byD[6]); 

	// MOT2
	Drv[i].nRPM2 = Byte2Int(byD[7], byD[8]);
	GetState2(i, byD[9]);
	Drv[i].lPosi2 = Byte2Long(byD[10], byD[11], byD[12], byD[13]); 

	if(i==Log.byID) {
		// Make robot data from com data immedieatly
		MotData2RobotPosture(Drv[i].byCtrlStatus, Drv[i].nRPM, Drv[i].lPosi, 
			Drv[i].byCtrlStatus2, Drv[i].nRPM2, Drv[i].lPosi2);
	}
	return 1;
}

int16_t GetPNTMainData(BYTE i, BYTE byD[])
{
    	Drv[i].nRPM = Byte2Int(byD[0], byD[1]);
    	Drv[i].nTq  = Byte2Int(byD[2], byD[3]);
    	GetState(i, byD[4]);
    	Drv[i].lPosi = Byte2Long(byD[5], byD[6], byD[7], byD[8]); 
    
    	Drv[i].nRPM2 = Byte2Int(byD[9], byD[10]);
    	Drv[i].nTq2  = Byte2Int(byD[11], byD[12]);
    	GetState2(i, byD[13]);
    	Drv[i].lPosi2 = Byte2Long(byD[14], byD[15], byD[16], byD[17]); 

	if(i==Log.byID) {
    		MotData2RobotPosture(Drv[i].byCtrlStatus, Drv[i].nRPM, Drv[i].lPosi, 
        		Drv[i].byCtrlStatus2, Drv[i].nRPM2, Drv[i].lPosi2);
	}
    	return 1;
}

int16_t GetPNTIOMonitor(BYTE i, BYTE byD[])
{
    	Drv[i].fgIntSpeed = byD[0] & BIT0; 
    	Drv[i].fgAlarmReset = (byD[0] & BIT1)>>1;
    	Drv[i].fgDir = (byD[0] & BIT2)>>2;
    	Drv[i].fgRunBrake = (byD[0] & BIT3)>>3;
    	Drv[i].fgStartStop = (byD[0] & BIT4)>>4;
    
    	Drv[i].fgIntSpeed2 = byD[1] & BIT0; 
    	Drv[i].fgDir2 = (byD[1] & BIT2)>>2;
    	Drv[i].fgRunBrake2 = (byD[1] & BIT3)>>3;
    	Drv[i].fgStartStop2 = (byD[1] & BIT4)>>4;

    	Drv[i].nVoltIn = Byte2Int(byD[8], byD[9]);
    	return 1;
}

/////////////////////////////////////////////////////// from jcom.c
// the data is just from ID1, MDT controller
int16_t MDComProc(BYTE byInData[], int16_t nPacketSize) //RS485A
{
	int16_t i, nDataNum;
	BYTE byPID, byID;
	BYTE byD[MAX_PACKET_SIZE];

	if(IsChkSumOK(byInData, nPacketSize)==0) return 0;

	byID = byInData[2];
	byPID = byInData[3];
	Ctrl.byRcvPID = byPID;
	nDataNum = byInData[4];
	if(nDataNum>=MAX_DATA_SIZE) return 0;

	for(i=0; i<nDataNum; i++) byD[i] = byInData[i+5];

	switch(byPID) {
        case PID_ACK:
            	// Delete ack. action.
            	if(byD[0]==MCom.byAckPID) {
                	MCom.fgIsAck = 0;
                	MCom.byAckPID = 0;
            	}
            	Ctrl.byAckPID = byD[0];
            	break;
        case PID_VER: 
            	Drv[byID].byVer = byD[0]; 
            	break;
        case PID_HALL_TYPE:
		Drv[byID].byHallType = byD[0];
		Ctrl.nHallType = byD[0];
		RB.nMotPole = GetPoleNumberFromHallType(Drv[byID].byHallType);
            	break;
        case PID_USE_EPOSI:
            	Drv[byID].fgUseEPosi = byD[0];    
            	break;
	case PID_VOLT_IN:
            	Drv[byID].nVoltIn = Byte2Int(byD[0], byD[1]);
            	break;
	case PID_IN_POSITION_OK:
		Drv[byID].fgInPositionOK = byD[0] & 0x01;	
		Drv[byID].fgInPositionOK2 = (byD[0] & 0x02)>>1;	
		break;	


        //////////////////////////////////////////
        case PID_CTRL_STATUS: 
            Drv[byID].byCtrlStatus = byD[0]; 
            break;
        case PID_MAIN_DATA: 
            GetMainData(byID, byD); 
            break;
        case PID_MONITOR: 
            GetMonitor(byID, byD, nDataNum);
            break;
        case PID_MAX_LOAD: 
            Drv[byID].nMaxTq = Byte2Int(byD[0], byD[1]);
            break;
        case PID_INT_RPM_DATA:
            	Drv[byID].nRPM = Byte2Int(byD[0], byD[1]);
            	break;
        case PID_TQ_DATA:
            	Drv[byID].nTq = Byte2Int(byD[0], byD[1]);
            	break;
        case PID_SLOW_START:
            	Drv[byID].nSlowStart = Byte2Int(byD[0], byD[1]);
		Ctrl.nSlowStart = Drv[byID].nSlowStart;
            	break;
        case PID_SLOW_DOWN:
            	Drv[byID].nSlowDown = Byte2Int(byD[0], byD[1]);
		Ctrl.nSlowDown = Drv[byID].nSlowDown;
            	break;
        case PID_POSI_DATA:
            	Drv[byID].lPosi = Byte2Long(byD[0], byD[1], byD[2], byD[3]);
            	break;
        case PID_STOP_STATUS:
            	Drv[byID].byStopStatus = byD[0]; 
            	break;
        case PID_GAIN:
            	Drv[byID].nPVGain = Byte2Int(byD[0], byD[1]);		
		Ctrl.nPVGain = Drv[byID].nPVGain;	
            	Drv[byID].nPGain = Byte2Int(byD[2], byD[3]);		
		Ctrl.nPGain = Drv[byID].nPGain;
            	Drv[byID].nIGain = Byte2Int(byD[4], byD[5]);		
		Ctrl.nIGain = Drv[byID].nIGain;
            	break;
        case PID_COM_WATCH_DELAY:
            	Drv[byID].nComWatchDelay = Byte2Int(byD[0], byD[1]);
            	break;    
        case PID_USE_LIMIT_SW:
            	Drv[byID].fgUseLSW = byD[0];	
            	break;
        case PID_INV_SIGN_CMD:
            	Drv[byID].fgInvSignCmd = byD[0];	
		// use INT for the SmartUpD..() func.
		Ctrl.nInvSignCmd = byD[0] & 0x01;	
            	break;
        case PID_INV_SIGN_CMD2:
            	Drv[byID].fgInvSignCmd2 = byD[0];	
		Ctrl.nInvSignCmd2 = byD[0] & 0x01;	
            	break;
        case PID_MAX_RPM:
            	Drv[byID].nMaxRPM = Byte2Int(byD[0], byD[1]);    
            	break;
	case PID_ENC_PPR:
		Drv[byID].lENCRawPPR = Byte2Int(byD[0], byD[1]);    
		break;
        case PID_PNT_IO_MONITOR:
            	GetPNTIOMonitor(byID, byD);
            	break;
        case PID_PNT_MAIN_DATA:
            	GetPNTMainData(byID, byD);
            	break;
        case PID_PNT_MONITOR:
            	GetPNTMonitor(byID, byD);
            	break;
	case PID_BMS_MONITOR:
		GetBMSMonitor(byD);
		break;
        default: 
            break;
	}
	return 1;
}

///////////////////////// the end
