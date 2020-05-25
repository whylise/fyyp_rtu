/*ռ�õĴ洢�ռ�
**EEPROM		:1200--1213
*/



#ifndef __SL427_H__
#define __SL427_H__



#include "rtthread.h"
#include "ptcl.h"
#include "user_priority.h"



//����ַ
#define SL427_PARAM_BASE_ADDR			1200

//������ַ(14�ֽ�)
#define SL427_POS_RTU_ADDR				(SL427_PARAM_BASE_ADDR + 0)		//11�ֽڣ��豸��ַ
#define SL427_POS_REPORT_PERIOD			(SL427_PARAM_BASE_ADDR + 11)	// 2�ֽڣ��ϱ����
#define SL427_POS_SMS_WARN_RECEIPT		(SL427_PARAM_BASE_ADDR + 13)	// 1�ֽڣ�����Ԥ����ִ

//������Ϣ
#define SL427_BYTES_SMS_FRAME_HEAD		6			//smsָ��֡ͷ��󳤶�
#define SL427_BYTES_GPRS_FRAME_HEAD		18			//gprsָ��֡ͷ����
#define SL427_BYTES_GPRS_FRAME_END		2			//gprsָ��֡β����
#define SL427_FRAME_HEAD_1				0x68
#define SL427_FRAME_HEAD_2				0
#define SL427_BYTES_RTU_ADDR			11
#define SL427_BYTES_MAIN_SOFT_EDITION	16
#define SL427_BYTES_PWR_SOFT_EDITION	16
#define SL427_POS_CSQ_IN_REPORT			69
#define SL427_BYTES_WARN_INFO_MAX		500
#define SL427_BYTES_REPORT_DATA			100

//����
#define SL427_STK_REPORT				640
#ifndef SL427_PRIO_REPORT
#define SL427_PRIO_REPORT				26
#endif

//smsָ��
#define SL427_CMD_BJ					0			//���͸澯
#define SL427_CMD_JGLH					1			//�ӹ����
#define SL427_CMD_JSQH					2			//����Ȩ��
#define SL427_CMD_SGLH					3			//ɾ�������
#define SL427_CMD_SSQH					4			//ɾ����Ȩ��
#define SL427_CMD_SZDZ					5			//�����豸��ַ
#define SL427_CMD_SZSB					6			//�����ϱ����
#define SL427_CMD_SZHZ					7			//���û�ִ
#define SL427_CMD_SZCS					8			//���ø澯����
#define SL427_CMD_SZPL					9			//����fmƵ��
#define SL427_CMD_SZBF					10			//���ø澯���ȼ�
#define SL427_CMD_SZIP					11			//����ip
#define SL427_CMD_SZJJ					12			//���ý����澯����
#define SL427_CMD_CGLH					13			//��ѯ�����
#define SL427_CMD_CSQH					14			//��ѯ��Ȩ��
#define SL427_CMD_CXDZ					15			//��ѯ�豸��ַ
#define SL427_CMD_CXSB					16			//��ѯ�ϱ����
#define SL427_CMD_CXPL					17			//��ѯfmƵ��
#define SL427_CMD_CXBF					18			//��ѯ�澯���ȼ�
#define SL427_CMD_CXIP					19			//��ѯip
#define SL427_CMD_CXCS					20			//�澯����
#define SL427_NUM_CMD					21

//����
#define SL427_PARAM_RTU_ADDR			0x01
#define SL427_PARAM_REPORT_PERIOD		0x02
#define SL427_PARAM_SMS_WARN_RECEIPT	0x04
#define SL427_PARAM_ALL					0x07

//������
#define SL427_AFN_QUERY_SUPER_PHONE		0x01		//��ѯȫ���绰����
#define SL427_AFN_QUERY_SYS_PARAM		0x02		//��ѯϵͳ����
#define SL427_AFN_REQUEST_WARN			0x03		//���󱨾�
#define SL427_AFN_ADD_SUPER_PHONE		0x04		//���Ӻ���
#define SL427_AFN_DEL_SUPER_PHONE		0x05		//ɾ������
#define SL427_AFN_MODIFY_SUPER_PHONE	0x06		//�޸ĺ���
#define SL427_AFN_SET_SYS_PARAM			0x07		//����ϵͳ����
#define SL427_AFN_QUERY_TIME			0x08		//��ѯʱ��
#define SL427_AFN_SET_TIME				0x09		//����ʱ��
#define SL427_AFN_SET_PRESET_WARN_INFO	0x0a		//����Ԥ�ø澯��Ϣ
#define SL427_AFN_DIAL_PHONE			0x11		//����绰
#define SL427_AFN_HEART					0x1b		//����
#define SL427_AFN_AUTO_REPORT			0x1c		//�Ա�
#define SL427_AFN_QUERY_SCREEN_TXT		0x2a		//��ѯ��ʾ���ı�
#define SL427_AFN_REQUEST_WARN_EX		0x13
#define SL427_AFN_REQUEST_AD			0x53		//����֪ͨ
#define SL427_AFN_REQUEST_WEATHER		0x54		//����������Ϣ

//�����ı�����
#define SL427_WARN_DATA_TYPE_ASCII		4
#define SL427_WARN_DATA_TYPE_UNICODE	8

//������
#define SL427_ERR_AFN					3
#define SL427_ERR_DATA_VALUE			4
#define SL427_ERR_DATA_LEN				5
#define SL427_ERR_DEVICE				6
#define SL427_ERR_ID					7
#define SL427_ERR_WARN_DATA_TYPE		8
#define SL427_ERR_WARN_TOO_LONG			9
#define SL427_ERR_NONE					15

//���緽ʽ
#define SL427_PWR_TYPE_BATT				1
#define SL427_PWR_TYPE_ACDC				2

//������Ϣ
typedef struct sl427_warn_info
{
	rt_uint8_t			packet_total;
	rt_uint8_t			packet_cur;
	rt_uint8_t			data_type;
	rt_uint8_t			warn_times;
	rt_uint16_t			data_len;
	rt_uint8_t			*pdata;
} SL427_WARN_INFO;



//�������ö�ȡ
void sl427_get_rtu_addr(rt_uint8_t *rtu_addr_ptr);
void sl427_set_rtu_addr(rt_uint8_t const *rtu_addr_ptr, rt_uint8_t addr_len);

rt_uint16_t sl427_get_report_period(void);
void sl427_set_report_period(rt_uint16_t period);

rt_uint8_t sl427_get_sms_warn_receipt(void);
void sl427_set_sms_warn_receipt(rt_uint8_t en);

void sl427_param_restore(void);

//����
rt_uint16_t sl427_heart_encoder(rt_uint8_t *pdata, rt_uint16_t data_len, rt_uint8_t ch);

//smsЭ�����
rt_uint8_t sl427_sms_data_decoder(PTCL_REPORT_DATA *report_data_ptr, PTCL_RECV_DATA const *recv_data_ptr, PTCL_PARAM_INFO **param_info_ptr_ptr);
//gprsЭ�����
rt_uint8_t sl427_gprs_data_decoder(PTCL_REPORT_DATA *report_data_ptr, PTCL_RECV_DATA const *recv_data_ptr, PTCL_PARAM_INFO **param_info_ptr_ptr);

//gprsЭ�����֮lora�ŵ�
rt_uint8_t sl427_gprs_data_decoder_lora(PTCL_REPORT_DATA *report_data_ptr, PTCL_RECV_DATA const *recv_data_ptr);



#endif

