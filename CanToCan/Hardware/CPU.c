#include <xc.h>
#include "CPU.h"
#include "Public.h"
#include "Can.h"

vu16 SYS_TICK, SYS_TICK_1S;
vu8 Timer2cnt;
u32 timeHex;
CTime Time;
extern CSys Sys;
extern _LocalSensor LocalSensors[MaxLocalSensorCnt];
extern _Breaker Breakers[MaxBreakerCnt];
extern _RemoteSensor RemoteSensors[MaxRemoteSensorCnt];
/*
u8 ReadEEprom(u16 addr)
{
	EEADRH = addr >> 8;
	EEADR = addr;
	EECON1bits.EEPGD = 0;
	EECON1bits.CFGS = 0;
	EECON1bits.RD = 1;
	while (EECON1bits.RD); //等待读周期完成
	return EEDATA;
}

void WriteEEprom(u16 addr, u8 val)
{
	EEADRH = addr >> 8;
	EEADR = addr;
	EEDATA = val;
	EECON1bits.EEPGD = 0;
	EECON1bits.CFGS = 0;
	EECON1bits.WREN = 1;
	GIE = 0; //关闭中断

	EECON2 = 0x55;
	EECON2 = 0xAA;
	EECON1bits.WR = 1;
	while (EECON1bits.WR); //等待写周期完成
	GIE = 1;
	EECON1bits.WREN = 0;
	asm("clrwdt");
}

void WriteBurstEEPROM(u16 Startaddr, u8 *buf, u16 len)
{
	if (len == 0)
		return;
	do
	{
		WriteEEprom(Startaddr++, *(buf++));
	} while (--len);
}
*/

void WriteBurstEEprom(u16 addr,u8 *buf,u16 len)
{
     u16 i;
     EECON1bits.EEPGD = 0;
     EECON1bits.CFGS  = 0;
     EECON1bits.WREN = 1;
     GIE = 0;           //关闭中断
     for(i = 0;i < len;i ++)
     {
          EEADRH = addr >> 8;
          EEADR  = addr;
          EEDATA = buf[i];
          EECON2 = 0x55;
          EECON2 = 0xAA;
          EECON1bits.WR = 1;
          while(EECON1bits.WR);   //等待写周期完成
          Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();
          addr ++;
     }
     GIE = 1;
     EECON1bits.WREN = 0;
}

void ReadBurstEEprom(u16 addr,u8 *buf,u16 len)
{
     u16 i;
     EECON1bits.EEPGD = 0;
     EECON1bits.CFGS = 0;
     GIE = 0;
     for(i = 0;i < len;i ++)
     {
          EEADRH = addr >> 8;
          EEADR  = addr;
          EECON1bits.RD = 1;
          while(EECON1bits.RD);   //等待读周期完成
          Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();
          buf[i] = EEDATA;
          EECON1bits.RD = 0;
          addr ++;
     }
     GIE = 1;
}

// RB5---LED   RB4---信号输入  RA0 RA1 RA2 RA3 RA5 RC0 RC1 地址选通

void IoInit(void)
{
	TRISB &= ~0x20;
	TRISB |= 0x10;
	TRISA |= 0x2F;
	TRISC |= 0x03;
}
//D0...D6 对应 RA0 RA1 RA2 RA3 RA5 RC0 RC1

void ReadAddr(void)
{
	u8 AddrOffsets[8] = {0, 16, 32, 48, 64, 80, 96, 112};
	u8 t;
	t = PORTA;
	t &= 0x07;
	Sys.Addr = t;
	Sys.AddrOffset = *(AddrOffsets + t);
}

void ClockInit(void)
{
	OSCCON = 0x60;
	ANCON0 = 0;
	ANCON1 = 0;
}

void CpuInit(void)
{
	ClockInit();
	IoInit();
	Timer0Init();
	Timer2Init();
	AdInit();
	GIE = 1;
	PEIE = 1; //外设中断
	CanUpInit();
	CanDownInit();
}

void Timer0Init(void)
{
	T0CON = 0xC4; //内部外部时钟频率：16/4 M = 4M,并且32分频--->等价于 125K频率
	TMR0IE = 1;
	TMR0IF = 0;
	TMR0 = 128; //1 ms定时。TMR0递增模式到255溢出。
}

void Timer2Init(void) // 产生16ms-----》产生1s
{
	T2CON = 0x7F; // Fosc/4 时钟，前后各16分频
	TMR2IE = 1;
	TMR2IF = 0;
	PR2 = 124; // 15625 = 125 X 125
}
// RB0---AN10 对应的采样AD。
// 对应的AD采样得到电压之后，乘以11，得到总电压

void AdInit(void)
{
	TRISB |= 0x01;
	ANCON1 |= 0x01; // RB0 对应的模拟接口是 ANSEL8
	ADCON1 = 0x20; //内部参考电压2.048V
	ADCON2 = 0xF3; //右对齐，内部AD时钟源
	ADCON0 |= 0x01; //使能AD模块
}

u16 GetVolInputValue(u8 ch)
{
	u16 Vol;
	ADCON0bits.CHS = ch; //12 通道
	ADCON0bits.GO = 1;
	while (ADCON0bits.GO);
	Vol = ADRESH;
	Vol <<= 8;
	Vol += ADRESL;
	return Vol;
}
// 电压采集

void VolSample(void)
{
	u16 temp;
	temp = GetVolInputValue(VolInputSample);
	temp >>= 1; // 12bit，4096---》2048mv，每一刻度代表0.5mv
	temp *= 11;
	temp /= 100;
	Sys.Vol = temp;
}

static void interrupt SystemISR(void)
{
	if (TMR0IF) //定时器0中断，系统1ms时钟
	{
		TMR0IF = 0;
		TMR0 = 128;
		SYS_TICK++;
	}
	if (TMR2IF) //定时器2中断，8ms中断一次
	{
		TMR2IF = 0;
		Timer2cnt++;
		if (Timer2cnt == 125) // 1s 精确定时到
		{
			Timer2cnt = 0;
			SYS_TICK_1S++;
		}
	}
}

/*
 * 传感器初始化信息存储，每个传感器初始化信息占用13个字节 CRC(初始化) 主动上报时间 上断 上复 下断 下复  CRC(存储信息)
 */
void WriteLocalSenserConfig(_LocalSensor Sensor)
{
	u8 ConfigBuf[LocalSensorInfoCnt], i = 0;
	u16 crc;
	ConfigBuf[i++] = Sensor.Crc;
	ConfigBuf[i++] = Sensor.SensorFlag;
	ConfigBuf[i++] = Sensor.UpDuanDian;
	ConfigBuf[i++] = Sensor.UpDuanDian >> 8;
	ConfigBuf[i++] = Sensor.UpFuDian;
	ConfigBuf[i++] = Sensor.UpFuDian >> 8;
	ConfigBuf[i++] = Sensor.DownDuanDian;
	ConfigBuf[i++] = Sensor.DownDuanDian >> 8;
	ConfigBuf[i++] = Sensor.DownFuDian;
	ConfigBuf[i++] = Sensor.DownFuDian >> 8;
	crc = CalCrc16(ConfigBuf, i);
	ConfigBuf[i++] = crc;
	ConfigBuf[i++] = crc >> 8;
//	WriteBurstEEPROM((Sensor.Addr - Sys.AddrOffset - 1) * LocalSensorInfoCnt, ConfigBuf, LocalSensorInfoCnt);
     WriteBurstEEprom((Sensor.Addr - Sys.AddrOffset - 1) * LocalSensorInfoCnt, ConfigBuf, LocalSensorInfoCnt);
}

void ReadLocalSenserConfig(u8 Index)
{
	u8 ConfigBuf[LocalSensorInfoCnt], i = 0;
	u16 StartAddr;

	LocalSensors[Index].Addr = Index + 1 + Sys.AddrOffset;
	StartAddr = Index * LocalSensorInfoCnt;
     /*
	for (i = 0; i < LocalSensorInfoCnt; i++)
	{
		*(ConfigBuf + i) = ReadEEprom(StartAddr + i);
	}*/
     ReadBurstEEprom(StartAddr,ConfigBuf,LocalSensorInfoCnt);
	if (CRC16Check(ConfigBuf, LocalSensorInfoCnt))//EEPROM存在传感器记录
	{
		i = 0;
		LocalSensors[Index].Crc = ConfigBuf[i++];
		LocalSensors[Index].SensorFlag = ConfigBuf[i++];
		i += 2;
		LocalSensors[Index].UpDuanDian = ConfigBuf[i + 1];
		LocalSensors[Index].UpDuanDian <<= 8;
		LocalSensors[Index].UpDuanDian += ConfigBuf[i];
		i += 2;
		LocalSensors[Index].UpFuDian = ConfigBuf[i + 1];
		LocalSensors[Index].UpFuDian <<= 8;
		LocalSensors[Index].UpFuDian += ConfigBuf[i];
		i += 2;
		LocalSensors[Index].DownDuanDian = ConfigBuf[i + 1];
		LocalSensors[Index].DownDuanDian <<= 8;
		LocalSensors[Index].DownDuanDian += ConfigBuf[i];
		i += 2;
		LocalSensors[Index].DownFuDian = ConfigBuf[i + 1];
		LocalSensors[Index].DownFuDian <<= 8;
		LocalSensors[Index].DownFuDian += ConfigBuf[i];
		
		LocalSensors[Index].OffTimeout = 2350;
	} else//EEPROM中没有传感器记录
	{
		LocalSensors[Index].Name = 0xFF;
		LocalSensors[Index].OffTimeout = 20000;
		LocalSensors[Index].UpDuanDian = 0xFFFF;
		LocalSensors[Index].UpFuDian = 0xFFFF;
		LocalSensors[Index].DownDuanDian = 0xFFFF;
		LocalSensors[Index].DownFuDian = 0xFFFF;
		LocalSensors[Index].Crc = 0x00;
		if (Index == 15)
			LocalSensors[Index].OffTimeout = 180;
	}
}

void EraseLocalSenser(u8 Addr)
{
	u8 buf[LocalSensorInfoCnt],i;
	_LocalSensor* LocalSensor = &LocalSensors[Addr - 1];
	LocalSensor->Addr = 0;
	LocalSensor->Crc = 0;
	LocalSensor->SensorFlag = 0x00;
	LocalSensor->UpDuanDian = 0xFFFF;
	LocalSensor->UpFuDian = 0xFFFF;
	LocalSensor->UpWarn = 0xFFFF;
	LocalSensor->DownDuanDian = 0xFFFF;
	LocalSensor->DownFuDian = 0xFFFF;
	LocalSensor->DownWarn = 0xFFFF;
	LocalSensor->OffTimeout = 20000;
     for(i = 0;i < LocalSensorInfoCnt;i ++)
          buf[i] = 0xFF;
     /*
	for (i = 0; i < LocalSensorInfoCnt; i++)
		WriteEEprom((Addr - 1) * LocalSensorInfoCnt + i, 0xFF);
      * */
     WriteBurstEEprom((Addr - 1) * LocalSensorInfoCnt,buf,LocalSensorInfoCnt);
}
//一个转换板最多16个设备，上电时读取16个设备初始化信息

void ReadAllLocalSensorConfig(void)
{
	u8 i;
	for (i = 0; i < MaxLocalSensorCnt; i++)
	{
		ReadLocalSenserConfig(i);
	}
}

void TimeChange(void)
{
	timeHex = Time.Buf[0]; //年
	timeHex <<= 4;
	timeHex += Time.Buf[1]; //月
	timeHex <<= 5;
	timeHex += Time.Buf[2]; //日
	timeHex <<= 5;
	timeHex += Time.Buf[4]; //小时
	timeHex <<= 6;
	timeHex += Time.Buf[5]; //分
	timeHex <<= 6;
	timeHex += Time.Buf[6]; //秒
}

void TimePro(void)
{
	Time.Buf[6]++;
	if (Time.Buf[6] >= 60)
	{
		Time.Buf[6] = 0;
		Time.Buf[5]++;
		if (Time.Buf[5] >= 60)
		{
			Time.Buf[5] = 0;
			Time.Buf[4]++;
			if (Time.Buf[4] >= 24)
				Time.Buf[4] = 0;
		}
	}
}

void WriteBreaker(_Breaker Breaker)
{
	u8 ConfigBuf[BreakerInfoCnt]={0}, i = 0, j, BreakerIndex;
	u16 StartAddr, crc;
	BreakerIndex = GetBreakerIndex(Breaker.Addr);
	if (BreakerIndex == 0xFF)
		return;
	StartAddr = (BreakerInfoStartAddr + (BreakerIndex * BreakerInfoCnt));
	ConfigBuf[i++] = Breaker.Addr;
	ConfigBuf[i++] = Breaker.Crc;
	ConfigBuf[i++] = Breaker.ForceControlFlag;
	ConfigBuf[i++] = Breaker.ForceControlPort;
	ConfigBuf[i++] = Breaker.CrossControlFlag;
	ConfigBuf[i++] = Breaker.CrossControlPort;
	for (j = 0; j < MaxLocalLinkCnt; j++)
	{
		ConfigBuf[i++] = Breaker.LocalTriggerAddrs[j];
		ConfigBuf[i++] = Breaker.LocalTriggers[j];
	}

	for (j = 0; j < MaxRemoteLinkCnt; j++)
	{
		ConfigBuf[i++] = Breaker.RemoteTriggerAddrs[j];
		ConfigBuf[i++] = Breaker.RemoteTriggers[j];
	}
	crc = CalCrc16(ConfigBuf, i);
	ConfigBuf[i++] = crc;
	ConfigBuf[i++] = crc >> 8;
//	WriteBurstEEPROM(StartAddr, ConfigBuf, BreakerInfoCnt);
     WriteBurstEEprom(StartAddr, ConfigBuf, BreakerInfoCnt);
}

void ReadBreaker(u8 Index)
{
	u8 ConfigBuf[BreakerInfoCnt]={0}, i = 0, j;
	u16 StartAddr;
	StartAddr = (BreakerInfoStartAddr + (BreakerInfoCnt * Index));
     /*
	for (i = 0; i < BreakerInfoCnt; i++)
	{
		*(ConfigBuf + i) = ReadEEprom(StartAddr + i);
	}*/
     
     ReadBurstEEprom(StartAddr,ConfigBuf,BreakerInfoCnt);

	if (CRC16Check(ConfigBuf, BreakerInfoCnt))//EEPROM存在传感器记录
	{
		i = 0;
		Breakers[Index].Addr = ConfigBuf[i++];
		Breakers[Index].Crc = ConfigBuf[i++];
		Breakers[Index].ForceControlFlag = ConfigBuf[i++];
		Breakers[Index].ForceControlPort = ConfigBuf[i++];
		Breakers[Index].CrossControlFlag = ConfigBuf[i++];
		Breakers[Index].CrossControlPort = ConfigBuf[i++];
		i += 2;

		for (j = 0; j < MaxLocalLinkCnt; j++)
		{
			Breakers[Index].LocalTriggerAddrs[j] = ConfigBuf[i++];
			Breakers[Index].LocalTriggers[j] = ConfigBuf[i++];
			if (Breakers[Index].LocalTriggerAddrs[j] != 0)
				Breakers[Index].RelevanceLocalSensorCnt++;
		}

		for (j = 0; j < MaxRemoteLinkCnt; j++)
		{
			Breakers[Index].RemoteTriggerAddrs[j] = ConfigBuf[i++];
			Breakers[Index].RemoteTriggers[j] = ConfigBuf[i++];
			if (Breakers[Index].RemoteTriggerAddrs[j] != 0)
				Breakers[Index].RelevanceRemoteSensorCnt++;
		}
	}
}

void EraseBreaker(u8 Addr)
{
	u8 i, j, Index,buf[BreakerInfoCnt];
	u16 StartAddr;
     for(i = 0;i < BreakerInfoCnt;i ++)
          buf[i] = 0xFF;
	Index = GetBreakerIndex(Addr);
	if (Index == 0xFF)
		return;
	Breakers[Index].Addr = 0;
	Breakers[Index].Crc = 0;
	Breakers[Index].ForceControlFlag = 0;
	Breakers[Index].ForceControlPort = 0;
	Breakers[Index].CrossControlFlag = 0;
	Breakers[Index].CrossControlPort = 0;

	for (j = 0; j < MaxLocalLinkCnt; j++)
	{
		Breakers[Index].LocalTriggerAddrs[j] = 0;
		Breakers[Index].LocalTriggers[j] = 0;

	}
	Breakers[Index].RelevanceLocalSensorCnt = 0;
	for (j = 0; j < MaxRemoteLinkCnt; j++)
	{
		Breakers[Index].RemoteTriggerAddrs[j] = 0;
		Breakers[Index].RemoteTriggers[j] = 0;

	}
	Breakers[Index].RelevanceRemoteSensorCnt = 0;
	StartAddr = (BreakerInfoStartAddr + (BreakerInfoCnt * Index));
     /*
	for (i = 0; i < BreakerInfoCnt; i++)
		WriteEEprom(StartAddr + i, 0xFF);
      * */
     WriteBurstEEprom(StartAddr,buf,BreakerInfoCnt);
}

void ReadAllBreaker(void)
{
	u8 i;
	for (i = 0; i < MaxBreakerCnt; i++)
	{
		ReadBreaker(i);
	}
}

void WriteRemoteSensor(_RemoteSensor* RemoteSensor)
{
	u8 ConfigBuf[RemoteSensorInfoCnt]={0}, i = 0, RemoteSensorIndex;
	u16 StartAddr, crc;
	RemoteSensorIndex = GetRemoteSensorIndex(RemoteSensor->Addr);
	if (RemoteSensorIndex == 0xFF)
		return;
	StartAddr = (RemoteSensorInfoStartAddr + (RemoteSensorIndex * RemoteSensorInfoCnt));
	ConfigBuf[i++] = RemoteSensor->Addr;
	ConfigBuf[i++] = RemoteSensor->UpDuanDian;
	ConfigBuf[i++] = RemoteSensor->UpDuanDian >> 8;
	ConfigBuf[i++] = RemoteSensor->UpFuDian;
	ConfigBuf[i++] = RemoteSensor->UpFuDian >> 8;
	ConfigBuf[i++] = RemoteSensor->DownDuanDian;
	ConfigBuf[i++] = RemoteSensor->DownDuanDian >> 8;
	ConfigBuf[i++] = RemoteSensor->DownFuDian;
	ConfigBuf[i++] = RemoteSensor->DownFuDian >> 8;
	crc = CalCrc16(ConfigBuf, i);
	ConfigBuf[i++] = crc;
	ConfigBuf[i++] = crc >> 8;
//	WriteBurstEEPROM(StartAddr, ConfigBuf, RemoteSensorInfoCnt);
     WriteBurstEEprom(StartAddr, ConfigBuf, RemoteSensorInfoCnt);
}

void ReadRemoteSensor(u8 Index)
{
	u8 ConfigBuf[RemoteSensorInfoCnt]={0}, i = 0;
	u16 StartAddr;
	StartAddr = (RemoteSensorInfoStartAddr + (RemoteSensorInfoCnt * Index));
     /*
	for (i = 0; i < RemoteSensorInfoCnt; i++)
	{
		*(ConfigBuf + i) = ReadEEprom(StartAddr + i);
	}
     */
     ReadBurstEEprom(StartAddr,ConfigBuf,RemoteSensorInfoCnt);
	if (CRC16Check(ConfigBuf, RemoteSensorInfoCnt))//EEPROM存在传感器记录
	{
		i = 0;
		RemoteSensors[Index].Addr = ConfigBuf[i++];

		RemoteSensors[Index].UpDuanDian = ConfigBuf[i + 1];
		RemoteSensors[Index].UpDuanDian <<= 8;
		RemoteSensors[Index].UpDuanDian += ConfigBuf[i];
		i += 2;

		RemoteSensors[Index].UpFuDian = ConfigBuf[i + 1];
		RemoteSensors[Index].UpFuDian <<= 8;
		RemoteSensors[Index].UpFuDian += ConfigBuf[i];
		i += 2;

		RemoteSensors[Index].DownDuanDian = ConfigBuf[i + 1];
		RemoteSensors[Index].DownDuanDian <<= 8;
		RemoteSensors[Index].DownDuanDian += ConfigBuf[i];
		i += 2;

		RemoteSensors[Index].DownFuDian = ConfigBuf[i + 1];
		RemoteSensors[Index].DownFuDian <<= 8;
		RemoteSensors[Index].DownFuDian += ConfigBuf[i];
		i += 2;
	}
}

void EraseRemoteSensor(u8 Addr)
{
	u8 i, RemoteSensorIndex,buf[RemoteSensorInfoCnt];
	u16 StartAddr;
	RemoteSensorIndex = GetRemoteSensorIndex(Addr);
	if (RemoteSensorIndex == 0xFF)
		return;
	StartAddr = (RemoteSensorInfoStartAddr + (RemoteSensorInfoCnt * RemoteSensorIndex));
     /*
	for (i = 0; i < RemoteSensorInfoCnt; i++)
		WriteEEprom(StartAddr + i, 0xFF);*/
     for(i = 0;i < RemoteSensorInfoCnt;i ++)
          buf[i] = 0xFF;
     WriteBurstEEprom(StartAddr,buf,RemoteSensorInfoCnt);
}

void ReadAllRemoteSensor(void)
{
	u8 i;
	for (i = 0; i < MaxRemoteSensorCnt; i++)
	{
		ReadRemoteSensor(i);
	}
}
