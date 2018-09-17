#ifndef CPU_H
#define CPU_H

#include "Public.h"

// RB5---运行指示灯   低电平点亮  高电平熄灭
#define RunLedOff      LATB |=  0x20
#define RunLedChange   LATB ^=  0x20
#define RunLedOn       LATB &= ~0x20      

#define VolInputSample       10

typedef struct
{
    u8 Buf[7];
    u8 HexTime[7];
}CTime;

void CpuInit(void);
void Timer0Init(void);
void Timer2Init(void);
void VolSample(void);
void ReadConfig(void);
void ReadAddr(void);
void IoInit(void);
void AdInit(void);
u16 GetVolInputValue(u8 ch);
void VolSample(void);
//u8 ReadEEprom(u16 addr);
//void WriteEEprom(u16 addr,u8 val);
//void WriteBurstEEprom(u16 addr,u8 *buf,u16 len);
void WriteLocalSenserConfig(_LocalSensor Sensor);
void ReadAllLocalSensorConfig(void);
void EraseLocalSenser(u8 Addr);

void ReadAllRemoteSensor(void);
void ReadRemoteSensor(u8 Index);
void WriteRemoteSensor(_RemoteSensor* RemoteSensor);
void EraseRemoteSensor(u8 Addr);

void WriteBreaker(_Breaker Breaker);
void EraseBreaker(u8 Addr);
void ReadAllBreaker(void);
void TimePro(void);
void TimeChange(void);

void WriteBurstEEprom(u16 addr,u8 *buf,u16 len);
void ReadBurstEEprom(u16 addr,u8 *buf,u16 len);
#endif


