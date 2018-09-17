#ifndef __PROSWITCH_H
#define __PROSWITCH_H
#include "Public.h"
#include "CanData.h"
/*���ݷ���*/
#define DIR_UP                                                 0x01
#define DIR_DOWN                                               0x00
/*����Ժָ������*/
#define CMD_BREAKPOWER                                         0x00
#define CMD_INITUPLOAD                                         0x01
#define CMD_TIMEUPLOAD                                         0x02
#define CMD_DOWNLOADWORINGVALUE                                0x03
#define CMD_SYNCCLOCK                                          0x04
#define CMD_GETSENSERHISTORYVALUE                              0x05
#define CMD_PREHEATVALUE                                       0x06
#define CMD_GETSENSERVALUE                                     0x07
#define CMD_DOWNLOADCHANGEVALUE                                0x08
#define CMD_SETGRADALARM                                       0x0B
/*����Ժ����������*/
#define CH4_4                                                  0x01
#define CH4_40                                                 0x02
#define CH4_100                                                0x03
#define CO_1000                                                0x05
#define PRESSURE                                               0x07
#define TEMPERATURE                                            0x09
#define DUST_1000                                              0x0A
#define CO2_5                                                  0x0B
#define O2_100                                                 0x0C
#define GKT                                                    0x0D
#define AIRDOOR                                                0x0E
#define POWERBREAKER                                           0x0F
#define WINDSPEED                                              0x10
#define SMOG                                                   0x11
#define HAIRDRYER                                              0x12
#define DUST_500                                               0x19
#define POWER                                                  0x28
#define CH4LASER_100                                           0x2B

/*��Ī����������*/
#define FM_CH4LOW                                              0x00
#define FM_CH4HL                                               0x01
#define FM_CH4INFRARED                                         0x02
#define FM_CH4LASER                                            0x03
#define FM_CO                                                  0x04
#define FM_O2                                                  0x05
#define FM_PRESSURE                                            0x06
#define FM_TEMPERATURE                                         0x07
#define FM_WINDSPEED                                           0x08
#define FM_CO2                                                 0x09
#define FM_H2S                                                 0x0A
#define FM_DUST                                                0x0C
#define FM_SMOG                                                0x0E
#define FM_AIRDOOR                                             0x0F
#define FM_POWERBREAKER                                        0x1F
#define FM_GKT                                                 0x22
#define FM_POWER                                               0x23
#define FM_HAIRDRYER                                           0x2A
#define FM_PKPOWER                                             0x39
#define FM_SWITCHER                                            0x37
/*��������*/
#define NCTR                                                   0x00
#define CTR                                                    0x01
/*������*/
#define INITIATIVEUPLOAD                                       0x01 //��ĪЭ�������ϱ�������
#define SWITCHUPLOAD                                           0x03 //��ĪЭ�鴫�����ϴ�״̬������
#define BREAKERUPLOAD                                          0x09 //��ĪЭ��ϵ����ϴ�״̬������
#define BREAKERLOGLOAD                                         0x0A //��ĪЭ��ϵ����ϴ�״̬������
#define POWERUPLOAD                                            0x06
/*�ϵ���״̬*/
#define NOBREAK                                                0x00 //����
#define BREAKED                                                0x01 //�ϵ�
/*�ϱ�����С����λ��*/
#define POINTNUM_0                                             0x00;//��С����
#define POINTNUM_1                                             0x01;//һ��С����
#define POINTNUM_2                                             0x02;//����С����
#define POINTNUM_3                                             0x03;//����С����
/*�ϱ���������*/
#define DATATYPE_NORMAL                                        0x00//��������
#define DATATYPE_PREHOT                                        0x01//Ԥ������
#define DATATYPE_DEMARCATE                                     0x02//�궨����
#define DATATYPE_HISTORY                                       0x03//��ʷ����

#define CANIDSWITCH_B2F                                        0x00
#define CANIDSWITCH_F2B                                        0x01
typedef struct
{
    u8 UploadCnt;
    u8 Tick;
}_PowerInfoUploader;
void CanProSwitch(CCan Can_Peking);
void SyncClk(void);
void SetWornValue(u8 Addr);
void RePower(u8 Addr);
void DuanDianPro(void);
u32 MakeFeimoCanId(u8 FramCnt, u8 Cmd, u8 CtrFlag, u8 Dir, u8 Type, u8 Addr);
void UploadPowerData(void);
void UpLoadSensorData(void);
void UpLoadControlInfo(void);
void Get3_0Config(void);
void SensorOnlineCheck(void);

#endif
