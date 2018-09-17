#ifndef CANDATA_H
#define	CANDATA_H


#include "Public.h"

#define ACK   0x10
#define FRAMTYPE_SINGLE         0x00
#define FRAMTYPE_FIRST          0x04
#define FRAMTYPE_MIDDLE         0x08
#define FRAMTYPE_END            0x0C
/*����������*/
#define SENSERTYPE_SWITCH       0x80 //������������
#define SENSERTYPE_ANALOG       0x00 //ģ����������

/*CANָ������*/
#define CMD_INIT                0x51 //���ó�ʼ������
#define CMD_CLRINIT             0x52 //�����ʼ������
#define CMD_PKPOWERUPLOAD       0x0E //����Ժ��Դ�����ϴ� 
#define CMD_GETSENSERINFO       0x11 //��ѯ����������
#define CMD_GETRELEVANCEADDR    0x19 //��ѯ�ϵ���������������ַ��CANת������Ŀǰֻ�жϵ������й�����Ϣ��
#define CMD_GETRELEVANCEFLAG    0x1C //��ѯ�ϵ���������������������
#define CMD_GETUPWORNVALUE      0x14 //��ѯ���������ޱ���ֵ
#define CMD_GETDOWNWORNVALUE    0x15 //��ѯ���������ޱ���ֵ
#define CMD_GETTIME             0x18 //��ѯ��������ǰʱ��
#define CMD_GETSOFTVERB         0x22 //��ѯת��������汾
#define CMD_SYNCTIME            0x23 //ͬ��������ʱ��
#define CMD_RESET               0x24 //�豸����ָ�CANת���������⴫�����ӵ������������ת���壩
#define CMD_GETUPLOADTIME       0x33 //��ȡ�����������ϱ�ʱ��
#define CMD_SETUPLOADTIME       0x34 //��ȡ�����������ϱ�ʱ��
#define CMD_GETOFFLINETIME      0x3B //��ѯ�����������ж�ʱ��
#define CMD_SETOFFLINETIME      0x3C //���ô����������ж�ʱ��
#define CMD_FORCECONTROL        0x40 //�ֶ�����ָ��
#define CMD_CROSSCONTROL        0x41 //�������
#define CMD_SWITCHUPLOAD        0x55 //ת�����ϴ�������ʵʱ���ݣ�4���ֽڴ���1����������Ϣ
#define CMD_SWITCTRINFO         0x56 //ת�����ϴ���ǰ״̬��ÿ���ֽڴ���2��������
#define CMD_Break3_0CONFIG      0x57 //ת�����ϴ�����������

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

