#include "ProSwitch.h"
#include "Can.h"
#include "CPU.h"
#include "math.h"

u8 PekingPowerData[7][8];
_PowerInfoUploader PowerInfoUploader;
extern CCan Can;
extern vu16 SYS_TICK, SYS_TICK_1S;
extern _LocalSensor LocalSensors[MaxLocalSensorCnt];
extern _RemoteSensor RemoteSensors[MaxRemoteSensorCnt];
extern CSys Sys;
extern u32 timeHex;

void CheckBreaker(u8 BreakAddr);
void InitUpLoadDeal(_LocalSensor InitSenser);
void PowerDataDeal(CCan PowerDataCan);


u8 SenserSwitch_B2F[] = {0xEE, 0x00, 0x01, 0x02, 0xEE, 0x04, 0xEE, 0x06, 0xEE, 0x07,
	0x0C, 0x09, 0x05, 0x22, 0x0F, 0x1F, 0x08, 0x0E, 0x2A, 0xEE,
	0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0x0C, 0xEE, 0xEE, 0xEE, 0xEE,
	0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE,
	0x39, 0xEE, 0x03, 0x03}; //0xEEΪ����ú��Ժ�������Īδ���崫����
u8 SenserSwitch_F2B[] = {0x01, 0x02, 0x03, 0x2B, 0x05, 0x0C, 0x07, 0x09, 0x10, 0x0B,
	0x13, 0xED, 0x0A, 0xED, 0x11, 0x0E, 0xED, 0xED, 0xED, 0xED,
	0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED,
	0x0F, 0xED, 0xED, 0x0D, 0x28, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0x12};
u8 senserState1[3] = {0x41, 0x00, 0x00}; //������������״̬0x00
u8 senserState2[3] = {0x01, 0x80, 0x00}; //������������״̬0x01
u8 senserState3[3] = {0x02, 0x80, 0x00}; //������������״̬0x02
u8 senserState4[3] = {0x02, 0xC0, 0x01}; //������������״̬0x03
u8 sensor_result[3];

/*
 * ��װ����ԺЭ��CAN ID
 */
u32 MakePekingCanId(u8 Priority, u8 SenserType, u8 SenserAddr, u8 Dir, u8 Cmd)
{
	u32 PekingCanId;
	u32 temp1, temp2;
	PekingCanId = (Priority & 0x07); //֡���ȼ�
	PekingCanId <<= 6;
	PekingCanId += (SenserType & 0x3F); //������������
	PekingCanId <<= 7;
	PekingCanId += (SenserAddr & 0x7F); //��������ַ��
	PekingCanId <<= 1;
	PekingCanId += (Dir & 0x01); //���ݷ���λ
	PekingCanId <<= 12;
	PekingCanId += (Cmd & 0x1F); //ָ����

	temp1 = PekingCanId >> 18;
	temp2 = (PekingCanId & 0x001FFFFF) << 11;
	PekingCanId = temp1 | temp2;

	return PekingCanId;
}

u32 MakeFeimoCanId(u8 FramCnt, u8 Cmd, u8 CtrFlag, u8 Dir, u8 Type, u8 Addr)
{
	u32 FeimoId = 0;
	FeimoId = FramCnt;
	FeimoId <<= 7;
	FeimoId += Cmd;
	FeimoId <<= 2;
	FeimoId += CtrFlag;
	FeimoId <<= 1;
	FeimoId += Dir;
	FeimoId <<= 6;
	FeimoId += Type;
	FeimoId <<= 8;
	FeimoId += Addr;
	return FeimoId;
}

void BreakPower(u8 Addr)
{
	CCan BreakCan;
	BreakCan.ID = MakePekingCanId(0, POWERBREAKER, Addr, DIR_DOWN, CMD_BREAKPOWER);
	BreakCan.Buf[0] = 0x01;
	EarseBuf(&BreakCan.Buf[1], 7);
	BreakCan.Len = 8;
	CanDownSend(BreakCan);
}

void RePower(u8 Addr)
{
	CCan BreakCan;
	BreakCan.ID = MakePekingCanId(0, POWERBREAKER, Addr, DIR_DOWN, CMD_BREAKPOWER);
	BreakCan.Buf[0] = 0x02;
	EarseBuf(&BreakCan.Buf[1], 7);
	BreakCan.Len = 8;
	CanDownSend(BreakCan);
}

/* ��������BufCompare
 * �������ܣ�����Ա�
 * �������룺��Ҫ�Աȵ�����
 * ����ֵ��0x00���������ݲ�ͬ 0x01������������ͬ
 */
u8 BufCompare(u8 *s, u8 *d, u8 len)
{
	u8 i;
	for (i = 0; i < len; i++)
	{
		if (*(s + i) != *(d + i))
			return 0;
	}
	return 1;
}

/* ��������ConverToByeFloat
 * �������ܣ�������ú��Ժ���ֽڸ���������ת��Ϊdouble�ͼ��ֵ
 * �������룺����ú��Ժ���ֽڸ���������
 * ����ֵ������ú��Ժ���������ֵ
 */
double ConverToByeFloat(u8 *tran)
{
	double ddvalue;
	unsigned int immm = tran[1] *256 + tran[2];
	int signal = ((tran[0] & 0x80) > 0 ? -1 : 1); //����
	int radixsignal = ((tran[0] & 0x40) > 0 ? -1 : 1); //�������
	int radix = tran[0] & 0x3F;
	if (radixsignal == -1)//����(����)�Ǹ���,ת�������Ϊԭ��
		radix = (radix ^ 0x3F) + 1;
	radix = radix * radixsignal;
	ddvalue = signal * immm * pow(2, (radix - 16));
	return(ddvalue);
}

//��ڲ���ΪFLOAT�����ݣ����ڲ���Ϊ���ֽڸ�����
//void ConverToMFloat(double dvalue, unsigned char  *sensor_result)

void ConverToMFloat(double dvalue)
{
	unsigned char kk;
	sensor_result[0] = sensor_result[1] = sensor_result[2] = 0;

	if (dvalue == 0)
	{
		sensor_result[0] = 0x41;
	} else
	{
		if (dvalue < 0)
		{
			sensor_result[0] = 0x80;
			dvalue = -dvalue;
		}

		if (dvalue >= 1.0)
		{
			int radix = 1;
			double itemp = dvalue;
			while ((itemp = itemp / 2.0) >= 1.0)
			{
				radix += 1;
			}
			sensor_result[0] += radix;

			dvalue = dvalue / pow(2, radix);
			for (kk = 0; kk < 16; kk++)
			{
				if (((dvalue * 2) - 1) >= 0)
				{
					if (kk < 8) sensor_result[1] += (int) pow(2, 8 - kk - 1);
					else sensor_result[2] += (int) pow(2, 16 - kk - 1);
					dvalue = dvalue * 2 - 1;
				} else
				{
					dvalue = dvalue * 2;
				}
			}

		} else
		{
			int radix = 0;
			double dtemp = dvalue;
			while ((dtemp = dtemp * 2) < 1)
			{
				radix += 1;
			}
			dvalue = dvalue * pow(2, radix);
			if (radix > 0)
			{
				radix = (radix ^ 0x3F) + 1; //����Ϊ����ʱ���÷���
				sensor_result[0] = sensor_result[0] + 0x40 + radix;
			}

			for (kk = 0; kk < 16; kk++)
			{
				if (((dvalue * 2) - 1) >= 0)
				{
					if (kk < 8) sensor_result[1] += (int) pow(2, 8 - kk - 1);
					else sensor_result[2] += (int) pow(2, 16 - kk - 1);
					dvalue = dvalue * 2 - 1;
				} else
				{
					dvalue = dvalue * 2;
				}
			}
		}
	}

}

//����Ժ��������ʼ���ϴ�Ӧ��

void AnswerInitUpLoad(u8 type, u8 addr)
{
	CCan AckCan;
	if (type == GKT || type == HAIRDRYER || type == SMOG)//����������������ͣ����Ͳ����
		LocalSensors[addr - 1].Delay = 6;
	else
		LocalSensors[addr - 1].Delay = 0;
	AckCan.ID = MakePekingCanId(0, type, addr, DIR_DOWN, CMD_INITUPLOAD);
	AckCan.Len = 0;
	CanDownSend(AckCan);
}

//����Ժ��������ʼ���ϴ�Ӧ��

void AnswerPowerUpLoad(void)
{
	CCan AckCan;
	AckCan.ID = MakePekingCanId(0, POWER, 0, DIR_DOWN, CMD_TIMEUPLOAD);
	AckCan.Len = 8;
	CanDownSend(AckCan);
}

u32 GetUpLoadCanID(_LocalSensor Sensor, u8 Cmd)
{
	if ((Sensor.SensorFlag & 0x80))
		return MakeFeimoCanId(0x00, Cmd, CTR, DIR_UP, Sensor.Name, Sensor.Addr);
	else
		return MakeFeimoCanId(0x00, Cmd, NCTR, DIR_UP, Sensor.Name, Sensor.Addr);
}

u16 MakeUpLoadData(u8 PointNum, u8 DataType, u16 Value)
{
	u16 UpLoadData = 0;
	UpLoadData = ((Value > 0 ? 0x00 : 0x01) << 15); //��������
	UpLoadData <<= 2;
	UpLoadData += PointNum;
	UpLoadData <<= 2;
	UpLoadData += DataType;
	UpLoadData <<= 11;
	UpLoadData += (Value & 0x07FF);
	return UpLoadData;
}

u32 CanIdSwitch(u32 CanId, u8 SwitchDir)
{
	u32 CanID = 0;
	switch (SwitchDir)
	{
	case CANIDSWITCH_B2F:
		break;
	case CANIDSWITCH_F2B:
		CanID = ((CanId & 0x000007FF) << 18);
		CanID |= ((CanId & 0x7FFFF800) >> 11);
		break;
	}
	return CanID;
}

/* ��������CanProSwitch
 * �������ܣ���Can�����Ͻ��յ��ı���ú��ԺЭ���Can����ת��Ϊ��ĪЭ���Can�����ϴ�
 * �������룺Can_Peking��Can���߽��յ��ı���ú��ԺЭ������
 * ����ֵ����
 */
void CanProSwitch(CCan Can_Peking)
{
	CCan Can_Feimo;
	u8 addr, sensorType, cmd;
	u16 CanData = 0;
	u16 value;
	float i, j, k;
	_LocalSensor* UploadSensor;
	_Breaker* Breaker;

	TimeChange();
	Can_Feimo.ID = 0;
	Can_Feimo.Len = 0;
	Can_Peking.ID = CanIdSwitch(Can_Peking.ID, CANIDSWITCH_F2B);
	cmd = Can_Peking.ID & 0x1F; //����ú��ԺЭ���������
	addr = Can_Peking.Buf[0]; //����ú��ԺЭ�鴫������ַ
	sensorType = (Can_Peking.ID >> 20) & 0x3F; //��ȡ����ú��ԺЭ���豸������
	if (sensorType == POWER)//��Դ���ϴ�����
	{ //��Դ�ϱ�����
		addr = ((Can_Peking.ID >> 13)&0x7F);
		UploadSensor = &LocalSensors[15];
		UploadSensor->Addr = addr + Sys.AddrOffset + 16; //����Ժ��������ַ��ת����ƫ�Ƶ�ַ
		UploadSensor->Tick = SYS_TICK_1S; //���´���������ʱ��
		UploadSensor->SensorFlag &= ~0x01;//������߱�־λ
		UploadSensor->CtrFlag &= ~ENOFFLINEDUANDIAN; //������������߶ϵ���λ	
		UploadSensor->Name = SenserSwitch_B2F[sensorType]; //����Ժ�豸������ת��
		if (cmd == CMD_INITUPLOAD)
		{
			AnswerInitUpLoad(POWER, 0);
			return;
		}
		if (Can_Peking.ID == 0x02801002)//��Դ������ϴ����ݣ���ظ�
			AnswerPowerUpLoad();
		PowerDataDeal(Can_Peking);
		return;
	}
	if (sensorType != POWERBREAKER)
	{
		UploadSensor = &LocalSensors[addr - 1];
		UploadSensor->Addr = addr + Sys.AddrOffset; //����Ժ��������ַ��ת����ƫ�Ƶ�ַ
		UploadSensor->Tick = SYS_TICK; //���´���������ʱ��
		UploadSensor->SensorFlag &= ~0x01;//������߱�־λ
		UploadSensor->CtrFlag &= ~ENOFFLINEDUANDIAN; //������������߶ϵ���λ	
		UploadSensor->Name = SenserSwitch_B2F[sensorType]; //����Ժ�豸������ת��
		if (UploadSensor->Delay)
			UploadSensor->Delay--;
	}
	switch (cmd)
	{
	case CMD_TIMEUPLOAD://��ʱ�ϱ�����������
		if (sensorType == GKT || sensorType == HAIRDRYER || sensorType == SMOG)//����������������ͣ����Ͳ����
		{
			UploadSensor->SensorFlag |= 0x40;
			//Can IDת��
			if (UploadSensor->Delay)
				return;
			Can_Feimo.ID = GetUpLoadCanID(LocalSensors[addr - 1], SWITCHUPLOAD);
			if (BufCompare(&Can_Peking.Buf[2], senserState1, 3))
			{
				Can_Feimo.Buf[0] = 0x00;
				Can_Feimo.Buf[1] = 0x01;
				CanData = 0x0001;
			} else
			{
				Can_Feimo.Buf[0] = 0x00;
				Can_Feimo.Buf[1] = 0x00;
				CanData = 0x0000;
			}
			Can_Feimo.Buf[2] = UploadSensor->Crc;
			Can_Feimo.Len = 3;
		}//����ú��Ժ��������
		else if (sensorType == AIRDOOR)
		{
			UploadSensor->SensorFlag |= 0x40;
			Can_Feimo.ID = GetUpLoadCanID(*UploadSensor, SWITCHUPLOAD);
			if (BufCompare(&Can_Peking.Buf[2], senserState1, 3))//�������������Źر�
			{
				Can_Feimo.Buf[0] = 0x00;
				Can_Feimo.Buf[1] = 0x01;
			} else if (BufCompare(&Can_Peking.Buf[2], senserState2, 3))//һ��һ��
			{
				Can_Feimo.Buf[0] = 0x00;
				Can_Feimo.Buf[1] = 0x00;
			} else if (BufCompare(&Can_Peking.Buf[2], senserState3, 3))//�����ſ���
			{
				Can_Feimo.Buf[0] = 0x00;
				Can_Feimo.Buf[1] = 0x02;
			}
			CanData = 0x0000 | Can_Feimo.Buf[1];
			
			Can_Feimo.Buf[2] = UploadSensor->Crc;
			Can_Feimo.Len = 3;
		} else if (sensorType == POWERBREAKER)
		{ //�ϵ���
			Breaker->Flag &= ~0x02;
			Breaker = FilterBreaker(addr + Sys.AddrOffset);
			if (Breaker->Addr != 0xFF)
			{
				Breaker->Addr = addr + Sys.AddrOffset;
				Breaker->Tick = SYS_TICK;
				Can_Feimo.ID = MakeFeimoCanId(0x00, BREAKERUPLOAD, NCTR, DIR_UP, FM_POWERBREAKER, Breaker->Addr);
				if (BufCompare(&Can_Peking.Buf[2], senserState1, 3))
				{
					Can_Feimo.Buf[0] = 0x03; //�ϵ��޵�
					CanData = 0x0003;
					Breaker->Flag |= 0x01;
				} else if (BufCompare(&Can_Peking.Buf[2], senserState2, 3))
				{
					Can_Feimo.Buf[0] = 0x00; //�����е�
					CanData = 0x0000;
					Breaker->Flag &= ~0x01;
				} else if (BufCompare(&Can_Peking.Buf[2], senserState3, 3))
				{
					Can_Feimo.Buf[0] = 0x01; //�ϵ��е�
					CanData = 0x0001;
					Breaker->Flag |= 0x01;
				} else if (BufCompare(&Can_Peking.Buf[2], senserState4, 3))
				{
					Can_Feimo.Buf[0] = 0x02; //�����޵�
					CanData = 0x0002;
					Breaker->Flag &= ~0x01;
				}
				if (Breaker->ForceControlFlag)//����ϵ������ֿ�����λ�ֿر�־λ
					Can.Buf[0] |= (1 << 2);
				if (Breaker->CrossControlFlag)//����ϵ����н�������λ���ر�־λ
					Can.Buf[0] |= (1 << 3);
				Can_Feimo.Buf[1] = 0;
				Can_Feimo.Buf[2] = timeHex;
				Can_Feimo.Buf[3] = timeHex >> 8;
				Can_Feimo.Buf[4] = timeHex >> 16;
				Can_Feimo.Buf[5] = timeHex >> 24;
				Can_Feimo.Buf[6] = Breaker->Crc;
				Can_Feimo.Len = 7;
				if (Breaker->Addr != 0)
				{
					if (Breaker->CurValue != CanData)
					{
						Breaker->CurValue = CanData;
						CanUpSend(Can_Feimo);
						Breaker->UpLoadTick = SYS_TICK;
					}
				}
				break;
			}
		} else
		{
			UploadSensor->SensorFlag &= (~0x40);
			Can_Feimo.ID = GetUpLoadCanID(*UploadSensor, INITIATIVEUPLOAD);
			if (sensorType == PRESSURE || sensorType == CO_1000 || sensorType == CO2_5 || sensorType == CH4LASER_100)
			{
				value = (u16) (ConverToByeFloat(&Can_Peking.Buf[2])*100); //���������ֵ
				CanData = MakeUpLoadData(0x02, DATATYPE_NORMAL, value);
				i = CanData & 0x0FFF;
				i /= GetChuShu((CanData >> 13) & 0x03);
				if ((UploadSensor->SensorFlag & 0x80) && UploadSensor->UpDuanDian != 0xFFFF)
				{
					j = UploadSensor->UpDuanDian & 0x0FFF;
					j /= GetChuShu((UploadSensor->UpDuanDian >> 13) & 0x03);
					if (i >= j)
					{
						Can_Feimo.ID |= 0x01000000; //�����ط��Ͷϵ�����
						UploadSensor->CtrFlag |= ENUPDUANDIAN;
					}
					k = UploadSensor->UpFuDian & 0x0FFF;
					k /= GetChuShu((UploadSensor->UpFuDian >> 13)&0x03);
					if (i < k)
						UploadSensor->CtrFlag &= ~ENUPDUANDIAN;
				}
				
				if(UploadSensor->SensorFlag & 0x02)
				{
					if(value >= 300)
					{
						UploadSensor->CtrFlag |= 0x02;
					}
					if(value < 150)
					{
						UploadSensor->CtrFlag &= ~(0x02);
					}
				}
			} 
			else
			{
				value = (u16) (ConverToByeFloat(&Can_Peking.Buf[2]) * 10); //���������ֵ
				CanData = MakeUpLoadData(0x01, DATATYPE_NORMAL, value);
				if ((UploadSensor->SensorFlag & 0x80) && UploadSensor->UpDuanDian != 0xFFFF)
				{
					i = CanData & 0x0FFF;
					i /= GetChuShu((CanData >> 13) & 0x03);
					j = (UploadSensor->UpDuanDian & 0x0FFF);
					j /= GetChuShu((UploadSensor->UpDuanDian >> 13) & 0x03);
					if (i >= j)
					{
						Can_Feimo.ID |= 0x01000000; //�����ط��ͳ��޶ϵ��־
						UploadSensor->CtrFlag |= ENUPDUANDIAN;
					}
					k = UploadSensor->UpFuDian & 0x0FFF;
					k /= GetChuShu((UploadSensor->UpFuDian >> 13) & 0x03);
					if (i < k)
						UploadSensor->CtrFlag &= ~ENUPDUANDIAN;
				}
			}
			Can_Feimo.Buf[0] = CanData;
			Can_Feimo.Buf[1] = CanData >> 8;
			Can_Feimo.Buf[2] = timeHex;
			Can_Feimo.Buf[3] = timeHex >> 8;
			Can_Feimo.Buf[4] = timeHex >> 16;
			Can_Feimo.Buf[5] = timeHex >> 24;
			Can_Feimo.Buf[6] = UploadSensor->Crc;
			Can_Feimo.Len = 7;
		}
		if(Can_Peking.Buf[1] == 0x06) //����Ժ�������ϴ���У����
		{
			CanData &= ~(0x1800);
			CanData |= 0x1000;
		}
		
		if (UploadSensor->CurValue != CanData)
		{
			if (!UploadSensor->Delay)
			{
				UploadSensor->CurValue = CanData;
				CanUpSend(Can_Feimo);
				UploadSensor->UpLoadTick = SYS_TICK;
			}
		}
		break;
		
	case CMD_INITUPLOAD:
		if (sensorType != POWERBREAKER)
			AnswerInitUpLoad(SenserSwitch_F2B[UploadSensor->Name], UploadSensor->Addr - Sys.AddrOffset);
		else
		{
			Breaker = FilterBreaker(addr + Sys.AddrOffset);
			if (Breaker->Addr != 0xFF)
			{
				Breaker->Addr = addr + Sys.AddrOffset;
				AnswerInitUpLoad(POWERBREAKER, Breaker->Addr - Sys.AddrOffset);
			}
		}
		break; //��ʼ���ϴ�����ظ����ظ��󴫸��������Զ��ϱ�״̬	
		
	case CMD_BREAKPOWER://�ϵ������ϢӦ��
		if (Can_Peking.Buf[2] == 0x02)
		{
			Breaker->CurValue |= 0x01;
			Breaker->Flag |= 0x01;
		} else if (Can_Peking.Buf[2] |= 0x01)
		{
			Breaker->CurValue &= ~0x01;
			Breaker->Flag &= ~0x01;
		}
		Can_Feimo.ID = MakeFeimoCanId(0x00, BREAKERUPLOAD, NCTR, DIR_UP, FM_POWERBREAKER, Breaker->Addr);
		Can_Feimo.Buf[0] = Breaker->CurValue;
		Can_Feimo.Buf[1] = Breaker->CurValue >> 8;
		Can_Feimo.Buf[2] = timeHex;
		Can_Feimo.Buf[3] = timeHex >> 8;
		Can_Feimo.Buf[4] = timeHex >> 16;
		Can_Feimo.Buf[5] = timeHex >> 24;
		Can_Feimo.Buf[6] = Breaker->Crc;
		Can_Feimo.Len = 7;
		CanUpSend(Can_Feimo);
		break;
	case CMD_DOWNLOADWORINGVALUE://���ñ���ֵӦ��

		break;
	case CMD_PREHEATVALUE://����Ԥ������
		if (UploadSensor->SensorFlag & 0x80)
			Can_Feimo.ID = MakeFeimoCanId(0x00, INITIATIVEUPLOAD, CTR, DIR_UP, UploadSensor->Name, UploadSensor->Addr); //֡���0 ָ�����Ͷ�ʱ�ϱ� �������
		else
			Can_Feimo.ID = MakeFeimoCanId(0x00, INITIATIVEUPLOAD, NCTR, DIR_UP, UploadSensor->Name, UploadSensor->Addr); //֡���0 ָ�����Ͷ�ʱ�ϱ� ��������� ����

		if (sensorType == GKT || sensorType == HAIRDRYER || sensorType == SMOG || sensorType == AIRDOOR)//����������������ͣ����Ͳ����
			return;
		CanData = MakeUpLoadData(0, DATATYPE_PREHOT, CanData);
		Can_Feimo.Buf[0] = CanData;
		Can_Feimo.Buf[1] = CanData >> 8;
		Can_Feimo.Buf[2] = timeHex;
		Can_Feimo.Buf[3] = timeHex >> 8;
		Can_Feimo.Buf[4] = timeHex >> 16;
		Can_Feimo.Buf[5] = timeHex >> 24;
		Can_Feimo.Buf[6] = UploadSensor->Crc;
		Can_Feimo.Len = 7;
		CanUpSend(Can_Feimo);
		break;
	default:
		break;
	}
}

/*���ͱ���Ժͬ���ϴ�ʱ��ָ��*/
void SyncClk(void)
{
	CCan ClkCan;
	ClkCan.ID = MakePekingCanId(0, 0, 0x10, DIR_DOWN, CMD_SYNCCLOCK);
	ClkCan.Len = 5;
	ClkCan.Buf[0] = 0;
	ClkCan.Buf[1] = 0;
	ClkCan.Buf[2] = 0;
	ClkCan.Buf[3] = 0;
	ClkCan.Buf[4] = 0x01; //������ͨ�ű�־λ
	CanDownSend(ClkCan);
}

void SetWornValue(u8 Addr)
{
	double WronValue;
	u8 type;
	u16 i;
	CCan WornSetCan;
	type = SenserSwitch_F2B[LocalSensors[Addr - 1].Name];
	WornSetCan.ID = MakePekingCanId(0, type, Addr, DIR_DOWN, CMD_DOWNLOADWORINGVALUE);
	WornSetCan.Buf[0] = 0;
	WornSetCan.Buf[1] = 0;
	//�������޶ϵ�ֵ
	if (LocalSensors[Addr - 1].UpWarn != 0xFFFF)
	{
		WronValue = (LocalSensors[Addr - 1].UpWarn & 0x0FFF);
		i = GetChuShu((LocalSensors[Addr - 1].UpWarn >> 13) & 0x03);
		WronValue /= i;
	} else
	{
		WronValue = 0x0FFF;
	}
	ConverToMFloat(WronValue);
	BufCopy(&WornSetCan.Buf[2], sensor_result, 3);
	if (LocalSensors[Addr - 1].DownWarn == 0xFFFF)//���޶ϵ�ֵ��λ��δ���ã��·����޶ϵ�ֵ0
		WronValue = 0x8FFF;
	else
	{
		WronValue = (LocalSensors[Addr - 1].DownDuanDian & 0x0FFF);
		i = GetChuShu((LocalSensors[Addr - 1].UpWarn >> 13) & 0x03);
		WronValue /= i;
	}
	ConverToMFloat(WronValue);
	BufCopy(&WornSetCan.Buf[5], sensor_result, 3);
	WornSetCan.Len = 8;
	CanDownSend(WornSetCan);
}

void DuanDianPro(void)
{
	u8 i;
	for (i = 0; i < MaxBreakerCnt; i++)
	{
		CheckBreaker(i); //�ҵ��ϵ��������ϵ����Ƿ���Ҫ�ϸ���
	}
}

void CheckBreaker(u8 BreakerIndex)
{
	u8 i, ActSensorAddr;
	_Breaker* Breaker;
	_LocalSensor* ActSensor;
	_RemoteSensor* RemoteSensor;
	CCan BreakerLog;

	Breaker = GetBreakerByIndex(BreakerIndex);
	if (Breaker->Addr == 0)
		return;
	
	if (Breaker->ForceControlFlag)//�öϵ��������ֶ�����
	{
		if ((Breaker->ForceControlPort & 0x01) && (!(Breaker->Flag & 0x01)) && (MsTickDiff(Breaker->ActTick) >= 3000))//�ֶ��ϵ�
		{
			Breaker->ActTick = SYS_TICK;
			Breaker->Flag = !Breaker->Flag;
			BreakPower(Breaker->Addr - Sys.AddrOffset);
		} else if ((!(Breaker->ForceControlPort & 0x01) && (Breaker->Flag & 0x01)) && (MsTickDiff(Breaker->ActTick) >= 3000))//�ֶ�����
		{
			Breaker->ActTick = SYS_TICK;
			RePower(Breaker->Addr - Sys.AddrOffset); //����Ҫ�ϵ磬����
		}
		return; //�ϵ��������ֶ���������Ҫ�ж������������ϸ�������
	}

	for(i=0;i<Breaker->Break3_0Cnt;i++)
	{
		ActSensor = &LocalSensors[Breaker->Break3_0Addrs[i] - 1];
		if(ActSensor->CtrFlag & 0x02)
		{
			if((!(Breaker->Flag & 0x01)) && (MsTickDiff(Breaker->ActTick) >= 3000))
			{
				Breaker->ActTick = SYS_TICK;
				Breaker->Flag = !Breaker->Flag;
				BreakPower(Breaker->Addr - Sys.AddrOffset);
			}
			return;
		}
	}
	
	if (Breaker->CrossControlFlag)//���ϵ���������ƶϵ�
	{
		if ((Breaker->CrossControlPort & 0x01)&&((Breaker->Flag & 0x01) == 0))
		{
			BreakPower(Breaker->Addr - Sys.AddrOffset);
			Breaker->Flag = !Breaker->Flag;
		}
		return; //������ƴ����ϵ粻�������豸�Ƿ���Ҫ�ϵ磬ֱ���˳���ǰ�ϵ����ϸ�����
	}

	for (i = 0; i < Breaker->RelevanceLocalSensorCnt; i++)//���öϵ������������д������Ƿ���Ҫ���ƶϵ����ϵ�
	{
		ActSensorAddr = Breaker->LocalTriggerAddrs[i] - Sys.AddrOffset;
		ActSensor = &LocalSensors[ActSensorAddr - 1];
		//�����ǰ�ϵ����Ըô�������������Ϊ���޶ϵ��д�������ǰ״̬����
		if (
			((Breaker->LocalTriggers[i] & ENUPDUANDIAN) && (ActSensor->CtrFlag & (ENUPDUANDIAN)))//��ǰ�ϵ��������ĵ�i�����������������޶ϵ��Ҹô�������λ�����޶ϵ��־λ
			|| ((Breaker->LocalTriggers[i] & ENOFFLINEDUANDIAN) && (ActSensor->CtrFlag & (ENOFFLINEDUANDIAN)))//��ǰ�ϵ��������ĵ�i�������������˶��߶ϵ��Ҹô�������λ�˶��߶ϵ��־λ
			|| (((Breaker->LocalTriggers[i] & 0xC0) == 0xC0) && ((ActSensor->CurValue & 0x0001) == 0x0001))//��ǰ�ϵ��������ĵ�i��������������1̬�ϵ磨��Ϊ�ô�����Ϊ���������Ҹô�������ǰ״̬Ϊ1̬
			|| (((Breaker->LocalTriggers[i] & 0xA0) == 0xA0) && ((ActSensor->CurValue & 0x0001) == 0x0000))//��ǰ�ϵ��������ĵ�i��������������0̬�ϵ磨��Ϊ�ô�����Ϊ���������Ҹô�������ǰ״̬Ϊ0̬
			)
		{
			if (((Breaker->Flag & 0x01) == 0) && (MsTickDiff(Breaker->ActTick) >= 3000))//�ϵ�����ǰΪ��ͨ״̬�Ŷϵ磨�ϵ���������ȴʱ��200���룩
			{
				Breaker->ActTick = SYS_TICK; //��¼��ǰ����ʱ��
				Breaker->Flag = !Breaker->Flag;
				BreakPower(Breaker->Addr - Sys.AddrOffset); //���Ͷϵ�ָ��
				Breaker->TriggerAddr = ActSensor->Addr; //��¼��ǰ�����ϵ�Ĵ�������ַ
				//����������¼
				BreakerLog.ID = MakeFeimoCanId(0, BREAKERLOGLOAD, NCTR, DIR_UP, FM_POWERBREAKER, Breaker->Addr);
				BreakerLog.Buf[0] = ActSensor->Addr; //���������Ĵ�������ַ
				BreakerLog.Buf[1] = (Breaker->LocalTriggers[i] | 0x80); //������ʼ
				BreakerLog.Buf[2] = (ActSensor->CtrFlag); //�����¼�
				//����ʱ��
				TimeChange();
				BreakerLog.Buf[3] = timeHex;
				BreakerLog.Buf[4] = timeHex >> 8;
				BreakerLog.Buf[5] = timeHex >> 16;
				BreakerLog.Buf[6] = timeHex >> 24;
				BreakerLog.Len = 7;
				CanUpSend(BreakerLog);
				Breaker->ActCnt++;
			}
			return; //ֻҪ�����ϵ�Ͳ��������豸�Ƿ���Ҫ�ϵ磬ֱ���˳�
		}
	}

	for (i = 0; i < Breaker->RelevanceRemoteSensorCnt; i++)//���öϵ������������д������Ƿ���Ҫ���ƶϵ����ϵ�
	{
		RemoteSensor = GetRemoteSensor(Breaker->RemoteTriggerAddrs[i]);
		if (RemoteSensor->Addr == 0)
			continue;
		if (
			((Breaker->RemoteTriggers[i] & ENUPDUANDIAN) && (RemoteSensor->CtrFlag & (ENUPDUANDIAN)))//��ǰ�ϵ��������ĵ�i�����������������޶ϵ��Ҹô�������λ�����޶ϵ��־λ
			|| ((Breaker->RemoteTriggers[i] & ENOFFLINEDUANDIAN) && (RemoteSensor->CtrFlag & (ENOFFLINEDUANDIAN)))//��ǰ�ϵ��������ĵ�i�������������˶��߶ϵ��Ҹô�������λ�˶��߶ϵ��־λ
			|| (((Breaker->RemoteTriggers[i] & 0xC0) == 0xC0) && ((RemoteSensor->CurValue & 0x0001) == 0x0001))//��ǰ�ϵ��������ĵ�i��������������1̬�ϵ磨��Ϊ�ô�����Ϊ���������Ҹô�������ǰ״̬Ϊ1̬
			|| (((Breaker->RemoteTriggers[i] & 0xA0) == 0xA0) && ((RemoteSensor->CurValue & 0x0001) == 0x0000))//��ǰ�ϵ��������ĵ�i��������������0̬�ϵ磨��Ϊ�ô�����Ϊ���������Ҹô�������ǰ״̬Ϊ0̬
			)
		{
			if (((Breaker->Flag & 0x01) == 0) && (MsTickDiff(Breaker->ActTick) >= 3000))//�ϵ�����ǰΪ��ͨ״̬�Ŷϵ磨�ϵ���������ȴʱ��200���룩
			{
				Breaker->ActTick = SYS_TICK;
				Breaker->Flag = !Breaker->Flag;
				BreakPower(Breaker->Addr - Sys.AddrOffset);
				Breaker->TriggerAddr = RemoteSensor->Addr; //��¼��ǰ�����ϵ�Ĵ�������ַ
				BreakerLog.ID = MakeFeimoCanId(0, BREAKERLOGLOAD, NCTR, DIR_UP, FM_POWERBREAKER, Breaker->Addr);
				BreakerLog.Buf[0] = RemoteSensor->Addr; //���������Ĵ�������ַ
				BreakerLog.Buf[1] = (Breaker->RemoteTriggers[i] | 0x80); //������ʼ
				BreakerLog.Buf[2] = (RemoteSensor->CtrFlag); //�����¼�
				TimeChange();
				BreakerLog.Buf[3] = timeHex;
				BreakerLog.Buf[4] = timeHex >> 8;
				BreakerLog.Buf[5] = timeHex >> 16;
				BreakerLog.Buf[6] = timeHex >> 24;
				BreakerLog.Len = 7;
				Breaker->ActCnt++;
				CanUpSend(BreakerLog);
			}
			return; //ֻҪ�����ϵ�Ͳ��������豸�Ƿ���Ҫ�ϵ磬ֱ���˳�
		}
	}

	if ((Breaker->Flag & 0x01) && (MsTickDiff(Breaker->ActTick) >= 3000))//û���ֿ���û���κδ����ϵ硢��ǰ�ϵ���״̬Ϊ�ϵ�ʱ�򸴵磨�ϵ���������ȴʱ��3�룩
	{
		Breaker->ActTick = SYS_TICK;
		Breaker->Flag = !Breaker->Flag;
		RePower(Breaker->Addr - Sys.AddrOffset);
		BreakerLog.ID = MakeFeimoCanId(0, BREAKERLOGLOAD, NCTR, DIR_UP, FM_POWERBREAKER, Breaker->Addr);
		RemoteSensor = GetRemoteSensor(Breaker->TriggerAddr);
		Breaker->ActCnt++;
		if (RemoteSensor->Addr == 0)
		{
			ActSensor = &LocalSensors[Breaker->TriggerAddr - Sys.AddrOffset - 1];
			BreakerLog.Buf[0] = ActSensor->Addr;
			BreakerLog.Buf[1] = (Breaker->LocalTriggers[Breaker->TriggerAddr - 1] & (~0x80)); //��������
			BreakerLog.Buf[2] = (ActSensor->CtrFlag); //�����¼�
		} else
		{
			BreakerLog.Buf[0] = RemoteSensor->Addr;
			BreakerLog.Buf[1] = (Breaker->RemoteTriggers[GetRemoteSensorIndex(Breaker->TriggerAddr)] & (~0x80)); //��������
			BreakerLog.Buf[2] = (RemoteSensor->CtrFlag); //�����¼�
		}
		//��������ʱ��
		TimeChange();
		BreakerLog.Buf[3] = timeHex;
		BreakerLog.Buf[4] = timeHex >> 8;
		BreakerLog.Buf[5] = timeHex >> 16;
		BreakerLog.Buf[6] = timeHex >> 24;
		BreakerLog.Len = 7;
		CanUpSend(BreakerLog);
	}
}

void PowerDataDeal(CCan PowerDataCan)
{
	u8 FramCnt;
	FramCnt = ((PowerDataCan.Buf[0] >> 3) & 0x07); //��Դ����֡���
	BufCopy(PekingPowerData[FramCnt], PowerDataCan.Buf, PowerDataCan.Len);
}

void UploadPowerData(void)
{
	CCan PowerUploadCan;
	//��Դ�����Ҵ���������Ϊ��Դ
	if (((LocalSensors[15].CtrFlag & ENOFFLINEDUANDIAN) == 0x00) && LocalSensors[15].Name == FM_PKPOWER)
	{
		PowerUploadCan.ID = MakeFeimoCanId(0, CMD_PKPOWERUPLOAD, NCTR, DIR_UP, FM_PKPOWER, LocalSensors[15].Addr);
		if (PowerInfoUploader.UploadCnt < 6)
		{
			BufCopy(PowerUploadCan.Buf, PekingPowerData[PowerInfoUploader.UploadCnt++], 8);
			PowerUploadCan.Len = 8;
		} else
		{
			BufCopy(PowerUploadCan.Buf, PekingPowerData[PowerInfoUploader.UploadCnt++], 3);
			PowerUploadCan.Len = 3;
			PowerInfoUploader.UploadCnt = 0;
		}
		CanUpSend(PowerUploadCan);
		PowerInfoUploader.Tick = SYS_TICK;
	}
}

//�ϴ���ǰת���崫���������Ϣ��ÿ֡2����������Ϣ��ÿ��������ռ��4���ֽڣ��ֱ�Ϊ ���ֵ��8λ ���ֵ��8λ ������CRC ����������
void UpLoadSensorData(void)
{
	u8 i, index,temp;
	CCan DataInfo;
	_LocalSensor* UploadSensor;
	_Breaker* UploadBreaker;
	
	if(Sys.UploadCnt >= 8)
		Sys.UploadCnt = 0;
	DataInfo.ID = MakeFeimoCanId(Sys.UploadCnt, CMD_SWITCHUPLOAD, NCTR, DIR_UP, FM_SWITCHER, Sys.AddrOffset);
	
	for (i = 0; i < 2; i++)
	{
		temp = 0;
		index = Sys.UploadCnt * 2 + i;
		UploadBreaker = GetBreaker(Sys.AddrOffset+index + 1);
		if(UploadBreaker->Addr == 0)
		{
			UploadSensor = &LocalSensors[index];
			DataInfo.Buf[4*i] = (UploadSensor->CurValue & 0xFF);
			
			if(UploadSensor->SensorFlag & 0x40)
				temp |= 0x40;
			if(UploadSensor->SensorFlag & 0x01)
				temp |= 0x80;
			
			DataInfo.Buf[4*i+1] = ((UploadSensor->CurValue >> 8) & 0x7F);
			DataInfo.Buf[4*i+2] = UploadSensor->Crc;
			DataInfo.Buf[4*i+3] = ((UploadSensor->Name & 0x3F) | (temp & 0xC0));
		}
		else
		{
			DataInfo.Buf[4*i] = UploadBreaker->CurValue;
			DataInfo.Buf[4*i+1] = UploadBreaker->CurValue >> 8;
			DataInfo.Buf[4*i+2] = UploadBreaker->Crc;
			DataInfo.Buf[4*i+3] = FM_POWERBREAKER | 0x40;
               LocalSensors[(UploadBreaker->Addr)-1].Crc = UploadBreaker->Crc;
		}
	}
	DataInfo.Len = 8;
	CanUpSend(DataInfo);
	Sys.UploadCnt++;
}

//�ϴ���ǰת�������д���������������Ϣ
//Can������8���ֽڣ�ÿ���ֽڴ�������������������Ϣ��4������λ����1����������Ϣ��D3��D2��00--ģ���������� 01---������ 11---�ϵ���
//D1��0---����������  1---����������	D0��0---ģ����Ϊ�ô�����δ���ޣ���������ʾ��ǰΪ0̬ 1---ģ����Ϊ�ô������ѳ��ޣ���������ʾ��ǰΪ1̬
void UpLoadControlInfo(void)
{
	u8 i,j,temp;
	CCan ControlInfo;
	_LocalSensor* Sensor;
	_Breaker* Breaker;
	ControlInfo.ID = MakeFeimoCanId(0, CMD_SWITCTRINFO, CTR, DIR_UP, FM_SWITCHER, Sys.AddrOffset);
	for(i=0;i<8;i++)
	{
		for(j=0;j<2;j++)
		{
			temp = 0;
			Breaker = GetBreaker(Sys.AddrOffset+((2*i)+j) + 1);
			if(Breaker->Addr == 0)
			{
				Sensor = &LocalSensors[((2*i)+j)];
				if(Sensor->SensorFlag & 0x40)//������Ϊ������������
				{
					temp |=  0x08;//���������������λ
					temp |= Sensor->CurValue & 0x01;//��������������ǰ״̬
				}
				else
					temp |= Sensor->CtrFlag & 0x01;//ģ������������ǰ�Ƿ���
				if(Sensor->SensorFlag & 0x01)//���������߱��
					temp |= 0x02;
				if(j==0)
				{
					ControlInfo.Buf[i] = temp;
					ControlInfo.Buf[i] <<= 4;
				}
				else if(j==1)
				{
					ControlInfo.Buf[i] |= temp;
				}
			}
			else
			{
				temp |= 0x0C;
				temp |= Breaker->CurValue & 0x03;
				if(j==0)
				{
					ControlInfo.Buf[i] = temp;
					ControlInfo.Buf[i] <<= 4;
				}
				else if(j==1)
				{
					ControlInfo.Buf[i] |= temp;
				}
			}
		}
	}
	ControlInfo.Len = 8;
	CanUpSend(ControlInfo);
}

void SensorOnlineCheck(void)
{
	u8 i;
	_LocalSensor* Sensor;
	_Breaker* Breaker;
	for (i = 0; i < MaxLocalSensorCnt - 1; i++)
	{
		Breaker = GetBreaker(Sys.AddrOffset + i + 1);
		if (Breaker->Addr == 0)
		{
			Sensor = &LocalSensors[i];
			if (MsTickDiff(Sensor->Tick) >= Sensor->OffTimeout)
			{
				Sensor->SensorFlag |= 0x01;
				Sensor->CtrFlag |= 0x10;
			}
		} else
		{
			if (MsTickDiff(Breaker->Tick) >= 20000)
			{
				Breaker->Flag |= 0x02;
				Breaker->Addr = 0;
				Breaker->Crc = 0;
			}
		}
	}
}

void Get3_0Config(void)
{
	CCan Quest;
	Quest.ID =  MakeFeimoCanId(0, CMD_Break3_0CONFIG, NCTR, DIR_UP, FM_SWITCHER, Sys.AddrOffset);
	Quest.Len = 0;
	CanUpSend(Quest);
}
