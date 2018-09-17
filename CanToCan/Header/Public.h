/* 
 * File:   Public.h
 * Author: dengcongyang
 *
 * Created on 2017��3��2��, ����3:43
 */

#ifndef PUBLIC_H
#define	PUBLIC_H


typedef unsigned char u8;
typedef unsigned int u16;
typedef unsigned long u32;
typedef volatile unsigned char vu8;
typedef volatile unsigned int vu16;

// RB4 �ź�����  �ߵ�ƽ�����豸ͣ   �͵�ƽ�����豸��
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

#define LocalSensorInfoCnt              12 //���ش�������ʼ����Ϣ�洢�ֽ���
#define RemoteSensorInfoCnt             11 //��ش�������ʼ����Ϣ�洢�ֽ���
#define BreakerInfoCnt                  86 //�ϵ�����ʼ����Ϣ�洢�ֽ���     
#define MaxLocalSensorCnt               16 //���ش������������
#define MaxRemoteSensorCnt              24 //���ضϵ���������ش������������
#define MaxBreakerCnt                    5 //���ضϵ����������

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
 *     1:������� 0:���������  1:������ 0:ģ����                                                  1:3.0�ϵ� 0:3.0���ϵ�        1:����  0:����
 * 0:�����������Ͷ���  1:���������Ͷ���
 * CtrFlag:
 * D4:���߶ϵ�  D1:3.0�ϵ�  D0:���޶ϵ�
 */
typedef struct {
    u8 Crc; //������Crc
    u8 SensorFlag; //
    u16 UpWarn;
    u16 UpDuanDian; //���޶ϵ�ֵ
    u16 UpFuDian; //���޻ָ�ֵ
    u16 DownWarn;
    u16 DownDuanDian; //���޶ϵ�ֵ
    u16 DownFuDian; //���޻ָ�ֵ
    
    u8 Addr;
    u8 Name; //����������
    u8 CtrFlag;
    u8 Delay;
   
    u16 OffTimeout; //���������߳�ʱʱ��
    u16 CurValue; //��ǰ���������ֵ
    u16 Tick; //���������߼��ʹ��
    u16 UpLoadTick; //��������ʱ�ϱ�ʱ��
} _LocalSensor;

typedef struct {
    u8 Addr; //��������ַ
    u16 UpDuanDian; //���޶ϵ�ֵ
    u16 UpFuDian; //���޻ָ�ֵ
    u16 DownDuanDian; //���޶ϵ�ֵ
    u16 DownFuDian; //���޻ָ�ֵ
    
    u8 CtrFlag;
    u16 Tick;
    u16 CurValue;
} _RemoteSensor;

/*Trigger:
 *               D7                 D6            D5          D4          D3          D2          D1         D0        
 *     1:������� 0:���������    0̬����      1̬����     ���߿���    ���ޱ���    ���޶ϵ�    ���ޱ���   ���޶ϵ�
 * 0:�����������Ͷ���  1:���������Ͷ���
 */
typedef struct {
    u8 Addr; //�ϵ�����ַ
    u8 Crc;
    u8 ForceControlFlag; //�ϵ����ֶ����Ʊ��
    u8 ForceControlPort; //�ϵ����ֶ����ƶ˿�
    u8 CrossControlFlag; //�ϵ���������Ʊ��
    u8 CrossControlPort; //�ϵ���������ƶ˿�

    u8 LocalTriggers[MaxLocalLinkCnt]; //��ǰ�ϵ����������ش����������ϵ���
    u8 LocalTriggerAddrs[MaxLocalLinkCnt]; //��ǰ�ϵ����������ش�������ַ

    u8 RemoteTriggers[MaxRemoteLinkCnt]; //��ǰ�ϵ���������ش����������ϵ���
    u8 RemoteTriggerAddrs[MaxRemoteLinkCnt]; //��ǰ�ϵ���������ش�������ַ

    u8 RelevanceLocalSensorCnt;
    u8 RelevanceRemoteSensorCnt;
    
    u8 Break3_0Addrs[4];
    u8 Break3_0Cnt;
    u16 Tick; //���������߼��ʹ��
    u16 UpLoadTick;
    u8 ActCnt;
    u8 Flag; //�ϵ�����ǰ״̬ D0:0---��ͨ 1----�Ͽ�  D1: 0---����  1---����

    u8 TriggerAddr; //���һ�δ����ϵ����ϵ�Ĵ�������ַ
    u16 ActTick;
    u16 CurValue;
} _Breaker;

/*
 * Addr ת�����ַ
 * AddrOffset:ת����ҽӴ�������ַƫ����
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

