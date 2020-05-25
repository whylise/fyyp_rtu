#include "sl427.h"
#include "warn_manage.h"
#include "dtu.h"
#include "drv_fyyp.h"
#include "drv_mempool.h"
#include "pwr_manage.h"
#include "drv_si4702.h"
#include "drv_cat9555.h"
#include "bx5k.h"



//smsָ��
static char const			*m_sl427_cmd_type[SL427_NUM_CMD] = {
"bj",	"jglh",	"jsqh",	"sglh",	"ssqh",	"szdz",	"szsb",	"szhz",	"szcs",	"szpl",
"szbf",	"szip",	"szjj",	"cglh",	"csqh",	"cxdz",	"cxsb",	"cxpl",	"cxbf",	"cxip",
"cxcs"
};

static struct rt_event		m_sl427_event_module;
static rt_uint8_t			m_sl427_pwr_type = SL427_PWR_TYPE_ACDC;

//����
static struct rt_thread		m_sl427_thread_report;
static rt_uint8_t			m_sl427_stk_report[SL427_STK_REPORT];

//����
static rt_uint8_t			m_sl427_rtu_addr[SL427_BYTES_RTU_ADDR];
static rt_uint16_t			m_sl427_report_period;
static rt_uint8_t			m_sl427_sms_warn_receipt;



#include "sl427_io.c"



//smsָ��ʶ��
static rt_uint8_t _sl427_cmd_type_identify(char const *pdata)
{
	rt_uint8_t cmd_type;
	
	for(cmd_type = 0; cmd_type < SL427_NUM_CMD; cmd_type++)
	{
		if((char *)0 != rt_strstr(pdata, m_sl427_cmd_type[cmd_type]))
		{
			break;
		}
	}

	return cmd_type;
}

//ָ���а����Ľ����ַ�����
static rt_uint8_t _sl427_num_cmd_tail(rt_uint8_t cmd_type)
{
	if(cmd_type >= SL427_NUM_CMD)
	{
		return 0;
	}

	switch(cmd_type)
	{
	case SL427_CMD_SZDZ:
	case SL427_CMD_SZHZ:
	case SL427_CMD_SZCS:
	case SL427_CMD_SZJJ:
		return 2;
	default:
		return 1;
	}
}

//��������
static void _sl427_param_pend(rt_uint32_t param)
{
	if(RT_EOK != rt_event_recv(&m_sl427_event_module, param, RT_EVENT_FLAG_AND + RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, (rt_uint32_t *)0))
	{
		while(1);
	}
}
static void _sl427_param_post(rt_uint32_t param)
{
	if(RT_EOK != rt_event_send(&m_sl427_event_module, param))
	{
		while(1);
	}
}

//crc
static rt_uint16_t _sl427_crc_value(rt_uint8_t const *pdata, rt_uint16_t data_len)
{
	rt_uint16_t	crc_value = 0xffff;
	rt_uint8_t	i;
	
	while(data_len)
	{
		data_len--;
		crc_value ^= *pdata++;
		for(i = 0; i < 8; i++)
		{
			if(crc_value & 1)
			{
				crc_value >>= 1;
				crc_value ^= 0xa001;
			}
			else
			{
				crc_value >>= 1;
			}
		}
	}
	
	return crc_value;
}

//���ͷ
static rt_uint16_t _sl427_bao_tou(rt_uint8_t *pdata, rt_uint8_t id, rt_uint8_t packet_total, rt_uint8_t packet_cur, rt_uint8_t afn)
{
	rt_uint16_t i = 0;

	pdata[i++] = SL427_FRAME_HEAD_1;
	pdata[i++] = SL427_FRAME_HEAD_2;
	pdata[i++] = id;
	_sl427_param_pend(SL427_PARAM_RTU_ADDR);
	rt_memmove((void *)(pdata + i), (void *)m_sl427_rtu_addr, SL427_BYTES_RTU_ADDR);
	_sl427_param_post(SL427_PARAM_RTU_ADDR);
	i += SL427_BYTES_RTU_ADDR;
	pdata[i++] = packet_total;
	pdata[i++] = packet_cur;
	pdata[i++] = afn;
	pdata[i++] = 0;

	return i;
}

//���뱨����Ϣ
static SL427_WARN_INFO *_sl427_warn_info_req(rt_uint16_t bytes_req, rt_int32_t ticks)
{
	SL427_WARN_INFO *warn_info_ptr;
	
	if(0 == bytes_req)
	{
		return (SL427_WARN_INFO *)0;
	}
	
	bytes_req += sizeof(SL427_WARN_INFO);
	warn_info_ptr = (SL427_WARN_INFO *)mempool_req(bytes_req, ticks);
	if((SL427_WARN_INFO *)0 != warn_info_ptr)
	{
		warn_info_ptr->pdata = (rt_uint8_t *)(warn_info_ptr + 1);
	}
	
	return warn_info_ptr;
}

//gsm�ź�ǿ�ȸ���
static void _sl427_csq_update(rt_uint8_t *pdata, rt_uint16_t data_len, rt_uint8_t csq_value)
{
	if(data_len > SL427_POS_CSQ_IN_REPORT)
	{
		pdata[SL427_POS_CSQ_IN_REPORT] = csq_value;
		data_len -= 2;
		*(rt_uint16_t *)(pdata + data_len) = _sl427_crc_value(pdata, data_len);
	}
}

//�Ա�����
static void _sl427_task_report(void *parg)
{
	struct tm			time;
	rt_uint16_t			min, period;
	PTCL_REPORT_DATA	*report_data_ptr;
	PWRM_PWRSYS_INFO	*pwrsys_info_ptr;
	rt_base_t			level;
	
	while(1)
	{
		ptcl_event_min_pend();

		pwrsys_info_ptr = (PWRM_PWRSYS_INFO *)mempool_req(sizeof(PWRM_PWRSYS_INFO), RT_WAITING_FOREVER);
		if((PWRM_PWRSYS_INFO *)0 != pwrsys_info_ptr)
		{
			pwrm_get_pwrsys_info(pwrsys_info_ptr);

			if(pwrsys_info_ptr->charging_sta)
			{
				cat9555_write_pins(CAT9555_PIN_BATT_LED_R + CAT9555_PIN_BATT_LED_G + CAT9555_PIN_BATT_LED_B, CAT9555_PIN_BATT_LED_G);
			}
			else
			{
				cat9555_write_pins(CAT9555_PIN_BATT_LED_R + CAT9555_PIN_BATT_LED_G + CAT9555_PIN_BATT_LED_B, 0);
			}

			level = rt_hw_interrupt_disable();
			m_sl427_pwr_type = (pwrsys_info_ptr->batt_value > pwrsys_info_ptr->acdc_value) ? SL427_PWR_TYPE_BATT : SL427_PWR_TYPE_ACDC;
			rt_hw_interrupt_enable(level);
			
			_sl427_param_pend(SL427_PARAM_REPORT_PERIOD);
			period = m_sl427_report_period;
			_sl427_param_post(SL427_PARAM_REPORT_PERIOD);

			if(0 != period)
			{
				time = rtcee_rtc_get_calendar();
				min = time.tm_hour;
				min *= 60;
				min += time.tm_min;
				if(0 == min % period)
				{
					report_data_ptr = ptcl_report_data_req(SL427_BYTES_REPORT_DATA, RT_WAITING_FOREVER);
					if((PTCL_REPORT_DATA *)0 != report_data_ptr)
					{
						report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, 2, 1, 1, SL427_AFN_AUTO_REPORT);
						//���緽ʽ1
						report_data_ptr->pdata[report_data_ptr->data_len++] = (pwrsys_info_ptr->batt_value > pwrsys_info_ptr->acdc_value) ? SL427_PWR_TYPE_BATT : SL427_PWR_TYPE_ACDC;
						//�Ƿ���1
						report_data_ptr->pdata[report_data_ptr->data_len++] = (pwrsys_info_ptr->charging_sta) ? 1 : 0;
						//�Ƿ������Դ1
						report_data_ptr->pdata[report_data_ptr->data_len++] = (pwrsys_info_ptr->extpwr_sta) ? 1 : 0;
						//��ص�ѹ2
						*(rt_uint16_t *)(report_data_ptr->pdata + report_data_ptr->data_len) = pwrsys_info_ptr->batt_value;
						report_data_ptr->data_len += sizeof(rt_uint16_t);
						//15V��ѹ2
						*(rt_uint16_t *)(report_data_ptr->pdata + report_data_ptr->data_len) = pwrsys_info_ptr->acdc_value;
						report_data_ptr->data_len += sizeof(rt_uint16_t);
						//���У׼��ѹ2
						*(rt_uint16_t *)(report_data_ptr->pdata + report_data_ptr->data_len) = pwrsys_info_ptr->batadj_value;
						report_data_ptr->data_len += sizeof(rt_uint16_t);
						//̫���ܰ��ѹ2
						*(rt_uint16_t *)(report_data_ptr->pdata + report_data_ptr->data_len) = pwrsys_info_ptr->solar_value;
						report_data_ptr->data_len += sizeof(rt_uint16_t);
						//������2
						*(rt_uint16_t *)(report_data_ptr->pdata + report_data_ptr->data_len) = pwrsys_info_ptr->cc_value;
						report_data_ptr->data_len += sizeof(rt_uint16_t);
						//�ŵ����2
						*(rt_uint16_t *)(report_data_ptr->pdata + report_data_ptr->data_len) = pwrsys_info_ptr->dc_value;
						report_data_ptr->data_len += sizeof(rt_uint16_t);
						//������4
						*(rt_uint32_t *)(report_data_ptr->pdata + report_data_ptr->data_len) = 0;
						report_data_ptr->data_len += sizeof(rt_uint32_t);
						//��Դ��汾16
						rt_memset((void *)(report_data_ptr->pdata + report_data_ptr->data_len), 0, SL427_BYTES_PWR_SOFT_EDITION);
						min = rt_strlen((char *)pwrsys_info_ptr->soft_edition);
						if(min < SL427_BYTES_PWR_SOFT_EDITION)
						{
							rt_memmove((void *)(report_data_ptr->pdata + report_data_ptr->data_len), (void *)pwrsys_info_ptr->soft_edition, min);
						}
						report_data_ptr->data_len += SL427_BYTES_PWR_SOFT_EDITION;
						//���ذ汾16
						rt_memset((void *)(report_data_ptr->pdata + report_data_ptr->data_len), 0, SL427_BYTES_MAIN_SOFT_EDITION);
						min = rt_strlen(PTCL_SOFTWARE_EDITION);
						if(min < SL427_BYTES_MAIN_SOFT_EDITION)
						{
							rt_memmove((void *)(report_data_ptr->pdata + report_data_ptr->data_len), (void *)PTCL_SOFTWARE_EDITION, min);
						}
						report_data_ptr->data_len += SL427_BYTES_MAIN_SOFT_EDITION;
						//gsm�ź�ǿ��1
						report_data_ptr->pdata[report_data_ptr->data_len++] = 0;
						//fm�ź�ǿ��1
						report_data_ptr->pdata[report_data_ptr->data_len++] = si4702_get_rssi();
						//�ϱ�ʱ��6
						report_data_ptr->pdata[report_data_ptr->data_len++] = time.tm_year % 100;
						report_data_ptr->pdata[report_data_ptr->data_len++] = time.tm_mon + 1;
						report_data_ptr->pdata[report_data_ptr->data_len++] = time.tm_mday;
						report_data_ptr->pdata[report_data_ptr->data_len++] = time.tm_hour;
						report_data_ptr->pdata[report_data_ptr->data_len++] = time.tm_min;
						report_data_ptr->pdata[report_data_ptr->data_len++] = time.tm_sec;

						report_data_ptr->pdata[SL427_BYTES_GPRS_FRAME_HEAD - 1] = report_data_ptr->data_len - SL427_BYTES_GPRS_FRAME_HEAD;
						*(rt_uint16_t *)(report_data_ptr->pdata + report_data_ptr->data_len) = _sl427_crc_value(report_data_ptr->pdata, report_data_ptr->data_len);
						report_data_ptr->data_len += sizeof(rt_uint16_t);

						report_data_ptr->fun_csq_update	= (void *)_sl427_csq_update;
						report_data_ptr->data_id		= SL427_AFN_AUTO_REPORT;
						report_data_ptr->need_reply		= RT_FALSE;
						report_data_ptr->fcb_value		= 0;
						report_data_ptr->ptcl_type		= PTCL_PTCL_TYPE_SL427;

						if(RT_FALSE == ptcl_auto_report_post(report_data_ptr))
						{
							rt_mp_free((void *)report_data_ptr);
						}
					}
				}
			}

			rt_mp_free((void *)pwrsys_info_ptr);
		}
	}
}

static int _sl427_component_init(void)
{
	if(RT_EOK != rt_event_init(&m_sl427_event_module, "sl427", RT_IPC_FLAG_PRIO))
	{
		while(1);
	}
	if(RT_EOK != rt_event_send(&m_sl427_event_module, SL427_PARAM_ALL))
	{
		while(1);
	}
	
	//rtu addr
	_sl427_get_rtu_addr(m_sl427_rtu_addr);
	//�ϱ����
	m_sl427_report_period = _sl427_get_report_period();
	//����Ԥ����ִ
	m_sl427_sms_warn_receipt = _sl427_get_sms_warn_receipt();
	
	return 0;
}
INIT_COMPONENT_EXPORT(_sl427_component_init);

static int _sl427_app_init(void)
{
	//�Ա�����
	if(RT_EOK != rt_thread_init(&m_sl427_thread_report, "sl427", _sl427_task_report, (void *)0, (void *)m_sl427_stk_report, SL427_STK_REPORT, SL427_PRIO_REPORT, 2))
	{
		while(1);
	}
	if(RT_EOK != rt_thread_startup(&m_sl427_thread_report))
	{
		while(1);
	}
	
	return 0;
}
INIT_APP_EXPORT(_sl427_app_init);



//�������ö�ȡ
void sl427_get_rtu_addr(rt_uint8_t *rtu_addr_ptr)
{
	_sl427_param_pend(SL427_PARAM_RTU_ADDR);
	rt_memmove((void *)rtu_addr_ptr, (void *)m_sl427_rtu_addr, SL427_BYTES_RTU_ADDR);
	_sl427_param_post(SL427_PARAM_RTU_ADDR);
}
void sl427_set_rtu_addr(rt_uint8_t const *rtu_addr_ptr, rt_uint8_t addr_len)
{
	if(addr_len >= SL427_BYTES_RTU_ADDR)
	{
		return;
	}

	_sl427_param_pend(SL427_PARAM_RTU_ADDR);
	rt_memmove((void *)m_sl427_rtu_addr, (void *)rtu_addr_ptr, addr_len);
	m_sl427_rtu_addr[addr_len] = 0;
	_sl427_set_rtu_addr(m_sl427_rtu_addr);
	_sl427_param_post(SL427_PARAM_RTU_ADDR);
}

rt_uint16_t sl427_get_report_period(void)
{
	rt_uint16_t period;
	
	_sl427_param_pend(SL427_PARAM_REPORT_PERIOD);
	period = m_sl427_report_period;
	_sl427_param_post(SL427_PARAM_REPORT_PERIOD);

	return period;
}
void sl427_set_report_period(rt_uint16_t period)
{
	_sl427_param_pend(SL427_PARAM_REPORT_PERIOD);
	if(period != m_sl427_report_period)
	{
		m_sl427_report_period = period;
		_sl427_set_report_period(period);
	}
	_sl427_param_post(SL427_PARAM_REPORT_PERIOD);
}

rt_uint8_t sl427_get_sms_warn_receipt(void)
{
	rt_uint8_t en;
	
	_sl427_param_pend(SL427_PARAM_SMS_WARN_RECEIPT);
	en = m_sl427_sms_warn_receipt;
	_sl427_param_post(SL427_PARAM_SMS_WARN_RECEIPT);

	return en;
}
void sl427_set_sms_warn_receipt(rt_uint8_t en)
{
	if((RT_TRUE != en) && (RT_FALSE != en))
	{
		return;
	}
	
	_sl427_param_pend(SL427_PARAM_SMS_WARN_RECEIPT);
	if(en != m_sl427_sms_warn_receipt)
	{
		m_sl427_sms_warn_receipt = en;
		_sl427_set_sms_warn_receipt(en);
	}
	_sl427_param_post(SL427_PARAM_SMS_WARN_RECEIPT);
}

void sl427_param_restore(void)
{
	_sl427_param_pend(SL427_PARAM_ALL);

	//rtu addr
	rt_sprintf((char *)m_sl427_rtu_addr, "smt-h7660");
	_sl427_set_rtu_addr(m_sl427_rtu_addr);

	//�Ա����
	m_sl427_report_period = 60;
	_sl427_set_report_period(m_sl427_report_period);

	//����Ԥ����ִ
	m_sl427_sms_warn_receipt = RT_TRUE;
	_sl427_set_sms_warn_receipt(m_sl427_sms_warn_receipt);
	
	_sl427_param_post(SL427_PARAM_ALL);
}

//����
rt_uint16_t sl427_heart_encoder(rt_uint8_t *pdata, rt_uint16_t data_len, rt_uint8_t ch)
{
	rt_base_t level;
	
	if(data_len < 21)
	{
		return 0;
	}
	if(ch >= PTCL_NUM_CENTER)
	{
		return 0;
	}

	data_len = _sl427_bao_tou(pdata, 1, 1, 1, SL427_AFN_HEART);
	
	level = rt_hw_interrupt_disable();
	pdata[data_len++] = m_sl427_pwr_type;
	rt_hw_interrupt_enable(level);
	
	pdata[SL427_BYTES_GPRS_FRAME_HEAD - 1] = data_len - SL427_BYTES_GPRS_FRAME_HEAD;
	*(rt_uint16_t *)(pdata + data_len) = _sl427_crc_value(pdata, data_len);
	data_len += sizeof(rt_uint16_t);

	return data_len;
}

//smsЭ�����
rt_uint8_t sl427_sms_data_decoder(PTCL_REPORT_DATA *report_data_ptr, PTCL_RECV_DATA const *recv_data_ptr, PTCL_PARAM_INFO **param_info_ptr_ptr)
{
	rt_uint8_t			cmd_type, is_unicode = RT_FALSE;
	rt_uint16_t			i, j;
	static rt_uint8_t	frame_head[SL427_BYTES_SMS_FRAME_HEAD + 1];
	PTCL_PARAM_INFO		*param_info_ptr;
	void				*pmem;

	if(0 == recv_data_ptr->data_len)
	{
		return RT_FALSE;
	}

	//�ҳ�֡ͷ
	j = 0;
	for(i = 0; i < recv_data_ptr->data_len; i++)
	{
		if(0 != recv_data_ptr->pdata[i])
		{
			frame_head[j++] = recv_data_ptr->pdata[i];
			if(j >= SL427_BYTES_SMS_FRAME_HEAD)
			{
				break;
			}
		}
		else
		{
			is_unicode = RT_TRUE;
		}
	}
	//ʶ��ָ��
	frame_head[j] = 0;
	cmd_type = _sl427_cmd_type_identify((char *)frame_head);
	//δʶ��ָ��
	if(cmd_type >= SL427_NUM_CMD)
	{
		return RT_FALSE;
	}
	report_data_ptr->data_len = 0;
	//�������
	for(i = 0; i < j; i++)
	{
		if(*(rt_uint8_t *)m_sl427_cmd_type[cmd_type] == frame_head[i])
		{
			break;
		}
		report_data_ptr->pdata[report_data_ptr->data_len++] = frame_head[i];
	}
	//������ʼλ��
	*frame_head = *((rt_uint8_t *)m_sl427_cmd_type[cmd_type] + rt_strlen(m_sl427_cmd_type[cmd_type]) - 1);
	j = _sl427_num_cmd_tail(cmd_type);
	for(i = 0; i < recv_data_ptr->data_len; i++)
	{
		if(*frame_head == recv_data_ptr->pdata[i])
		{
			j--;
			if(0 == j)
			{
				i++;
				break;
			}
		}
	}
	//����
	switch(cmd_type)
	{
	//�����澯
	case SL427_CMD_BJ:
		//�澯����
		*frame_head = 0;
		if(RT_FALSE == is_unicode)
		{
			for(j = 0; j < 2; j++)
			{
				if((i + 1) > recv_data_ptr->data_len)
				{
					report_data_ptr->data_len = 0;
					return RT_TRUE;
				}
				if((recv_data_ptr->pdata[i] >= '0') && (recv_data_ptr->pdata[i] <= '9'))
				{
					*frame_head *= 10;
					*frame_head += (recv_data_ptr->pdata[i++] - '0');
				}
				else
				{
					break;
				}
			}
		}
		else
		{
			for(j = 0; j < 2; j++)
			{
				if((i + 2) > recv_data_ptr->data_len)
				{
					report_data_ptr->data_len = 0;
					return RT_TRUE;
				}
				if(0 != recv_data_ptr->pdata[i])
				{
					break;
				}
				if((recv_data_ptr->pdata[i + 1] >= '0') && (recv_data_ptr->pdata[i + 1] <= '9'))
				{
					*frame_head *= 10;
					*frame_head += (recv_data_ptr->pdata[i + 1] - '0');
					i += 2;
				}
				else
				{
					break;
				}
			}
		}
		//�����澯
		if(i >= recv_data_ptr->data_len)
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		if(RT_FALSE == wm_warn_tts_trigger(WM_WARN_TYPE_SMS, *frame_head, (RT_TRUE == is_unicode) ? DTU_TTS_DATA_TYPE_UCS2 : DTU_TTS_DATA_TYPE_GBK, recv_data_ptr->pdata + i, recv_data_ptr->data_len - i, RT_FALSE))
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		//�ظ�
		if(RT_TRUE == sl427_get_sms_warn_receipt())
		{
			report_data_ptr->data_len += rt_sprintf((char *)(report_data_ptr->pdata + report_data_ptr->data_len), "%s%d", m_sl427_cmd_type[cmd_type], *frame_head);
		}
		else
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		break;
	//��ӹ����
	case SL427_CMD_JGLH:
	//�����Ȩ��
	case SL427_CMD_JSQH:
		if((RT_TRUE == is_unicode) || (RT_FALSE == dtu_is_previlige_phone(recv_data_ptr->ch)))
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		if(i >= recv_data_ptr->data_len)
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		j = i;
		while(j < recv_data_ptr->data_len)
		{
			if(',' == recv_data_ptr->pdata[j])
			{
				if(i < j)
				{
					if((j - i) <= DTU_BYTES_SMS_ADDR)
					{
						param_info_ptr = ptcl_param_info_req(sizeof(DTU_SMS_ADDR), RT_WAITING_NO);
						if((PTCL_PARAM_INFO *)0 != param_info_ptr)
						{
							param_info_ptr->param_type	= PTCL_PARAM_INFO_ADD_SUPER_PHONE;
							param_info_ptr->data_len	= (cmd_type == SL427_CMD_JGLH) ? RT_TRUE : RT_FALSE;
							((DTU_SMS_ADDR *)(param_info_ptr->pdata))->addr_len = j - i;
							rt_memmove((void *)(((DTU_SMS_ADDR *)(param_info_ptr->pdata))->addr_data), (void *)(recv_data_ptr->pdata + i), j - i);
							ptcl_param_info_add(param_info_ptr_ptr, param_info_ptr);
						}
					}
				}
				j++;
				i = j;
			}
			else
			{
				j++;
			}
		}
		if(i < j)
		{
			if((j - i) <= DTU_BYTES_SMS_ADDR)
			{
				param_info_ptr = ptcl_param_info_req(sizeof(DTU_SMS_ADDR), RT_WAITING_NO);
				if((PTCL_PARAM_INFO *)0 != param_info_ptr)
				{
					param_info_ptr->param_type	= PTCL_PARAM_INFO_ADD_SUPER_PHONE;
					param_info_ptr->data_len	= (cmd_type == SL427_CMD_JGLH) ? RT_TRUE : RT_FALSE;
					((DTU_SMS_ADDR *)(param_info_ptr->pdata))->addr_len = j - i;
					rt_memmove((void *)(((DTU_SMS_ADDR *)(param_info_ptr->pdata))->addr_data), (void *)(recv_data_ptr->pdata + i), j - i);
					ptcl_param_info_add(param_info_ptr_ptr, param_info_ptr);
				}
			}
		}
		//�ظ�
		report_data_ptr->data_len += rt_sprintf((char *)(report_data_ptr->pdata + report_data_ptr->data_len), "%s", m_sl427_cmd_type[cmd_type]);
		break;
	//��ѯ�����
	case SL427_CMD_CGLH:
	//��ѯ��Ȩ��
	case SL427_CMD_CSQH:
		if((RT_TRUE == is_unicode) || (RT_FALSE == dtu_is_previlige_phone(recv_data_ptr->ch)))
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		pmem = mempool_req(sizeof(DTU_SMS_ADDR), RT_WAITING_NO);
		if((void *)0 == pmem)
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		if(SL427_CMD_CGLH == cmd_type)
		{
			i = 0;
			j = DTU_NUM_PREVILIGE_PHONE;
		}
		else
		{
			i = DTU_NUM_PREVILIGE_PHONE;
			j = DTU_NUM_SUPER_PHONE;
		}
		report_data_ptr->data_len += rt_sprintf((char *)(report_data_ptr->pdata + report_data_ptr->data_len), "%s", m_sl427_cmd_type[cmd_type]);
		while(i < j)
		{
			dtu_get_super_phone((DTU_SMS_ADDR *)pmem, i++);
			if(0 != ((DTU_SMS_ADDR *)pmem)->addr_len)
			{
				if((report_data_ptr->data_len + ((DTU_SMS_ADDR *)pmem)->addr_len) < PTCL_BYTES_ACK_REPORT)
				{
					rt_memmove((void *)(report_data_ptr->pdata + report_data_ptr->data_len), (void *)(((DTU_SMS_ADDR *)pmem)->addr_data), ((DTU_SMS_ADDR *)pmem)->addr_len);
					report_data_ptr->data_len += ((DTU_SMS_ADDR *)pmem)->addr_len;
					report_data_ptr->pdata[report_data_ptr->data_len++] = ',';
				}
			}
		}
		rt_mp_free(pmem);
		break;
	//ɾ�������
	case SL427_CMD_SGLH:
	//ɾ����Ȩ��
	case SL427_CMD_SSQH:
		if((RT_TRUE == is_unicode) || (RT_FALSE == dtu_is_previlige_phone(recv_data_ptr->ch)))
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		if(i >= recv_data_ptr->data_len)
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		j = i;
		while(j < recv_data_ptr->data_len)
		{
			if(',' == recv_data_ptr->pdata[j])
			{
				if(i < j)
				{
					if((j - i) <= DTU_BYTES_SMS_ADDR)
					{
						param_info_ptr = ptcl_param_info_req(sizeof(DTU_SMS_ADDR), RT_WAITING_NO);
						if((PTCL_PARAM_INFO *)0 != param_info_ptr)
						{
							param_info_ptr->param_type	= PTCL_PARAM_INFO_DEL_SUPER_PHONE;
							((DTU_SMS_ADDR *)(param_info_ptr->pdata))->addr_len = j - i;
							rt_memmove((void *)(((DTU_SMS_ADDR *)(param_info_ptr->pdata))->addr_data), (void *)(recv_data_ptr->pdata + i), j - i);
							ptcl_param_info_add(param_info_ptr_ptr, param_info_ptr);
						}
					}
				}
				j++;
				i = j;
			}
			else
			{
				j++;
			}
		}
		if(i < j)
		{
			if((j - i) <= DTU_BYTES_SMS_ADDR)
			{
				param_info_ptr = ptcl_param_info_req(sizeof(DTU_SMS_ADDR), RT_WAITING_NO);
				if((PTCL_PARAM_INFO *)0 != param_info_ptr)
				{
					param_info_ptr->param_type	= PTCL_PARAM_INFO_DEL_SUPER_PHONE;
					((DTU_SMS_ADDR *)(param_info_ptr->pdata))->addr_len = j - i;
					rt_memmove((void *)(((DTU_SMS_ADDR *)(param_info_ptr->pdata))->addr_data), (void *)(recv_data_ptr->pdata + i), j - i);
					ptcl_param_info_add(param_info_ptr_ptr, param_info_ptr);
				}
			}
		}
		//�ظ�
		report_data_ptr->data_len += rt_sprintf((char *)(report_data_ptr->pdata + report_data_ptr->data_len), "%s", m_sl427_cmd_type[cmd_type]);
		break;
	//����fmƵ��
	case SL427_CMD_SZPL:
		if((RT_TRUE == is_unicode) || (RT_FALSE == dtu_is_previlige_phone(recv_data_ptr->ch)))
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		if(i >= recv_data_ptr->data_len)
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		param_info_ptr = ptcl_param_info_req(sizeof(rt_uint16_t), RT_WAITING_NO);
		if((PTCL_PARAM_INFO *)0 == param_info_ptr)
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		param_info_ptr->param_type = PTCL_PARAM_INFO_FM_FREQUENCY;
		*(rt_uint16_t *)(param_info_ptr->pdata) = fyyp_str_to_hex(recv_data_ptr->pdata + i, recv_data_ptr->data_len - i);
		ptcl_param_info_add(param_info_ptr_ptr, param_info_ptr);
		//�ظ�
		report_data_ptr->data_len += rt_sprintf((char *)(report_data_ptr->pdata + report_data_ptr->data_len), "%s", m_sl427_cmd_type[cmd_type]);
		break;
	//��ѯfmƵ��
	case SL427_CMD_CXPL:
		if((RT_TRUE == is_unicode) || (RT_FALSE == dtu_is_previlige_phone(recv_data_ptr->ch)))
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		report_data_ptr->data_len += rt_sprintf((char *)(report_data_ptr->pdata + report_data_ptr->data_len), "%s%d", m_sl427_cmd_type[cmd_type], wm_get_fm_frequency());
		break;
	//����ip
	case SL427_CMD_SZIP:
		if((RT_TRUE == is_unicode) || (RT_FALSE == dtu_is_previlige_phone(recv_data_ptr->ch)))
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		if(i >= recv_data_ptr->data_len)
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		j = recv_data_ptr->data_len - i;
		if(j > DTU_BYTES_IP_ADDR)
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		param_info_ptr = ptcl_param_info_req(sizeof(DTU_IP_ADDR), RT_WAITING_NO);
		if((PTCL_PARAM_INFO *)0 == param_info_ptr)
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		param_info_ptr->param_type = PTCL_PARAM_INFO_IP_ADDR;
		param_info_ptr->data_len = 0;
		((DTU_IP_ADDR *)(param_info_ptr->pdata))->addr_len = j;
		rt_memmove((void *)(((DTU_IP_ADDR *)(param_info_ptr->pdata))->addr_data), (void *)(recv_data_ptr->pdata + i), j);
		for(i = 0; i < j; i++)
		{
			if(',' == ((DTU_IP_ADDR *)(param_info_ptr->pdata))->addr_data[i])
			{
				((DTU_IP_ADDR *)(param_info_ptr->pdata))->addr_data[i] = ':';
			}
		}
		ptcl_param_info_add(param_info_ptr_ptr, param_info_ptr);
		//�ظ�
		report_data_ptr->data_len += rt_sprintf((char *)(report_data_ptr->pdata + report_data_ptr->data_len), "%s", m_sl427_cmd_type[cmd_type]);
		break;
	//��ѯip
	case SL427_CMD_CXIP:
		if((RT_TRUE == is_unicode) || (RT_FALSE == dtu_is_previlige_phone(recv_data_ptr->ch)))
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		report_data_ptr->data_len += rt_sprintf((char *)(report_data_ptr->pdata + report_data_ptr->data_len), "%s", m_sl427_cmd_type[cmd_type]);
		if(RT_TRUE == dtu_get_ip_ch(0))
		{
			pmem = mempool_req(sizeof(DTU_IP_ADDR), RT_WAITING_NO);
			if((void *)0 == pmem)
			{
				report_data_ptr->data_len = 0;
				return RT_TRUE;
			}
			dtu_get_ip_addr((DTU_IP_ADDR *)pmem, 0);
			for(j = 0; j < ((DTU_IP_ADDR *)pmem)->addr_len; j++)
			{
				if(':' == ((DTU_IP_ADDR *)pmem)->addr_data[j])
				{
					((DTU_IP_ADDR *)pmem)->addr_data[j] = ',';
				}
			}
			rt_memmove((void *)(report_data_ptr->pdata + report_data_ptr->data_len), (void *)(((DTU_IP_ADDR *)pmem)->addr_data), ((DTU_IP_ADDR *)pmem)->addr_len);
			report_data_ptr->data_len += ((DTU_IP_ADDR *)pmem)->addr_len;
			rt_mp_free(pmem);
		}
		break;
	//���ý����澯����
	case SL427_CMD_SZJJ:
		if(RT_FALSE == dtu_is_previlige_phone(recv_data_ptr->ch))
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		if(i >= recv_data_ptr->data_len)
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		j = recv_data_ptr->data_len - i;
		if(j > WM_BYTES_PRESET_WARN_INFO)
		{
			report_data_ptr->data_len = 0;
			return RT_TRUE;
		}
		wm_set_preset_warn_info(recv_data_ptr->pdata + i, j, (RT_TRUE == is_unicode) ? DTU_TTS_DATA_TYPE_UCS2 : DTU_TTS_DATA_TYPE_GBK);
		report_data_ptr->data_len += rt_sprintf((char *)(report_data_ptr->pdata + report_data_ptr->data_len), "%s", m_sl427_cmd_type[cmd_type]);
		break;
	default:
		report_data_ptr->data_len = 0;
		return RT_TRUE;
	}

	report_data_ptr->fun_csq_update		= (void *)0;
	report_data_ptr->data_id			= 0;
	report_data_ptr->need_reply			= RT_FALSE;
	report_data_ptr->fcb_value			= 0;
	report_data_ptr->ptcl_type			= PTCL_PTCL_TYPE_SL427;

	return RT_TRUE;
}

//gprsЭ�����
rt_uint8_t sl427_gprs_data_decoder(PTCL_REPORT_DATA *report_data_ptr, PTCL_RECV_DATA const *recv_data_ptr, PTCL_PARAM_INFO **param_info_ptr_ptr)
{
	rt_uint16_t				i, crc_value;
	rt_uint8_t				id, packet_total, packet_cur, afn, data_len;
	void					*pmem;
	struct tm				time;
	static SL427_WARN_INFO	*warn_info_ptr = (SL427_WARN_INFO *)0;
	rt_uint32_t				water_value;

	//crc
	if(recv_data_ptr->data_len <= SL427_BYTES_GPRS_FRAME_END)
	{
		return RT_FALSE;
	}
	i = recv_data_ptr->data_len - SL427_BYTES_GPRS_FRAME_END;
	crc_value = _sl427_crc_value(recv_data_ptr->pdata, i);
	if((crc_value != *(rt_uint16_t *)(recv_data_ptr->pdata + i)) && (0xffff != *(rt_uint16_t *)(recv_data_ptr->pdata + i)))
	{
		return RT_FALSE;
	}
	//֡ͷ
	i = 0;
	//Э��id
	if((i + SL427_BYTES_GPRS_FRAME_HEAD) > recv_data_ptr->data_len)
	{
		return RT_FALSE;
	}
	if(SL427_FRAME_HEAD_1 != recv_data_ptr->pdata[i++])
	{
		return RT_FALSE;
	}
	if(SL427_FRAME_HEAD_2 != recv_data_ptr->pdata[i++])
	{
		return RT_FALSE;
	}
	//����id
	id = recv_data_ptr->pdata[i++];
	//�豸��ַ
	i += SL427_BYTES_RTU_ADDR;
	//�ܰ����͵�ǰ����
	packet_total = recv_data_ptr->pdata[i++];
	packet_cur = recv_data_ptr->pdata[i++];
	if((0 == packet_cur) || (packet_cur > packet_total))
	{
		return RT_FALSE;
	}
	//������
	afn = recv_data_ptr->pdata[i++];
	//�����򳤶�
	data_len = recv_data_ptr->pdata[i++];
	if((i + data_len + SL427_BYTES_GPRS_FRAME_END) != recv_data_ptr->data_len)
	{
		return RT_FALSE;
	}

	//����
	switch(afn)
	{
	case SL427_AFN_QUERY_SUPER_PHONE:
		while(1)
		{
			if(data_len)
			{
				report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
				report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DATA_LEN;
				break;
			}
			pmem = mempool_req(sizeof(DTU_SMS_ADDR), RT_WAITING_NO);
			if((void *)0 == pmem)
			{
				report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
				report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DEVICE;
				break;
			}
			report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn);
			for(packet_total = 0; packet_total < DTU_NUM_SUPER_PHONE; packet_total++)
			{
				dtu_get_super_phone((DTU_SMS_ADDR *)pmem, packet_total);
				if((0 != ((DTU_SMS_ADDR *)pmem)->addr_len) && (((DTU_SMS_ADDR *)pmem)->addr_len < 20))
				{
					if((report_data_ptr->data_len + 22 + SL427_BYTES_GPRS_FRAME_END) <= PTCL_BYTES_ACK_REPORT)
					{
						if((report_data_ptr->data_len + 22 - SL427_BYTES_GPRS_FRAME_HEAD) <= 0xff)
						{
							rt_memset((void *)(report_data_ptr->pdata + report_data_ptr->data_len), 0, 20);
							rt_memcpy((void *)(report_data_ptr->pdata + report_data_ptr->data_len), (void *)(((DTU_SMS_ADDR *)pmem)->addr_data), ((DTU_SMS_ADDR *)pmem)->addr_len);
							report_data_ptr->data_len += 20;
							if(packet_total < DTU_NUM_PREVILIGE_PHONE)
							{
								report_data_ptr->pdata[report_data_ptr->data_len++] = 1;
								report_data_ptr->pdata[report_data_ptr->data_len++] = 0;
							}
							else
							{
								report_data_ptr->pdata[report_data_ptr->data_len++] = 0;
								report_data_ptr->pdata[report_data_ptr->data_len++] = 1;
							}
						}
						else
						{
							break;
						}
					}
					else
					{
						break;
					}
				}
			}
			rt_mp_free(pmem);
			
			break;
		}
		break;
	case SL427_AFN_ADD_SUPER_PHONE:
		while(1)
		{
			if(data_len < 15)
			{
				report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
				report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DATA_LEN;
				break;
			}
			//���볤��
			packet_total = rt_strlen((char *)(recv_data_ptr->pdata + i));
			if(packet_total >= 13)
			{
				report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
				report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DATA_VALUE;
				break;
			}
			//����
			pmem = (void *)ptcl_param_info_req(sizeof(DTU_SMS_ADDR), RT_WAITING_NO);
			if((void *)0 == pmem)
			{
				report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
				report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DEVICE;
				break;
			}
			((PTCL_PARAM_INFO *)pmem)->param_type = PTCL_PARAM_INFO_ADD_SUPER_PHONE;
			((DTU_SMS_ADDR *)(((PTCL_PARAM_INFO *)pmem)->pdata))->addr_len = packet_total;
			rt_memcpy((void *)(((DTU_SMS_ADDR *)(((PTCL_PARAM_INFO *)pmem)->pdata))->addr_data), (void *)(recv_data_ptr->pdata + i), packet_total);
			i += 13;
			if(recv_data_ptr->pdata[i++])
			{
				((PTCL_PARAM_INFO *)pmem)->data_len = RT_TRUE;
				ptcl_param_info_add(param_info_ptr_ptr, ((PTCL_PARAM_INFO *)pmem));

				report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn);
				break;
			}
			if(recv_data_ptr->pdata[i])
			{
				((PTCL_PARAM_INFO *)pmem)->data_len = RT_FALSE;
				ptcl_param_info_add(param_info_ptr_ptr, ((PTCL_PARAM_INFO *)pmem));

				report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn);
				break;
			}
			rt_mp_free(pmem);
			
			report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
			report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DATA_VALUE;
			
			break;
		}
		break;
	case SL427_AFN_DEL_SUPER_PHONE:
		while(1)
		{
			if(data_len < 15)
			{
				report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
				report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DATA_LEN;
				break;
			}
			//���볤��
			packet_total = rt_strlen((char *)(recv_data_ptr->pdata + i));
			if(packet_total >= 13)
			{
				report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
				report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DATA_VALUE;
				break;
			}
			//����
			pmem = (void *)ptcl_param_info_req(sizeof(DTU_SMS_ADDR), RT_WAITING_NO);
			if((void *)0 == pmem)
			{
				report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
				report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DEVICE;
				break;
			}
			((PTCL_PARAM_INFO *)pmem)->param_type = PTCL_PARAM_INFO_DEL_SUPER_PHONE;
			((DTU_SMS_ADDR *)(((PTCL_PARAM_INFO *)pmem)->pdata))->addr_len = packet_total;
			rt_memcpy((void *)(((DTU_SMS_ADDR *)(((PTCL_PARAM_INFO *)pmem)->pdata))->addr_data), (void *)(recv_data_ptr->pdata + i), packet_total);
			ptcl_param_info_add(param_info_ptr_ptr, ((PTCL_PARAM_INFO *)pmem));
			report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn);
			
			break;
		}
		break;
	case SL427_AFN_REQUEST_AD:
	case SL427_AFN_REQUEST_WEATHER:
		while(1)
		{
			if(data_len <= 1)
			{
				report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
				report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DATA_LEN;
				break;
			}
			data_len -= 1;
			//�����ı�����
			if(SL427_WARN_DATA_TYPE_ASCII != recv_data_ptr->pdata[i++])
			{
				report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
				report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_WARN_DATA_TYPE;
				break;
			}
			//ִ��
			if(SL427_AFN_REQUEST_AD == afn)
			{
				packet_total = bx5k_set_ad_txt(recv_data_ptr->pdata + i, data_len);
			}
			else
			{
				packet_total = bx5k_set_weather_txt(recv_data_ptr->pdata + i, data_len);
			}
			//���
			if(RT_TRUE == packet_total)
			{
				report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn);
			}
			else
			{
				report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
				report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DEVICE;
			}
			
			break;
		}
		break;
	case SL427_AFN_QUERY_SYS_PARAM:
		if(0 == data_len)
		{
			report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn);
			//��������
			rt_snprintf((char *)(report_data_ptr->pdata + report_data_ptr->data_len), 13, "---");
			report_data_ptr->data_len += 13;
			//�豸id
			pmem = mempool_req(12, RT_WAITING_FOREVER);
			if((void *)0 == pmem)
			{
				rt_sprintf((char *)(report_data_ptr->pdata + report_data_ptr->data_len), "00112233445566778899aabb");
			}
			else
			{
				HAL_GetUID((rt_uint32_t *)pmem);
				for(packet_total = 0; packet_total < 12; packet_total++)
				{
					rt_sprintf((char *)(report_data_ptr->pdata + report_data_ptr->data_len + packet_total * 2), "%02x", *((rt_uint8_t *)pmem + packet_total));
				}
				rt_mp_free(pmem);
			}
			report_data_ptr->data_len += 25;
			//�豸��ַ
			_sl427_param_pend(SL427_PARAM_RTU_ADDR);
			rt_memcpy((void *)(report_data_ptr->pdata + report_data_ptr->data_len), (void *)m_sl427_rtu_addr, SL427_BYTES_RTU_ADDR);
			_sl427_param_post(SL427_PARAM_RTU_ADDR);
			report_data_ptr->data_len += SL427_BYTES_RTU_ADDR;
			//�绰���ȼ�
			report_data_ptr->pdata[report_data_ptr->data_len++] = wm_get_warn_priority(WM_WARN_TYPE_TELEPHONE);
			//�������ȼ�
			report_data_ptr->pdata[report_data_ptr->data_len++] = wm_get_warn_priority(WM_WARN_TYPE_SMS);
			//gprs���ȼ�
			report_data_ptr->pdata[report_data_ptr->data_len++] = wm_get_warn_priority(WM_WARN_TYPE_GPRS);
			//microphone���ȼ�
			report_data_ptr->pdata[report_data_ptr->data_len++] = wm_get_warn_priority(WM_WARN_TYPE_MICROPHONE);
			//�Խ������ȼ�
			report_data_ptr->pdata[report_data_ptr->data_len++] = wm_get_warn_priority(WM_WARN_TYPE_INTERPHONE);
			//fm���ȼ�
			report_data_ptr->pdata[report_data_ptr->data_len++] = wm_get_warn_priority(WM_WARN_TYPE_FM);
			//mp3���ȼ�
			report_data_ptr->pdata[report_data_ptr->data_len++] = wm_get_warn_priority(WM_WARN_TYPE_MP3);
			//�������ȼ�
			report_data_ptr->pdata[report_data_ptr->data_len++] = wm_get_warn_priority(WM_WARN_TYPE_AUX);
			//��������
			report_data_ptr->pdata[report_data_ptr->data_len++] = wm_get_warn_times();
			//ip��ַ
			pmem = mempool_req(sizeof(DTU_IP_ADDR), RT_WAITING_FOREVER);
			if((void *)0 == pmem)
			{
				rt_snprintf((char *)(report_data_ptr->pdata + report_data_ptr->data_len), 22, "---");
			}
			else
			{
				dtu_get_ip_addr((DTU_IP_ADDR *)pmem, 0);
				if(((DTU_IP_ADDR *)pmem)->addr_len < 22)
				{
					rt_memcpy((void *)(report_data_ptr->pdata + report_data_ptr->data_len), (void *)(((DTU_IP_ADDR *)pmem)->addr_data), ((DTU_IP_ADDR *)pmem)->addr_len);
					report_data_ptr->pdata[report_data_ptr->data_len + ((DTU_IP_ADDR *)pmem)->addr_len] = 0;
				}
				else
				{
					rt_snprintf((char *)(report_data_ptr->pdata + report_data_ptr->data_len), 22, "---");
				}
				rt_mp_free(pmem);
			}
			report_data_ptr->data_len += 22;
			//�ϱ����
			_sl427_param_pend(SL427_PARAM_REPORT_PERIOD);
			report_data_ptr->pdata[report_data_ptr->data_len++] = m_sl427_report_period;
			_sl427_param_post(SL427_PARAM_REPORT_PERIOD);
			//�Ƿ��ִ
			report_data_ptr->pdata[report_data_ptr->data_len++] = 0;
			//fmƵ��
			rt_snprintf((char *)(report_data_ptr->pdata + report_data_ptr->data_len), 6, "%d", wm_get_fm_frequency());
			report_data_ptr->data_len += 6;
			//����
			report_data_ptr->pdata[report_data_ptr->data_len++] = wm_get_fm_rssi_threshold();
			//̫���ܰ�
			report_data_ptr->pdata[report_data_ptr->data_len++] = 0;
		}
		else
		{
			report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
			report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DATA_LEN;
		}
		break;
	case SL427_AFN_REQUEST_WARN_EX:
		while(1)
		{
			if(data_len <= 8)
			{
				report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
				report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DATA_LEN;
				break;
			}
			data_len -= 8;
			//�����ı�����
			if((SL427_WARN_DATA_TYPE_ASCII != recv_data_ptr->pdata[i]) && (SL427_WARN_DATA_TYPE_UNICODE != recv_data_ptr->pdata[i]))
			{
				report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
				report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_WARN_DATA_TYPE;
				break;
			}
			packet_total = (SL427_WARN_DATA_TYPE_ASCII == recv_data_ptr->pdata[i++]) ? DTU_TTS_DATA_TYPE_GBK : DTU_TTS_DATA_TYPE_UCS2;
			//��������
			packet_cur = recv_data_ptr->pdata[i++];
			//ˮλ�ȼ�
			i += 2;
			//ˮλֵ
			water_value = (rt_uint32_t)(*(float *)(recv_data_ptr->pdata + i) * 1000);
			i += 4;
			//���󱨾�
			if(RT_TRUE == wm_warn_tts_trigger(WM_WARN_TYPE_GPRS, packet_cur, packet_total, recv_data_ptr->pdata + i, data_len, (DTU_TTS_DATA_TYPE_UCS2 == packet_total) ? RT_TRUE : RT_FALSE))
			{
				report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn);
				bx5k_set_screen_txt(recv_data_ptr->pdata + i, data_len);
				bx5k_set_screen_data(water_value);
			}
			else
			{
				report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
				report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DEVICE;
			}
			
			break;
		}
		break;
	//���󱨾�
	case SL427_AFN_REQUEST_WARN:
		//����
		if(1 == packet_total)
		{
			while(1)
			{
				if(data_len <= 2)
				{
					report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
					report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DATA_LEN;
					break;
				}
				data_len -= 2;
				//�����ı�����
				if((SL427_WARN_DATA_TYPE_ASCII != recv_data_ptr->pdata[i]) && (SL427_WARN_DATA_TYPE_UNICODE != recv_data_ptr->pdata[i]))
				{
					report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
					report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_WARN_DATA_TYPE;
					break;
				}
				packet_total = (SL427_WARN_DATA_TYPE_ASCII == recv_data_ptr->pdata[i++]) ? DTU_TTS_DATA_TYPE_GBK : DTU_TTS_DATA_TYPE_UCS2;
				//��������
				packet_cur = recv_data_ptr->pdata[i++];
				//���󱨾�
				if(RT_TRUE == wm_warn_tts_trigger(WM_WARN_TYPE_GPRS, packet_cur, packet_total, recv_data_ptr->pdata + i, data_len, (DTU_TTS_DATA_TYPE_UCS2 == packet_total) ? RT_TRUE : RT_FALSE))
				{
					report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn);
					bx5k_set_screen_txt(recv_data_ptr->pdata + i, data_len);
				}
				else
				{
					report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
					report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DEVICE;
				}

				break;
			}
		}
		//����еĵ�һ��
		else if(1 == packet_cur)
		{
			if((SL427_WARN_INFO *)0 != warn_info_ptr)
			{
				rt_mp_free((void *)warn_info_ptr);
				warn_info_ptr = (SL427_WARN_INFO *)0;
			}
			while(1)
			{
				if(data_len <= 2)
				{
					report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, packet_total, packet_cur, afn + 0x80);
					report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DATA_LEN;
					break;
				}
				data_len -= 2;
				//�����ı�����
				if((SL427_WARN_DATA_TYPE_ASCII != recv_data_ptr->pdata[i]) && (SL427_WARN_DATA_TYPE_UNICODE != recv_data_ptr->pdata[i]))
				{
					report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, packet_total, packet_cur, afn + 0x80);
					report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_WARN_DATA_TYPE;
					break;
				}
				//�����ı�����
				if(data_len > SL427_BYTES_WARN_INFO_MAX)
				{
					report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, packet_total, packet_cur, afn + 0x80);
					report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_WARN_TOO_LONG;
					break;
				}
				//����ռ�
				warn_info_ptr = _sl427_warn_info_req(SL427_BYTES_WARN_INFO_MAX, RT_WAITING_NO);
				if((SL427_WARN_INFO *)0 == warn_info_ptr)
				{
					report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, packet_total, packet_cur, afn + 0x80);
					report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DEVICE;
					break;
				}
				//���ݴ���
				warn_info_ptr->data_type	= (SL427_WARN_DATA_TYPE_ASCII == recv_data_ptr->pdata[i++]) ? DTU_TTS_DATA_TYPE_GBK : DTU_TTS_DATA_TYPE_UCS2;
				warn_info_ptr->warn_times	= recv_data_ptr->pdata[i++];
				warn_info_ptr->packet_total	= packet_total;
				warn_info_ptr->packet_cur	= packet_cur;
				warn_info_ptr->data_len		= data_len;
				rt_memmove((void *)warn_info_ptr->pdata, (void *)(recv_data_ptr->pdata + i), data_len);
				//�ظ�
				report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, packet_total, packet_cur, afn + 0x80);
				report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_NONE;

				break;
			}
		}
		//����е������
		else
		{
			while(1)
			{
				//�澯��ϢΪ��
				if((SL427_WARN_INFO *)0 == warn_info_ptr)
				{
					report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, packet_total, packet_cur, afn + 0x80);
					report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DATA_VALUE;
					break;
				}
				//�ܰ�������
				if(packet_total != warn_info_ptr->packet_total)
				{
					report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, packet_total, packet_cur, afn + 0x80);
					report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DATA_VALUE;
					break;
				}
				//��ǰ��������
				if(packet_cur != (warn_info_ptr->packet_cur + 1))
				{
					report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, packet_total, packet_cur, afn + 0x80);
					report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DATA_VALUE;
					break;
				}
				//���ݳ���
				if((warn_info_ptr->data_len + data_len) > SL427_BYTES_WARN_INFO_MAX)
				{
					report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, packet_total, packet_cur, afn + 0x80);
					report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_WARN_TOO_LONG;
					break;
				}
				//���ݴ���
				warn_info_ptr->packet_cur	= packet_cur;
				rt_memmove((void *)(warn_info_ptr->pdata + warn_info_ptr->data_len), (void *)(recv_data_ptr->pdata + i), data_len);
				warn_info_ptr->data_len		+= data_len;
				//�������һ��
				if(warn_info_ptr->packet_total != warn_info_ptr->packet_cur)
				{
					report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, packet_total, packet_cur, afn + 0x80);
					report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_NONE;
					break;
				}
				//�����澯�ɹ�
				if(RT_TRUE == wm_warn_tts_trigger(WM_WARN_TYPE_GPRS, warn_info_ptr->warn_times, warn_info_ptr->data_type, warn_info_ptr->pdata, warn_info_ptr->data_len, (DTU_TTS_DATA_TYPE_UCS2 == warn_info_ptr->data_type) ? RT_TRUE : RT_FALSE))
				{
					report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn);
					bx5k_set_screen_txt(warn_info_ptr->pdata, warn_info_ptr->data_len);
				}
				//�����澯ʧ��
				else
				{
					report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, packet_total, packet_cur, afn + 0x80);
					report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DEVICE;
				}
				//�ͷŸ澯��Ϣ
				rt_mp_free((void *)warn_info_ptr);
				warn_info_ptr = (SL427_WARN_INFO *)0;

				break;
			}
		}
		break;
	//��ѯʱ��
	case SL427_AFN_QUERY_TIME:
		if(0 == data_len)
		{
			time = rtcee_rtc_get_calendar();
			report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn);
			report_data_ptr->pdata[report_data_ptr->data_len++] = time.tm_year % 100;
			report_data_ptr->pdata[report_data_ptr->data_len++] = time.tm_mon + 1;
			report_data_ptr->pdata[report_data_ptr->data_len++] = time.tm_mday;
			report_data_ptr->pdata[report_data_ptr->data_len++] = time.tm_hour;
			report_data_ptr->pdata[report_data_ptr->data_len++] = time.tm_min;
			report_data_ptr->pdata[report_data_ptr->data_len++] = time.tm_sec;
		}
		else
		{
			report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
			report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DATA_LEN;
		}
		break;
	//��ѯ��ʾ���ı�
	case SL427_AFN_QUERY_SCREEN_TXT:
		if(0 == data_len)
		{
			report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn);
			report_data_ptr->data_len += bx5k_get_screen_txt(report_data_ptr->pdata + report_data_ptr->data_len);
		}
		else
		{
			report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
			report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DATA_LEN;
		}
		break;
	//����ʱ��
	case SL427_AFN_SET_TIME:
		if(6 == data_len)
		{
			time.tm_year	= recv_data_ptr->pdata[i++] + 2000;
			time.tm_mon		= recv_data_ptr->pdata[i++] - 1;
			time.tm_mday	= recv_data_ptr->pdata[i++];
			time.tm_hour	= recv_data_ptr->pdata[i++];
			time.tm_min		= recv_data_ptr->pdata[i++];
			time.tm_sec		= recv_data_ptr->pdata[i++];
			time = rtcee_rtc_unix_to_calendar(rtcee_rtc_calendar_to_unix(time));
			rtcee_rtc_set_calendar(time);
			report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn);
		}
		else
		{
			report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
			report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DATA_LEN;
		}
		break;
	//����Ԥ�ø澯��Ϣ
	case SL427_AFN_SET_PRESET_WARN_INFO:
		if((data_len > WM_BYTES_PRESET_WARN_INFO) || (0 == data_len))
		{
			report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
			report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_WARN_TOO_LONG;
		}
		else
		{
			wm_set_preset_warn_info(recv_data_ptr->pdata + i, data_len, DTU_TTS_DATA_TYPE_UCS2);
			report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn);
		}
		break;
	//����绰
	case SL427_AFN_DIAL_PHONE:
		if(0 == data_len)
		{
			report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
			report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DATA_LEN;
		}
		else
		{
			pmem = mempool_req(sizeof(DTU_SMS_ADDR), RT_WAITING_NO);
			if((void *)0 == pmem)
			{
				report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
				report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DEVICE;
			}
			else
			{
				((DTU_SMS_ADDR *)pmem)->addr_len = 0;
				while(data_len)
				{
					data_len--;
					if(0 == recv_data_ptr->pdata[i])
					{
						break;
					}
					if(((DTU_SMS_ADDR *)pmem)->addr_len < DTU_BYTES_SMS_ADDR)
					{
						((DTU_SMS_ADDR *)pmem)->addr_data[((DTU_SMS_ADDR *)pmem)->addr_len++] = recv_data_ptr->pdata[i++];
					}
					else
					{
						((DTU_SMS_ADDR *)pmem)->addr_len = 0;
						break;
					}
				}
				if(0 == ((DTU_SMS_ADDR *)pmem)->addr_len)
				{
					rt_mp_free(pmem);
					report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
					report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DATA_VALUE;
				}
				else if(RT_TRUE == dtu_dial_phone((DTU_SMS_ADDR *)pmem))
				{
					report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn);
				}
				else
				{
					rt_mp_free(pmem);
					report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
					report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_DEVICE;
				}
			}
		}
		break;
	//δʶ���afn
	default:
//		report_data_ptr->data_len = _sl427_bao_tou(report_data_ptr->pdata, id, 1, 1, afn + 0x80);
//		report_data_ptr->pdata[report_data_ptr->data_len++] = SL427_ERR_AFN;
		report_data_ptr->data_len = 0;
		return RT_TRUE;
	}

	//����
	report_data_ptr->pdata[SL427_BYTES_GPRS_FRAME_HEAD - 1] = report_data_ptr->data_len - SL427_BYTES_GPRS_FRAME_HEAD;
	//crc
	crc_value = _sl427_crc_value(report_data_ptr->pdata, report_data_ptr->data_len);
	*(rt_uint16_t *)(report_data_ptr->pdata + report_data_ptr->data_len) = crc_value;
	report_data_ptr->data_len += sizeof(rt_uint16_t);

	report_data_ptr->fun_csq_update		= (void *)0;
	report_data_ptr->data_id			= 0;
	report_data_ptr->need_reply			= RT_FALSE;
	report_data_ptr->fcb_value			= 0;
	report_data_ptr->ptcl_type			= PTCL_PTCL_TYPE_SL427;

	return RT_TRUE;
}

//gprsЭ�����֮lora�ŵ�
rt_uint8_t sl427_gprs_data_decoder_lora(PTCL_REPORT_DATA *report_data_ptr, PTCL_RECV_DATA const *recv_data_ptr)
{
	rt_uint16_t				i, crc_value, recv_len;
	rt_uint8_t				packet_total, packet_cur, afn, data_len;
	static SL427_WARN_INFO	*warn_info_ptr = (SL427_WARN_INFO *)0;
	rt_uint32_t				water_value;
	static rt_uint32_t		sn_ex = 0;

	if(recv_data_ptr->data_len <= SL427_BYTES_GPRS_FRAME_HEAD)
	{
		return RT_FALSE;
	}
	i = 0;
	if(SL427_FRAME_HEAD_1 != recv_data_ptr->pdata[i++])
	{
		return RT_FALSE;
	}
	if(SL427_FRAME_HEAD_2 != recv_data_ptr->pdata[i++])
	{
		return RT_FALSE;
	}
	//����id
	i++;
	//�豸��ַ
	i += SL427_BYTES_RTU_ADDR;
	//�ܰ����͵�ǰ����
	i += 2;
	//������
	i++;
	//�����򳤶�
	recv_len = recv_data_ptr->pdata[i++];
	recv_len += i;
	recv_len += SL427_BYTES_GPRS_FRAME_END;
	if(recv_len > recv_data_ptr->data_len)
	{
		return RT_FALSE;
	}
	
	//crc
	if(recv_len <= SL427_BYTES_GPRS_FRAME_END)
	{
		return RT_FALSE;
	}
	i = recv_len - SL427_BYTES_GPRS_FRAME_END;
	crc_value = _sl427_crc_value(recv_data_ptr->pdata, i);
	if((crc_value != *(rt_uint16_t *)(recv_data_ptr->pdata + i)) && (0xffff != *(rt_uint16_t *)(recv_data_ptr->pdata + i)))
	{
		return RT_FALSE;
	}
	//֡ͷ
	i = 0;
	//Э��id
	if((i + SL427_BYTES_GPRS_FRAME_HEAD) > recv_len)
	{
		return RT_FALSE;
	}
	if(SL427_FRAME_HEAD_1 != recv_data_ptr->pdata[i++])
	{
		return RT_FALSE;
	}
	if(SL427_FRAME_HEAD_2 != recv_data_ptr->pdata[i++])
	{
		return RT_FALSE;
	}
	//����id
	water_value = recv_data_ptr->pdata[i++];
	//�豸��ַ
	i += SL427_BYTES_RTU_ADDR;
	//�ܰ����͵�ǰ����
	packet_total = recv_data_ptr->pdata[i++];
	packet_cur = recv_data_ptr->pdata[i++];
	if((0 == packet_cur) || (packet_cur > packet_total))
	{
		return RT_FALSE;
	}
	//���ݹ���
	water_value <<= 8;
	water_value += packet_total;
	water_value <<= 8;
	water_value += packet_cur;
	if(sn_ex == water_value)
	{
		return RT_FALSE;
	}
	sn_ex = water_value;
	//������
	afn = recv_data_ptr->pdata[i++];
	//�����򳤶�
	data_len = recv_data_ptr->pdata[i++];
	if((i + data_len + SL427_BYTES_GPRS_FRAME_END) != recv_len)
	{
		return RT_FALSE;
	}

	//����
	switch(afn)
	{
	case SL427_AFN_REQUEST_WARN_EX:
		if(data_len <= 8)
		{
			return RT_FALSE;
		}
		data_len -= 8;
		//�����ı�����
		if((SL427_WARN_DATA_TYPE_ASCII != recv_data_ptr->pdata[i]) && (SL427_WARN_DATA_TYPE_UNICODE != recv_data_ptr->pdata[i]))
		{
			return RT_FALSE;
		}
		packet_total = (SL427_WARN_DATA_TYPE_ASCII == recv_data_ptr->pdata[i++]) ? DTU_TTS_DATA_TYPE_GBK : DTU_TTS_DATA_TYPE_UCS2;
		//��������
		packet_cur = recv_data_ptr->pdata[i++];
		//ˮλ�ȼ�
		i += 2;
		//ˮλֵ
		water_value = (rt_uint32_t)(*(float *)(recv_data_ptr->pdata + i) * 1000);
		i += 4;
		//���󱨾�
		if(RT_TRUE == wm_warn_tts_trigger(WM_WARN_TYPE_GPRS, packet_cur, packet_total, recv_data_ptr->pdata + i, data_len, (DTU_TTS_DATA_TYPE_UCS2 == packet_total) ? RT_TRUE : RT_FALSE))
		{
			bx5k_set_screen_txt(recv_data_ptr->pdata + i, data_len);
			bx5k_set_screen_data(water_value);
		}
		break;
	//���󱨾�
	case SL427_AFN_REQUEST_WARN:
		//����
		if(1 == packet_total)
		{
			if(data_len <= 2)
			{
				return RT_FALSE;
			}
			data_len -= 2;
			//�����ı�����
			if((SL427_WARN_DATA_TYPE_ASCII != recv_data_ptr->pdata[i]) && (SL427_WARN_DATA_TYPE_UNICODE != recv_data_ptr->pdata[i]))
			{
				return RT_FALSE;
			}
			packet_total = (SL427_WARN_DATA_TYPE_ASCII == recv_data_ptr->pdata[i++]) ? DTU_TTS_DATA_TYPE_GBK : DTU_TTS_DATA_TYPE_UCS2;
			//��������
			packet_cur = recv_data_ptr->pdata[i++];
			//���󱨾�
			if(RT_TRUE == wm_warn_tts_trigger(WM_WARN_TYPE_GPRS, packet_cur, packet_total, recv_data_ptr->pdata + i, data_len, (DTU_TTS_DATA_TYPE_UCS2 == packet_total) ? RT_TRUE : RT_FALSE))
			{
				bx5k_set_screen_txt(recv_data_ptr->pdata + i, data_len);
			}
		}
		//����еĵ�һ��
		else if(1 == packet_cur)
		{
			if(data_len <= 2)
			{
				return RT_FALSE;
			}
			data_len -= 2;
			//�����ı�����
			if((SL427_WARN_DATA_TYPE_ASCII != recv_data_ptr->pdata[i]) && (SL427_WARN_DATA_TYPE_UNICODE != recv_data_ptr->pdata[i]))
			{
				return RT_FALSE;
			}
			//�����ı�����
			if(data_len > SL427_BYTES_WARN_INFO_MAX)
			{
				return RT_FALSE;
			}
			//����ռ�
			if((SL427_WARN_INFO *)0 != warn_info_ptr)
			{
				rt_mp_free((void *)warn_info_ptr);
			}
			warn_info_ptr = _sl427_warn_info_req(SL427_BYTES_WARN_INFO_MAX, RT_WAITING_NO);
			if((SL427_WARN_INFO *)0 == warn_info_ptr)
			{
				report_data_ptr->data_len = 0;
				return RT_TRUE;
			}
			//���ݴ���
			warn_info_ptr->data_type	= (SL427_WARN_DATA_TYPE_ASCII == recv_data_ptr->pdata[i++]) ? DTU_TTS_DATA_TYPE_GBK : DTU_TTS_DATA_TYPE_UCS2;
			warn_info_ptr->warn_times	= recv_data_ptr->pdata[i++];
			warn_info_ptr->packet_total	= packet_total;
			warn_info_ptr->packet_cur	= packet_cur;
			warn_info_ptr->data_len		= data_len;
			rt_memcpy((void *)warn_info_ptr->pdata, (void *)(recv_data_ptr->pdata + i), data_len);
		}
		//����е������
		else
		{
			//�澯��ϢΪ��
			if((SL427_WARN_INFO *)0 == warn_info_ptr)
			{
				return RT_FALSE;
			}
			//�ܰ�������
			if(packet_total != warn_info_ptr->packet_total)
			{
				return RT_FALSE;
			}
			//��ǰ��������
			if(packet_cur != (warn_info_ptr->packet_cur + 1))
			{
				return RT_FALSE;
			}
			//���ݳ���
			if((warn_info_ptr->data_len + data_len) > SL427_BYTES_WARN_INFO_MAX)
			{
				return RT_FALSE;
			}
			//���ݴ���
			warn_info_ptr->packet_cur	= packet_cur;
			rt_memcpy((void *)(warn_info_ptr->pdata + warn_info_ptr->data_len), (void *)(recv_data_ptr->pdata + i), data_len);
			warn_info_ptr->data_len		+= data_len;
			//���һ��
			if(warn_info_ptr->packet_total == warn_info_ptr->packet_cur)
			{
				if(RT_TRUE == wm_warn_tts_trigger(WM_WARN_TYPE_GPRS, warn_info_ptr->warn_times, warn_info_ptr->data_type, warn_info_ptr->pdata, warn_info_ptr->data_len, (DTU_TTS_DATA_TYPE_UCS2 == warn_info_ptr->data_type) ? RT_TRUE : RT_FALSE))
				{
					bx5k_set_screen_txt(warn_info_ptr->pdata, warn_info_ptr->data_len);
				}
				//�ͷŸ澯��Ϣ
				rt_mp_free((void *)warn_info_ptr);
				warn_info_ptr = (SL427_WARN_INFO *)0;
			}
		}
		break;
	//δʶ���afn
	default:
		report_data_ptr->data_len = 0;
		return RT_TRUE;
	}

	if(recv_len > PTCL_BYTES_ACK_REPORT)
	{
		report_data_ptr->data_len = 0;
	}
	else
	{
		rt_memcpy((void *)report_data_ptr->pdata, (void *)recv_data_ptr->pdata, recv_len);
		report_data_ptr->data_len			= recv_len;
		report_data_ptr->data_id			= 0;
		report_data_ptr->fun_csq_update		= (void *)0;
		report_data_ptr->need_reply			= RT_FALSE;
		report_data_ptr->fcb_value			= 0;
		report_data_ptr->ptcl_type			= PTCL_PTCL_TYPE_SL427;
	}

	return RT_TRUE;
}

