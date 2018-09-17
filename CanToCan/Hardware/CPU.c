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
	while (EECON1bits.RD); //�ȴ����������
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
	GIE = 0; //�ر��ж�

	EECON2 = 0x55;
	EECON2 = 0xAA;
	EECON1bits.WR = 1;
	while (EECON1bits.WR); //�ȴ�д�������
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
     GIE = 0;           //�ر��ж�
     for(i = 0;i < len;i ++)
     {
          EEADRH = addr >> 8;
          EEADR  = addr;
          EEDATA = buf[i];
          EECON2 = 0x55;
          EECON2 = 0xAA;
          EECON1bits.WR = 1;
          while(EECON1bits.WR);   //�ȴ�д�������
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
          while(EECON1bits.RD);   //�ȴ����������
          Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();
          buf[i] = EEDATA;
          EECON1bits.RD = 0;
          addr ++;
     }
     GIE = 1;
}

// RB5---LED   RB4---�ź�����  RA0 RA1 RA2 RA3 RA5 RC0 RC1 ��ַѡͨ

void IoInit(void)
{
	TRISB &= ~0x20;
	TRISB |= 0x10;
	TRISA |= 0x2F;
	TRISC |= 0x03;
}
//D0...D6 ��Ӧ RA0 RA1 RA2 RA3 RA5 RC0 RC1

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
	PEIE = 1; //�����ж�
	CanUpInit();
	CanDownInit();
}

void Timer0Init(void)
{
	T0CON = 0xC4; //�ڲ��ⲿʱ��Ƶ�ʣ�16/4 M = 4M,����32��Ƶ--->�ȼ��� 125KƵ��
	TMR0IE = 1;
	TMR0IF = 0;
	TMR0 = 128; //1 ms��ʱ��TMR0����ģʽ��255�����
}

void Timer2Init(void) // ����16ms-----������1s
{
	T2CON = 0x7F; // Fosc/4 ʱ�ӣ�ǰ���16��Ƶ
	TMR2IE = 1;
	TMR2IF = 0;
	PR2 = 124; // 15625 = 125 X 125
}
// RB0---AN10 ��Ӧ�Ĳ���AD��
// ��Ӧ��AD�����õ���ѹ֮�󣬳���11���õ��ܵ�ѹ

void AdInit(void)
{
	TRISB |= 0x01;
	ANCON1 |= 0x01; // RB0 ��Ӧ��ģ��ӿ��� ANSEL8
	ADCON1 = 0x20; //�ڲ��ο���ѹ2.048V
	ADCON2 = 0xF3; //�Ҷ��룬�ڲ�ADʱ��Դ
	ADCON0 |= 0x01; //ʹ��ADģ��
}

u16 GetVolInputValue(u8 ch)
{
	u16 Vol;
	ADCON0bits.CHS = ch; //12 ͨ��
	ADCON0bits.GO = 1;
	while (ADCON0bits.GO);
	Vol = ADRESH;
	Vol <<= 8;
	Vol += ADRESL;
	return Vol;
}
// ��ѹ�ɼ�

void VolSample(void)
{
	u16 temp;
	temp = GetVolInputValue(VolInputSample);
	temp >>= 1; // 12bit��4096---��2048mv��ÿһ�̶ȴ���0.5mv
	temp *= 11;
	temp /= 100;
	Sys.Vol = temp;
}

static void interrupt SystemISR(void)
{
	if (TMR0IF) //��ʱ��0�жϣ�ϵͳ1msʱ��
	{
		TMR0IF = 0;
		TMR0 = 128;
		SYS_TICK++;
	}
	if (TMR2IF) //��ʱ��2�жϣ�8ms�ж�һ��
	{
		TMR2IF = 0;
		Timer2cnt++;
		if (Timer2cnt == 125) // 1s ��ȷ��ʱ��
		{
			Timer2cnt = 0;
			SYS_TICK_1S++;
		}
	}
}

/*
 * ��������ʼ����Ϣ�洢��ÿ����������ʼ����Ϣռ��13���ֽ� CRC(��ʼ��) �����ϱ�ʱ�� �϶� �ϸ� �¶� �¸�  CRC(�洢��Ϣ)
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
	if (CRC16Check(ConfigBuf, LocalSensorInfoCnt))//EEPROM���ڴ�������¼
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
	} else//EEPROM��û�д�������¼
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
//һ��ת�������16���豸���ϵ�ʱ��ȡ16���豸��ʼ����Ϣ

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
	timeHex = Time.Buf[0]; //��
	timeHex <<= 4;
	timeHex += Time.Buf[1]; //��
	timeHex <<= 5;
	timeHex += Time.Buf[2]; //��
	timeHex <<= 5;
	timeHex += Time.Buf[4]; //Сʱ
	timeHex <<= 6;
	timeHex += Time.Buf[5]; //��
	timeHex <<= 6;
	timeHex += Time.Buf[6]; //��
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

	if (CRC16Check(ConfigBuf, BreakerInfoCnt))//EEPROM���ڴ�������¼
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
	if (CRC16Check(ConfigBuf, RemoteSensorInfoCnt))//EEPROM���ڴ�������¼
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
