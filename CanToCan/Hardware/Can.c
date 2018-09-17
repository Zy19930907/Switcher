#include <xc.h>
#include "Can.h"


/*
  CANTX--------RB2
 *CANRX--------RB3
 */

extern CCan Can;
extern CSys Sys;
extern vu16 SYS_TICK;

/* �������������CAN����ֻ����2���������
 * 1.�����������е��������ݣ��Ƿ���Ҫת����Ҫ���ݸ��м����Ƿ�ҽӸ��豸
 * 2.������������Ĳ������ָ���ͬ�м����µ��豸����������ƣ�
 * */
void CanUpInit(void)
{
	u16 tick;
	TRISB &= 0xFB;
	TRISB |= 0x08;
	ECANCON = 0x00;
	CANCON = 0x80; //��������ģʽ
	while ((CANSTAT & 0x80) == 0);
	tick = SYS_TICK;
	while (MsTickDiff(tick) < 5);

	BRGCON1 = 0x3F; // Fosc = 16M, ��ƵBRP=63����Tq = 8us  
	BRGCON2 = 0xBF; // ���ɱ�̣�����һ�Σ�1+8+8+8ģʽ
	BRGCON3 = 0x47;
	CIOCON = 0x21;

	RXB0CON = 0x40; // ������չ֡
	RXB1CON = 0x40;

	// �������е���������
	RXM0SIDH = 0x00;
	RXM0SIDL = 0x08;
	RXM0EIDH = 0x00;
	RXM0EIDL = 0x08; //D14---EID3

	RXF0SIDH = 0; //��ַ��ͬ�Ž���
	RXF0SIDL = 0x08;
	RXF0EIDH = 0x00;
	RXF0EIDL = 0x00;

	RXF1SIDH = 0; //��ַ��ͬ�Ž���
	RXF1SIDL = 0x08;
	RXF1EIDH = 0x00;
	RXF1EIDL = 0x00;

	// �����������ݣ������ǲ��������Ϣ      
	RXM1SIDH = 0x00;
	RXM1SIDL = 0x08;
	RXM1EIDH = 0x00;
	RXM1EIDL = 0x18; //D15  D14  ��Ӧ EID4  EID3

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

	CANCON = 0x00; //����Ϊ��������ģʽ
	while ((CANSTAT & 0xE0) != 0);

	tick = SYS_TICK;
	while (MsTickDiff(tick) < 5);

	RXB1CONbits.RXFUL = 0;
	RXB1CONbits.RXFUL = 0;
}

// ���ͻ�����3��

u8 CheckCanUpTxBuf(void)
{
	u8 t;
	t = 0xFF;
	if (!(TXB0CON & 0x08)) //���ͻ�����1��
	{
		t = 0;
		return t;
	}
	if (!(TXB1CON & 0x08)) //���ͻ�����2��
	{
		t = 1;
		return t;
	}
	if (!(TXB2CON & 0x08)) //���ͻ�����3��
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
	case 0: // ���ͻ�����0
		TXB0EIDH = CanData.ID >> 19;
		TXB0EIDL = CanData.ID >> 11;
		TXB0SIDH = CanData.ID >> 3;
		TXB0SIDL = (CanData.ID << 5) + 0x08 + temp;
		TXB0DLC = CanData.Len;
		adr = 0x0F26; // 0x0F26 �� TXB0D0�ĵ�ַ
		ptr = (u8 *) adr;
		for (i = 0; i < CanData.Len; i++)
			*(ptr++) = CanData.Buf[i];
		TXB0CON |= 0x08; //��������
		break;
	case 1: // ���ͻ�����1
		TXB1EIDH = CanData.ID >> 19;
		TXB1EIDL = CanData.ID >> 11;
		TXB1SIDH = CanData.ID >> 3;
		TXB1SIDL = (CanData.ID << 5) + 0x08 + temp;
		TXB1DLC = CanData.Len;
		adr = 0x0F16; // 0x0F26 �� TXB0D0�ĵ�ַ
		ptr = (u8 *) adr;
		for (i = 0; i < CanData.Len; i++)
			*(ptr++) = CanData.Buf[i];
		TXB1CON |= 0x08; //��������
		break;
	case 2: // ���ͻ�����0
		TXB2EIDH = CanData.ID >> 19;
		TXB2EIDL = CanData.ID >> 11;
		TXB2SIDH = CanData.ID >> 3;
		TXB2SIDL = (CanData.ID << 5) + 0x08 + temp;
		TXB2DLC = CanData.Len;
		adr = 0x0F06; // 0x0F26 �� TXB0D0�ĵ�ַ
		ptr = (u8 *) adr;
		for (i = 0; i < CanData.Len; i++)
			*(ptr++) = CanData.Buf[i];
		TXB2CON |= 0x08; //��������
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
	WriteRegCan(TXB0DLC_M + RegShift, CanData.Len); //���ͻ�����0���ݳ�����
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

// �޸ļĴ����е�ĳЩλ

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
// �ҽӴ�������CAN���ߣ����ڸ����ߣ�ֻ������������

void CanDownInit(void)
{
	u16 tick;
	TRISC &= ~0x2C; //RC2  RC3  RC5
	TRISC |= 0x10; //RC4
	// SPI ����
	SSPCON1 = 0x21; // ʱ��Դ 32M , 16��Ƶ ---> 2M��ʵ��SPI����
	SSPSTAT = 0xC0;

	MCP2515Reset();
	tick = SYS_TICK;
	while (MsTickDiff(tick) < 10);
	// ���������¸�λ�����ϵ��ʱ�򣬶����Զ���������ģʽ���˾����ʡ��     
	WriteRegCan(CANCTRL_M, CONFIG_MODE_M); // CANCTRL �Ĵ���������Ϊ����ģʽ

	WriteRegCan(CNF1_M, 0x31); // ���ò����ʣ�5K
	WriteRegCan(CNF2_M, 0xA4);
	WriteRegCan(CNF3_M, 0x04);

	WriteRegCan(CANINTE_M, 0x00); // �ж�ʹ�ܼĴ�����CANINTE,3�����ͻ�����+2�����ջ�����
	WriteRegCan(CANINTF_M, 0x00);
	WriteRegCan(RXB0CTRL_M, 0x44); // ������չ֡�������洢
	WriteRegCan(RXB1CTRL_M, 0x40); // ֻ������չ֡
	// ֻ��עEID3
	WriteRegCan(RXM0EIDH_M, 0x00);
	WriteRegCan(RXM0EIDL_M, 0x08);
	WriteRegCan(RXM0SIDH_M, 0x00);
	WriteRegCan(RXM0SIDL_M, 0x08);

	WriteRegCan(RXF0EIDH_M, 0x00);
	WriteRegCan(RXF0EIDL_M, 0x08);
	WriteRegCan(RXF0SIDH_M, 0x00);
	WriteRegCan(RXF0SIDL_M, 0x08);

	WriteRegCan(TXRTSCTRL_M, 0);
	WriteRegCan(BFPCTRL_M, 0); //BFPCTRL_RXnBF ���ſ��ƼĴ�����״̬�Ĵ���
	WriteRegCan(CANCTRL_M, NORMAL_MODE_M); //����ģʽ
}



