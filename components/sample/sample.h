/*�洢�ռ�
**EEPROM		:1400--1999
**FLASH			:1150976--11636735(10MB)	11636736--22122495(10MB)
*/



#ifndef __SAMPLE_H__
#define __SAMPLE_H__



#include "rtthread.h"
#include "user_priority.h"
#include "drv_pin.h"



//����ַ
#define SAMPLE_BASE_ADDR_PARAM			1400			//�����洢����ַ
#define SAMPLE_BASE_ADDR_DATA			1150976			//���ݴ洢����ַ
#define SAMPLE_BASE_ADDR_DATA_EX		11636736		//��չ���ݴ洢����ַ

//������Ϣ
#define SAMPLE_NUM_CTRL_BLK				10				//�ɼ����ƿ�����
#define SAMPLE_PIN_ADC_BATT				PIN_INDEX_46
#define SAMPLE_PIN_ADC_VIN				PIN_INDEX_47
#if 0
#define SAMPLE_TASK_STORE_EN
#endif

//����
#define SAMPLE_STK_DATA_STORE			640				//���ݴ洢����ջ�ռ�
#define SAMPLE_STK_QUERY_DATA_EX		640
#ifndef SAMPLE_PRIO_DATA_STORE
#define SAMPLE_PRIO_DATA_STORE			30
#endif
#ifndef SAMPLE_PRIO_QUERY_DATA_EX
#define SAMPLE_PRIO_QUERY_DATA_EX		24
#endif

//���ڲɼ�������Ϣ
#define SAMPLE_BYTES_COM_RECV			96				//���ڽ����ֽڿռ�
#define SAMPLE_ERROR_TIMES				2				//�ɼ��������
#define SAMPLE_TIME_DATA_NO				1500			//����
#define SAMPLE_TIME_DATA_END			200				//����

//���ݴ洢��Ϣ
#define SAMPLE_BYTES_PER_ITEM			11				//һ�������ֽ���
#define SAMPLE_NUM_TOTAL_ITEM			953250			//����������
#define SAMPLE_BYTES_PER_ITEM_EX		512				//һ�������ֽ���
#define SAMPLE_NUM_TOTAL_ITEM_EX		20480			//����������
#define SAMPLE_BYTES_ITEM_INFO_EX		9				//��������Ϣ(�� �� �� ʱ �� CN LEN)

//����������(��4λ,�������16�ִ���������)
#define SAMPLE_SENSOR_WUSHUI			0				//��ˮ
#define SAMPLE_SENSOR_COD				1				//��ѧ������
#define SAMPLE_NUM_SENSOR_TYPE			2				//��������������
#define SAMPLE_IS_SENSOR_TYPE(VAL)		((VAL & 0x0f) < SAMPLE_NUM_SENSOR_TYPE)

//��������������(��4λ)
#define SAMPLE_SENSOR_DATA_CUR			(0 << 4)
#define SAMPLE_SENSOR_DATA_MIN_H		(1 << 4)
#define SAMPLE_SENSOR_DATA_MIN_D		(2 << 4)
#define SAMPLE_SENSOR_DATA_AVG_H		(3 << 4)
#define SAMPLE_SENSOR_DATA_AVG_D		(4 << 4)
#define SAMPLE_SENSOR_DATA_MAX_H		(5 << 4)
#define SAMPLE_SENSOR_DATA_MAX_D		(6 << 4)
#define SAMPLE_SENSOR_DATA_COU_M		(7 << 4)
#define SAMPLE_SENSOR_DATA_COU_H		(8 << 4)
#define SAMPLE_SENSOR_DATA_COU_D		(9 << 4)

//Ӳ���ӿ�
#define SAMPLE_HW_RS485_1				0
#define SAMPLE_HW_RS485_2				1
#define SAMPLE_HW_RS232_1				2
#define SAMPLE_HW_RS232_2				3
#define SAMPLE_HW_RS232_3				4
#define SAMPLE_HW_RS232_4				5
#define SAMPLE_HW_ADC					6
#define SAMPLE_NUM_HW_PORT				7

//Э��
#define SAMPLE_PTCL_A					0
#define SAMPLE_NUM_PTCL					1

//�¼���־
#define SAMPLE_EVENT_CTRL_BLK			0x01			//���ƿ�
#define SAMPLE_EVENT_HW_PORT			0x10000			//Ӳ���˿����¼�
#define SAMPLE_EVENT_DATA_POINTER		0x80000000		//�洢����ָ��
#define SAMPLE_EVENT_SUPPLY_VOLT		0x40000000
#define SAMPLE_EVENT_DATA_POINTER_EX	0x20000000		//��չ�洢����ָ��
#define SAMPLE_EVENT_INIT_VALUE			0xe000ffff



//���ݶ���
typedef struct sample_data_type
{
	rt_uint8_t			sta;							//״̬(�Ƿ���Ч)
	float				value;							//��ֵ
} SAMPLE_DATA_TYPE;

//���ڽ���
typedef struct sample_com_recv
{
	rt_uint32_t			recv_event;						//�����¼�
	rt_uint8_t			*pdata;							//����
	rt_uint16_t			data_len;						//���ݳ���
} SAMPLE_COM_RECV;

//�ɼ����ƿ�
typedef struct sample_ctrl_blk
{
	rt_uint8_t			sensor_type;					//����������
	rt_uint8_t			sensor_addr;					//��������ַ
	rt_uint8_t			hw_port;						//Ӳ���ӿ�
	rt_uint8_t			protocol;						//Э��
	rt_uint32_t			hw_rate;						//Ӳ������
	rt_uint16_t			store_period;					//�洢���
	rt_uint8_t			k_opt;
	float				k;								//1��ϵ��
	float				b;								//0��ϵ��
	float				base;							//��ֵ
	float				offset;							//����ֵ
	float				up;								//����
	float				down;							//����
	float				threshold;						//��ֵ
} SAMPLE_CTRL_BLK;

//����ָ��
typedef struct sample_data_pointer
{
	rt_uint8_t			sta;							//״̬(�Ƿ񸲸�)
	rt_uint32_t			p;								//ָ��λ��
} SAMPLE_DATA_POINTER;

//��չ���ݲ�ѯ��Ϣ
typedef struct sample_query_info_ex
{
	rt_uint16_t			cn;
	rt_uint32_t			unix_low;
	rt_uint32_t			unix_high;
	rt_mailbox_t		pmb;
} SAMPLE_QUERY_INFO_EX;

//����
typedef struct sample_param_set
{
	SAMPLE_DATA_POINTER	data_pointer;
	SAMPLE_CTRL_BLK		ctrl_blk[SAMPLE_NUM_CTRL_BLK];
	SAMPLE_DATA_POINTER	data_pointer_ex;
} SAMPLE_PARAM_SET;

//���ݱ��뺯��(���ݲ�ѯʱ��)
typedef rt_uint16_t (*SAMPLE_FUN_DATA_FORMAT)(rt_uint8_t *pdst, rt_uint8_t sensor_type, rt_uint8_t sta, float value);



//�������á���ȡ
void sample_set_sensor_type(rt_uint8_t ctrl_num, rt_uint8_t sensor_type);
rt_uint8_t sample_get_sensor_type(rt_uint8_t ctrl_num);

void sample_set_sensor_addr(rt_uint8_t ctrl_num, rt_uint8_t sensor_addr);
rt_uint8_t sample_get_sensor_addr(rt_uint8_t ctrl_num);

void sample_set_hw_port(rt_uint8_t ctrl_num, rt_uint8_t hw_port);
rt_uint8_t sample_get_hw_port(rt_uint8_t ctrl_num);

void sample_set_protocol(rt_uint8_t ctrl_num, rt_uint8_t protocol);
rt_uint8_t sample_get_protocol(rt_uint8_t ctrl_num);

void sample_set_hw_rate(rt_uint8_t ctrl_num, rt_uint32_t hw_rate);
rt_uint32_t sample_get_hw_rate(rt_uint8_t ctrl_num);

void sample_set_store_period(rt_uint8_t ctrl_num, rt_uint16_t store_period);
rt_uint16_t sample_get_store_period(rt_uint8_t ctrl_num);

void sample_set_kopt(rt_uint8_t ctrl_num, rt_uint8_t kopt);
rt_uint8_t sample_get_kopt(rt_uint8_t ctrl_num);

void sample_set_k(rt_uint8_t ctrl_num, float value);
float sample_get_k(rt_uint8_t ctrl_num);

void sample_set_b(rt_uint8_t ctrl_num, float value);
float sample_get_b(rt_uint8_t ctrl_num);

void sample_set_base(rt_uint8_t ctrl_num, float value);
float sample_get_base(rt_uint8_t ctrl_num);

void sample_set_offset(rt_uint8_t ctrl_num, float value);
float sample_get_offset(rt_uint8_t ctrl_num);

void sample_set_up(rt_uint8_t ctrl_num, float value);
float sample_get_up(rt_uint8_t ctrl_num);

void sample_set_down(rt_uint8_t ctrl_num, float value);
float sample_get_down(rt_uint8_t ctrl_num);

void sample_set_threshold(rt_uint8_t ctrl_num, float value);
float sample_get_threshold(rt_uint8_t ctrl_num);

//�ָ�����
void sample_param_restore(void);

//�Ƿ����
rt_uint8_t sample_sensor_type_exist(rt_uint8_t sensor_type);

//�õ����ƿ�
rt_uint8_t sample_get_ctrl_num_by_sensor_type(rt_uint8_t sensor_type);

//���ݲɼ�
SAMPLE_DATA_TYPE sample_get_cur_data(rt_uint8_t sensor_type);

//�������
void sample_clear_old_data(void);

//���ݲ�ѯ
rt_uint16_t sample_query_data(rt_uint8_t *pdata, rt_uint8_t sensor_type, rt_uint32_t unix_low, rt_uint32_t unix_high, rt_uint32_t unix_step, SAMPLE_FUN_DATA_FORMAT fun_data_format);
rt_uint8_t sample_query_data_ex(rt_uint16_t cn, rt_uint32_t unix_low, rt_uint32_t unix_high, rt_mailbox_t pmb);

//�����ѹ��0.01V
rt_uint16_t sample_supply_volt(rt_uint16_t adc_pin);

//�洢����
void sample_store_data(struct tm const *ptime, rt_uint8_t sensor_type, SAMPLE_DATA_TYPE const *pdata);
void sample_store_data_ex(struct tm const *ptime, rt_uint16_t cn, rt_uint16_t data_len, rt_uint8_t const *pdata);

void sample_query_min(rt_uint8_t sensor_type, rt_uint32_t unix_low, rt_uint32_t unix_high, rt_uint32_t unix_step, SAMPLE_DATA_TYPE *pval);
void sample_query_max(rt_uint8_t sensor_type, rt_uint32_t unix_low, rt_uint32_t unix_high, rt_uint32_t unix_step, SAMPLE_DATA_TYPE *pval);
void sample_query_avg(rt_uint8_t sensor_type, rt_uint32_t unix_low, rt_uint32_t unix_high, rt_uint32_t unix_step, SAMPLE_DATA_TYPE *pval);
void sample_query_cou(rt_uint8_t sensor_type, rt_uint32_t unix_low, rt_uint32_t unix_high, rt_uint32_t unix_step, SAMPLE_DATA_TYPE *pval);



#endif

