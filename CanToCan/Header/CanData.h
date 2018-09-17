#ifndef CANDATA_H
#define	CANDATA_H


#include "Public.h"

#define ACK   0x10
#define FRAMTYPE_SINGLE         0x00
#define FRAMTYPE_FIRST          0x04
#define FRAMTYPE_MIDDLE         0x08
#define FRAMTYPE_END            0x0C
/*传感器类型*/
#define SENSERTYPE_SWITCH       0x80 //开关量传感器
#define SENSERTYPE_ANALOG       0x00 //模拟量传感器

/*CAN指令类型*/
#define CMD_INIT                0x51 //配置初始化数据
#define CMD_CLRINIT             0x52 //清除初始化数据
#define CMD_PKPOWERUPLOAD       0x0E //北京院电源数据上传 
#define CMD_GETSENSERINFO       0x11 //查询传感器参数
#define CMD_GETRELEVANCEADDR    0x19 //查询断电器关联传感器地址（CAN转换板下目前只有断电器具有关联信息）
#define CMD_GETRELEVANCEFLAG    0x1C //查询断电器关联传感器控制类型
#define CMD_GETUPWORNVALUE      0x14 //查询传感器上限报警值
#define CMD_GETDOWNWORNVALUE    0x15 //查询传感器下限报警值
#define CMD_GETTIME             0x18 //查询传感器当前时间
#define CMD_GETSOFTVERB         0x22 //查询转换板软件版本
#define CMD_SYNCTIME            0x23 //同步传感器时间
#define CMD_RESET               0x24 //设备重启指令（CAN转换板下任意传感器接到此命令后重启转换板）
#define CMD_GETUPLOADTIME       0x33 //获取传感器主动上报时间
#define CMD_SETUPLOADTIME       0x34 //获取传感器主动上报时间
#define CMD_GETOFFLINETIME      0x3B //查询传感器断线判定时间
#define CMD_SETOFFLINETIME      0x3C //设置传感器断线判定时间
#define CMD_FORCECONTROL        0x40 //手动控制指令
#define CMD_CROSSCONTROL        0x41 //交叉控制
#define CMD_SWITCHUPLOAD        0x55 //转换板上传传感器实时数据，4个字节代表1个传感器信息
#define CMD_SWITCTRINFO         0x56 //转换板上传当前状态，每个字节代表2个传感器
#define CMD_Break3_0CONFIG      0x57 //转换板上传传感器数据

typedef enum
{
    INITCAN,
    NORMAL,
    TX,
    TXING,
}CCanStatus;

typedef struct
{
    CCanStatus Status;
    u8  Buf[8];
    u8  Len;
    u32 ID;
}CCan;

typedef struct
{
    u8 R;
    u8 W;
    u8 Addr;
    u8 InitValue[100];
}_InitInfo;

void CanUpReceiveFunc(void);
void CanDownReceiveFunc(void);
void UpdateInfo(void);


#endif	/* CANDATA_H */

