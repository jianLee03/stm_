////////////////////
// pid.h

#define WRITE_CHK			0xaa
#define DEFAULT_WRITE_CHK		0x55	// Default setting(write)
#define WRITE_PROTECT_CHK		0x66

///////////////////////////////////////////////////////
// Command : RMID, TMID, ID, PID, Data number, Data.., CHK 
/////////////////////////////////////////////
// PID one-byte data : PID 0~100

#define PID_VER				1
#define PID_DEBUG			2
#define PID_DEFAULT_SET			3
#define PID_REQ_PID_DATA		4
#define PID_TQ_OFF			5
#define PID_BRAKE			6
#define PID_ACK				7
#define PID_BACKWARD_RATIO		8

// 0:PulseIn signal, 1:Run/Brake, 2:Bumper SW
#define PID_PULSE_IN_TYPE		9	

/////////////////////////////////////////
#define PID_COMMAND			10

#define CMD_TQ_OFF			2

#define CMD_BRAKE			4
#define CMD_MAIN_DATA_BC_ON		5
#define CMD_MAIN_DATA_BC_OFF		6

#define CMD_ALARM_RESET			8

#define CMD_POSI_RESET			10
#define CMD_MONITOR_BC_ON		11
#define CMD_MONITOR_BC_OFF		12
#define CMD_IO_MONITOR_BC_ON		13
#define CMD_IO_MONITOR_BC_OFF		14

#define CMD_FAN_ON			15
#define CMD_FAN_OFF			16
#define CMD_CLUTCH_ON			17
#define CMD_CLUTCH_OFF			18 

#define CMD_TAR_VEL_OFF			20
#define CMD_SLOW_START_OFF		21
#define CMD_SLOW_DOWN_OFF		22
#define CMD_CAN_RESEND_ON		23
#define CMD_CAN_RESEND_OFF		24
#define CMD_MAX_LOAD_OFF		25
#define CMD_ENC_OFF			26
#define CMD_LOW_SPEED_LIMIT_OFF		27
#define CMD_HIGH_SPEED_LIMIT_OFF	28

//////////////////////////////////////////////
// Reset to default value of input range.
#define CMD_SPEED_LIMIT_OFF		29	
#define CMD_HALL_PPM_OFF		30	// for DC linear cylinder

#define CMD_CURVE_FITTING_OFF		31
#define CMD_STEP_INPUT_OFF		32
#define CMD_HALL2_PPM_OFF		34	// just MOT2 param. is deleted

#define CMD_DEVELOPER_ON		40
#define CMD_DEVELOPER_OFF		41
#define CMD_PHASE_FIL_ON		42
#define CMD_PHASE_FIL_OFF		43

#define CMD_UI_COM_OFF			44
#define CMD_UI_COM_ON			45
#define CMD_MAX_RPM_OFF			46
#define CMD_HALL_TYPE_OFF		47

#define CMD_LOW_POT_LIMIT_OFF		48
#define CMD_HIGH_POT_LIMIT_OFF		49

#define CMD_MAIN_DATA_BC_ON2		50
#define CMD_MAIN_DATA_BC_OFF2		51
#define CMD_MONITOR_BC_ON2		52
#define CMD_MONITOR_BC_OFF2		53
#define CMD_IO_MONITOR_BC_ON2		54
#define CMD_IO_MONITOR_BC_OFF2		55

#define CMD_SYS_START			56
#define CMD_SYS_STOP			57
#define CMD_SAVE_POSI_N_TIME		58
#define CMD_REGEN_TEST_ON		59
#define CMD_REGEN_TEST_OFF		60
#define CMD_PNT_MAIN_DATA_BC_ON		61
#define CMD_PNT_MAIN_DATA_BC_OFF	62
#define CMD_INIT_SET			63
#define CMD_CLR_ALARM_LOG		64
#define CMD_ENC1_OFF			65
#define CMD_ENC2_OFF			66
#define CMD_EMER_ON			67
#define CMD_EMER_OFF			68
#define CMD_BRAKE1			69
#define CMD_BRAKE2			70
#define CMD_POSI_SS_OFF			71
#define CMD_POSI_SD_OFF			72
#define CMD_INIT_SET_CW			73
#define CMD_INIT_SET_STOP		74
#define CMD_ROBOT_ANG_RESET		75
#define CMD_ROBOT_MONITOR_BC_ON		76
#define CMD_ROBOT_MONITOR_BC_OFF	77
#define CMD_GOTO_BOOT_MODE		78
#define CMD_RESET_SYSTEM		79
#define CMD_BUZZER_ON			80
#define CMD_BUZZER_OFF			81
#define CMD_SET_CEN_POSI		82	// Set the center-posi if the input_mode is POT
#define CMD_SYNC_OFF			83	// Clear sync-type settings
#define CMD_NO_SYSTEM_RESET		84
#define CMD_PNT_IO_MONITOR_ON		85
#define CMD_PNT_IO_MONITOR_OFF		86
#define CMD_AGING_ON			87
#define CMD_AGING_OFF			88 
#define CMD_RESET_TOTAL_CNT		89
#define CMD_INIT_SET2			90
#define CMD_STALL_ALARM_OFF		91
#define CMD_STALL_ALARM_ON		92	// default
#define CMD_PULSE_IN2RUN_BRK_ON		93
#define CMD_PULSE_IN2RUN_BRK_OFF	94
#define CMD_TQ_INIT_SET_CW		95
#define CMD_TQ_INIT_SET_CCW		96
#define CMD_POSI_SET_OFF		97	// delete position offset memory
#define CMD_LOW_BAT_WARN_ON		98
#define CMD_LOW_BAT_WARN_OFF		99
#define CMD_ROBOT_MONITOR2_BC_ON	100	// PID224
#define CMD_ROBOT_MONITOR2_BC_OFF	101
#define CMD_ROBOT_POSI_BC_ON		102	// PID228
#define CMD_ROBOT_POSI_BC_OFF		103
#define CMD_ROBOT_CMD2_BC_ON		104
#define CMD_ROBOT_CMD2_BC_OFF		105
#define CMD_ROBOT_MONITOR_CAN_BC_ON	106
#define CMD_ROBOT_MONITOR_CAN_BC_OFF	107
#define CMD_ROBOT_MONITOR2_CAN_BC_ON	108	
#define CMD_ROBOT_MONITOR2_CAN_BC_OFF	109
#define CMD_ROBOT_POSI_CAN_BC_ON	110
#define CMD_ROBOT_POSI_CAN_BC_OFF	111
#define CMD_ROBOT_CMD2_CAN_BC_ON	112
#define CMD_ROBOT_CMD2_CAN_BC_OFF	113

#define CMD_BUG_ON			114
#define CMD_BUG_OFF			115


// 25.06.10 revival
#define CMD_SET_MIN_LIMIT_POS		116	// ex ver.
#define CMD_CLEAR_MIN_LIMIT_POS		117	// ex ver.
#define CMD_SET_MAX_LIMIT_POS		118
#define CMD_CLEAR_MAX_LIMIT_POS		119

#define CMD_FOC_MONITOR_BC_ON		120
#define CMD_FOC_MONITOR_BC_OFF		121

// for 122~126 is the command for the PDIST80
#define CMD_CHARGING_ON                 122
#define CMD_CHARGING_OFF                123
#define CMD_DISCHARGING_ON              124
#define CMD_DISCHARGING_OFF             125
#define CMD_POWER_OFF                   126
#define CMD_WDT_CLEAR_OFF		127


#define CMD_DOOR_INIT			130
#define CMD_DOOR_CLS			131	// another DCU command
#define CMD_DOOR_OPEN			132
#define CMD_DOOR_OPEN2			133	

///////////////////////////////////////////////
#define PID_SPEED_LIMIT_BY_IO		11	// 40%, 70%, 100%(RC_IN, PULE_IN used)
#define PID_ALARM_RESET			12
#define PID_POSI_RESET			13
#define PID_MAIN_BC_STATUS		14
#define PID_MONITOR_BC_STATUS		15
#define PID_INV_SIGN_CMD		16
#define PID_USE_LIMIT_SW		17
#define PID_INV_SIGN_CMD2		18
#define PID_INV_ALARM			19

// at JSC, the output type is following
// D1: 1~5 output packet type
// D2: (0->don't care(20hz), 1->5Hz, 2->10Hz, 3->20Hz, 4->30Hz, 5->50Hz)
#define PID_JSC_OUT_TYPE		20	// two bytes packet
// JSC_NONE		0
// JSC_RC_DATA		1
// JSC_MONITOR		2
// JSC_PNT_OPEN_CMD	3
// JSC_PNT_VEL_CMD	4

//////////////// for the DCU controller
#define PID_DCU_CMD			20
#define DCU_CMD_INIT_DOOR		1
#define DCU_CMD_OPEN_DOOR		2
#define DCU_CMD_CLS_DOOR		3
#define	DCU_CMD_STOP_DOOR		4
#define DCU_CMD_ALARM_ON		5
#define DCU_CMD_ALARM_OFF		6
#define DCU_CMD_TURN_CW			7
#define DCU_CMD_TURN_CCW		8
#define DCU_CMD_SOL_ON			9	// About 1s solenoid ON
#define DCU_CMD_OPEN_DOOR2		10
#define DCU_CMD_TURN2_CW		11
#define DCU_CMD_READER2			11
#define DCU_CMD_TURN2_CCW		12
#define DCU_CMD_READER1			12
#define DCU_CMD_COUNT_RESET		13
#define DCU_CMD_AGING_DOOR		14	// Toggling

////////////////////////////////////////////
#define PID_HALL_TYPE			21
// HALL_TYPE_4P			0
// HALL_TYPE_8P			1
// HALL_TYPE_10P		2
// HALL_TYPE_12P		3 
// HALL_TYPE_2P			4
// HALL_TYPE_6P			5

#define PID_INV_SIGN_OUT		22
#define PID_INV_SIGN_OUT2		23
#define PID_STOP_STATUS			24
#define PID_INPUT_TYPE			25
// INPUT_ANALOG				0
// INPUT_JS				1
// INPUT_PULSE				2
// INPUT_RC				3
// INPUT_STEP				4
// INPUT_POT				5	// Position control by Potentiometer.

#define PID_POS_SEN_TYPE		26	// POS_SEN_TYPE, position sensor type(hall(0), pot(1), EPosi(2), MENA(3) 
#define PID_LIMIT_STOP_COND2		27	// 0:TqOff, 1:Brake
#define PID_STOP_STATUS2		28
#define PID_USE_LIMIT_SW2		29
#define PID_PRESET_SAVE			30
#define PID_PRESET_RECALL		31
#define PID_SAVE_POSI			32	// for the position save(if motor stop, then save the posi. N time)
#define PID_ROBOT_TYPE			32
#define PID_TOUR_START			33	// 0->stop, 1->start
#define PID_CTRL_STATUS			34
#define PID_INIT_SET			35	// AutoPan Limit detection.
#define PID_START_INV_SIGN		36
#define PID_RUN_INV_SIGN		37
#define PID_REGENERATION		38
#define PID_CTRL_STATUS2		39
#define PID_LIMIT_STOP_COND		40	// 0:TqOff, 1:Brake
#define PID_TQ_LIMIT_SW			41	// Use Tq as limit switchs.
#define PID_POSI_INPUT_MODE		42
#define PID_SINE_CTRL			43
#define PID_TQ_CTRL			44
#define PID_BLUETOOTH			45
#define PID_USE_EPOSI			46
#define PID_PWM_OUT_FAC			47	// 2 or 3
#define PID_DI				48
#define PID_IN_POSITION_OK		49
#define PID_USE_SIGNED_POS		50	// for the MENA control
#define PID_OVERLOAD_ALARM_ON		51
#define PID_DIP_INV			52
#define PID_DIP_OPEN			53
#define PID_DIP_CHG			54
#define PID_RECT_SINE			55
#define PID_TURN_RATIO			56
#define PID_MAX_SS_TIME			57
#define PID_DIR_INV_SIGN		58
#define PID_DELETE_PRESET		59
#define PID_LATE_ALARM			60

#define PID_ROBOT_DO			61	// 2023.01.18
#define PID_SPEED_OUT_TYPE		61	// 0->SPEED_OUT, 1->BUSY_OUT, just read data

#define PID_USE_MAX_TQ			62
#define PID_USE_CLUTCH			63
#define PID_INV_POS_SEN			64
#define PID_HALL2_TYPE			65
#define PID_TQ_RATIO			66
#define PID_BAT_PERCENT			67

#define PID_HALL1_TYPE			68
#define PID_RECT_SINE1			69
#define PID_RECT_SINE2			70
#define PID_USE_ENC_PHASE		71
#define PID_USE_LIMIT_SW1		72
#define PID_STOP_STATUS1		73
#define PID_LIMIT_STOP_COND1		74	// 0:TqOff, 1:Brake
#define PID_COMPLEX_DIR			75	// 1->same direction of both motor.
#define PID_USE_EMER_SW			76	// use run/brake signal as a Emergency SW
#define PID_FORWARD_RATIO		77	// contrary to BACKWARD_RATIO(0~100%)
#define PID_UI_COM			78	// 1->ON, 0->OFF
#define PID_MOT_TYPE			79
#define PID_ENC_INV_DIR			80
#define PID_CYCLIC_FAULT		81
#define PID_SYNC_TYPE			82
#define PID_TWIN_ALARM_ON		83
#define PID_NO_MODBUS			84	// 1->do not use modbus protocol
#define PID_FUNC_INIT_SET_DIR		85
#define PID_USE_RC_LIMIT_SW		86
#define PID_INIT_SET_OK			87

//#define PID_INIT_SET_OPEN_OUT_PERCENT	88
// reserved
#define PID_STOP_ALL_BY_LSW		89	// only MDxxT prg.
#define PID_INT_SPEED_INV_SIGN		90
#define PID_LIMIT_STOP_TURN		91
#define PID_STALL_ALARM_ON		92
#define PID_FOC_ON			93
#define PID_MDJS_MONITOR		94

//////////////////////////////////////////
// PID95~99 1~N byte PID(get the PPID)
// for MDJS and IR board
#define PID_ROBOT_CTRL                  95      // MDJS related PID

#define PPID_REQ_PPID                   4

#define PPID_COMMAND                    10
// for the CMD number
#define CMD_NEAR_OBST_BC_ON		11
#define CMD_NEAR_OBST_BC_OFF		12
#define CMD_STARGAZER_BC_ON		13
#define CMD_STARGAZER_BC_OFF		14

#define PPID_ROBOT_TYPE			11
#define RB_TYPE_2WHEEL			0
#define RB_TYPE_NORMAL         	 	0       // two wheel platform
#define RB_TYPE_4WHEEL          	1       // four wheels type(no handle for each wheel)
#define RB_TYPE_2WHEEL_1HANDLE  	2       // Two wheels and 1handle(like LBOT)
#define RB_TYPE_4WHEEL_4HANDLE  	3

#define PPID_BAT_PERCENT		61
#define PPID_ROBOT_CTRL			96
#define PPID_NEAR_OBST			100	// SMDUI or MDUI used
#define PPID_OBST_RECT			101
#define PPID_STARGAZER_DATA		102
#define PPID_IR_DATA			103
#define PPID_IR_CMD			104
#define PPID_REQ_PPID2			164

///////////////////////////////////////////
#define PID_PPID_CTRL			96	// reserved
#define PID_PPID_CTRL2			97	// reserved
#define PID_FOC_MONITOR			98	// for the FOC control monitor
// 99 reserved		
#define PID_START_STOP			100
			 
/////////////////////////////////////////////////
// PID two-byte data : PID 101 ~ 190 
#define PID_ALARM_TQ			101	// HOLD_TQ
#define PID_ALARM_TQ_DELAY		102
#define PID_CLUTCH_STOP_DELAY		103
#define PID_MAX_OUT			104

// 105, 106 is developer used 
#define PID_SINE_MIN_RPM		105
//#define PID_EXTENDED_CMD		106	// Just extra user usage
#define PID_BAT_LOW_VOLT		106

#define PID_PROP_BRAKE			107
#define PID_SLOW_START1			108
#define PID_SLOW_START2			109		
////////////////////////////////////////////
#define PID_BAT_HIGH_VOLT		110
//#define PID_EXTRA_2BYTE			110
// reserved
#define PID_SLOW_DOWN1			111
#define PID_SLOW_DOWN2			112
#define PID_POSI_SS1			113	
#define PID_POSI_SS2			114
#define PID_POSI_SD1			115
#define PID_POSI_SD2			116	
#define PID_TAR_VEL1			117
#define PID_TAR_VEL2			118
#define PID_IN_POSITION1		119
#define PID_IN_POSITION2		120
#define PID_MAX_RPM1			121
#define PID_MAX_RPM2			122
#define PID_ENC_PPR1			123
#define PID_MIN_SSSD			124
#define PID_HALL_RAW_PPM		125	// Hall sensor pulses per meter(linear cylinder)
#define PID_PPR				126	// for mobile platform, the driving wheel 1turn position
#define PID_ROBOT_STATUS		127	// for the status of motor control, just read data.
// 126~129 reserved

#define PID_VEL_CMD			130
#define PID_VEL_CMD2			131

// at the dual ch. controller, MOT2 is controller with the output of MOT1
// and the MOT1 is just speed control
#define PID_PNT_CMD			132	
#define PNT_CMD_TQ_OFF			0	// for D[0]=CMD_TYPE
#define PNT_CMD_BRAKE			1
#define PNT_CMD_SYNC_VEL		2

#define PID_ID				133	// default : 1
#define PID_OPEN_VEL_CMD		134
#define PID_BAUDRATE			135	// default : 7(115200bps) 
// PID136 reserved
#define PID_ECAN_BITRATE		137	// 50K,100K,250K,500K,1M
#define PID_INT_RPM_DATA		138
#define PID_TQ_DATA			139
#define PID_TQ_CMD			140
#define PID_SYNC_CMD			141
#define PID_SYNC_CMD2			142	// For dual channel controller

#define PID_VOLT_IN			143
#define PID_OPEN_VEL_CMD2		144
#define PID_PHASE_OFFSET		145
#define PID_PRESET_SPEED		146	// 10ea preset speed setting
// PID146, 147, 148 reserved

// 0 no return, 1:Monitor, 2:Ack return
#define PID_RETURN_TYPE			149	
#define RETURN_TYPE_MONITOR		1
#define RETURN_TYPE_ACK			2
#define RETURN_TYPE_IO_MONITOR		3
#define RETURN_TYPE_MAIN_DATA		4
#define RETURN_TYPE_POSI_DATA		5
#define RETURN_TYPE_VEL_DATA		6
#define RETURN_TYPE_EACH_MONITOR	7
#define RETURN_TYPE_EACH_MAIN_DATA	8
#define RETURN_TYPE_PNT_MONITOR1	9
#define RETURN_TYPE_PNT_MONITOR2	10

#define RETURN_TYPE_ROBOT_MONITOR	21
#define RETURN_TYPE_ROBOT_MONITOR2	22
#define RETURN_TYPE_ROBOT_IN		23


#define PID_TQ_PO			150
#define PID_SPEED_PO			151
#define PID_MODULATION_TYPE		152
#define PID_SLOW_START			153
#define PID_SLOW_DOWN			154
#define PID_TAR_VEL			155	// Write target data.
#define PID_ENC_PPR			156	
#define PID_LOW_SPEED_LIMIT		157
#define PID_HIGH_SPEED_LIMIT		158
#define PID_QUICK_SLOW_DOWN		159
#define PID_PWM_OUT			160	// 0~1023 PWM output of OUT3
#define PID_SPEED_RESOLUTION		161
#define PID_DEAD_ZONE			162	// 0~1023
#define PID_READ_ADDR			163
#define PID_REQ_PID_DATA2		164	// PID, and more one variable

#define PID_AUTO_PAN			165
#define PID_DRIVE_RELAY			165	// Used at RLY10

#define PID_REF_RPM			166
#define PID_PV_GAIN			167
#define PID_P_GAIN			168
#define PID_I_GAIN			169
#define PID_ENC_PPR2			170
#define PID_IN_POSITION			171
#define PID_LOW_POT_LIMIT		172
#define PID_HIGH_POT_LIMIT		173
#define PID_PNT_TQ_OFF			174
#define PID_PNT_BRAKE			175
#define PID_TAR_POSI_VEL		176
#define PID_PNT_VEL_DATA		177
#define PID_POSI_SS			178
#define PID_POSI_SD			179
#define PID_COM_TAR_SPEED		180
#define PID_CW_MAX_RPM			181
#define PID_CCW_MAX_RPM			182
#define PID_FUNC_CMD_TYPE		183
#define PID_FUNC_CMD			184
#define PID_COM_WATCH_DELAY		185

#define PID_TQ_LIMIT_SW_VAL		187
// this deleted(2024.04.22)
// #define PID_PNT_POSI_DATA		188
#define PID_REGEN_START_VOLT		189
#define PID_MAX_OPEN_OUT		190

///////////////////////////////////////////////////
// PID N-byte data : PID 191 ~ 253
#define PID_TOUR_DATA			191	// Speed(rpm), delay(10ms, 100->1s)
#define PID_PNT_PROP_BRAKE		192	// 0~1023
#define PID_MAIN_DATA			193	// States(vel, posi,etc)
#define PID_IO_MONITOR			194
#define PID_TAR_POSI			195
#define PID_MONITOR			196	// Velocity. Position.
#define PID_POSI_DATA			197				
#define PID_POSI_SET1			198
#define PID_INC_TAR_POSI		199
#define PID_MAIN_DATA2			200	// Data on 2nd motor
#define PID_MONITOR2			201	// For PNT controller
#define PID_IO_MONITOR2			202			
#define PID_GAIN			203
#define PID_POSI_VEL_DATA		204
#define PID_TYPE			205

//////////// For Pan/Tilt control
#define PID_PNT_POSI_VEL_CMD		206
#define PID_PNT_VEL_CMD			207
#define PID_PNT_OPEN_VEL_CMD		208
#define PID_PNT_TQ_CMD			209
#define PID_PNT_MAIN_DATA		210
#define PID_MAX_LOAD			211	// PID_MAX_CUR(LOAD)
#define PID_VEL_CMD_WITH_SD		212
#define PID_PNT_OPEN_VEL_CMD_WITH_SD	213
#define PID_PNT_VEL_CMD_WITH_SD		214	// Unit is rpm.
#define PID_PNT_INC_POSI_CMD		215
#define PID_PNT_MONITOR			216
#define PID_POSI_SET			217
#define PID_POSI_SET2			218
#define PID_POSI_VEL_CMD		219
#define PID_INC_POSI_VEL_CMD		220	// Incremental posi. cmd.
#define PID_MAX_RPM			221
#define PID_SPEED_LIMIT			222	// Treat low speed limit, high speed limit
#define PID_MIN_RPM			223
#define PID_ROBOT_MONITOR2		224
#define PID_INC_POSI_VEL_CMD2		224
#define PID_STEP_INPUT			225	// No, input.
#define PID_CURVE_PT			226	// No. PtX(int), PtY(int)
#define PID_PRESET_DATA			227	// only position.(recall), if write this item. then set the ForcedPreset()
#define PID_ROBOT_POSI			228	// Left/right wheel position and steering angle of 4 wheels
#define PID_MIN_LIMIT_POS		228	// 25.06.10 revival
#define PID_ALARM_LOG			229
#define PID_REF_POSI			230
#define PID_POSI_MIN_LIMIT		231	// for the position reference input
#define PID_POSI_CEN			232
#define PID_POSI_MAX_LIMIT		233

///////////// Developer accecible only.
#define PID_TIME			234
#define PID_TQ_GAIN			235
#define PID_POSI_VEL_CMD2		236
#define PID_VECTOR_CMD			237	// For the robot command
#define PID_PNT_COMPLEX_CMD		237
#define PID_DCU_STATUS			237	// just read data

// reserved, PID238
#define PID_BMS_MONITOR			238
#define PID_FUNC_SPEED			239
#define PID_POSTURE_DATA		240	// mdbot	
#define PID_MAX_LIMIT_POS		240	// 25.06.10, revival
#define PID_ZOOM_FOCUS_CMD		241
#define PID_PNT_IO_MONITOR		241

//////////// For sinusoidal control
#define PID_PNT_INC_POSI_VEL_CMD	242
#define PID_POSI_CMD			243
#define PID_INC_POSI_CMD		244
#define PID_WRITE_ADDR			245
#define PID_ROBOT_DATA2			245	
#define PID_PNT_POSI_CMD		246
#define PID_ROBOT_PARAM			247
#define PID_POSI_CMD2			247

// special PID
#define PID_DCU_SET_SPEED		248	// Open, Cls speed
#define PID_ROBOT_CMD2			248	
#define PID_ROBOT_IN			249
#define PID_INIT_SET_RPM		249

#define PID_FUNC_POSI			250

#define PID_INC_POSI_CMD2		251	
#define PID_TOTAL_CNT			251
#define PID_GAIN1			252	// Mot1 gain only
#define PID_ROBOT_CMD			252	// MDUI
#define PID_ROBOT_MONITOR		253	// Used at MDUI
#define PID_GAIN2			253	// Mot2 gain only

///////////////////////////////////////////////////
#define PID_RC_DATA			254	// MDUI(REMOTE_CONTROLLER)



////////////////////////////////////////////END
