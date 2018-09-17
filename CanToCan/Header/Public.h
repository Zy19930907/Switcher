/* 
 * File:   Public.h
 * Author: dengcongyang
 *
 * Created on 2017年3月2日, 下午3:43
 */

#ifndef PUBLIC_H
#define	PUBLIC_H


typedef unsigned char u8;
typedef unsigned int u16;
typedef unsigned long u32;
typedef volatile unsigned char vu8;
typedef volatile unsigned int vu16;

// RB4 信号输入  高电平代表设备停   低电平代表设备开
#define  SwitchInput    (PORTB & 0x10)
#define KaiGuanLiang      0x00
#define LianXuLiang       0x01

#ifdef FREQ_VALUE
#define ConvertAttri      0x21
#elif  SWITCH_VALUE
#define ConvertAttri      0x22
#endif

#define SoftVerb                        11

#define ConverterAddr                   3
#define ConvertNameAddr                 6
#define ResetTimesAddr                  9
#define FactoryTimeAddr                 12
#define AutoSendTimeAddr                18

#define LocalSensorInfoCnt              12 //本地传感器初始化信息存储字节数
#define RemoteSensorInfoCnt             11 //异地传感器初始化信息存储字节数
#define BreakerInfoCnt                  86 //断电器初始化信息存储字节数     
#define MaxLocalSensorCnt               16 //本地传感器最大数量
#define MaxRemoteSensorCnt              24 //本地断电器关联异地传感器最大数量
#define MaxBreakerCnt                    5 //本地断电器最大数量

#define LocalSensorInfoAddr              0
#define BreakerInfoStartAddr            (LocalSensorInfoCnt*MaxLocalSensorCnt)
#define RemoteSensorInfoStartAddr       ((LocalSensorInfoCnt*MaxLocalSensorCnt)+(MaxBreakerCnt*BreakerInfoCnt))

#define MaxLocalLinkCnt                 15
#define MaxRemoteLinkCnt                24

#define ENUPDUANDIAN                    0x01
#define DISUPDUANDIAN                   ~0x01
#define ENOFFLINEDUANDIAN               0x10
#define DISOFFLINEDUANDIAN              ~0x10

/*SensorFlag:
 *               D7                     D6            D5          D4          D3          D2                 D1                       D0        
 *     1:参与控制 0:不参与控制  1:开关量 0:模拟量                                                  1:3.0断电 0:3.0不断电        1:断线  0:断线
 * 0:不关联该类型动作  1:关联该类型动作
 * CtrFlag:
 * D4:断线断电  D1:3.0断电  D0:超限断电
 */
typedef struct {
    u8 Crc; //传感器Crc
    u8 SensorFlag; //
    u16 UpWarn;
    u16 UpDuanDian; //上限断电值
    u16 UpFuDian; //上限恢复值
    u16 DownWarn;
    u16 DownDuanDian; //下限断电值
    u16 DownFuDian; //下限恢复值
    
    u8 Addr;
    u8 Name; //传感器类型
    u8 CtrFlag;
    u8 Delay;
   
    u16 OffTimeout; //传感器断线超时时间
    u16 CurValue; //当前传感器监测值
    u16 Tick; //传感器掉线检测使用
    u16 UpLoadTick; //传感器定时上报时钟
} _LocalSensor;

typedef struct {
    u8 Addr; //传感器地址
    u16 UpDuanDian; //上限断电值
    u16 UpFuDian; //上限恢复值
    u16 DownDuanDian; //下限断电值
    u16 DownFuDian; //下限恢复值
    
    u8 CtrFlag;
    u16 Tick;
    u16 CurValue;
} _RemoteSensor;

/*Trigger:
 *               D7                 D6            D5          D4          D3          D2          D1         D0        
 *     1:参与控制 0:不参与控制    0态报警      1态报警     断线控制    下限报警    下限断电    上限报警   上限断电
 * 0:不关联该类型动作  1:关联该类型动作
 */
typedef struct {
    u8 Addr; //断电器地址
    u8 Crc;
    u8 ForceControlFlag; //断电器手动控制标记
    u8 ForceControlPort; //断电器手动控制端口
    u8 CrossControlFlag; //断电器交叉控制标记
    u8 CrossControlPort; //断电器交叉控制端口

    u8 LocalTriggers[MaxLocalLinkCnt]; //当前断电器关联本地传感器触发断电标记
    u8 LocalTriggerAddrs[MaxLocalLinkCnt]; //当前断电器关联本地传感器地址

    u8 RemoteTriggers[MaxRemoteLinkCnt]; //当前断电器关联异地传感器触发断电标记
    u8 RemoteTriggerAddrs[MaxRemoteLinkCnt]; //当前断电器关联异地传感器地址

    u8 RelevanceLocalSensorCnt;
    u8 RelevanceRemoteSensorCnt;
    
    u8 Break3_0Addrs[4];
    u8 Break3_0Cnt;
    u16 Tick; //传感器掉线检测使用
    u16 UpLoadTick;
    u8 ActCnt;
    u8 Flag; //断电器当前状态 D0:0---接通 1----断开  D1: 0---在线  1---断线

    u8 TriggerAddr; //最近一次触发断电器断电的传感器地址
    u16 ActTick;
    u16 CurValue;
} _Breaker;

/*
 * Addr 转换板地址
 * AddrOffset:转换板挂接传感器地址偏移量
 */
typedef struct {
    u8 Addr;
    u8 AddrOffset;
    u8 Delay;
    u8 Vol;
    u8 UploadCnt;
    u8 CrcSendCnt;
    u8 InitDelay;
    u8 UploadFlag;
    u8 Get3_0;

    u16 OfflineLianDongFlag;
    u16 TriggerLianDongFlag;
    u16 UpLoadTime;
    u16 OnlineFlag;
} CSys;

typedef enum {
    LedRunLevel,
    VoltageSample,
    CanUpLevel,
    CanDownLevel,
    SyncClkLevel,
    OnlineCheckLevel,
    CheckDuandianLevel,
    TimeLevel,
    PowerUploadLevel,
    UploadSwitcherInfo,
//    TestLevel,
} CRunLevel;

u16 MsTickDiff(u16 tick);
u16 SecTickDiff(u16 tick);
u8 CRC16Check(u8 *buf, u8 len);
void CRC16(u8 value);
void delay_us(u16 time);
u16 CalCrc16(u8 *buf, u8 len);
u16 Us100TickDiff(u16 tick);
u16 SecTickDiff(u16 tick);
void EarseBuf(u8 *buf, u16 len);
void BufCopy(u8 *s, u8 *d, u16 len);
u16 CalCrcInit(u8 *buf, u16 len, u8 flag);

_Breaker* GetBreaker(u8 addr);
_Breaker* FilterBreaker(u8 addr);
_Breaker* GetBreakerByIndex(u8 index);
u8 GetBreakerIndex(u8 addr);


_RemoteSensor* FilterRemoteSensor(u8 addr);

u8 GetRemoteSensorIndex(u8 addr);
_RemoteSensor* GetRemoteSensorByIndex(u8 index);
_RemoteSensor* GetRemoteSensor(u8 addr);

void CheckRemoteSensor(void);

u16 GetChuShu(u8 flag);
#endif	/* PUBLIC_H */

