#ifndef __PROSWITCH_H
#define __PROSWITCH_H
#include "Public.h"
#include "CanData.h"
/*数据方向*/
#define DIR_UP                                                 0x01
#define DIR_DOWN                                               0x00
/*北京院指令类型*/
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
/*北京院传感器类型*/
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

/*菲莫传感器类型*/
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
/*控制类型*/
#define NCTR                                                   0x00
#define CTR                                                    0x01
/*命令码*/
#define INITIATIVEUPLOAD                                       0x01 //菲莫协议主动上报类型码
#define SWITCHUPLOAD                                           0x03 //菲莫协议传感器上传状态类型码
#define BREAKERUPLOAD                                          0x09 //菲莫协议断电器上传状态类型码
#define BREAKERLOGLOAD                                         0x0A //菲莫协议断电器上传状态类型码
#define POWERUPLOAD                                            0x06
/*断电器状态*/
#define NOBREAK                                                0x00 //复电
#define BREAKED                                                0x01 //断电
/*上报数据小数点位数*/
#define POINTNUM_0                                             0x00;//无小数点
#define POINTNUM_1                                             0x01;//一个小数点
#define POINTNUM_2                                             0x02;//两个小数点
#define POINTNUM_3                                             0x03;//三个小数点
/*上报数据类型*/
#define DATATYPE_NORMAL                                        0x00//正常数据
#define DATATYPE_PREHOT                                        0x01//预热数据
#define DATATYPE_DEMARCATE                                     0x02//标定数据
#define DATATYPE_HISTORY                                       0x03//历史数据

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
