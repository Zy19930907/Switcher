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
	0x39, 0xEE, 0x03, 0x03}; //0xEE为北京煤科院定义而菲莫未定义传感器
u8 SenserSwitch_F2B[] = {0x01, 0x02, 0x03, 0x2B, 0x05, 0x0C, 0x07, 0x09, 0x10, 0x0B,
	0x13, 0xED, 0x0A, 0xED, 0x11, 0x0E, 0xED, 0xED, 0xED, 0xED,
	0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED,
	0x0F, 0xED, 0xED, 0x0D, 0x28, 0xED, 0xED, 0xED, 0xED, 0xED, 0xED, 0x12};
u8 senserState1[3] = {0x41, 0x00, 0x00}; //开关量传感器状态0x00
u8 senserState2[3] = {0x01, 0x80, 0x00}; //开关量传感器状态0x01
u8 senserState3[3] = {0x02, 0x80, 0x00}; //开关量传感器状态0x02
u8 senserState4[3] = {0x02, 0xC0, 0x01}; //开关量传感器状态0x03
u8 sensor_result[3];

/*
 * 组装北京院协议CAN ID
 */
u32 MakePekingCanId(u8 Priority, u8 SenserType, u8 SenserAddr, u8 Dir, u8 Cmd)
{
	u32 PekingCanId;
	u32 temp1, temp2;
	PekingCanId = (Priority & 0x07); //帧优先级
	PekingCanId <<= 6;
	PekingCanId += (SenserType & 0x3F); //传感器类型码
	PekingCanId <<= 7;
	PekingCanId += (SenserAddr & 0x7F); //传感器地址码
	PekingCanId <<= 1;
	PekingCanId += (Dir & 0x01); //数据方向位
	PekingCanId <<= 12;
	PekingCanId += (Cmd & 0x1F); //指令码

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

/* 函数名：BufCompare
 * 函数功能：数组对比
 * 函数输入：需要对比的数组
 * 返回值：0x00：数组内容不同 0x01：数组内容相同
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

/* 函数名：ConverToByeFloat
 * 函数功能：将北京煤科院三字节浮点数代码转换为double型监测值
 * 函数输入：北京煤科院三字节浮点数代码
 * 返回值：北京煤科院传感器监测值
 */
double ConverToByeFloat(u8 *tran)
{
	double ddvalue;
	unsigned int immm = tran[1] *256 + tran[2];
	int signal = ((tran[0] & 0x80) > 0 ? -1 : 1); //符号
	int radixsignal = ((tran[0] & 0x40) > 0 ? -1 : 1); //阶码符号
	int radix = tran[0] & 0x3F;
	if (radixsignal == -1)//阶码(补码)是负的,转换补码成为原码
		radix = (radix ^ 0x3F) + 1;
	radix = radix * radixsignal;
	ddvalue = signal * immm * pow(2, (radix - 16));
	return(ddvalue);
}

//入口参数为FLOAT型数据，出口参数为三字节浮点数
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
				radix = (radix ^ 0x3F) + 1; //阶码为负数时采用反码
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

//北京院传感器初始化上传应答

void AnswerInitUpLoad(u8 type, u8 addr)
{
	CCan AckCan;
	if (type == GKT || type == HAIRDRYER || type == SMOG)//开关量传感器，开停、风筒烟雾
		LocalSensors[addr - 1].Delay = 6;
	else
		LocalSensors[addr - 1].Delay = 0;
	AckCan.ID = MakePekingCanId(0, type, addr, DIR_DOWN, CMD_INITUPLOAD);
	AckCan.Len = 0;
	CanDownSend(AckCan);
}

//北京院传感器初始化上传应答

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
	UpLoadData = ((Value > 0 ? 0x00 : 0x01) << 15); //数据正负
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

/* 函数名：CanProSwitch
 * 函数功能：将Can总线上接收到的北京煤科院协议的Can数据转换为菲莫协议的Can数据上传
 * 函数输入：Can_Peking：Can总线接收到的北京煤科院协议数据
 * 返回值：无
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
	cmd = Can_Peking.ID & 0x1F; //北京煤科院协议编码类型
	addr = Can_Peking.Buf[0]; //北京煤科院协议传感器地址
	sensorType = (Can_Peking.ID >> 20) & 0x3F; //获取北京煤科院协议设备类型码
	if (sensorType == POWER)//电源箱上传数据
	{ //电源上报数据
		addr = ((Can_Peking.ID >> 13)&0x7F);
		UploadSensor = &LocalSensors[15];
		UploadSensor->Addr = addr + Sys.AddrOffset + 16; //北京院传感器地址加转换板偏移地址
		UploadSensor->Tick = SYS_TICK_1S; //更新传感器在线时钟
		UploadSensor->SensorFlag &= ~0x01;//清除断线标志位
		UploadSensor->CtrFlag &= ~ENOFFLINEDUANDIAN; //清除传感器断线断电标记位	
		UploadSensor->Name = SenserSwitch_B2F[sensorType]; //北京院设备类型码转换
		if (cmd == CMD_INITUPLOAD)
		{
			AnswerInitUpLoad(POWER, 0);
			return;
		}
		if (Can_Peking.ID == 0x02801002)//电源箱紧急上传数据，需回复
			AnswerPowerUpLoad();
		PowerDataDeal(Can_Peking);
		return;
	}
	if (sensorType != POWERBREAKER)
	{
		UploadSensor = &LocalSensors[addr - 1];
		UploadSensor->Addr = addr + Sys.AddrOffset; //北京院传感器地址加转换板偏移地址
		UploadSensor->Tick = SYS_TICK; //更新传感器在线时钟
		UploadSensor->SensorFlag &= ~0x01;//清除断线标志位
		UploadSensor->CtrFlag &= ~ENOFFLINEDUANDIAN; //清除传感器断线断电标记位	
		UploadSensor->Name = SenserSwitch_B2F[sensorType]; //北京院设备类型码转换
		if (UploadSensor->Delay)
			UploadSensor->Delay--;
	}
	switch (cmd)
	{
	case CMD_TIMEUPLOAD://定时上报传感器数据
		if (sensorType == GKT || sensorType == HAIRDRYER || sensorType == SMOG)//开关量传感器，开停、风筒烟雾
		{
			UploadSensor->SensorFlag |= 0x40;
			//Can ID转换
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
		}//北京煤科院语音风门
		else if (sensorType == AIRDOOR)
		{
			UploadSensor->SensorFlag |= 0x40;
			Can_Feimo.ID = GetUpLoadCanID(*UploadSensor, SWITCHUPLOAD);
			if (BufCompare(&Can_Peking.Buf[2], senserState1, 3))//语音风门两风门关闭
			{
				Can_Feimo.Buf[0] = 0x00;
				Can_Feimo.Buf[1] = 0x01;
			} else if (BufCompare(&Can_Peking.Buf[2], senserState2, 3))//一开一闭
			{
				Can_Feimo.Buf[0] = 0x00;
				Can_Feimo.Buf[1] = 0x00;
			} else if (BufCompare(&Can_Peking.Buf[2], senserState3, 3))//两风门开启
			{
				Can_Feimo.Buf[0] = 0x00;
				Can_Feimo.Buf[1] = 0x02;
			}
			CanData = 0x0000 | Can_Feimo.Buf[1];
			
			Can_Feimo.Buf[2] = UploadSensor->Crc;
			Can_Feimo.Len = 3;
		} else if (sensorType == POWERBREAKER)
		{ //断电器
			Breaker->Flag &= ~0x02;
			Breaker = FilterBreaker(addr + Sys.AddrOffset);
			if (Breaker->Addr != 0xFF)
			{
				Breaker->Addr = addr + Sys.AddrOffset;
				Breaker->Tick = SYS_TICK;
				Can_Feimo.ID = MakeFeimoCanId(0x00, BREAKERUPLOAD, NCTR, DIR_UP, FM_POWERBREAKER, Breaker->Addr);
				if (BufCompare(&Can_Peking.Buf[2], senserState1, 3))
				{
					Can_Feimo.Buf[0] = 0x03; //断电无电
					CanData = 0x0003;
					Breaker->Flag |= 0x01;
				} else if (BufCompare(&Can_Peking.Buf[2], senserState2, 3))
				{
					Can_Feimo.Buf[0] = 0x00; //复电有电
					CanData = 0x0000;
					Breaker->Flag &= ~0x01;
				} else if (BufCompare(&Can_Peking.Buf[2], senserState3, 3))
				{
					Can_Feimo.Buf[0] = 0x01; //断电有电
					CanData = 0x0001;
					Breaker->Flag |= 0x01;
				} else if (BufCompare(&Can_Peking.Buf[2], senserState4, 3))
				{
					Can_Feimo.Buf[0] = 0x02; //复电无电
					CanData = 0x0002;
					Breaker->Flag &= ~0x01;
				}
				if (Breaker->ForceControlFlag)//如果断电器有手控则置位手控标志位
					Can.Buf[0] |= (1 << 2);
				if (Breaker->CrossControlFlag)//如果断电器有交控则置位交控标志位
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
				value = (u16) (ConverToByeFloat(&Can_Peking.Buf[2])*100); //传感器监测值
				CanData = MakeUpLoadData(0x02, DATATYPE_NORMAL, value);
				i = CanData & 0x0FFF;
				i /= GetChuShu((CanData >> 13) & 0x03);
				if ((UploadSensor->SensorFlag & 0x80) && UploadSensor->UpDuanDian != 0xFFFF)
				{
					j = UploadSensor->UpDuanDian & 0x0FFF;
					j /= GetChuShu((UploadSensor->UpDuanDian >> 13) & 0x03);
					if (i >= j)
					{
						Can_Feimo.ID |= 0x01000000; //向网关发送断电请求
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
				value = (u16) (ConverToByeFloat(&Can_Peking.Buf[2]) * 10); //传感器监测值
				CanData = MakeUpLoadData(0x01, DATATYPE_NORMAL, value);
				if ((UploadSensor->SensorFlag & 0x80) && UploadSensor->UpDuanDian != 0xFFFF)
				{
					i = CanData & 0x0FFF;
					i /= GetChuShu((CanData >> 13) & 0x03);
					j = (UploadSensor->UpDuanDian & 0x0FFF);
					j /= GetChuShu((UploadSensor->UpDuanDian >> 13) & 0x03);
					if (i >= j)
					{
						Can_Feimo.ID |= 0x01000000; //向网关发送超限断电标志
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
		if(Can_Peking.Buf[1] == 0x06) //北京院传感器上传标校数据
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
		break; //初始化上传，需回复，回复后传感器进入自动上报状态	
		
	case CMD_BREAKPOWER://断电控制信息应答
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
	case CMD_DOWNLOADWORINGVALUE://设置报警值应答

		break;
	case CMD_PREHEATVALUE://开机预热数据
		if (UploadSensor->SensorFlag & 0x80)
			Can_Feimo.ID = MakeFeimoCanId(0x00, INITIATIVEUPLOAD, CTR, DIR_UP, UploadSensor->Name, UploadSensor->Addr); //帧序号0 指令类型定时上报 参与控制
		else
			Can_Feimo.ID = MakeFeimoCanId(0x00, INITIATIVEUPLOAD, NCTR, DIR_UP, UploadSensor->Name, UploadSensor->Addr); //帧序号0 指令类型定时上报 不参与控制 上行

		if (sensorType == GKT || sensorType == HAIRDRYER || sensorType == SMOG || sensorType == AIRDOOR)//开关量传感器，开停、风筒烟雾
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

/*发送北京院同步上传时钟指令*/
void SyncClk(void)
{
	CCan ClkCan;
	ClkCan.ID = MakePekingCanId(0, 0, 0x10, DIR_DOWN, CMD_SYNCCLOCK);
	ClkCan.Len = 5;
	ClkCan.Buf[0] = 0;
	ClkCan.Buf[1] = 0;
	ClkCan.Buf[2] = 0;
	ClkCan.Buf[3] = 0;
	ClkCan.Buf[4] = 0x01; //与网关通信标志位
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
	//配置上限断电值
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
	if (LocalSensors[Addr - 1].DownWarn == 0xFFFF)//下限断电值上位机未配置，下发下限断电值0
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
		CheckBreaker(i); //找到断电器，检查断电器是否需要断复电
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
	
	if (Breaker->ForceControlFlag)//该断电器存在手动控制
	{
		if ((Breaker->ForceControlPort & 0x01) && (!(Breaker->Flag & 0x01)) && (MsTickDiff(Breaker->ActTick) >= 3000))//手动断电
		{
			Breaker->ActTick = SYS_TICK;
			Breaker->Flag = !Breaker->Flag;
			BreakPower(Breaker->Addr - Sys.AddrOffset);
		} else if ((!(Breaker->ForceControlPort & 0x01) && (Breaker->Flag & 0x01)) && (MsTickDiff(Breaker->ActTick) >= 3000))//手动复电
		{
			Breaker->ActTick = SYS_TICK;
			RePower(Breaker->Addr - Sys.AddrOffset); //不需要断电，复电
		}
		return; //断电器存在手动控制则不需要判断其他传感器断复电条件
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
	
	if (Breaker->CrossControlFlag)//检查断电器交叉控制断电
	{
		if ((Breaker->CrossControlPort & 0x01)&&((Breaker->Flag & 0x01) == 0))
		{
			BreakPower(Breaker->Addr - Sys.AddrOffset);
			Breaker->Flag = !Breaker->Flag;
		}
		return; //交叉控制触发断电不检查后面设备是否需要断电，直接退出当前断电器断复电检查
	}

	for (i = 0; i < Breaker->RelevanceLocalSensorCnt; i++)//检查该断电器关联的所有传感器是否需要控制断电器断电
	{
		ActSensorAddr = Breaker->LocalTriggerAddrs[i] - Sys.AddrOffset;
		ActSensor = &LocalSensors[ActSensorAddr - 1];
		//如果当前断电器对该传感器关联类型为超限断电切传感器当前状态超限
		if (
			((Breaker->LocalTriggers[i] & ENUPDUANDIAN) && (ActSensor->CtrFlag & (ENUPDUANDIAN)))//当前断电器关联的第i个传感器关联了上限断电且该传感器置位了上限断电标志位
			|| ((Breaker->LocalTriggers[i] & ENOFFLINEDUANDIAN) && (ActSensor->CtrFlag & (ENOFFLINEDUANDIAN)))//当前断电器关联的第i个传感器关联了断线断电且该传感器置位了断线断电标志位
			|| (((Breaker->LocalTriggers[i] & 0xC0) == 0xC0) && ((ActSensor->CurValue & 0x0001) == 0x0001))//当前断电器关联的第i个传感器关联了1态断电（认为该传感器为开关量）且该传感器当前状态为1态
			|| (((Breaker->LocalTriggers[i] & 0xA0) == 0xA0) && ((ActSensor->CurValue & 0x0001) == 0x0000))//当前断电器关联的第i个传感器关联了0态断电（认为该传感器为开关量）且该传感器当前状态为0态
			)
		{
			if (((Breaker->Flag & 0x01) == 0) && (MsTickDiff(Breaker->ActTick) >= 3000))//断电器当前为接通状态才断电（断电器动作冷却时间200毫秒）
			{
				Breaker->ActTick = SYS_TICK; //记录当前动作时间
				Breaker->Flag = !Breaker->Flag;
				BreakPower(Breaker->Addr - Sys.AddrOffset); //发送断电指令
				Breaker->TriggerAddr = ActSensor->Addr; //记录当前触发断电的传感器地址
				//生成联动记录
				BreakerLog.ID = MakeFeimoCanId(0, BREAKERLOGLOAD, NCTR, DIR_UP, FM_POWERBREAKER, Breaker->Addr);
				BreakerLog.Buf[0] = ActSensor->Addr; //联动触发的传感器地址
				BreakerLog.Buf[1] = (Breaker->LocalTriggers[i] | 0x80); //联动开始
				BreakerLog.Buf[2] = (ActSensor->CtrFlag); //触发事件
				//联动时间
				TimeChange();
				BreakerLog.Buf[3] = timeHex;
				BreakerLog.Buf[4] = timeHex >> 8;
				BreakerLog.Buf[5] = timeHex >> 16;
				BreakerLog.Buf[6] = timeHex >> 24;
				BreakerLog.Len = 7;
				CanUpSend(BreakerLog);
				Breaker->ActCnt++;
			}
			return; //只要触发断电就不检查后面设备是否需要断电，直接退出
		}
	}

	for (i = 0; i < Breaker->RelevanceRemoteSensorCnt; i++)//检查该断电器关联的所有传感器是否需要控制断电器断电
	{
		RemoteSensor = GetRemoteSensor(Breaker->RemoteTriggerAddrs[i]);
		if (RemoteSensor->Addr == 0)
			continue;
		if (
			((Breaker->RemoteTriggers[i] & ENUPDUANDIAN) && (RemoteSensor->CtrFlag & (ENUPDUANDIAN)))//当前断电器关联的第i个传感器关联了上限断电且该传感器置位了上限断电标志位
			|| ((Breaker->RemoteTriggers[i] & ENOFFLINEDUANDIAN) && (RemoteSensor->CtrFlag & (ENOFFLINEDUANDIAN)))//当前断电器关联的第i个传感器关联了断线断电且该传感器置位了断线断电标志位
			|| (((Breaker->RemoteTriggers[i] & 0xC0) == 0xC0) && ((RemoteSensor->CurValue & 0x0001) == 0x0001))//当前断电器关联的第i个传感器关联了1态断电（认为该传感器为开关量）且该传感器当前状态为1态
			|| (((Breaker->RemoteTriggers[i] & 0xA0) == 0xA0) && ((RemoteSensor->CurValue & 0x0001) == 0x0000))//当前断电器关联的第i个传感器关联了0态断电（认为该传感器为开关量）且该传感器当前状态为0态
			)
		{
			if (((Breaker->Flag & 0x01) == 0) && (MsTickDiff(Breaker->ActTick) >= 3000))//断电器当前为接通状态才断电（断电器动作冷却时间200毫秒）
			{
				Breaker->ActTick = SYS_TICK;
				Breaker->Flag = !Breaker->Flag;
				BreakPower(Breaker->Addr - Sys.AddrOffset);
				Breaker->TriggerAddr = RemoteSensor->Addr; //记录当前触发断电的传感器地址
				BreakerLog.ID = MakeFeimoCanId(0, BREAKERLOGLOAD, NCTR, DIR_UP, FM_POWERBREAKER, Breaker->Addr);
				BreakerLog.Buf[0] = RemoteSensor->Addr; //触发联动的传感器地址
				BreakerLog.Buf[1] = (Breaker->RemoteTriggers[i] | 0x80); //联动开始
				BreakerLog.Buf[2] = (RemoteSensor->CtrFlag); //触发事件
				TimeChange();
				BreakerLog.Buf[3] = timeHex;
				BreakerLog.Buf[4] = timeHex >> 8;
				BreakerLog.Buf[5] = timeHex >> 16;
				BreakerLog.Buf[6] = timeHex >> 24;
				BreakerLog.Len = 7;
				Breaker->ActCnt++;
				CanUpSend(BreakerLog);
			}
			return; //只要触发断电就不检查后面设备是否需要断电，直接退出
		}
	}

	if ((Breaker->Flag & 0x01) && (MsTickDiff(Breaker->ActTick) >= 3000))//没有手控且没有任何触发断电、当前断电器状态为断电时则复电（断电器动作冷却时间3秒）
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
			BreakerLog.Buf[1] = (Breaker->LocalTriggers[Breaker->TriggerAddr - 1] & (~0x80)); //联动结束
			BreakerLog.Buf[2] = (ActSensor->CtrFlag); //触发事件
		} else
		{
			BreakerLog.Buf[0] = RemoteSensor->Addr;
			BreakerLog.Buf[1] = (Breaker->RemoteTriggers[GetRemoteSensorIndex(Breaker->TriggerAddr)] & (~0x80)); //联动结束
			BreakerLog.Buf[2] = (RemoteSensor->CtrFlag); //触发事件
		}
		//联动结束时间
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
	FramCnt = ((PowerDataCan.Buf[0] >> 3) & 0x07); //电源数据帧序号
	BufCopy(PekingPowerData[FramCnt], PowerDataCan.Buf, PowerDataCan.Len);
}

void UploadPowerData(void)
{
	CCan PowerUploadCan;
	//电源在线且传感器类型为电源
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

//上传当前转换板传感器监测信息，每帧2个传感器信息，每个传感器占用4个字节，分别为 监测值低8位 监测值高8位 传感器CRC 传感器类型
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

//上传当前转换板所有传感器联动控制信息
//Can数据区8个字节，每个字节代表两个传感器联动信息，4个比特位代表1个传感器信息，D3、D2：00--模拟量传感器 01---开关量 11---断电器
//D1：0---传感器在线  1---传感器掉线	D0：0---模拟量为该传感器未超限，开关量表示当前为0态 1---模拟量为该传感器已超限，开关量表示当前为1态
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
				if(Sensor->SensorFlag & 0x40)//传感器为开关量传感器
				{
					temp |=  0x08;//传感器开关量标记位
					temp |= Sensor->CurValue & 0x01;//开关量传感器当前状态
				}
				else
					temp |= Sensor->CtrFlag & 0x01;//模拟量传感器当前是否超限
				if(Sensor->SensorFlag & 0x01)//传感器断线标记
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
