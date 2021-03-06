/**
 * @brief		OIS system command for LC898123 F40
 *
 * @author		Copyright (C) 2016, ON Semiconductor, all right reserved.
 *
 * @file		OisCmd.c
 * @date		svn:$Date:: 2016-06-22 10:57:58 +0900#$
 * @version	svn:$Revision: 59 $
 * @attention
 **/

//**************************
//	Include Header File
//**************************
#define		__OISCMD__

#include	<stdlib.h>	/* use for abs() */
#include	<math.h>	/* use for sqrt() */
#include	"Ois.h"

//****************************************************
//	CUSTOMER NECESSARY CREATING LIST
//****************************************************
/* for I2C communication */
extern	void RamWrite32A(INT_32 addr, INT_32 data);
extern 	void RamRead32A( UINT_16 addr, void * data );
/* for Wait timer [Need to adjust for your system] */
extern void	WitTim( UINT_16 );

//**************************
//	extern  Function LIST
//**************************
UINT_32	UlBufDat[ 64 ] ;							//!< Calibration data write buffer(256 bytes)

//**************************
//	Local Function Prototype
//**************************
void	IniCmd( void ) ;							//!< Command Execute Process Initial
void	IniPtAve( void ) ;							//!< Average setting
void	MesFil( UINT_8 ) ;							//!< Measure Filter Setting
void	MeasureStart( INT_32 , INT_32 , INT_32 ) ;	//!< Measure Start Function
void	MeasureStart2( INT_32 , INT_32 , INT_32 , UINT_16 );	//!< Measure Start 2 Function
void	MeasureWait( void ) ;						//!< Measure Wait
void	MemoryClear( UINT_16 , UINT_16 ) ;			//!< Memory Cloear
void	SetWaitTime( UINT_16 ) ; 					//!< Set Wait Timer

UINT_32	TneOff( UnDwdVal, UINT_8 ) ;				//!< Hall Offset Tuning
UINT_32	TneOff_3Bit( UnDwdVal, UINT_8 , UINT_8 ) ;		//!< 3Bit DAC Hall Offset Tuning
UINT_32	TneBia( UnDwdVal, UINT_8 ) ;				//!< Hall Bias Tuning
UINT_32	TnePtp ( UINT_8	UcDirSel, UINT_8	UcBfrAft );
UINT_32	TneCen( UINT_8	UcTneAxs, UnDwdVal	StTneVal, UINT_8 UcDacSel);
UINT_32	LopGan( UINT_8	UcDirSel );
UINT_32	TneGvc( UINT_8	uc_mode );
UINT_8	TneHvc( void );
void	DacControl( UINT_8 UcMode, UINT_32 UiChannel, UINT_32 PuiData );

#ifdef	NEUTRAL_CENTER_FINE
void	TneFin( void ) ;							//!< Fine tune for natural center offset
#endif	// NEUTRAL_CENTER_FINE

void	RdHallCalData( void ) ;
void	SetSinWavePara( UINT_8 , UINT_8 ) ;			//!< Sin wave test function
void	SetSineWave( UINT_8 , UINT_8 );
void	SetSinWavGenInt( void );
void	SetTransDataAdr( UINT_16  , UINT_32  ) ;	//!< Hall VC Offset Adjust
//void	GetDir( UINT_8 *outX, UINT_8 *outY ) ;
void	MesFil2( UINT_16	UsMesFreq );


//**************************
//	define
//**************************
#define 	MES_XG1			0						//!< LXG1 Measure Mode
#define 	MES_XG2			1						//!< LXG2 Measure Mode

#define 	HALL_ADJ		0
#define 	LOOPGAIN		1
#define 	THROUGH			2
#define 	NOISE			3
#define		OSCCHK			4
#define		GAINCURV		5
#define		SELFTEST		6
#define 	LOOPGAIN2		7

// Measure Mode

 #define 	TNE 			80						//!< Waiting Time For Movement

 /******* Hall calibration Type 1 *******/

 #define 	OFFSET_DIV		2						//!< Divide Difference For Offset Step
 #define 	OFFSET_DIV_3BIT	0x2000					//!< Divide Difference For Offset Step
 #define 	TIME_OUT		40						//!< Time Out Count

 #define	BIAS_HLMT		(UINT_32)0xBF000000
 #define	BIAS_LLMT		(UINT_32)0x20000000

 /******* Hall calibration Type 2 *******/
 #define 	MARGIN			0x0300					//!< Margin

 #define 	BIAS_ADJ_RANGE_XY	0xA664				//!< 65%
// #define 	BIAS_ADJ_RANGE_XY	0xB332				//!< 70%

 #define 	HALL_MAX_RANGE_XY	BIAS_ADJ_RANGE_XY + MARGIN
 #define 	HALL_MIN_RANGE_XY	BIAS_ADJ_RANGE_XY - MARGIN


#ifdef	SEL_CLOSED_AF
 #define	BIAS_ADJ_RANGE_Z	0xAFDE				//!< 68.7%
 #define 	HALL_MAX_RANGE_Z	BIAS_ADJ_RANGE_Z + MARGIN
 #define 	HALL_MIN_RANGE_Z	BIAS_ADJ_RANGE_Z - MARGIN
#endif

 #define 	DECRE_CAL		0x0100					//!< decrease value

#define		MESHGH	0x0630
#define		MESLOW	0x0634

/************** posture check ************/
#define		SENSITIVITY		2048	// LSB/g
#define		PSENS_MARG		(2048 / 4)	// 1/4g
#define		POSTURETH_P		(SENSITIVITY - PSENS_MARG)	// LSB/g
#define		POSTURETH_M		(-POSTURETH_P)				// LSB/g
/************** posture check ************/


/***************************************/
//#define		SLT_OFFSET		(0x1000)
#define		SLT_OFFSET		(0x0C30)
#define		VRT_OFFSET		(0xF3D0)
#define		HRZ_OFFSET		(0xF3D0)
#define		LENS_MARGIN		(0x0800)
#define		PIXEL_SIZE		(1.40f)							// pixel size (um)
#define		SPEC_RANGE		(158.0f)						// TBD spec need movable range @ 1.4deg (+-um)
#define		SPEC_PIXEL		(PIXEL_SIZE / SPEC_RANGE)		// TBD spec need movable range pixel
/***************************************/
// Threshold of osciration amplitude
#define ULTHDVAL	0x01000000								// Threshold of the hale value

//**************************
//	Global Variable
//**************************
INT_16		SsNvcX = 1 ;									// NVC move direction X
INT_16		SsNvcY = 1 ;									// NVC move direction Y

//**************************
//	Const
//**************************
const UINT_8	UcDacArray[ 7 ]	= {
	0x06,
	0x05,
	0x04,
	0x03,
	0x02,
	0x01,
	0x00
} ;

//********************************************************************************
// Function Name 	: MemClr
// Retun Value		: void
// Argment Value	: Clear Target PoINT_32er, Clear Byte Number
// Explanation		: Memory Clear Function
// History			: First edition
//********************************************************************************
void	MemClr( UINT_8	*NcTgtPtr, UINT_16	UsClrSiz )
{
	UINT_16	UsClrIdx ;

	for ( UsClrIdx = 0 ; UsClrIdx < UsClrSiz ; UsClrIdx++ )
	{
		*NcTgtPtr	= 0 ;
		NcTgtPtr++ ;
	}
}


//********************************************************************************
// Function Name 	: TneRun
// Retun Value		: Hall Tuning SUCCESS or FAILURE
// Argment Value	: NON
// Explanation		: Hall System Auto Adjustment Function
// History			: First edition 									2015.08.12
//********************************************************************************
UINT_32	TneRun( void )
{
	UINT_32	UlHlySts, UlHlxSts, UlAtxSts, UlAtySts, UlGvcSts ;
	UnDwdVal		StTneVal ;
	UINT_32	UlFinSts, UlReadVal ;
	UINT_32	UlCurDac ;

//--------------------------------------
// Initialize Calibration data
//--------------------------------------
	RtnCen( BOTH_OFF ) ;		// Both OFF
	WitTim( TNE ) ;
TRACE("TGT,%04x,%04x \n", BIAS_ADJ_RANGE_XY, MARGIN );

	RamWrite32A( HALL_RAM_HXOFF,  0x00000000 ) ;			//< X Offset Clr
	RamWrite32A( HALL_RAM_HYOFF,  0x00000000 ) ;			//< Y Offset Clr
	RamWrite32A( HallFilterCoeffX_hxgain0 , SXGAIN_LOP ) ;
	RamWrite32A( HallFilterCoeffY_hygain0 , SYGAIN_LOP ) ;
	DacControl( 0 , HLXBO , XY_BIAS ) ;
	RamWrite32A( StCaliData_UiHallBias_X , XY_BIAS ) ;
	DacControl( 0 , HLYBO , XY_BIAS ) ;
	RamWrite32A( StCaliData_UiHallBias_Y , XY_BIAS ) ;

	RamWrite32A( CMD_IO_ADR_ACCESS , VGAVREF ) ;
	RamRead32A(  CMD_IO_DAT_ACCESS , &UlCurDac ) ;			//< Offset DACデータRead
	UlCurDac = (UlCurDac & 0x00070000) | XY_OFST;
	RamWrite32A( CMD_IO_ADR_ACCESS , VGAVREF ) ;
	RamWrite32A( CMD_IO_DAT_ACCESS , UlCurDac ) ;			//< set Offset DAC initial data

//--------------------------------------
// Calibration Hall Bias/Offset
//--------------------------------------
	// Calibration Y axis(1st)
	StTneVal.UlDwdVal	= TnePtp( Y_DIR , PTP_BEFORE ) ;
	UlHlySts	= TneCen( Y_DIR, StTneVal, OFFDAC_3BIT ) ;
	RtnCen( YONLY_ON ) ;		// Y ON / X OFF
	WitTim( TNE ) ;

	// Calibration X axis(1st)
	StTneVal.UlDwdVal	= TnePtp( X_DIR , PTP_BEFORE ) ;
	UlHlxSts	= TneCen( X_DIR, StTneVal, OFFDAC_3BIT ) ;
	RtnCen( XONLY_ON ) ;		// Y OFF / X ON
	WitTim( TNE ) ;

	// Calibration Y axis(2nd)
	StTneVal.UlDwdVal	= TnePtp( Y_DIR , PTP_AFTER ) ;
	UlHlySts	= TneCen( Y_DIR, StTneVal, OFFDAC_3BIT ) ;
	RtnCen( YONLY_ON ) ;		// Y ON / X OFF
	WitTim( TNE ) ;

	// Calibration X axis(2nd)
	StTneVal.UlDwdVal	= TnePtp( X_DIR , PTP_AFTER ) ;
	UlHlxSts	= TneCen( X_DIR, StTneVal, OFFDAC_3BIT ) ;
	RtnCen( BOTH_OFF ) ;		// Both OFF

//--------------------------------------
// Calibration Mecha/Neutral center
//--------------------------------------
#ifdef	NEUTRAL_CENTER
	TneHvc();
#endif	//NEUTRAL_CENTER

	WitTim( TNE ) ;

	StAdjPar.StHalAdj.UsAdxOff = StAdjPar.StHalAdj.UsHlxCna  ;
	StAdjPar.StHalAdj.UsAdyOff = StAdjPar.StHalAdj.UsHlyCna  ;

	RamWrite32A( HALL_RAM_HXOFF,  (UINT_32)((StAdjPar.StHalAdj.UsAdxOff << 16 ) & 0xFFFF0000 )) ;
	RamWrite32A( HALL_RAM_HYOFF,  (UINT_32)((StAdjPar.StHalAdj.UsAdyOff << 16 ) & 0xFFFF0000 )) ;

//---------------------------------------------------
// Copy Hall Bias/Offset data to temporary variable
//---------------------------------------------------
	RamRead32A( StCaliData_UiHallOffset_X , &UlReadVal ) ;
	StAdjPar.StHalAdj.UsHlxOff = (UINT_16)( UlReadVal ) ;
TRACE("***StAdjPar.StHalAdj.UsHlxOff = %08X\n", StAdjPar.StHalAdj.UsHlxOff);

	RamRead32A( StCaliData_UiHallBias_X , &UlReadVal ) ;
	StAdjPar.StHalAdj.UsHlxGan = (UINT_16)( UlReadVal >> 16 ) ;

	RamRead32A( StCaliData_UiHallOffset_Y , &UlReadVal ) ;
	StAdjPar.StHalAdj.UsHlyOff = (UINT_16)( UlReadVal ) ;
TRACE("***StAdjPar.StHalAdj.UsHlyOff = %08X\n", StAdjPar.StHalAdj.UsHlyOff);

	RamRead32A( StCaliData_UiHallBias_Y , &UlReadVal ) ;
	StAdjPar.StHalAdj.UsHlyGan = (UINT_16)( UlReadVal >> 16 ) ;

//--------------------------------------
// Fine calibration neutral center
//--------------------------------------
#ifdef	NEUTRAL_CENTER_FINE
		TneFin();

TRACE("    XadofFin = %04xh \n", StAdjPar.StHalAdj.UsAdxOff ) ;
TRACE("    YadofFin = %04xh \n", StAdjPar.StHalAdj.UsAdyOff ) ;
		RamWrite32A( HALL_RAM_HXOFF,  (UINT_32)((StAdjPar.StHalAdj.UsAdxOff << 16 ) & 0xFFFF0000 )) ;
		RamWrite32A( HALL_RAM_HYOFF,  (UINT_32)((StAdjPar.StHalAdj.UsAdyOff << 16 ) & 0xFFFF0000 )) ;
#endif	//NEUTRAL_CENTER_FINE

//--------------------------------------
// Adjust loop gain
//--------------------------------------
	RtnCen( BOTH_ON ) ;					// Y ON / X ON
	WitTim( TNE ) ;

	UlAtxSts	= LopGan( X_DIR ) ;		// X Loop Gain Adjust
	UlAtySts	= LopGan( Y_DIR ) ;		// Y Loop Gain Adjust

//--------------------------------------
// Adjust gyro offset
//--------------------------------------
#ifdef		SEL_SHIFT_COR			// Shift correction
	UlGvcSts = TneGvc((UINT_8)0) ;
	UlGvcSts |= TneGvc((UINT_8)1) ;
	UlGvcSts |= TneAvc(0x10) ;
	if( (UlGvcSts & (EXE_AXADJ | EXE_AYADJ | EXE_AZADJ)) == EXE_END ){
		TneAvc(0x80);
	}
#else		//SEL_SHIFT_COR			// Shift correction
	UlGvcSts = TneGvc((UINT_8)0) ;
#endif		//SEL_SHIFT_COR			// Shift correction

//--------------------------------------
// Calculation adjust status
//--------------------------------------
	UlFinSts	= ( UlHlxSts - EXE_END ) + ( UlHlySts - EXE_END ) + ( UlAtxSts - EXE_END ) + ( UlAtySts - EXE_END )  + ( UlGvcSts - EXE_END ) + EXE_END ;
	StAdjPar.StHalAdj.UlAdjPhs = UlFinSts ;

	return( UlFinSts ) ;
}


#ifdef	SEL_CLOSED_AF
//********************************************************************************
// Function Name 	: TneRunZ
// Retun Value		: Z axis Hall Tuning SUCCESS or FAILURE
// Argment Value	: NON
// Explanation		: Z_axis Hall System Auto Adjustment Function
// History			: First edition 									2015.08.12
//********************************************************************************
UINT_32	TneRunZ( void )
{
	UINT_32	UlHlzSts, UlAtzSts ;
	UnDwdVal		StTneVal ;
	UINT_32	UlFinSts , UlReadVal ;
	UINT_32	UlCurDac ;

//--------------------------------------
// Initialize Calibration data
//--------------------------------------
	RtnCen( ZONLY_OFF ) ;
	WitTim( TNE ) ;

TRACE("TGT,%d,%d", BIAS_ADJ_RANGE_Z, MARGIN );

	RamWrite32A( CLAF_RAMA_AFOFFSET,  0x00000000 ) ;		// Z Offset Clr
	RamWrite32A( CLAF_Gain_afloop2 , SZGAIN_LOP ) ;
	DacControl( 0 , HLAFBO , Z_BIAS ) ;
	RamWrite32A( StCaliData_UiHallBias_AF , Z_BIAS) ;
	DacControl( 0 , HLAFO, Z_OFST ) ;
	RamWrite32A( StCaliData_UiHallOffset_AF , Z_OFST ) ;

	RamWrite32A( CMD_IO_ADR_ACCESS , VGAVREF ) ;
	RamRead32A(  CMD_IO_DAT_ACCESS , &UlCurDac ) ;			// Offset DACデータRead
	UlCurDac = (UlCurDac & 0x00000707) | Z_OFST;
	RamWrite32A( CMD_IO_ADR_ACCESS , VGAVREF ) ;
	RamWrite32A( CMD_IO_DAT_ACCESS , UlCurDac ) ;			// set Offset DAC initial data

	//--------------------------------------
// Calibration Hall Bias/Offset
//--------------------------------------
	StTneVal.UlDwdVal	= TnePtp( Z_DIR , PTP_BEFORE ) ;
	UlHlzSts	= TneCen( Z_DIR, StTneVal, OFFDAC_8BIT ) ;

	WitTim( TNE ) ;

//--------------------------------------
// Calibration Mecha center
//--------------------------------------
	UlReadVal = 0x00010000 - (UINT_32)StAdjPar.StHalAdj.UsHlzCna ;
	StAdjPar.StHalAdj.UsAdzOff = (UINT_16)UlReadVal ;

	RamWrite32A( CLAF_RAMA_AFOFFSET,  (UINT_32)((StAdjPar.StHalAdj.UsAdzOff << 16 ) & 0xFFFF0000 )) ;

//---------------------------------------------------
// Copy Hall Bias/Offset data to temporary variable
//---------------------------------------------------
	RamRead32A( StCaliData_UiHallOffset_AF , &UlReadVal ) ;
	StAdjPar.StHalAdj.UsHlzOff = (UINT_16)( UlReadVal >> 16 ) ;

	RamRead32A( StCaliData_UiHallBias_AF , &UlReadVal ) ;
	StAdjPar.StHalAdj.UsHlzGan = (UINT_16)( UlReadVal >> 16 ) ;

	RtnCen( ZONLY_ON ) ;				// Z ON

	WitTim( TNE ) ;

//--------------------------------------
// Adjust loop gain
//--------------------------------------
	UlAtzSts	= LopGan( Z_DIR ) ;		// Z Loop Gain Adjust

//--------------------------------------
// Calculation adjust status
//--------------------------------------
	UlFinSts	= ( UlHlzSts - EXE_END ) + ( UlAtzSts - EXE_END ) + EXE_END ;
	StAdjPar.StHalAdj.UlAdjPhs = UlFinSts ;

	return( UlFinSts ) ;
}

//********************************************************************************
// Function Name 	: TneRunA
// Retun Value		: AF + OIS Hall Tuning SUCCESS or FAILURE
// Argment Value	: NON
// Explanation		: AF + OIS Hall System Auto Adjustment Function
// History			: First edition
//********************************************************************************
UINT_32	TneRunA( void )
{
	UINT_32	UlFinSts ;

	UlFinSts = TneRunZ();
	UlFinSts |= TneRun();

	StAdjPar.StHalAdj.UlAdjPhs = UlFinSts ;
	return( UlFinSts ) ;
}

#endif

//********************************************************************************
// Function Name 	: TnePtp
// Retun Value		: Hall Top & Bottom Gaps
// Argment Value	: X,Y Direction, Adjust Before After Parameter
// Explanation		: Measuring Hall Paek To Peak
// History			: First edition
//********************************************************************************

UINT_32	TnePtp( UINT_8	UcDirSel, UINT_8	UcBfrAft )
{
	UnDwdVal		StTneVal ;
	INT_32			SlMeasureParameterA , SlMeasureParameterB ;
	INT_32			SlMeasureParameterNum ;
	INT_32			SlMeasureMaxValue, SlMeasureMinValue ;

//	SlMeasureParameterNum	=	2004 ;		// 20.0195/0.010 < x
	SlMeasureParameterNum	=	1800 ;		// 20.0195/0.010 < x

	if( UcDirSel == X_DIR ) {								// X axis
		SlMeasureParameterA		=	HALL_RAM_HXIDAT ;		// Set Measure RAM Address
		SlMeasureParameterB		=	HALL_RAM_HYIDAT ;		// Set Measure RAM Address
	} else if( UcDirSel == Y_DIR ) {						// Y axis
		SlMeasureParameterA		=	HALL_RAM_HYIDAT ;		// Set Measure RAM Address
		SlMeasureParameterB		=	HALL_RAM_HXIDAT ;		// Set Measure RAM Address
#ifdef	SEL_CLOSED_AF
	} else {												// Z axis
		SlMeasureParameterA		=	CLAF_RAMA_AFADIN ;		// Set Measure RAM Address
		SlMeasureParameterB		=	CLAF_RAMA_AFADIN ;		// Set Measure RAM Address
#endif
	}
	SetSinWavGenInt();

	RamWrite32A( SinWave_Offset		,	0x105E36 ) ;									// Freq Setting = Freq * 80000000h / Fs	: 10Hz
	RamWrite32A( SinWave_Gain		,	0x7FFFFFFF ) ;									// Set Sine Wave Gain
	RamWrite32A( SinWaveC_Regsiter	,	0x00000001 ) ;									// Sine Wave Start
	if( UcDirSel == X_DIR ) {
		SetTransDataAdr( SinWave_OutAddr	,	(UINT_32)HALL_RAM_SINDX1 ) ;		// Set Sine Wave Input RAM
	}else if( UcDirSel == Y_DIR ){
		SetTransDataAdr( SinWave_OutAddr	,	(UINT_32)HALL_RAM_SINDY1 ) ;		// Set Sine Wave Input RAM
#ifdef	SEL_CLOSED_AF
	}else{
		SetTransDataAdr( SinWave_OutAddr	,	(UINT_32)CLAF_RAMA_AFOUT ) ;		// Set Sine Wave Input RAM
#endif
	}

	MesFil( THROUGH ) ;					// Filter setting for measurement

//	MeasureStart( SlMeasureParameterNum , SlMeasureParameterA , SlMeasureParameterB ) ;	// Start measure
	MeasureStart2( SlMeasureParameterNum , SlMeasureParameterA , SlMeasureParameterB , 1 ) ;	// Start measure

	MeasureWait() ;						// Wait complete of measurement

	RamWrite32A( SinWaveC_Regsiter	,	0x00000000 ) ;									// Sine Wave Stop

	if( UcDirSel == X_DIR ) {
		SetTransDataAdr( SinWave_OutAddr	,	(UINT_32)0x00000000 ) ;			// Set Sine Wave Input RAM
		RamWrite32A( HALL_RAM_SINDX1		,	0x00000000 ) ;							// DelayRam Clear
	}else if( UcDirSel == Y_DIR ){
		SetTransDataAdr( SinWave_OutAddr	,	(UINT_32)0x00000000 ) ;			// Set Sine Wave Input RAM
		RamWrite32A( HALL_RAM_SINDY1		,	0x00000000 ) ;							// DelayRam Clear
#ifdef	SEL_CLOSED_AF
	}else{
		SetTransDataAdr( SinWave_OutAddr	,	(UINT_32)0x00000000 ) ;			// Set Sine Wave Input RAM
		RamWrite32A( CLAF_RAMA_AFOUT		,	0x00000000 ) ;							// DelayRam Clear
#endif
	}
	RamRead32A( StMeasFunc_MFA_SiMax1 , ( UINT_32 * )&SlMeasureMaxValue ) ;		// Max value
	RamRead32A( StMeasFunc_MFA_SiMin1 , ( UINT_32 * )&SlMeasureMinValue ) ;		// Min value

	StTneVal.StDwdVal.UsHigVal = (UINT_16)((SlMeasureMaxValue >> 16) & 0x0000FFFF );
	StTneVal.StDwdVal.UsLowVal = (UINT_16)((SlMeasureMinValue >> 16) & 0x0000FFFF );
//TRACE_USB("PTP,%d,%04x,%04x",  UcDirSel, StTneVal.StDwdVal.UsHigVal, StTneVal.StDwdVal.UsLowVal );
TRACE("\nPTP topbtm H = %04xh , L = %04xh , AXIS = %02x \n", StTneVal.StDwdVal.UsHigVal,StTneVal.StDwdVal.UsLowVal ,UcDirSel ) ;

	if( UcBfrAft == 0 ) {
		if( UcDirSel == X_DIR ) {
			StAdjPar.StHalAdj.UsHlxCen	= ( ( INT_16 )StTneVal.StDwdVal.UsHigVal + ( INT_16 )StTneVal.StDwdVal.UsLowVal ) / 2 ;
			StAdjPar.StHalAdj.UsHlxMax	= StTneVal.StDwdVal.UsHigVal ;
			StAdjPar.StHalAdj.UsHlxMin	= StTneVal.StDwdVal.UsLowVal ;
		} else if( UcDirSel == Y_DIR ){
			StAdjPar.StHalAdj.UsHlyCen	= ( ( INT_16 )StTneVal.StDwdVal.UsHigVal + ( INT_16 )StTneVal.StDwdVal.UsLowVal ) / 2 ;
			StAdjPar.StHalAdj.UsHlyMax	= StTneVal.StDwdVal.UsHigVal ;
			StAdjPar.StHalAdj.UsHlyMin	= StTneVal.StDwdVal.UsLowVal ;
#ifdef	SEL_CLOSED_AF
		} else {
			StAdjPar.StHalAdj.UsHlzCen	= ( ( INT_16 )StTneVal.StDwdVal.UsHigVal + ( INT_16 )StTneVal.StDwdVal.UsLowVal ) / 2 ;
			StAdjPar.StHalAdj.UsHlzMax	= StTneVal.StDwdVal.UsHigVal ;
			StAdjPar.StHalAdj.UsHlzMin	= StTneVal.StDwdVal.UsLowVal ;
//TRACE_USB("PTP,%d,%04x,%04x",  UcDirSel, StTneVal.StDwdVal.UsHigVal, StTneVal.StDwdVal.UsLowVal );
#endif
		}
	} else {
		if( UcDirSel == X_DIR ){
			StAdjPar.StHalAdj.UsHlxCna	= ( ( INT_16 )StTneVal.StDwdVal.UsHigVal + ( INT_16 )StTneVal.StDwdVal.UsLowVal ) / 2 ;
			StAdjPar.StHalAdj.UsHlxMxa	= StTneVal.StDwdVal.UsHigVal ;
			StAdjPar.StHalAdj.UsHlxMna	= StTneVal.StDwdVal.UsLowVal ;
		} else if( UcDirSel == Y_DIR ){
			StAdjPar.StHalAdj.UsHlyCna	= ( ( INT_16 )StTneVal.StDwdVal.UsHigVal + ( INT_16 )StTneVal.StDwdVal.UsLowVal ) / 2 ;
			StAdjPar.StHalAdj.UsHlyMxa	= StTneVal.StDwdVal.UsHigVal ;
			StAdjPar.StHalAdj.UsHlyMna	= StTneVal.StDwdVal.UsLowVal ;
#ifdef	SEL_CLOSED_AF
		} else {
			StAdjPar.StHalAdj.UsHlzCna	= ( ( INT_16 )StTneVal.StDwdVal.UsHigVal + ( INT_16 )StTneVal.StDwdVal.UsLowVal ) / 2 ;
			StAdjPar.StHalAdj.UsHlzMxa	= StTneVal.StDwdVal.UsHigVal ;
			StAdjPar.StHalAdj.UsHlzMna	= StTneVal.StDwdVal.UsLowVal ;
//TRACE_USB("PTP,%d,%04x,%04x",  UcDirSel, StTneVal.StDwdVal.UsHigVal, StTneVal.StDwdVal.UsLowVal );
#endif
		}
	}

	StTneVal.StDwdVal.UsHigVal	= 0x7fff - StTneVal.StDwdVal.UsHigVal ;		// Maximum Gap = Maximum - Hall Peak Top
	StTneVal.StDwdVal.UsLowVal	= StTneVal.StDwdVal.UsLowVal - 0x8000 ; 	// Minimum Gap = Hall Peak Bottom - Minimum

TRACE("PTP margin H = %04xh , L = %04xh , AXIS = %02x \n", StTneVal.StDwdVal.UsHigVal,StTneVal.StDwdVal.UsLowVal ,UcDirSel ) ;

	return( StTneVal.UlDwdVal ) ;
}

//********************************************************************************
// Function Name 	: TneCen
// Retun Value		: Hall Center Tuning Result
// Argment Value	: X,Y Direction, Hall Top & Bottom Gaps
// Explanation		: Hall Center Tuning Function
// History			: First edition 									2015.08.12
//********************************************************************************
UINT_16	UsValBef,UsValNow ;
UINT_32	UlValBefB,UlValNowB ;
UINT_32	TneCen( UINT_8	UcTneAxs, UnDwdVal	StTneVal, UINT_8 UcDacSel)
{
	UINT_8 	UcTmeOut, UcTofRst ;
	UINT_16	UsBiasVal ;
	UINT_32	UlTneRst, UlBiasVal , UlValNow ;
	UINT_16	UsHalMaxRange , UsHalMinRange ;

	UcTmeOut	= 1 ;
	UlTneRst	= FAILURE ;
	UcTofRst	= FAILURE ;

#ifdef	SEL_CLOSED_AF
	if(UcTneAxs == Z_DIR){
		UsHalMaxRange = HALL_MAX_RANGE_Z ;
		UsHalMinRange = HALL_MIN_RANGE_Z ;
	}else{
		UsHalMaxRange = HALL_MAX_RANGE_XY;
		UsHalMinRange = HALL_MIN_RANGE_XY;
	}
#else
	UsHalMaxRange = HALL_MAX_RANGE_XY ;
	UsHalMinRange = HALL_MIN_RANGE_XY ;
#endif

	if(UcTneAxs == X_DIR){
		RamRead32A( StCaliData_UiHallBias_X , &UlValNowB ) ;
		UlValBefB = UlValNowB ;
	}else if(UcTneAxs == Y_DIR){
		RamRead32A( StCaliData_UiHallBias_Y , &UlValNowB ) ;
		UlValBefB = UlValNowB ;
	}

	if( UcDacSel == 1){				// 3bit Offset DAC選択
TRACE("TneCen\nTneOff_3Bit\n");
		StTneVal.UlDwdVal	= TneOff_3Bit( StTneVal, UcTneAxs , 0 ) ;		// 3Bit Hall Offset調整 w/o ptp
		UcTofRst	= SUCCESS ;		//Offsetをざっくり取り除いたあとBiasを先ず変化させてみる。時間短縮
	}

	while ( UlTneRst && (UINT_32)UcTmeOut )
	{
		UlValBefB = UlValNowB ;
		
		if( UcTofRst == FAILURE ) {
//			StTneVal.UlDwdVal	= TneBia( StTneVal, UcTneAxs ) ;	// Bias調整成功有無に関わらず、3bit Offset調整後はBias調整を行う
			StTneVal.UlDwdVal	= TneOff_3Bit( StTneVal, UcTneAxs , 1 ) ;		// 3Bit Hall Offset調整 with ptp

		} else {
			StTneVal.UlDwdVal	= TneBia( StTneVal, UcTneAxs ) ;		// Hall Bias調整
			UcTofRst	= FAILURE ;
		}

TRACE(" CMP BiasVal = %08X , %08X\n", UlValBefB , UlValNowB);
		if(UcTneAxs == X_DIR){
			RamRead32A( StCaliData_UiHallBias_X , &UlValNowB ) ;
		}else if(UcTneAxs == Y_DIR){
			RamRead32A( StCaliData_UiHallBias_Y , &UlValNowB ) ;
		}
//		if(( UlValBefB == BIAS_HLMT ) && ( UlValNowB == BIAS_HLMT )){
		if((( UlValBefB == BIAS_HLMT ) && ( UlValNowB == BIAS_HLMT ))
		|| (( UlValBefB == BIAS_LLMT ) && ( UlValNowB == BIAS_LLMT ))){
			UcTmeOut += 20;
		}

		if( (StTneVal.StDwdVal.UsHigVal > MARGIN ) && (StTneVal.StDwdVal.UsLowVal > MARGIN ) )	/* position check */
		{
			UcTofRst	= SUCCESS ;
			UsValBef = UsValNow = 0x0000 ;
		}else if( (StTneVal.StDwdVal.UsHigVal <= MARGIN ) && (StTneVal.StDwdVal.UsLowVal <= MARGIN ) ){
			UcTofRst	= SUCCESS ;
			UlTneRst	= (UINT_32)FAILURE ;
		}else{
			UcTofRst	= FAILURE ;

			UsValBef = UsValNow ;

			if( UcTneAxs == X_DIR  ) {
				RamRead32A( StCaliData_UiHallOffset_X , &UlValNow ) ;
				UsValNow = (UINT_16)( UlValNow ) ;
TRACE("X_UsValNow = %08X\n", (UINT_32)UsValNow);
			}else if( UcTneAxs == Y_DIR ){
				RamRead32A( StCaliData_UiHallOffset_Y , &UlValNow ) ;
				UsValNow = (UINT_16)( UlValNow ) ;
TRACE("Y_UsValNow = %08X\n", (UINT_32)UsValNow);
#ifdef	SEL_CLOSED_AF
			}else{
				RamRead32A( StCaliData_UiHallOffset_AF , &UlValNow ) ;
				UsValNow = (UINT_16)( UlValNow ) ;
TRACE("AF_UsValNow = %08X\n",(UINT_32)UsValNow);
#endif
			}
			if( (( UsValBef == 0x0006 ) && ( UsValNow == 0x0006 ))
			 || (( UsValBef == 0x0000 ) && ( UsValNow == 0x0000 )) )
			{
				UcTmeOut += 20;
				if( UcTneAxs == X_DIR ) {
					RamRead32A( StCaliData_UiHallBias_X , &UlBiasVal ) ;
					UsBiasVal = (UINT_16)( UlBiasVal >> 16 ) ;

				}else if( UcTneAxs == Y_DIR ){
					RamRead32A( StCaliData_UiHallBias_Y , &UlBiasVal ) ;
					UsBiasVal = (UINT_16)( UlBiasVal >> 16 ) ;
#ifdef	SEL_CLOSED_AF
				}else{
					RamRead32A( StCaliData_UiHallBias_AF , &UlBiasVal ) ;
					UsBiasVal = (UINT_16)( UlBiasVal >> 16 ) ;
#endif
				}

				if( UsBiasVal > DECRE_CAL )
				{
					UsBiasVal -= DECRE_CAL ;
				}

				if( UcTneAxs == X_DIR ) {
					UlBiasVal = ( UINT_32 )( UsBiasVal << 16 ) ;
					DacControl( 0 , HLXBO , UlBiasVal ) ;
					RamWrite32A( StCaliData_UiHallBias_X , UlBiasVal ) ;
				}else if( UcTneAxs == Y_DIR ){
					UlBiasVal = ( UINT_32 )( UsBiasVal << 16 ) ;
					DacControl( 0 , HLYBO , UlBiasVal ) ;
					RamWrite32A( StCaliData_UiHallBias_Y , UlBiasVal ) ;
#ifdef	SEL_CLOSED_AF
				}else{
					UlBiasVal = ( UINT_32 )( UsBiasVal << 16 ) ;
					DacControl( 0 , HLAFBO , UlBiasVal ) ;
					RamWrite32A( StCaliData_UiHallBias_AF , UlBiasVal ) ;
#endif
				}
			}
		}

		if((( (UINT_16)0xFFFF - ( StTneVal.StDwdVal.UsHigVal + StTneVal.StDwdVal.UsLowVal )) < UsHalMaxRange )
		&& (( (UINT_16)0xFFFF - ( StTneVal.StDwdVal.UsHigVal + StTneVal.StDwdVal.UsLowVal )) > UsHalMinRange ) ) {
			if(UcTofRst	== SUCCESS)
			{
				UlTneRst	= (UINT_32)SUCCESS ;
				break ;
			}
		}
		UlTneRst	= (UINT_32)FAILURE ;
		UcTmeOut++ ;

		if ( UcTmeOut >= TIME_OUT ) {		// Set Time Out Count
			UcTmeOut	= 0 ;
		}
	}

	SetSinWavGenInt() ;		//

	if( UlTneRst == (UINT_32)FAILURE ) {
		if( UcTneAxs == X_DIR ) {
			UlTneRst					= EXE_HXADJ ;
			StAdjPar.StHalAdj.UsHlxGan	= 0xFFFF ;
			StAdjPar.StHalAdj.UsHlxOff	= 0xFFFF ;
		}else if( UcTneAxs == Y_DIR ) {
			UlTneRst					= EXE_HYADJ ;
			StAdjPar.StHalAdj.UsHlyGan	= 0xFFFF ;
			StAdjPar.StHalAdj.UsHlyOff	= 0xFFFF ;
#ifdef	SEL_CLOSED_AF
		} else {
			UlTneRst					= EXE_HZADJ ;
			StAdjPar.StHalAdj.UsHlzGan	= 0xFFFF ;
			StAdjPar.StHalAdj.UsHlzOff	= 0xFFFF ;
#endif
		}
	} else {
		UlTneRst	= EXE_END ;
	}

	return( UlTneRst ) ;
}

//********************************************************************************
// Function Name 	: TneBia
// Retun Value		: Hall Top & Bottom Gaps
// Argment Value	: Hall Top & Bottom Gaps , X,Y Direction
// Explanation		: Hall Bias Tuning Function
// History			: First edition
//********************************************************************************
UINT_32	TneBia( UnDwdVal	StTneVal, UINT_8	UcTneAxs )
{
	UINT_32			UlSetBia ;
	UINT_16			UsHalAdjRange;
#ifdef	SEL_CLOSED_AF
	if(UcTneAxs == Z_DIR){
		UsHalAdjRange = BIAS_ADJ_RANGE_Z ;
	}else{
		UsHalAdjRange = BIAS_ADJ_RANGE_XY ;
	}
#else
		UsHalAdjRange = BIAS_ADJ_RANGE_XY ;
#endif

	if( UcTneAxs == X_DIR ) {
		RamRead32A( StCaliData_UiHallBias_X , &UlSetBia ) ;
	} else if( UcTneAxs == Y_DIR ) {
		RamRead32A( StCaliData_UiHallBias_Y , &UlSetBia ) ;
#ifdef	SEL_CLOSED_AF
	} else {
		RamRead32A( StCaliData_UiHallBias_AF , &UlSetBia ) ;
#endif
	}

	if( UlSetBia == 0x00000000 )	UlSetBia = 0x01000000 ;
	UlSetBia = (( UlSetBia >> 16 ) & (UINT_32)0x0000FF00 ) ;
	UlSetBia *= (UINT_32)UsHalAdjRange ;
	if((UINT_32)( StTneVal.StDwdVal.UsHigVal + StTneVal.StDwdVal.UsLowVal ) >= (UINT_32)0xFFFF )
	{
		UlSetBia = BIAS_HLMT ;
	}else{
		UlSetBia /= (UINT_32)( 0xFFFF - ( StTneVal.StDwdVal.UsHigVal + StTneVal.StDwdVal.UsLowVal )) ;
		if( UlSetBia > (UINT_32)0x0000FFFF )		UlSetBia = 0x0000FFFF ;
		UlSetBia = ( UlSetBia << 16 ) ;
		if( UlSetBia > BIAS_HLMT )		UlSetBia = BIAS_HLMT ;
		if( UlSetBia < BIAS_LLMT )		UlSetBia = BIAS_LLMT ;
	}

TRACE("( AXIS = %02x , BIAS = %08xh ) , \n", UcTneAxs , (INT_32)UlSetBia ) ;
	if( UcTneAxs == X_DIR ) {
		DacControl( 0 , HLXBO , UlSetBia ) ;
		RamWrite32A( StCaliData_UiHallBias_X , UlSetBia) ;
	} else if( UcTneAxs == Y_DIR ){
		DacControl( 0 , HLYBO , UlSetBia ) ;
		RamWrite32A( StCaliData_UiHallBias_Y , UlSetBia) ;
#ifdef	SEL_CLOSED_AF
	} else {
		DacControl( 0 , HLAFBO , UlSetBia ) ;
		RamWrite32A( StCaliData_UiHallBias_AF , UlSetBia) ;
#endif
	}

	StTneVal.UlDwdVal	= TnePtp( UcTneAxs , PTP_AFTER ) ;

	return( StTneVal.UlDwdVal ) ;
}


//********************************************************************************
// Function Name 	: TneOff_3Bit
// Retun Value		: 3Bit DAC Hall Top & Bottom Gaps
// Argment Value	: 3Bit DAC Hall Top & Bottom Gaps , X,Y Direction
// Explanation		: Hall Offset Tuning Function(3bit DAC)
// History			: First edition 									2015.08.12
//********************************************************************************
UINT_32	TneOff_3Bit( UnDwdVal	StTneVal, UINT_8	UcTneAxs , UINT_8 UcPtpMod )
{
	INT_16	SsCenVal ;											// 各軸のOffset調整値
	UINT_32	UlDacDat, UlCurDac ;						// 3bit DACへセットする変数
	INT_8	i, index ;

//--------------------------------------
// Current value search
//--------------------------------------
	if( UcTneAxs == X_DIR ) {
		RamWrite32A( CMD_IO_ADR_ACCESS , VGAVREF ) ;
		RamRead32A(  CMD_IO_DAT_ACCESS , &UlCurDac ) ;			// Offset DACデータRead
		UlCurDac	&= 0x0000000F ;
	} else {
		RamWrite32A( CMD_IO_ADR_ACCESS , VGAVREF ) ;
		RamRead32A(  CMD_IO_DAT_ACCESS , &UlCurDac ) ;			// Offset DACデータRead
		UlCurDac	= ( UlCurDac & 0x00000F00 ) >> 8 ;
	}

TRACE("UlCurDac = %08X\n", (UINT_32)UlCurDac);
	for( i = 0 ; i < 7 ; i++ ) {
		if( ( UINT_8 )UlCurDac == UcDacArray[ i ] ) {
			break ;
		}
	}

//--------------------------------------
// Calculate Hall center value
//--------------------------------------
	SsCenVal	= ( INT_16 )(( StTneVal.StDwdVal.UsHigVal - StTneVal.StDwdVal.UsLowVal ) / OFFSET_DIV_3BIT );	// Calculating Value For Increase Step
	if( SsCenVal < 0){
		SsCenVal = (INT_16)(( SsCenVal - 1 ) / OFFSET_DIV );
	}else{
		SsCenVal = (INT_16)(( SsCenVal + 1 ) / OFFSET_DIV );
	}
TRACE("UlCenVal = %08X\n", SsCenVal);

	index	= (i - ( INT_8 )SsCenVal) ;

TRACE("TneOff_3Bit index = %d -> ", index);

	if( index > 6 ) {
		index	= 6 ;
	} else if( index < 0 ) {
		index	= 0 ;
	}
TRACE("%d\n", index);

	UlDacDat	= ( UINT_32 )UcDacArray[ index ] ;

//--------------------------------------
// Calibration Hall offset
//--------------------------------------
	if( UcTneAxs == X_DIR ) {									// X軸Offset調整
		RamWrite32A( StCaliData_UiHallOffset_X , UlDacDat ) ;	// X軸Offset値を一時変数へコピー

		RamWrite32A( CMD_IO_ADR_ACCESS , VGAVREF ) ;
		RamRead32A(  CMD_IO_DAT_ACCESS , &UlCurDac ) ;			// Offset DACデータRead
		UlDacDat	= ( UlCurDac & 0x000F0F00) | UlDacDat ;		// Y軸データを保持しX軸の調整値を上書き

		RamWrite32A( CMD_IO_ADR_ACCESS , VGAVREF ) ;
		RamWrite32A( CMD_IO_DAT_ACCESS , UlDacDat ) ;			// X軸調整値をセット
TRACE("Set Offset DAC Data UlCenVal= %08X\n", (UINT_32)UlDacDat);

	} else if( UcTneAxs == Y_DIR ){								// Y軸Offset調整
		RamWrite32A( StCaliData_UiHallOffset_Y , UlDacDat ) ;	// Y軸Offset値を一時変数へコピー

		RamWrite32A( CMD_IO_ADR_ACCESS , VGAVREF ) ;
		RamRead32A(  CMD_IO_DAT_ACCESS , &UlCurDac ) ;			// Offset DACデータRead
		UlDacDat = ( UlCurDac & 0x000F000F ) | ( UlDacDat << 8 ) ;	// X軸データを保持しY軸の調整値を上書き

		RamWrite32A( CMD_IO_ADR_ACCESS , VGAVREF ) ;
		RamWrite32A( CMD_IO_DAT_ACCESS , UlDacDat ) ;			// Y軸調整値をセット
TRACE("Set Offset DAC Data UlCenVal= %08X\n", (UINT_32)UlDacDat);
	}

TRACE("( AXIS = %02x , OFST = %08xh ) , \n", UcTneAxs , (INT_32)UlDacDat ) ;

	if( UcPtpMod ){
		StTneVal.UlDwdVal	= TnePtp( UcTneAxs, PTP_AFTER ) ;
	}

	return( StTneVal.UlDwdVal ) ;
}

//********************************************************************************
// Function Name 	: MesFil
// Retun Value		: NON
// Argment Value	: Measure Filter Mode
// Explanation		: Measure Filter Setting Function
// History			: First edition
//********************************************************************************
void	MesFil( UINT_8	UcMesMod )		// 20.019kHz
{
	UINT_32	UlMeasFilaA , UlMeasFilaB , UlMeasFilaC ;
	UINT_32	UlMeasFilbA , UlMeasFilbB , UlMeasFilbC ;

	if( !UcMesMod ) {								// Hall Bias&Offset Adjust

		UlMeasFilaA	=	0x02F19B01 ;	// LPF 150Hz
		UlMeasFilaB	=	0x02F19B01 ;
		UlMeasFilaC	=	0x7A1CC9FF ;
		UlMeasFilbA	=	0x7FFFFFFF ;	// Through
		UlMeasFilbB	=	0x00000000 ;
		UlMeasFilbC	=	0x00000000 ;

	} else if( UcMesMod == LOOPGAIN ) {				// Loop Gain Adjust

		UlMeasFilaA	=	0x115CC757 ;	// LPF1000Hz
		UlMeasFilaB	=	0x115CC757 ;
		UlMeasFilaC	=	0x5D467153 ;
		UlMeasFilbA	=	0x7F667431 ;	// HPF30Hz
		UlMeasFilbB	=	0x80998BCF ;
		UlMeasFilbC	=	0x7ECCE863 ;

	} else if( UcMesMod == LOOPGAIN2 ) {				// Loop Gain Adjust2

		UlMeasFilaA	=	0x01C856C9 ;	// LPF90Hz
		UlMeasFilaB	=	0x01C856C9 ;
		UlMeasFilaC	=	0x7C6F526D ;
		UlMeasFilbA	=	0x7FCCA8FF ;	// HPF10Hz
		UlMeasFilbB	=	0x80335701 ;
		UlMeasFilbC	=	0x7F9951FD ;

	} else if( UcMesMod == THROUGH ) {				// for Through

		UlMeasFilaA	=	0x7FFFFFFF ;	// Through
		UlMeasFilaB	=	0x00000000 ;
		UlMeasFilaC	=	0x00000000 ;
		UlMeasFilbA	=	0x7FFFFFFF ;	// Through
		UlMeasFilbB	=	0x00000000 ;
		UlMeasFilbC	=	0x00000000 ;

	} else if( UcMesMod == NOISE ) {				// SINE WAVE TEST for NOISE

		UlMeasFilaA	=	0x02F19B01 ;	// LPF150Hz
		UlMeasFilaB	=	0x02F19B01 ;
		UlMeasFilaC	=	0x7A1CC9FF ;
		UlMeasFilbA	=	0x02F19B01 ;	// LPF150Hz
		UlMeasFilbB	=	0x02F19B01 ;
		UlMeasFilbC	=	0x7A1CC9FF ;

	} else if(UcMesMod == OSCCHK) {
		UlMeasFilaA	=	0x05C141BB ;	// LPF300Hz
		UlMeasFilaB	=	0x05C141BB ;
		UlMeasFilaC	=	0x747D7C88 ;
		UlMeasFilbA	=	0x05C141BB ;	// LPF300Hz
		UlMeasFilbB	=	0x05C141BB ;
		UlMeasFilbC	=	0x747D7C88 ;

	} else if( UcMesMod == SELFTEST ) {				// GYRO SELF TEST

		UlMeasFilaA	=	0x115CC757 ;	// LPF1000Hz
		UlMeasFilaB	=	0x115CC757 ;
		UlMeasFilaC	=	0x5D467153 ;
		UlMeasFilbA	=	0x7FFFFFFF ;	// Through
		UlMeasFilbB	=	0x00000000 ;
		UlMeasFilbC	=	0x00000000 ;

	}

	RamWrite32A ( MeasureFilterA_Coeff_a1	, UlMeasFilaA ) ;
	RamWrite32A ( MeasureFilterA_Coeff_b1	, UlMeasFilaB ) ;
	RamWrite32A ( MeasureFilterA_Coeff_c1	, UlMeasFilaC ) ;

	RamWrite32A ( MeasureFilterA_Coeff_a2	, UlMeasFilbA ) ;
	RamWrite32A ( MeasureFilterA_Coeff_b2	, UlMeasFilbB ) ;
	RamWrite32A ( MeasureFilterA_Coeff_c2	, UlMeasFilbC ) ;

	RamWrite32A ( MeasureFilterB_Coeff_a1	, UlMeasFilaA ) ;
	RamWrite32A ( MeasureFilterB_Coeff_b1	, UlMeasFilaB ) ;
	RamWrite32A ( MeasureFilterB_Coeff_c1	, UlMeasFilaC ) ;

	RamWrite32A ( MeasureFilterB_Coeff_a2	, UlMeasFilbA ) ;
	RamWrite32A ( MeasureFilterB_Coeff_b2	, UlMeasFilbB ) ;
	RamWrite32A ( MeasureFilterB_Coeff_c2	, UlMeasFilbC ) ;
}

//********************************************************************************
// Function Name 	: ClrMesFil
// Retun Value		: NON
// Argment Value	: NON
// Explanation		: Clear Measure Filter Function
// History			: First edition
//********************************************************************************
void	ClrMesFil( void )
{
	RamWrite32A ( MeasureFilterA_Delay_z11	, 0 ) ;
	RamWrite32A ( MeasureFilterA_Delay_z12	, 0 ) ;

	RamWrite32A ( MeasureFilterA_Delay_z21	, 0 ) ;
	RamWrite32A ( MeasureFilterA_Delay_z22	, 0 ) ;

	RamWrite32A ( MeasureFilterB_Delay_z11	, 0 ) ;
	RamWrite32A ( MeasureFilterB_Delay_z12	, 0 ) ;

	RamWrite32A ( MeasureFilterB_Delay_z21	, 0 ) ;
	RamWrite32A ( MeasureFilterB_Delay_z22	, 0 ) ;
}

//********************************************************************************
// Function Name 	: MeasureStart
// Retun Value		: NON
// Argment Value	: NON
// Explanation		: Measure start setting Function
// History			: First edition
//********************************************************************************
void	MeasureStart( INT_32 SlMeasureParameterNum , INT_32 SlMeasureParameterA , INT_32 SlMeasureParameterB )
{
	MemoryClear( StMeasFunc_SiSampleNum , sizeof( MeasureFunction_Type ) ) ;
	RamWrite32A( StMeasFunc_MFA_SiMax1	 , 0x80000000 ) ;					// Set Min
	RamWrite32A( StMeasFunc_MFB_SiMax2	 , 0x80000000 ) ;					// Set Min
	RamWrite32A( StMeasFunc_MFA_SiMin1	 , 0x7FFFFFFF ) ;					// Set Max
	RamWrite32A( StMeasFunc_MFB_SiMin2	 , 0x7FFFFFFF ) ;					// Set Max

	SetTransDataAdr( StMeasFunc_MFA_PiMeasureRam1	, ( UINT_32 )SlMeasureParameterA ) ;		// Set Measure Filter A Ram Address
	SetTransDataAdr( StMeasFunc_MFB_PiMeasureRam2	, ( UINT_32 )SlMeasureParameterB ) ;		// Set Measure Filter B Ram Address
	RamWrite32A( StMeasFunc_MFA_SiSampleNumA	 	, 0 ) ;									// Clear Measure Counter
	RamWrite32A( StMeasFunc_MFB_SiSampleNumB	 	, 0 ) ;									// Clear Measure Counter
	RamWrite32A( StMeasFunc_PMC_UcPhaseMesMode		, 0 ) ;									// Set Phase Measure Mode
	ClrMesFil() ;																			// Clear Delay Ram
	SetWaitTime(50) ;
	RamWrite32A( StMeasFunc_MFB_SiSampleMaxB		, SlMeasureParameterNum ) ;				// Set Measure Max Number
	RamWrite32A( StMeasFunc_MFA_SiSampleMaxA		, SlMeasureParameterNum ) ;				// Set Measure Max Number
}


//********************************************************************************
// Function Name 	: MeasureStart2
// Retun Value		: NON
// Argment Value	: NON
// Explanation		: Measure start setting Function
// History			: First edition 						
//********************************************************************************
void	MeasureStart2( INT_32 SlMeasureParameterNum , INT_32 SlMeasureParameterA , INT_32 SlMeasureParameterB , UINT_16 UsTime )
{
	MemoryClear( StMeasFunc_SiSampleNum , sizeof( MeasureFunction_Type ) ) ;
	RamWrite32A( StMeasFunc_MFA_SiMax1	 , 0x80000001 ) ;					// Set Min 
	RamWrite32A( StMeasFunc_MFB_SiMax2	 , 0x80000001 ) ;					// Set Min 
	RamWrite32A( StMeasFunc_MFA_SiMin1	 , 0x7FFFFFFF ) ;					// Set Max 
	RamWrite32A( StMeasFunc_MFB_SiMin2	 , 0x7FFFFFFF ) ;					// Set Max 
	
	SetTransDataAdr( StMeasFunc_MFA_PiMeasureRam1	 , ( UINT_32 )SlMeasureParameterA ) ;		// Set Measure Filter A Ram Address
	SetTransDataAdr( StMeasFunc_MFB_PiMeasureRam2	 , ( UINT_32 )SlMeasureParameterB ) ;		// Set Measure Filter B Ram Address
	RamWrite32A( StMeasFunc_MFA_SiSampleNumA	 	, 0 ) ;									// Clear Measure Counter
	RamWrite32A( StMeasFunc_MFB_SiSampleNumB	 	, 0 ) ;									// Clear Measure Counter
	RamWrite32A( StMeasFunc_PMC_UcPhaseMesMode		, 0 ) ;									// Set Phase Measure Mode
	ClrMesFil() ;						// Clear Delay Ram
	SetWaitTime(UsTime) ;
	RamWrite32A( StMeasFunc_MFB_SiSampleMaxB		, SlMeasureParameterNum ) ;				// Set Measure Max Number
	RamWrite32A( StMeasFunc_MFA_SiSampleMaxA		, SlMeasureParameterNum ) ;				// Set Measure Max Number

}

//********************************************************************************
// Function Name 	: MeasureWait
// Retun Value		: NON
// Argment Value	: NON
// Explanation		: Wait complete of Measure Function
// History			: First edition
//********************************************************************************
void	MeasureWait( void )
{
	UINT_32	SlWaitTimerStA, SlWaitTimerStB ;
	UINT_16	UsTimeOut = 2000;

	do {
		RamRead32A( StMeasFunc_MFA_SiSampleMaxA, &SlWaitTimerStA ) ;
		RamRead32A( StMeasFunc_MFB_SiSampleMaxB, &SlWaitTimerStB ) ;
		UsTimeOut--;
	} while ( (SlWaitTimerStA || SlWaitTimerStB) && UsTimeOut );

}

//********************************************************************************
// Function Name 	: MemoryClear
// Retun Value		: NON
// Argment Value	: Top poINT_32er , Size
// Explanation		: Memory Clear Function
// History			: First edition
//********************************************************************************
void	MemoryClear( UINT_16 UsSourceAddress, UINT_16 UsClearSize )
{
	UINT_16	UsLoopIndex ;

	for ( UsLoopIndex = 0 ; UsLoopIndex < UsClearSize ;  ) {
		RamWrite32A( UsSourceAddress	, 	0x00000000 ) ;				// 4Byte
		UsSourceAddress += 4;
		UsLoopIndex += 4 ;
	}
}

//********************************************************************************
// Function Name 	: SetWaitTime
// Retun Value		: NON
// Argment Value	: NON
// Explanation		: Set Timer wait Function
// History			: First edition
//********************************************************************************
#define 	ONE_MSEC_COUNT	20			// 20.0195kHz * 20 ≒ 1ms
void	SetWaitTime( UINT_16 UsWaitTime )
{
	RamWrite32A( WaitTimerData_UiWaitCounter	, 0 ) ;
	RamWrite32A( WaitTimerData_UiTargetCount	, (UINT_32)(ONE_MSEC_COUNT * UsWaitTime)) ;
}


//********************************************************************************
// Function Name 	: LopGan
// Retun Value		: Execute Result
// Argment Value	: X,Y Direction
// Explanation		: Loop Gain Adjust Function
// History			: First edition
//********************************************************************************

#define 	LOOP_NUM		2136			// 20.019kHz/0.152kHz*16times
#define 	LOOP_FREQ		0x00F8CCE9		// 	152Hz  = Freq * 80000000h / Fs
//#define 	LOOP_GAIN		0x16C310E3		// -15dB
#define 	LOOP_GAIN		0x05B7B15A		// -27dB
#define		GAIN_GAP		(1000)			// 20*log(1000/1000)=0dB

#define 	LOOP_MAX_X		SXGAIN_LOP << 1	// x2
#define 	LOOP_MIN_X		SXGAIN_LOP >> 1	// x0.5
#define 	LOOP_MAX_Y		SYGAIN_LOP << 1	// x2
#define 	LOOP_MIN_Y		SYGAIN_LOP >> 1	// x0.5

#ifdef	SEL_CLOSED_AF
#define 	LOOP_NUM_Z		1885			// 20.019kHz/0.170kHz*16times
#define 	LOOP_FREQ_Z		0x0116437F		// 	170Hz  = Freq * 80000000h / Fs
#define 	LOOP_GAIN_Z		0x0207567A		// -36dB
#define 	LOOP_MAX_Z		SZGAIN_LOP << 1	// x2
#define 	LOOP_MIN_Z		SZGAIN_LOP >> 1	// x0.5
#endif

UINT_32	LopGan( UINT_8	UcDirSel )
{
	UnllnVal		StMeasValueA , StMeasValueB ;
	INT_32			SlMeasureParameterA , SlMeasureParameterB ;
	INT_32			SlMeasureParameterNum ;
	UINT_64	UllCalculateVal ;
	UINT_32	UlReturnState ;
	UINT_16	UsSinAdr ;
	UINT_32	UlLopFreq , UlLopGain , UlGainGap;
	UINT_8	UcMesFil;

#ifdef	SEL_CLOSED_AF
	UINT_32	UlSwitchBk ;
#endif

		SlMeasureParameterNum	=	(INT_32)LOOP_NUM ;
		UlLopFreq = LOOP_FREQ;
		UlLopGain = LOOP_GAIN;
		UlGainGap = GAIN_GAP;
		UcMesFil  = LOOPGAIN;

	if( UcDirSel == X_DIR ) {		// X axis
		SlMeasureParameterA		=	HALL_RAM_HXOUT1 ;		// Set Measure RAM Address
		SlMeasureParameterB		=	HALL_RAM_HXLOP ;		// Set Measure RAM Address
		RamWrite32A( HallFilterCoeffX_hxgain0 , SXGAIN_LOP ) ;
		UsSinAdr = HALL_RAM_SINDX0;
	} else if( UcDirSel == Y_DIR ){						// Y axis
		SlMeasureParameterA		=	HALL_RAM_HYOUT1 ;		// Set Measure RAM Address
		SlMeasureParameterB		=	HALL_RAM_HYLOP ;		// Set Measure RAM Address
		RamWrite32A( HallFilterCoeffY_hygain0 , SYGAIN_LOP ) ;
		UsSinAdr = HALL_RAM_SINDY0;
#ifdef	SEL_CLOSED_AF
	} else {						// Y axis
		SlMeasureParameterNum	=	(signed INT_32)LOOP_NUM_Z ;
		SlMeasureParameterA		=	CLAF_RAMA_AFLOP2 ;		// Set Measure RAM Address
		SlMeasureParameterB		=	CLAF_DELAY_AFPZ0 ;		// Set Measure RAM Address
		RamWrite32A( CLAF_Gain_afloop2 , SZGAIN_LOP ) ;
		RamRead32A( CLAF_RAMA_AFCNT , &UlSwitchBk ) ;
//		RamWrite32A( CLAF_RAMA_AFCNT , UlSwitchBk & 0xffffffef ) ;
		RamWrite32A( CLAF_RAMA_AFCNT , UlSwitchBk & 0xffffff0f ) ;
		UsSinAdr = CLAF_RAMA_AFCNTO;
		UlLopFreq = LOOP_FREQ_Z;
		UlLopGain = LOOP_GAIN_Z;
		UcMesFil  = LOOPGAIN;
#endif
	}

	SetSinWavGenInt();

	RamWrite32A( SinWave_Offset		,	UlLopFreq ) ;								// Freq Setting
	RamWrite32A( SinWave_Gain		,	UlLopGain ) ;								// Set Sine Wave Gain

	RamWrite32A( SinWaveC_Regsiter	,	0x00000001 ) ;								// Sine Wave Start

	SetTransDataAdr( SinWave_OutAddr	,	( UINT_32 )UsSinAdr ) ;	// Set Sine Wave Input RAM

	MesFil( UcMesFil ) ;					// Filter setting for measurement


	MeasureStart( SlMeasureParameterNum , SlMeasureParameterA , SlMeasureParameterB ) ;					// Start measure

	MeasureWait() ;						// Wait complete of measurement

	RamRead32A( StMeasFunc_MFA_LLiAbsInteg1 		, &StMeasValueA.StUllnVal.UlLowVal ) ;	// X axis
	RamRead32A( StMeasFunc_MFA_LLiAbsInteg1 + 4 	, &StMeasValueA.StUllnVal.UlHigVal ) ;
	RamRead32A( StMeasFunc_MFB_LLiAbsInteg2 		, &StMeasValueB.StUllnVal.UlLowVal ) ;	// Y axis
	RamRead32A( StMeasFunc_MFB_LLiAbsInteg2 + 4		, &StMeasValueB.StUllnVal.UlHigVal ) ;

	SetSinWavGenInt();		// Sine wave stop

	SetTransDataAdr( SinWave_OutAddr	,	(UINT_32)0x00000000 ) ;	// Set Sine Wave Input RAM
	RamWrite32A( UsSinAdr		,	0x00000000 ) ;				// DelayRam Clear

	if( UcDirSel == X_DIR ) {		// X axis
		if( StMeasValueA.UllnValue == 0 ){
			UlReturnState = EXE_LXADJ ;
		}else{
			UllCalculateVal = ( StMeasValueB.UllnValue * 1000 / StMeasValueA.UllnValue ) * SXGAIN_LOP / UlGainGap ;
			if( UllCalculateVal > (UINT_64)0x000000007FFFFFFF )		UllCalculateVal = (UINT_64)0x000000007FFFFFFF ;
			StAdjPar.StLopGan.UlLxgVal = (UINT_32)UllCalculateVal ;
			RamWrite32A( HallFilterCoeffX_hxgain0 , StAdjPar.StLopGan.UlLxgVal ) ;
			if( UllCalculateVal > LOOP_MAX_X ){
				UlReturnState = EXE_LXADJ ;
			}else if( UllCalculateVal < LOOP_MIN_X ){
				UlReturnState = EXE_LXADJ ;
			}else{
				UlReturnState = EXE_END ;
			}
		}

	}else if( UcDirSel == Y_DIR ){							// Y axis
		if( StMeasValueA.UllnValue == 0 ){
			UlReturnState = EXE_LYADJ ;
		}else{
			UllCalculateVal = ( StMeasValueB.UllnValue * 1000 / StMeasValueA.UllnValue ) * SYGAIN_LOP / UlGainGap ;
			if( UllCalculateVal > (UINT_64)0x000000007FFFFFFF )		UllCalculateVal = (UINT_64)0x000000007FFFFFFF ;
			StAdjPar.StLopGan.UlLygVal = (UINT_32)UllCalculateVal ;
			RamWrite32A( HallFilterCoeffY_hygain0 , StAdjPar.StLopGan.UlLygVal ) ;
			if( UllCalculateVal > LOOP_MAX_Y ){
				UlReturnState = EXE_LYADJ ;
			}else if( UllCalculateVal < LOOP_MIN_Y ){
				UlReturnState = EXE_LYADJ ;
			}else{
				UlReturnState = EXE_END ;
			}
		}
#ifdef	SEL_CLOSED_AF
	}else{							// Z axis
		UllCalculateVal = ( StMeasValueB.UllnValue * 1000 / StMeasValueA.UllnValue ) * SZGAIN_LOP / GAIN_GAP ;
		if( UllCalculateVal > (UINT_64)0x000000007FFFFFFF )		UllCalculateVal = (UINT_64)0x000000007FFFFFFF ;
		StAdjPar.StLopGan.UlLzgVal = (UINT_32)UllCalculateVal ;
		RamWrite32A( CLAF_Gain_afloop2 , StAdjPar.StLopGan.UlLzgVal ) ;
		if( UllCalculateVal > LOOP_MAX_Z ){
			UlReturnState = EXE_LZADJ ;
		}else if( UllCalculateVal < LOOP_MIN_Z ){
			UlReturnState = EXE_LZADJ ;
		}else{
			UlReturnState = EXE_END ;
		}
		RamWrite32A( CLAF_RAMA_AFCNT , UlSwitchBk ) ;
#endif
	}

	return( UlReturnState ) ;

}




//********************************************************************************
// Function Name 	: TneGvc
// Retun Value		: NON
// Argment Value	: NON
// Explanation		: Tunes the Gyro VC offset
// History			: First edition
//********************************************************************************
#define 	GYROF_NUM		2048			// 2048times
#define 	GYROF_UPPER		0x0600			// ICM_20690（±500[dps]、65.5[LSB/dps]）
#define 	GYROF_LOWER		0xFA00			// ±23.4[dps]
UINT_32	TneGvc( UINT_8	uc_mode )
{
	UINT_32	UlRsltSts;
	INT_32			SlMeasureParameterA , SlMeasureParameterB ;
	INT_32			SlMeasureParameterNum ;
	UnllnVal		StMeasValueA , StMeasValueB ;
	INT_32			SlMeasureAveValueA , SlMeasureAveValueB ;
#ifdef	SEL_SHIFT_COR
	UINT_16	UsGzoVal;
#endif	//SEL_SHIFT_COR


	//平均値測定

	MesFil( THROUGH ) ;					// Set Measure Filter

	SlMeasureParameterNum	=	GYROF_NUM ;					// Measurement times
#ifdef	SEL_SHIFT_COR
	if( uc_mode == 0){
#endif	//SEL_SHIFT_COR
		SlMeasureParameterA		=	GYRO_RAM_GX_ADIDAT ;		// Set Measure RAM Address
		SlMeasureParameterB		=	GYRO_RAM_GY_ADIDAT ;		// Set Measure RAM Address
#ifdef	SEL_SHIFT_COR
	}else{
		SlMeasureParameterA		=	GYRO_ZRAM_GZ_ADIDAT ;		// Set Measure RAM Address
		SlMeasureParameterB		=	GYRO_ZRAM_GZ_ADIDAT ;		// Set Measure RAM Address
	}
#endif	//SEL_SHIFT_COR

	MeasureStart( SlMeasureParameterNum , SlMeasureParameterA , SlMeasureParameterB ) ;					// Start measure

//	ClrMesFil();					// Clear Delay RAM
//	SetWaitTime(50) ;
//	SetWaitTime(1) ;

	MeasureWait() ;					// Wait complete of measurement

	RamRead32A( StMeasFunc_MFA_LLiIntegral1 		, &StMeasValueA.StUllnVal.UlLowVal ) ;	// X axis
	RamRead32A( StMeasFunc_MFA_LLiIntegral1 + 4		, &StMeasValueA.StUllnVal.UlHigVal ) ;
	RamRead32A( StMeasFunc_MFB_LLiIntegral2 		, &StMeasValueB.StUllnVal.UlLowVal ) ;	// Y axis
	RamRead32A( StMeasFunc_MFB_LLiIntegral2 + 4		, &StMeasValueB.StUllnVal.UlHigVal ) ;

	SlMeasureAveValueA = (INT_32)( (INT_64)StMeasValueA.UllnValue / SlMeasureParameterNum ) ;
	SlMeasureAveValueB = (INT_32)( (INT_64)StMeasValueB.UllnValue / SlMeasureParameterNum ) ;

	SlMeasureAveValueA = ( SlMeasureAveValueA >> 16 ) & 0x0000FFFF ;
	SlMeasureAveValueB = ( SlMeasureAveValueB >> 16 ) & 0x0000FFFF ;

//	SlMeasureAveValueA = 0x00010000 - SlMeasureAveValueA ;
//	SlMeasureAveValueB = 0x00010000 - SlMeasureAveValueB ;

	UlRsltSts = EXE_END ;
#ifdef	SEL_SHIFT_COR
	if( uc_mode == 0){
#endif	//SEL_SHIFT_COR
		StAdjPar.StGvcOff.UsGxoVal = ( UINT_16 )( SlMeasureAveValueA & 0x0000FFFF );		//Measure Result Store
		if(( (INT_16)StAdjPar.StGvcOff.UsGxoVal > (INT_16)GYROF_UPPER ) || ( (INT_16)StAdjPar.StGvcOff.UsGxoVal < (INT_16)GYROF_LOWER )){



			UlRsltSts |= EXE_GXADJ ;
		}
		RamWrite32A( GYRO_RAM_GXOFFZ , (( SlMeasureAveValueA << 16 ) & 0xFFFF0000 ) ) ;		// X axis Gyro offset

		StAdjPar.StGvcOff.UsGyoVal = ( UINT_16 )( SlMeasureAveValueB & 0x0000FFFF );		//Measure Result Store
		if(( (INT_16)StAdjPar.StGvcOff.UsGyoVal > (INT_16)GYROF_UPPER ) || ( (INT_16)StAdjPar.StGvcOff.UsGyoVal < (INT_16)GYROF_LOWER )){
			UlRsltSts |= EXE_GYADJ ;
		}
		RamWrite32A( GYRO_RAM_GYOFFZ , (( SlMeasureAveValueB << 16 ) & 0xFFFF0000 ) ) ;		// Y axis Gyro offset


		RamWrite32A( GYRO_RAM_GYROX_OFFSET , 0x00000000 ) ;			// X axis Drift Gyro offset
		RamWrite32A( GYRO_RAM_GYROY_OFFSET , 0x00000000 ) ;			// Y axis Drift Gyro offset
		RamWrite32A( GyroFilterDelayX_GXH1Z2 , 0x00000000 ) ;		// X axis H1Z2 Clear
		RamWrite32A( GyroFilterDelayY_GYH1Z2 , 0x00000000 ) ;		// Y axis H1Z2 Clear

#ifdef	SEL_SHIFT_COR
	}else{
		UsGzoVal = ( UINT_16 )( SlMeasureAveValueA & 0x0000FFFF );		//Measure Result Store
		if(( (INT_16)UsGzoVal > (INT_16)GYROF_UPPER ) || ( (INT_16)UsGzoVal < (INT_16)GYROF_LOWER )){
			UlRsltSts |= EXE_GZADJ ;
		}
		RamWrite32A( GYRO_ZRAM_GZOFFZ , (( SlMeasureAveValueA << 16 ) & 0xFFFF0000 ) ) ;		// Z axis Gyro offset

		RamWrite32A( GyroRAM_Z_GYRO_OFFSET , 0x00000000 ) ;			// Z axis Drift Gyro offset
	}
#endif	//SEL_SHIFT_COR
	return( UlRsltSts );


}


#ifdef	SEL_SHIFT_COR
//********************************************************************************
// Function Name 	: TneAvc
// Retun Value		: NON
// Argment Value	: NON
// Explanation		: Tunes the Accel VC offset for All
// History			: First edition
//********************************************************************************
#define 	ACCLOF_NUM		4096				// 4096times
UINT_32	TneAvc( UINT_8 ucposture )
{
	UINT_32			UlRsltSts;
	INT_32			SlMeasureParameterA , SlMeasureParameterB ;
	INT_32			SlMeasureParameterNum ;
	UnllnVal		StMeasValueA , StMeasValueB ;
	INT_32			SlMeasureAveValueA , SlMeasureAveValueB ;
	INT_32			SlMeasureRetValueX , SlMeasureRetValueY , SlMeasureRetValueZ;
	UINT_8			i , j , k;
	INT_32			SlDiff[3] ;

	UlRsltSts = EXE_END ;
	if( ucposture < 0x7f ){
		//平均値測定
		for( i=0 ; i<2 ; i++ )
		{
			MesFil( THROUGH ) ;					// Set Measure Filter

			SlMeasureParameterNum	=	ACCLOF_NUM ;					// Measurement times
			switch(i){
			case 0:
				SlMeasureParameterA		=	ACCLRAM_X_AC_ADIDAT ;			// Set Measure RAM Address
				SlMeasureParameterB		=	ACCLRAM_Y_AC_ADIDAT ;			// Set Measure RAM Address
				break;
			case 1:
				SlMeasureParameterA		=	ACCLRAM_Z_AC_ADIDAT ;			// Set Measure RAM Address
				SlMeasureParameterB		=	ACCLRAM_Z_AC_ADIDAT ;			// Set Measure RAM Address
				break;
			}

			MeasureStart( SlMeasureParameterNum , SlMeasureParameterA , SlMeasureParameterB ) ;					// Start measure

			MeasureWait() ;					// Wait complete of measurement

			RamRead32A( StMeasFunc_MFA_LLiIntegral1 		, &StMeasValueA.StUllnVal.UlLowVal ) ;
			RamRead32A( StMeasFunc_MFA_LLiIntegral1 + 4		, &StMeasValueA.StUllnVal.UlHigVal ) ;
			RamRead32A( StMeasFunc_MFB_LLiIntegral2 		, &StMeasValueB.StUllnVal.UlLowVal ) ;
			RamRead32A( StMeasFunc_MFB_LLiIntegral2 + 4		, &StMeasValueB.StUllnVal.UlHigVal ) ;

			SlMeasureAveValueA = (INT_32)( (INT_64)StMeasValueA.UllnValue / SlMeasureParameterNum ) ;
			SlMeasureAveValueB = (INT_32)( (INT_64)StMeasValueB.UllnValue / SlMeasureParameterNum ) ;

			switch(i){
			case 0:
				SlMeasureRetValueX = SlMeasureAveValueA ;
				SlMeasureRetValueY = SlMeasureAveValueB ;
				break;
			case 1:
				SlMeasureRetValueZ = SlMeasureAveValueA ;
				break;
			}

		}


TRACE("VAL(X,Y,Z) pos = \t%08xh\t%08xh\t%08xh\t%d \n",(INT_32)SlMeasureRetValueX ,(INT_32)SlMeasureRetValueY,(INT_32)SlMeasureRetValueZ, ucposture ) ;
		switch( ucposture ){
		case 0x10:	/* g = +Z only */
			if( SlMeasureRetValueZ < (INT_32)(POSTURETH_P<<16) ){
				UlRsltSts = EXE_AZADJ ;
				StPosOff.StPos.Pos[4][0] = 0;
				StPosOff.StPos.Pos[4][1] = 0;
				StPosOff.StPos.Pos[4][2] = 0;
TRACE(" POS14 [ERROR] \t%08xh < %08xh\n",(INT_32)(abs(SlMeasureRetValueZ)) , (INT_32)(POSTURETH_P<<16) ) ;
			}else{
				if( abs(SlMeasureRetValueX) > ZEROG_MRGN_XY )									UlRsltSts |= EXE_AXADJ ;
				if( abs(SlMeasureRetValueY) > ZEROG_MRGN_XY )									UlRsltSts |= EXE_AYADJ ;
				if( abs( (INT_32)(ACCL_SENS << 16) - abs(SlMeasureRetValueZ)) > ZEROG_MRGN_Z )	UlRsltSts |= EXE_AZADJ ;
				if( UlRsltSts == EXE_END ){
					StPosOff.UlAclOfSt |= 0x0000003F;
TRACE("POS14(X,Y,Z) st = \t%08xh\t%08xh\t%08xh\t%08xh \n",(INT_32)StPosOff.StPos.Pos[4][0] ,(INT_32)StPosOff.StPos.Pos[4][1],(INT_32)StPosOff.StPos.Pos[4][2],(INT_32)StPosOff.UlAclOfSt ) ;

					SlDiff[0] = SlMeasureRetValueX - (INT_32)0;
					SlDiff[1] = SlMeasureRetValueY - (INT_32)0;
					SlDiff[2] = SlMeasureRetValueZ - (INT_32)(ACCL_SENS << 16);
					
					StPosOff.StPos.Pos[4][0] = SlDiff[0];
					StPosOff.StPos.Pos[4][1] = SlDiff[1];
					StPosOff.StPos.Pos[4][2] = SlDiff[2];
					
TRACE("POS4(X,Y,Z) st = \t%08xh\t%08xh\t%08xh\t%08xh \n",(INT_32)StPosOff.StPos.Pos[4][0] ,(INT_32)StPosOff.StPos.Pos[4][1],(INT_32)StPosOff.StPos.Pos[4][2],(INT_32)StPosOff.UlAclOfSt ) ;

				}
			}
			break;

		}
	}else{
		switch(ucposture){
		case 0x80:	/* 計算 */

			if(StPosOff.UlAclOfSt == 0x3fL ){
				/*X offset*/
				StAclVal.StAccel.SlOffsetX = StPosOff.StPos.Pos[4][0] ;
				/*Y offset*/
				StAclVal.StAccel.SlOffsetY = StPosOff.StPos.Pos[4][1] ;
				/*Z offset*/
				StAclVal.StAccel.SlOffsetZ = StPosOff.StPos.Pos[4][2] ;
#ifdef DEBUG
TRACE("ACLOFST(X,Y,Z) = \t%08xh\t%08xh\t%08xh \n",(INT_32)StAclVal.StAccel.SlOffsetX ,(INT_32)StAclVal.StAccel.SlOffsetY,(INT_32)StAclVal.StAccel.SlOffsetZ   ) ;
#endif //DEBUG
				RamWrite32A( ACCLRAM_X_AC_OFFSET , StAclVal.StAccel.SlOffsetX ) ;	// X axis Accel offset
				RamWrite32A( ACCLRAM_Y_AC_OFFSET , StAclVal.StAccel.SlOffsetY ) ;	// Y axis Accel offset
				RamWrite32A( ACCLRAM_Z_AC_OFFSET , StAclVal.StAccel.SlOffsetZ ) ;	// Z axis Accel offset
				
				StAclVal.StAccel.SlOffsetX = ( StAclVal.StAccel.SlOffsetX >> 16 ) & 0x0000FFFF;
				StAclVal.StAccel.SlOffsetY = ( StAclVal.StAccel.SlOffsetY >> 16 ) & 0x0000FFFF;
				StAclVal.StAccel.SlOffsetZ = ( StAclVal.StAccel.SlOffsetZ >> 16 ) & 0x0000FFFF;
				
				for( j=0 ; j < 6 ; j++ ){
					k = 4 * j;
					RamWrite32A( AcclFilDly_X + k , 0x00000000 ) ;			// X axis Accl LPF Clear
					RamWrite32A( AcclFilDly_Y + k , 0x00000000 ) ;			// Y axis Accl LPF Clear
					RamWrite32A( AcclFilDly_Z + k , 0x00000000 ) ;			// Z axis Accl LPF Clear
				}

			}else{
				UlRsltSts = EXE_ERR ;
			}
			break;
			
		case 0xFF:	/* RAM clear */
			MemClr( ( UINT_8 * )&StPosOff, sizeof( stPosOff ) ) ;	// Adjust Parameter Clear
			MemClr( ( UINT_8 * )&StAclVal, sizeof( stAclVal ) ) ;	// Adjust Parameter Clear
//			StPosOff.UlAclOfSt = 0L;
			break;
		}
	}

TRACE(" Result = %08x\n",(INT_32)UlRsltSts ) ;
	return( UlRsltSts );


}
#endif	//SEL_SHIFT_COR


//********************************************************************************
// Function Name 	: RtnCen
// Retun Value		: Command Status
// Argment Value	: Command Parameter
// Explanation		: Return to center Command Function
// History			: First edition
//********************************************************************************
UINT_8	RtnCen( UINT_8	UcCmdPar )
{
	UINT_8	UcSndDat = FAILURE;

	if( !UcCmdPar ){								// X,Y centering
		RamWrite32A( CMD_RETURN_TO_CENTER , BOTH_SRV_ON ) ;
	}else if( UcCmdPar == XONLY_ON ){				// only X centering
		RamWrite32A( CMD_RETURN_TO_CENTER , XAXS_SRV_ON ) ;
	}else if( UcCmdPar == YONLY_ON ){				// only Y centering
		RamWrite32A( CMD_RETURN_TO_CENTER , YAXS_SRV_ON ) ;
#ifdef	SEL_CLOSED_AF
	}else if( UcCmdPar == ZONLY_OFF ){				// only Z centering off
		RamWrite32A( CMD_RETURN_TO_CENTER , ZAXS_SRV_OFF ) ;
	}else if( UcCmdPar == ZONLY_ON ){				// only Z centering
		RamWrite32A( CMD_RETURN_TO_CENTER , ZAXS_SRV_ON ) ;
#endif
	}else{											// Both off
		RamWrite32A( CMD_RETURN_TO_CENTER , BOTH_SRV_OFF ) ;
	}

	do {
		UcSndDat = RdStatus(1);
	} while( UcSndDat == FAILURE );

#ifdef	SEL_CLOSED_AF
	if( UcCmdPar == ZONLY_OFF ){				// only Z centering off
		RamWrite32A( CLAF_RAMA_AFOUT		,	0x00000000 ) ;				// DelayRam Clear
	}
#endif
TRACE("RtnCen() = %02x\n", UcSndDat ) ;
	return( UcSndDat );
}



//********************************************************************************
// Function Name 	: OisEna
// Retun Value		: NON
// Argment Value	: Command Parameter
// Explanation		: OIS Enable Control Function
// History			: First edition
//********************************************************************************
void	OisEna( void )
{
	UINT_8	UcStRd = 1;

	RamWrite32A( CMD_OIS_ENABLE , OIS_ENABLE ) ;
	while( UcStRd ) {
		UcStRd = RdStatus(1);
	}
TRACE(" OisEna( Status) = %02x\n", UcStRd ) ;
}

//********************************************************************************
// Function Name 	: OisEnaNCL
// Retun Value		: NON
// Argment Value	: Command Parameter
// Explanation		: OIS Enable Control Function w/o delay clear
// History			: First edition
//********************************************************************************
void	OisEnaNCL( void )
{
	UINT_8	UcStRd = 1;

	RamWrite32A( CMD_OIS_ENABLE , OIS_ENA_NCL | OIS_ENABLE ) ;
	while( UcStRd ) {
		UcStRd = RdStatus(1);
	}
TRACE(" OisEnaNCL( Status) = %02x\n", UcStRd ) ;
}

//********************************************************************************
// Function Name 	: OisEnaDrCl
// Retun Value		: NON
// Argment Value	: Command Parameter
// Explanation		: OIS Enable Control Function force drift cancel
// History			: First edition
//********************************************************************************
void	OisEnaDrCl( void )
{
	UINT_8	UcStRd = 1;

	RamWrite32A( CMD_OIS_ENABLE , OIS_ENA_DOF | OIS_ENABLE ) ;
	while( UcStRd ) {
		UcStRd = RdStatus(1);
	}
TRACE(" OisEnaDrCl( Status) = %02x\n", UcStRd ) ;
}

//********************************************************************************
// Function Name 	: OisEnaDrNcl
// Retun Value		: NON
// Argment Value	: Command Parameter
// Explanation		: OIS Enable Control Function w/o delay clear and force drift cancel
// History			: First edition
//********************************************************************************
void	OisEnaDrNcl( void )
{
	UINT_8	UcStRd = 1;

	RamWrite32A( CMD_OIS_ENABLE , OIS_ENA_DOF | OIS_ENA_NCL | OIS_ENABLE ) ;
	while( UcStRd ) {
		UcStRd = RdStatus(1);
	}
TRACE(" OisEnaDrCl( Status) = %02x\n", UcStRd ) ;
}
//********************************************************************************
// Function Name 	: OisDis
// Retun Value		: NON
// Argment Value	: Command Parameter
// Explanation		: OIS Disable Control Function
// History			: First edition
//********************************************************************************
void	OisDis( void )
{
	UINT_8	UcStRd = 1;

	RamWrite32A( CMD_OIS_ENABLE , OIS_DISABLE ) ;
	while( UcStRd ) {
		UcStRd = RdStatus(1);
	}
TRACE(" OisDis( Status) = %02x\n", UcStRd ) ;
}

//********************************************************************************
// Function Name 	: OisPause
// Retun Value		: NON
// Argment Value	: Command Parameter
// Explanation		: OIS pause Control Function
// History			: First edition 						
//********************************************************************************
void	OisPause( void )
{
	UINT_8	UcStRd = 1;
	
	RamWrite32A( CMD_OIS_ENABLE , OIS_DIS_PUS ) ;
	while( UcStRd ) {
		UcStRd = RdStatus(1);
	}
TRACE(" OisPause( Status , cnt ) = %02x\n", UcStRd ) ;
}

//********************************************************************************
// Function Name 	: SscEna
// Retun Value		: NON
// Argment Value	: Command Parameter
// Explanation		: Ssc Enable Control Function
// History			: First edition
//********************************************************************************
void	SscEna( void )
{
	UINT_8	UcStRd = 1;

	RamWrite32A( CMD_SSC_ENABLE , SSC_ENABLE ) ;
	while( UcStRd ) {
		UcStRd = RdStatus(1);
	}
TRACE(" SscEna( Status) = %02x\n", UcStRd ) ;
}

//********************************************************************************
// Function Name 	: SscDis
// Retun Value		: NON
// Argment Value	: Command Parameter
// Explanation		: Ssc Disable Control Function
// History			: First edition
//********************************************************************************
void	SscDis( void )
{
	UINT_8	UcStRd = 1;

	RamWrite32A( CMD_SSC_ENABLE , SSC_DISABLE ) ;
	while( UcStRd ) {
		UcStRd = RdStatus(1);
	}
TRACE(" SscDis( Status) = %02x\n", UcStRd ) ;
}


//********************************************************************************
// Function Name 	: SetRec
// Retun Value		: NON
// Argment Value	: Command Parameter
// Explanation		: Rec Mode Enable Function
// History			: First edition
//********************************************************************************
void	SetRec( void )
{
	UINT_8	UcStRd = 1;

	RamWrite32A( CMD_MOVE_STILL_MODE ,	MOVIE_MODE ) ;
	while( UcStRd ) {
		UcStRd = RdStatus(1);
	}
TRACE(" SetRec( Status) = %02x\n", UcStRd ) ;
}


//********************************************************************************
// Function Name 	: SetStill
// Retun Value		: NON
// Argment Value	: Command Parameter
// Explanation		: Set Still Mode Enable Function
// History			: First edition
//********************************************************************************
void	SetStill( void )
{
	UINT_8	UcStRd = 1;

	RamWrite32A( CMD_MOVE_STILL_MODE ,	STILL_MODE ) ;
	while( UcStRd ) {
		UcStRd = RdStatus(1);
	}
TRACE(" SetRec( Status) = %02x\n", UcStRd ) ;
}

//********************************************************************************
// Function Name 	: SetRecPreview
// Retun Value		: NON
// Argment Value	: Command Parameter
// Explanation		: Rec Preview Mode Enable Function
// History			: First edition
//********************************************************************************
void	SetRecPreview( UINT_8 mode )
{
	UINT_8	UcStRd = 1;

	switch( mode ){
	case 0:
		RamWrite32A( CMD_MOVE_STILL_MODE ,	MOVIE_MODE ) ;
		break;
	case 1:
		RamWrite32A( CMD_MOVE_STILL_MODE ,	MOVIE_MODE1 ) ;
		break;
	case 2:
		RamWrite32A( CMD_MOVE_STILL_MODE ,	MOVIE_MODE2 ) ;
		break;
	case 3:
		RamWrite32A( CMD_MOVE_STILL_MODE ,	MOVIE_MODE3 ) ;
		break;
	}
	while( UcStRd ) {
		UcStRd = RdStatus(1);
	}
//TRACE(" SetRec( %02x ) = %02x\n", mode , UcStRd ) ;
}


//********************************************************************************
// Function Name 	: SetStillPreview
// Retun Value		: NON
// Argment Value	: Command Parameter
// Explanation		: Set Still Preview Mode Enable Function
// History			: First edition
//********************************************************************************
void	SetStillPreview( UINT_8 mode )
{
	UINT_8	UcStRd = 1;

	switch( mode ){
	case 0:
		RamWrite32A( CMD_MOVE_STILL_MODE ,	STILL_MODE ) ;
		break;
	case 1:
		RamWrite32A( CMD_MOVE_STILL_MODE ,	STILL_MODE1 ) ;
		break;
	case 2:
		RamWrite32A( CMD_MOVE_STILL_MODE ,	STILL_MODE2 ) ;
		break;
	case 3:
		RamWrite32A( CMD_MOVE_STILL_MODE ,	STILL_MODE3 ) ;
		break;
	}
	while( UcStRd ) {
		UcStRd = RdStatus(1);
	}
//TRACE(" SetRec( %02x ) = %02x\n", mode , UcStRd ) ;
}


//********************************************************************************
// Function Name 	: SetSinWavePara
// Retun Value		: NON
// Argment Value	: NON
// Explanation		: Sine wave Test Function
// History			: First edition
//********************************************************************************
	/********* Parameter Setting *********/
	/* Servo Sampling Clock		=	20.0195kHz						*/
	/* Freq						=	SinFreq*80000000h/Fs			*/
	/* 05 00 XX MM 				XX:Freq MM:Sin or Circle */
const UINT_32	CucFreqVal[ 17 ]	= {
		0xFFFFFFFF,				//  0:  Stop
		0x0001A306,				//  1: 1Hz
		0x0003460B,				//  2: 2Hz
		0x0004E911,				//  3: 3Hz
		0x00068C16,				//  4: 4Hz
		0x00082F1C,				//  5: 5Hz
		0x0009D222,				//  6: 6Hz
		0x000B7527,				//  7: 7Hz
		0x000D182D,				//  8: 8Hz
		0x000EBB32,				//  9: 9Hz
		0x00105E38,				//  A: 10Hz
		0x0012013E,				//  B: 11Hz
		0x0013A443,				//  C: 12Hz
		0x00154749,				//  D: 13Hz
		0x0016EA4E,				//  E: 14Hz
		0x00188D54,				//  F: 15Hz
		0x001A305A				// 10: 16Hz
	} ;

// 	RamWrite32A( SinWave.Gain , 0x00000000 ) ;			// Gainはそれぞれ設定すること
// 	RamWrite32A( CosWave.Gain , 0x00000000 ) ;			// Gainはそれぞれ設定すること
void	SetSinWavePara( UINT_8 UcTableVal ,  UINT_8 UcMethodVal )
{
	UINT_32	UlFreqDat ;


	if(UcTableVal > 0x10 )
		UcTableVal = 0x10 ;			/* Limit */
	UlFreqDat = CucFreqVal[ UcTableVal ] ;

	if( UcMethodVal == CIRCWAVE) {
		RamWrite32A( SinWave_Phase	,	0x00000000 ) ;		// 正弦波の位相量
		RamWrite32A( CosWave_Phase 	,	0x20000000 );		// 正弦波の位相量
	}else{
		RamWrite32A( SinWave_Phase	,	0x00000000 ) ;		// 正弦波の位相量
		RamWrite32A( CosWave_Phase 	,	0x00000000 );		// 正弦波の位相量
	}


	if( UlFreqDat == 0xFFFFFFFF )			/* Sine波中止 */
	{
		RamWrite32A( SinWave_Offset		,	0x00000000 ) ;									// 発生周波数のオフセットを設定
		RamWrite32A( SinWave_Phase		,	0x00000000 ) ;									// 正弦波の位相量
//		RamWrite32A( SinWave_Gain		,	0x00000000 ) ;									// 発生周波数のアッテネータ(初期値は0[dB])
//		SetTransDataAdr( SinWave_OutAddr	,	 (UINT_32)SinWave_Output );			// 出力先アドレス

		RamWrite32A( CosWave_Offset		,	0x00000000 );									// 発生周波数のオフセットを設定
		RamWrite32A( CosWave_Phase 		,	0x00000000 );									// 正弦波の位相量
//		RamWrite32A( CosWave_Gain 		,	0x00000000 );									// 発生周波数のアッテネータ(初期値はCut)
//		SetTransDataAdr( CosWave_OutAddr	,	 (UINT_32)CosWave_Output );			// 出力先アドレス

		RamWrite32A( SinWaveC_Regsiter	,	0x00000000 ) ;									// Sine Wave Stop
		SetTransDataAdr( SinWave_OutAddr	,	0x00000000 ) ;		// 出力先アドレス
		SetTransDataAdr( CosWave_OutAddr	,	0x00000000 );		// 出力先アドレス
		RamWrite32A( HALL_RAM_HXOFF1		,	0x00000000 ) ;				// DelayRam Clear
		RamWrite32A( HALL_RAM_HYOFF1		,	0x00000000 ) ;				// DelayRam Clear
	}
	else
	{
		RamWrite32A( SinWave_Offset		,	UlFreqDat ) ;									// 発生周波数のオフセットを設定
		RamWrite32A( CosWave_Offset		,	UlFreqDat );									// 発生周波数のオフセットを設定

		RamWrite32A( SinWaveC_Regsiter	,	0x00000001 ) ;									// Sine Wave Start
		SetTransDataAdr( SinWave_OutAddr	,	(UINT_32)HALL_RAM_HXOFF1 ) ;		// 出力先アドレス
		SetTransDataAdr( CosWave_OutAddr	,	 (UINT_32)HALL_RAM_HYOFF1 );		// 出力先アドレス

	}


}




//********************************************************************************
// Function Name 	: SetPanTiltMode
// Retun Value		: NON
// Argment Value	: NON
// Explanation		: Pan-Tilt Enable/Disable
// History			: First edition
//********************************************************************************
void	SetPanTiltMode( UINT_8 UcPnTmod )
{
	UINT_8	UcStRd = 1;

	switch ( UcPnTmod ) {
		case OFF :
			RamWrite32A( CMD_PAN_TILT ,	PAN_TILT_OFF ) ;
			break ;
		case ON :
			RamWrite32A( CMD_PAN_TILT ,	PAN_TILT_ON ) ;
			break ;
	}

	while( UcStRd ) {
		UcStRd = RdStatus(1);
	}
TRACE(" PanTilt( Status) = %02x , %02x \n", UcStRd , UcPnTmod ) ;
}

 #ifdef	NEUTRAL_CENTER
//********************************************************************************
// Function Name 	: TneHvc
// Retun Value		: NON
// Argment Value	: NON
// Explanation		: Tunes the Hall VC offset
// History			: First edition
//********************************************************************************
UINT_8	TneHvc( void )
{
	UINT_8	UcRsltSts;
	INT_32			SlMeasureParameterA , SlMeasureParameterB ;
	INT_32			SlMeasureParameterNum ;
	UnllnVal		StMeasValueA , StMeasValueB ;
	INT_32			SlMeasureAveValueA , SlMeasureAveValueB ;

	RtnCen( BOTH_OFF ) ;		// Both OFF

	WitTim( 500 ) ;

	//平均値測定

	MesFil( THROUGH ) ;					// Set Measure Filter

	SlMeasureParameterNum	=	64 ;		// 64times
	SlMeasureParameterA		=	(UINT_32)HALL_RAM_HXIDAT ;		// Set Measure RAM Address
	SlMeasureParameterB		=	(UINT_32)HALL_RAM_HYIDAT ;		// Set Measure RAM Address

	MeasureStart( SlMeasureParameterNum , SlMeasureParameterA , SlMeasureParameterB ) ;					// Start measure

	ClrMesFil();					// Clear Delay RAM
	SetWaitTime(50) ;

	MeasureWait() ;					// Wait complete of measurement

	RamRead32A( StMeasFunc_MFA_LLiIntegral1 		, &StMeasValueA.StUllnVal.UlLowVal ) ;	// X axis
	RamRead32A( StMeasFunc_MFA_LLiIntegral1 + 4 	, &StMeasValueA.StUllnVal.UlHigVal ) ;
	RamRead32A( StMeasFunc_MFB_LLiIntegral2 		, &StMeasValueB.StUllnVal.UlLowVal ) ;	// Y axis
	RamRead32A( StMeasFunc_MFB_LLiIntegral2 + 4	, &StMeasValueB.StUllnVal.UlHigVal ) ;

	SlMeasureAveValueA = (INT_32)((( (INT_64)StMeasValueA.UllnValue * 100 ) / SlMeasureParameterNum ) / 100 ) ;
	SlMeasureAveValueB = (INT_32)((( (INT_64)StMeasValueB.UllnValue * 100 ) / SlMeasureParameterNum ) / 100 ) ;

	StAdjPar.StHalAdj.UsHlxCna = ( UINT_16 )(( SlMeasureAveValueA >> 16 ) & 0x0000FFFF );		//Measure Result Store
	StAdjPar.StHalAdj.UsHlxCen = StAdjPar.StHalAdj.UsHlxCna;											//Measure Result Store

	StAdjPar.StHalAdj.UsHlyCna = ( UINT_16 )(( SlMeasureAveValueB >> 16 ) & 0x0000FFFF );		//Measure Result Store
	StAdjPar.StHalAdj.UsHlyCen = StAdjPar.StHalAdj.UsHlyCna;											//Measure Result Store

	UcRsltSts = EXE_END ;				// Clear Status

	return( UcRsltSts );
}
 #endif	//NEUTRAL_CENTER

 #ifdef	NEUTRAL_CENTER_FINE
//********************************************************************************
// Function Name 	: TneFin
// Retun Value		: NON
// Argment Value	: NON
// Explanation		: Tunes the Hall VC offset current optimize
// History			: First edition
//********************************************************************************
void	TneFin( void )
{
	UINT_32	UlReadVal ;
	UINT_16	UsAdxOff, UsAdyOff ;
	INT_32			SlMeasureParameterNum ;
	INT_32			SlMeasureAveValueA , SlMeasureAveValueB ;
	UnllnVal		StMeasValueA , StMeasValueB ;
	UINT_32	UlMinimumValueA, UlMinimumValueB ;
	UINT_16	UsAdxMin, UsAdyMin ;
	UINT_8	UcFin ;

	// Get natural center offset
	RamRead32A( HALL_RAM_HXOFF,  &UlReadVal ) ;
	UsAdxOff = UsAdxMin = (UINT_16)( UlReadVal >> 16 ) ;

	RamRead32A( HALL_RAM_HYOFF,  &UlReadVal ) ;
	UsAdyOff = UsAdyMin = (UINT_16)( UlReadVal >> 16 ) ;
TRACE("*****************************************************\n" );
TRACE("TneFin: Before Adx=%04X, Ady=%04X\n", UsAdxOff, UsAdyOff );

	// Servo ON
	RtnCen( BOTH_ON ) ;
	WitTim( TNE ) ;

	MesFil( THROUGH ) ;					// Filter setting for measurement

	SlMeasureParameterNum = 2000 ;

	MeasureStart( SlMeasureParameterNum , HALL_RAM_HALL_X_OUT , HALL_RAM_HALL_Y_OUT ) ;					// Start measure

	MeasureWait() ;						// Wait complete of measurement

//	RamRead32A( StMeasFunc_MFA_SiMax1 , ( UINT_32 * )&SlMeasureMaxValueA ) ;		// Max value
//	RamRead32A( StMeasFunc_MFA_SiMin1 , ( UINT_32 * )&SlMeasureMinValueA ) ;		// Min value
//	RamRead32A( StMeasFunc_MFA_UiAmp1 , ( UINT_32 * )&SlMeasureAmpValueA ) ;		// Amp value
	RamRead32A( StMeasFunc_MFA_LLiIntegral1 	, &StMeasValueA.StUllnVal.UlLowVal ) ;	// X axis
	RamRead32A( StMeasFunc_MFA_LLiIntegral1 + 4 , &StMeasValueA.StUllnVal.UlHigVal ) ;
	SlMeasureAveValueA = (INT_32)((( (INT_64)StMeasValueA.UllnValue * 100 ) / SlMeasureParameterNum ) / 100 ) ;

//	RamRead32A( StMeasFunc_MFB_SiMax2 , ( UINT_32 * )&SlMeasureMaxValueB ) ;	// Max value
//	RamRead32A( StMeasFunc_MFB_SiMin2 , ( UINT_32 * )&SlMeasureMinValueB ) ;	// Min value
//	RamRead32A( StMeasFunc_MFB_UiAmp2 , ( UINT_32 * )&SlMeasureAmpValueB ) ;		// Amp value
	RamRead32A( StMeasFunc_MFB_LLiIntegral2 	, &StMeasValueB.StUllnVal.UlLowVal ) ;	// Y axis
	RamRead32A( StMeasFunc_MFB_LLiIntegral2 + 4	, &StMeasValueB.StUllnVal.UlHigVal ) ;
	SlMeasureAveValueB = (INT_32)((( (INT_64)StMeasValueB.UllnValue * 100 ) / SlMeasureParameterNum ) / 100 ) ;


//TRACE("TneFin: MaxA=%08X\n", SlMeasureMaxValueA );
//TRACE("        MinA=%08X\n", SlMeasureMinValueA );
//TRACE("        AmpA=%08X\n", SlMeasureAmpValueA );
TRACE("TneFin: AveA=%08X\n", SlMeasureAveValueA );

//TRACE("        MaxB=%08X\n", SlMeasureMaxValueB );
//TRACE("        MinB=%08X\n", SlMeasureMinValueB );
//TRACE("        AmpB=%08X\n", SlMeasureAmpValueB );
TRACE("        AveB=%08X\n", SlMeasureAveValueB );

	UlMinimumValueA = abs(SlMeasureAveValueA) ;
	UlMinimumValueB = abs(SlMeasureAveValueB) ;
	UcFin = 0x11 ;

	while( UcFin ) {
		if( UcFin & 0x01 ) {
			if( UlMinimumValueA >= abs(SlMeasureAveValueA) ) {
				UlMinimumValueA = abs(SlMeasureAveValueA) ;
				UsAdxMin = UsAdxOff ;
				// 収束を早めるために、出力値に比例させる
				if( SlMeasureAveValueA > 0 )
					UsAdxOff = (INT_16)UsAdxOff + (SlMeasureAveValueA >> 17) + 1 ;
				else
					UsAdxOff = (INT_16)UsAdxOff + (SlMeasureAveValueA >> 17) - 1 ;

				RamWrite32A( HALL_RAM_HXOFF,  (UINT_32)((UsAdxOff << 16 ) & 0xFFFF0000 )) ;
			} else {
TRACE("X fine\n");
				UcFin &= 0xFE ;		// clear exec flag X
			}
		}

		if( UcFin & 0x10 ) {
			if( UlMinimumValueB >= abs(SlMeasureAveValueB) ) {
				UlMinimumValueB = abs(SlMeasureAveValueB) ;
				UsAdyMin = UsAdyOff ;
				// 収束を早めるために、出力値に比例させる
				if( SlMeasureAveValueB > 0 )
					UsAdyOff = (INT_16)UsAdyOff + (SlMeasureAveValueB >> 17) + 1 ;
				else
					UsAdyOff = (INT_16)UsAdyOff + (SlMeasureAveValueB >> 17) - 1 ;

				RamWrite32A( HALL_RAM_HYOFF,  (UINT_32)((UsAdyOff << 16 ) & 0xFFFF0000 )) ;
			} else {
TRACE("Y fine\n");
				UcFin &= 0xEF ;		// clear exec flag Y
			}
		}

		MeasureStart( SlMeasureParameterNum , HALL_RAM_HALL_X_OUT , HALL_RAM_HALL_Y_OUT ) ;					// Start measure
		MeasureWait() ;						// Wait complete of measurement

//		RamRead32A( StMeasFunc_MFA_SiMax1 , ( UINT_32 * )&SlMeasureMaxValueA ) ;		// Max value
//		RamRead32A( StMeasFunc_MFA_SiMin1 , ( UINT_32 * )&SlMeasureMinValueA ) ;		// Min value
//		RamRead32A( StMeasFunc_MFA_UiAmp1 , ( UINT_32 * )&SlMeasureAmpValueA ) ;		// Amp value
		RamRead32A( StMeasFunc_MFA_LLiIntegral1 	, &StMeasValueA.StUllnVal.UlLowVal ) ;	// X axis
		RamRead32A( StMeasFunc_MFA_LLiIntegral1 + 4 , &StMeasValueA.StUllnVal.UlHigVal ) ;
		SlMeasureAveValueA = (INT_32)((( (INT_64)StMeasValueA.UllnValue * 100 ) / SlMeasureParameterNum ) / 100 ) ;

//		RamRead32A( StMeasFunc_MFB_SiMax2 , ( UINT_32 * )&SlMeasureMaxValueB ) ;	// Max value
//		RamRead32A( StMeasFunc_MFB_SiMin2 , ( UINT_32 * )&SlMeasureMinValueB ) ;	// Min value
//		RamRead32A( StMeasFunc_MFB_UiAmp2 , ( UINT_32 * )&SlMeasureAmpValueB ) ;		// Amp value
		RamRead32A( StMeasFunc_MFB_LLiIntegral2 	, &StMeasValueB.StUllnVal.UlLowVal ) ;	// Y axis
		RamRead32A( StMeasFunc_MFB_LLiIntegral2 + 4	, &StMeasValueB.StUllnVal.UlHigVal ) ;
		SlMeasureAveValueB = (INT_32)((( (INT_64)StMeasValueB.UllnValue * 100 ) / SlMeasureParameterNum ) / 100 ) ;

TRACE("-->Adx %04X, Ady %04X\n", UsAdxOff, UsAdyOff );

//TRACE("TneFin: MaxA=%08X\n", SlMeasureMaxValueA );
//TRACE("        MinA=%08X\n", SlMeasureMinValueA );
//TRACE("        AmpA=%08X\n", SlMeasureAmpValueA );
TRACE("TneFin: AveA=%08X\n", SlMeasureAveValueA );

//TRACE("        MaxB=%08X\n", SlMeasureMaxValueB );
//TRACE("        MinB=%08X\n", SlMeasureMinValueB );
//TRACE("        AmpB=%08X\n", SlMeasureAmpValueB );
TRACE("        AveB=%08X\n", SlMeasureAveValueB );
	}	// while


TRACE("TneFin: After Adx=%04X, Ady=%04X\n", UsAdxMin, UsAdyMin );


	StAdjPar.StHalAdj.UsHlxCna = UsAdxMin;								//Measure Result Store
	StAdjPar.StHalAdj.UsHlxCen = StAdjPar.StHalAdj.UsHlxCna;			//Measure Result Store

	StAdjPar.StHalAdj.UsHlyCna = UsAdyMin;								//Measure Result Store
	StAdjPar.StHalAdj.UsHlyCen = StAdjPar.StHalAdj.UsHlyCna;			//Measure Result Store

	StAdjPar.StHalAdj.UsAdxOff = StAdjPar.StHalAdj.UsHlxCna  ;
	StAdjPar.StHalAdj.UsAdyOff = StAdjPar.StHalAdj.UsHlyCna  ;

	// Servo OFF
	RtnCen( BOTH_OFF ) ;		// Both OFF

}
 #endif	//NEUTRAL_CENTER_FINE


//********************************************************************************
// Function Name 	: TneSltPos
// Retun Value		: NON
// Argment Value	: Position number(1, 2, 3, 4, 5, 6, 7, 0:reset)
// Explanation		: Move measurement position function
//********************************************************************************
void	TneSltPos( UINT_8 UcPos )
{
	INT_16 SsOff = 0x0000 ;
	INT_32	SlX, SlY;
//	UINT_8	outX, outY;

//	GetDir( &outX, &outY );		// Get direction and driver port of actuator

	UcPos &= 0x07 ;

	if ( UcPos ) {
		SsOff = SLT_OFFSET * (UcPos - 4);
		SsOff = (INT_16)(SsOff / sqrt(2));		// for circle limit(root2)
	}

#if 0
	if ( outX & 0x01 )	SsNvcX = -1;
	else				SsNvcX = 1;

	if ( outY & 0x01 )	SsNvcY = -1;
	else				SsNvcY = 1;

	SlX = (INT_32)((SsOff * SsNvcX) << 16);
	SlY = (INT_32)((SsOff * SsNvcY) << 16);

	if ( outX & 0x02 ) {
		// exchange port
		if ( outY & 0x01 )	SsNvcX = -1;
		else				SsNvcX = 1;

		if ( outX & 0x01 )	SsNvcY = -1;
		else				SsNvcY = 1;

		SlY = (INT_32)((SsOff * SsNvcX) << 16);
		SlX = (INT_32)((SsOff * SsNvcY) << 16);
	} else {
		// normal port
		if ( outX & 0x01 )	SsNvcX = -1;
		else				SsNvcX = 1;

		if ( outY & 0x01 )	SsNvcY = -1;
		else				SsNvcY = 1;

		SlX = (INT_32)((SsOff * SsNvcX) << 16);
		SlY = (INT_32)((SsOff * SsNvcY) << 16);
	}
#else
	SsNvcX = -1;
	SsNvcY = -1;

	SlX = (INT_32)((SsOff * SsNvcX) << 16);
	SlY = (INT_32)((SsOff * SsNvcY) << 16);
#endif

	RamWrite32A( HALL_RAM_GYROX_OUT, SlX ) ;
	RamWrite32A( HALL_RAM_GYROY_OUT, SlY ) ;

}

//********************************************************************************
// Function Name 	: TneVrtPos
// Retun Value		: NON
// Argment Value	: Position number(1, 2, 3, 4, 5, 6, 7, 0:reset)
// Explanation		: Move measurement position function
//********************************************************************************
void	TneVrtPos( UINT_8 UcPos )
{
	INT_16 SsOff = 0x0000 ;
	INT_32	SlX, SlY;
//	UINT_8	outX, outY;

//	GetDir( &outX, &outY );		// Get direction and driver port of actuator

	UcPos &= 0x07 ;

	if ( UcPos ) {
		SsOff = (INT_16)VRT_OFFSET * (UcPos - 4);
	}

		SsNvcY = 1;
		SlX = 0x00000000;
		SlY = (INT_32)((SsOff * SsNvcY) << 16);

	RamWrite32A( HALL_RAM_GYROX_OUT, SlX ) ;
	RamWrite32A( HALL_RAM_GYROY_OUT, SlY ) ;
}

//********************************************************************************
// Function Name 	: TneHrzPos
// Retun Value		: NON
// Argment Value	: Position number(1, 2, 3, 4, 5, 6, 7, 0:reset)
// Explanation		: Move measurement position function
//********************************************************************************
void	TneHrzPos( UINT_8 UcPos )
{
	INT_16 SsOff = 0x0000 ;
	INT_32	SlX, SlY;
//	UINT_8	outX, outY;

//	GetDir( &outX, &outY );		// Get direction and driver port of actuator

	UcPos &= 0x07 ;

	if ( UcPos ) {
		SsOff = (INT_16)HRZ_OFFSET * (UcPos - 4);
	}

		SsNvcX = 1;
		SlX = (INT_32)((SsOff * SsNvcX) << 16);
		SlY = 0x00000000;

	RamWrite32A( HALL_RAM_GYROX_OUT, SlX ) ;
	RamWrite32A( HALL_RAM_GYROY_OUT, SlY ) ;
}

//********************************************************************************
// Function Name 	: SetSinWavGenInt
// Retun Value		: NON
// Argment Value	: NON
// Explanation		: Sine wave generator initial Function
// History			: First edition
//********************************************************************************
void	SetSinWavGenInt( void )
{

	RamWrite32A( SinWave_Offset		,	0x00000000 ) ;		// 発生周波数のオフセットを設定
	RamWrite32A( SinWave_Phase		,	0x60000000 ) ;		// 正弦波の位相量
	RamWrite32A( SinWave_Gain		,	0x00000000 ) ;		// 発生周波数のアッテネータ(初期値は0[dB])
//	RamWrite32A( SinWave_Gain		,	0x7FFFFFFF ) ;		// 発生周波数のアッテネータ(初期値はCut)
//	SetTransDataAdr( SinWave_OutAddr	,	(UINT_32)SinWave_Output ) ;		// 初期値の出力先アドレスは、自分のメンバ

	RamWrite32A( CosWave_Offset		,	0x00000000 );		// 発生周波数のオフセットを設定
	RamWrite32A( CosWave_Phase 		,	0x00000000 );		// 正弦波の位相量
	RamWrite32A( CosWave_Gain 		,	0x00000000 );		// 発生周波数のアッテネータ(初期値はCut)
//	RamWrite32A( CosWave_Gain 		,	0x7FFFFFFF );		// 発生周波数のアッテネータ(初期値は0[dB])
//	SetTransDataAdr( CosWave_OutAddr	,	(UINT_32)CosWave_Output );		// 初期値の出力先アドレスは、自分のメンバ

	RamWrite32A( SinWaveC_Regsiter	,	0x00000000 ) ;								// Sine Wave Stop

}


//********************************************************************************
// Function Name 	: SetTransDataAdr
// Retun Value		: NON
// Argment Value	: NON
// Explanation		: Trans Address for Data Function
// History			: First edition
//********************************************************************************
void	SetTransDataAdr( UINT_16 UsLowAddress , UINT_32 UlLowAdrBeforeTrans )
{
	UnDwdVal	StTrsVal ;

	if( UlLowAdrBeforeTrans < 0x00009000 ){
		StTrsVal.StDwdVal.UsHigVal = (UINT_16)(( UlLowAdrBeforeTrans & 0x0000F000 ) >> 8 ) ;
		StTrsVal.StDwdVal.UsLowVal = (UINT_16)( UlLowAdrBeforeTrans & 0x00000FFF ) ;
	}else{
		StTrsVal.UlDwdVal = UlLowAdrBeforeTrans ;
	}
//TRACE(" TRANS  ADR = %04xh , DAT = %08xh \n",UsLowAddress , StTrsVal.UlDwdVal ) ;
	RamWrite32A( UsLowAddress	,	StTrsVal.UlDwdVal );

}


//********************************************************************************
// Function Name 	: RdStatus
// Retun Value		: 0:success 1:FAILURE
// Argment Value	: bit check  0:ALL  1:bit24
// Explanation		: High level status check Function
// History			: First edition
//********************************************************************************
UINT_8	RdStatus( UINT_8 UcStBitChk )
{
	UINT_32	UlReadVal ;

	RamRead32A( CMD_READ_STATUS , &UlReadVal );
TRACE(" (Rd St) = %08x\n", (UINT_32)UlReadVal ) ;
	if( UcStBitChk ){
		UlReadVal &= READ_STATUS_INI ;
	}
	if( !UlReadVal ){
		return( SUCCESS );
	}else{
		return( FAILURE );
	}
}


//********************************************************************************
// Function Name 	: DacControl
// Retun Value		: Firmware version
// Argment Value	: NON
// Explanation		: Dac Control Function
// History			: First edition
//********************************************************************************
void	DacControl( UINT_8 UcMode, UINT_32 UiChannel, UINT_32 PuiData )
{
	UINT_32	UlAddaInt ;
	if( !UcMode ) {
		RamWrite32A( CMD_IO_ADR_ACCESS , ADDA_DASEL ) ;
		RamWrite32A( CMD_IO_DAT_ACCESS , UiChannel ) ;
		RamWrite32A( CMD_IO_ADR_ACCESS , ADDA_DAO ) ;
		RamWrite32A( CMD_IO_DAT_ACCESS , PuiData ) ;
		;
		;
		UlAddaInt = 0x00000040 ;
		while ( (UlAddaInt & 0x00000040) != 0 ) {
			RamWrite32A( CMD_IO_ADR_ACCESS , ADDA_ADDAINT ) ;
			RamRead32A(  CMD_IO_DAT_ACCESS , &UlAddaInt ) ;
			;
		}
	} else {
		RamWrite32A( CMD_IO_ADR_ACCESS , ADDA_DASEL ) ;
		RamWrite32A( CMD_IO_DAT_ACCESS , UiChannel ) ;
		RamWrite32A( CMD_IO_ADR_ACCESS , ADDA_DAO ) ;
		RamRead32A(  CMD_IO_DAT_ACCESS , &PuiData ) ;
		;
		;
		UlAddaInt = 0x00000040 ;
		while ( (UlAddaInt & 0x00000040) != 0 ) {
			RamWrite32A( CMD_IO_ADR_ACCESS , ADDA_ADDAINT ) ;
			RamRead32A(  CMD_IO_DAT_ACCESS , &UlAddaInt ) ;
			;
		}
	}

	return ;
}

//********************************************************************************
// Function Name 	: WrHallCalData
// Retun Value		: SUCCESS or FAILURE
// Argment Value	: NON
// Explanation		: Flash Write Hall Calibration Data Function
// History			: First edition 									2015.7.14
//********************************************************************************
UINT_8	WrHallCalData( void )
{
	UINT_32	UiChkSum1,	UiChkSum2 ;
	UINT_32	UlSrvStat,	UlOisStat ;
	UINT_8 ans;
#ifdef	SEL_SHIFT_COR
	UINT_32	UlGzoff;

	RamRead32A( GYRO_ZRAM_GZOFFZ , &UlGzoff ) ;		// Z axis Gyro offset
#endif	//SEL_SHIFT_COR
//------------------------------------------------------------------------------------------------
// Servo Off & Get OIS enable status (for F40)
//------------------------------------------------------------------------------------------------
	RamRead32A( CMD_RETURN_TO_CENTER , &UlSrvStat ) ;
	RamRead32A( CMD_OIS_ENABLE , &UlOisStat ) ;
	RtnCen( BOTH_OFF ) ;		// Both OFF
//------------------------------------------------------------------------------------------------
// Backup ALL Calibration data
//------------------------------------------------------------------------------------------------
	ReadCalDataF40(UlBufDat, &UiChkSum2 );
//------------------------------------------------------------------------------------------------
// Sector erase NVR2 Calibration area
//------------------------------------------------------------------------------------------------
	ans = EraseCalDataF40();
	if ( ans == 0 ){
//------------------------------------------------------------------------------------------------
// Calibration Status flag set
//------------------------------------------------------------------------------------------------
#ifdef	SEL_CLOSED_AF
		UlBufDat[0] &= ~( HALL_CALB_FLG | CLAF_CALB_FLG | HALL_CALB_BIT );
#else
		UlBufDat[0] &= ~( HALL_CALB_FLG | HALL_CALB_BIT | ACCL_OFST_FLG );
#endif
		UlBufDat[0] |= StAdjPar.StHalAdj.UlAdjPhs ;							// Calibration Status
//------------------------------------------------------------------------------------------------
// Set Calibration data
//------------------------------------------------------------------------------------------------
		UlBufDat[ HALL_MAX_BEFORE_X	 ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlxMax << 16) ;		// OIS Hall X Max Before
		UlBufDat[ HALL_MIN_BEFORE_X	 ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlxMin << 16) ;		// OIS Hall X Min Before
		UlBufDat[ HALL_MAX_AFTER_X	 ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlxMxa << 16) ;		// OIS Hall X Max After
		UlBufDat[ HALL_MIN_AFTER_X	 ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlxMna << 16) ;		// OIS Hall X Min After
		UlBufDat[ HALL_MAX_BEFORE_Y	 ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlyMax << 16) ;		// OIS Hall Y Max Before
		UlBufDat[ HALL_MIN_BEFORE_Y	 ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlyMin << 16) ;		// OIS Hall Y Min Before
		UlBufDat[ HALL_MAX_AFTER_Y	 ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlyMxa << 16) ;		// OIS Hall Y Max After
		UlBufDat[ HALL_MIN_AFTER_Y	 ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlyMna << 16) ;		// OIS Hall Y Min After
		UlBufDat[ HALL_BIAS_DAC_X	 ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlxGan << 16) ;		// OIS Hall Bias X
		UlBufDat[ HALL_OFFSET_DAC_X	 ] 	= (UINT_32)(StAdjPar.StHalAdj.UsHlxOff) ;			// OIS Hall Offset X
		UlBufDat[ HALL_BIAS_DAC_Y	 ] 	= (UINT_32)(StAdjPar.StHalAdj.UsHlyGan << 16) ;		// OIS Hall Bias Y
		UlBufDat[ HALL_OFFSET_DAC_Y	 ] 	= (UINT_32)(StAdjPar.StHalAdj.UsHlyOff) ;			// OIS Hall Offset Y
		UlBufDat[ LOOP_GAIN_X		 ] 	= StAdjPar.StLopGan.UlLxgVal ;						// OIS Hall Loop Gain X
		UlBufDat[ LOOP_GAIN_Y		 ]	= StAdjPar.StLopGan.UlLygVal ;						// OIS Hall Loop Gain Y
		UlBufDat[ MECHA_CENTER_X	 ]	= (UINT_32)(StAdjPar.StHalAdj.UsAdxOff << 16) ;		// OIS Mecha center X
		UlBufDat[ MECHA_CENTER_Y	 ]	= (UINT_32)(StAdjPar.StHalAdj.UsAdyOff << 16) ;		// OIS Mecha center Y
		UlBufDat[ OPT_CENTER_X		 ]	= 0L;												// OIS optical center X
		UlBufDat[ OPT_CENTER_Y		 ]	= 0L;												// OIS optical center Y
		UlBufDat[ GYRO_OFFSET_X		 ]	= (UINT_32)(StAdjPar.StGvcOff.UsGxoVal << 16) ;		// OIS gyro offset X
		UlBufDat[ GYRO_OFFSET_Y		 ]	= (UINT_32)(StAdjPar.StGvcOff.UsGyoVal << 16) ;		// OIS gyro offset Y
//		UlBufDat[ GYRO_GAIN_X		 ]	= 0L;												// OIS gyro gain X
//		UlBufDat[ GYRO_GAIN_Y		 ]	= 0L;												// OIS gyro gain Y
#ifdef	SEL_CLOSED_AF
		UlBufDat[ AF_HALL_BIAS_DAC	 ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlzGan << 16) ;		// AF Hall Gain Value
		UlBufDat[ AF_HALL_OFFSET_DAC ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlzOff << 16) ;		// AF Hall Offset Value
		UlBufDat[ AF_LOOP_GAIN		 ]	= StAdjPar.StLopGan.UlLzgVal  ;						// AF Loop Gain
		UlBufDat[ AF_MECHA_CENTER	 ]	= (UINT_32)(StAdjPar.StHalAdj.UsAdzOff << 16) ;		// AF Mecha center
		UlBufDat[ AF_HALL_AMP_MAG	 ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlzAmp << 16) ;		// AF Hall Amp Magnification
		UlBufDat[ AF_HALL_MAX_BEFORE ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlzMax << 16) ;		// AF Hall Max Before
		UlBufDat[ AF_HALL_MIN_BEFORE ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlzMin << 16) ;		// AF Hall Min Before
		UlBufDat[ AF_HALL_MAX_AFTER	 ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlzMxa << 16) ; 	// AF Hall Max After
		UlBufDat[ AF_HALL_MIN_AFTER	 ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlzMna << 16) ;		// AF Hall Min After
#endif
#ifdef	SEL_SHIFT_COR
		UlBufDat[ GYRO_OFFSET_Z ] = UlGzoff ;												// OIS gyro offset Z
		
		UlBufDat[ ACCL_OFFSET_X ] 		= (UINT_32)(StAclVal.StAccel.SlOffsetX << 16) ;		// OIS accl offset X
		UlBufDat[ ACCL_OFFSET_Y ] 		= (UINT_32)(StAclVal.StAccel.SlOffsetY << 16) ;		// OIS accl offset Y
		UlBufDat[ ACCL_OFFSET_Z ] 		= (UINT_32)(StAclVal.StAccel.SlOffsetZ << 16) ;		// OIS accl offset Z
#endif	//SEL_SHIFT_COR

//------------------------------------------------------------------------------------------------
// Write Hall calibration data
//------------------------------------------------------------------------------------------------
		WriteCalDataF40( UlBufDat, &UiChkSum1 );							// NVR2へ調整値をwriteしCheckSumを計算
//------------------------------------------------------------------------------------------------
// Calculate calibration data checksum
//------------------------------------------------------------------------------------------------
		ReadCalDataF40( UlBufDat, &UiChkSum2 );								// NVR2へwriteされている調整値のCheckSumを計算

		if(UiChkSum1 != UiChkSum2 ){
			TRACE("CheckSum error\n");
			TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
			ans = 0x10;														// CheckSumエラー
		}
		TRACE("CheckSum OK\n");
		TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
	}
//------------------------------------------------------------------------------------------------
// Resume
//------------------------------------------------------------------------------------------------
	if( !UlSrvStat ) {
		RtnCen( BOTH_OFF ) ;
	} else if( UlSrvStat == 3 ) {
		RtnCen( BOTH_ON ) ;
	} else {
		RtnCen( UlSrvStat ) ;
	}

	if( UlOisStat != 0)               OisEna() ;

	return( ans );															// CheckSum OK
}


//********************************************************************************
// Function Name 	: WrGyroGainData
// Retun Value		: SUCCESS or FAILURE
// Argment Value	: NON
// Explanation		: Flash Write Gyro gain data Function
// History			: First edition 									2015.7.14
//********************************************************************************
UINT_8	WrGyroGainData( void )
{
	UINT_32	UlReadVal ;
	UINT_32	UiChkSum1,	UiChkSum2 ;
	UINT_32	UlSrvStat,	UlOisStat ;
	UINT_8 ans;
//------------------------------------------------------------------------------------------------
// Servo Off & Get OIS enable status (for F40)
//------------------------------------------------------------------------------------------------
	RamRead32A( CMD_RETURN_TO_CENTER , &UlSrvStat ) ;
	RamRead32A( CMD_OIS_ENABLE , &UlOisStat ) ;
	RtnCen( BOTH_OFF ) ;		// Both OFF
//------------------------------------------------------------------------------------------------
// Backup ALL Calibration data
//------------------------------------------------------------------------------------------------
	ReadCalDataF40( UlBufDat, &UiChkSum2 );
//------------------------------------------------------------------------------------------------
// Sector erase NVR2 Calibration area
//------------------------------------------------------------------------------------------------
	ans = EraseCalDataF40();
	if ( ans == 0 ){
//------------------------------------------------------------------------------------------------
// Calibration Status flag set
//------------------------------------------------------------------------------------------------
		UlBufDat[0] &= ~GYRO_GAIN_FLG;										// Calibration Status
//------------------------------------------------------------------------------------------------
// Set Calibration data
//------------------------------------------------------------------------------------------------
		RamRead32A(  GyroFilterTableX_gxzoom , &UlReadVal ) ;
		UlBufDat[ GYRO_GAIN_X ] 	= UlReadVal;							// OIS gyro gain X
		RamRead32A(  GyroFilterTableY_gyzoom , &UlReadVal ) ;
		UlBufDat[ GYRO_GAIN_Y ] 	= UlReadVal;							// OIS gyro gain Y
//------------------------------------------------------------------------------------------------
// Write gyro calibration data
//------------------------------------------------------------------------------------------------
		WriteCalDataF40( UlBufDat, &UiChkSum1 );							// NVR2へ調整値をwriteしCheckSumを計算
//------------------------------------------------------------------------------------------------
// Calculate calibration data checksum
//------------------------------------------------------------------------------------------------
		ReadCalDataF40( UlBufDat, &UiChkSum2 );								// NVR2へwriteされている調整値のCheckSumを計算

		if(UiChkSum1 != UiChkSum2 ){
			TRACE("CheckSum error\n");
			TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
			ans = 0x10;														// CheckSumエラー
		}
		TRACE("CheckSum OK\n");
		TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
	}
//------------------------------------------------------------------------------------------------
// Resume
//------------------------------------------------------------------------------------------------
	if( !UlSrvStat ) {
		RtnCen( BOTH_OFF ) ;
	} else if( UlSrvStat == 3 ) {
		RtnCen( BOTH_ON ) ;
	} else {
		RtnCen( UlSrvStat ) ;
	}

	if( UlOisStat != 0)               OisEna() ;

	return( ans );															// CheckSum OK
}


//********************************************************************************
// Function Name 	: WrGyroAngleData
// Retun Value		: SUCCESS or FAILURE
// Argment Value	: NON
// Explanation		: Flash Write Gyro Angle Data Function
// History			: First edition 									2015.7.14
//********************************************************************************
UINT_8	WrGyroAngleData( void )
{
	UINT_32	UlReadVal ;
	UINT_32	UiChkSum1,	UiChkSum2 ;
	UINT_32	UlSrvStat,	UlOisStat ;
	UINT_8	ans;
//------------------------------------------------------------------------------------------------
// Servo Off & Get OIS enable status (for F40)
//------------------------------------------------------------------------------------------------
	RamRead32A( CMD_RETURN_TO_CENTER , &UlSrvStat ) ;
	RamRead32A( CMD_OIS_ENABLE , &UlOisStat ) ;
	RtnCen( BOTH_OFF ) ;													// Both OFF
//------------------------------------------------------------------------------------------------
// Backup ALL Calibration data
//------------------------------------------------------------------------------------------------
	ReadCalDataF40( UlBufDat, &UiChkSum2 );
//------------------------------------------------------------------------------------------------
// Sector erase NVR2 Calibration area
//------------------------------------------------------------------------------------------------
	ans = EraseCalDataF40();
	if ( ans == 0 ){
//------------------------------------------------------------------------------------------------
// Calibration Status flag set
//------------------------------------------------------------------------------------------------
		UlReadVal = UlBufDat[0];											// FlashよりreadしたCalibrationStatusをセット
		UlReadVal &= ~ANGL_CORR_FLG;
//------------------------------------------------------------------------------------------------
// Set Calibration data
//------------------------------------------------------------------------------------------------
		UlBufDat[ 0 ] 	= UlReadVal;										// Calibration Status
		RamRead32A(  GyroFilterTableX_gx45x , &UlReadVal ) ;				// gx45x
		UlBufDat[ MIXING_GX45X ] 	= UlReadVal;
		RamRead32A(  GyroFilterTableX_gx45y , &UlReadVal ) ;				// gx45y
		UlBufDat[ MIXING_GX45Y ] 	= UlReadVal;
		RamRead32A(  GyroFilterTableY_gy45y , &UlReadVal ) ;				// gy45y
		UlBufDat[ MIXING_GY45Y ] 	= UlReadVal;
		RamRead32A(  GyroFilterTableY_gy45x , &UlReadVal ) ;				// gy45x
		UlBufDat[ MIXING_GY45X ] 	= UlReadVal;
//------------------------------------------------------------------------------------------------
// Write gyro angle data
//------------------------------------------------------------------------------------------------
		WriteCalDataF40( UlBufDat, &UiChkSum1 );							// NVR2へ調整値をwriteしCheckSumを計算
//------------------------------------------------------------------------------------------------
// Calculate calibration data checksum
//------------------------------------------------------------------------------------------------
		ReadCalDataF40( UlBufDat, &UiChkSum2 );								// NVR2へwriteされている調整値のCheckSumを計算

		if(UiChkSum1 != UiChkSum2 ){
			TRACE("CheckSum error\n");
			TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
			ans = 0x10;														// CheckSumエラー
		}
		TRACE("CheckSum OK\n");
		TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
	}
//------------------------------------------------------------------------------------------------
// Resume
//------------------------------------------------------------------------------------------------
	if( !UlSrvStat ) {
		RtnCen( BOTH_OFF ) ;
	} else if( UlSrvStat == 3 ) {
		RtnCen( BOTH_ON ) ;
	} else {
		RtnCen( UlSrvStat ) ;
	}

	if( UlOisStat != 0)               OisEna() ;

	return( ans );															// CheckSum OK
}


//********************************************************************************
// Function Name 	: WrGyroOffsetData
// Retun Value		: 0:OK, 1:NG
// Argment Value	: NON
// Explanation		: Flash Write Gyro offset Data Function
// History			: First edition
//********************************************************************************
UINT_8	WrGyroOffsetData( void )
{
	UINT_32	UlFctryX, UlFctryY;
	UINT_32	UlCurrX, UlCurrY;
	UINT_32	UlGofX, UlGofY;
	UINT_32	UiChkSum1,	UiChkSum2 ;
	UINT_32	UlSrvStat,	UlOisStat ;
	UINT_8	ans;
//------------------------------------------------------------------------------------------------
// Servo Off & Get OIS enable status (for F40)
//------------------------------------------------------------------------------------------------
	RamRead32A( CMD_RETURN_TO_CENTER , &UlSrvStat ) ;
	RamRead32A( CMD_OIS_ENABLE , &UlOisStat ) ;
	RtnCen( BOTH_OFF ) ;													// Both OFF
//------------------------------------------------------------------------------------------------
// Backup ALL Calibration data
//------------------------------------------------------------------------------------------------
	ReadCalDataF40( UlBufDat, &UiChkSum2 );
//------------------------------------------------------------------------------------------------
// Sector erase NVR2 Calibration area
//------------------------------------------------------------------------------------------------
	ans = EraseCalDataF40();
	if ( ans == 0 ){
//------------------------------------------------------------------------------------------------
// Set Calibration data
//------------------------------------------------------------------------------------------------
		RamRead32A(  GYRO_RAM_GXOFFZ , &UlGofX ) ;
		RamWrite32A( StCaliData_SiGyroOffset_X ,	UlGofX ) ;

		RamRead32A(  GYRO_RAM_GYOFFZ , &UlGofY ) ;
		RamWrite32A( StCaliData_SiGyroOffset_Y ,	UlGofY ) ;

		UlCurrX		= UlBufDat[ GYRO_OFFSET_X ] ;
		UlCurrY		= UlBufDat[ GYRO_OFFSET_Y ] ;
		UlFctryX	= UlBufDat[ GYRO_FCTRY_OFST_X ] ;
		UlFctryY	= UlBufDat[ GYRO_FCTRY_OFST_Y ] ;

		if( UlFctryX == 0xFFFFFFFF )
			UlBufDat[ GYRO_FCTRY_OFST_X ] = UlCurrX ;

		if( UlFctryY == 0xFFFFFFFF )
			UlBufDat[ GYRO_FCTRY_OFST_Y ] = UlCurrY ;

		UlBufDat[ GYRO_OFFSET_X ] = UlGofX ;
		UlBufDat[ GYRO_OFFSET_Y ] = UlGofY ;

//------------------------------------------------------------------------------------------------
// Write gyro angle data
//------------------------------------------------------------------------------------------------
		WriteCalDataF40( UlBufDat, &UiChkSum1 );							// NVR2へ調整値をwriteしCheckSumを計算
//------------------------------------------------------------------------------------------------
// Calculate calibration data checksum
//------------------------------------------------------------------------------------------------
		ReadCalDataF40( UlBufDat, &UiChkSum2 );								// NVR2へwriteされている調整値のCheckSumを計算

		if(UiChkSum1 != UiChkSum2 ){
			TRACE("CheckSum error\n");
			TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
			ans = 0x10;														// CheckSumエラー
		}
		TRACE("CheckSum OK\n");
		TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
	}
//------------------------------------------------------------------------------------------------
// Resume
//------------------------------------------------------------------------------------------------
	if( !UlSrvStat ) {
		RtnCen( BOTH_OFF ) ;
	} else if( UlSrvStat == 3 ) {
		RtnCen( BOTH_ON ) ;
	} else {
		RtnCen( UlSrvStat ) ;
	}

	if( UlOisStat != 0)               OisEna() ;

	return( ans );															// CheckSum OK

}

#ifdef	SEL_CLOSED_AF
//********************************************************************************
// Function Name 	: WrCLAFData
// Retun Value		: SUCCESS or FAILURE
// Argment Value	: NON
// Explanation		: Flash Write CL-AF Calibration Data Function
// History			: First edition 									2015.7.14
//********************************************************************************
UINT_8	WrCLAFData( void )
{
	UINT_32	UlReadVal ;
	UINT_32	UiChkSum1,	UiChkSum2 ;
	UINT_32	UlSrvStat,	UlOisStat ;
	UINT_8	ans;
//------------------------------------------------------------------------------------------------
// Servo Off & Get OIS enable status (for F40)
//------------------------------------------------------------------------------------------------
	RamRead32A( CMD_RETURN_TO_CENTER , &UlSrvStat ) ;
	RamRead32A( CMD_OIS_ENABLE , &UlOisStat ) ;
	RtnCen( BOTH_OFF ) ;													// Both OFF
//------------------------------------------------------------------------------------------------
// Backup ALL Calibration data
//------------------------------------------------------------------------------------------------
	ReadCalDataF40( UlBufDat, &UiChkSum2 );
//------------------------------------------------------------------------------------------------
// Sector erase NVR2 Calibration area
//------------------------------------------------------------------------------------------------
	ans = EraseCalDataF40();
	if ( ans == 0 ){
//------------------------------------------------------------------------------------------------
// Calibration Status flag set
//------------------------------------------------------------------------------------------------
		UlReadVal = UlBufDat[0];											// FlashよりreadしたCalibrationStatusをセット
		UlReadVal &= ~CLAF_CALB_FLG;
//------------------------------------------------------------------------------------------------
// Set Calibration data
//------------------------------------------------------------------------------------------------
// RAMアドレスが確定次第アドレス情報をセットする。
		UlBufDat[  0 ]	= UlReadVal;										// Calibration Status
		UlBufDat[ AF_HALL_BIAS_DAC ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlzGan << 16);		// AF Hall Bias
		UlBufDat[ AF_HALL_OFFSET_DAC ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlzOff << 16);		// AF Hall Offset
		UlBufDat[ AF_LOOP_GAIN ]		= StAdjPar.StLopGan.UlLzgVal;						// AF Loop Gain
		UlBufDat[ AF_MECHA_CENTER ]		= (UINT_32)(StAdjPar.StHalAdj.UsAdzOff << 16);		// AF Mecha Center
		UlBufDat[ AF_HALL_AMP_MAG ]		= (UINT_32)(StAdjPar.StHalAdj.UsHlzAmp << 16);		// AF Hall Amp Magnification
		UlBufDat[ AF_HALL_MAX_BEFORE ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlzMax << 16);		// AF Hall Max Before
		UlBufDat[ AF_HALL_MIN_BEFORE ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlzMin << 16);		// AF Hall Min Before
		UlBufDat[ AF_HALL_MAX_AFTER ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlzMxa << 16);		// AF Hall Max After
		UlBufDat[ AF_HALL_MIN_AFTER ]	= (UINT_32)(StAdjPar.StHalAdj.UsHlzMna << 16);		// AF Hall Min After
//------------------------------------------------------------------------------------------------
// Write gyro calibration data
//------------------------------------------------------------------------------------------------
		WriteCalDataF40( UlBufDat, &UiChkSum1 );							// NVR2へ調整値をwriteしCheckSumを計算
//------------------------------------------------------------------------------------------------
// Calculate calibration data checksum
//------------------------------------------------------------------------------------------------
		ReadCalDataF40( UlBufDat, &UiChkSum2 );								// NVR2へwriteされている調整値のCheckSumを計算

		if(UiChkSum1 != UiChkSum2 ){
			TRACE("CheckSum error\n");
			TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
			ans = 0x10;														// CheckSumエラー
		}
		TRACE("CheckSum OK\n");
		TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
	}
//------------------------------------------------------------------------------------------------
// Resume
//------------------------------------------------------------------------------------------------
	if( !UlSrvStat ) {
		RtnCen( BOTH_OFF ) ;
	} else if( UlSrvStat == 3 ) {
		RtnCen( BOTH_ON ) ;
	} else {
		RtnCen( UlSrvStat ) ;
	}

	if( UlOisStat != 0)               OisEna() ;

	return( ans );															// CheckSum OK
}
#endif


//********************************************************************************
// Function Name 	: WrMixingData
// Retun Value		: SUCCESS or FAILURE
// Argment Value	: NON
// Explanation		: Flash Write Mixing Data Function
// History			: First edition 									2016.01.27
//********************************************************************************
UINT_8	WrMixingData( void )
{
	UINT_32		UlReadVal ;
	UINT_32		UiChkSum1,	UiChkSum2 ;
	UINT_32		UlSrvStat,	UlOisStat ;
	UnDwdVal	StShift ;
	UINT_8		ans;
//------------------------------------------------------------------------------------------------
// Servo Off & Get OIS enable status (for F40)
//------------------------------------------------------------------------------------------------
	RamRead32A( CMD_RETURN_TO_CENTER , &UlSrvStat ) ;
	RamRead32A( CMD_OIS_ENABLE , &UlOisStat ) ;
	RtnCen( BOTH_OFF ) ;													// Both OFF
//------------------------------------------------------------------------------------------------
// Backup ALL Calibration data
//------------------------------------------------------------------------------------------------
	ReadCalDataF40( UlBufDat, &UiChkSum2 );
//------------------------------------------------------------------------------------------------
// Sector erase NVR2 Calibration area
//------------------------------------------------------------------------------------------------
	ans = EraseCalDataF40();
	if ( ans == 0 ){
//------------------------------------------------------------------------------------------------
// Calibration Status flag set
//------------------------------------------------------------------------------------------------
		UlReadVal = UlBufDat[0];											// FlashよりreadしたCalibrationStatusをセット
		UlReadVal &= ~CROS_TALK_FLG;
//------------------------------------------------------------------------------------------------
// Set Calibration data
//------------------------------------------------------------------------------------------------
		UlBufDat[ 0 ] 	= UlReadVal;										// Calibration Status
		RamRead32A(  HF_hx45x , &UlReadVal ) ;
		UlBufDat[ MIXING_HX45X	 ] 	= UlReadVal;
		RamRead32A(  HF_hx45y , &UlReadVal ) ;
		UlBufDat[ MIXING_HX45Y	 ] 	= UlReadVal;
		RamRead32A(  HF_hy45y , &UlReadVal ) ;
		UlBufDat[ MIXING_HY45Y	 ] 	= UlReadVal;
		RamRead32A(  HF_hy45x , &UlReadVal ) ;
		UlBufDat[ MIXING_HY45X	 ] 	= UlReadVal;
		RamRead32A(  HF_ShiftX, &StShift.UlDwdVal ) ;
//TRACE( "StShift.UlDwdVal=%08x\n", (UINT_32)StShift.UlDwdVal );
		UlBufDat[ MIXING_HXSX	 ]	= ((UINT_32)StShift.StCdwVal.UcRamVa0 << 16) | ((UINT_32)StShift.StCdwVal.UcRamVa1 << 0) ;
//TRACE( "UlBufDat[ MIXING_HXSX ]=%08x\n", (UINT_32)UlBufDat[ MIXING_HXSX	 ] );

//------------------------------------------------------------------------------------------------
// Write gyro calibration data
//------------------------------------------------------------------------------------------------
		WriteCalDataF40( UlBufDat, &UiChkSum1 );							// NVR2へ調整値をwriteしCheckSumを計算
//------------------------------------------------------------------------------------------------
// Calculate calibration data checksum
//------------------------------------------------------------------------------------------------
		ReadCalDataF40( UlBufDat, &UiChkSum2 );								// NVR2へwriteされている調整値のCheckSumを計算

		if(UiChkSum1 != UiChkSum2 ){
			TRACE("CheckSum error\n");
			TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
			ans = 0x10;														// CheckSumエラー
		}
		TRACE("CheckSum OK\n");
		TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
	}
//------------------------------------------------------------------------------------------------
// Resume
//------------------------------------------------------------------------------------------------
	if( !UlSrvStat ) {
		RtnCen( BOTH_OFF ) ;
	} else if( UlSrvStat == 3 ) {
		RtnCen( BOTH_ON ) ;
	} else {
		RtnCen( UlSrvStat ) ;
	}

	if( UlOisStat != 0)               OisEna() ;

	return( ans );															// CheckSum OK
}


//********************************************************************************
// Function Name 	: WrFstData
// Retun Value		: SUCCESS or FAILURE
// Argment Value	: NON
// Explanation		: Flash Write FST Data Function
// History			: First edition 									2016.4.28
//********************************************************************************
UINT_8	WrFstData( void )
{
	UINT_32	UlReadVal ;
	UINT_32	UiChkSum1,	UiChkSum2 ;
	UINT_32	UlSrvStat,	UlOisStat ;
	UINT_8	ans;
//------------------------------------------------------------------------------------------------
// Servo Off & Get OIS enable status (for F40)
//------------------------------------------------------------------------------------------------
	RamRead32A( CMD_RETURN_TO_CENTER , &UlSrvStat ) ;
	RamRead32A( CMD_OIS_ENABLE , &UlOisStat ) ;
	RtnCen( BOTH_OFF ) ;													// Both OFF
//------------------------------------------------------------------------------------------------
// Backup ALL Calibration data
//------------------------------------------------------------------------------------------------
	ReadCalDataF40( UlBufDat, &UiChkSum2 );
//------------------------------------------------------------------------------------------------
// Sector erase NVR2 Calibration area
//------------------------------------------------------------------------------------------------
	ans = EraseCalDataF40();
	if ( ans == 0 ){
//------------------------------------------------------------------------------------------------
// Calibration Status flag set
//------------------------------------------------------------------------------------------------
		UlReadVal = UlBufDat[0];											// FlashよりreadしたCalibrationStatusをセット
		UlReadVal &= ~OPAF_FST_FLG;
//------------------------------------------------------------------------------------------------
// Set Calibration data
//------------------------------------------------------------------------------------------------
		UlBufDat[ 0 ] 	= UlReadVal;										// Calibration Status

		RamRead32A(  OLAF_Long_M_RRMD1 , &UlReadVal ) ;						// RRMD1	Ft
		UlBufDat[ AF_LONG_M_RRMD1 ] 	= UlReadVal;
		RamRead32A(  OLAF_Long_I_RRMD1 , &UlReadVal ) ;						// RRMD1	Ft
		UlBufDat[ AF_LONG_I_RRMD1 ] 	= UlReadVal;

		RamRead32A(  OLAF_Short_IIM_RRMD1 , &UlReadVal ) ;					// RRMD1	Ft
		UlBufDat[ AF_SHORT_IIM_RRMD1 ] 	= UlReadVal;
		RamRead32A(  OLAF_Short_IMI_RRMD1 , &UlReadVal ) ;					// RRMD1	Ft
		UlBufDat[ AF_SHORT_IMI_RRMD1 ] 	= UlReadVal;

		RamRead32A(  OLAF_Short_MIM_RRMD1 , &UlReadVal ) ;					// RRMD1	Ft
		UlBufDat[ AF_SHORT_MIM_RRMD1 ] 	= UlReadVal;
		RamRead32A(  OLAF_Short_MMI_RRMD1 , &UlReadVal ) ;					// RRMD1	Ft
		UlBufDat[ AF_SHORT_MMI_RRMD1 ] 	= UlReadVal;

//------------------------------------------------------------------------------------------------
// Write gyro angle data
//------------------------------------------------------------------------------------------------
		WriteCalDataF40( UlBufDat, &UiChkSum1 );							// NVR2へ調整値をwriteしCheckSumを計算
//------------------------------------------------------------------------------------------------
// Calculate calibration data checksum
//------------------------------------------------------------------------------------------------
		ReadCalDataF40( UlBufDat, &UiChkSum2 );								// NVR2へwriteされている調整値のCheckSumを計算

		if(UiChkSum1 != UiChkSum2 ){
			TRACE("CheckSum error\n");
			TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
			ans = 0x10;														// CheckSumエラー
		}
		TRACE("CheckSum OK\n");
		TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
	}
//------------------------------------------------------------------------------------------------
// Resume
//------------------------------------------------------------------------------------------------
	if( !UlSrvStat ) {
		RtnCen( BOTH_OFF ) ;
	} else if( UlSrvStat == 3 ) {
		RtnCen( BOTH_ON ) ;
	} else {
		RtnCen( UlSrvStat ) ;
	}

	if( UlOisStat != 0)               OisEna() ;

	return( ans );															// CheckSum OK
}

//********************************************************************************
// Function Name 	: WrMixCalData
// Retun Value		: 0:OK, 1:NG
// Argment Value	: UcMode	0:disable	1:enable
//					: mlMixingValue *mixval
// Explanation		: Flash write mixing calibration data function
// History			: First edition
//********************************************************************************
UINT_8	WrMixCalData( UINT_8 UcMode, mlMixingValue *mixval )
{
	UINT_32	UlReadVal ;
	UINT_32	UiChkSum1,	UiChkSum2 ;
	UINT_32	UlSrvStat,	UlOisStat ;
	UINT_8	ans;
//	UINT_8	outX, outY;

//	GetDir( &outX, &outY );		// Get direction and driver port of actuator

//------------------------------------------------------------------------------------------------
// Servo Off & Get OIS enable status (for F40)
//------------------------------------------------------------------------------------------------
	RamRead32A( CMD_RETURN_TO_CENTER , &UlSrvStat ) ;
	RamRead32A( CMD_OIS_ENABLE , &UlOisStat ) ;
	RtnCen( BOTH_OFF ) ;													// Both OFF
//------------------------------------------------------------------------------------------------
// Backup ALL Calibration data
//------------------------------------------------------------------------------------------------
	ReadCalDataF40( UlBufDat, &UiChkSum2 );
//------------------------------------------------------------------------------------------------
// Sector erase NVR2 Calibration area
//------------------------------------------------------------------------------------------------
	ans = EraseCalDataF40();
	if ( ans == 0 ){
//------------------------------------------------------------------------------------------------
// Calibration Status flag set
//------------------------------------------------------------------------------------------------
		UlReadVal = UlBufDat[0];											// FlashよりreadしたCalibrationStatusをセット
		if( UcMode )
			UlReadVal &= ~CROS_TALK_FLG;
		else
			UlReadVal |= CROS_TALK_FLG;

//------------------------------------------------------------------------------------------------
// Set Calibration data
//------------------------------------------------------------------------------------------------
		UlBufDat[ 0 ] 	= UlReadVal;										// Calibration Status

		// normal port
		UlBufDat[ MIXING_HXSX ] = mixval->hysx | (((UINT_32)mixval->hxsx) << 16) ;

		mixval->hx45yL = (-1)*mixval->hx45yL;
		mixval->hy45xL = (-1)*mixval->hy45xL;
	
		if(mixval->hy45yL<0){			/* for MeasurementLibrary 1.X */
			mixval->hy45yL = (-1)*mixval->hy45yL;
			mixval->hx45yL = (-1)*mixval->hx45yL;
		}
		
		/* X と Y でStep 極性が違う時 */
		if(( (INT_16)VRT_OFFSET > 0 && (INT_16)HRZ_OFFSET < 0 ) || ( (INT_16)VRT_OFFSET < 0 && (INT_16)HRZ_OFFSET > 0 )){
			mixval->hx45yL = (-1)*mixval->hx45yL;
			mixval->hy45xL = (-1)*mixval->hy45xL;
		}
		
		if( UcMode ){
			UlBufDat[ MIXING_HX45X ] = mixval->hx45xL ;
			UlBufDat[ MIXING_HX45Y ] = mixval->hx45yL ;
			UlBufDat[ MIXING_HY45Y ] = mixval->hy45yL ;
			UlBufDat[ MIXING_HY45X ] = mixval->hy45xL ;
		}else{
			UlBufDat[ MIXING_HXSX ]  = (UINT_32)0xFFFFFFFF ;
			UlBufDat[ MIXING_HX45X ] = (UINT_32)0xFFFFFFFF ;
			UlBufDat[ MIXING_HX45Y ] = (UINT_32)0xFFFFFFFF ;
			UlBufDat[ MIXING_HY45Y ] = (UINT_32)0xFFFFFFFF ;
			UlBufDat[ MIXING_HY45X ] = (UINT_32)0xFFFFFFFF ;
		}

//------------------------------------------------------------------------------------------------
// Write gyro angle data
//------------------------------------------------------------------------------------------------
		WriteCalDataF40( UlBufDat, &UiChkSum1 );							// NVR2へ調整値をwriteしCheckSumを計算
//------------------------------------------------------------------------------------------------
// Calculate calibration data checksum
//------------------------------------------------------------------------------------------------
		ReadCalDataF40( UlBufDat, &UiChkSum2 );								// NVR2へwriteされている調整値のCheckSumを計算

		if(UiChkSum1 != UiChkSum2 ){
			TRACE("CheckSum error\n");
			TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
			ans = 0x10;														// CheckSumエラー
		}
		TRACE("CheckSum OK\n");
		TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
	}
//------------------------------------------------------------------------------------------------
// Resume
//------------------------------------------------------------------------------------------------
	if( !UlSrvStat ) {
		RtnCen( BOTH_OFF ) ;
	} else if( UlSrvStat == 3 ) {
		RtnCen( BOTH_ON ) ;
	} else {
		RtnCen( UlSrvStat ) ;
	}

	if( UlOisStat != 0)               OisEna() ;

	return( ans );
}

#ifdef	SEL_SHIFT_COR
//********************************************************************************
// Function Name 	: WrAclOffsetData
// Retun Value		: SUCCESS or FAILURE
// Argment Value	: NON
// Explanation		: Flash Write Accel offset Data Function
// History			: First edition 									2016.5.13
//********************************************************************************
UINT_8	WrAclOffsetData( void )
{
	UINT_32	SiChkSum1,	SiChkSum2 ;
	UINT_32	UlSrvStat,	UlOisStat ;
	UINT_8 	ans;
	UINT_8	 i , j=0;
//------------------------------------------------------------------------------------------------
// Servo Off & Get OIS enable status (for F40)
//------------------------------------------------------------------------------------------------
	RamRead32A( CMD_RETURN_TO_CENTER , &UlSrvStat ) ;
	RamRead32A( CMD_OIS_ENABLE , &UlOisStat ) ;
	RtnCen( BOTH_OFF ) ;		// Both OFF
//------------------------------------------------------------------------------------------------
// Backup ALL Calibration data
//------------------------------------------------------------------------------------------------
	ReadCalDataF40(UlBufDat, &SiChkSum2 );
//------------------------------------------------------------------------------------------------
// Sector erase NVR2 Calibration area
//------------------------------------------------------------------------------------------------
	ans = EraseCalDataF40();
	if ( ans == 0 ){
//------------------------------------------------------------------------------------------------
// Calibration Status flag set
//------------------------------------------------------------------------------------------------
		UlBufDat[0] &= ~ACCL_OFST_FLG ;
//------------------------------------------------------------------------------------------------
// Set Calibration data
//------------------------------------------------------------------------------------------------
		UlBufDat[ ACCL_OFFSET_X ] = StAclVal.StAccel.SlOffsetX;
		UlBufDat[ ACCL_OFFSET_Y ] = StAclVal.StAccel.SlOffsetY;
		UlBufDat[ ACCL_OFFSET_Z ] = StAclVal.StAccel.SlOffsetZ;
		for( i=ACCL_MTRX_0 ; i<=ACCL_MTRX_8 ; i++ ){
			UlBufDat[ i ] = StAclVal.SlInvMatrix[j];
			j++;
		}
//		UlBufDat[ 16 ] = StAclVal.StAccel.SlOffsetX;
//		UlBufDat[ 17 ] = StAclVal.StAccel.SlOffsetY;
//		UlBufDat[ 18 ] = StAclVal.StAccel.SlOffsetZ;
//		for( i=19 ; i<28 ; i++ ){
//			UlBufDat[ i ] = StAclVal.SlInvMatrix[j];
//			j++;
//		}

//------------------------------------------------------------------------------------------------
// Write Hall calibration data
//------------------------------------------------------------------------------------------------
		WriteCalDataF40( UlBufDat, &SiChkSum1 );											// NVR2へ調整値をwriteしCheckSumを計算
//------------------------------------------------------------------------------------------------
// Calculate calibration data checksum
//------------------------------------------------------------------------------------------------
		ReadCalDataF40( UlBufDat, &SiChkSum2 );											// NVR2へwriteされている調整値のCheckSumを計算

		if(SiChkSum1 != SiChkSum2 ){
			TRACE("CheckSum error\n");
			TRACE("SiChkSum1 = %08X, SiChkSum2 = %08X\n",(UINT_32)SiChkSum1, (UINT_32)SiChkSum2 );
			ans = 0x10;													// CheckSumエラー
		}
		TRACE("CheckSum OK\n");
		TRACE("SiChkSum1 = %08X, SiChkSum2 = %08X\n",(UINT_32)SiChkSum1, (UINT_32)SiChkSum2 );
	}
//------------------------------------------------------------------------------------------------
// Resume
//------------------------------------------------------------------------------------------------
	if( !UlSrvStat ) {
		RtnCen( BOTH_OFF ) ;
	} else if( UlSrvStat == 3 ) {
		RtnCen( BOTH_ON ) ;
	} else {
		RtnCen( UlSrvStat ) ;
	}

	if( UlOisStat != 0)		OisEna() ;

	return( ans );														// CheckSum OK
}
#endif	//SEL_SHIFT_COR

//********************************************************************************
// Function Name 	: ErCalData
// Retun Value		: 0:OK, 1:NG
// Argment Value	: UsFlag	HALL_CALB_FLG | GYRO_GAIN_FLG | ANGL_CORR_FLG | FOCL_GAIN_FLG
//					:			CLAF_CALB_FLG | HLLN_CALB_FLG | CROS_TALK_FLG
// Explanation		: Erase each calibration data function
// History			: First edition
//********************************************************************************
UINT_8	ErCalData( UINT_16 UsFlag )
{
	UINT_32	UlReadVal ;
	UINT_32	UiChkSum1,	UiChkSum2 ;
	UINT_32	UlSrvStat,	UlOisStat ;
	UINT_8	ans;
//------------------------------------------------------------------------------------------------
// Servo Off & Get OIS enable status (for F40)
//------------------------------------------------------------------------------------------------
	RamRead32A( CMD_RETURN_TO_CENTER , &UlSrvStat ) ;
	RamRead32A( CMD_OIS_ENABLE , &UlOisStat ) ;
	RtnCen( BOTH_OFF ) ;													// Both OFF
//------------------------------------------------------------------------------------------------
// Backup ALL Calibration data
//------------------------------------------------------------------------------------------------
	ReadCalDataF40( UlBufDat, &UiChkSum2 );
//------------------------------------------------------------------------------------------------
// Sector erase NVR2 Calibration area
//------------------------------------------------------------------------------------------------
	ans = EraseCalDataF40();
	if ( ans == 0 ){
//------------------------------------------------------------------------------------------------
// Calibration Status flag set
//------------------------------------------------------------------------------------------------
		UlReadVal = UlBufDat[0];											// FlashよりreadしたCalibrationStatusをセット
//------------------------------------------------------------------------------------------------
// Set Calibration data
//------------------------------------------------------------------------------------------------
		// Make clear bits
		UlReadVal |= (UINT_32)UsFlag ;

		UlBufDat[ 0 ] 	= UlReadVal ;										// Calibration Status

		// Erase hall calibration data
		if ( UsFlag & HALL_CALB_FLG ) {
			UlBufDat[ HALL_MAX_BEFORE_X	 ]	= 0xFFFFFFFF ;					// OIS Hall X Max Before
			UlBufDat[ HALL_MIN_BEFORE_X	 ]	= 0xFFFFFFFF ;					// OIS Hall X Min Before
			UlBufDat[ HALL_MAX_AFTER_X	 ]	= 0xFFFFFFFF ;					// OIS Hall X Max After
			UlBufDat[ HALL_MIN_AFTER_X	 ]	= 0xFFFFFFFF ;					// OIS Hall X Min After
			UlBufDat[ HALL_MAX_BEFORE_Y	 ]	= 0xFFFFFFFF ;					// OIS Hall Y Max Before
			UlBufDat[ HALL_MIN_BEFORE_Y	 ]	= 0xFFFFFFFF ;					// OIS Hall Y Min Before
			UlBufDat[ HALL_MAX_AFTER_Y	 ]	= 0xFFFFFFFF ;					// OIS Hall Y Max After
			UlBufDat[ HALL_MIN_AFTER_Y	 ]	= 0xFFFFFFFF ;					// OIS Hall Y Min After
			UlBufDat[ HALL_BIAS_DAC_X	 ]	= 0xFFFFFFFF ;					// OIS Hall Bias X
			UlBufDat[ HALL_OFFSET_DAC_X	 ] 	= 0xFFFFFFFF ;					// OIS Hall Offset X
			UlBufDat[ HALL_BIAS_DAC_Y	 ] 	= 0xFFFFFFFF ;					// OIS Hall Bias Y
			UlBufDat[ HALL_OFFSET_DAC_Y	 ] 	= 0xFFFFFFFF ;					// OIS Hall Offset Y
			UlBufDat[ LOOP_GAIN_X		 ] 	= 0xFFFFFFFF ;					// OIS Hall Loop Gain X
			UlBufDat[ LOOP_GAIN_Y		 ]	= 0xFFFFFFFF ;					// OIS Hall Loop Gain Y
			UlBufDat[ MECHA_CENTER_X	 ]	= 0xFFFFFFFF ;					// OIS Mecha center X
			UlBufDat[ MECHA_CENTER_Y	 ]	= 0xFFFFFFFF ;					// OIS Mecha center Y
			UlBufDat[ OPT_CENTER_X		 ]	= 0xFFFFFFFF ;					// OIS optical center X
			UlBufDat[ OPT_CENTER_Y		 ]	= 0xFFFFFFFF ;					// OIS optical center Y
			UlBufDat[ GYRO_OFFSET_X		 ]	= 0xFFFFFFFF ;					// OIS gyro offset X
			UlBufDat[ GYRO_OFFSET_Y		 ]	= 0xFFFFFFFF ;					// OIS gyro offset Y
#ifdef	SEL_SHIFT_COR
			UlBufDat[ GYRO_OFFSET_Z		 ]	= 0xFFFFFFFF ;					// OIS gyro offset Z
#endif	//SEL_SHIFT_COR
#ifdef	SEL_CLOSED_AF
			UlBufDat[ AF_HALL_BIAS_DAC	 ]	= 0xFFFFFFFF ;					// AF Hall Gain Value
			UlBufDat[ AF_HALL_OFFSET_DAC ]	= 0xFFFFFFFF ;					// AF Hall Offset Value
			UlBufDat[ AF_LOOP_GAIN		 ]	= 0xFFFFFFFF ;					// AF Loop Gain
			UlBufDat[ AF_MECHA_CENTER	 ]	= 0xFFFFFFFF ;					// AF Mecha center
			UlBufDat[ AF_HALL_AMP_MAG	 ]	= 0xFFFFFFFF ;					// AF Hall Amp Magnification
			UlBufDat[ AF_HALL_MAX_BEFORE ]	= 0xFFFFFFFF ;					// AF Hall Max Before
			UlBufDat[ AF_HALL_MIN_BEFORE ]	= 0xFFFFFFFF ;     				// AF Hall Min Before
			UlBufDat[ AF_HALL_MAX_AFTER	 ]	= 0xFFFFFFFF ;     				// AF Hall Max After
			UlBufDat[ AF_HALL_MIN_AFTER	 ]	= 0xFFFFFFFF ;      			// AF Hall Min After
#endif
		}

		// Erase gyro gain calibration data
		if ( UsFlag & GYRO_GAIN_FLG ) {
			UlBufDat[ GYRO_GAIN_X		 ]	= 0xFFFFFFFF ;					// OIS gyro gain X
			UlBufDat[ GYRO_GAIN_Y		 ]	= 0xFFFFFFFF ;					// OIS gyro gain Y
		}

		// Erase angle correction calibration data
		if ( UsFlag & ANGL_CORR_FLG ) {
			UlBufDat[ MIXING_GX45X		 ] 	= 0xFFFFFFFF ;
			UlBufDat[ MIXING_GX45Y		 ] 	= 0xFFFFFFFF ;
			UlBufDat[ MIXING_GY45Y		 ] 	= 0xFFFFFFFF ;
			UlBufDat[ MIXING_GY45X		 ] 	= 0xFFFFFFFF ;
		}

	#ifdef	SEL_CLOSED_AF
		// Erase close AF calibration data
		if ( UsFlag & CLAF_CALB_FLG ) {
			UlBufDat[ AF_HALL_BIAS_DAC	 ] 	= 0xFFFFFFFF ;
			UlBufDat[ AF_HALL_OFFSET_DAC ] 	= 0xFFFFFFFF ;
			UlBufDat[ AF_LOOP_GAIN		 ] 	= 0xFFFFFFFF ;
			UlBufDat[ AF_MECHA_CENTER	 ] 	= 0xFFFFFFFF ;
			UlBufDat[ AF_HALL_AMP_MAG	 ] 	= 0xFFFFFFFF ;
			UlBufDat[ AF_HALL_MAX_BEFORE ] 	= 0xFFFFFFFF ;
			UlBufDat[ AF_HALL_MIN_BEFORE ] 	= 0xFFFFFFFF ;
			UlBufDat[ AF_HALL_MAX_AFTER	 ] 	= 0xFFFFFFFF ;
			UlBufDat[ AF_HALL_MIN_AFTER	 ] 	= 0xFFFFFFFF ;
		}
	#else
		// Erase FST calibration data
		if ( UsFlag & OPAF_FST_FLG ) {
			UlBufDat[ AF_LONG_M_RRMD1	 ] 	= 0xFFFFFFFF ;
			UlBufDat[ AF_LONG_I_RRMD1	 ] 	= 0xFFFFFFFF ;
			UlBufDat[ AF_SHORT_IIM_RRMD1 ] 	= 0xFFFFFFFF ;
			UlBufDat[ AF_SHORT_IMI_RRMD1 ] 	= 0xFFFFFFFF ;
			UlBufDat[ AF_SHORT_MIM_RRMD1 ] 	= 0xFFFFFFFF ;
			UlBufDat[ AF_SHORT_MMI_RRMD1 ] 	= 0xFFFFFFFF ;
		}
	#endif

		// Erase linearity calibration data
		if ( UsFlag & HLLN_CALB_FLG ) {
			UlBufDat[ LN_POS1			 ]	= 0xFFFFFFFF ;		// Position 1
			UlBufDat[ LN_POS2			 ]	= 0xFFFFFFFF ;		// Position 2
			UlBufDat[ LN_POS3			 ]	= 0xFFFFFFFF ;		// Position 3
			UlBufDat[ LN_POS4			 ]	= 0xFFFFFFFF ;		// Position 4
			UlBufDat[ LN_POS5			 ]	= 0xFFFFFFFF ;		// Position 5
			UlBufDat[ LN_POS6			 ]	= 0xFFFFFFFF ;		// Position 6
			UlBufDat[ LN_POS7			 ]	= 0xFFFFFFFF ;		// Position 7
			UlBufDat[ LN_STEP			 ]	= 0xFFFFFFFF ;		// Step
		}

		// Erase gyro cross talk calibration data
		if ( UsFlag & CROS_TALK_FLG ) {
			UlBufDat[ MIXING_HX45X		 ] 	= 0xFFFFFFFF ;
			UlBufDat[ MIXING_HX45Y		 ] 	= 0xFFFFFFFF ;
			UlBufDat[ MIXING_HY45Y		 ] 	= 0xFFFFFFFF ;
			UlBufDat[ MIXING_HY45X		 ] 	= 0xFFFFFFFF ;
			UlBufDat[ MIXING_HXSX		 ] 	= 0xFFFFFFFF ;
		}

//------------------------------------------------------------------------------------------------
// Write gyro angle data
//------------------------------------------------------------------------------------------------
		WriteCalDataF40( UlBufDat, &UiChkSum1 );							// NVR2へ調整値をwriteしCheckSumを計算
//------------------------------------------------------------------------------------------------
// Calculate calibration data checksum
//------------------------------------------------------------------------------------------------
		ReadCalDataF40( UlBufDat, &UiChkSum2 );								// NVR2へwriteされている調整値のCheckSumを計算

		if(UiChkSum1 != UiChkSum2 ){
			TRACE("CheckSum error\n");
			TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
			ans = 0x10;														// CheckSumエラー
		}
		TRACE("CheckSum OK\n");
		TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
	}
//------------------------------------------------------------------------------------------------
// Resume
//------------------------------------------------------------------------------------------------
	if( !UlSrvStat ) {
		RtnCen( BOTH_OFF ) ;
	} else if( UlSrvStat == 3 ) {
		RtnCen( BOTH_ON ) ;
	} else {
		RtnCen( UlSrvStat ) ;
	}

	if( UlOisStat != 0)               OisEna() ;

	return( ans );															// CheckSum OK
}

//********************************************************************************
// Function Name 	: RdHallCalData
// Retun Value		: Read calibration data
// Argment Value	: NON
// Explanation		: Read calibration Data Function
// History			: First edition
//********************************************************************************
void	RdHallCalData( void )
{
	UnDwdVal		StReadVal ;

	RamRead32A(  StCaliData_UsCalibrationStatus, &StAdjPar.StHalAdj.UlAdjPhs ) ;

	RamRead32A( StCaliData_SiHallMax_Before_X,	&StReadVal.UlDwdVal ) ;
	StAdjPar.StHalAdj.UsHlxMax = StReadVal.StDwdVal.UsHigVal ;
	RamRead32A( StCaliData_SiHallMin_Before_X, &StReadVal.UlDwdVal ) ;
	StAdjPar.StHalAdj.UsHlxMin = StReadVal.StDwdVal.UsHigVal ;
	RamRead32A( StCaliData_SiHallMax_After_X,	&StReadVal.UlDwdVal ) ;
	StAdjPar.StHalAdj.UsHlxMxa = StReadVal.StDwdVal.UsHigVal ;
	RamRead32A( StCaliData_SiHallMin_After_X,	&StReadVal.UlDwdVal ) ;
	StAdjPar.StHalAdj.UsHlxMna = StReadVal.StDwdVal.UsHigVal ;
	RamRead32A( StCaliData_SiHallMax_Before_Y, &StReadVal.UlDwdVal ) ;
	StAdjPar.StHalAdj.UsHlyMax = StReadVal.StDwdVal.UsHigVal ;
	RamRead32A( StCaliData_SiHallMin_Before_Y, &StReadVal.UlDwdVal ) ;
	StAdjPar.StHalAdj.UsHlyMin = StReadVal.StDwdVal.UsHigVal ;
	RamRead32A( StCaliData_SiHallMax_After_Y,	&StReadVal.UlDwdVal ) ;
	StAdjPar.StHalAdj.UsHlyMxa = StReadVal.StDwdVal.UsHigVal ;
	RamRead32A( StCaliData_SiHallMin_After_Y,	&StReadVal.UlDwdVal ) ;
	StAdjPar.StHalAdj.UsHlyMna = StReadVal.StDwdVal.UsHigVal ;
	RamRead32A( StCaliData_UiHallBias_X,	&StReadVal.UlDwdVal ) ;
	StAdjPar.StHalAdj.UsHlxGan = StReadVal.StDwdVal.UsHigVal ;
	RamRead32A( StCaliData_UiHallOffset_X,	&StReadVal.UlDwdVal ) ;
	StAdjPar.StHalAdj.UsHlxOff = StReadVal.StDwdVal.UsHigVal ;
	RamRead32A( StCaliData_UiHallBias_Y,	&StReadVal.UlDwdVal ) ;
	StAdjPar.StHalAdj.UsHlyGan = StReadVal.StDwdVal.UsHigVal ;
	RamRead32A( StCaliData_UiHallOffset_Y,	&StReadVal.UlDwdVal ) ;
	StAdjPar.StHalAdj.UsHlyOff = StReadVal.StDwdVal.UsHigVal ;

	RamRead32A( StCaliData_SiLoopGain_X,	&StAdjPar.StLopGan.UlLxgVal ) ;
	RamRead32A( StCaliData_SiLoopGain_Y,	&StAdjPar.StLopGan.UlLygVal ) ;

	RamRead32A( StCaliData_SiLensCen_Offset_X,	&StReadVal.UlDwdVal ) ;
	StAdjPar.StHalAdj.UsAdxOff = StReadVal.StDwdVal.UsHigVal ;
	RamRead32A( StCaliData_SiLensCen_Offset_Y,	&StReadVal.UlDwdVal ) ;
	StAdjPar.StHalAdj.UsAdyOff = StReadVal.StDwdVal.UsHigVal ;

	RamRead32A( StCaliData_SiGyroOffset_X,		&StReadVal.UlDwdVal ) ;
	StAdjPar.StGvcOff.UsGxoVal = StReadVal.StDwdVal.UsHigVal ;
	RamRead32A( StCaliData_SiGyroOffset_Y,		&StReadVal.UlDwdVal ) ;
	StAdjPar.StGvcOff.UsGyoVal = StReadVal.StDwdVal.UsHigVal ;

}

//********************************************************************************
// Function Name 	: TneADO
// Retun Value		: 0x0000:PASS, 0x0001:X MAX OVER, 0x0002:Y MAX OVER, 0x0003:X MIN OVER, 0x0004:Y MIN OVER, FFFF:Verify error
//					: 0x0100:X MAX RANGE ERROR, 0x0200:Y MAX RANGE ERROR, 0x0300:X MIN RANGE ERROR, 0x0400:Y MIN ERROR
//					: 0x0005:WPB LOW ERROR
// Argment Value	:
// Explanation		: calculation margin Function
// History			: First edition
//********************************************************************************
UINT_16	TneADO( )
{
	INT_16 iRetVal;
	UINT_16	UsSts = 0 ;
	UnDwdVal rg ;
	INT_32 limit ;
	INT_32 gxgain ;
	INT_32 gygain ;
	INT_16 gout_x_marginp ;
	INT_16 gout_x_marginm ;
	INT_16 gout_y_marginp ;
	INT_16 gout_y_marginm ;

	INT_16 x_max ;
	INT_16 x_min ;
	INT_16 x_off ;
	INT_16 y_max ;
	INT_16 y_min ;
	INT_16 y_off ;
	INT_16 x_max_after ;
	INT_16 x_min_after ;
	INT_16 y_max_after ;
	INT_16 y_min_after ;
	INT_16 gout_x ;
	INT_16 gout_y ;

	UINT_32	UiChkSum1,	UiChkSum2 ;
	UINT_32	UlSrvStat,	UlOisStat ;

//------------------------------------------------------------------------------------------------
// Backup ALL Calibration data
//------------------------------------------------------------------------------------------------
	ReadCalDataF40( UlBufDat, &UiChkSum2 );

	// Read calibration data
	RdHallCalData();

	x_max = (INT_16)StAdjPar.StHalAdj.UsHlxMxa ;
	x_min = (INT_16)StAdjPar.StHalAdj.UsHlxMna ;
	x_off = (INT_16)StAdjPar.StHalAdj.UsAdxOff ;
	y_max = (INT_16)StAdjPar.StHalAdj.UsHlyMxa ;
	y_min = (INT_16)StAdjPar.StHalAdj.UsHlyMna ;
	y_off = (INT_16)StAdjPar.StHalAdj.UsAdyOff ;

	RamRead32A( GF_LimitX_HLIMT,	&limit ) ;
	RamRead32A( StCaliData_SiGyroGain_X,	&gxgain ) ;
	RamRead32A( StCaliData_SiGyroGain_Y,	&gygain ) ;
	RamRead32A( GyroFilterShiftX,	&rg.UlDwdVal ) ;

	x_max_after = (x_max - x_off) ;
	if (x_off < 0)
	{
	    if ((0x7FFF - abs(x_max)) < abs(x_off)) x_max_after = 0x7FFF ;
	}

	x_min_after = (x_min - x_off) ;
	if (x_off > 0)
	{
	    if ((0x7FFF - abs(x_min)) < abs(x_off)) x_min_after = 0x8001 ;
	}

	y_max_after = (y_max - y_off) ;
	if (y_off < 0)
	{
	    if ((0x7FFF - abs(y_max)) < abs(y_off)) y_max_after = 0x7FFF ;
	}

	y_min_after = (y_min - y_off);
	if (y_off > 0)
	{
	    if ((0x7FFF - abs(y_min)) < abs(y_off)) y_min_after = 0x8001 ;
	}

	gout_x = (INT_16)((INT_32)(((float)gxgain / 0x7FFFFFFF) * limit * (2^rg.StCdwVal.UcRamVa1)) >> 16);
	gout_y = (INT_16)((INT_32)(((float)gygain / 0x7FFFFFFF) * limit * (2^rg.StCdwVal.UcRamVa1)) >> 16);

TRACE( "RG_G2X4XB\t=\t%02X(%04X)\n", rg.StCdwVal.UcRamVa1, rg.UlDwdVal );
TRACE( "ADOFF X\t=\t0x%04X\r\n", x_off ) ;
TRACE( "ADOFF Y\t=\t0x%04X\r\n", y_off ) ;
TRACE( "MAX GOUT X\t=\t0x%04X\r\n", gout_x ) ;
TRACE( "MIN GOUT X\t=\t0x%04X\r\n", (gout_x * -1) ) ;
TRACE( "MAX GOUT Y\t=\t0x%04X\r\n", gout_y) ;
TRACE( "MIN GOUT Y\t=\t0x%04X\r\n", (gout_y * -1) ) ;

	gout_x_marginp = (INT_16)(gout_x + LENS_MARGIN);			// MARGIN X+
	gout_x_marginm = (INT_16)((gout_x + LENS_MARGIN) * -1);	// MARGIN X-
	gout_y_marginp = (INT_16)(gout_y + LENS_MARGIN);			// MARGIN Y+
	gout_y_marginm = (INT_16)((gout_y + LENS_MARGIN) * -1);	// MARGIN Y-

TRACE( "MAX GOUT with margin X\t=\t0x%04X\r\n", gout_x_marginp ) ;
TRACE( "MIN GOUT with margin X\t=\t0x%04X\r\n", gout_x_marginm ) ;
TRACE( "MAX GOUT with margin Y\t=\t0x%04X\r\n", gout_y_marginp ) ;
TRACE( "MIN GOUT with margin Y\t=\t0x%04X\r\n", gout_y_marginm ) ;

TRACE( "MAX AFTER X\t=\t0x%04X\r\n", x_max_after ) ;
TRACE( "MIN AFTER X\t=\t0x%04X\r\n", x_min_after ) ;
TRACE( "MAX AFTER Y\t=\t0x%04X\r\n", y_max_after ) ;
TRACE( "MIN AFTER Y\t=\t0x%04X\r\n", y_min_after ) ;

	// マージンがまったくないものは不良とする
	if (x_max_after < gout_x) {
		UsSts = 1 ;
	}
	else if (y_max_after < gout_y) {
		UsSts = 2 ;
	}
	else if (x_min_after > (gout_x * -1)) {
		UsSts = 3 ;
	}
	else if (y_min_after > (gout_y * -1)) {
		UsSts = 4 ;
	}
	else {
		// マージンオーバーであれば、ADOFFSETを更新する
		if (x_max_after < gout_x_marginp) {
			x_off -= (gout_x_marginp - x_max_after);
TRACE( "UPDATE ADOFF X\t=\t0x%04X\r\n", x_off ) ;
		}
		if (x_min_after > gout_x_marginm) {
			x_off += abs(x_min_after - gout_x_marginm);
TRACE( "UPDATE ADOFF X\t=\t0x%04X\r\n", x_off ) ;
		}
		if (y_max_after < gout_y_marginp) {
			y_off -= (gout_y_marginp - y_max_after);
TRACE( "UPDATE ADOFF Y\t=\t0x%04X\r\n", y_off ) ;
		}
		if (y_min_after > gout_y_marginm) {
			y_off += abs(y_min_after - gout_y_marginm);
TRACE( "UPDATE ADOFF Y\t=\t0x%04X\r\n", y_off ) ;
		}
		// マージンを変更したらFlashを更新する
		if ( (StAdjPar.StHalAdj.UsAdxOff != (UINT_16)x_off) || (StAdjPar.StHalAdj.UsAdyOff != (UINT_16)y_off) ) {
			StAdjPar.StHalAdj.UsAdxOff = x_off ;
			StAdjPar.StHalAdj.UsAdyOff = y_off ;

			RamWrite32A( StCaliData_SiLensCen_Offset_X ,	(UINT_32)(StAdjPar.StHalAdj.UsAdxOff << 16) ) ;
			RamWrite32A( StCaliData_SiLensCen_Offset_Y ,	(UINT_32)(StAdjPar.StHalAdj.UsAdyOff << 16) ) ;

			// Update flash calibration data
//------------------------------------------------------------------------------------------------
// Servo Off & Get OIS enable status (for F40)
//------------------------------------------------------------------------------------------------
			RamRead32A( CMD_RETURN_TO_CENTER , &UlSrvStat ) ;
			RamRead32A( CMD_OIS_ENABLE , &UlOisStat ) ;
			RtnCen( BOTH_OFF ) ;												// Both OFF
//------------------------------------------------------------------------------------------------
// Sector erase NVR2 Calibration area
//------------------------------------------------------------------------------------------------
			iRetVal = EraseCalDataF40();
			if ( iRetVal != 0 ) return( iRetVal );

			UlBufDat[ MECHA_CENTER_X ] = (UINT_32)(StAdjPar.StHalAdj.UsAdxOff << 16) ;
			UlBufDat[ MECHA_CENTER_Y ] = (UINT_32)(StAdjPar.StHalAdj.UsAdyOff << 16) ;

//------------------------------------------------------------------------------------------------
// Write gyro angle data
//------------------------------------------------------------------------------------------------
			WriteCalDataF40( UlBufDat, &UiChkSum1 );							// NVR2へ調整値をwriteしCheckSumを計算
//------------------------------------------------------------------------------------------------
// Calculate calibration data checksum
//------------------------------------------------------------------------------------------------
			ReadCalDataF40( UlBufDat, &UiChkSum2 );								// NVR2へwriteされている調整値のCheckSumを計算

			if(UiChkSum1 != UiChkSum2 ){
				TRACE("CheckSum error\n");
				TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
				iRetVal = 0x10;													// CheckSumエラー
				return( iRetVal );
			}
			TRACE("CheckSum OK\n");
			TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );

//------------------------------------------------------------------------------------------------
// Resume
//------------------------------------------------------------------------------------------------
			if( !UlSrvStat ) {
				RtnCen( BOTH_OFF ) ;
			} else if( UlSrvStat == 3 ) {
				RtnCen( BOTH_ON ) ;
			} else {
				RtnCen( UlSrvStat ) ;
			}

			if( UlOisStat != 0)               OisEna() ;

			// re calculate data
			x_max_after = (x_max - x_off) ;
			if (x_off < 0)
			{
			    if ((0x7FFF - abs(x_max)) < abs(x_off)) x_max_after = 0x7FFF ;
			}

			x_min_after = (x_min - x_off) ;
			if (x_off > 0)
			{
			    if ((0x7FFF - abs(x_min)) < abs(x_off)) x_min_after = 0x8001 ;
			}

			y_max_after = (y_max - y_off) ;
			if (y_off < 0)
			{
			    if ((0x7FFF - abs(y_max)) < abs(y_off)) y_max_after = 0x7FFF ;
			}

			y_min_after = (y_min - y_off);
			if (y_off > 0)
			{
			    if ((0x7FFF - abs(y_min)) < abs(y_off)) y_min_after = 0x8001 ;
			}
		}
	}

	// *******************************
	// effective range check
	// *******************************
	if (UsSts == 0) {
		UINT_16 UsReadVal ;
		float flDistanceX, flDistanceY ;
		float flDistanceAD = SLT_OFFSET * 6 ;

		UsReadVal = abs((UlBufDat[ LN_POS7 ] >> 16) - (UlBufDat[ LN_POS1 ] >> 16)) ;
		flDistanceX = ((float)UsReadVal) / 10.0f ;

		// effective range check
TRACE("DISTANCE (X, Y) pixel = (%04X", UsReadVal );

		UsReadVal = abs((UlBufDat[ LN_POS7 ] & 0xFFFF) - (UlBufDat[ LN_POS1 ] & 0xFFFF)) ;
		flDistanceY = ((float)UsReadVal) / 10.0f ;
TRACE(", %04X)\r\n", UsReadVal );

TRACE("DISTANCE (X, Y) pixel = (%f, %f)\r\n", flDistanceX, flDistanceY );
TRACE("X MAX um = %d\r\n", (INT_32)((x_max_after * (flDistanceX / flDistanceAD)) * PIXEL_SIZE) ) ;
TRACE("Y MAX um = %d\r\n", (INT_32)((y_max_after * (flDistanceY / flDistanceAD)) * PIXEL_SIZE) ) ;
TRACE("X MIN um = %d\r\n", (INT_32)((abs(x_min_after) * (flDistanceX / flDistanceAD)) * PIXEL_SIZE) ) ;
TRACE("Y MIN um = %d\r\n", (INT_32)((abs(y_min_after) * (flDistanceY / flDistanceAD)) * PIXEL_SIZE) ) ;
TRACE("SPEC PIXEL = %f\r\n", SPEC_PIXEL ) ;

		if ( (x_max_after * (flDistanceX / flDistanceAD)) < SPEC_PIXEL ) {
TRACE("X MAX < %dpixel\r\n", SPEC_PIXEL);
			// error
			UsSts |= 0x0100 ;
		}
		else if ( (y_max_after * (flDistanceY / flDistanceAD)) < SPEC_PIXEL ) {
TRACE("Y MAX < %dpixel\r\n", SPEC_PIXEL);
			// error
			UsSts |= 0x0200 ;
		}
		else if ( (abs(x_min_after) * (flDistanceX / flDistanceAD)) < SPEC_PIXEL ) {
TRACE("X MIN < %dpixel\r\n", SPEC_PIXEL);
			// error
			UsSts |= 0x0300 ;
		}
		else if ( (abs(y_min_after) * (flDistanceY / flDistanceAD)) < SPEC_PIXEL ) {
TRACE("Y MAX < %dpixel\r\n", SPEC_PIXEL);
			// error
			UsSts |= 0x0400 ;
		}
	}

	return( UsSts ) ;
}


#if 0
//********************************************************************************
// Function Name 	: SetHalLnData
// Retun Value		: 0:OK, 1:NG
// Argment Value	: non
// Explanation		: SRAM Write Hall Calibration Data Function
// History			: First edition
//********************************************************************************
void	SetHalLnData( UINT_16 *UsPara )
{
	UINT_8	i;
	UINT_16	UsRdAdr;

	UsRdAdr = HAL_LN_COEFAX;

	for( i=0; i<17 ; i++ ){
		RamWrite32A( UsRdAdr + (i * 4) , (UINT_32)UsPara[i*2+1] << 16 | (UINT_32)UsPara[i*2] );
	}
}
#endif

#if 0
//********************************************************************************
// Function Name 	: WrHalLnData
// Retun Value		: SUCCESS or FAILURE
// Argment Value	: NON
// Explanation		: Flash Write Hall Linearity correction value Function
// History			: First edition 									2016.01.27
//********************************************************************************
UINT_8	WrHalLnData( UINT_8 UcMode )
{
	UINT_32		UlReadVal ;
	UINT_32		UiChkSum1,	UiChkSum2 ;
	UINT_32		UlSrvStat,	UlOisStat ;
	UINT_16		UsRdAdr;
	UnDwdVal	StRdDat[17];
	UnDwdVal	StWrDat;
	UINT_8		i;
	UnDwdVal	StShift ;
	UINT_8		ans;
//------------------------------------------------------------------------------------------------
// Servo Off & Get OIS enable status (for F40)
//------------------------------------------------------------------------------------------------
	RamRead32A( CMD_RETURN_TO_CENTER , &UlSrvStat ) ;
	RamRead32A( CMD_OIS_ENABLE , &UlOisStat ) ;
	RtnCen( BOTH_OFF ) ;													// Both OFF
//------------------------------------------------------------------------------------------------
// Backup ALL Calibration data
//------------------------------------------------------------------------------------------------
	ReadCalDataF40( UlBufDat, &UiChkSum2 );
//------------------------------------------------------------------------------------------------
// Sector erase NVR2 Calibration area
//------------------------------------------------------------------------------------------------
	ans = EraseCalDataF40();
	if ( ans == 0 ){
//------------------------------------------------------------------------------------------------
// Calibration Status flag set
//------------------------------------------------------------------------------------------------
	UlReadVal = UlBufDat[0];												// FlashよりreadしたCalibrationStatusをセット
	if( UcMode ){
		UlReadVal &= ~HLLN_CALB_FLG;
	}else{
		UlReadVal |= HLLN_CALB_FLG;
	}
//------------------------------------------------------------------------------------------------
// Set Calibration data
//------------------------------------------------------------------------------------------------
	UlBufDat[ 0 ] 	= UlReadVal;											// Calibration Status

	UsRdAdr = HAL_LN_COEFAX;
	for( i=0; i<17 ; i++ ){
		RamRead32A( UsRdAdr + (i * 4) , &(StRdDat[i].UlDwdVal));
		//TRACE("StRdDat[ %04X ].UlDwdVal= %08X \n", i, StRdDat[i].UlDwdVal );
	}

	for( i=0; i<9 ; i++ ){
							//Y											//X
		StWrDat.UlDwdVal = (INT_32)StRdDat[i+8].StDwdVal.UsHigVal << 16 | StRdDat[i].StDwdVal.UsLowVal ;
		//TRACE(" %08X : ", StWrDat.UlDwdVal );
		//PUT_UINT_32( StWrDat.UlDwdVal , LN_ZONE1_COEFA + i * 8 ) ;					// coefficient store
		UlBufDat[ 44 + i*2 ] 	= StWrDat.UlDwdVal;
	}

	//TRACE("\n");
	for( i=0; i<8 ; i++ ){
							//Y											//X
		StWrDat.UlDwdVal = (INT_32)StRdDat[i+9].StDwdVal.UsLowVal << 16 | StRdDat[i].StDwdVal.UsHigVal ;
		//TRACE(" %08X : ", StWrDat.UlDwdVal );
		//PUT_UINT_32( StWrDat.UlDwdVal , LN_ZONE1_COEFA + 4 + i * 8 ) ;				// coefficient store
		UlBufDat[ 45 + i*2 ] 	= StWrDat.UlDwdVal;
	}
//------------------------------------------------------------------------------------------------
// Write gyro calibration data
//------------------------------------------------------------------------------------------------
		WriteCalDataF40( UlBufDat, &UiChkSum1 );							// NVR2へ調整値をwriteしCheckSumを計算
//------------------------------------------------------------------------------------------------
// Calculate calibration data checksum
//------------------------------------------------------------------------------------------------
		ReadCalDataF40( UlBufDat, &UiChkSum2 );								// NVR2へwriteされている調整値のCheckSumを計算

		if(UiChkSum1 != UiChkSum2 ){
			TRACE("CheckSum error\n");
			TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
			ans = 0x10;														// CheckSumエラー
		}
		TRACE("CheckSum OK\n");
		TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
	}
//------------------------------------------------------------------------------------------------
// Resume
//------------------------------------------------------------------------------------------------
	if( !UlSrvStat ) {
		RtnCen( BOTH_OFF ) ;
	} else if( UlSrvStat == 3 ) {
		RtnCen( BOTH_ON ) ;
	} else {
		RtnCen( UlSrvStat ) ;
	}

	if( UlOisStat != 0)               OisEna() ;

	return( ans );															// CheckSum OK
}

#endif


//********************************************************************************
// Function Name 	: WrLinCalData
// Retun Value		: 0:OK, 1:NG
// Argment Value	: UcMode	0:disable	1:enable
//					: mlLinearityValue *linval
// Explanation		: Flash write linearity correction data function
// History			: First edition
//********************************************************************************
UINT_8	WrLinCalData( UINT_8 UcMode, mlLinearityValue *linval )
{
	UINT_32		UlReadVal ;
	UINT_32		UiChkSum1,	UiChkSum2 ;
	UINT_32		UlSrvStat,	UlOisStat ;
	UINT_8		ans;
	double		*pPosX, *pPosY;

//------------------------------------------------------------------------------------------------
// Servo Off & Get OIS enable status (for F40)
//------------------------------------------------------------------------------------------------
	RamRead32A( CMD_RETURN_TO_CENTER , &UlSrvStat ) ;
	RamRead32A( CMD_OIS_ENABLE , &UlOisStat ) ;
	RtnCen( BOTH_OFF ) ;													// Both OFF

//------------------------------------------------------------------------------------------------
// Backup ALL Calibration data
//------------------------------------------------------------------------------------------------
	ReadCalDataF40( UlBufDat, &UiChkSum2 );
//------------------------------------------------------------------------------------------------
// Sector erase NVR2 Calibration area
//------------------------------------------------------------------------------------------------
	ans = EraseCalDataF40();
	if ( ans == 0 ) {
//------------------------------------------------------------------------------------------------
// Calibration Status flag set
//------------------------------------------------------------------------------------------------
		UlReadVal = UlBufDat[0];											// FlashよりreadしたCalibrationStatusをセット
		if( UcMode ){
			UlReadVal &= ~HLLN_CALB_FLG;
		}else{
			UlReadVal |= HLLN_CALB_FLG;
		}
//------------------------------------------------------------------------------------------------
// Set Calibration data
//------------------------------------------------------------------------------------------------
		UlBufDat[ 0 ] 	= UlReadVal;										// Calibration Status

#if 1
		pPosX = linval->positionX;
		pPosY = linval->positionY;
#else
		// exchange axis data
		pPosX = linval->positionY;
		pPosY = linval->positionX;
#endif

		if( UcMode ){
#ifdef	_BIG_ENDIAN_
			UlBufDat[ LN_POS1 ]	= (UINT_32)(*pPosX * 10) | ((UINT_32)(*pPosY *10) << 16); pPosX++; pPosY++;		// Position 2
			UlBufDat[ LN_POS2 ]	= (UINT_32)(*pPosX * 10) | ((UINT_32)(*pPosY *10) << 16); pPosX++; pPosY++;		// Position 2
			UlBufDat[ LN_POS3 ]	= (UINT_32)(*pPosX * 10) | ((UINT_32)(*pPosY *10) << 16); pPosX++; pPosY++;		// Position 3
			UlBufDat[ LN_POS4 ]	= (UINT_32)(*pPosX * 10) | ((UINT_32)(*pPosY *10) << 16); pPosX++; pPosY++;		// Position 4
			UlBufDat[ LN_POS5 ]	= (UINT_32)(*pPosX * 10) | ((UINT_32)(*pPosY *10) << 16); pPosX++; pPosY++;		// Position 5
			UlBufDat[ LN_POS6 ]	= (UINT_32)(*pPosX * 10) | ((UINT_32)(*pPosY *10) << 16); pPosX++; pPosY++;		// Position 6
			UlBufDat[ LN_POS7 ]	= (UINT_32)(*pPosX * 10) | ((UINT_32)(*pPosY *10) << 16); pPosX++; pPosY++;		// Position 7
#else
			UlBufDat[ LN_POS1 ]	= (UINT_32)(*pPosY * 10) | ((UINT_32)(*pPosX *10) << 16); pPosX++; pPosY++;		// Position 1
			UlBufDat[ LN_POS2 ]	= (UINT_32)(*pPosY * 10) | ((UINT_32)(*pPosX *10) << 16); pPosX++; pPosY++;		// Position 2
			UlBufDat[ LN_POS3 ]	= (UINT_32)(*pPosY * 10) | ((UINT_32)(*pPosX *10) << 16); pPosX++; pPosY++;		// Position 3
			UlBufDat[ LN_POS4 ]	= (UINT_32)(*pPosY * 10) | ((UINT_32)(*pPosX *10) << 16); pPosX++; pPosY++;		// Position 4
			UlBufDat[ LN_POS5 ]	= (UINT_32)(*pPosY * 10) | ((UINT_32)(*pPosX *10) << 16); pPosX++; pPosY++;		// Position 5
			UlBufDat[ LN_POS6 ]	= (UINT_32)(*pPosY * 10) | ((UINT_32)(*pPosX *10) << 16); pPosX++; pPosY++;		// Position 6
			UlBufDat[ LN_POS7 ]	= (UINT_32)(*pPosY * 10) | ((UINT_32)(*pPosX *10) << 16); pPosX++; pPosY++;		// Position 7
#endif
			UlBufDat[ LN_STEP ]	= ((linval->dacY[1] - linval->dacY[0]) >> 16) | ((linval->dacX[1] - linval->dacX[0]) & 0xFFFF0000);					// Step
		}else{
			UlBufDat[ LN_POS1 ]	= (UINT_32)0xFFFFFFFF;		// Position 1
			UlBufDat[ LN_POS2 ]	= (UINT_32)0xFFFFFFFF;		// Position 2
			UlBufDat[ LN_POS3 ]	= (UINT_32)0xFFFFFFFF;		// Position 3
			UlBufDat[ LN_POS4 ]	= (UINT_32)0xFFFFFFFF;		// Position 4
			UlBufDat[ LN_POS5 ]	= (UINT_32)0xFFFFFFFF;		// Position 5
			UlBufDat[ LN_POS6 ]	= (UINT_32)0xFFFFFFFF;		// Position 6
			UlBufDat[ LN_POS7 ]	= (UINT_32)0xFFFFFFFF;		// Position 7
			UlBufDat[ LN_STEP ]	= (UINT_32)0xFFFFFFFF;		// Step
		}
		TRACE("STEP=%d (0x%x)\n", (int)UlBufDat[ LN_STEP ], UlBufDat[ LN_STEP ] );
//------------------------------------------------------------------------------------------------
// Write gyro calibration data
//------------------------------------------------------------------------------------------------
		WriteCalDataF40( UlBufDat, &UiChkSum1 );							// NVR2へ調整値をwriteしCheckSumを計算
//------------------------------------------------------------------------------------------------
// Calculate calibration data checksum
//------------------------------------------------------------------------------------------------
		ReadCalDataF40( UlBufDat, &UiChkSum2 );								// NVR2へwriteされている調整値のCheckSumを計算

		if(UiChkSum1 != UiChkSum2 ){
			TRACE("CheckSum error\n");
			TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
			ans = 0x10;														// CheckSumエラー
		}
		TRACE("CheckSum OK\n");
		TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
	}
//------------------------------------------------------------------------------------------------
// Resume
//------------------------------------------------------------------------------------------------
	if( !UlSrvStat ) {
		RtnCen( BOTH_OFF ) ;
	} else if( UlSrvStat == 3 ) {
		RtnCen( BOTH_ON ) ;
	} else {
		RtnCen( UlSrvStat ) ;
	}

	if( UlOisStat != 0)               OisEna() ;

	return( ans );															// CheckSum OK
}


//********************************************************************************
// Function Name 	: WrLinMixCalData
// Retun Value		: 0:OK, 1:NG
// Argment Value	: UcMode	0:disable	1:enable
//					: mlMixingValue *mixval
//					: mlLinearityValue *linval
// Explanation		: Flash write linearity correction data function
// History			: First edition
//********************************************************************************
UINT_8	WrLinMixCalData( UINT_8 UcMode, mlMixingValue *mixval, mlLinearityValue *linval )
{
	UINT_32		UlReadVal ;
	UINT_32		UiChkSum1,	UiChkSum2 ;
	UINT_32		UlSrvStat,	UlOisStat ;
	UINT_8		ans;
	double		*pPosX, *pPosY;

//	UINT_8		outX, outY;

//	GetDir( &outX, &outY );		// Get direction and driver port of actuator

//------------------------------------------------------------------------------------------------
// Servo Off & Get OIS enable status (for F40)
//------------------------------------------------------------------------------------------------
	RamRead32A( CMD_RETURN_TO_CENTER , &UlSrvStat ) ;
	RamRead32A( CMD_OIS_ENABLE , &UlOisStat ) ;
	RtnCen( BOTH_OFF ) ;													// Both OFF

//------------------------------------------------------------------------------------------------
// Backup ALL Calibration data
//------------------------------------------------------------------------------------------------
	ReadCalDataF40( UlBufDat, &UiChkSum2 );
//------------------------------------------------------------------------------------------------
// Sector erase NVR2 Calibration area
//------------------------------------------------------------------------------------------------
	ans = EraseCalDataF40();
	if ( ans == 0 ) {
//------------------------------------------------------------------------------------------------
// Calibration Status flag set
//------------------------------------------------------------------------------------------------
		UlReadVal = UlBufDat[0];											// FlashよりreadしたCalibrationStatusをセット
		if( UcMode ){
			UlReadVal &= ~(HLLN_CALB_FLG | CROS_TALK_FLG);
		}else{
			UlReadVal |= (HLLN_CALB_FLG | CROS_TALK_FLG);
		}

//------------------------------------------------------------------------------------------------
// Set Calibration data
//------------------------------------------------------------------------------------------------
		UlBufDat[ 0 ] 	= UlReadVal;										// Calibration Status

		if( UcMode ){
			// mixing correction data
			//------------------------------------------------------------------------------------------------
			// normal port
			UlBufDat[ MIXING_HXSX ] = mixval->hysx | (((UINT_32)mixval->hxsx) << 16) ;

			mixval->hx45yL = (-1)*mixval->hx45yL;
			mixval->hy45xL = (-1)*mixval->hy45xL;
			
			if(mixval->hy45yL<0){			/* for MeasurementLibrary 1.X */
				mixval->hy45yL = (-1)*mixval->hy45yL;
				mixval->hx45yL = (-1)*mixval->hx45yL;
			}
			
			/* X と Y でStep 極性が違う時 */
			if(( (INT_32)linval->dacY[4] > 0 && (INT_32)linval->dacX[4] < 0 ) || ( (INT_32)linval->dacY[4] < 0 && (INT_32)linval->dacX[4] > 0 )){
				mixval->hx45yL = (-1)*mixval->hx45yL;
				mixval->hy45xL = (-1)*mixval->hy45xL;
			}
		
			
			UlBufDat[ MIXING_HX45X ] = mixval->hx45xL ;
			UlBufDat[ MIXING_HX45Y ] = mixval->hx45yL ;
			UlBufDat[ MIXING_HY45Y ] = mixval->hy45yL ;
			UlBufDat[ MIXING_HY45X ] = mixval->hy45xL ;

			// linearity correction data
			//------------------------------------------------------------------------------------------------
#if 1
			pPosX = linval->positionX;
			pPosY = linval->positionY;
#else
			// exchange axis data
			pPosX = linval->positionY;
			pPosY = linval->positionX;
#endif

#ifdef	_BIG_ENDIAN_
			UlBufDat[ LN_POS1 ]	= (UINT_32)(*pPosX * 10) | ((UINT_32)(*pPosY *10) << 16); pPosX++; pPosY++;		// Position 2
			UlBufDat[ LN_POS2 ]	= (UINT_32)(*pPosX * 10) | ((UINT_32)(*pPosY *10) << 16); pPosX++; pPosY++;		// Position 2
			UlBufDat[ LN_POS3 ]	= (UINT_32)(*pPosX * 10) | ((UINT_32)(*pPosY *10) << 16); pPosX++; pPosY++;		// Position 3
			UlBufDat[ LN_POS4 ]	= (UINT_32)(*pPosX * 10) | ((UINT_32)(*pPosY *10) << 16); pPosX++; pPosY++;		// Position 4
			UlBufDat[ LN_POS5 ]	= (UINT_32)(*pPosX * 10) | ((UINT_32)(*pPosY *10) << 16); pPosX++; pPosY++;		// Position 5
			UlBufDat[ LN_POS6 ]	= (UINT_32)(*pPosX * 10) | ((UINT_32)(*pPosY *10) << 16); pPosX++; pPosY++;		// Position 6
			UlBufDat[ LN_POS7 ]	= (UINT_32)(*pPosX * 10) | ((UINT_32)(*pPosY *10) << 16); pPosX++; pPosY++;		// Position 7
#else
			UlBufDat[ LN_POS1 ]	= (UINT_32)(*pPosY * 10) | ((UINT_32)(*pPosX *10) << 16); pPosX++; pPosY++;		// Position 1
			UlBufDat[ LN_POS2 ]	= (UINT_32)(*pPosY * 10) | ((UINT_32)(*pPosX *10) << 16); pPosX++; pPosY++;		// Position 2
			UlBufDat[ LN_POS3 ]	= (UINT_32)(*pPosY * 10) | ((UINT_32)(*pPosX *10) << 16); pPosX++; pPosY++;		// Position 3
			UlBufDat[ LN_POS4 ]	= (UINT_32)(*pPosY * 10) | ((UINT_32)(*pPosX *10) << 16); pPosX++; pPosY++;		// Position 4
			UlBufDat[ LN_POS5 ]	= (UINT_32)(*pPosY * 10) | ((UINT_32)(*pPosX *10) << 16); pPosX++; pPosY++;		// Position 5
			UlBufDat[ LN_POS6 ]	= (UINT_32)(*pPosY * 10) | ((UINT_32)(*pPosX *10) << 16); pPosX++; pPosY++;		// Position 6
			UlBufDat[ LN_POS7 ]	= (UINT_32)(*pPosY * 10) | ((UINT_32)(*pPosX *10) << 16); pPosX++; pPosY++;		// Position 7
#endif
			UlBufDat[ LN_STEP ]	= ((linval->dacY[1] - linval->dacY[0]) >> 16) | ((linval->dacX[1] - linval->dacX[0]) & 0xFFFF0000);					// Step

			TRACE("STEP=%d (0x%x)\n", (int)UlBufDat[ LN_STEP ], UlBufDat[ LN_STEP ] );
		}else{
			UlBufDat[ MIXING_HXSX ]  = (UINT_32)0xFFFFFFFF;
			UlBufDat[ MIXING_HX45X ] = (UINT_32)0xFFFFFFFF;
			UlBufDat[ MIXING_HX45Y ] = (UINT_32)0xFFFFFFFF;
			UlBufDat[ MIXING_HY45Y ] = (UINT_32)0xFFFFFFFF;
			UlBufDat[ MIXING_HY45X ] = (UINT_32)0xFFFFFFFF;
			UlBufDat[ LN_POS1 ]		 = (UINT_32)0xFFFFFFFF;
			UlBufDat[ LN_POS2 ]		 = (UINT_32)0xFFFFFFFF;
			UlBufDat[ LN_POS3 ]		 = (UINT_32)0xFFFFFFFF;
			UlBufDat[ LN_POS4 ]		 = (UINT_32)0xFFFFFFFF;
			UlBufDat[ LN_POS5 ]		 = (UINT_32)0xFFFFFFFF;
			UlBufDat[ LN_POS6 ]		 = (UINT_32)0xFFFFFFFF;
			UlBufDat[ LN_POS7 ]		 = (UINT_32)0xFFFFFFFF;
			UlBufDat[ LN_STEP ]		 = (UINT_32)0xFFFFFFFF;
		}
//------------------------------------------------------------------------------------------------
// Write gyro calibration data
//------------------------------------------------------------------------------------------------
		WriteCalDataF40( UlBufDat, &UiChkSum1 );							// NVR2へ調整値をwriteしCheckSumを計算
//------------------------------------------------------------------------------------------------
// Calculate calibration data checksum
//------------------------------------------------------------------------------------------------
		ReadCalDataF40( UlBufDat, &UiChkSum2 );								// NVR2へwriteされている調整値のCheckSumを計算

		if(UiChkSum1 != UiChkSum2 ){
			TRACE("CheckSum error\n");
			TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
			ans = 0x10;														// CheckSumエラー
		}
		TRACE("CheckSum OK\n");
		TRACE("UiChkSum1 = %08X, UiChkSum2 = %08X\n",(UINT_32)UiChkSum1, (UINT_32)UiChkSum2 );
	}
//------------------------------------------------------------------------------------------------
// Resume
//------------------------------------------------------------------------------------------------
	if( !UlSrvStat ) {
		RtnCen( BOTH_OFF ) ;
	} else if( UlSrvStat == 3 ) {
		RtnCen( BOTH_ON ) ;
	} else {
		RtnCen( UlSrvStat ) ;
	}

	if( UlOisStat != 0)               OisEna() ;

	return( ans );															// CheckSum OK
}

//********************************************************************************
// Function Name 	: OscStb
// Retun Value		: NON
// Argment Value	: Command Parameter
// Explanation		: Osc Standby Function
// History			: First edition
//********************************************************************************
void	OscStb( void )
{
	RamWrite32A( CMD_IO_ADR_ACCESS , STBOSCPLL ) ;
	RamWrite32A( CMD_IO_DAT_ACCESS , OSC_STB ) ;
}

#if 0
//********************************************************************************
// Function Name 	: GyrSlf
// Retun Value		: Gyro self test SUCCESS or FAILURE
// Argment Value	: NON
// Explanation		: Gyro self test Function
// History			: First edition 									2016.8.3
//********************************************************************************
UINT_8	GyrSlf( void )
{
	UINT_8		UcFinSts = 0 ;
	float		flGyrRltX ;
	float		flGyrRltY ;
	float		flMeasureAveValueA , flMeasureAveValueB ;
	INT_32		SlMeasureParameterNum ;
	INT_32		SlMeasureParameterA, SlMeasureParameterB ;
	UnllnVal	StMeasValueA , StMeasValueB ;
	UINT_32		UlReadVal;

	// Setup for self test
	RamWrite32A( 0xF01D, 0x75000000 );	// Read who am I
	while( RdStatus( 0 ) ) ;

	RamRead32A ( 0xF01D, &UlReadVal );

	if( (UlReadVal >> 24) != 0x85 )
	{
		TRACE("WHO AM I read error %08X\n", UlReadVal );
		return	(0xFF);
	}

	// Pre test
	RamWrite32A( 0xF01E, 0x1B100000 );	// GYRO_CONFIG FS_SEL=2(175LSB/dps) XG_ST=OFF YG_ST=OFF
	while( RdStatus( 0 ) ) ;

	MesFil( SELFTEST ) ;

	SlMeasureParameterNum	=	20 * 4;									// 20 sample * 4FS ( 40ms )
	SlMeasureParameterA		=	(UINT_32)GYRO_RAM_GX_ADIDAT ;			// Set Measure RAM Address
	SlMeasureParameterB		=	(UINT_32)GYRO_RAM_GY_ADIDAT ;			// Set Measure RAM Address

	ClrMesFil() ;														// Clear Delay Ram
	WitTim( 300 ) ;

	RamWrite32A( StMeasFunc_PMC_UcPhaseMesMode, 0x00000000 ) ;			// Set Phase Measure Mode

	// Start measure
	MeasureStart( SlMeasureParameterNum, SlMeasureParameterA , SlMeasureParameterB ) ;
	MeasureWait() ;														// Wait complete of measurement


	RamRead32A( StMeasFunc_MFA_LLiIntegral1 	, &StMeasValueA.StUllnVal.UlLowVal ) ;	// X axis
	RamRead32A( StMeasFunc_MFA_LLiIntegral1 + 4 , &StMeasValueA.StUllnVal.UlHigVal ) ;

	RamRead32A( StMeasFunc_MFB_LLiIntegral2 	, &StMeasValueB.StUllnVal.UlLowVal ) ;	// Y axis
	RamRead32A( StMeasFunc_MFB_LLiIntegral2 + 4	, &StMeasValueB.StUllnVal.UlHigVal ) ;

	flMeasureAveValueA = (float)((( (INT_64)StMeasValueA.UllnValue >> 16 ) / (float)SlMeasureParameterNum ) ) ;
	flMeasureAveValueB = (float)((( (INT_64)StMeasValueB.UllnValue >> 16 ) / (float)SlMeasureParameterNum ) ) ;

	flGyrRltX = flMeasureAveValueA / 175.0 ;	// sensitivity 175 dps
	flGyrRltY = flMeasureAveValueB / 175.0 ;	// sensitivity 175 dps

	TRACE("SlMeasureParameterNum = %08X\n", (UINT_32)SlMeasureParameterNum);
	TRACE("StMeasValueA.StUllnVal.UlLowVal = %08X\n", (UINT_32)StMeasValueA.StUllnVal.UlLowVal);
	TRACE("StMeasValueA.StUllnVal.UlHigVal = %08X\n", (UINT_32)StMeasValueA.StUllnVal.UlHigVal);
	TRACE("StMeasValueB.StUllnVal.UlLowVal = %08X\n", (UINT_32)StMeasValueB.StUllnVal.UlLowVal);
	TRACE("StMeasValueB.StUllnVal.UlHigVal = %08X\n", (UINT_32)StMeasValueB.StUllnVal.UlHigVal);
	TRACE("flMeasureAveValueA = %f\n", flMeasureAveValueA );
	TRACE("flMeasureAveValueB = %f\n", flMeasureAveValueB );
	TRACE("flGyrRltX = %f dps\n", flGyrRltX );
	TRACE("flGyrRltY = %f dps\n", flGyrRltY );

	if( fabs(flGyrRltX) >= 25 ){
		UcFinSts |= 0x10;
		TRACE( "X self test 175dps NG\n" );
	}

	if( fabs(flGyrRltY) >= 25 ){
		UcFinSts |= 0x01;
		TRACE( "Y self test 175dps NG\n" );
	}

	// Self test main
	RamWrite32A( 0xF01E, 0x1BDB0000 );	// GYRO_CONFIG FS_SEL=3(87.5LSB/dps) XG_ST=ON YG_ST=ON
	while( RdStatus( 0 ) ) ;

	ClrMesFil() ;														// Clear Delay Ram
	WitTim( 300 ) ;

	RamWrite32A( StMeasFunc_PMC_UcPhaseMesMode, 0x00000000 ) ;			// Set Phase Measure Mode

	// Start measure
	MeasureStart( SlMeasureParameterNum, SlMeasureParameterA , SlMeasureParameterB ) ;
	MeasureWait() ;														// Wait complete of measurement


	RamRead32A( StMeasFunc_MFA_LLiIntegral1 	, &StMeasValueA.StUllnVal.UlLowVal ) ;	// X axis
	RamRead32A( StMeasFunc_MFA_LLiIntegral1 + 4 , &StMeasValueA.StUllnVal.UlHigVal ) ;

	RamRead32A( StMeasFunc_MFB_LLiIntegral2 	, &StMeasValueB.StUllnVal.UlLowVal ) ;	// Y axis
	RamRead32A( StMeasFunc_MFB_LLiIntegral2 + 4	, &StMeasValueB.StUllnVal.UlHigVal ) ;

	flMeasureAveValueA = (float)((( (INT_64)StMeasValueA.UllnValue >> 16 ) / (float)SlMeasureParameterNum ) ) ;
	flMeasureAveValueB = (float)((( (INT_64)StMeasValueB.UllnValue >> 16 ) / (float)SlMeasureParameterNum ) ) ;

	flGyrRltX = flMeasureAveValueA / GYRO_SENSITIVITY ;
	flGyrRltY = flMeasureAveValueB / GYRO_SENSITIVITY ;

	TRACE("SlMeasureParameterNum = %08X\n", (UINT_32)SlMeasureParameterNum);
	TRACE("StMeasValueA.StUllnVal.UlLowVal = %08X\n", (UINT_32)StMeasValueA.StUllnVal.UlLowVal);
	TRACE("StMeasValueA.StUllnVal.UlHigVal = %08X\n", (UINT_32)StMeasValueA.StUllnVal.UlHigVal);
	TRACE("StMeasValueB.StUllnVal.UlLowVal = %08X\n", (UINT_32)StMeasValueB.StUllnVal.UlLowVal);
	TRACE("StMeasValueB.StUllnVal.UlHigVal = %08X\n", (UINT_32)StMeasValueB.StUllnVal.UlHigVal);
	TRACE("flMeasureAveValueA = %f\n", flMeasureAveValueA );
	TRACE("flMeasureAveValueB = %f\n", flMeasureAveValueB );
	TRACE("flGyrRltX = %f dps\n", flGyrRltX );
	TRACE("flGyrRltY = %f dps\n", flGyrRltY );


	if( UcFinSts != 0 )	{
		// error 175 dps
		if( fabs(flGyrRltX) >= 60){
			UcFinSts |= 0x20;
			TRACE( "X self test 87.5dps NG\n" );
		}

		if( fabs(flGyrRltY) >= 60){
			UcFinSts |= 0x02;
			TRACE( "Y self test 87.5dps NG\n" );
		}
	} else {
		// normal
		if( fabs(flGyrRltX) < 60){
			UcFinSts |= 0x20;
			TRACE( "X self test 87.5dps NG\n" );
		}

		if( fabs(flGyrRltY) < 60){
			UcFinSts |= 0x02;
			TRACE( "Y self test 87.5dps NG\n" );
		}
	}

	// Set normal mode
	RamWrite32A( 0xF01E, 0x1B180000 );	// GYRO_CONFIG FS_SEL=3(87.5LSB/dps) XG_ST=OFF YG_ST=OFF
	while( RdStatus( 0 ) ) ;


	TRACE("GyrSlf result=%02X\n", UcFinSts );
	return( UcFinSts ) ;
}
#endif

//********************************************************************************
// Function Name 	: GyrWhoAmIRead
// Retun Value		: Gyro Who am I Read
// Argment Value	: NON
// Explanation		: Gyro Who am I Read Function
// History			: First edition 									2016.11.01
//********************************************************************************
UINT_8	GyrWhoAmIRead( void )
{
	UINT_8		UcRtnVal ;
	UINT_32		UlReadVal;

	// Setup for self test
	RamWrite32A( 0xF01D, 0x75000000 );	// Read who am I
	while( RdStatus( 0 ) ) ;

	RamRead32A ( 0xF01D, &UlReadVal );
	
	UcRtnVal = UlReadVal >> 24;
	
TRACE("WHO AM I read %02X\n", UcRtnVal );
	
	return(UcRtnVal);
}

//********************************************************************************
// Function Name 	: GyrWhoAmICheck
// Retun Value		: Gyro Who am I Check
// Argment Value	: NON
// Explanation		: Gyro Who am I Chek Function
// History			: First edition 									2016.11.01
//********************************************************************************
UINT_8	GyrWhoAmICheck( void )
{
	UINT_8		UcReadVal ;
	
	UcReadVal = GyrWhoAmIRead();
	
	if( UcReadVal == 0x20 ){		// ICM-20690
TRACE("WHO AM I read success\n");
		return	(FAILURE);
	}
	else{
TRACE("WHO AM I read failure\n");
		return	(SUCCESS);
	}
}

//********************************************************************************
// Function Name 	: GyrIdRead
// Retun Value		: Gyro ID Read
// Argment Value	: NON
// Explanation		: Gyro ID Read Function
// History			: First edition 									2016.11.07
//********************************************************************************
UINT_8	GyrIdRead( UINT_8 *UcGyroID )
{
	UINT_8		i ;
	UINT_32		UlReadVal;

	for( i=0; i<7 ; i++ ){
		
		// bank_sel
		RamWrite32A( 0xF01E, 0x6D000000 );
		while( RdStatus( 0 ) ) ;
		
		// start_addr
		RamWrite32A( 0xF01E, 0x6E000000 | (i << 16) );
		while( RdStatus( 0 ) ) ;
		
		// mem_r_w
		RamWrite32A( 0xF01D, 0x6F000000 );
		while( RdStatus( 0 ) ) ;
		
		// ID0[7:0] / ID1[7:0] ... ID6[7:0]
		RamRead32A ( 0xF01D, &UlReadVal );
		UcGyroID[i] = UlReadVal >> 24;
	}
	
TRACE("UcGyroID %02X %02X %02X %02X %02X %02X %02X \n", UcGyroID[0], UcGyroID[1], UcGyroID[2], UcGyroID[3], UcGyroID[4], UcGyroID[5], UcGyroID[6] );
	
	return(SUCCESS);
}

#ifdef	SEL_CLOSED_AF
//********************************************************************************
// Function Name 	: AFHallAmp
// Retun Value		: Mesure AF Amp
// Argment Value	:
// Explanation		:
// History			: First edition
//********************************************************************************
UINT_32 AFHallAmp( void )
{
	INT_32		SlMeasureParameterA , SlMeasureParameterB ;
	INT_32		SlMeasureParameterNum ;
	UINT_32 		UlDatVal;
	UnllnVal	StMeasValueA ;

	SlMeasureParameterNum = 1024;													// Mesurement count set

	RtnCen( BOTH_OFF ) ;													// Both OFF

	SlMeasureParameterA		=	CLAF_RAMA_AFADIN ;									// Set Measure RAM Address
	SlMeasureParameterB		=	CLAF_RAMA_AFADIN ;									// Set Measure RAM Address

	RamWrite32A( 0x03FC ,	0x7FFFFFFF) ;											// Set 7FFFFFFF Gain
	WitTim(500);
	MeasureStart( SlMeasureParameterNum , SlMeasureParameterA , SlMeasureParameterB ) ;		// Start measure
	MeasureWait() ;						// Wait complete of measurement

	RamRead32A( StMeasFunc_MFA_LLiIntegral1, &StMeasValueA.StUllnVal.UlLowVal ) ;		// Max value
	RamRead32A( StMeasFunc_MFA_LLiIntegral1+4, &StMeasValueA.StUllnVal.UlHigVal) ;		// Min value

	 UlDatVal = (UINT_32)( (INT_64)StMeasValueA.UllnValue / SlMeasureParameterNum ) ;

	RamWrite32A( MESHGH,UlDatVal) ;													// Temp25 Parameter
	SlMeasureParameterNum = 1024;														// Mesurement count set

	RamWrite32A( 0x03FC ,	0x80000000) ;												// Set 8000 Gain
	WitTim(500);
	MeasureStart( SlMeasureParameterNum , SlMeasureParameterA , SlMeasureParameterB ) ;	// Start measure
	MeasureWait() ;						// Wait complete of measurement

	RamRead32A( StMeasFunc_MFA_LLiIntegral1, &StMeasValueA.StUllnVal.UlLowVal ) ;		// Max value
	RamRead32A( StMeasFunc_MFA_LLiIntegral1+4, &StMeasValueA.StUllnVal.UlHigVal) ;		// Min value

	UlDatVal = (UINT_32)( (INT_64)StMeasValueA.UllnValue / SlMeasureParameterNum ) ;

	RamWrite32A( MESLOW	,UlDatVal) ;													// Temp25 Parameter
	RamWrite32A( 0x03FC ,	0x0) ;														// Set Sine Wave Gain

	return(( UINT_32 )0x0202);
}
#endif

#if 0
//********************************************************************************
// Function Name 	: AFTmp25
// Retun Value		: Mesure AF Amp
// Argment Value	:
// Explanation		:
// History			: First edition
//********************************************************************************
UINT_32 AFTmp25(void)
{
	INT_32		SlMeasureParameterA , SlMeasureParameterB ;
	INT_32		SlMeasureParameterNum ;
	UINT_32 		UlDatVal;
	UnllnVal	StMeasValueA ;

	SlMeasureParameterNum = 1024;													// Mesurement count set
	RtnCen( BOTH_OFF ) ;															// Both OFF
	SlMeasureParameterA		=	0x0620;												// Set Measure RAM Address ClosedAF_TempInfo25
	SlMeasureParameterB		=	0x0620;												// Set Measure RAM Address ClosedAF_TempInfo25

	MeasureStart( SlMeasureParameterNum , SlMeasureParameterA , SlMeasureParameterB ) ;		// Start measure
	MeasureWait() ;						// Wait complete of measurement

	RamRead32A( StMeasFunc_MFA_LLiIntegral1, &StMeasValueA.StUllnVal.UlLowVal ) ;		// Max value
	RamRead32A( StMeasFunc_MFA_LLiIntegral1+4, &StMeasValueA.StUllnVal.UlHigVal) ;	// Min value

	 UlDatVal = (UINT_32)( (INT_64)StMeasValueA.UllnValue / SlMeasureParameterNum ) ;


	RamWrite32A( 0x0624		,UlDatVal) ;										// Temp25 Parameter



	return(UlDatVal);
}

//********************************************************************************
// Function Name 	: AFMesUpDn
// Retun Value		: Mesure AF Amp
// Argment Value	: Command Parameter
// Explanation		: Osc Standby Function
// History			: First edition
//********************************************************************************
void AFMesUpDn(void)
{
	INT_32		SlMeasureParameterA , SlMeasureParameterB ;
	INT_32		SlMeasureParameterNum ;
	UINT_32 		UlDatVal;
	UnllnVal	StMeasValueA ;

	SlMeasureParameterNum = 1024;														// Mesurement count set

	RamRead32A( 0x03C0		,	( UINT_32 * )&SlMeasureParameterA ) ;					// Servo OFF
	RamWrite32A( 0x03C0		,	SlMeasureParameterA & 0xFE) ;							// Servo OFF

	SlMeasureParameterA		=	0x0620;													// Set Measure RAM Address ClosedAF_TempInfo25
	SlMeasureParameterB		=	0x0620;													// Set Measure RAM Address ClosedAF_TempInfo25

	RamWrite32A( 0x03FC ,	0x7FFFFFFF) ;												// Set Sine Wave Gain
	WitTim(500);
	MeasureStart( SlMeasureParameterNum , SlMeasureParameterA , SlMeasureParameterB ) ;	// Start measure
	MeasureWait() ;						// Wait complete of measurement

	RamRead32A( StMeasFunc_MFA_LLiIntegral1, &StMeasValueA.StUllnVal.UlLowVal ) ;		// Max value
	RamRead32A( StMeasFunc_MFA_LLiIntegral1+4, &StMeasValueA.StUllnVal.UlHigVal) ;		// Min value

	 UlDatVal = (UINT_32)( (INT_64)StMeasValueA.UllnValue / SlMeasureParameterNum ) ;

	RamWrite32A( MESHGH,UlDatVal) ;														// Temp25 Parameter
	SlMeasureParameterNum = 1024;														// Mesurement count set

	RamWrite32A( 0x03FC ,	0x80000000) ;												// Set Sine Wave Gain
	WitTim(500);
	MeasureStart( SlMeasureParameterNum , SlMeasureParameterA , SlMeasureParameterB ) ;	// Start measure
	MeasureWait() ;						// Wait complete of measurement

	RamRead32A( StMeasFunc_MFA_LLiIntegral1, &StMeasValueA.StUllnVal.UlLowVal ) ;		// Max value
	RamRead32A( StMeasFunc_MFA_LLiIntegral1+4, &StMeasValueA.StUllnVal.UlHigVal) ;		// Min value

	UlDatVal = (UINT_32)( (INT_64)StMeasValueA.UllnValue / SlMeasureParameterNum ) ;

	RamWrite32A( MESLOW	,UlDatVal) ;													// Temp25 Parameter
	RamWrite32A( 0x03FC ,	0x0) ;														// Set Sine Wave Gain


}
#endif

//********************************************************************************
// Function Name 	: GyroReCalib
// Retun Value		: Command Status
// Argment Value	: Offset information data pointer
// Explanation		: Re calibration Command Function
// History			: First edition
//********************************************************************************
UINT_8	GyroReCalib( stReCalib * pReCalib )
{
	UINT_8	UcSndDat ;
	UINT_32	UlRcvDat ;
	UINT_32	UlGofX, UlGofY ;
	UINT_32	UiChkSum ;

//------------------------------------------------------------------------------------------------
// Backup ALL Calibration data
//------------------------------------------------------------------------------------------------
	ReadCalDataF40( UlBufDat, &UiChkSum );

	// HighLevelコマンド
	RamWrite32A( CMD_CALIBRATION , 0x00000000 ) ;

	do {
		UcSndDat = RdStatus(1);
	} while (UcSndDat != 0);

	RamRead32A( CMD_CALIBRATION , &UlRcvDat ) ;
	UcSndDat = (unsigned char)(UlRcvDat >> 24);								// 終了ステータス

	// 戻り値を編集
	if( UlBufDat[ GYRO_FCTRY_OFST_X ] == 0xFFFFFFFF )
		pReCalib->SsFctryOffX = (UlBufDat[ GYRO_OFFSET_X ] >> 16) ;
	else
		pReCalib->SsFctryOffX = (UlBufDat[ GYRO_FCTRY_OFST_X ] >> 16) ;

	if( UlBufDat[ GYRO_FCTRY_OFST_Y ] == 0xFFFFFFFF )
		pReCalib->SsFctryOffY = (UlBufDat[ GYRO_OFFSET_Y ] >> 16) ;
	else
		pReCalib->SsFctryOffY = (UlBufDat[ GYRO_FCTRY_OFST_Y ] >> 16) ;

	// キャリブレーション後の値を取得
	RamRead32A(  GYRO_RAM_GXOFFZ , &UlGofX ) ;
	RamRead32A(  GYRO_RAM_GYOFFZ , &UlGofY ) ;

	pReCalib->SsRecalOffX = (UlGofX >> 16) ;
	pReCalib->SsRecalOffY = (UlGofY >> 16) ;
	pReCalib->SsDiffX = abs( pReCalib->SsFctryOffX - pReCalib->SsRecalOffX) ;
	pReCalib->SsDiffY = abs( pReCalib->SsFctryOffY - pReCalib->SsRecalOffY) ;

TRACE("GyroReCalib() = %02x\n", UcSndDat ) ;
TRACE("Factory X = %04X, Y = %04X\n", pReCalib->SsFctryOffX, pReCalib->SsFctryOffY );
TRACE("Recalib X = %04X, Y = %04X\n", pReCalib->SsRecalOffX, pReCalib->SsRecalOffY );
TRACE("Diff    X = %04X, Y = %04X\n", pReCalib->SsDiffX, pReCalib->SsDiffY );
TRACE("UlBufDat[19] = %08X, [20] = %08X\n", UlBufDat[19], UlBufDat[20] );
TRACE("UlBufDat[49] = %08X, [50] = %08X\n", UlBufDat[49], UlBufDat[50] );

	return( UcSndDat );
}

#if 0
//********************************************************************************
// Function Name 	: GetDirection
// Retun Value		: Direction and driver port
// Argment Value	: NONE
// Explanation		: Return to direction and driver port Function
// History			: First edition
//********************************************************************************
void GetDir( UINT_8 *outX, UINT_8 *outY )
{
	UINT_32	UlReadVal;

	RamWrite32A( CMD_IO_ADR_ACCESS, DRVCH1SEL ) ;
	RamRead32A ( CMD_IO_DAT_ACCESS, &UlReadVal ) ;
	*outX = (UINT_8)(UlReadVal & 0xFF);

	RamWrite32A( CMD_IO_ADR_ACCESS, DRVCH2SEL ) ;
	RamRead32A ( CMD_IO_DAT_ACCESS, &UlReadVal ) ;
	*outY = (UINT_8)(UlReadVal & 0xFF);
}
#endif

//********************************************************************************
// Function Name 	: ReadCalibID
// Retun Value		: Calibraion ID
// Argment Value	: NONE
// Explanation		: Read calibraion ID Function
// History			: First edition
//********************************************************************************
UINT_32	ReadCalibID( void )
{
	UINT_32	UlCalibId;

	// Read calibration data
	RamRead32A( SiCalID, &UlCalibId );

	return( UlCalibId );
}


//********************************************************************************
// Function Name 	: FrqDet
// Retun Value		: 0:PASS, 1:OIS X NG, 2:OIS Y NG, 4:CLAF NG
// Argment Value	: NON
// Explanation		: Module Check
// History			: First edition
//********************************************************************************
UINT_8 FrqDet( void )
{
	INT_32 SlMeasureParameterA , SlMeasureParameterB ;
	INT_32 SlMeasureParameterNum ;
	UINT_32 UlXasP_P , UlYasP_P ;
#ifdef	SEL_CLOSED_AF
	UINT_32 UlAasP_P ;
#endif	// SEL_CLOSED_AF

	UINT_8 UcRtnVal;

	UcRtnVal = 0;

	//Measurement Setup
	MesFil( OSCCHK ) ;													// Set Measure Filter

	// waiting for stable the actuator
	WitTim( 300 ) ;

	SlMeasureParameterNum	=	1000 ;									// 1000times( 50ms )

	SlMeasureParameterA		=	(UINT_32)HALL_RAM_HXIDAT ;				// Set Measure RAM Address
	SlMeasureParameterB		=	(UINT_32)HALL_RAM_HYIDAT ;				// Set Measure RAM Address

	// Start measure
	MeasureStart2( SlMeasureParameterNum , SlMeasureParameterA , SlMeasureParameterB, 50 ) ;
	MeasureWait() ;														// Wait complete of measurement
	RamRead32A( StMeasFunc_MFA_UiAmp1, &UlXasP_P ) ;					// X Axis Peak to Peak
	RamRead32A( StMeasFunc_MFB_UiAmp2, &UlYasP_P ) ;					// Y Axis Peak to Peak
TRACE("UlXasP_P = 0x%08X\r\n", (UINT_32)UlXasP_P ) ;
TRACE("UlYasP_P = 0x%08X\r\n", (UINT_32)UlYasP_P ) ;

	// Amplitude value check X
	if(  UlXasP_P > ULTHDVAL ){
		UcRtnVal = 1;
	}
	// Amplitude value check Y
	if(  UlYasP_P > ULTHDVAL ){
		UcRtnVal |= 2;
	}

#ifdef	SEL_CLOSED_AF
	// CLAF
	SlMeasureParameterA		=	(UINT_32)CLAF_RAMA_AFDEV ;		// Set Measure RAM Address
	SlMeasureParameterB		=	(UINT_32)CLAF_RAMA_AFDEV ;		// Set Measure RAM Address

	// impulse Set
//	RamWrite32A( CLAF_RAMA_AFTARGET , STEP1 ) ;							// CLAF manual

//	RamWrite32A( CLAF_RAMA_AFTARGET , STEP2 ) ;							// CLAF manual

	// waiting for stable the actuator
	WitTim( 300 ) ;

	// Start measure
	MeasureStart2( SlMeasureParameterNum , SlMeasureParameterA , SlMeasureParameterB, 50 ) ;
	MeasureWait() ;														// Wait complete of measurement
	RamRead32A( StMeasFunc_MFA_UiAmp1, &UlAasP_P ) ;					// CLAF Axis Peak to Peak
TRACE("UlAasP_P = %X\r\n", (int)UlAasP_P ) ;

	// Amplitude value check CLAF
	if(  UlAasP_P > ULTHDVAL ){
		UcRtnVal |= 4;
	}

#endif	// SEL_CLOSED_AF

	return(UcRtnVal);													// Retun Status value
}

//********************************************************************************
// Function Name 	: MesRam
// Retun Value		: NON
// Argment Value	: NON
// Explanation		: Measure 
// History			: First edition 						2015.07.06 
//********************************************************************************
UINT_8	 MesRam( INT_32 SlMeasureParameterA, INT_32 SlMeasureParameterB, INT_32 SlMeasureParameterNum, stMesRam* pStMesRamA, stMesRam* pStMesRamB )
{
	UnllnVal	StMeasValueA , StMeasValueB ;
	
	MesFil( THROUGH ) ;							// Set Measure Filter
	
	MeasureStart2( SlMeasureParameterNum,  SlMeasureParameterA, SlMeasureParameterB, 1 ) ;		// Start measure
	
	MeasureWait() ;								// Wait complete of measurement
	
	// A : X axis
	RamRead32A( StMeasFunc_MFA_SiMax1 , &(pStMesRamA->SlMeasureMaxValue) ) ;			// Max value
	RamRead32A( StMeasFunc_MFA_SiMin1 , &(pStMesRamA->SlMeasureMinValue) ) ;			// Min value
	RamRead32A( StMeasFunc_MFA_UiAmp1 , &(pStMesRamA->SlMeasureAmpValue) ) ;			// Amp value
	RamRead32A( StMeasFunc_MFA_LLiIntegral1,	 &(StMeasValueA.StUllnVal.UlLowVal) ) ;	// Integration Low
	RamRead32A( StMeasFunc_MFA_LLiIntegral1 + 4, &(StMeasValueA.StUllnVal.UlHigVal) ) ;	// Integration Hig
	pStMesRamA->SlMeasureAveValue = 
				(INT_32)( (INT_64)StMeasValueA.UllnValue / SlMeasureParameterNum ) ;	// Ave value
	
	// B : Y axis
	RamRead32A( StMeasFunc_MFB_SiMax2 , &(pStMesRamB->SlMeasureMaxValue) ) ;			// Max value
	RamRead32A( StMeasFunc_MFB_SiMin2 , &(pStMesRamB->SlMeasureMinValue) ) ;			// Min value
	RamRead32A( StMeasFunc_MFB_UiAmp2 , &(pStMesRamB->SlMeasureAmpValue) ) ;			// Amp value
	RamRead32A( StMeasFunc_MFB_LLiIntegral2,	 &(StMeasValueB.StUllnVal.UlLowVal) ) ;	// Integration Low
	RamRead32A( StMeasFunc_MFB_LLiIntegral2 + 4, &(StMeasValueB.StUllnVal.UlHigVal) ) ;	// Integration Hig
	pStMesRamB->SlMeasureAveValue = 
				(INT_32)( (INT_64)StMeasValueB.UllnValue / SlMeasureParameterNum ) ;	// Ave value
	
	return( 0 );
}

//********************************************************************************
// Function Name 	: MeasGain
// Retun Value		: Hall amp & Sine amp
// Argment Value	: X,Y Direction, Freq
// Explanation		: Measuring Hall Paek To Peak
// History			: First edition 						
//********************************************************************************
#define	FS4TIME	(UINT_32)0x000138CC		// 20019 * 4
#define	FRQOFST	(UINT_32)0x0001A308		// 80000000h / 20019

UINT_32	MeasGain ( UINT_16	UcDirSel, UINT_16	UsMeasFreq , UINT_32 UlMesAmp )
{
	INT_32			SlMeasureParameterA , SlMeasureParameterB ;
	INT_32			SlMeasureParameterNum , SlSineWaveOffset;
	UnllnVal		StMeasValueA  , StMeasValueB ;
	UINT_32	UlReturnVal;

	StMeasValueA.UllnValue = 0;
	StMeasValueB.UllnValue = 0;
	SlMeasureParameterNum	=	(INT_32)( FS4TIME / (UINT_32)UsMeasFreq) * 2;	// 
	
	if( UcDirSel == X_DIR ) {								// X axis
		SlMeasureParameterA		=	HALL_RAM_HXOUT0 ;		// Set Measure RAM Address
		SlMeasureParameterB		=	HallFilterD_HXDAZ1 ;	// Set Measure RAM Address
	} else if( UcDirSel == Y_DIR ) {						// Y axis
		SlMeasureParameterA		=	HALL_RAM_HYOUT0 ;		// Set Measure RAM Address
		SlMeasureParameterB		=	HallFilterD_HYDAZ1 ;	// Set Measure RAM Address
	}
	SetSinWavGenInt();
	
	SlSineWaveOffset = (INT_32)( FRQOFST * (UINT_32)UsMeasFreq );
	RamWrite32A( SinWave_Offset		,	SlSineWaveOffset ) ;		// Freq Setting = Freq * 80000000h / Fs	

	RamWrite32A( SinWave_Gain		,	UlMesAmp ) ;			// Set Sine Wave Gain

	RamWrite32A( SinWaveC_Regsiter	,	0x00000001 ) ;				// Sine Wave Start
	if( UcDirSel == X_DIR ) {
		SetTransDataAdr( SinWave_OutAddr	,	(UINT_32)HALL_RAM_HXOFF1 ) ;	// Set Sine Wave Input RAM
	}else if( UcDirSel == Y_DIR ){
		SetTransDataAdr( SinWave_OutAddr	,	(UINT_32)HALL_RAM_HYOFF1 ) ;	// Set Sine Wave Input RAM
	}
	
	MesFil2( UsMeasFreq ) ;					// Filter setting for measurement

	MeasureStart2( SlMeasureParameterNum , SlMeasureParameterA , SlMeasureParameterB , 8000/UsMeasFreq ) ;			// Start measure
	
//#if !LPF_ENA
	WitTim( 8000 / UsMeasFreq ) ;
//#endif
	MeasureWait() ;						// Wait complete of measurement
	
	RamWrite32A( SinWaveC_Regsiter	,	0x00000000 ) ;								// Sine Wave Stop
	
	if( UcDirSel == X_DIR ) {
		SetTransDataAdr( SinWave_OutAddr	,	(UINT_32)0x00000000 ) ;	// Set Sine Wave Input RAM
		RamWrite32A( HALL_RAM_HXOFF1		,	0x00000000 ) ;				// DelayRam Clear
	}else if( UcDirSel == Y_DIR ){
		SetTransDataAdr( SinWave_OutAddr	,	(UINT_32)0x00000000 ) ;	// Set Sine Wave Input RAM
		RamWrite32A( HALL_RAM_HYOFF1		,	0x00000000 ) ;				// DelayRam Clear
	}

	RamRead32A( StMeasFunc_MFA_LLiAbsInteg1 		, &StMeasValueA.StUllnVal.UlLowVal ) ;	
	RamRead32A( StMeasFunc_MFA_LLiAbsInteg1 + 4 	, &StMeasValueA.StUllnVal.UlHigVal ) ;
	RamRead32A( StMeasFunc_MFB_LLiAbsInteg2 		, &StMeasValueB.StUllnVal.UlLowVal ) ;	
	RamRead32A( StMeasFunc_MFB_LLiAbsInteg2 + 4		, &StMeasValueB.StUllnVal.UlHigVal ) ;

	
	UlReturnVal = (INT_32)((INT_64)StMeasValueA.UllnValue * 100 / (INT_64)StMeasValueB.UllnValue  ) ;


	return( UlReturnVal ) ;
}
//********************************************************************************
// Function Name 	: MesFil2
// Retun Value		: NON
// Argment Value	: Measure Filter Mode
// Explanation		: Measure Filter Setting Function
// History			: First edition 		
//********************************************************************************
#define	DivOffset	6375.47f		/* 20019/3.14 */

void	MesFil2( UINT_16	UsMesFreq )		
{
	UINT_32	UlMeasFilA1 , UlMeasFilB1 , UlMeasFilC1 , UlTempval ;
	UINT_32	UlMeasFilA2 , UlMeasFilC2 ;
		
	UlTempval = (UINT_32)(2147483647 * (float)UsMesFreq / ((float)UsMesFreq + DivOffset ));
	UlMeasFilA1	=	0x7fffffff - UlTempval;
	UlMeasFilB1	=	~UlMeasFilA1 + 0x00000001;	
	UlMeasFilC1	=	0x7FFFFFFF - ( UlTempval << 1 ) ;

	UlMeasFilA2	=	UlTempval ;	
	UlMeasFilC2	=	UlMeasFilC1 ;

	
	RamWrite32A ( MeasureFilterA_Coeff_a1	, UlMeasFilA1 ) ;
	RamWrite32A ( MeasureFilterA_Coeff_b1	, UlMeasFilB1 ) ;
	RamWrite32A ( MeasureFilterA_Coeff_c1	, UlMeasFilC1 ) ;

	RamWrite32A ( MeasureFilterA_Coeff_a2	, UlMeasFilA2 ) ;
	RamWrite32A ( MeasureFilterA_Coeff_b2	, UlMeasFilA2 ) ;
	RamWrite32A ( MeasureFilterA_Coeff_c2	, UlMeasFilC2 ) ;

	RamWrite32A ( MeasureFilterB_Coeff_a1	, UlMeasFilA1 ) ;
	RamWrite32A ( MeasureFilterB_Coeff_b1	, UlMeasFilB1 ) ;
	RamWrite32A ( MeasureFilterB_Coeff_c1	, UlMeasFilC1 ) ;

	RamWrite32A ( MeasureFilterB_Coeff_a2	, UlMeasFilA2 ) ;
	RamWrite32A ( MeasureFilterB_Coeff_b2	, UlMeasFilA2 ) ;
	RamWrite32A ( MeasureFilterB_Coeff_c2	, UlMeasFilC2 ) ;
}



