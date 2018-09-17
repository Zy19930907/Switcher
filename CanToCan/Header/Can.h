/* 
 * File:   Can.h
 * Author: dengcongyang
 *
 * Created on June 4, 2017, 11:37 AM
 */

#ifndef CAN_H
#define	CAN_H

#include "Public.h"
#include "CanData.h"
/*
  CANTX--------RB2
 *CANRX--------RB3
 */


#define CLR_CS     LATC &= ~0x04
#define SET_CS     LATC |=  0x04


void CanDownInit(void);
u8 ReadStatus(void);
void ModifyReg(u8 addr,u8 mask,u8 val);
void ReadBurstRegCan(u8 addr,u8 *buf,u8 len);
u8 ReadRegCan(u8 addr);
void WriteBurstRegCan(u8 addr, u8 *buf, u8 len);
void WriteRegCan(u8 addr, u8 value);
void MCP2515Reset(void);
u8 SendByte(u8 dat);

void CanUpInit(void);
void CanUpSendData(void);
void CanDownSend(CCan CanData);
void CanUpSend(CCan CanData);
/*
 工作模式定义
 */

#define NORMAL_MODE_M    0x00
#define SLEEP_MODE_M     0x20
#define LOOPBACK_MODE_M  0x40
#define LISTEN_MODE_M    0x60
#define CONFIG_MODE_M    0x80


/* Configuration Registers */
#define CANSTAT_M         0x0E
#define CANCTRL_M         0x0F
#define BFPCTRL_M         0x0C
#define TEC_M             0x1C
#define REC_M             0x1D
#define CNF3_M            0x28
#define CNF2_M            0x29
#define CNF1_M            0x2A
#define CANINTE_M         0x2B
#define CANINTF_M         0x2C
#define EFLG_M            0x2D
#define TXRTSCTRL_M       0x0D

/*  Recieve Filters */
#define RXF0SIDH_M        0x00
#define RXF0SIDL_M        0x01
#define RXF0EIDH_M        0x02
#define RXF0EIDL_M        0x03
#define RXF1SIDH_M        0x04
#define RXF1SIDL_M        0x05
#define RXF1EID8_M        0x06
#define RXF1EID0_M        0x07
#define RXF2SIDH_M        0x08
#define RXF2SIDL_M        0x09
#define RXF2EID8_M        0x0A
#define RXF2EID0_M        0x0B
#define RXF3SIDH_M        0x10
#define RXF3SIDL_M        0x11
#define RXF3EID8_M        0x12
#define RXF3EID0_M        0x13
#define RXF4SIDH_M        0x14
#define RXF4SIDL_M        0x15
#define RXF4EID8_M        0x16
#define RXF4EID0_M        0x17
#define RXF5SIDH_M        0x18
#define RXF5SIDL_M        0x19
#define RXF5EID8_M        0x1A
#define RXF5EID0_M        0x1B

/* Receive Masks */
#define RXM0SIDH_M        0x20
#define RXM0SIDL_M        0x21
#define RXM0EIDH_M        0x22
#define RXM0EIDL_M        0x23
#define RXM1SIDH_M        0x24
#define RXM1SIDL_M        0x25
#define RXM1EID8_M        0x26
#define RXM1EID0_M        0x27

/* Tx Buffer 0 */
#define TXB0CTRL_M        0x30
#define TXB0SIDH_M        0x31
#define TXB0SIDL_M        0x32
//#define TXB0EID8        0x33    //数据手册定义TXB0EID8，自我定义 TXB0EIDH
//#define TXB0EID0        0x34
#define TXB0EIDH_M        0x33
#define TXB0EIDL_M        0x34
#define TXB0DLC_M         0x35
#define TXB0D0_M          0x36
#define TXB0D1_M          0x37
#define TXB0D2_M          0x38
#define TXB0D3_M          0x39
#define TXB0D4_M          0x3A
#define TXB0D5_M          0x3B
#define TXB0D6_M          0x3C
#define TXB0D7_M          0x3D
                         
/* Tx Buffer 1 */
#define TXB1CTRL_M        0x40
#define TXB1SIDH_M        0x41
#define TXB1SIDL_M        0x42
#define TXB1EID8_M        0x43
#define TXB1EID0_M        0x44
#define TXB1DLC_M         0x45
#define TXB1D0_M          0x46
#define TXB1D1_M          0x47
#define TXB1D2_M          0x48
#define TXB1D3_M          0x49
#define TXB1D4_M          0x4A
#define TXB1D5_M          0x4B
#define TXB1D6_M          0x4C
#define TXB1D7_M          0x4D

/* Tx Buffer 2 */
#define TXB2CTRL_M        0x50
#define TXB2SIDH_M        0x51
#define TXB2SIDL_M        0x52
#define TXB2EID8_M        0x53
#define TXB2EID0_M        0x54
#define TXB2DLC_M         0x55
#define TXB2D0_M          0x56
#define TXB2D1_M          0x57
#define TXB2D2_M          0x58
#define TXB2D3_M          0x59
#define TXB2D4_M          0x5A
#define TXB2D5_M          0x5B
#define TXB2D6_M          0x5C
#define TXB2D7_M          0x5D
                         
/* Rx Buffer 0 */
#define RXB0CTRL_M        0x60
#define RXB0SIDH_M        0x61
#define RXB0SIDL_M        0x62
//#define RXB0EID8        0x63
//#define RXB0EID0        0x64
#define RXB0EIDH_M       0x63
#define RXB0EIDL_M        0x64
#define RXB0DLC_M         0x65
#define RXB0D0_M          0x66
#define RXB0D1_M          0x67
#define RXB0D2_M          0x68
#define RXB0D3_M          0x69
#define RXB0D4_M          0x6A
#define RXB0D5_M          0x6B
#define RXB0D6_M          0x6C
#define RXB0D7_M          0x6D
                         
/* Rx Buffer 1 */
#define RXB1CTRL_M        0x70
#define RXB1SIDH_M        0x71
#define RXB1SIDL_M        0x72
//#define RXB1EID8        0x73
//#define RXB1EID0        0x74
#define RXB1EIDH_M        0x73
#define RXB1EIDL_M        0x74
#define RXB1DLC_M         0x75
#define RXB1D0_M          0x76
#define RXB1D1_M          0x77
#define RXB1D2_M          0x78
#define RXB1D3_M          0x79
#define RXB1D4_M          0x7A
#define RXB1D5_M          0x7B
#define RXB1D6_M          0x7C
#define RXB1D7_M          0x7D
/*
//定义寄存器BFPCTRL位信息
#define B1BFS  5
#define B0BFS  4
#define B1BFE  3
#define B0BFE  2
#define B1BFM  1
#define B0BFM  0

//定义寄存器TXRTSCTRL位信息
#define B2RTS  5
#define B1RTS  4
#define B0RTS  3
#define B2RTSM  2
#define B1RTSM  1
#define B0RTSM  0

//定义寄存器CANSTAT位信息
#define OPMOD2  7
#define OPMOD1  6
#define OPMOD0  5
#define ICOD2  3
#define ICOD1  2
#define ICOD0  1


//定义寄存器CANCTRL位信息
#define REQOP2  7
#define REQOP1  6
#define REQOP0  5
#define ABAT  4
#define CLKEN  2
#define CLKPRE1  1
#define CLKPRE0  0


 //定义寄存器CNF3位信息
#define WAKFIL  6
#define PHSEG22  2
#define PHSEG21  1
#define PHSEG20  0


 //定义寄存器CNF2位信息
#define BTLMODE  7
//#define SAM   6
#define PHSEG12  5
#define PHSEG11  4
#define PHSEG10  3
#define PHSEG2  2
#define PHSEG1  1
#define PHSEG0  0


 //定义寄存器CNF1位信息
#define SJW1  7
#define SJW0  6
#define BRP5  5
#define BRP4  4
#define BRP3  3
#define BRP2  2
#define BRP1  1
#define BRP0  0


 //定义寄存器CANINTE位信息
#define MERRE  7
#define WAKIE  6
#define ERRIE  5
#define TX2IE  4
#define TX1IE  3
#define TX0IE  2
#define RX1IE  1
#define RX0IE  0


 //定义寄存器CANINTF位信息
#define MERRF  7
#define WAKIF  6
#define ERRIF  5
#define TX2IF  4
#define TX1IF  3
#define TX0IF  2
#define RX1IF  1
#define RX0IF  0


 //定义寄存器EFLG位信息
#define RX1OVR  7
#define RX0OVR  6
#define TXB0  5
#define TXEP  4
#define RXEP  3
#define TXWAR  2
#define RXWAR  1
#define EWARN  0


 //定义寄存器TXBnCTRL ( n = 0, 1, 2 )位信息
#define ABTF  6
#define MLOA  5
#define TXERR  4
#define TXREQ  3
#define TXP1  1
#define TXP0  0


 //定义寄存器RXB0CTRL位信息
#define RXM1  6
#define RXM0  5
#define RXRTR  3
#define BUKT  2
#define BUKT1  1
#define FILHIT0  0

//定义发送缓冲寄存器 TXBnSIDL ( n = 0, 1 )的位信息

#define EXIDE  3

//定义接受缓冲器1控制寄存器的位信息
#define FILHIT2  2
#define FILHIT1  1
*/
/**
 * 定义接收缓冲器n标准标示符低位 RXBnSIDL ( n = 0, 1 )的位信息
 *//*
#define SRR   4
#define IDE   3


// 定义接收缓冲器n数据长度码 RXBnDLC ( n = 0, 1 )的位信息

//#define RTR   6
#define DLC3  3
#define DLC2  2
#define DLC1  1
#define DLC0  0
*/

#define CAN_RESET_M       0xC0
#define CAN_WRITE_M       0x02
#define CAN_READ_M        0x03
#define CAN_RTS_M         0x80    // 请求发送
#define CAN_RTS_TXB0_M    0x81    // 请求发送缓冲区1
#define CAN_RTS_TXB1_M    0x82    // 请求发送缓冲区2
#define CAN_RTS_TXB2_M    0x84    // 请求发送缓冲区3
#define CAN_RD_STATUS_M   0xA0    // 读状态指令
#define CAN_BIT_MODIFY_M  0x05    // 寄存器位修改指令
#define CAN_RX_STATUS_M   0xB0    // 接收状态指令
#define CAN_RD_RX_BUFF_M  0x90    // 读接收缓冲区状态
#define CAN_LOAD_TX_M     0X40    // 装载发送缓冲区



#endif	/* CAN_H */

