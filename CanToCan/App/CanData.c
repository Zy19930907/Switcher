#include <xc.h>
#include "Public.h"
#include "CanData.h"
#include "Can.h"
#include "CPU.h"
#include "ProSwitch.h"

CCan Can;
CSys Sys;
extern vu16 SYS_TICK, SYS_TICK_1S;
extern _LocalSensor LocalSensors[16];
extern _Breaker Breakers[MaxBreakerCnt];
extern CTime Time;
extern u32 timeHex;
_InitInfo InitInfo;

void ResetInitInfo(void)
{
	InitInfo.R = 0;
	InitInfo.W = 0;
	InitInfo.Addr = 0xFF;
	EarseBuf(InitInfo.InitValue, 100);
}

u16 CalCrcConfig(_LocalSensor Sensor)
{
	u8 buf[13], i = 0;
	buf[i++] = Sensor.Addr;
	buf[i++] = Sensor.UpWarn;
	buf[i++] = Sensor.UpWarn >> 8;
	buf[i++] = Sensor.UpDuanDian;
	buf[i++] = Sensor.UpDuanDian >> 8;
	buf[i++] = Sensor.UpFuDian;
	buf[i++] = Sensor.UpFuDian >> 8;
	buf[i++] = Sensor.DownWarn;
	buf[i++] = Sensor.DownWarn >> 8;
	buf[i++] = Sensor.DownDuanDian;
	buf[i++] = Sensor.DownDuanDian >> 8;
	buf[i++] = Sensor.DownFuDian;
	buf[i++] = Sensor.DownFuDian >> 8;
	return CalCrcInit(buf, i, 1);
}

u8 SensorAtSwitcher(u8 addr)
{
	addr -= Sys.AddrOffset;
	if ((addr > 0) && (addr < MaxLocalSensorCnt))
		return 1;
	else
		return 0;
}

void UpDateInit(u8 SensorAddr)
{
	_Breaker* Breaker;
	_LocalSensor* ActSensor;
	_RemoteSensor* RemoteSensor;

	if (InitInfo.W <= 0)
		return;
	Breaker = GetBreaker(SensorAddr);
	//��ʼ����Ϣ�еĶϵ�����ϢSensers[SenserAddr - 1]
	if (Breaker->Addr != 0)
	{
		Breaker->RelevanceLocalSensorCnt = 0;
		Breaker->RelevanceRemoteSensorCnt = 0;
		EarseBuf(Breaker->LocalTriggerAddrs, MaxLocalLinkCnt); //���������ַ
		EarseBuf(Breaker->RemoteTriggerAddrs, MaxRemoteLinkCnt); //���������ַ
		do
		{
			switch (InitInfo.InitValue[InitInfo.R + 1] & 0x80)
			{
			case SENSERTYPE_SWITCH:
				if (SensorAtSwitcher(InitInfo.InitValue[InitInfo.R]))//�ϵ��������Ĵ������ڵ�ǰת������
				{
					ActSensor = &LocalSensors[(InitInfo.InitValue[InitInfo.R] - Sys.AddrOffset - 1)];
					ActSensor->SensorFlag |= 0x40;
					Breaker->LocalTriggerAddrs[Breaker->RelevanceLocalSensorCnt] = InitInfo.InitValue[InitInfo.R];
					Breaker->LocalTriggers[Breaker->RelevanceLocalSensorCnt] = InitInfo.InitValue[InitInfo.R + 1]; //��������������
					Breaker->RelevanceLocalSensorCnt++;
				} else
				{
					Breaker->RemoteTriggerAddrs[Breaker->RelevanceRemoteSensorCnt] = InitInfo.InitValue[InitInfo.R];
					Breaker->RemoteTriggers[Breaker->RelevanceRemoteSensorCnt] = InitInfo.InitValue[InitInfo.R + 1]; //��������������
					Breaker->RelevanceRemoteSensorCnt++;
					RemoteSensor = FilterRemoteSensor(InitInfo.InitValue[InitInfo.R]);
					if (RemoteSensor->Addr != 0xFF)
						RemoteSensor->Addr = InitInfo.InitValue[InitInfo.R];
					WriteRemoteSensor(RemoteSensor);
				}
				InitInfo.R += 2;
				break;
			case SENSERTYPE_ANALOG:
				if (SensorAtSwitcher(InitInfo.InitValue[InitInfo.R]))//�ϵ��������Ĵ������ڵ�ǰת������
				{
					ActSensor = &LocalSensors[(InitInfo.InitValue[InitInfo.R] - Sys.AddrOffset - 1)];
					ActSensor->SensorFlag &= (~0x40);
					Breaker->LocalTriggerAddrs[Breaker->RelevanceLocalSensorCnt] = InitInfo.InitValue[InitInfo.R];
					Breaker->LocalTriggers[Breaker->RelevanceLocalSensorCnt] = InitInfo.InitValue[InitInfo.R + 1]; //��������������
					Breaker->RelevanceLocalSensorCnt++;
				} else
				{
					Breaker->RemoteTriggerAddrs[Breaker->RelevanceRemoteSensorCnt] = InitInfo.InitValue[InitInfo.R];
					Breaker->RemoteTriggers[Breaker->RelevanceRemoteSensorCnt] = InitInfo.InitValue[InitInfo.R + 1]; //��������������
					Breaker->RelevanceRemoteSensorCnt++;
					RemoteSensor = FilterRemoteSensor(InitInfo.InitValue[InitInfo.R]);
					if (RemoteSensor->Addr != 0xFF)
					{
						RemoteSensor->Addr = InitInfo.InitValue[InitInfo.R];
						RemoteSensor->UpDuanDian = InitInfo.InitValue[InitInfo.R + 5];
						RemoteSensor->UpDuanDian <<= 8;
						RemoteSensor->UpDuanDian += InitInfo.InitValue[InitInfo.R + 4];

						RemoteSensor->UpFuDian = InitInfo.InitValue[InitInfo.R + 7];
						RemoteSensor->UpFuDian <<= 8;
						RemoteSensor->UpFuDian += InitInfo.InitValue[InitInfo.R + 6];

						RemoteSensor->DownDuanDian = InitInfo.InitValue[InitInfo.R + 11];
						RemoteSensor->DownDuanDian <<= 8;
						RemoteSensor->DownDuanDian += InitInfo.InitValue[InitInfo.R + 10];

						RemoteSensor->DownFuDian = InitInfo.InitValue[InitInfo.R + 13];
						RemoteSensor->DownFuDian <<= 8;
						RemoteSensor->DownFuDian += InitInfo.InitValue[InitInfo.R + 12];
						WriteRemoteSensor(RemoteSensor);
					}
				}
				InitInfo.R += 14;
				break;
			}
		} while (InitInfo.R < InitInfo.W);
		Breaker->Crc = CalCrcInit(InitInfo.InitValue, InitInfo.W, 1); //�ϵ���Crc
		WriteBreaker(*Breaker);
		CheckRemoteSensor();
	} else
	{
		ActSensor = &LocalSensors[SensorAddr - 1];
		ActSensor->SensorFlag |= 0x80;
		ActSensor->SensorFlag &= (~0x40);
		ActSensor->OffTimeout = 2350;
		if (InitInfo.W >= 8)
		{
			ActSensor->SensorFlag &= (~0x40);
			ActSensor->UpWarn = InitInfo.InitValue[1];
			ActSensor->UpWarn <<= 8;
			ActSensor->UpWarn += InitInfo.InitValue[0];

			ActSensor->UpDuanDian = InitInfo.InitValue[3];
			ActSensor->UpDuanDian <<= 8;
			ActSensor->UpDuanDian += InitInfo.InitValue[2];

			ActSensor->UpFuDian = InitInfo.InitValue[5];
			ActSensor->UpFuDian <<= 8;
			ActSensor->UpFuDian += InitInfo.InitValue[4];

			ActSensor->DownWarn = InitInfo.InitValue[7];
			ActSensor->DownWarn <<= 8;
			ActSensor->DownWarn += InitInfo.InitValue[6];

			ActSensor->DownDuanDian = InitInfo.InitValue[9];
			ActSensor->DownDuanDian <<= 8;
			ActSensor->DownDuanDian += InitInfo.InitValue[8];

			ActSensor->DownFuDian = InitInfo.InitValue[11];
			ActSensor->DownFuDian <<= 8;
			ActSensor->DownFuDian += InitInfo.InitValue[10];

			ActSensor->Crc = CalCrcConfig(*ActSensor);

			WriteLocalSenserConfig(*ActSensor);
			SetWornValue(SensorAddr);
		} else
		{
			ActSensor->SensorFlag |= 0x40;
			ActSensor->Crc = InitInfo.InitValue[0]; //������������Crc
			WriteLocalSenserConfig(*ActSensor);
		}
	}
	ResetInitInfo();
}
//Can������8���ֽڣ�ÿ���ֽڴ�������������������Ϣ��4������λ����1����������Ϣ��D3��D2��00--ģ���������� 01---������ 11---�ϵ���
//D1��0---����������  1---����������	D0��0---ģ����Ϊ�ô�����δ���ޣ���������ʾ��ǰΪ0̬ 1---ģ����Ϊ�ô������ѳ��ޣ���������ʾ��ǰΪ1̬
void SwitchCtrInfoDeal(u32 Id,u8 *buf)
{
	u8 i,j,SwitcherAddr,SensorAddr,SensorInfo1;
	u8 SensorFlag,SensorInfo;
	SwitcherAddr = Id & 0xFF; //ת�����ַ
	_RemoteSensor* Sensor;
	for(i=0;i<8;i++)
	{
		SensorInfo1 = buf[i];
		for(j=0;j<2;j++)
		{
			SensorAddr = SwitcherAddr + 2*i + j + 1;
			Sensor = GetRemoteSensor(SensorAddr);
			if(Sensor->Addr == 0)
				continue;
			
			if(j==0)
				SensorInfo = SensorInfo1 >> 4;
			else
				SensorInfo = SensorInfo1 & 0x0F;
			
			SensorFlag = ((SensorInfo & 0x0C) >> 2);
			
			if(SensorInfo & 0x02)//���������߱�־λ����
				Sensor->CtrFlag |= 0x40;
			else
			{
				Sensor->Tick = SYS_TICK;
				Sensor->CtrFlag &= ~0x40;
			}
			
			if(SensorFlag == 0x02) //��������������ȡ��ǰ״̬
				Sensor->CurValue = SensorInfo & 0x01;
			else if(SensorFlag == 0x00)//ģ���������������ޱ�־λ
			{
				if(SensorInfo & 0x01)
					Sensor->CtrFlag |= 0x01;
				else
					Sensor->CtrFlag &= ~0x01;
			}
		}
	}
}

void Break3_0InfoDeal(u8 *buf)
{
	_Breaker* Breaker;
	_LocalSensor* Sensor;
	u8 i,j,addr;
	for(i=0;i<MaxLocalSensorCnt;i++)
	{
		LocalSensors[i].SensorFlag &= ~0x02;
		if(i<5)
		{
			for(j=0;j<Breakers[i].Break3_0Cnt;j++)
				Breakers[i].Break3_0Addrs[j] = 0;
			Breakers[i].Break3_0Cnt = 0;
		}
	}
	
	for(i=0;i<Can.Len>>1;i++)
	{
		Breaker = GetBreaker(buf[2*i+1]);
		if(Breaker->Addr == 0)
			continue;
		addr = buf[2*i] - Sys.AddrOffset;
		if((addr <= MaxLocalSensorCnt) && (addr > 0))
		{
			Sensor = &LocalSensors[addr-1];
			Sensor->SensorFlag |= 0x02;
			Breaker->Break3_0Addrs[Breaker->Break3_0Cnt++] = Sensor->Addr;
		}
	}
	Sys.Get3_0 = 1;
}

//����������2�������1.���ظ��Ļ�Ӧ���߿�������   2.�����豸���㲥��Ϣ������
//  ������λ�����������豸������������
void HandleCanData(u8 index)
{
	u8 addr, cmd, fram;
	_Breaker* Breaker;
	_LocalSensor* ActSensor;
	_RemoteSensor* RemoteSensor;
	float i, j, k;
	switch (index)
	{
	case 0:
		Can.ID = (RXB0SIDL & 0x03);
		Can.ID <<= 8;
		Can.ID += RXB0EIDH;
		Can.ID <<= 8;
		Can.ID += RXB0EIDL;
		Can.ID <<= 8;
		Can.ID += RXB0SIDH;
		Can.ID <<= 3;
		Can.ID += (RXB0SIDL >> 5);
		break;
	case 1:
		Can.ID = (RXB1SIDL & 0x03);
		Can.ID <<= 8;
		Can.ID += RXB1EIDH;
		Can.ID <<= 8;
		Can.ID += RXB1EIDL;
		Can.ID <<= 8;
		Can.ID += RXB1SIDH;
		Can.ID <<= 3;
		Can.ID += (RXB1SIDL >> 5);
		break;
	}
	addr = Can.ID;
	cmd = ((Can.ID >> 17) & 0x7F);
	if(cmd == CMD_SWITCTRINFO)//����ת�����ϴ�����������Ϣ
	{
		SwitchCtrInfoDeal(Can.ID,&Can.Buf[0]);
		return;
	}
	else if(cmd == CMD_Break3_0CONFIG)
	{
		Break3_0InfoDeal(Can.Buf);
		return;
	}
	
	if (((addr - Sys.AddrOffset) <= MaxLocalSensorCnt) && ((addr - Sys.AddrOffset) > 0))//����豸��ַΪ��ǰת��������ַ
	{
		addr -= Sys.AddrOffset;
		Breaker = GetBreaker(addr + Sys.AddrOffset);
		if (Breaker->Addr == 0)
		{
			ActSensor = &LocalSensors[addr - 1];
			if (ActSensor->Addr == 0)
				return;
		}
          else
               ActSensor = &LocalSensors[addr - 1];
		switch (cmd)
		{
		case CMD_GETRELEVANCEADDR://��ѯ������������ַ
			if (Breaker->Addr != 0)
			{
				Can.ID = MakeFeimoCanId(0, CMD_GETRELEVANCEADDR, NCTR, DIR_UP, FM_POWERBREAKER, Breaker->Addr);
				for (Can.Len = 0; Can.Len < Breaker->RelevanceLocalSensorCnt; Can.Len++)
					Can.Buf[Can.Len] = Breaker->LocalTriggerAddrs[Can.Len];
				CanUpSend(Can);
			}
			break;

		case CMD_GETRELEVANCEFLAG://��ѯ��������
			if (Breaker->Addr != 0)
			{
				Can.ID = MakeFeimoCanId(0, CMD_GETRELEVANCEFLAG, NCTR, DIR_UP, FM_POWERBREAKER, Breaker->Addr);
				for (Can.Len = 0; Can.Len < Breaker->RelevanceLocalSensorCnt; Can.Len++)
					Can.Buf[Can.Len] = Breaker->LocalTriggers[Can.Len];
				CanUpSend(Can);
			}
			break;

		case CMD_GETUPWORNVALUE://��ѯ���ޱ���ֵ
			Can.ID = MakeFeimoCanId(0, CMD_GETUPWORNVALUE, NCTR, DIR_UP, ActSensor->Name, addr + Sys.AddrOffset);
			Can.Len = 0;
			Can.Buf[Can.Len++] = ActSensor->UpDuanDian;
			Can.Buf[Can.Len++] = ActSensor->UpDuanDian >> 8;
			Can.Buf[Can.Len++] = ActSensor->UpFuDian;
			Can.Buf[Can.Len++] = ActSensor->UpFuDian >> 8;
			Can.Buf[Can.Len++] = ActSensor->UpWarn;
			Can.Buf[Can.Len++] = ActSensor->UpWarn >> 8;
			CanUpSend(Can);
			break;
		case CMD_GETSOFTVERB:
			Can.ID = MakeFeimoCanId(0, CMD_GETSOFTVERB, NCTR, DIR_UP, ActSensor->Name, addr + Sys.AddrOffset);
			Can.Buf[0] = SoftVerb;
			Can.Len = 1;
			CanUpSend(Can);
			break;
		case CMD_GETDOWNWORNVALUE://��ѯ���ޱ���ֵ
			Can.ID = MakeFeimoCanId(0, CMD_GETDOWNWORNVALUE, NCTR, DIR_UP, ActSensor->Name, addr + Sys.AddrOffset);
			Can.Len = 0;
			Can.Buf[Can.Len++] = ActSensor->DownDuanDian;
			Can.Buf[Can.Len++] = ActSensor->DownDuanDian >> 8;
			Can.Buf[Can.Len++] = ActSensor->DownFuDian;
			Can.Buf[Can.Len++] = ActSensor->DownFuDian >> 8;
			Can.Buf[Can.Len++] = ActSensor->DownWarn;
			Can.Buf[Can.Len++] = ActSensor->DownWarn >> 8;
			CanUpSend(Can);
			break;

		case CMD_RESET://�����豸��CANת���壩
			Can.ID = MakeFeimoCanId(0, CMD_RESET, NCTR, DIR_UP, ActSensor->Name, addr + Sys.AddrOffset);
			Can.Len = 0;
			CanUpSend(Can);
			while (1);
			break;
          case 0x27://��ѯ�豸crc
			Can.ID = MakeFeimoCanId(0, 0x27, NCTR, DIR_UP, ActSensor->Name, addr + Sys.AddrOffset);
			Can.Len = 0;
			Can.Buf[Can.Len++] = ActSensor->Crc;
			CanUpSend(Can);
			break;

		case CMD_INIT://�����·���ʼ����Ϣ
			fram = ((Can.ID >> 24) & 0x0C); //��ȡID�е�֡����
			switch (fram)//֡����
			{
			case FRAMTYPE_SINGLE:
				InitInfo.Addr = addr;
				Sys.InitDelay = 10;
				if (Breaker->Addr != 0)//�����·��ϵ�����ʼ����Ϣ�������Ӧ�ϵ���������Ϣ
					InitInfo.Addr = addr + Sys.AddrOffset;
				BufCopy(InitInfo.InitValue, Can.Buf, Can.Len);
				InitInfo.W += Can.Len;
				UpDateInit(InitInfo.Addr);
				break;
			case FRAMTYPE_FIRST://��֡�е���֡
				Sys.InitDelay = 10;
				ResetInitInfo();
				InitInfo.Addr = addr;
				if (Breaker->Addr != 0)//�����·��ϵ�����ʼ����Ϣ�������Ӧ�ϵ���������Ϣ
					InitInfo.Addr = addr + Sys.AddrOffset;
				BufCopy(InitInfo.InitValue, Can.Buf, Can.Len);
				InitInfo.W += Can.Len;
				break;
			case FRAMTYPE_MIDDLE://��֡�е��м�֡
				BufCopy(&InitInfo.InitValue[InitInfo.W], Can.Buf, Can.Len);
				InitInfo.W += Can.Len;
				break;
			case FRAMTYPE_END://��֡�е�ĩβ֡
				BufCopy(&InitInfo.InitValue[InitInfo.W], Can.Buf, Can.Len);
				InitInfo.W += Can.Len;
				UpDateInit(InitInfo.Addr);
				break;
			}
               if(Breaker->Addr == 0)         // ������Ǵ�����
                    Can.ID = MakeFeimoCanId(0, CMD_INIT, NCTR, DIR_UP, ActSensor->Name, ActSensor->Addr);
               else
                    Can.ID = MakeFeimoCanId(0, CMD_INIT, NCTR, DIR_UP, ActSensor->Name, Breaker->Addr);
			Can.Len = 0;
			CanUpSend(Can);
			break;

		case CMD_CLRINIT://�����ʼ����Ϣ
			if (Breaker->Addr == 0)
			{
				EraseLocalSenser(addr);
				SetWornValue(addr);
				Can.ID = MakeFeimoCanId(0, ACK, NCTR, DIR_UP, ActSensor->Name, addr + Sys.AddrOffset);
				Can.Len = 0;
			} else
			{
				EraseBreaker(addr);
				Can.ID = MakeFeimoCanId(0, ACK, NCTR, DIR_UP, FM_POWERBREAKER, addr + Sys.AddrOffset);
				Can.Len = 0;
			}
			CanUpSend(Can);
			break;

		case CMD_SYNCTIME://ͬ��ʱ��ָ��
			Time.Buf[0] = (((Can.Buf[0] >> 4)*10) + Can.Buf[0] % 16); //��
			Time.Buf[1] = ((((Can.Buf[1] & 0x1F) >> 4)*10) + (Can.Buf[1]&0x1F) % 16); //�º��ܼ��ϲ���
			Time.Buf[2] = (((Can.Buf[2] >> 4)*10) + Can.Buf[2] % 16); //��
			Time.Buf[3] = ((((Can.Buf[1] >> 5) >> 4)*10) + (Can.Buf[1] >> 5) % 16); //�ܼ�
			Time.Buf[4] = (((Can.Buf[3] >> 4)*10) + Can.Buf[3] % 16);
			Time.Buf[5] = (((Can.Buf[4] >> 4)*10) + Can.Buf[4] % 16);
			Time.Buf[6] = (((Can.Buf[5] >> 4)*10) + Can.Buf[5] % 16);
			break;

		case CMD_GETTIME: //��ѯ�豸��ǰʱ��
			if (addr == 0x00) //�㲥����
				break;
			TimeChange();
			Can.Buf[0] = timeHex;
			Can.Buf[1] = timeHex >> 8;
			Can.Buf[2] = timeHex >> 16;
			Can.Buf[3] = timeHex >> 24;
			Can.Len = 4;
			Can.ID = MakeFeimoCanId(0, CMD_GETTIME, NCTR, DIR_UP, ActSensor->Name, addr);
			CanUpSend(Can);
			break;

		case CMD_GETSENSERINFO: //��ȡ�豸������Ϣ
			Can.Len = 0;
			if (Breaker->Addr == 0)
			{
				Can.Buf[Can.Len++] = ActSensor->Name;
				Can.Buf[Can.Len++] = 0;
				Can.Buf[Can.Len++] = Sys.Vol;
				Can.Buf[Can.Len++] = ActSensor->CurValue;
				Can.Buf[Can.Len++] = ActSensor->CurValue >> 8;
				Can.Buf[Can.Len++] = Sys.UpLoadTime;
				Can.Buf[Can.Len++] = (Sys.UpLoadTime >> 8);
				Can.ID = MakeFeimoCanId(0, CMD_GETSENSERINFO, NCTR, DIR_UP, ActSensor->Name, addr);
			} 
			else
			{
				Can.Buf[Can.Len++] = FM_POWERBREAKER;
				Can.Buf[Can.Len++] = Breaker->ActCnt;
				Can.Buf[Can.Len++] = Sys.Vol;
				Can.Buf[Can.Len++] = 0;
				Can.Buf[Can.Len++] = 0;
				Can.Buf[Can.Len++] = Sys.UpLoadTime;
				Can.Buf[Can.Len++] = (Sys.UpLoadTime >> 8);
				Can.ID = MakeFeimoCanId(0, CMD_GETSENSERINFO, NCTR, DIR_UP, FM_POWERBREAKER, addr);
			}
			CanUpSend(Can);
			break;

		case CMD_GETUPLOADTIME: //��ȡ�����ϴ�ʱ��
			Can.Len = 0;
			Can.Buf[Can.Len++] = Sys.UpLoadTime;
			Can.Buf[Can.Len++] = (Sys.UpLoadTime >> 8);
			Can.ID = MakeFeimoCanId(0, CMD_GETUPLOADTIME, NCTR, DIR_UP, ActSensor->Name, addr);
			CanUpSend(Can);
			break;

		case CMD_SETUPLOADTIME://���������ϴ�ʱ��
			Sys.UpLoadTime = Can.Buf[1];
			Sys.UpLoadTime <<= 8;
			Sys.UpLoadTime += Can.Buf[0];
			Can.ID = MakeFeimoCanId(0, CMD_SETUPLOADTIME, NCTR, DIR_UP, FM_POWERBREAKER, addr);
			Can.Len = 2;
			WriteBreaker(*Breaker);
			CanUpSend(Can);
			break;

		case CMD_GETOFFLINETIME://��ѯ�����������ж�ʱ��
			if (Breaker->Addr == 0)
			{
				Can.Buf[0] = ActSensor->OffTimeout;
				Can.Buf[1] = ActSensor->OffTimeout >> 8;
				Can.ID = MakeFeimoCanId(0, CMD_GETOFFLINETIME, NCTR, DIR_UP, ActSensor->Name, addr);
				Can.Len = 2;
				CanUpSend(Can);
			}
			break;

		case CMD_SETOFFLINETIME://���ô����������ж�ʱ��
			if (Breaker->Addr == 0)
			{
				ActSensor->OffTimeout = Can.Buf[1];
				ActSensor->OffTimeout <<= 8;
				ActSensor->OffTimeout += Can.Buf[0];
				Can.ID = MakeFeimoCanId(0, CMD_SETOFFLINETIME, NCTR, DIR_UP, ActSensor->Name, addr);
				Can.Len = 2;
				WriteLocalSenserConfig(*ActSensor);
				CanUpSend(Can);
			}
			break;

		case CMD_FORCECONTROL://�ֶ�����ָ��
			Breaker = GetBreaker(addr);
			if (Breaker->Addr != 0)
			{
				Breaker->ForceControlFlag = Can.Buf[0];
				Breaker->ForceControlPort = Can.Buf[1];
				WriteBreaker(*Breaker);
			}
			break;

		case CMD_CROSSCONTROL://�������ָ��
			Breaker = GetBreaker(addr);
			if (Breaker->Addr != 0)
			{
				Breaker->CrossControlFlag = Can.Buf[0];
				Breaker->CrossControlPort = Can.Buf[1];
				WriteBreaker(*Breaker);
			}
			break;
		}
	} else////����豸��ַ���ڵ�ǰת��������ַ
	{
		if (cmd == CMD_INIT)
			Sys.InitDelay = 10;
		if (!(Can.ID & 0x00008000))//ֻ��������������Ϣ
			return;
		RemoteSensor = GetRemoteSensor(addr);
		if (RemoteSensor->Addr == 0)
			return;
		RemoteSensor->Tick = SYS_TICK;
		RemoteSensor->CtrFlag &= ~ENOFFLINEDUANDIAN; //������������߶ϵ���λ
		switch (cmd)
		{
		case INITIATIVEUPLOAD://ģ���������ϴ�
			RemoteSensor->CurValue = Can.Buf[1];
			RemoteSensor->CurValue <<= 8;
			RemoteSensor->CurValue += Can.Buf[0];
			if (RemoteSensor->UpDuanDian != 0xFFFF)
			{
				i = RemoteSensor->CurValue & 0x0FFF;
				i /= GetChuShu((RemoteSensor->CurValue >> 13)&0x03);
				j = RemoteSensor->UpDuanDian & 0x0FFF;
				j /= GetChuShu((RemoteSensor->UpDuanDian >> 13)&0x03);
				if (i >= j)
				{
					RemoteSensor->CtrFlag |= 0x01;
					break;
				}
			}
			if (RemoteSensor->UpFuDian != 0xFFFF)
			{
				k = RemoteSensor->UpFuDian & 0x0FFF;
				k /= GetChuShu((RemoteSensor->UpFuDian >> 13)&0x03);
				if (i < k)
					RemoteSensor->CtrFlag &= ~0x01;
			}
			break;
		case SWITCHUPLOAD://�����������ϴ�
			RemoteSensor->CurValue = Can.Buf[0];
			RemoteSensor->CurValue <<= 8;
			RemoteSensor->CurValue += Can.Buf[1];
			break;
		}
	}
}

// ����CAN�����յ����ݣ�����ת��

void CanUpReceiveFunc(void)
{
	u8 i, *ptr;
	u16 adr;
	if (RXB0CON & 0x80)
	{
		Can.Len = RXB0DLC & 0x0F;
		adr = 0x0F66; //0x0F66 ��RXB0D0�ĵ�ַ
		ptr = (u8*) adr;
		for (i = 0; i < Can.Len; i++)
			Can.Buf[i] = *(ptr++);
		RXB0CONbits.RXFUL = 0;
		HandleCanData(0);
	}
	if (RXB1CON & 0x80)
	{
		Can.Len = RXB1DLC & 0x0F;
		adr = 0x0F36; //0x0F36 ��RXB1D0�ĵ�ַ
		ptr = (u8*) adr;

		for (i = 0; i < Can.Len; i++)
			Can.Buf[i] = *(ptr++);
		RXB1CONbits.RXFUL = 0;
		HandleCanData(1);
	}
}

void CanDownReceiveFunc(void)
{
	u8 flag;
	flag = ReadRegCan(CANINTF_M);
	if (flag & 0x01) //  ���ջ�����0
	{
		Can.Len = ReadRegCan(RXB0DLC_M);
		Can.ID = (ReadRegCan(RXB0SIDL_M) & 0x03); //EID16  EID15
		Can.ID <<= 8;
		Can.ID += ReadRegCan(RXB0EIDH_M);
		Can.ID <<= 8;
		Can.ID += ReadRegCan(RXB0EIDL_M);
		Can.ID <<= 8;
		Can.ID += ReadRegCan(RXB0SIDH_M);
		Can.ID <<= 3;
		Can.ID += (ReadRegCan(RXB0SIDL_M) >> 5);
		ReadBurstRegCan(RXB0D0_M, &Can.Buf[0], Can.Len);
		CanProSwitch(Can);
		ModifyReg(CANINTF_M, 0x21, 0x00);
	}
	if (flag & 0x02) //  ���ջ�����1
	{
		Can.Len = ReadRegCan(RXB1DLC_M);
		Can.ID = (ReadRegCan(RXB1SIDL_M) & 0x03); //EID16  EID15
		Can.ID <<= 8;
		Can.ID += ReadRegCan(RXB1EIDH_M);
		Can.ID <<= 8;
		Can.ID += ReadRegCan(RXB1EIDL_M);
		Can.ID <<= 8;
		Can.ID += ReadRegCan(RXB1SIDH_M);
		Can.ID <<= 3;
		Can.ID += (ReadRegCan(RXB1SIDL_M) >> 5);
		ReadBurstRegCan(RXB1D0_M, &Can.Buf[0], Can.Len);
		CanProSwitch(Can);
		ModifyReg(CANINTF_M, 0x22, 0x00);
	}
}
