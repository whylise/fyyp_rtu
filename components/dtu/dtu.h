/*ռ�õĴ洢�ռ�
**eeprom		:0--1199
*/



#ifndef __DTU_H__
#define __DTU_H__



#include "rtthread.h"
#include "user_priority.h"



//ģ��ѡ��ֻ��ѡ��һ��
//#define DTU_MODULE_SIM800C
#define DTU_MODULE_EC20
//#define DTU_MODULE_HT7710

//smsģʽ
//#define DTU_SMS_MODE_TEXT
#define DTU_SMS_MODE_PDU

//��������ַ
#define DTU_BASE_ADDR_PARAM				0

//������Ϣ
#define DTU_NUM_RECV_DATA				8							//������������
#define DTU_NUM_IP_CH					5							//ipͨ������
#define DTU_NUM_SMS_CH					5							//smsͨ������
#define DTU_BYTES_IP_ADDR				29							//ip��ַ�ռ�
#define DTU_BYTES_SMS_ADDR				19							//sms��ַ�ռ�
#define DTU_BYTES_APN					14							//�����ռ�
#define DTU_BYTES_HEART					100							//�������ռ�
#define DTU_BYTES_RECV_BUF				1000						//���ݽ��ջ�����
#define DTU_BOOT_TIMES_MAX				2							//boot��������
#define DTU_TIME_IP_IDLE_L				600							//����ʱ�䳤����
#define DTU_TIME_IP_IDLE_S				10							//����ʱ��̣���
#define DTU_TIME_SMS_IDLE_L				600							//����ʱ�䳤����
#define DTU_TIME_SMS_IDLE_S				10							//����ʱ��̣���
#define DTU_NUM_SUPER_PHONE				30							//Ȩ�޺�������
#define DTU_NUM_PREVILIGE_PHONE			10							//Ȩ�޺���������Ȩ���������
#define DTU_TIME_TELEPHONE_MAX			600							//��绰ʱ�䣬��
#define DTU_TIME_TTS_MAX				15							//�ttsʱ�䣬��
#define DTU_TIME_DIAL_PHONE				20							//����绰ʱ�䣬��
#define DTU_RECV_BUF_TIMEOUT			100							//���ջ�������ʱʱ�䣬����
#define DTU_NUM_AT_DATA					2
#define DTU_NUM_HTTP_DATA				2							//httpЭ�����ݽ�������

//����
#define DTU_STK_TIME_TICK				640							//��ʱ����ջ
#define DTU_STK_TELEPHONE_HANDLER		640							//�绰��������ջ
#define DTU_STK_RECV_BUF_HANDLER		512							//���մ�������ջ
#define DTU_STK_DIAL_PHONE				640							//����绰����
#define DTU_STK_AT_DATA_HANDLER			512
#ifndef DTU_PRIO_TIME_TICK
#define DTU_PRIO_TIME_TICK				30
#endif
#ifndef DTU_PRIO_TELEPHONE_HANDLER
#define DTU_PRIO_TELEPHONE_HANDLER		2
#endif
#ifndef DTU_PRIO_RECV_BUF_HANDLER
#define DTU_PRIO_RECV_BUF_HANDLER		2
#endif
#ifndef DTU_PRIO_DIAL_PHONE
#define DTU_PRIO_DIAL_PHONE				4
#endif
#ifndef DTU_PRIO_AT_DATA_HANDLER
#define DTU_PRIO_AT_DATA_HANDLER		3
#endif

//����ģʽ
#define DTU_RUN_MODE_PWR				0							//�ϵ�ģʽ
#define DTU_RUN_MODE_OFFLINE			1							//����ģʽ
#define DTU_RUN_MODE_ONLINE				2							//����ģʽ
#define DTU_NUM_RUN_MODE				3

//�ŵ�����
#define DTU_COMM_TYPE_SMS				0							//sms
#define DTU_COMM_TYPE_IP				1							//ip

//������������
#define DTU_PACKET_TYPE_DATA			0							//���ݰ�
#define DTU_PACKET_TYPE_HEART			1							//������

//״̬
#define DTU_STA_PWR						0x01						//����״̬
#define DTU_STA_DTU						0x02						//����״̬
#define DTU_STA_SMS						0x04						//����״̬
#define DTU_STA_IP						0x08						//����״̬
#define DTU_STA_HW_OPEN					0x10						//Ӳ����״̬

//�����¼���־
#define DTU_PARAM_ALL					0xffffffff
#define DTU_PARAM_IP_ADDR				0x01
#define DTU_PARAM_SMS_ADDR				0x100
#define DTU_PARAM_IP_TYPE				0x10000
#define DTU_PARAM_HEART_PERIOD			0x20000
#define DTU_PARAM_RUN_MODE				0x40000
#define DTU_PARAM_SUPER_PHONE			0x80000
#define DTU_PARAM_IP_CH					0x100000
#define DTU_PARAM_SMS_CH				0x200000
#define DTU_PARAM_APN					0x400000

//�¼�m_dtu_event_module_ex
#define DTU_EVENT_HANG_UP_ME			0x01
#define DTU_EVENT_HANG_UP_SHE			0x02
#define DTU_EVENT_ATA_OK				0x04
#define DTU_EVENT_ATA_ERROR				0x08
#define DTU_EVENT_TTS_STOP_ME			0x10
#define DTU_EVENT_TTS_STOP_SHE			0x20
#define DTU_EVENT_TTS_START_OK			0x40
#define DTU_EVENT_TTS_START_ERROR		0x80
#define DTU_EVENT_RECV_BUF				0x100
#define DTU_EVENT_RECV_BUF_FULL			0x200
#define DTU_EVENT_NEED_CONN_CH			0x1000000
#define DTU_EVENT_SEND_IMMEDIATELY		0x10000

//ttsģʽ
#define DTU_TTS_DATA_TYPE_UCS2			1
#define DTU_TTS_DATA_TYPE_GBK			2



//�ź�ǿ�ȸ��º���ָ��
typedef void (*DTU_FUN_CSQ_UPDATE)(rt_uint8_t *pdata, rt_uint16_t data_len, rt_uint8_t csq_value);

//���������뺯��ָ��
typedef rt_uint16_t (*DTU_FUN_HEART_ENCODER)(rt_uint8_t *pdata, rt_uint16_t data_len, rt_uint8_t ch);

//�������ݽṹ
typedef struct dtu_recv_data
{
	rt_uint8_t					comm_type;							//�ŵ�����
	rt_uint8_t					ch;									//ͨ��
	rt_uint16_t					data_len;							//���ݳ���
	rt_uint8_t					*pdata;								//����
} DTU_RECV_DATA;

//ip��ַ���ݽṹ
typedef struct dtu_ip_addr
{
	rt_uint8_t					addr_data[DTU_BYTES_IP_ADDR];		//ip��ַ����ʽ����Ϊ1.85.44.234:9602
	rt_uint8_t					addr_len;							//ip��ַ����
} DTU_IP_ADDR;

//sms��ַ���ݽṹ
typedef struct dtu_sms_addr
{
	rt_uint8_t					addr_data[DTU_BYTES_SMS_ADDR];		//sms��ַ
	rt_uint8_t					addr_len;							//sms��ַ����
} DTU_SMS_ADDR;

//apn���ݽṹ
typedef struct dtu_apn
{
	rt_uint8_t					apn_data[DTU_BYTES_APN];			//apn����
	rt_uint8_t					apn_len;							//apn����
} DTU_APN;

//tts����
typedef struct dtu_tts_data
{
	rt_uint8_t					data_type;							//��������gbk����ucs2
	rt_uint16_t					data_len;							//���ݳ���
	rt_uint8_t					*pdata;								//����
	void (*fun_tts_over)(void);										//����֪ͨ
} DTU_TTS_DATA;

//������,��Աλ�ò���������仯
typedef struct dtu_param_set
{
	DTU_IP_ADDR					ip_addr[DTU_NUM_IP_CH];
	DTU_SMS_ADDR				sms_addr[DTU_NUM_SMS_CH];
	rt_uint8_t					ip_ch;
	rt_uint8_t					sms_ch;
	rt_uint8_t					ip_type;
	rt_uint8_t					run_mode[DTU_NUM_IP_CH];
	rt_uint16_t					heart_period[DTU_NUM_IP_CH];
	DTU_SMS_ADDR				super_phone[DTU_NUM_SUPER_PHONE];
	DTU_APN						apn;
} DTU_PARAM_SET;



//������д
void dtu_set_ip_addr(DTU_IP_ADDR const *ip_addr_ptr, rt_uint8_t ch);
void dtu_get_ip_addr(DTU_IP_ADDR *ip_addr_ptr, rt_uint8_t ch);

void dtu_set_sms_addr(DTU_SMS_ADDR const *sms_addr_ptr, rt_uint8_t ch);
void dtu_get_sms_addr(DTU_SMS_ADDR *sms_addr_ptr, rt_uint8_t ch);

void dtu_set_ip_type(rt_uint8_t ip_type, rt_uint8_t ch);
rt_uint8_t dtu_get_ip_type(rt_uint8_t ch);

void dtu_set_heart_period(rt_uint16_t heart_period, rt_uint8_t ch);
rt_uint16_t dtu_get_heart_period(rt_uint8_t ch);

void dtu_set_run_mode(rt_uint8_t run_mode, rt_uint8_t ch);
rt_uint8_t dtu_get_run_mode(rt_uint8_t ch);

void dtu_set_super_phone(DTU_SMS_ADDR const *sms_addr_ptr, rt_uint8_t ch);
void dtu_get_super_phone(DTU_SMS_ADDR *sms_addr_ptr, rt_uint8_t ch);
void dtu_del_super_phone(DTU_SMS_ADDR const *sms_addr_ptr);
rt_uint8_t dtu_add_super_phone(DTU_SMS_ADDR const *sms_addr_ptr, rt_uint8_t previlige);
rt_uint8_t dtu_is_previlige_phone(rt_uint8_t ch);

void dtu_set_ip_ch(rt_uint8_t sta, rt_uint8_t ch);
rt_uint8_t dtu_get_ip_ch(rt_uint8_t ch);

void dtu_set_sms_ch(rt_uint8_t sta, rt_uint8_t ch);
rt_uint8_t dtu_get_sms_ch(rt_uint8_t ch);

void dtu_set_apn(DTU_APN const *apn_ptr);
void dtu_get_apn(DTU_APN *apn_ptr);

//��������
void dtu_param_restore(void);

//��������
rt_uint8_t dtu_send_data(rt_uint8_t comm_type, rt_uint8_t ch, rt_uint8_t const *pdata, rt_uint16_t data_len, DTU_FUN_CSQ_UPDATE fun_csq_update);

//��������
DTU_RECV_DATA *dtu_recv_data_pend(void);

DTU_RECV_DATA *dtu_http_data_pend(rt_int32_t timeout);

void dtu_http_data_clear(void);

//��������
void dtu_hold_enable(rt_uint8_t comm_type, rt_uint8_t ch, rt_uint8_t en);

//��������
void dtu_fun_mount(DTU_FUN_HEART_ENCODER fun_heart_encoder);

rt_uint8_t dtu_get_conn_sta(rt_uint8_t ch);

//����绰
rt_uint8_t dtu_dial_phone(DTU_SMS_ADDR *sms_addr_ptr);

//��������
void dtu_ip_send_immediately(rt_uint8_t ch);

rt_uint8_t dtu_get_csq_value(void);

rt_uint8_t dtu_http_send(DTU_IP_ADDR const *ip_addr_ptr, rt_uint8_t const *pdata, rt_uint16_t data_len);



#endif

