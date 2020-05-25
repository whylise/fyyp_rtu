/*ռ�õĴ洢�ռ�
**EEPROM		:1700--1806
**flash			:524588--524787
*/



#ifndef __SL651_H__
#define __SL651_H__



#include "rtthread.h"
#include "drv_rtcee.h"
#include "ptcl.h"



//��������ַ
#define SL651_PARAM_BASE_ADDR				1700
#define SL651_WARN_INFO_ADDR				524588

//�����洢λ��(107bytes)
#define SL651_POS_RTU_ADDR					(SL651_PARAM_BASE_ADDR + 0)		//  5���ֽڣ���ַ
#define SL651_POS_PW						(SL651_PARAM_BASE_ADDR + 5)		//  2���ֽڣ�����
#define SL651_POS_SN						(SL651_PARAM_BASE_ADDR + 7)		//  2���ֽڣ���ˮ��
#define SL651_POS_RTU_TYPE					(SL651_PARAM_BASE_ADDR + 9)		//  1���ֽڣ���վ������
#define SL651_POS_ELEMENT					(SL651_PARAM_BASE_ADDR + 10)	//  8���ֽڣ����Ҫ��
#define SL651_POS_WORK_MODE					(SL651_PARAM_BASE_ADDR + 18)	//  1���ֽڣ�����ģʽ
#define SL651_POS_REPORT_PERIOD_HOUR		(SL651_PARAM_BASE_ADDR + 19)	//  1���ֽڣ���ʱ�����
#define SL651_POS_REPORT_PERIOD_MIN			(SL651_PARAM_BASE_ADDR + 20)	//  1���ֽڣ��ӱ����
#define SL651_POS_ERC						(SL651_PARAM_BASE_ADDR + 21)	// 64���ֽڣ��¼���¼
#define SL651_POS_DEVICE_ID					(SL651_PARAM_BASE_ADDR + 85)	// 20���ֽڣ��豸ʶ���
#define SL651_POS_RANDOM_SAMPLE_PERIOD		(SL651_PARAM_BASE_ADDR + 105)	//  2���ֽڣ�����ɼ����

//������Ϣ
#define SL651_BYTES_REPORT_DATA				100		//�ϱ����ݿռ�
#define SL651_BYTES_MANUAL_DATA				50		//�˹��������ݿռ�
#define SL651_BYTES_DEVICE_ID				19		//�豸ʶ�������ݿռ�
#define SL651_DATA_POINT_PER_PACKET			20
#define SL651_BYTES_WARN_INFO_MAX			24
#define SL651_WARN_INFO_VALID				0xa55a
#define SL651_NUM_WARN_INFO					4
#define SL651_BYTES_WARN_INFO_IN_FLASH		50
#define SL651_DEF_WARN_TIMES				3

//����
#define SL651_STK_DATA_DOWNLOAD				640		//������������ջ
#define SL651_STK_RANDOM_SAMPLE				640		//�˹���������ջ
#define SL651_STK_REPORT_TIME				640		//��ʱ������ջ
#ifndef SL651_PRIO_DATA_DOWNLOAD
#define SL651_PRIO_DATA_DOWNLOAD			20
#endif
#ifndef SL651_PRIO_RANDOM_SAMPLE
#define SL651_PRIO_RANDOM_SAMPLE			20
#endif
#ifndef SL651_PRIO_REPORT_TIME
#define SL651_PRIO_REPORT_TIME				21
#endif

//֡�ṹ
#define SL651_BYTES_FRAME_SYMBOL			2		//֡��־�ֽ���
#define SL651_BYTES_RTU_ADDR				5		//RTU��ַ�ֽ���
#define SL651_BYTES_CENTER_ADDR				1		//����վ��ַ�ֽ���
#define SL651_BYTES_PW						2		//�����ֽ���
#define SL651_BYTES_AFN						1		//�������ֽ���
#define SL651_BYTES_UP_DOWN_LEN				2		//�����б�ʶ�������ֽ���
#define SL651_BYTES_START_SYMBOL			1		//������ʼ���ֽ���
#define SL651_BYTES_SN						2		//��ˮ���ֽ���
#define SL651_BYTES_SEND_TIME				6		//����ʱ���ֽ���
#define SL651_BYTES_CRC_VALUE				2		//CRCУ�����ֽ���
#define SL651_BYTES_FRAME_HEAD				14		//֡ͷ�ֽ���
#define SL651_BYTES_FRAME_END				3		//֡β�ֽ���
#define SL651_BYTES_RTU_TYPE				1		//��վ�������ֽ���
#define SL651_BYTES_FRAME_OBSERVE_TIME		7		//����ʶ���Ĺ۲�ʱ���ֽ���
#define SL651_BYTES_FRAME_RTU_ADDR			7		//����ʶ����RTU��ַ�ֽ���
#define SL651_BYTES_FRAME_SHANGQING			5		//����ʶ�������������ֽ���(������)
#define SL651_BYTES_FRAME_SHANGQING_EX		11		//�����������ֽ���
#define SL651_BYTES_FRAME_VOLTAGE			4		//����ʶ���Ĺ����ѹ�����ֽ���
#define SL651_BYTES_FRAME_VOLTAGE_EX		5		//����ʶ���ĳ���ѹ�����ֽ���
#define SL651_BYTES_FRAME_STATUS_ALARM		6		//����ʶ����״̬��������Ϣ�ֽ���
#define SL651_BYTES_FRAME_RAIN				5
#define SL651_BYTES_FRAME_WATER_CUR			7
#define SL651_BYTES_FRAME_CSQ_EX			4
#define SL651_BYTES_FRAME_ERR_CODE_EX		6
#define SL651_BYTES_FRAME_WATER_STA_EX		4
#define SL651_BYTES_FRAME_TIME_RANGE		8		//ʱ�䷶Χ�ֽ���
#define SL651_BYTES_FRAME_TIME_STEP			5		//ʱ�䲽���ֽ���
#define SL651_BYTES_FRAME_ELEMENT_SYMBOL	3		//Ҫ�ر�ʶ���ֽ���(����չFF)
#define SL651_BYTES_FRAME_PACKET_INFO		3		//�������������ֽ���
#define SL651_FRAME_DEFAULT_FCB				3		//Ĭ��FCBֵ
#define SL651_FRAME_DEFAULT_PW				0		//ȱʡ����
#define SL651_FRAME_SYMBOL					0x7e	//֡��־
#define SL651_FRAME_SYMBOL_STX				0x02	//������ʼ��������
#define SL651_FRAME_SYMBOL_SYN				0x16	//������ʼ�������
#define SL651_FRAME_SYMBOL_ETX				0x03	//���Ľ������������ޱ���
#define SL651_FRAME_SYMBOL_ETB				0x17	//���Ľ������������б���
#define SL651_FRAME_SYMBOL_ENQ				0x05	//���Ľ�������ѯ��
#define SL651_FRAME_SYMBOL_EOT				0x04	//���Ľ��������˳�ͨ��
#define SL651_FRAME_SYMBOL_ACK				0x06	//���Ľ�������ȷ�ϣ�������ͽ���
#define SL651_FRAME_SYMBOL_NAK				0x15	//���Ľ����������ϣ��ط�ĳ��
#define SL651_FRAME_SYMBOL_ESC				0x1b	//���Ľ���������������
#define SL651_FRAME_INVALID_DATA			0xff	//��Ч����
#define SL651_FRAME_SIGN_NEGATIVE			0xff	//����־
#define SL651_FRAME_DIR_UP					0		//���б�־
#define SL651_FRAME_DIR_DOWN				8		//���б�־
#define SL651_FRAME_SYMBOL_ELEMENT_EX		0xff	//��չҪ�ر�ʶ����־

//��վ������
#define SL651_RTU_TYPE_JIANGSHUI			0x50	//��ˮ
#define SL651_RTU_TYPE_HEDAO				0x48	//�ӵ�
#define SL651_RTU_TYPE_SHUIKU				0x4b	//ˮ��
#define SL651_RTU_TYPE_ZHABA				0x5a	//բ��
#define SL651_RTU_TYPE_BENGZHAN				0x44	//��վ
#define SL651_RTU_TYPE_CHAOXI				0x54	//��ϫ
#define SL651_RTU_TYPE_SHANGQING			0x4d	//����
#define SL651_RTU_TYPE_DIXIASHUI			0x47	//����ˮ
#define SL651_RTU_TYPE_SHUIZHI				0x51	//ˮ��
#define SL651_RTU_TYPE_QUSHUIKOU			0x49	//ȡˮ��
#define SL651_RTU_TYPE_PAISHUIKOU			0x4f	//��ˮ��

//������
#define SL651_AFN_REPORT_HEART				0x2f	//��·ά�ֱ�
#define SL651_AFN_REPORT_TEST				0x30	//���Ա�
#define SL651_AFN_REPORT_AVERAGE			0x31	//����ʱ�α�
#define SL651_AFN_REPORT_TIMING				0x32	//��ʱ��
#define SL651_AFN_REPORT_ADD				0x33	//�ӱ���
#define SL651_AFN_REPORT_HOUR				0x34	//Сʱ��
#define SL651_AFN_REPORT_MANUAL				0x35	//�˹�����
#define SL651_AFN_QUERY_PHOTO				0x36	//��ѯͼƬ��Ϣ
#define SL651_AFN_QUERY_CUR_DATA			0x37	//��ѯʵʱ����
#define SL651_AFN_QUERY_OLD_DATA			0x38	//��ѯ��ʷ����
#define SL651_AFN_QUERY_MANUAL_DATA			0x39	//��ѯ�˹�����
#define SL651_AFN_QUERY_ELEMENT_DATA		0x3a	//��ѯָ��Ҫ������
#define SL651_AFN_SET_BASIC_PARAM			0x40	//�޸Ļ�������
#define SL651_AFN_QUERY_BASIC_PARAM			0x41	//��ȡ��������
#define SL651_AFN_SET_RUN_PARAM				0x42	//�޸���������
#define SL651_AFN_QUERY_RUN_PARAM			0x43	//��ȡ��������
#define SL651_AFN_QUERY_MOTO_DATA			0x44	//��ѯˮ�õ������
#define SL651_AFN_QUERY_SOFT_VER			0x45	//��ѯ����汾
#define SL651_AFN_QUERY_RTU_STATUS			0x46	//��ѯң��վ״̬��������Ϣ
#define SL651_AFN_CLEAR_OLD_DATA			0x47	//�����ʷ����
#define SL651_AFN_RTU_RESTORE				0x48	//�ָ���������
#define SL651_AFN_CHANGE_PW					0x49	//�޸�����
#define SL651_AFN_SET_TIME					0x4a	//����ʱ��
#define SL651_AFN_SET_IC_STATUS				0x4b	//����IC��״̬
#define SL651_AFN_CTRL_PUMP					0x4c	//����ˮ�ÿ������ˮ��״̬��Ϣ�Ա�
#define SL651_AFN_CTRL_VALVE				0x4d	//���Ʒ��ſ����������״̬��Ϣ�Ա�
#define SL651_AFN_CTRL_SLUICE				0x4e	//����բ�ſ������բ��״̬��Ϣ�Ա�
#define SL651_AFN_CTRL_FIXED_WATER			0x4f	//ˮ����ֵ��������
#define SL651_AFN_QUERY_ERC					0x50	//��ѯ�¼���¼
#define SL651_AFN_QUERY_TIME				0x51	//��ѯʱ��

//Ҫ�ر�ʶ��������
#define SL651_BOOT_OBSERVE_TIME				0xf0	//�۲�ʱ��
#define SL651_BOOT_RTU_CODE					0xf1	//��վ����
#define SL651_BOOT_MANUAL_DATA				0xf2	//�˹�����
#define SL651_BOOT_PHOTO_INFO				0xf3	//ͼƬ��Ϣ
#define SL651_BOOT_WATER_TEMPER				0x03	//˲ʱˮ��
#define SL651_BOOT_TIME_STEP				0x04	//ʱ�䲽��
#define SL651_BOOT_GROUND_WATER				0x0e	//����ˮ˲ʱ����
#define SL651_BOOT_SHANGQING_10CM			0x10	//10cm��������ˮ��
#define SL651_BOOT_SHANGQING_20CM			0x11	//20cm��������ˮ��
#define SL651_BOOT_SHANGQING_30CM			0x12	//30cm��������ˮ��
#define SL651_BOOT_SHANGQING_40CM			0x13	//40cm��������ˮ��
#define SL651_BOOT_SHANGQING_50CM			0x14	//50cm��������ˮ��
#define SL651_BOOT_SHANGQING_60CM			0x15	//60cm��������ˮ��
#define SL651_BOOT_SHANGQING_80CM			0x16	//80cm��������ˮ��
#define SL651_BOOT_SHANGQING_100CM			0x17	//100cm��������ˮ��
#define SL651_BOOT_SHANGQING_10CM_EX		0xff10	//10cm��������ˮ��(������)
#define SL651_BOOT_SHANGQING_20CM_EX		0xff20	//20cm��������ˮ��(������)
#define SL651_BOOT_SHANGQING_30CM_EX		0xff30	//40cm��������ˮ��(������)
#define SL651_BOOT_SHANGQING_40CM_EX		0xff40	//40cm��������ˮ��(������)
#define SL651_BOOT_RAIN_TOTAL				0x26	//��ˮ���ۼ�ֵ
#define SL651_BOOT_LIUSU_CUR				0x37	//˲ʱ����
#define SL651_BOOT_PWR_VOLTAGE				0x38	//��Դ��ѹ
#define SL651_BOOT_PWR_VOLTAGE_EX			0xff38	//��Դ��ѹ
#define SL651_BOOT_WATER_CUR				0x39	//˲ʱˮλ
#define SL651_BOOT_WATER_CUR_1				0x3c	//ȡ��ˮ��ˮλ1
#define SL651_BOOT_WATER_CUR_2				0x3d	//ȡ��ˮ��ˮλ2
#define SL651_BOOT_WATER_CUR_3				0x3e	//ȡ��ˮ��ˮλ3
#define SL651_BOOT_WATER_CUR_4				0x3f	//ȡ��ˮ��ˮλ4
#define SL651_BOOT_WATER_CUR_5				0x40	//ȡ��ˮ��ˮλ5
#define SL651_BOOT_WATER_CUR_6				0x41	//ȡ��ˮ��ˮλ6
#define SL651_BOOT_WATER_CUR_7				0x42	//ȡ��ˮ��ˮλ7
#define SL651_BOOT_WATER_CUR_8				0x43	//ȡ��ˮ��ˮλ8
#define SL651_BOOT_STATUS_ALARM				0x45	//״̬��������Ϣ
#define SL651_BOOT_FLOW_CUR_1				0x68	//ˮ��1ÿСʱˮ��
#define SL651_BOOT_FLOW_CUR_2				0x69	//ˮ��2ÿСʱˮ��
#define SL651_BOOT_FLOW_CUR_3				0x6a	//ˮ��3ÿСʱˮ��
#define SL651_BOOT_FLOW_CUR_4				0x6b	//ˮ��4ÿСʱˮ��
#define SL651_BOOT_FLOW_CUR_5				0x6c	//ˮ��5ÿСʱˮ��
#define SL651_BOOT_FLOW_CUR_6				0x6d	//ˮ��6ÿСʱˮ��
#define SL651_BOOT_FLOW_CUR_7				0x6e	//ˮ��7ÿСʱˮ��
#define SL651_BOOT_FLOW_CUR_8				0x6f	//ˮ��8ÿСʱˮ��
#define SL651_BOOT_CSQ_EX					0xff85
#define SL651_BOOT_ERR_CODE_EX				0xff81
#define SL651_BOOT_WATER_LIMIT_1_EX			0xff3a
#define SL651_BOOT_WATER_LIMIT_2_EX			0xff3b
#define SL651_BOOT_WATER_STA_EX				0xff3c

//����������ʶ��������
#define SL651_BOOT_CENTER_ADDR				0x01	//����վ��ַ
#define SL651_BOOT_RTU_ADDR					0x02	//RTU��ַ
#define SL651_BOOT_PW						0x03	//����
#define SL651_BOOT_CENTER_1_COMM_1			0x04	//����վ1���ŵ�
#define SL651_BOOT_CENTER_1_COMM_2			0x05	//����վ1���ŵ�
#define SL651_BOOT_CENTER_2_COMM_1			0x06	//����վ2���ŵ�
#define SL651_BOOT_CENTER_2_COMM_2			0x07	//����վ2���ŵ�
#define SL651_BOOT_CENTER_3_COMM_1			0x08	//����վ3���ŵ�
#define SL651_BOOT_CENTER_3_COMM_2			0x09	//����վ3���ŵ�
#define SL651_BOOT_CENTER_4_COMM_1			0x0a	//����վ4���ŵ�
#define SL651_BOOT_CENTER_4_COMM_2			0x0b	//����վ4���ŵ�
#define SL651_BOOT_WORK_MODE				0x0c	//������ʽ
#define SL651_BOOT_SAMPLE_ELEMENT			0x0d	//�ɼ�Ҫ��
#define SL651_BOOT_DEVICE_ID				0x0f	//�豸ʶ����

//���в�����ʶ��������
#define SL651_BOOT_REPORT_TIMEING			0x20	//��ʱ��ʱ����
#define SL651_BOOT_REPORT_ADD				0x21	//�ӱ�ʱ����
#define SL651_BOOT_RAIN_UNIT				0x25	//�����ֱ���
#define SL651_BOOT_WATER_BASE_1				0x28	//ˮλ��ֵ1
#define SL651_BOOT_WATER_BASE_2				0x29	//ˮλ��ֵ2
#define SL651_BOOT_WATER_BASE_3				0x2a	//ˮλ��ֵ3
#define SL651_BOOT_WATER_BASE_4				0x2b	//ˮλ��ֵ4
#define SL651_BOOT_WATER_BASE_5				0x2c	//ˮλ��ֵ5
#define SL651_BOOT_WATER_BASE_6				0x2d	//ˮλ��ֵ6
#define SL651_BOOT_WATER_BASE_7				0x2e	//ˮλ��ֵ7
#define SL651_BOOT_WATER_BASE_8				0x2f	//ˮλ��ֵ8
#define SL651_BOOT_WATER_OFFSET_1			0x30	//ˮλ����ֵ1
#define SL651_BOOT_WATER_OFFSET_2			0x31	//ˮλ����ֵ2
#define SL651_BOOT_WATER_OFFSET_3			0x32	//ˮλ����ֵ3
#define SL651_BOOT_WATER_OFFSET_4			0x33	//ˮλ����ֵ4
#define SL651_BOOT_WATER_OFFSET_5			0x34	//ˮλ����ֵ5
#define SL651_BOOT_WATER_OFFSET_6			0x35	//ˮλ����ֵ6
#define SL651_BOOT_WATER_OFFSET_7			0x36	//ˮλ����ֵ7
#define SL651_BOOT_WATER_OFFSET_8			0x37	//ˮλ����ֵ8
#define SL651_BOOT_CLEAR_OLD_DATA			0x97	//�����ʷ����
#define SL651_BOOT_RTU_RESTORE				0x98	//�ָ���������

//Ҫ�����ݶ���
#define SL651_LEN_OBSERVE_TIME				0xf0	//�۲�ʱ��
#define SL651_LEN_RTU_CODE					0xf1	//��վ����
#define SL651_LEN_MANUAL_DATA				0xf2	//�˹�����
#define SL651_LEN_PHOTO_INFO				0xf3	//ͼƬ��Ϣ
#define SL651_LEN_TIME_STEP					0x18	//ʱ�䲽��
#define SL651_LEN_PWR_VOLTAGE				0x12	//��Դ��ѹ
#define SL651_LEN_PWR_VOLTAGE_EX			0x12	//��Դ��ѹ
#define SL651_LEN_STATUS_ALARM				0x20	//״̬��������Ϣ
#define SL651_LEN_FLOW_CUR					0x2a	//ˮ��ÿСʱˮ��
#define SL651_LEN_RAIN_TOTAL				0x19	//��ˮ���ۼ�ֵ
#define SL651_LEN_WATER_CUR					0x23	//ˮλ
#define SL651_LEN_LIUSU_CUR					0x1b	//����
#define SL651_LEN_GROUND_WATER				0x1a	//����ˮ˲ʱ����
#define SL651_LEN_SHANGQING					0x11	//����
#define SL651_LEN_SHANGQING_EX				0x08	//����(������)
#define SL651_LEN_CSQ_EX					0x08
#define SL651_LEN_ERR_CODE_EX				0x18
#define SL651_LEN_WATER_STA_EX				0x08

//�����������ݶ���
#define SL651_LEN_CENTER_ADDR				0x20	//����վ��ַ
#define SL651_LEN_RTU_ADDR					0x28	//RTU��ַ
#define SL651_LEN_PW						0x10	//����
#define SL651_LEN_WORK_MODE					0x08	//������ʽ
#define SL651_LEN_SAMPLE_ELEMENT			0x40	//�ɼ�Ҫ��

//���в������ݶ���
#define SL651_LEN_REPORT_TIMEING			0x08	//��ʱ��ʱ����
#define SL651_LEN_REPORT_ADD				0x08	//�ӱ�ʱ����
#define SL651_LEN_RAIN_UNIT					0x09	//�����ֱ���
#define SL651_LEN_WATER_BASE				0x23	//ˮλ��ֵ
#define SL651_LEN_WATER_OFFSET				0x1b	//ˮλ����ֵ
#define SL651_LEN_CLEAR_OLD_DATA			0x00	//�����ʷ����
#define SL651_LEN_RTU_RESTORE				0x00	//�ָ���������

//���Ҫ��
#define SL651_BYTES_ELEMENT					8		//���Ҫ���ֽ���
#define SL651_ELEMENT_QIYA					0		//��ѹ
#define SL651_ELEMENT_DIWEN					1		//����
#define SL651_ELEMENT_SHIDU					2		//ʪ��
#define SL651_ELEMENT_QIWEN					3		//����
#define SL651_ELEMENT_FENGSU				4		//����
#define SL651_ELEMENT_FENGXIANG				5		//����
#define SL651_ELEMENT_ZHENGFA				6		//������
#define SL651_ELEMENT_JIANGSHUI				7		//��ˮ��
#define SL651_ELEMENT_SHUIWEI_1				8		//ˮλ1
#define SL651_ELEMENT_SHUIWEI_2				9		//ˮλ2
#define SL651_ELEMENT_SHUIWEI_3				10		//ˮλ3
#define SL651_ELEMENT_SHUIWEI_4				11		//ˮλ4
#define SL651_ELEMENT_SHUIWEI_5				12		//ˮλ5
#define SL651_ELEMENT_SHUIWEI_6				13		//ˮλ6
#define SL651_ELEMENT_SHUIWEI_7				14		//ˮλ7
#define SL651_ELEMENT_SHUIWEI_8				15		//ˮλ8
#define SL651_ELEMENT_SHUIYA				16		//ˮѹ
#define SL651_ELEMENT_LIULIANG				17		//����
#define SL651_ELEMENT_LIUSU					18		//����
#define SL651_ELEMENT_SHUILIANG				19		//ˮ��
#define SL651_ELEMENT_ZHAMEN				20		//բ��
#define SL651_ELEMENT_BOLANG				21		//����
#define SL651_ELEMENT_TUPIAN				22		//ͼƬ
#define SL651_ELEMENT_MAISHEN				23		//����ˮ����
#define SL651_ELEMENT_SHUIBIAO_1			24		//ˮ��1
#define SL651_ELEMENT_SHUIBIAO_2			25		//ˮ��2
#define SL651_ELEMENT_SHUIBIAO_3			26		//ˮ��3
#define SL651_ELEMENT_SHUIBIAO_4			27		//ˮ��4
#define SL651_ELEMENT_SHUIBIAO_5			28		//ˮ��5
#define SL651_ELEMENT_SHUIBIAO_6			29		//ˮ��6
#define SL651_ELEMENT_SHUIBIAO_7			30		//ˮ��7
#define SL651_ELEMENT_SHUIBIAO_8			31		//ˮ��8
#define SL651_ELEMENT_SHANGQING_10CM		32		//10CM����
#define SL651_ELEMENT_SHANGQING_20CM		33		//20CM����
#define SL651_ELEMENT_SHANGQING_30CM		34		//30CM����
#define SL651_ELEMENT_SHANGQING_40CM		35		//40CM����
#define SL651_ELEMENT_SHANGQING_50CM		36		//50CM����
#define SL651_ELEMENT_SHANGQING_60CM		37		//60CM����
#define SL651_ELEMENT_SHANGQING_80CM		38		//80CM����
#define SL651_ELEMENT_SHANGQING_100CM		39		//100CM����
#define SL651_ELEMENT_SHUIWEN				40		//ˮ��
#define SL651_ELEMENT_ANDAN					41		//����
#define SL651_ELEMENT_GAOMENGSUAN			42		//��������ָ��
#define SL651_ELEMENT_YANGHUA				43		//������ԭ��λ
#define SL651_ELEMENT_ZHUODU				44		//�Ƕ�
#define SL651_ELEMENT_DIANDAOLV				45		//�絼��
#define SL651_ELEMENT_RONGJIEYANG			46		//�ܽ���
#define SL651_ELEMENT_PH					47		//PHֵ
#define SL651_ELEMENT_GE					48		//��
#define SL651_ELEMENT_ZONGGONG				49		//�ܹ�
#define SL651_ELEMENT_SHEN					50		//��
#define SL651_ELEMENT_XI					51		//��
#define SL651_ELEMENT_XIN					52		//п
#define SL651_ELEMENT_ZONGLIN				53		//����
#define SL651_ELEMENT_ZONGDAN				54		//�ܵ�
#define SL651_ELEMENT_ZONGTAN				55		//���л�̼
#define SL651_ELEMENT_QIAN					56		//Ǧ
#define SL651_ELEMENT_TONG					57		//ͭ
#define SL651_ELEMENT_YELVSU_A				58		//Ҷ����a
#define SL651_NUM_ELEMENT					59		//���Ҫ������


//״̬�ͱ�����Ϣ
#define SL651_NUM_RTU_STA					32		//״̬��Ϣ����
#define SL651_STA_AC_OFF					0		//��������״̬
#define SL651_STA_DC_LACK					1		//���ص�ѹ״̬
#define SL651_STA_WATER_LEVEL_OVER			2		//ˮλ���ޱ���״̬
#define SL651_STA_FLOW_OVER					3		//�������ޱ���״̬
#define SL651_STA_WATER_QUALITY_OVER		4		//ˮ�ʳ��ޱ���״̬
#define SL651_STA_FLOW_SENSOR_WRONG			5		//�����Ǳ�״̬
#define SL651_STA_WATER_SENSOR_WRONG		6		//ˮλ�Ǳ�״̬
#define SL651_STA_DOOR_CLOSE				7		//�ն�����״̬
#define SL651_STA_FLASH_WRONG				8		//�洢��״̬
#define SL651_STA_IC_EFFECTIVE				9		//IC������״̬
#define SL651_STA_PUMP_OFF					10		//ˮ�ù���״̬
#define SL651_STA_WATER_OVER				11		//ʣ��ˮ������

//����ģʽ
#define SL651_MODE_REPORT					1		//�Ա�ģʽ
#define SL651_MODE_REPLY					2		//�Ա�ȷ��ģʽ
#define SL651_MODE_QA						3		//��ѯӦ��ģʽ
#define SL651_MODE_DEBUG					4		//����ģʽ

//�¼���¼
#define SL651_NUM_ERC_TYPE					32		//�¼�����
#define SL651_ERC_CLEAR_OLD_DATA			0		//��ʷ���ݳ�ʼ����¼
#define SL651_ERC_PARAM_CHANGE				1		//���������¼
#define SL651_ERC_STATUS_CHANGE				2		//״̬����λ��¼
#define SL651_ERC_SENSOR_WRONG				3		//�Ǳ���ϼ�¼
#define SL651_ERC_PW_CHANGE					4		//�����޸ļ�¼
#define SL651_ERC_RTU_WRONG					5		//�ն˹��ϼ�¼
#define SL651_ERC_AC_OFF					6		//����ʧ���¼
#define SL651_ERC_DC_LACK					7		//���ص�ѹ�͸澯��¼
#define SL651_ERC_DOOR_OPEN					8		//�ն����ŷǷ��򿪼�¼
#define SL651_ERC_PUMP_WRONG				9		//ˮ�ù��ϼ�¼
#define SL651_ERC_WATER_OVER				10		//ʣ��ˮ��Խ�޸澯��¼
#define SL651_ERC_WATER_LEVEL_OVER			11		//ˮλ���޸澯��¼
#define SL651_ERC_WATER_PRESSURE_OVER		12		//ˮѹ���޸澯��¼
#define SL651_ERC_WATER_QUALITY_OVER		13		//ˮ�ʲ������޸澯��¼
#define SL651_ERC_DATA_WRONG				14		//���ݳ����¼
#define SL651_ERC_SEND_DATA					15		//�����ļ�¼
#define SL651_ERC_RECV_DATA					16		//�ձ��ļ�¼
#define SL651_ERC_SEND_FAIL					17		//�����ĳ����¼

//��������
#define SL651_EVENT_PARAM_RTU_ADDR			0x01
#define SL651_EVENT_PARAM_PW				0x02
#define SL651_EVENT_PARAM_SN				0x04
#define SL651_EVENT_PARAM_RTU_TYPE			0x08
#define SL651_EVENT_PARAM_ELEMENT			0x10
#define SL651_EVENT_PARAM_WORK_MODE			0x20
#define SL651_EVENT_PARAM_REPORT_HOUR		0x40
#define SL651_EVENT_PARAM_REPORT_MIN		0x80
#define SL651_EVENT_PARAM_ERC				0x100
#define SL651_EVENT_PARAM_DEVICE_ID			0x200
#define SL651_EVENT_PARAM_RANDOM_PERIOD		0x400
#define SL651_EVENT_PARAM_ALL				0x7ff



//��������
typedef struct sl651_data_download
{
	rt_uint8_t			comm_type;
	rt_uint8_t			ch;
	rt_uint8_t			center_addr;
	rt_uint16_t			element_boot;
	rt_uint8_t			element_len;
	rt_uint32_t			unix_low;
	rt_uint32_t			unix_high;
	rt_uint32_t			unix_step;
} SL651_DATA_DOWNLOAD;

//�˹�����
typedef struct sl651_manual_data
{
	rt_uint8_t			data[SL651_BYTES_MANUAL_DATA];
	rt_uint16_t			data_len;
} SL651_MANUAL_DATA;

//�豸ʶ����
typedef struct sl651_device_id
{
	rt_uint8_t			data[SL651_BYTES_DEVICE_ID];
	rt_uint8_t			data_len;
} SL651_DEVICE_ID;

//�澯��Ϣ
typedef struct sl651_warn_info
{
	rt_uint16_t			valid;
	rt_uint16_t			data_len;
	rt_uint8_t			data[SL651_BYTES_WARN_INFO_MAX];
} SL651_WARN_INFO;



//��������
void sl651_get_rtu_addr(rt_uint8_t *rtu_addr_ptr);
void sl651_set_rtu_addr(rt_uint8_t const *rtu_addr_ptr);

rt_uint16_t sl651_get_pw(void);
void sl651_set_pw(rt_uint16_t pw);

rt_uint16_t sl651_get_sn(void);
void sl651_set_sn(rt_uint16_t sn);

rt_uint8_t sl651_get_rtu_type(void);
void sl651_set_rtu_type(rt_uint8_t rtu_type);

#if 0
void sl651_get_element(rt_uint8_t *pelement);
void sl651_set_element(rt_uint8_t const *pelement);

rt_uint8_t sl651_get_work_mode(void);
void sl651_set_work_mode(rt_uint8_t work_mode);

rt_uint8_t sl651_get_device_id(rt_uint8_t *pid);
void sl651_set_device_id(rt_uint8_t const *pid, rt_uint8_t id_len);
#endif

rt_uint8_t sl651_get_report_hour(void);
void sl651_set_report_hour(rt_uint8_t period);

rt_uint8_t sl651_get_report_min(void);
void sl651_set_report_min(rt_uint8_t period);

rt_uint16_t sl651_get_random_period(void);
void sl651_set_random_period(rt_uint16_t period);

void sl651_set_center_comm(rt_uint8_t center, rt_uint8_t prio, rt_uint8_t const *pdata, rt_uint8_t data_len);

rt_uint16_t sl651_get_warn_info(rt_uint8_t *pdata, rt_uint8_t warn_level);
void sl651_set_warn_info(rt_uint8_t const *pdata, rt_uint16_t data_len, rt_uint8_t warn_level);



//��������
void sl651_param_restore(void);

//���ݺϷ��Լ�⺯��
rt_uint8_t sl651_data_valid(rt_uint8_t const *pdata, rt_uint16_t data_len, rt_uint16_t data_id, rt_uint8_t reply_sta, rt_uint8_t fcb_value);

//���ݸ��º���
rt_uint8_t sl651_data_update(rt_uint8_t *pdata, rt_uint16_t data_len, rt_uint8_t center_addr, struct tm time);

//���������뺯��
rt_uint16_t sl651_heart_encoder(rt_uint8_t *pdata, rt_uint16_t data_len, rt_uint8_t ch);

//���ݽ��
rt_uint8_t sl651_data_decoder(PTCL_REPORT_DATA *report_data_ptr, PTCL_RECV_DATA const *recv_data_ptr, PTCL_PARAM_INFO **param_info_ptr_ptr);

rt_uint8_t sl651_report_decoder(PTCL_REPORT_DATA *report_data_ptr, PTCL_RECV_DATA const *recv_data_ptr);



#endif

