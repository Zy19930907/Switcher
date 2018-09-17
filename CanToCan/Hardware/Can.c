#include <xc.h>
#include "Can.h"


/*
  CANTX--------RB2
 *CANRX--------RB3
 */

extern CCan Can;
extern CSys Sys;
extern vu16 SYS_TICK;

/* 连接主干网络的CAN总线只接受2种情况数据
 * 1.接收网关所有的下行数据，是否需要转发需要根据该中继器是否挂接该设备
 * 2.接收主干网络的参与控制指令（不同中继器下的设备互相闭锁控制）
 * */
void CanUpInit(void)
{
	u16 tick;
	TRISB &= 0xFB;
	TRISB |= 0x08;
	ECANCON = 0x00;
	CANCON = 0x80; //进入配置模式
	while ((CANSTAT & 0x80) == 0);
	tick = SYS_TICK;
	while (MsTickDiff(tick) < 5);

	BRGCON1 = 0x3F; // Fosc = 16M, 分频BRP=63，则Tq = 8us  
	BRGCON2 = 0xBF; // 自由编程，采样一次，1+8+8+8模式
	BRGCON3 = 0x47;
	CIOCON = 0x21;

	RXB0CON = 0x40; // 接收扩展帧
	RXB1CON = 0x40;

	// 接收所有的下行数据
	RXM0SIDH = 0x00;
	RXM0SIDL = 0x08;
	RXM0EIDH = 0x00;
	RXM0EIDL = 0x08; //D14---EID3

	RXF0SIDH = 0; //地址相同才接受
	RXF0SIDL = 0x08;
	RXF0EIDH = 0x00;
	RXF0EIDL = 0x00;

	RXF1SIDH = 0; //地址相同才接受
	RXF1SIDL = 0x08;
	RXF1EIDH = 0x00;
	RXF1EIDL = 0x00;

	// 接收上行数据，并且是参与控制信息      
	RXM1SIDH = 0x00;
	RXM1SIDL = 0x08;
	RXM1EIDH = 0x00;
	RXM1EIDL = 0x18; //D15  D14  对应 EID4  EID3

	RXF2SIDH = 0x00;
	RXF2SIDL = 0x08;
	RXF2EIDH = 0x00;
	RXF2EIDL = 0x18;

	RXF3SIDH = 0x00;
	RXF3SIDL = 0x08;
	RXF3EIDH = 0x00;
	RXF3EIDL = 0x18;


	RXF4SIDH = 0x00;
	RXF4SIDL = 0x08;
	RXF4EIDH = 0x00;
	RXF4EIDL = 0x18;

	RXF5SIDH = 0x00;
	RXF5SIDL = 0x08;
	RXF5EIDH = 0x00;
	RXF5EIDL = 0x18;

	CANCON = 0x00; //设置为正常工作模式
	while ((CANSTAT & 0xE0) != 0);

	tick = SYS_TICK;
	while (MsTickDiff(tick) < 5);

	RXB1CONbits.RXFUL = 0;
	RXB1CONbits.RXFUL = 0;
}

// 发送缓冲区3个

u8 CheckCanUpTxBuf(void)
{
	u8 t;
	t = 0xFF;
	if (!(TXB0CON & 0x08)) //发送缓冲区1空
	{
		t = 0;
		return t;
	}
	if (!(TXB1CON & 0x08)) //发送缓冲区2空
	{
		t = 1;
		return t;
	}
	if (!(TXB2CON & 0x08)) //发送缓冲区3空
	{
		t = 2;
		return t;
	}
	return t;
}

void CanUpSend(CCan CanData)
{
	u8 i, *ptr, temp;
	u16 adr;
	
	temp = CanData.ID >> 27;
	temp &= 0x03;

	switch (CheckCanUpTxBuf()) 
	{
	case 0: // 发送缓冲区0
		TXB0EIDH = CanData.ID >> 19;
		TXB0EIDL = CanData.ID >> 11;
		TXB0SIDH = CanData.ID >> 3;
		TXB0SIDL = (CanData.ID << 5) + 0x08 + temp;
		TXB0DLC = CanData.Len;
		adr = 0x0F26; // 0x0F26 是 TXB0D0的地址
		ptr = (u8 *) adr;
		for (i = 0; i < CanData.Len; i++)
			*(ptr++) = CanData.Buf[i];
		TXB0CON |= 0x08; //启动发送
		break;
	case 1: // 发送缓冲区1
		TXB1EIDH = CanData.ID >> 19;
		TXB1EIDL = CanData.ID >> 11;
		TXB1SIDH = CanData.ID >> 3;
		TXB1SIDL = (CanData.ID << 5) + 0x08 + temp;
		TXB1DLC = CanData.Len;
		adr = 0x0F16; // 0x0F26 是 TXB0D0的地址
		ptr = (u8 *) adr;
		for (i = 0; i < CanData.Len; i++)
			*(ptr++) = CanData.Buf[i];
		TXB1CON |= 0x08; //启动发送
		break;
	case 2: // 发送缓冲区0
		TXB2EIDH = CanData.ID >> 19;
		TXB2EIDL = CanData.ID >> 11;
		TXB2SIDH = CanData.ID >> 3;
		TXB2SIDL = (CanData.ID << 5) + 0x08 + temp;
		TXB2DLC = CanData.Len;
		adr = 0x0F06; // 0x0F26 是 TXB0D0的地址
		ptr = (u8 *) adr;
		for (i = 0; i < CanData.Len; i++)
			*(ptr++) = CanData.Buf[i];
		TXB2CON |= 0x08; //启动发送
		break;
	default:
		break;
	}
}

u8 CheckCanDownTxBuf(void)
{
	u8 flag;
	flag = ReadRegCan(TXB0CTRL_M);
	if (!(flag & 0x08))
		return 0;
	flag = ReadRegCan(TXB1CTRL_M);
	if (!(flag & 0x08))
		return 1;
	flag = ReadRegCan(TXB2CTRL_M);
	if (!(flag & 0x08))
		return 2;
	return 0xFF;
}

void CanDownSend(CCan CanData)
{
	u8 t, RegShift;
	t = CheckCanDownTxBuf();
	if (t == 0xFF)
		return;
	switch (t) {
	case 0: RegShift = 0x00;
		break;
	case 1: RegShift = 0x10;
		break;
	case 2: RegShift = 0x20;
		break;
	}
	WriteRegCan(TXB0EIDH_M + RegShift, CanData.ID >> 19);
	WriteRegCan(TXB0EIDL_M + RegShift, CanData.ID >> 11);
	WriteRegCan(TXB0SIDH_M + RegShift, CanData.ID >> 3);
	WriteRegCan(TXB0SIDL_M + RegShift, (CanData.ID << 5) + 0x08 + (CanData.ID >> 27));
	WriteRegCan(TXB0DLC_M + RegShift, CanData.Len); //发送缓冲器0数据长度码
	WriteBurstRegCan(TXB0D0_M + RegShift, &CanData.Buf[0], CanData.Len);
	ModifyReg(TXB0CTRL_M + RegShift, 0x08, 0x08);
}

u8 SendByte(u8 dat)
{
	SSPBUF = dat;
	while (BF == 0);
	return SSPBUF;
}

void MCP2515Reset(void)
{
	CLR_CS;
	SendByte(CAN_RESET_M);
	SET_CS;
}

void WriteRegCan(u8 addr, u8 value)
{
	CLR_CS;
	SendByte(CAN_WRITE_M);
	SendByte(addr);
	SendByte(value);
	SET_CS;
}

void WriteBurstRegCan(u8 addr, u8 *buf, u8 len)
{
	u8 i;
	CLR_CS;
	SendByte(CAN_WRITE_M);
	SendByte(addr);
	for (i = 0; i < len; i++)
		SendByte(buf[i]);
	SET_CS;
}

u8 ReadRegCan(u8 addr)
{
	u8 value;
	CLR_CS;
	SendByte(CAN_READ_M);
	SendByte(addr);
	value = SendByte(0x00);
	SET_CS;
	return value;
}

void ReadBurstRegCan(u8 addr, u8 *buf, u8 len)
{
	u8 i;
	CLR_CS;
	SendByte(CAN_READ_M);
	SendByte(addr);
	for (i = 0; i < len; i++)
		buf[i] = SendByte(0x00);
	SET_CS;
}

// 修改寄存器中的某些位

void ModifyReg(u8 addr, u8 mask, u8 val)
{
	CLR_CS;
	SendByte(CAN_BIT_MODIFY_M);
	SendByte(addr);
	SendByte(mask);
	SendByte(val);
	SET_CS;
}
/*
u8 ReadStatus(void)
{
	 u8 temp;
	 CLR_CS;
	 SendByte(CAN_RD_STATUS);
	 temp = SendByte(0x00);
	 SET_CS;
	 return temp;
}*/
// RC2--CS  RC3--SCLK  RC4--MISO  RC5--MOSI
// 挂接传感器的CAN总线，对于该总线，只接收上行数据

void CanDownInit(void)
{
	u16 tick;
	TRISC &= ~0x2C; //RC2  RC3  RC5
	TRISC |= 0x10; //RC4
	// SPI 配置
	SSPCON1 = 0x21; // 时钟源 32M , 16分频 ---> 2M的实际SPI速率
	SSPSTAT = 0xC0;

	MCP2515Reset();
	tick = SYS_TICK;
	while (MsTickDiff(tick) < 10);
	// 器件在重新复位或者上电的时候，都会自动进入配置模式。此句可以省掉     
	WriteRegCan(CANCTRL_M, CONFIG_MODE_M); // CANCTRL 寄存器，设置为配置模式

	WriteRegCan(CNF1_M, 0x31); // 设置波特率：5K
	WriteRegCan(CNF2_M, 0xA4);
	WriteRegCan(CNF3_M, 0x04);

	WriteRegCan(CANINTE_M, 0x00); // 中断使能寄存器，CANINTE,3个发送缓冲区+2个接收缓冲区
	WriteRegCan(CANINTF_M, 0x00);
	WriteRegCan(RXB0CTRL_M, 0x44); // 接收扩展帧，滚动存储
	WriteRegCan(RXB1CTRL_M, 0x40); // 只接收扩展帧
	// 只关注EID3
	WriteRegCan(RXM0EIDH_M, 0x00);
	WriteRegCan(RXM0EIDL_M, 0x08);
	WriteRegCan(RXM0SIDH_M, 0x00);
	WriteRegCan(RXM0SIDL_M, 0x08);

	WriteRegCan(RXF0EIDH_M, 0x00);
	WriteRegCan(RXF0EIDL_M, 0x08);
	WriteRegCan(RXF0SIDH_M, 0x00);
	WriteRegCan(RXF0SIDL_M, 0x08);

	WriteRegCan(TXRTSCTRL_M, 0);
	WriteRegCan(BFPCTRL_M, 0); //BFPCTRL_RXnBF 引脚控制寄存器和状态寄存器
	WriteRegCan(CANCTRL_M, NORMAL_MODE_M); //正常模式
}



