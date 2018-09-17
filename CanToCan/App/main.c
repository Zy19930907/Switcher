/* 
 * File:   main.c
 * Author: dengcongyang
 *
 * Created on June 5, 2017, 11:49 PM
 */
#include <xc.h>
#include "CPU.h"
#include "Public.h"
#include "CanData.h"
#include "ProSwitch.h"
#include "Can.h"

// CONFIG1L
#pragma config RETEN = OFF      // VREG Sleep Enable bit (Ultra low-power regulator is Disabled (Controlled by REGSLP bit))
#pragma config INTOSCSEL = HIGH  // LF-INTOSC Low-power Enable bit (LF-INTOSC in Low-power mode during Sleep)
#pragma config SOSCSEL = DIG   // SOSC Power Selection and mode Configuration bits (High Power SOSC circuit selected)
#pragma config XINST = OFF       // Extended Instruction Set (Enabled)

// CONFIG1H
#pragma config FOSC = HS1       // Oscillator (EC oscillator (Medium power, 160 kHz - 16 MHz))
#pragma config PLLCFG = OFF     // PLL x4 Enable bit (Disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor (Disabled)
#pragma config IESO = OFF       // Internal External Oscillator Switch Over Mode (Disabled)

// CONFIG2L
#pragma config PWRTEN = ON     // Power Up Timer (Disabled)
#pragma config BOREN = SBORDIS  // Brown Out Detect (Disabled in hardware, SBOREN disabled)
#pragma config BORV = 3         // Brown-out Reset Voltage bits (1.8V)
#pragma config BORPWR = ZPBORMV // BORMV Power level (ZPBORMV instead of BORMV is selected)

// CONFIG2H
#pragma config WDTEN = ON      // Watchdog Timer (WDT controlled by SWDTEN bit setting)
#pragma config WDTPS = 512     // Watchdog Postscaler (1:256)，大概1s延时  4ms一个周期

// CONFIG3H
#pragma config CANMX = PORTB    // ECAN Mux bit (ECAN TX and RX pins are located on RB2 and RB3, respectively)
#pragma config MSSPMSK = MSK7   // MSSP address masking (7 Bit address masking mode)
#pragma config MCLRE = ON       // Master Clear Enable (MCLR Enabled, RE3 Disabled)

// CONFIG4L
#pragma config STVREN = ON     // Stack Overflow Reset (Disabled)
#pragma config BBSIZ = BB2K     // Boot Block Size (2K word Boot Block size)

// CONFIG5L
#pragma config CP0 = OFF        // Code Protect 00800-01FFF (Disabled)
#pragma config CP1 = OFF        // Code Protect 02000-03FFF (Disabled)
#pragma config CP2 = OFF        // Code Protect 04000-05FFF (Disabled)
#pragma config CP3 = OFF        // Code Protect 06000-07FFF (Disabled)

// CONFIG5H
#pragma config CPB = OFF        // Code Protect Boot (Disabled)
#pragma config CPD = OFF        // Data EE Read Protect (Disabled)

// CONFIG6L
#pragma config WRT0 = OFF       // Table Write Protect 00800-01FFF (Disabled)
#pragma config WRT1 = OFF       // Table Write Protect 02000-03FFF (Disabled)
#pragma config WRT2 = OFF       // Table Write Protect 04000-05FFF (Disabled)
#pragma config WRT3 = OFF       // Table Write Protect 06000-07FFF (Disabled)

// CONFIG6H
#pragma config WRTC = OFF       // Config. Write Protect (Disabled)
#pragma config WRTB = OFF       // Table Write Protect Boot (Disabled)
#pragma config WRTD = OFF       // Data EE Write Protect (Disabled)

// CONFIG7L
#pragma config EBTR0 = OFF      // Table Read Protect 00800-01FFF (Disabled)
#pragma config EBTR1 = OFF      // Table Read Protect 02000-03FFF (Disabled)
#pragma config EBTR2 = OFF      // Table Read Protect 04000-05FFF (Disabled)
#pragma config EBTR3 = OFF      // Table Read Protect 06000-07FFF (Disabled)

// CONFIG7H
#pragma config EBTRB = OFF      // Table Read Protect Boot (Disabled)

#pragma warning disable 752    //必须要加此项，否则会出现数据转换错误
#pragma warning disable 228

#define MaxLevel  10
CRunLevel CurRunLevel;
u16 RunTick[MaxLevel];
u16 LedTime;
u8 TestCnt = 0;
extern vu16 SYS_TICK;
extern _LocalSensor LocalSensors[MaxLocalSensorCnt];
extern CSys Sys;
extern CTime Time;
extern _RemoteSensor RemoteSensors[MaxRemoteSensorCnt];

void FlashLed(u8 times)
{
	u16 tick;
	u8 i;
	for (i = 0; i < times; i++)
	{
		tick = SYS_TICK;
		RunLedOn;
		while (MsTickDiff(tick) < 100);
		RunLedOff;
		tick = SYS_TICK;
		while (MsTickDiff(tick) < 100);
	}
}

void main(void)
{
	CpuInit();
	FlashLed(10);
	ReadAllLocalSensorConfig();
	ReadAllBreaker();
	ReadAllRemoteSensor();
	ReadAddr();
	WDTCON |= 0x01; //开启看门狗，1s   4ms一个周期
	LedTime = 500;
	Sys.Delay = 20;
	Sys.Get3_0 = 0;
	Sys.UploadFlag = 0;
	for (;;)
	{
		switch (CurRunLevel)
		{
			//............LED显示任务 
		case LedRunLevel:
			if (MsTickDiff(RunTick[CurRunLevel]) > LedTime)
			{
				RunTick[CurRunLevel] = SYS_TICK;
				RunLedChange;
				asm("clrwdt");
			}
			CurRunLevel++;
			break;

		case VoltageSample:
			if (MsTickDiff(RunTick[CurRunLevel]) > 3000)
			{
				RunTick[CurRunLevel] = SYS_TICK;
				VolSample();
			}
			CurRunLevel++;
			break;

		case CanUpLevel: //连接主干网络的CAN，收到数据之后，向下转发
			CanUpReceiveFunc();
			CurRunLevel++;
			break;

		case CanDownLevel: //连接传感器网络的CAN，收到数据之后，向上转发
			CanDownReceiveFunc();
			CurRunLevel++;
			break;

		case SyncClkLevel://每6秒发送同步传感器上传周期指令
			if (MsTickDiff(RunTick[CurRunLevel]) >= 6000)
			{
				RunTick[SyncClkLevel] = SYS_TICK;
				SyncClk();
			}
			CurRunLevel++;
			break;

		case OnlineCheckLevel://检查传感器在线状态
			SensorOnlineCheck();
			for (TestCnt = 0; TestCnt < MaxRemoteLinkCnt; TestCnt++)
			{
				if (MsTickDiff(RemoteSensors[TestCnt].Tick) >= 2500)
					RemoteSensors[TestCnt].CtrFlag |= ENOFFLINEDUANDIAN; //置位传感器断线断电标记位
			}

			if (SecTickDiff(LocalSensors[15].Tick) >= LocalSensors[15].OffTimeout)
			{
				LocalSensors[15].CtrFlag |= ENOFFLINEDUANDIAN; //置位传感器断线断电标记位
				LocalSensors[15].SensorFlag |= 0x01;
			}
			CurRunLevel++;
			break;

		case CheckDuandianLevel://断电器检查是否需要断电、复电
			if (Sys.Delay == 0 && Sys.InitDelay == 0)
				DuanDianPro();
			CurRunLevel++;
			break;

		case TimeLevel://模拟时钟
			if (MsTickDiff(RunTick[CurRunLevel]) >= 1000)
			{
				RunTick[CurRunLevel] = SYS_TICK;
				TimePro();
				if (Sys.Delay)
					Sys.Delay--;
				if (Sys.InitDelay)
					Sys.InitDelay--;
				if((!Sys.Delay) && !(Sys.Get3_0))
					Get3_0Config();
			}
			CurRunLevel++;
			break;

		case PowerUploadLevel:
			if (MsTickDiff(RunTick[CurRunLevel]) >= 10000)
			{
				RunTick[CurRunLevel] = SYS_TICK;
				UploadPowerData();
			}
			CurRunLevel++;
			break;
			
		case UploadSwitcherInfo:
			if (MsTickDiff(RunTick[CurRunLevel]) >= 500)
			{
				RunTick[CurRunLevel] = SYS_TICK;
				if(!Sys.Delay)
				{
					if(Sys.UploadFlag)
					{
						Sys.UploadFlag = 0;
						UpLoadControlInfo();
					}
					else
					{
						Sys.UploadFlag = 1;
						UpLoadSensorData();
					}
				}
			}
			CurRunLevel = LedRunLevel;
			break;
			//case TestLevel:
			//if (MsTickDiff(Sys.tTick) >= 60000)
			//{
			//		Sys.tTick = SYS_TICK;
			//		for (TestCnt = 0; TestCnt < 8; TestCnt++)
			//		{
			//			TestCan.Buf[TestCnt] = LocalSensors[TestCnt].RecvCnt;
			//			LocalSensors[TestCnt].RecvCnt = 0;
			//		}
			//			TestCan.Len = 8;
			//			TestCan.ID = MakeFeimoCanId(0, 0x53, NCTR, DIR_UP, 0, 1);
			//			CanUpSend(TestCan);
			//}
			//CurRunLevel = LedRunLevel;
			//break;
		default:
			CurRunLevel = LedRunLevel;
			break;
		}
	}
}

