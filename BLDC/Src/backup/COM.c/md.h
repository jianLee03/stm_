///////////////////////////////////////////
//	Filename : md.h
//	2007.11.05, JCLEE


//////////////// BLDC Alarm state.
#define ALARM_BIT		0x01
#define CTRL_FAIL_BIT		0x02
#define OVER_VOLT_BIT		0x04
#define OVER_TEMP_BIT		0x08
#define OVER_LOAD_BIT		0x10
#define HALL_FAIL_BIT		0x20
#define INV_VEL_BIT		0x40
#define STALL_BIT		0x80

#define MAX_DRIVER_NUM		10

typedef struct {
	BYTE byVer, byHallType;
	BYTE byType, byType2;
	BYTE fgDir, fgIntSpeed, fgAlarmReset, fgRunBrake, fgStartStop;
	BYTE fgDir2, fgIntSpeed2, fgAlarmReset2, fgRunBrake2, fgStartStop2;
	BYTE fgAlarm, fgOverTemp;
	BYTE fgTqOff, fgOverLoad, fgHallFail;
	BYTE fgAlarm2, fgOverTemp2;
	BYTE fgTqOff2, fgOverLoad2, fgHallFail2;
	BYTE fgStall, fgStall2;
	BYTE fgOverVolt, fgInvVel;
	BYTE fgOverVolt2, fgInvVel2;
	BYTE byCtrlStatus, byCtrlStatus2;
	BYTE fgInvSignCmd, fgInvSignCmd2;
	BYTE byStopStatus, byStopStatus2;
    
	//////////// For GetIO() function.
	BYTE fgInPositionOK, fgInPositionOK2;

	int16_t nVoltIn;
	BYTE byDipSW;
	BYTE fgUseEPosi;
	BYTE byTemp;
	int32_t lENCRawPPR;
	BYTE fgUseLSW;

	int16_t nRPM, nTq, nMaxTq;
	int16_t nRPM2, nTq2;
	int32_t lPosi, lPosi2;
	int16_t nOut, nOut2;
	int16_t nSlowStart, nSlowDown;
	int16_t nPVGain, nPGain, nIGain;
	int16_t nComWatchDelay;
	int16_t nRefRPM, nRefRPM2;
	int16_t nMaxRPM;
	int16_t nAng, nAng2;	// 0.1deg unit(-3600~3600deg)
} DriverStruct;
extern DriverStruct Drv[MAX_DRIVER_NUM];



extern int16_t MDPutBrakeCmd(BYTE byID);
extern int16_t MDPutByteCmd(BYTE byID, BYTE byPID, BYTE byData);
extern int16_t MDPutWordCmd(BYTE byID, BYTE byPID, int16_t nData);
extern int16_t MDPutTwoByteCmd(BYTE byID, BYTE byPID, BYTE byData1, BYTE byData2);
extern int16_t PutReqPIDData(BYTE byRMID, BYTE byTMID, BYTE byID, BYTE byPID);
extern int16_t MDPutAlarmReset(BYTE byID);
extern int16_t MDPutTqOff(BYTE byID);
extern int16_t MDPutTqCmd(BYTE byID, int16_t nTq);
extern int16_t PutCommand(BYTE byRMID, BYTE byID, BYTE byCmd);
extern int16_t MDPutPosiCmd(BYTE byID, int32_t lTarPosi);
extern int16_t MDPutPosiSet2(BYTE byID, int32_t lSetPosi);
extern int16_t MDPutPosiVelCmd(BYTE byID, int32_t lPosi, int16_t nMaxVel);
extern int16_t MDPutVelCmd(BYTE byID, int16_t nVel);
extern int16_t MDPutSyncVelCmd(BYTE byID, int16_t nVel);
extern int16_t MDPutOpenVelCmd(BYTE byID, int16_t nOut);
extern int16_t MDPutPosiSet(BYTE byID, int32_t lSetPosi);

extern int16_t MDPutPNTVelCmd(BYTE byID, BYTE fgEn1, int16_t nVel1, BYTE fgEn2, int16_t nVel2, BYTE fgREQ);
extern int16_t MDPutPNTTqCmd(BYTE byID, BYTE fgPanEn, int16_t nPan, BYTE fgTiltEn, int16_t nTilt, BYTE byReqID);
extern int16_t MDPutPNTBrake(BYTE byID, BYTE fgPanEn, BYTE fgTiltEn, BYTE byReqID);
extern int16_t MDPutPNTTqOff(BYTE byID, BYTE fgPanEn, BYTE fgTiltEn, BYTE byReqID);
extern int16_t MDPutPNTOpenVelCmd(BYTE byID, BYTE fgPanEn, int16_t nPan, 
	BYTE fgTiltEn, int16_t nTilt, BYTE byReqID);
extern int16_t MDPutPNTPosiCmd(BYTE byID, BYTE fgPanEn, int32_t lPan,  int16_t nTarRPM1,
	BYTE fgTiltEn, int32_t lTilt, int16_t nTarRPM2, BYTE byReqID);
extern int16_t MDPutPNTIncPosiCmd(BYTE byID, BYTE fgEN1, int32_t lPos1, int16_t nMaxRPM1,
	BYTE fgEN2, int32_t lPos2, int16_t nMaxRPM2, BYTE byReqType);
extern int16_t MDPutGain(BYTE byRMID, BYTE byID, int16_t nPVGain, int16_t nPGain, int16_t nIGain);
extern int16_t ReqBMSMonitor(BYTE byRMID, BYTE byPort);

extern int16_t MDComProc(BYTE byInData[], int16_t nPacketSize);

////////////////////////////// end of file
