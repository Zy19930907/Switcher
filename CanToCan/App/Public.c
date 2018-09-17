#include "Public.h"
#include "CPU.h"
#define GENP   0xA001

_LocalSensor LocalSensors[MaxLocalSensorCnt];
_RemoteSensor RemoteSensors[MaxRemoteSensorCnt];
_Breaker Breakers[MaxBreakerCnt];
_Breaker NullBreaker, NoBreaker;
_RemoteSensor NullRemoteSensor, NoRemoteSensor;
extern vu16 SYS_TICK, SYS_TICK_1S;
u16 Crc16;

u16 MsTickDiff(u16 tick)
{
	if (SYS_TICK >= tick)
		return SYS_TICK - tick;
	else
		return 0xffff - tick + SYS_TICK;
}

u16 SecTickDiff(u16 tick)
{
	if (SYS_TICK_1S >= tick)
		return SYS_TICK_1S - tick;
	else
		return 0xffff - tick + SYS_TICK_1S;
}

void BufCopy(u8 *s, u8 *d, u16 len)
{
	if (len <= 0)
		return;
	do
	{
		*s++ = *d++;
	} while (--len);
}

void EarseBuf(u8 *buf, u16 len)
{
	if (len <= 0)
		return;
	do
	{
		*(buf++) = '\0';
	} while (--len);
}

void CRC16(u8 value)
{
	unsigned char i, temp = 0;
	Crc16 ^= value;
	for (i = 0; i < 8; i++)
	{
		temp = (Crc16 & 0x0001);
		Crc16 >>= 1;
		Crc16 &= 0x7fff;
		if (temp)
			Crc16 ^= GENP;
	}
}

u16 CalCrc16(u8 *buf, u8 len)
{
	u8 i;
	Crc16 = 0xffff;
	for (i = 0; i < len; i++)
		CRC16(buf[i]);
	return Crc16;
}

u8 CRC16Check(u8 *buf, u8 len)
{
	Crc16 = 0xffff;
	CalCrc16(buf, len - 2);
	if (((Crc16 & 0x00ff) == buf[len - 2]) && ((Crc16 >> 8) == buf[len - 1]))
		return 0x01;
	else
		return 0x00;
}

// flag 0 ������˫�ֽ�   1�����ظ��ֽ�+���ֽ� (��ʼ��У��ʱ��)

u16 CalCrcInit(u8 *buf, u16 len, u8 flag)
{
	u16 i;
	Crc16 = 0xffff;
	for (i = 0; i < len; i++)
		CRC16(buf[i]);
	if (flag) //  ���Ͱ�λ��߰�λ���
		Crc16 += (Crc16 >> 8);
	return Crc16;
}

_Breaker* GetBreaker(u8 addr)
{
	int i;
	for (i = 0; i < MaxBreakerCnt; i++)
	{
		if (Breakers[i].Addr == addr)
			return(&Breakers[i]);
	}
	return &NullBreaker;
}

_Breaker* FilterBreaker(u8 addr)
{
	u8 i;
	_Breaker* Breaker = GetBreaker(addr);
	if (Breaker->Addr == addr)
		return GetBreaker(addr);
	if (Breaker->Addr == 0)
	{
		for (i = 0; i < MaxBreakerCnt; i++)
		{
			if (Breakers[i].Addr == 0)
				return(&Breakers[i]);
		}
	}
	NoBreaker.Addr = 0xFF;
	return &NoBreaker;
}

u8 IsSensorLinkWithBreaker(u8 SensorAddr, _Breaker* Breaker)
{
	u8 i;
	for (i = 0; i < Breaker->RelevanceRemoteSensorCnt; i++)
	{
		if (SensorAddr == Breaker->RemoteTriggerAddrs[i])
			return 1;
	}
	return 0;
}

u8 GetBreakerIndex(u8 addr)
{
	u8 i;
	for (i = 0; i < MaxBreakerCnt; i++)
	{
		if (Breakers[i].Addr == addr)
			return i;
	}
	return 0xFF;
}

_Breaker* GetBreakerByIndex(u8 index)
{
	return &Breakers[index];
}

_RemoteSensor* GetRemoteSensor(u8 addr)
{
	int i;
	for (i = 0; i < MaxRemoteSensorCnt; i++)
	{
		if (RemoteSensors[i].Addr == addr)
			return(&RemoteSensors[i]);
	}
	return &NullRemoteSensor;
}
//Ϊ�µ���ش��������б���Ѱ��һ��λ��
_RemoteSensor* FilterRemoteSensor(u8 addr)
{
	u8 i;
	_RemoteSensor* RemoteSensor = GetRemoteSensor(addr);
	if (RemoteSensor->Addr == addr)
		return GetRemoteSensor(addr);//�ô����������б���
	if (RemoteSensor->Addr == 0)
	{
		for (i = 0; i < MaxRemoteSensorCnt; i++)
		{
			if (RemoteSensors[i].Addr == 0)//�ҵ���λ
				return(&RemoteSensors[i]);
		}
	}
	//�б�����
	NoRemoteSensor.Addr = 0xFF;
	return &NoRemoteSensor;
}
//��ȡ��ش��������б��е�λ��
u8 GetRemoteSensorIndex(u8 addr)
{
	u8 i;
	for (i = 0; i < MaxRemoteSensorCnt; i++)
	{
		if (RemoteSensors[i].Addr == addr)
			return i;
	}
	return 0xFF;
}

/*_RemoteSensor* GetRemoteSensorByIndex(u8 index)
{
	return &RemoteSensors[index];
}*/
//��鴫�����Ƿ�����һ���ϵ�������
u8 RemoteSensorAtSwitcher(u8 Addr)
{
	u8 i;
	for (i = 0; i < MaxBreakerCnt; i++)
	{
		if(Breakers[i].Addr == 0)
			continue;
		if (IsSensorLinkWithBreaker(Addr, &Breakers[i]))
			return 1;
	}
	return 0;
}
//�����ǰת����δ��������ش�����
void CheckRemoteSensor(void)
{
	u8 i;
	for (i = 0; i < MaxRemoteSensorCnt; i++)
	{
		if (RemoteSensors[i].Addr == 0)
			continue;
		if(RemoteSensorAtSwitcher(RemoteSensors[i].Addr))
			continue;
		EraseRemoteSensor(RemoteSensors[i].Addr);
	}
}

u16 GetChuShu(u8 flag)
{
	u16 chushu[4] = {1, 10, 100, 1000};
	return *(chushu+flag);
}

