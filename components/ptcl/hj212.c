#include "hj212.h"
#include "string.h"
#include "drv_debug.h"
#include "drv_rtcee.h"
#include "stdio.h"
#include "drv_lpm.h"
#include "drv_tevent.h"
#include "equip.h"
#include "sample.h"
#include "drv_mempool.h"



//�¼�
static struct rt_event				m_hj212_event_module;
//��Ϣ����
static struct rt_mailbox			m_hj212_mb_report_min;
static rt_ubase_t					m_hj212_msgpool_report_min;

//����
static struct rt_thread				m_hj212_thread_time_cali_req;
static struct rt_thread				m_hj212_thread_report_rtd;
static struct rt_thread				m_hj212_thread_time_tick;
static struct rt_thread				m_hj212_thread_data_hdr;
static struct rt_thread				m_hj212_thread_report_min;
static rt_uint8_t					m_hj212_stk_time_cali_req[HJ212_STK_TIME_CALI_REQ];
static rt_uint8_t					m_hj212_stk_report_rtd[HJ212_STK_REPORT_RTD];
static rt_uint8_t					m_hj212_stk_time_tick[HJ212_STK_TIME_TICK];
static rt_uint8_t					m_hj212_stk_data_hdr[HJ212_STK_DATA_HDR];
static rt_uint8_t					m_hj212_stk_report_min[HJ212_STK_REPORT_MIN];

//��������
static HJ212_PARAM_SET				m_hj212_param_set;
#define m_hj212_st					m_hj212_param_set.st
#define m_hj212_pw					m_hj212_param_set.pw
#define m_hj212_mn					m_hj212_param_set.mn
#define m_hj212_over_time			m_hj212_param_set.over_time
#define m_hj212_recount				m_hj212_param_set.recount
#define m_hj212_rtd_en				m_hj212_param_set.rtd_en
#define m_hj212_rtd_interval		m_hj212_param_set.rtd_interval
#define m_hj212_min_interval		m_hj212_param_set.min_interval
#define m_hj212_rs_en				m_hj212_param_set.rs_en



#include "hj212_io.c"



static void _hj212_param_pend(rt_uint32_t param)
{
	if(RT_EOK != rt_event_recv(&m_hj212_event_module, param, RT_EVENT_FLAG_AND + RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, (rt_uint32_t *)0))
	{
		while(1);
	}
}

static void _hj212_param_post(rt_uint32_t param)
{
	if(RT_EOK != rt_event_send(&m_hj212_event_module, param))
	{
		while(1);
	}
}

static rt_uint16_t _hj212_crc_value(rt_uint8_t const *pdata, rt_uint16_t data_len)
{
	rt_uint16_t	crc_value = 0xffff;
	rt_uint8_t	i;

	while(data_len)
	{
		data_len--;

		crc_value >>= 8;
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

static rt_uint16_t _hj212_frm_baotou(rt_uint16_t max_len, rt_uint8_t *pdata)
{
	rt_uint16_t data_len;

	data_len = snprintf((char *)pdata, max_len, "%s0000", HJ212_FRAME_BAOTOU);
	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static void _hj212_frm_len(rt_uint8_t *pdata, rt_uint16_t data_len)
{
	rt_uint8_t tmp;

	if((HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN) < data_len)
	{
		tmp = pdata[HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN];
		snprintf((char *)(pdata + HJ212_BYTES_FRAME_BAOTOU), HJ212_BYTES_FRAME_LEN + 1, "%04d", data_len - HJ212_BYTES_FRAME_BAOTOU - HJ212_BYTES_FRAME_LEN);
		pdata[HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN] = tmp;
	}
}

static rt_uint16_t _hj212_frm_qn(rt_uint16_t max_len, rt_uint8_t *pdata)
{
	rt_uint16_t	data_len;
	struct tm	time;

	time = rtcee_rtc_get_calendar();
	data_len = snprintf((char *)pdata, max_len, "%s%04d%02d%02d%02d%02d%02d511;", HJ212_FRAME_QN, time.tm_year, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_qn_ex(rt_uint16_t max_len, rt_uint8_t *pdata, struct tm const *ptime)
{
	rt_uint16_t data_len;

	data_len = snprintf((char *)pdata, max_len, "%s%04d%02d%02d%02d%02d%02d511;", HJ212_FRAME_QN, ptime->tm_year, ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min, ptime->tm_sec);
	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_st(rt_uint16_t max_len, rt_uint8_t *pdata, rt_uint8_t owned)
{
	rt_uint16_t	data_len;

	if(RT_TRUE == owned)
	{
		if(max_len <= HJ212_BYTES_FRAME_ST)
		{
			return 0;
		}
		
		data_len = snprintf((char *)pdata, max_len, "%s", HJ212_FRAME_ST);
		_hj212_param_pend(HJ212_EVENT_PARAM_ST);
		memcpy((void *)(pdata + data_len), (void *)m_hj212_st, HJ212_BYTES_ST);
		_hj212_param_post(HJ212_EVENT_PARAM_ST);
		data_len += HJ212_BYTES_ST;
		pdata[data_len++] = ';';
	}
	else
	{
		data_len = snprintf((char *)pdata, max_len, "%s%s;", HJ212_FRAME_ST, HJ212_ST_SYS_COMM);
		if(data_len >= max_len)
		{
			data_len = 0;
		}
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_cn(rt_uint16_t max_len, rt_uint8_t *pdata, rt_uint16_t cn)
{
	rt_uint16_t data_len;

	data_len = snprintf((char *)pdata, max_len, "%s%04d;", HJ212_FRAME_CN, cn);
	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_pw(rt_uint16_t max_len, rt_uint8_t *pdata)
{
	rt_uint16_t	data_len;

	if(max_len <= HJ212_BYTES_FRAME_PW)
	{
		return 0;
	}

	data_len = snprintf((char *)pdata, max_len, "%s", HJ212_FRAME_PW);
	_hj212_param_pend(HJ212_EVENT_PARAM_PW);
	memcpy((void *)(pdata + data_len), (void *)m_hj212_pw, HJ212_BYTES_PW);
	_hj212_param_post(HJ212_EVENT_PARAM_PW);
	data_len += HJ212_BYTES_PW;
	pdata[data_len++] = ';';

	return data_len;
}

static rt_uint16_t _hj212_frm_mn(rt_uint16_t max_len, rt_uint8_t *pdata)
{
	rt_uint16_t	data_len;

	if(max_len <= HJ212_BYTES_FRAME_MN)
	{
		return 0;
	}

	data_len = snprintf((char *)pdata, max_len, "%s", HJ212_FRAME_MN);
	_hj212_param_pend(HJ212_EVENT_PARAM_MN);
	memcpy((void *)(pdata + data_len), (void *)m_hj212_mn, HJ212_BYTES_MN);
	_hj212_param_post(HJ212_EVENT_PARAM_MN);
	data_len += HJ212_BYTES_MN;
	pdata[data_len++] = ';';

	return data_len;
}

static rt_uint16_t _hj212_frm_flag(rt_uint16_t max_len, rt_uint8_t *pdata, rt_uint8_t flag)
{
	rt_uint16_t data_len;

	data_len = snprintf((char *)pdata, max_len, "%s%d;", HJ212_FRAME_FLAG, flag);
	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_pnum(rt_uint16_t max_len, rt_uint8_t *pdata, rt_uint16_t pnum)
{
	rt_uint16_t data_len;

	data_len = snprintf((char *)pdata, max_len, "%s%d;", HJ212_FRAME_PNUM, pnum);
	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_pno(rt_uint16_t max_len, rt_uint8_t *pdata, rt_uint16_t pno)
{
	rt_uint16_t data_len;

	data_len = snprintf((char *)pdata, max_len, "%s%d;", HJ212_FRAME_PNO, pno);
	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_cp_start(rt_uint16_t max_len, rt_uint8_t *pdata)
{
	rt_uint16_t	data_len;

	data_len = snprintf((char *)pdata, max_len, "%s", HJ212_FRAME_CP_START);
	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_cp_end(rt_uint16_t max_len, rt_uint8_t *pdata)
{
	rt_uint16_t	data_len;

	data_len = snprintf((char *)pdata, max_len, "%s", HJ212_FRAME_CP_END);
	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_data_head(rt_uint8_t *pdata, rt_uint16_t data_len, rt_uint8_t const *pqn, rt_uint8_t st_owned, rt_uint16_t cn, rt_uint8_t flag, rt_uint16_t pnum, rt_uint16_t pno)
{
	rt_uint16_t i, len;

	i = _hj212_frm_baotou(data_len, pdata);
	if(!i)
	{
		goto __exit;
	}
	//QN
	if((rt_uint8_t *)0 != pqn)
	{
		memcpy((void *)(pdata + i), (void *)pqn, HJ212_BYTES_FRAME_QN + 1);
		i += (HJ212_BYTES_FRAME_QN + 1);
	}
	else
	{
		len = _hj212_frm_qn(data_len - i, pdata + i);
		if(len)
		{
			i += len;
		}
		else
		{
			i = 0;
			goto __exit;
		}
	}
	//ST
	len = _hj212_frm_st(data_len - i, pdata + i, st_owned);
	if(len)
	{
		i += len;
	}
	else
	{
		i = 0;
		goto __exit;
	}
	//CN
	len = _hj212_frm_cn(data_len - i, pdata + i, cn);
	if(len)
	{
		i += len;
	}
	else
	{
		i = 0;
		goto __exit;
	}
	//PW
	len = _hj212_frm_pw(data_len - i, pdata + i);
	if(len)
	{
		i += len;
	}
	else
	{
		i = 0;
		goto __exit;
	}
	//MN
	len = _hj212_frm_mn(data_len - i, pdata + i);
	if(len)
	{
		i += len;
	}
	else
	{
		i = 0;
		goto __exit;
	}
	//FLAG
	len = _hj212_frm_flag(data_len - i, pdata + i, flag);
	if(len)
	{
		i += len;
	}
	else
	{
		i = 0;
		goto __exit;
	}
	//PNUM PNO
	if(flag & HJ212_FLAG_PACKET_DIV_EN)
	{
		//PNUM
		len = _hj212_frm_pnum(data_len - i, pdata + i, pnum);
		if(len)
		{
			i += len;
		}
		else
		{
			i = 0;
			goto __exit;
		}
		//PNO
		len = _hj212_frm_pno(data_len - i, pdata + i, pno);
		if(len)
		{
			i += len;
		}
		else
		{
			i = 0;
			goto __exit;
		}
	}
	//CP_START
	len = _hj212_frm_cp_start(data_len - i, pdata + i);
	if(len)
	{
		i += len;
	}
	else
	{
		i = 0;
		goto __exit;
	}

__exit:
	return i;
}

static rt_uint16_t _hj212_frm_data_head_ex(rt_uint8_t *pdata, rt_uint16_t data_len, struct tm const *ptime, rt_uint8_t st_owned, rt_uint16_t cn, rt_uint8_t flag, rt_uint16_t pnum, rt_uint16_t pno)
{
	rt_uint16_t i, len;

	i = _hj212_frm_baotou(data_len, pdata);
	if(!i)
	{
		goto __exit;
	}
	//QN
	len = _hj212_frm_qn_ex(data_len - i, pdata + i, ptime);
	if(len)
	{
		i += len;
	}
	else
	{
		i = 0;
		goto __exit;
	}
	//ST
	len = _hj212_frm_st(data_len - i, pdata + i, st_owned);
	if(len)
	{
		i += len;
	}
	else
	{
		i = 0;
		goto __exit;
	}
	//CN
	len = _hj212_frm_cn(data_len - i, pdata + i, cn);
	if(len)
	{
		i += len;
	}
	else
	{
		i = 0;
		goto __exit;
	}
	//PW
	len = _hj212_frm_pw(data_len - i, pdata + i);
	if(len)
	{
		i += len;
	}
	else
	{
		i = 0;
		goto __exit;
	}
	//MN
	len = _hj212_frm_mn(data_len - i, pdata + i);
	if(len)
	{
		i += len;
	}
	else
	{
		i = 0;
		goto __exit;
	}
	//FLAG
	len = _hj212_frm_flag(data_len - i, pdata + i, flag);
	if(len)
	{
		i += len;
	}
	else
	{
		i = 0;
		goto __exit;
	}
	//PNUM PNO
	if(flag & HJ212_FLAG_PACKET_DIV_EN)
	{
		//PNUM
		len = _hj212_frm_pnum(data_len - i, pdata + i, pnum);
		if(len)
		{
			i += len;
		}
		else
		{
			i = 0;
			goto __exit;
		}
		//PNO
		len = _hj212_frm_pno(data_len - i, pdata + i, pno);
		if(len)
		{
			i += len;
		}
		else
		{
			i = 0;
			goto __exit;
		}
	}
	//CP_START
	len = _hj212_frm_cp_start(data_len - i, pdata + i);
	if(len)
	{
		i += len;
	}
	else
	{
		i = 0;
		goto __exit;
	}

__exit:
	return i;
}

static rt_uint16_t _hj212_frm_qnrtn(rt_uint16_t max_len, rt_uint8_t *pdata, rt_uint8_t rtn)
{
	rt_uint16_t data_len;

	data_len = snprintf((char *)pdata, max_len, "%s%d", HJ212_FIELD_NAME_QNRTN, rtn);
	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_exertn(rt_uint16_t max_len, rt_uint8_t *pdata, rt_uint8_t rtn)
{
	rt_uint16_t data_len;

	data_len = snprintf((char *)pdata, max_len, "%s%d", HJ212_FIELD_NAME_EXERTN, rtn);
	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_sys_time(rt_uint16_t max_len, rt_uint8_t *pdata)
{
	rt_uint16_t	data_len;
	struct tm	time;

	time = rtcee_rtc_get_calendar();
	data_len = snprintf((char *)pdata, max_len, "%s%04d%02d%02d%02d%02d%02d", HJ212_FIELD_NAME_SYSTIME, time.tm_year, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_rtd_interval(rt_uint16_t max_len, rt_uint8_t *pdata)
{
	rt_uint16_t data_len;

	data_len = snprintf((char *)pdata, max_len, "%s%d", HJ212_FIELD_NAME_RTDINTERVAL, hj212_get_rtd_interval());
	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_min_interval(rt_uint16_t max_len, rt_uint8_t *pdata)
{
	rt_uint16_t data_len;

	data_len = snprintf((char *)pdata, max_len, "%s%d", HJ212_FIELD_NAME_MININTERVAL, hj212_get_min_interval());
	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_data_time(rt_uint16_t max_len, rt_uint8_t *pdata, struct tm const *ptime)
{
	rt_uint16_t data_len;

	data_len = snprintf((char *)pdata, max_len, "%s%04d%02d%02d%02d%02d%02d;", HJ212_FIELD_NAME_DATA_TIME, ptime->tm_year, ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min, ptime->tm_sec);
	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_data(rt_uint16_t max_len, rt_uint8_t *pdata, char const *pname, char const *ptype, float data, rt_uint8_t num)
{
	rt_uint16_t data_len;

	switch(num)
	{
	default:
		data_len = snprintf((char *)pdata, max_len, "%s%s%d,", pname, ptype, (rt_uint32_t)data);
		break;
	case 1:
		data_len = snprintf((char *)pdata, max_len, "%s%s%.1f,", pname, ptype, data);
		break;
	case 2:
		data_len = snprintf((char *)pdata, max_len, "%s%s%.2f,", pname, ptype, data);
		break;
	case 3:
		data_len = snprintf((char *)pdata, max_len, "%s%s%.3f,", pname, ptype, data);
		break;
	case 4:
		data_len = snprintf((char *)pdata, max_len, "%s%s%.4f,", pname, ptype, data);
		break;
	}

	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_data_flag(rt_uint16_t max_len, rt_uint8_t *pdata, char const *pname, char flag)
{
	rt_uint16_t data_len;

	data_len = snprintf((char *)pdata, max_len, "%s%s%c;", pname, HJ212_FIELD_NAME_DATA_FLAG, flag);
	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_data_all(rt_uint16_t max_len, rt_uint8_t *pdata, rt_uint16_t cn, rt_uint8_t sensor_type, rt_uint8_t bit_num, rt_uint32_t unix_low, rt_uint32_t unix_high, float k, char const *pname)
{
	rt_uint8_t			cou, min, avg, max;
	rt_uint16_t			data_len = 0, tmp;
	rt_uint32_t			step;
	SAMPLE_DATA_TYPE	data_type;

	switch(cn)
	{
	default:
		return 0;
	case HJ212_CN_MIN_DATA:
	case HJ212_CN_HOUR_DATA:
		step	= 60;
		cou		= SAMPLE_SENSOR_DATA_COU_M;
		min		= SAMPLE_SENSOR_DATA_CUR;
		avg		= SAMPLE_SENSOR_DATA_CUR;
		max		= SAMPLE_SENSOR_DATA_CUR;
		break;
	case HJ212_CN_DAY_DATA:
		step	= 3600;
		cou		= SAMPLE_SENSOR_DATA_COU_H;
		min		= SAMPLE_SENSOR_DATA_MIN_H;
		avg		= SAMPLE_SENSOR_DATA_AVG_H;
		max		= SAMPLE_SENSOR_DATA_MAX_H;
		break;
	}
	
	//�ŷ���
	sample_query_cou(sensor_type + cou, unix_low, unix_high, step, &data_type);
	if(RT_TRUE == data_type.sta)
	{
		data_type.value /= k;
		tmp = _hj212_frm_data(max_len - data_len, pdata + data_len, pname, HJ212_FIELD_NAME_DATA_COU, data_type.value, bit_num);
	}
	else
	{
		tmp = _hj212_frm_data(max_len - data_len, pdata + data_len, pname, HJ212_FIELD_NAME_DATA_COU, 0, 0);
	}
	if(tmp)
	{
		data_len += tmp;
	}
	else
	{
		return 0;
	}
	//��Сֵ
	sample_query_min(sensor_type + min, unix_low, unix_high, step, &data_type);
	if(RT_TRUE == data_type.sta)
	{
		tmp = _hj212_frm_data(max_len - data_len, pdata + data_len, pname, HJ212_FIELD_NAME_DATA_MIN, data_type.value, bit_num);
	}
	else
	{
		tmp = _hj212_frm_data(max_len - data_len, pdata + data_len, pname, HJ212_FIELD_NAME_DATA_MIN, 0, 0);
	}
	if(tmp)
	{
		data_len += tmp;
	}
	else
	{
		return 0;
	}
	//ƽ��ֵ
	sample_query_avg(sensor_type + avg, unix_low, unix_high, step, &data_type);
	if(RT_TRUE == data_type.sta)
	{
		tmp = _hj212_frm_data(max_len - data_len, pdata + data_len, pname, HJ212_FIELD_NAME_DATA_AVG, data_type.value, bit_num);
	}
	else
	{
		tmp = _hj212_frm_data(max_len - data_len, pdata + data_len, pname, HJ212_FIELD_NAME_DATA_AVG, 0, 0);
	}
	if(tmp)
	{
		data_len += tmp;
	}
	else
	{
		return 0;
	}
	//���ֵ
	sample_query_max(sensor_type + max, unix_low, unix_high, step, &data_type);
	if(RT_TRUE == data_type.sta)
	{
		tmp = _hj212_frm_data(max_len - data_len, pdata + data_len, pname, HJ212_FIELD_NAME_DATA_MAX, data_type.value, bit_num);
	}
	else
	{
		tmp = _hj212_frm_data(max_len - data_len, pdata + data_len, pname, HJ212_FIELD_NAME_DATA_MAX, 0, 0);
	}
	if(tmp)
	{
		data_len += tmp;
	}
	else
	{
		return 0;
	}
	//Flag
	tmp = _hj212_frm_data_flag(max_len - data_len, pdata + data_len, pname, HJ212_DATA_FLAG_RUN);
	if(tmp)
	{
		data_len += tmp;
	}
	else
	{
		return 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_data_sbrt(rt_uint16_t max_len, rt_uint8_t *pdata, rt_uint8_t equip, float rt)
{
	rt_uint16_t data_len;

	data_len = snprintf((char *)pdata, max_len, HJ212_FIELD_NAME_RUN_TIME, equip + 1, rt);
	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_run_sta(rt_uint16_t max_len, rt_uint8_t *pdata, rt_uint8_t equip, rt_uint8_t sta)
{
	rt_uint16_t data_len;

	data_len = snprintf((char *)pdata, max_len, HJ212_FIELD_NAME_RUN_STA, equip + 1, (RT_TRUE == sta) ? '1' : '0');
	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_restart_time(rt_uint16_t max_len, rt_uint8_t *pdata, struct tm const *ptime)
{
	rt_uint16_t data_len;

	data_len = snprintf((char *)pdata, max_len, "%s%04d%02d%02d%02d%02d%02d", HJ212_FIELD_NAME_RESTARTTIME, ptime->tm_year, ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min, ptime->tm_sec);
	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_baowei(rt_uint16_t max_len, rt_uint8_t *pdata, rt_uint16_t crc_val)
{
	rt_uint16_t data_len;

	data_len = snprintf((char *)pdata, max_len, "%04X%s", crc_val, HJ212_FRAME_BAOWEI);
	if(data_len >= max_len)
	{
		data_len = 0;
	}

	return data_len;
}

static rt_uint16_t _hj212_frm_is_qn(rt_uint8_t const *pdata, rt_uint16_t data_len)
{
	rt_uint16_t i = 0;
	
	if(data_len <= HJ212_BYTES_FRAME_QN)
	{
		return 0;
	}
	
	if(memcmp((void *)pdata, (void *)HJ212_FRAME_QN, strlen(HJ212_FRAME_QN)))
	{
		return 0;
	}
	i += strlen(HJ212_FRAME_QN);

	if(RT_FALSE == fyyp_is_number(pdata + i, HJ212_BYTES_QN))
	{
		return 0;
	}
	i += HJ212_BYTES_QN;

	if(';' != pdata[i++])
	{
		return 0;
	}

	return i;
}

static rt_uint16_t _hj212_frm_is_st(rt_uint8_t const *pdata, rt_uint16_t data_len)
{
	rt_uint16_t i = 0;
	
	if(data_len <= HJ212_BYTES_FRAME_ST)
	{
		return 0;
	}
	
	if(memcmp((void *)pdata, (void *)HJ212_FRAME_ST, strlen(HJ212_FRAME_ST)))
	{
		return 0;
	}
	i += strlen(HJ212_FRAME_ST);

	_hj212_param_pend(HJ212_EVENT_PARAM_ST);
	if(memcmp((void *)(pdata + i), (void *)m_hj212_st, HJ212_BYTES_ST) && memcmp((void *)(pdata + i), (void *)HJ212_ST_SYS_COMM, HJ212_BYTES_ST))
	{
		_hj212_param_post(HJ212_EVENT_PARAM_ST);
		return 0;
	}
	else
	{
		_hj212_param_post(HJ212_EVENT_PARAM_ST);
		i += HJ212_BYTES_ST;
	}

	if(';' != pdata[i++])
	{
		return 0;
	}

	return i;
}

static rt_uint16_t _hj212_frm_is_cn(rt_uint8_t const *pdata, rt_uint16_t data_len, rt_uint16_t *cn)
{
	rt_uint16_t i = 0;
	
	if(data_len <= HJ212_BYTES_FRAME_CN)
	{
		return 0;
	}
	
	if(memcmp((void *)pdata, (void *)HJ212_FRAME_CN, strlen(HJ212_FRAME_CN)))
	{
		return 0;
	}
	i += strlen(HJ212_FRAME_CN);

	if(RT_FALSE == fyyp_is_number(pdata + i, HJ212_BYTES_CN))
	{
		return 0;
	}
	*cn = fyyp_str_to_hex(pdata + i, HJ212_BYTES_CN);
	i += HJ212_BYTES_CN;

	if(';' != pdata[i++])
	{
		return 0;
	}

	return i;
}

static rt_uint16_t _hj212_frm_is_pw(rt_uint8_t const *pdata, rt_uint16_t data_len)
{
	rt_uint16_t i = 0;
	
	if(data_len <= HJ212_BYTES_FRAME_PW)
	{
		return 0;
	}
	
	if(memcmp((void *)pdata, (void *)HJ212_FRAME_PW, strlen(HJ212_FRAME_PW)))
	{
		return 0;
	}
	i += strlen(HJ212_FRAME_PW);

	_hj212_param_pend(HJ212_EVENT_PARAM_PW);
	if(memcmp((void *)(pdata + i), (void *)m_hj212_pw, HJ212_BYTES_PW))
	{
		_hj212_param_post(HJ212_EVENT_PARAM_PW);
		return 0;
	}
	else
	{
		_hj212_param_post(HJ212_EVENT_PARAM_PW);
		i += HJ212_BYTES_PW;
	}

	if(';' != pdata[i++])
	{
		return 0;
	}

	return i;
}

static rt_uint16_t _hj212_frm_is_mn(rt_uint8_t const *pdata, rt_uint16_t data_len)
{
	rt_uint16_t i = 0;
	
	if(data_len <= HJ212_BYTES_FRAME_MN)
	{
		return 0;
	}
	
	if(memcmp((void *)pdata, (void *)HJ212_FRAME_MN, strlen(HJ212_FRAME_MN)))
	{
		return 0;
	}
	i += strlen(HJ212_FRAME_MN);

	_hj212_param_pend(HJ212_EVENT_PARAM_MN);
	if(memcmp((void *)(pdata + i), (void *)m_hj212_mn, HJ212_BYTES_MN))
	{
		_hj212_param_post(HJ212_EVENT_PARAM_MN);
		return 0;
	}
	else
	{
		_hj212_param_post(HJ212_EVENT_PARAM_MN);
		i += HJ212_BYTES_MN;
	}

	if(';' != pdata[i++])
	{
		return 0;
	}

	return i;
}

static rt_uint16_t _hj212_frm_is_flag(rt_uint8_t const *pdata, rt_uint16_t data_len, rt_uint8_t *flag)
{
	rt_uint16_t	i = 0, len;
	rt_uint8_t	*p;

	len = strlen(HJ212_FRAME_FLAG);
	if(data_len <= len)
	{
		return 0;
	}
	
	if(memcmp((void *)pdata, (void *)HJ212_FRAME_FLAG, len))
	{
		return 0;
	}
	i += len;

	p = fyyp_mem_find(pdata + i, data_len - len, ";", 1);
	if((rt_uint8_t *)0 == p)
	{
		return 0;
	}
	len = p - pdata - i;

	if(RT_FALSE == fyyp_is_number(pdata + i, len))
	{
		return 0;
	}
	*flag = fyyp_str_to_hex(pdata + i, len);
	i += len;

	i++;

	return i;
}

static rt_uint16_t _hj212_frm_is_cp(rt_uint8_t const *pdata, rt_uint16_t data_len)
{
	rt_uint16_t start, end;

	start	= strlen(HJ212_FRAME_CP_START);
	end		= strlen(HJ212_FRAME_CP_END);
	if(data_len < (start + end))
	{
		return 0;
	}
	
	if(memcmp((void *)pdata, (void *)HJ212_FRAME_CP_START, start))
	{
		return 0;
	}
	if(memcmp((void *)(pdata + data_len - end), (void *)HJ212_FRAME_CP_END, end))
	{
		return 0;
	}

	return start;
}

static rt_uint16_t _hj212_frm_is_ot(rt_uint8_t const *pdata, rt_uint16_t data_len, rt_uint16_t *over_time)
{
	rt_uint16_t	i = 0, len;
	rt_uint8_t	*p;

	len = strlen(HJ212_FIELD_NAME_OVER_TIME);
	if(data_len <= len)
	{
		return 0;
	}
	
	if(memcmp((void *)pdata, (void *)HJ212_FIELD_NAME_OVER_TIME, len))
	{
		return 0;
	}
	i += len;

	p = fyyp_mem_find(pdata + i, data_len - len, ";", 1);
	if((rt_uint8_t *)0 == p)
	{
		return 0;
	}
	len = p - pdata - i;

	if(RT_FALSE == fyyp_is_number(pdata + i, len))
	{
		return 0;
	}
	*over_time = fyyp_str_to_hex(pdata + i, len);
	i += len;

	i++;

	return i;
}

static rt_uint16_t _hj212_frm_is_recount(rt_uint8_t const *pdata, rt_uint16_t data_len, rt_uint8_t *recount)
{
	rt_uint16_t i = 0, len;

	len = strlen(HJ212_FIELD_NAME_RECOUNT);
	if(data_len <= len)
	{
		return 0;
	}
	
	if(memcmp((void *)pdata, (void *)HJ212_FIELD_NAME_RECOUNT, len))
	{
		return 0;
	}
	i += len;

	if(RT_FALSE == fyyp_is_number(pdata + i, data_len - i))
	{
		return 0;
	}
	*recount = fyyp_str_to_hex(pdata + i, data_len - i);

	return data_len;
}

static rt_uint16_t _hj212_frm_is_sys_time(rt_uint8_t const *pdata, rt_uint16_t data_len, struct tm *time)
{
	rt_uint16_t i = 0, len;

	len = strlen(HJ212_FIELD_NAME_SYSTIME);
	if(data_len <= len)
	{
		return 0;
	}
	
	if(memcmp((void *)pdata, (void *)HJ212_FIELD_NAME_SYSTIME, len))
	{
		return 0;
	}
	i += len;

	if((i + 14) != data_len)
	{
		return 0;
	}
	if(RT_FALSE == fyyp_is_number(pdata + i, 14))
	{
		return 0;
	}

	time->tm_year	= fyyp_str_to_hex(pdata + i, 4);
	i += 4;
	time->tm_mon	= fyyp_str_to_hex(pdata + i, 2) - 1;
	i += 2;
	time->tm_mday	= fyyp_str_to_hex(pdata + i, 2);
	i += 2;
	time->tm_hour	= fyyp_str_to_hex(pdata + i, 2);
	i += 2;
	time->tm_min	= fyyp_str_to_hex(pdata + i, 2);
	i += 2;
	time->tm_sec	= fyyp_str_to_hex(pdata + i, 2);
	i += 2;

	return i;
}

static rt_uint16_t _hj212_frm_is_rtd_interval(rt_uint8_t const *pdata, rt_uint16_t data_len, rt_uint16_t *interval)
{
	rt_uint16_t i = 0, len;

	len = strlen(HJ212_FIELD_NAME_RTDINTERVAL);
	if(data_len <= len)
	{
		return 0;
	}
	
	if(memcmp((void *)pdata, (void *)HJ212_FIELD_NAME_RTDINTERVAL, len))
	{
		return 0;
	}
	i += len;

	if(RT_FALSE == fyyp_is_number(pdata + i, data_len - i))
	{
		return 0;
	}
	*interval = fyyp_str_to_hex(pdata + i, data_len - i);

	return data_len;
}

static rt_uint16_t _hj212_frm_is_min_interval(rt_uint8_t const *pdata, rt_uint16_t data_len, rt_uint16_t *interval)
{
	rt_uint16_t i = 0, len;

	len = strlen(HJ212_FIELD_NAME_MININTERVAL);
	if(data_len <= len)
	{
		return 0;
	}
	
	if(memcmp((void *)pdata, (void *)HJ212_FIELD_NAME_MININTERVAL, len))
	{
		return 0;
	}
	i += len;

	if(RT_FALSE == fyyp_is_number(pdata + i, data_len - i))
	{
		return 0;
	}
	*interval = fyyp_str_to_hex(pdata + i, data_len - i);

	return data_len;
}

static rt_uint16_t _hj212_frm_is_new_pw(rt_uint8_t const *pdata, rt_uint16_t data_len, rt_uint8_t *pw)
{
	rt_uint16_t i = 0, len;

	len = strlen(HJ212_FIELD_NAME_NEW_PW);
	if(data_len <= len)
	{
		return 0;
	}
	
	if(memcmp((void *)pdata, (void *)HJ212_FIELD_NAME_NEW_PW, len))
	{
		return 0;
	}
	i += len;

	if((i + HJ212_BYTES_PW) != data_len)
	{
		return 0;
	}

	memcpy((void *)pw, (void *)(pdata + i), HJ212_BYTES_PW);

	return data_len;
}

static rt_uint16_t _hj212_frm_is_begin_time(rt_uint8_t const *pdata, rt_uint16_t data_len, rt_uint32_t *punix)
{
	rt_uint16_t	i = 0, len;
	struct tm	time;

	len = strlen(HJ212_FIELD_NAME_BEGIN_TIME);
	if(data_len <= len)
	{
		return 0;
	}
	
	if(memcmp((void *)pdata, (void *)HJ212_FIELD_NAME_BEGIN_TIME, len))
	{
		return 0;
	}
	i += len;

	if((i + 15) > data_len)
	{
		return 0;
	}
	if(RT_FALSE == fyyp_is_number(pdata + i, 14))
	{
		return 0;
	}

	time.tm_year	= fyyp_str_to_hex(pdata + i, 4);
	i += 4;
	time.tm_mon		= fyyp_str_to_hex(pdata + i, 2) - 1;
	i += 2;
	time.tm_mday	= fyyp_str_to_hex(pdata + i, 2);
	i += 2;
	time.tm_hour	= fyyp_str_to_hex(pdata + i, 2);
	i += 2;
	time.tm_min		= fyyp_str_to_hex(pdata + i, 2);
	i += 2;
	time.tm_sec		= fyyp_str_to_hex(pdata + i, 2);
	i += 2;
	if(';' != pdata[i++])
	{
		return 0;
	}
	*punix = rtcee_rtc_calendar_to_unix(time);

	return i;
}

static rt_uint16_t _hj212_frm_is_end_time(rt_uint8_t const *pdata, rt_uint16_t data_len, rt_uint32_t *punix)
{
	rt_uint16_t	i = 0, len;
	struct tm	time;

	len = strlen(HJ212_FIELD_NAME_END_TIME);
	if(data_len <= len)
	{
		return 0;
	}
	
	if(memcmp((void *)pdata, (void *)HJ212_FIELD_NAME_END_TIME, len))
	{
		return 0;
	}
	i += len;

	if((i + 14) != data_len)
	{
		return 0;
	}
	if(RT_FALSE == fyyp_is_number(pdata + i, 14))
	{
		return 0;
	}

	time.tm_year	= fyyp_str_to_hex(pdata + i, 4);
	i += 4;
	time.tm_mon		= fyyp_str_to_hex(pdata + i, 2) - 1;
	i += 2;
	time.tm_mday	= fyyp_str_to_hex(pdata + i, 2);
	i += 2;
	time.tm_hour	= fyyp_str_to_hex(pdata + i, 2);
	i += 2;
	time.tm_min		= fyyp_str_to_hex(pdata + i, 2);
	i += 2;
	time.tm_sec		= fyyp_str_to_hex(pdata + i, 2);
	i += 2;
	*punix = rtcee_rtc_calendar_to_unix(time);

	return i;
}

static rt_uint16_t _hj212_data_id(rt_uint8_t const *pdata, rt_uint16_t data_len)
{
	rt_uint16_t	i = HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN, id = 0;
	struct tm	time;

	if((i + HJ212_BYTES_FRAME_QN) > data_len)
	{
		return 0;
	}
	i += strlen(HJ212_FRAME_QN);

	data_len = HJ212_BYTES_FRAME_QN - strlen(HJ212_FRAME_QN);
	if(RT_FALSE == fyyp_is_number(pdata + i, data_len))
	{
		return 0;
	}

	time.tm_year	= fyyp_str_to_hex(pdata + i, 4);
	i += 4;
	time.tm_mon		= fyyp_str_to_hex(pdata + i, 2) - 1;
	i += 2;
	time.tm_mday	= fyyp_str_to_hex(pdata + i, 2);
	i += 2;
	time.tm_hour	= fyyp_str_to_hex(pdata + i, 2);
	i += 2;
	time.tm_min		= fyyp_str_to_hex(pdata + i, 2);
	i += 2;
	time.tm_sec		= fyyp_str_to_hex(pdata + i, 2);
	id = (rt_uint16_t)rtcee_rtc_calendar_to_unix(time);

	return id;
}

static void _hj212_task_time_cali_req(void *parg)
{
	PTCL_REPORT_DATA	*report_data_p;
	rt_uint16_t			crc_val;

	while(1)
	{
		if(RT_EOK != rt_event_recv(&m_hj212_event_module, HJ212_EVENT_TIME_CALI_REQ, RT_EVENT_FLAG_AND + RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, (rt_uint32_t *)0))
		{
			while(1);
		}
		lpm_cpu_ref(RT_TRUE);

		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]Уʱ�����¼�"));
		report_data_p = ptcl_report_data_req(HJ212_BYTES_TIME_CALI_REQ, RT_WAITING_NO);
		if((PTCL_REPORT_DATA *)0 != report_data_p)
		{
			//��ͷ
			report_data_p->data_len = _hj212_frm_data_head(report_data_p->pdata, HJ212_BYTES_TIME_CALI_REQ, (rt_uint8_t *)0, RT_TRUE, HJ212_CN_REQ_SYS_TIME, HJ212_FLAG_PTCL_VER + HJ212_FLAG_ACK_EN, 0, 0);
			if(!report_data_p->data_len)
			{
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]Уʱ�����ʧ��-��ͷ"));
				goto __exit;
			}
			//CP_END
			crc_val = _hj212_frm_cp_end(HJ212_BYTES_TIME_CALI_REQ - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len);
			if(crc_val)
			{
				report_data_p->data_len += crc_val;
			}
			else
			{
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]Уʱ�����ʧ��-cp_end"));
				goto __exit;
			}
			//��β����
			_hj212_frm_len(report_data_p->pdata, report_data_p->data_len);
			crc_val = _hj212_crc_value(report_data_p->pdata + HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN, report_data_p->data_len - HJ212_BYTES_FRAME_BAOTOU - HJ212_BYTES_FRAME_LEN);
			crc_val = _hj212_frm_baowei(DEBUG_INFO_TYPE_HJ212 - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, crc_val);
			if(crc_val)
			{
				report_data_p->data_len += crc_val;

				report_data_p->data_id			= _hj212_data_id(report_data_p->pdata, report_data_p->data_len);
				report_data_p->fun_csq_update	= (void *)0;
				report_data_p->need_reply		= HJ212_REPORT_REPLY_EN;
				report_data_p->fcb_value		= hj212_get_recount();
				report_data_p->ptcl_type		= PTCL_PTCL_TYPE_HJ212;
			}
			else
			{
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]Уʱ�����ʧ��-��β"));
				goto __exit;
			}
			//Уʱ������
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]Уʱ��[%d]:", report_data_p->data_id));
			DEBUG_INFO_OUTPUT(DEBUG_INFO_TYPE_HJ212, (report_data_p->pdata, report_data_p->data_len));
			//ѹ���Ա�����
			if(RT_TRUE == ptcl_auto_report_post(report_data_p))
			{
				report_data_p = (PTCL_REPORT_DATA *)0;
			}

__exit:
			if((PTCL_REPORT_DATA *)0 != report_data_p)
			{
				rt_mp_free((void *)report_data_p);
			}
		}

		lpm_cpu_ref(RT_FALSE);
	}
}

static void _hj212_task_report_rtd(void *parg)
{
	rt_uint8_t			i;
	rt_uint16_t			len;
	rt_uint32_t			run_sta;
	SAMPLE_DATA_TYPE	data_type;
	PTCL_REPORT_DATA	*report_data_p;
	struct tm			time;

	while(1)
	{
		if(RT_EOK != rt_event_recv(&m_hj212_event_module, HJ212_EVENT_REPORT_RTD, RT_EVENT_FLAG_AND + RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, (rt_uint32_t *)0))
		{
			while(1);
		}
		lpm_cpu_ref(RT_TRUE);
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]ʵʱ�����ϱ�"));

		time = rtcee_rtc_get_calendar();

		//ʵʱ����
		if(RT_TRUE == hj212_get_rtd_en())
		{
			report_data_p = ptcl_report_data_req(HJ212_BYTES_DATA_RTD, RT_WAITING_NO);
			if((PTCL_REPORT_DATA *)0 != report_data_p)
			{
				//���
				report_data_p->data_len = _hj212_frm_data_head_ex(report_data_p->pdata, HJ212_BYTES_DATA_RTD, &time, RT_TRUE, HJ212_CN_RTD_DATA, HJ212_FLAG_PTCL_VER + HJ212_FLAG_ACK_EN, 0, 0);
				if(!report_data_p->data_len)
				{
					goto __report_rtd;
				}
				//����ʱ��
				len = _hj212_frm_data_time(HJ212_BYTES_DATA_RTD - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, &time);
				if(len)
				{
					report_data_p->data_len += len;
				}
				else
				{
					goto __report_rtd;
				}
				//��ˮ����
				data_type = sample_get_cur_data(SAMPLE_SENSOR_WUSHUI);
				if(RT_TRUE == data_type.sta)
				{
					len = _hj212_frm_data(HJ212_BYTES_DATA_RTD - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, HJ212_FACTOR_WUSHUI, HJ212_FIELD_NAME_DATA_RTD, data_type.value, HJ212_BIT_FACTOR_WUSHUI);
				}
				else
				{
					len = _hj212_frm_data(HJ212_BYTES_DATA_RTD - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, HJ212_FACTOR_WUSHUI, HJ212_FIELD_NAME_DATA_RTD, 0, 0);
				}
				if(len)
				{
					report_data_p->data_len += len;
				}
				else
				{
					goto __report_rtd;
				}
				if(RT_TRUE == data_type.sta)
				{
					len = _hj212_frm_data_flag(HJ212_BYTES_DATA_RTD - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, HJ212_FACTOR_WUSHUI, HJ212_DATA_FLAG_RUN);
				}
				else
				{
					len = _hj212_frm_data_flag(HJ212_BYTES_DATA_RTD - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, HJ212_FACTOR_WUSHUI, HJ212_DATA_FLAG_ERR);
				}
				if(len)
				{
					report_data_p->data_len += len;
				}
				else
				{
					goto __report_rtd;
				}
				//��ѧ������COD
				data_type = sample_get_cur_data(SAMPLE_SENSOR_COD);
				if(RT_TRUE == data_type.sta)
				{
					len = _hj212_frm_data(HJ212_BYTES_DATA_RTD - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, HJ212_FACTOR_COD, HJ212_FIELD_NAME_DATA_RTD, data_type.value, HJ212_BIT_FACTOR_COD);
				}
				else
				{
					len = _hj212_frm_data(HJ212_BYTES_DATA_RTD - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, HJ212_FACTOR_COD, HJ212_FIELD_NAME_DATA_RTD, 0, 0);
				}
				if(len)
				{
					report_data_p->data_len += len;
				}
				else
				{
					goto __report_rtd;
				}
				if(RT_TRUE == data_type.sta)
				{
					len = _hj212_frm_data_flag(HJ212_BYTES_DATA_RTD - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, HJ212_FACTOR_COD, HJ212_DATA_FLAG_RUN);
				}
				else
				{
					len = _hj212_frm_data_flag(HJ212_BYTES_DATA_RTD - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, HJ212_FACTOR_COD, HJ212_DATA_FLAG_ERR);
				}
				if(len)
				{
					report_data_p->data_len += len;
				}
				else
				{
					goto __report_rtd;
				}
				//CP_END
				report_data_p->data_len--;
				len = _hj212_frm_cp_end(HJ212_BYTES_DATA_RTD - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len);
				if(len)
				{
					report_data_p->data_len += len;
				}
				else
				{
					goto __report_rtd;
				}
				//��β����
				_hj212_frm_len(report_data_p->pdata, report_data_p->data_len);
				len = _hj212_crc_value(report_data_p->pdata + HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN, report_data_p->data_len - HJ212_BYTES_FRAME_BAOTOU - HJ212_BYTES_FRAME_LEN);
				len = _hj212_frm_baowei(HJ212_BYTES_DATA_RTD - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, len);
				if(len)
				{
					report_data_p->data_len += len;

					report_data_p->data_id			= _hj212_data_id(report_data_p->pdata, report_data_p->data_len);
					report_data_p->fun_csq_update	= (void *)0;
					report_data_p->need_reply		= HJ212_REPORT_REPLY_EN;
					report_data_p->fcb_value		= hj212_get_recount();
					report_data_p->ptcl_type		= PTCL_PTCL_TYPE_HJ212;
				}
				else
				{
					goto __report_rtd;
				}
				//ʵʱ��������
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]ʵʱ����:"));
				DEBUG_INFO_OUTPUT(DEBUG_INFO_TYPE_HJ212, (report_data_p->pdata, report_data_p->data_len));

				//ѹ���Ա�����
				if(RT_TRUE == ptcl_auto_report_post(report_data_p))
				{
					report_data_p = (PTCL_REPORT_DATA *)0;
				}
__report_rtd:
				if((PTCL_REPORT_DATA *)0 != report_data_p)
				{
					rt_mp_free((void *)report_data_p);
				}
			}
		}
		
		//��������
		if(RT_TRUE == hj212_get_rs_en())
		{
			report_data_p = ptcl_report_data_req(HJ212_BYTES_DATA_RTD, RT_WAITING_NO);
			if((PTCL_REPORT_DATA *)0 != report_data_p)
			{
				//���
				report_data_p->data_len = _hj212_frm_data_head_ex(report_data_p->pdata, HJ212_BYTES_DATA_RTD, &time, RT_TRUE, HJ212_CN_RS_DATA, HJ212_FLAG_PTCL_VER + HJ212_FLAG_ACK_EN, 0, 0);
				if(!report_data_p->data_len)
				{
					goto __report_rs;
				}
				//����ʱ��
				len = _hj212_frm_data_time(HJ212_BYTES_DATA_RTD - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, &time);
				if(len)
				{
					report_data_p->data_len += len;
				}
				else
				{
					goto __report_rs;
				}
				//�豸����״̬
				run_sta = equip_get_run_sta();
				for(i = 0; i < EQUIP_NUM_EQUIP; i++)
				{
					len = _hj212_frm_run_sta(HJ212_BYTES_DATA_RTD - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, i, (run_sta & (1 << i)) ? RT_TRUE : RT_FALSE);
					if(len)
					{
						report_data_p->data_len += len;
					}
					else
					{
						goto __report_rs;
					}
				}
				//CP_END
				report_data_p->data_len--;
				len = _hj212_frm_cp_end(HJ212_BYTES_DATA_RTD - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len);
				if(len)
				{
					report_data_p->data_len += len;
				}
				else
				{
					goto __report_rs;
				}
				//��β����
				_hj212_frm_len(report_data_p->pdata, report_data_p->data_len);
				len = _hj212_crc_value(report_data_p->pdata + HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN, report_data_p->data_len - HJ212_BYTES_FRAME_BAOTOU - HJ212_BYTES_FRAME_LEN);
				len = _hj212_frm_baowei(HJ212_BYTES_DATA_RTD - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, len);
				if(len)
				{
					report_data_p->data_len += len;

					report_data_p->data_id			= _hj212_data_id(report_data_p->pdata, report_data_p->data_len);
					report_data_p->fun_csq_update	= (void *)0;
					report_data_p->need_reply		= HJ212_REPORT_REPLY_EN;
					report_data_p->fcb_value		= hj212_get_recount();
					report_data_p->ptcl_type		= PTCL_PTCL_TYPE_HJ212;
				}
				else
				{
					goto __report_rs;
				}
				//������������
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]��������:"));
				DEBUG_INFO_OUTPUT(DEBUG_INFO_TYPE_HJ212, (report_data_p->pdata, report_data_p->data_len));

				//ѹ���Ա�����
				if(RT_TRUE == ptcl_auto_report_post(report_data_p))
				{
					report_data_p = (PTCL_REPORT_DATA *)0;
				}
__report_rs:
				if((PTCL_REPORT_DATA *)0 != report_data_p)
				{
					rt_mp_free((void *)report_data_p);
				}
			}
		}

		lpm_cpu_ref(RT_FALSE);
	}
}

static void _hj212_task_time_tick(void *parg)
{
	rt_uint16_t sec = 0, period;

	while(1)
	{
		tevent_pend(TEVENT_EVENT_SEC);
		lpm_cpu_ref(RT_TRUE);

		period = hj212_get_rtd_interval();
		if(period)
		{
			if(++sec >= period)
			{
				sec = 0;

				if(RT_EOK != rt_event_send(&m_hj212_event_module, HJ212_EVENT_REPORT_RTD))
				{
					while(1);
				}
			}
		}
		
		lpm_cpu_ref(RT_FALSE);
	}
}

static void _hj212_task_data_hdr(void *parg)
{
	struct tm			time;
	rt_uint32_t			unix;
	SAMPLE_DATA_TYPE	q, c, d;

	while(1)
	{
		tevent_pend(TEVENT_EVENT_MIN);
		lpm_cpu_ref(RT_TRUE);

		time = rtcee_rtc_get_calendar();
		time.tm_sec = 0;
		unix = rtcee_rtc_calendar_to_unix(time);

		//��ˮ��������/��
		q = sample_get_cur_data(SAMPLE_SENSOR_WUSHUI);
		sample_store_data(&time, SAMPLE_SENSOR_WUSHUI + SAMPLE_SENSOR_DATA_CUR, &q);
		//��ˮ�ŷ����������ϱ�ʱҪת����������
		if(RT_TRUE == q.sta)
		{
			d.sta	= RT_TRUE;
			d.value	= q.value;
			d.value	*= 60;
			sample_store_data(&time, SAMPLE_SENSOR_WUSHUI + SAMPLE_SENSOR_DATA_COU_M, &d);
		}

		//��ѧ������CODŨ�ȣ�����/��
		c = sample_get_cur_data(SAMPLE_SENSOR_COD);
		sample_store_data(&time, SAMPLE_SENSOR_COD + SAMPLE_SENSOR_DATA_CUR, &c);
		//��ѧ������COD�ŷ��������ˣ��ϱ�ʱҪת����ǧ��
		if((RT_TRUE == d.sta) && (RT_TRUE == c.sta))
		{
			q.sta	= RT_TRUE;
			q.value	= d.value;
			q.value	*= c.value;
			sample_store_data(&time, SAMPLE_SENSOR_COD + SAMPLE_SENSOR_DATA_COU_M, &q);
		}

		//����
		if(time.tm_min)
		{
			goto __exit;
		}
		//��ˮ������Сֵ
		sample_query_min(SAMPLE_SENSOR_WUSHUI + SAMPLE_SENSOR_DATA_CUR, unix - 3540, unix, 60, &q);
		sample_store_data(&time, SAMPLE_SENSOR_WUSHUI + SAMPLE_SENSOR_DATA_MIN_H, &q);
		//��ˮ����ƽ��ֵ
		sample_query_avg(SAMPLE_SENSOR_WUSHUI + SAMPLE_SENSOR_DATA_CUR, unix - 3540, unix, 60, &q);
		sample_store_data(&time, SAMPLE_SENSOR_WUSHUI + SAMPLE_SENSOR_DATA_AVG_H, &q);
		//��ˮ�������ֵ
		sample_query_max(SAMPLE_SENSOR_WUSHUI + SAMPLE_SENSOR_DATA_CUR, unix - 3540, unix, 60, &q);
		sample_store_data(&time, SAMPLE_SENSOR_WUSHUI + SAMPLE_SENSOR_DATA_MAX_H, &q);
		//��ˮ�ŷ���
		sample_query_cou(SAMPLE_SENSOR_WUSHUI + SAMPLE_SENSOR_DATA_COU_M, unix - 3540, unix, 60, &q);
		sample_store_data(&time, SAMPLE_SENSOR_WUSHUI + SAMPLE_SENSOR_DATA_COU_H, &q);
		
		//��ѧ������CODŨ����Сֵ
		sample_query_min(SAMPLE_SENSOR_COD + SAMPLE_SENSOR_DATA_CUR, unix - 3540, unix, 60, &q);
		sample_store_data(&time, SAMPLE_SENSOR_COD + SAMPLE_SENSOR_DATA_MIN_H, &q);
		//��ѧ������CODŨ��ƽ��ֵ
		sample_query_avg(SAMPLE_SENSOR_COD + SAMPLE_SENSOR_DATA_CUR, unix - 3540, unix, 60, &q);
		sample_store_data(&time, SAMPLE_SENSOR_COD + SAMPLE_SENSOR_DATA_AVG_H, &q);
		//��ѧ������CODŨ�����ֵ
		sample_query_max(SAMPLE_SENSOR_COD + SAMPLE_SENSOR_DATA_CUR, unix - 3540, unix, 60, &q);
		sample_store_data(&time, SAMPLE_SENSOR_COD + SAMPLE_SENSOR_DATA_MAX_H, &q);
		//��ѧ������COD�ŷ���
		sample_query_cou(SAMPLE_SENSOR_COD + SAMPLE_SENSOR_DATA_COU_M, unix - 3540, unix, 60, &q);
		sample_store_data(&time, SAMPLE_SENSOR_COD + SAMPLE_SENSOR_DATA_COU_H, &q);

		//����
		if(time.tm_hour)
		{
			goto __exit;
		}
		//��ˮ������Сֵ
		sample_query_min(SAMPLE_SENSOR_WUSHUI + SAMPLE_SENSOR_DATA_MIN_H, unix - 82800, unix, 3600, &q);
		sample_store_data(&time, SAMPLE_SENSOR_WUSHUI + SAMPLE_SENSOR_DATA_MIN_D, &q);
		//��ˮ����ƽ��ֵ
		sample_query_avg(SAMPLE_SENSOR_WUSHUI + SAMPLE_SENSOR_DATA_AVG_H, unix - 82800, unix, 3600, &q);
		sample_store_data(&time, SAMPLE_SENSOR_WUSHUI + SAMPLE_SENSOR_DATA_AVG_D, &q);
		//��ˮ�������ֵ
		sample_query_max(SAMPLE_SENSOR_WUSHUI + SAMPLE_SENSOR_DATA_MAX_H, unix - 82800, unix, 3600, &q);
		sample_store_data(&time, SAMPLE_SENSOR_WUSHUI + SAMPLE_SENSOR_DATA_MAX_D, &q);
		//��ˮ�ŷ���
		sample_query_cou(SAMPLE_SENSOR_WUSHUI + SAMPLE_SENSOR_DATA_COU_H, unix - 82800, unix, 3600, &q);
		sample_store_data(&time, SAMPLE_SENSOR_WUSHUI + SAMPLE_SENSOR_DATA_COU_D, &q);
		
		//��ѧ������CODŨ����Сֵ
		sample_query_min(SAMPLE_SENSOR_COD + SAMPLE_SENSOR_DATA_MIN_H, unix - 82800, unix, 3600, &q);
		sample_store_data(&time, SAMPLE_SENSOR_COD + SAMPLE_SENSOR_DATA_MIN_D, &q);
		//��ѧ������CODŨ��ƽ��ֵ
		sample_query_avg(SAMPLE_SENSOR_COD + SAMPLE_SENSOR_DATA_AVG_H, unix - 82800, unix, 3600, &q);
		sample_store_data(&time, SAMPLE_SENSOR_COD + SAMPLE_SENSOR_DATA_AVG_D, &q);
		//��ѧ������CODŨ�����ֵ
		sample_query_max(SAMPLE_SENSOR_COD + SAMPLE_SENSOR_DATA_MAX_H, unix - 82800, unix, 3600, &q);
		sample_store_data(&time, SAMPLE_SENSOR_COD + SAMPLE_SENSOR_DATA_MAX_D, &q);
		//��ѧ������COD�ŷ���
		sample_query_cou(SAMPLE_SENSOR_COD + SAMPLE_SENSOR_DATA_COU_H, unix - 82800, unix, 3600, &q);
		sample_store_data(&time, SAMPLE_SENSOR_COD + SAMPLE_SENSOR_DATA_COU_D, &q);

__exit:
		//����ʱ��
		rt_mb_send_wait(&m_hj212_mb_report_min, (rt_ubase_t)unix, RT_WAITING_NO);
		
		lpm_cpu_ref(RT_FALSE);
	}
}

static void _hj212_task_report_min(void *parg)
{
	rt_uint32_t			unix;
	struct tm			time, ex_time;
	rt_uint16_t			period, tmp, pos;
	PTCL_REPORT_DATA	*report_data_p;
	float				run_time;

	//�ϴ�����ʱ��
	lpm_cpu_ref(RT_TRUE);
	
	report_data_p = ptcl_report_data_req(HJ212_BYTES_DATA_MIN, RT_WAITING_NO);
	if((PTCL_REPORT_DATA *)0 != report_data_p)
	{
		time	= rtcee_rtc_get_calendar();
		unix	= rtcee_rtc_calendar_to_unix(time);
		ex_time	= rtcee_rtc_unix_to_calendar(unix + 30);
		//���
		report_data_p->data_len = _hj212_frm_data_head_ex(report_data_p->pdata, HJ212_BYTES_DATA_MIN, &ex_time, RT_TRUE, HJ212_CN_RESTART_TIME, HJ212_FLAG_PTCL_VER + HJ212_FLAG_ACK_EN, 0, 0);
		if(!report_data_p->data_len)
		{
			goto __report_restart;
		}
		//����ʱ��
		tmp = _hj212_frm_data_time(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, &ex_time);
		if(tmp)
		{
			report_data_p->data_len += tmp;
		}
		else
		{
			goto __report_restart;
		}
		//����ʱ��
		tmp = _hj212_frm_restart_time(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, &time);
		if(tmp)
		{
			report_data_p->data_len += tmp;
		}
		else
		{
			goto __report_restart;
		}
		//CP_END
		tmp = _hj212_frm_cp_end(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len);
		if(tmp)
		{
			report_data_p->data_len += tmp;
		}
		else
		{
			goto __report_restart;
		}
		//��β����
		_hj212_frm_len(report_data_p->pdata, report_data_p->data_len);
		tmp = _hj212_crc_value(report_data_p->pdata + HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN, report_data_p->data_len - HJ212_BYTES_FRAME_BAOTOU - HJ212_BYTES_FRAME_LEN);
		tmp = _hj212_frm_baowei(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, tmp);
		if(tmp)
		{
			report_data_p->data_len += tmp;

			report_data_p->data_id			= _hj212_data_id(report_data_p->pdata, report_data_p->data_len);
			report_data_p->fun_csq_update	= (void *)0;
			report_data_p->need_reply		= HJ212_REPORT_REPLY_EN;
			report_data_p->fcb_value		= hj212_get_recount();
			report_data_p->ptcl_type		= PTCL_PTCL_TYPE_HJ212;
		}
		else
		{
			goto __report_restart;
		}
		//������������
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����ʱ������:"));
		DEBUG_INFO_OUTPUT(DEBUG_INFO_TYPE_HJ212, (report_data_p->pdata, report_data_p->data_len));

		//ѹ���Ա�����
		if(RT_TRUE == ptcl_auto_report_post(report_data_p))
		{
			report_data_p = (PTCL_REPORT_DATA *)0;
		}
__report_restart:
		if((PTCL_REPORT_DATA *)0 != report_data_p)
		{
			rt_mp_free((void *)report_data_p);
		}
	}
	
	lpm_cpu_ref(RT_FALSE);

	while(1)
	{
		if(RT_EOK != rt_mb_recv(&m_hj212_mb_report_min, (rt_ubase_t *)&unix, RT_WAITING_FOREVER))
		{
			while(1);
		}
		lpm_cpu_ref(RT_TRUE);

		time = rtcee_rtc_unix_to_calendar(unix);

		//���������ϱ�
		period = hj212_get_min_interval();
		if(!period)
		{
			goto __report_hour_day;
		}
		tmp = time.tm_hour;
		tmp *= 60;
		tmp += time.tm_min;
		if(tmp % period)
		{
			goto __report_hour_day;
		}
		report_data_p = ptcl_report_data_req(HJ212_BYTES_DATA_MIN, RT_WAITING_NO);
		if((PTCL_REPORT_DATA *)0 != report_data_p)
		{
			//���
			report_data_p->data_len = _hj212_frm_data_head_ex(report_data_p->pdata, HJ212_BYTES_DATA_MIN, &time, RT_TRUE, HJ212_CN_MIN_DATA, HJ212_FLAG_PTCL_VER + HJ212_FLAG_ACK_EN, 0, 0);
			if(!report_data_p->data_len)
			{
				goto __report_min;
			}
			pos = report_data_p->data_len;
			//����ʱ��
			ex_time = rtcee_rtc_unix_to_calendar(unix - period * 60);
			tmp = _hj212_frm_data_time(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, &ex_time);
			if(tmp)
			{
				report_data_p->data_len += tmp;
			}
			else
			{
				goto __report_min;
			}
			//��ˮ����
			tmp = _hj212_frm_data_all(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, HJ212_CN_MIN_DATA, SAMPLE_SENSOR_WUSHUI, HJ212_BIT_FACTOR_WUSHUI, unix + 60 - period * 60, unix, 1000, HJ212_FACTOR_WUSHUI);
			if(tmp)
			{
				report_data_p->data_len += tmp;
			}
			else
			{
				goto __report_min;
			}
			//��ѧ������COD����
			tmp = _hj212_frm_data_all(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, HJ212_CN_MIN_DATA, SAMPLE_SENSOR_COD, HJ212_BIT_FACTOR_COD, unix + 60 - period * 60, unix, 1000000, HJ212_FACTOR_COD);
			if(tmp)
			{
				report_data_p->data_len += tmp;
			}
			else
			{
				goto __report_min;
			}
			//CP_END
			report_data_p->data_len--;
			tmp = _hj212_frm_cp_end(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len);
			if(tmp)
			{
				report_data_p->data_len += tmp;
			}
			else
			{
				goto __report_min;
			}
			//��β����
			_hj212_frm_len(report_data_p->pdata, report_data_p->data_len);
			tmp = _hj212_crc_value(report_data_p->pdata + HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN, report_data_p->data_len - HJ212_BYTES_FRAME_BAOTOU - HJ212_BYTES_FRAME_LEN);
			tmp = _hj212_frm_baowei(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, tmp);
			if(tmp)
			{
				report_data_p->data_len += tmp;

				report_data_p->data_id			= _hj212_data_id(report_data_p->pdata, report_data_p->data_len);
				report_data_p->fun_csq_update	= (void *)0;
				report_data_p->need_reply		= HJ212_REPORT_REPLY_EN;
				report_data_p->fcb_value		= hj212_get_recount();
				report_data_p->ptcl_type		= PTCL_PTCL_TYPE_HJ212;
			}
			else
			{
				goto __report_min;
			}
			//������������
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]��������:"));
			DEBUG_INFO_OUTPUT(DEBUG_INFO_TYPE_HJ212, (report_data_p->pdata, report_data_p->data_len));

			//�洢������
			sample_store_data_ex(&time, HJ212_CN_MIN_DATA, report_data_p->data_len - pos - strlen(HJ212_FRAME_CP_END) - HJ212_BYTES_FRAME_CRC - HJ212_BYTES_FRAME_BAOWEI, report_data_p->pdata + pos);

			//ѹ���Ա�����
			if(RT_TRUE == ptcl_auto_report_post(report_data_p))
			{
				report_data_p = (PTCL_REPORT_DATA *)0;
			}
__report_min:
			if((PTCL_REPORT_DATA *)0 != report_data_p)
			{
				rt_mp_free((void *)report_data_p);
			}
		}

__report_hour_day:
		//Сʱ�����ϱ�
		if(time.tm_min)
		{
			goto __exit;
		}
		report_data_p = ptcl_report_data_req(HJ212_BYTES_DATA_MIN, RT_WAITING_NO);
		if((PTCL_REPORT_DATA *)0 != report_data_p)
		{
			//���
			report_data_p->data_len = _hj212_frm_data_head_ex(report_data_p->pdata, HJ212_BYTES_DATA_MIN, &time, RT_TRUE, HJ212_CN_HOUR_DATA, HJ212_FLAG_PTCL_VER + HJ212_FLAG_ACK_EN, 0, 0);
			if(!report_data_p->data_len)
			{
				goto __report_hour;
			}
			pos = report_data_p->data_len;
			//����ʱ��
			ex_time = rtcee_rtc_unix_to_calendar(unix - 3600);
			tmp = _hj212_frm_data_time(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, &ex_time);
			if(tmp)
			{
				report_data_p->data_len += tmp;
			}
			else
			{
				goto __report_hour;
			}
			//��ˮ����
			tmp = _hj212_frm_data_all(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, HJ212_CN_HOUR_DATA, SAMPLE_SENSOR_WUSHUI, HJ212_BIT_FACTOR_WUSHUI, unix - 3540, unix, 1000, HJ212_FACTOR_WUSHUI);
			if(tmp)
			{
				report_data_p->data_len += tmp;
			}
			else
			{
				goto __report_hour;
			}
			//��ѧ������COD����
			tmp = _hj212_frm_data_all(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, HJ212_CN_HOUR_DATA, SAMPLE_SENSOR_COD, HJ212_BIT_FACTOR_COD, unix - 3540, unix, 1000000, HJ212_FACTOR_COD);
			if(tmp)
			{
				report_data_p->data_len += tmp;
			}
			else
			{
				goto __report_hour;
			}
			//CP_END
			report_data_p->data_len--;
			tmp = _hj212_frm_cp_end(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len);
			if(tmp)
			{
				report_data_p->data_len += tmp;
			}
			else
			{
				goto __report_hour;
			}
			//��β����
			_hj212_frm_len(report_data_p->pdata, report_data_p->data_len);
			tmp = _hj212_crc_value(report_data_p->pdata + HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN, report_data_p->data_len - HJ212_BYTES_FRAME_BAOTOU - HJ212_BYTES_FRAME_LEN);
			tmp = _hj212_frm_baowei(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, tmp);
			if(tmp)
			{
				report_data_p->data_len += tmp;

				report_data_p->data_id			= _hj212_data_id(report_data_p->pdata, report_data_p->data_len);
				report_data_p->fun_csq_update	= (void *)0;
				report_data_p->need_reply		= HJ212_REPORT_REPLY_EN;
				report_data_p->fcb_value		= hj212_get_recount();
				report_data_p->ptcl_type		= PTCL_PTCL_TYPE_HJ212;
			}
			else
			{
				goto __report_hour;
			}
			//Сʱ��������
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]Сʱ����:"));
			DEBUG_INFO_OUTPUT(DEBUG_INFO_TYPE_HJ212, (report_data_p->pdata, report_data_p->data_len));

			//�洢������
			sample_store_data_ex(&time, HJ212_CN_HOUR_DATA, report_data_p->data_len - pos - strlen(HJ212_FRAME_CP_END) - HJ212_BYTES_FRAME_CRC - HJ212_BYTES_FRAME_BAOWEI, report_data_p->pdata + pos);

			//ѹ���Ա�����
			if(RT_TRUE == ptcl_auto_report_post(report_data_p))
			{
				report_data_p = (PTCL_REPORT_DATA *)0;
			}
__report_hour:
			if((PTCL_REPORT_DATA *)0 != report_data_p)
			{
				rt_mp_free((void *)report_data_p);
			}
		}

		//�������ϱ�
		if(time.tm_hour)
		{
			goto __exit;
		}
		//��Ⱦ��������
		report_data_p = ptcl_report_data_req(HJ212_BYTES_DATA_MIN, RT_WAITING_NO);
		if((PTCL_REPORT_DATA *)0 != report_data_p)
		{
			//���
			report_data_p->data_len = _hj212_frm_data_head_ex(report_data_p->pdata, HJ212_BYTES_DATA_MIN, &time, RT_TRUE, HJ212_CN_DAY_DATA, HJ212_FLAG_PTCL_VER + HJ212_FLAG_ACK_EN, 0, 0);
			if(!report_data_p->data_len)
			{
				goto __report_day;
			}
			pos = report_data_p->data_len;
			//����ʱ��
			ex_time = rtcee_rtc_unix_to_calendar(unix - 86400);
			tmp = _hj212_frm_data_time(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, &ex_time);
			if(tmp)
			{
				report_data_p->data_len += tmp;
			}
			else
			{
				goto __report_day;
			}
			//��ˮ����
			tmp = _hj212_frm_data_all(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, HJ212_CN_DAY_DATA, SAMPLE_SENSOR_WUSHUI, HJ212_BIT_FACTOR_WUSHUI, unix - 82800, unix, 1000, HJ212_FACTOR_WUSHUI);
			if(tmp)
			{
				report_data_p->data_len += tmp;
			}
			else
			{
				goto __report_day;
			}
			//��ѧ������COD����
			tmp = _hj212_frm_data_all(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, HJ212_CN_DAY_DATA, SAMPLE_SENSOR_COD, HJ212_BIT_FACTOR_COD, unix - 82800, unix, 1000000, HJ212_FACTOR_COD);
			if(tmp)
			{
				report_data_p->data_len += tmp;
			}
			else
			{
				goto __report_day;
			}
			//CP_END
			report_data_p->data_len--;
			tmp = _hj212_frm_cp_end(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len);
			if(tmp)
			{
				report_data_p->data_len += tmp;
			}
			else
			{
				goto __report_day;
			}
			//��β����
			_hj212_frm_len(report_data_p->pdata, report_data_p->data_len);
			tmp = _hj212_crc_value(report_data_p->pdata + HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN, report_data_p->data_len - HJ212_BYTES_FRAME_BAOTOU - HJ212_BYTES_FRAME_LEN);
			tmp = _hj212_frm_baowei(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, tmp);
			if(tmp)
			{
				report_data_p->data_len += tmp;

				report_data_p->data_id			= _hj212_data_id(report_data_p->pdata, report_data_p->data_len);
				report_data_p->fun_csq_update	= (void *)0;
				report_data_p->need_reply		= HJ212_REPORT_REPLY_EN;
				report_data_p->fcb_value		= hj212_get_recount();
				report_data_p->ptcl_type		= PTCL_PTCL_TYPE_HJ212;
			}
			else
			{
				goto __report_day;
			}
			//��Ⱦ������������
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]��Ⱦ��������:"));
			DEBUG_INFO_OUTPUT(DEBUG_INFO_TYPE_HJ212, (report_data_p->pdata, report_data_p->data_len));

			//�洢������
			sample_store_data_ex(&time, HJ212_CN_DAY_DATA, report_data_p->data_len - pos - strlen(HJ212_FRAME_CP_END) - HJ212_BYTES_FRAME_CRC - HJ212_BYTES_FRAME_BAOWEI, report_data_p->pdata + pos);

			//ѹ���Ա�����
			if(RT_TRUE == ptcl_auto_report_post(report_data_p))
			{
				report_data_p = (PTCL_REPORT_DATA *)0;
			}
__report_day:
			if((PTCL_REPORT_DATA *)0 != report_data_p)
			{
				rt_mp_free((void *)report_data_p);
			}
		}

		//�豸����ʱ��������
		report_data_p = ptcl_report_data_req(HJ212_BYTES_DATA_MIN, RT_WAITING_NO);
		if((PTCL_REPORT_DATA *)0 != report_data_p)
		{
			//���
			report_data_p->data_len = _hj212_frm_data_head_ex(report_data_p->pdata, HJ212_BYTES_DATA_MIN, &time, RT_TRUE, HJ212_CN_DAY_RS_DATA, HJ212_FLAG_PTCL_VER + HJ212_FLAG_ACK_EN, 0, 0);
			if(!report_data_p->data_len)
			{
				goto __report_day_sbrt;
			}
			pos = report_data_p->data_len;
			//����ʱ��
			ex_time = rtcee_rtc_unix_to_calendar(unix - 86400);
			tmp = _hj212_frm_data_time(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, &ex_time);
			if(tmp)
			{
				report_data_p->data_len += tmp;
			}
			else
			{
				goto __report_day_sbrt;
			}
			//�豸����ʱ��
			for(period = 0; period < EQUIP_NUM_EQUIP; period++)
			{
				run_time = equip_get_run_time(period, RT_TRUE);
				run_time /= 3600;
				tmp = _hj212_frm_data_sbrt(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, period + 1, run_time);
				if(tmp)
				{
					report_data_p->data_len += tmp;
				}
				else
				{
					goto __report_day_sbrt;
				}
			}
			//CP_END
			report_data_p->data_len--;
			tmp = _hj212_frm_cp_end(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len);
			if(tmp)
			{
				report_data_p->data_len += tmp;
			}
			else
			{
				goto __report_day_sbrt;
			}
			//��β����
			_hj212_frm_len(report_data_p->pdata, report_data_p->data_len);
			tmp = _hj212_crc_value(report_data_p->pdata + HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN, report_data_p->data_len - HJ212_BYTES_FRAME_BAOTOU - HJ212_BYTES_FRAME_LEN);
			tmp = _hj212_frm_baowei(HJ212_BYTES_DATA_MIN - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, tmp);
			if(tmp)
			{
				report_data_p->data_len += tmp;

				report_data_p->data_id			= _hj212_data_id(report_data_p->pdata, report_data_p->data_len);
				report_data_p->fun_csq_update	= (void *)0;
				report_data_p->need_reply		= HJ212_REPORT_REPLY_EN;
				report_data_p->fcb_value		= hj212_get_recount();
				report_data_p->ptcl_type		= PTCL_PTCL_TYPE_HJ212;
			}
			else
			{
				goto __report_day_sbrt;
			}
			//�豸����ʱ������������
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]�豸����ʱ��������:"));
			DEBUG_INFO_OUTPUT(DEBUG_INFO_TYPE_HJ212, (report_data_p->pdata, report_data_p->data_len));

			//�洢������
			sample_store_data_ex(&time, HJ212_CN_DAY_RS_DATA, report_data_p->data_len - pos - strlen(HJ212_FRAME_CP_END) - HJ212_BYTES_FRAME_CRC - HJ212_BYTES_FRAME_BAOWEI, report_data_p->pdata + pos);

			//ѹ���Ա�����
			if(RT_TRUE == ptcl_auto_report_post(report_data_p))
			{
				report_data_p = (PTCL_REPORT_DATA *)0;
			}
__report_day_sbrt:
			if((PTCL_REPORT_DATA *)0 != report_data_p)
			{
				rt_mp_free((void *)report_data_p);
			}
		}

__exit:
		lpm_cpu_ref(RT_FALSE);
	}
}

static int _hj212_components_init(void)
{
	//�¼�
	if(RT_EOK != rt_event_init(&m_hj212_event_module, "hj212", RT_IPC_FLAG_PRIO))
	{
		while(1);
	}
	if(RT_EOK != rt_event_send(&m_hj212_event_module, HJ212_EVENT_INIT_VALUE))
	{
		while(1);
	}
	//��Ϣ����
	if(RT_EOK != rt_mb_init(&m_hj212_mb_report_min, "hj212", (void *)&m_hj212_msgpool_report_min, 1, RT_IPC_FLAG_PRIO))
	{
		while(1);
	}
	//��������
	if(RT_TRUE == _hj212_get_param_set(&m_hj212_param_set))
	{
		_hj212_validate_param_set(&m_hj212_param_set);
	}
	else
	{
		_hj212_reset_param_set(&m_hj212_param_set);
	}
	
	return 0;
}
INIT_COMPONENT_EXPORT(_hj212_components_init);

static int _hj212_app_init(void)
{
	//Уʱ����
	if(RT_EOK != rt_thread_init(&m_hj212_thread_time_cali_req, "hj212", _hj212_task_time_cali_req, (void *)0, (void *)m_hj212_stk_time_cali_req, HJ212_STK_TIME_CALI_REQ, HJ212_PRIO_TIME_CALI_REQ, 2))
	{
		while(1);
	}
	if(RT_EOK != rt_thread_startup(&m_hj212_thread_time_cali_req))
	{
		while(1);
	}
	//ʵʱ�����ϱ�����
	if(RT_EOK != rt_thread_init(&m_hj212_thread_report_rtd, "hj212", _hj212_task_report_rtd, (void *)0, (void *)m_hj212_stk_report_rtd, HJ212_STK_REPORT_RTD, HJ212_PRIO_REPORT_RTD, 2))
	{
		while(1);
	}
	if(RT_EOK != rt_thread_startup(&m_hj212_thread_report_rtd))
	{
		while(1);
	}
	//tick����
	if(RT_EOK != rt_thread_init(&m_hj212_thread_time_tick, "hj212", _hj212_task_time_tick, (void *)0, (void *)m_hj212_stk_time_tick, HJ212_STK_TIME_TICK, HJ212_PRIO_TIME_TICK, 2))
	{
		while(1);
	}
	if(RT_EOK != rt_thread_startup(&m_hj212_thread_time_tick))
	{
		while(1);
	}
	//���ݴ�������
	if(RT_EOK != rt_thread_init(&m_hj212_thread_data_hdr, "hj212", _hj212_task_data_hdr, (void *)0, (void *)m_hj212_stk_data_hdr, HJ212_STK_DATA_HDR, HJ212_PRIO_DATA_HDR, 2))
	{
		while(1);
	}
	if(RT_EOK != rt_thread_startup(&m_hj212_thread_data_hdr))
	{
		while(1);
	}
	//���ӡ�Сʱ���������ϱ�����
	if(RT_EOK != rt_thread_init(&m_hj212_thread_report_min, "hj212", _hj212_task_report_min, (void *)0, (void *)m_hj212_stk_report_min, HJ212_STK_REPORT_MIN, HJ212_PRIO_REPORT_MIN, 2))
	{
		while(1);
	}
	if(RT_EOK != rt_thread_startup(&m_hj212_thread_report_min))
	{
		while(1);
	}
	
	return 0;
}
INIT_APP_EXPORT(_hj212_app_init);



void hj212_get_st(rt_uint8_t *pdata)
{
	if((rt_uint8_t *)0 == pdata)
	{
		return;
	}
	
	_hj212_param_pend(HJ212_EVENT_PARAM_ST);
	memcpy((void *)pdata, (void *)m_hj212_st, HJ212_BYTES_ST);
	_hj212_param_post(HJ212_EVENT_PARAM_ST);
}

void hj212_set_st(rt_uint8_t const *pdata)
{
	if((rt_uint8_t *)0 == pdata)
	{
		return;
	}
	
	_hj212_param_pend(HJ212_EVENT_PARAM_ST);
	memcpy((void *)m_hj212_st, (void *)pdata, HJ212_BYTES_ST);
	_hj212_set_st(m_hj212_st);
	_hj212_param_post(HJ212_EVENT_PARAM_ST);
}

void hj212_get_pw(rt_uint8_t *pdata)
{
	if((rt_uint8_t *)0 == pdata)
	{
		return;
	}
	
	_hj212_param_pend(HJ212_EVENT_PARAM_PW);
	memcpy((void *)pdata, (void *)m_hj212_pw, HJ212_BYTES_PW);
	_hj212_param_post(HJ212_EVENT_PARAM_PW);
}

void hj212_set_pw(rt_uint8_t const *pdata)
{
	if((rt_uint8_t *)0 == pdata)
	{
		return;
	}
	
	_hj212_param_pend(HJ212_EVENT_PARAM_PW);
	memcpy((void *)m_hj212_pw, (void *)pdata, HJ212_BYTES_PW);
	_hj212_set_pw(m_hj212_pw);
	_hj212_param_post(HJ212_EVENT_PARAM_PW);
}

void hj212_get_mn(rt_uint8_t *pdata)
{
	if((rt_uint8_t *)0 == pdata)
	{
		return;
	}
	
	_hj212_param_pend(HJ212_EVENT_PARAM_MN);
	memcpy((void *)pdata, (void *)m_hj212_mn, HJ212_BYTES_MN);
	_hj212_param_post(HJ212_EVENT_PARAM_MN);
}

void hj212_set_mn(rt_uint8_t const *pdata)
{
	if((rt_uint8_t *)0 == pdata)
	{
		return;
	}
	
	_hj212_param_pend(HJ212_EVENT_PARAM_MN);
	memcpy((void *)m_hj212_mn, (void *)pdata, HJ212_BYTES_MN);
	_hj212_set_mn(m_hj212_mn);
	_hj212_param_post(HJ212_EVENT_PARAM_MN);
}

rt_uint16_t hj212_get_over_time(void)
{
	rt_uint16_t over_time;

	_hj212_param_pend(HJ212_EVENT_PARAM_OT);
	over_time = m_hj212_over_time;
	_hj212_param_post(HJ212_EVENT_PARAM_OT);

	return over_time;
}

void hj212_set_over_time(rt_uint16_t over_time)
{
	if(!over_time)
	{
		return;
	}

	_hj212_param_pend(HJ212_EVENT_PARAM_OT);
	if(m_hj212_over_time != over_time)
	{
		m_hj212_over_time = over_time;
		_hj212_set_over_time(m_hj212_over_time);
	}
	_hj212_param_post(HJ212_EVENT_PARAM_OT);
}

rt_uint8_t hj212_get_recount(void)
{
	rt_uint8_t recount;

	_hj212_param_pend(HJ212_EVENT_PARAM_RECOUNT);
	recount = m_hj212_recount;
	_hj212_param_post(HJ212_EVENT_PARAM_RECOUNT);

	return recount;
}

void hj212_set_recount(rt_uint8_t recount)
{
	_hj212_param_pend(HJ212_EVENT_PARAM_RECOUNT);
	if(m_hj212_recount != recount)
	{
		m_hj212_recount = recount;
		_hj212_set_recount(m_hj212_recount);
	}
	_hj212_param_post(HJ212_EVENT_PARAM_RECOUNT);
}

rt_uint8_t hj212_get_rtd_en(void)
{
	rt_uint8_t rtd_en;

	_hj212_param_pend(HJ212_EVENT_PARAM_RTD_EN);
	rtd_en = m_hj212_rtd_en;
	_hj212_param_post(HJ212_EVENT_PARAM_RTD_EN);

	return rtd_en;
}

void hj212_set_rtd_en(rt_uint8_t rtd_en)
{
	if((RT_FALSE != rtd_en) && (RT_TRUE != rtd_en))
	{
		return;
	}
	
	_hj212_param_pend(HJ212_EVENT_PARAM_RTD_EN);
	if(m_hj212_rtd_en != rtd_en)
	{
		m_hj212_rtd_en = rtd_en;
		_hj212_set_rtd_en(m_hj212_rtd_en);
	}
	_hj212_param_post(HJ212_EVENT_PARAM_RTD_EN);
}

rt_uint16_t hj212_get_rtd_interval(void)
{
	rt_uint16_t rtd_interval;

	_hj212_param_pend(HJ212_EVENT_PARAM_RTD_INTERVAL);
	rtd_interval = m_hj212_rtd_interval;
	_hj212_param_post(HJ212_EVENT_PARAM_RTD_INTERVAL);

	return rtd_interval;
}

void hj212_set_rtd_interval(rt_uint16_t rtd_interval)
{
	_hj212_param_pend(HJ212_EVENT_PARAM_RTD_INTERVAL);
	if(m_hj212_rtd_interval != rtd_interval)
	{
		m_hj212_rtd_interval = rtd_interval;
		_hj212_set_rtd_interval(m_hj212_rtd_interval);
	}
	_hj212_param_post(HJ212_EVENT_PARAM_RTD_INTERVAL);
}

rt_uint16_t hj212_get_min_interval(void)
{
	rt_uint16_t min_interval;

	_hj212_param_pend(HJ212_EVENT_PARAM_MIN_INTERVAL);
	min_interval = m_hj212_min_interval;
	_hj212_param_post(HJ212_EVENT_PARAM_MIN_INTERVAL);

	return min_interval;
}

void hj212_set_min_interval(rt_uint16_t min_interval)
{
	_hj212_param_pend(HJ212_EVENT_PARAM_MIN_INTERVAL);
	if(m_hj212_min_interval != min_interval)
	{
		m_hj212_min_interval = min_interval;
		_hj212_set_min_interval(m_hj212_min_interval);
	}
	_hj212_param_post(HJ212_EVENT_PARAM_MIN_INTERVAL);
}

rt_uint8_t hj212_get_rs_en(void)
{
	rt_uint8_t en;

	_hj212_param_pend(HJ212_EVENT_PARAM_RS_EN);
	en = m_hj212_rs_en;
	_hj212_param_post(HJ212_EVENT_PARAM_RS_EN);

	return en;
}

void hj212_set_rs_en(rt_uint8_t en)
{
	if((RT_FALSE != en) && (RT_TRUE != en))
	{
		return;
	}
	
	_hj212_param_pend(HJ212_EVENT_PARAM_RS_EN);
	if(m_hj212_rs_en != en)
	{
		m_hj212_rs_en = en;
		_hj212_set_rs_en(m_hj212_rs_en);
	}
	_hj212_param_post(HJ212_EVENT_PARAM_RS_EN);
}

void hj212_param_restore(void)
{
	_hj212_param_pend(HJ212_EVENT_PARAM_ALL);

	//ST
	memcpy((void *)m_hj212_st, (void *)HJ212_DEF_VAL_ST, HJ212_BYTES_ST);
	//PW
	memcpy((void *)m_hj212_pw, (void *)HJ212_DEF_VAL_PW, HJ212_BYTES_PW);
	//MN
	memcpy((void *)m_hj212_mn, (void *)HJ212_DEF_VAL_MN, HJ212_BYTES_MN);
	//over_time
	m_hj212_over_time		= HJ212_DEF_VAL_OT;
	//recount
	m_hj212_recount			= HJ212_DEF_VAL_RECOUNT;
	//rtd_en
	m_hj212_rtd_en			= RT_TRUE;
	//rtd_interval
	m_hj212_rtd_interval	= HJ212_DEF_VAL_RTD_INTERVAL;
	//min_interval
	m_hj212_min_interval	= HJ212_DEF_VAL_MIN_INTERVAL;
	//rs_en
	m_hj212_rs_en			= RT_TRUE;

	_hj212_set_param_set(&m_hj212_param_set);
	
	_hj212_param_post(HJ212_EVENT_PARAM_ALL);
}

rt_uint16_t hj212_heart_encoder(rt_uint8_t *pdata, rt_uint16_t data_len, rt_uint8_t ch)
{
	rt_uint16_t len;

	len = snprintf((char *)pdata, data_len, "HJ 212-2017[%d]\r\n", ch);
	if(len >= data_len)
	{
		len = 0;
	}

	return len;
}

void hj212_time_cali_req(void)
{
	if(RT_EOK != rt_event_send(&m_hj212_event_module, HJ212_EVENT_TIME_CALI_REQ))
	{
		while(1);
	}
}

rt_uint8_t hj212_data_decoder(PTCL_REPORT_DATA *report_data_p, PTCL_RECV_DATA const *recv_data_p, PTCL_PARAM_INFO **param_info_pp)
{
	rt_uint16_t		i, data_len, crc_val, cn;
	rt_uint8_t		rtn, qn_en = RT_FALSE, flag;
	PTCL_PARAM_INFO	*param_info_p;
	rt_mailbox_t	pmb;
	rt_ubase_t		msg_pool;
	rt_uint32_t		unix_low, unix_high;

	i = 0;
	//��ͷ
	if((i + HJ212_BYTES_FRAME_BAOTOU) > recv_data_p->data_len)
	{
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]���ݳ��ȴ���-��ͷ,[%d]", recv_data_p->data_len));
		return RT_FALSE;
	}
	if(memcmp((void *)HJ212_FRAME_BAOTOU, (void *)(recv_data_p->pdata + i), HJ212_BYTES_FRAME_BAOTOU))
	{
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]֡��ʽ����-��ͷ"));
		return RT_FALSE;
	}
	i += HJ212_BYTES_FRAME_BAOTOU;
	//���ݶγ���
	if((i + HJ212_BYTES_FRAME_LEN) > recv_data_p->data_len)
	{
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]���ݳ��ȴ���-���ݶγ���,[%d]", recv_data_p->data_len));
		return RT_FALSE;
	}
	if(RT_FALSE == fyyp_is_number(recv_data_p->pdata + i, HJ212_BYTES_FRAME_LEN))
	{
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]֡��ʽ����-���ݶγ����ַ�����"));
		return RT_FALSE;
	}
	data_len = fyyp_str_to_hex(recv_data_p->pdata + i, HJ212_BYTES_FRAME_LEN);
	i += HJ212_BYTES_FRAME_LEN;
	//���ݶ�
	if((i + data_len + HJ212_BYTES_FRAME_CRC + HJ212_BYTES_FRAME_BAOWEI) != recv_data_p->data_len)
	{
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]֡��ʽ����-���ݳ���"));
		return RT_FALSE;
	}
	i += data_len;
	//У����
	fyyp_str_to_array((rt_uint8_t *)&crc_val, recv_data_p->pdata + i, HJ212_BYTES_FRAME_CRC);
	rtn = (rt_uint8_t)(crc_val >> 8);
	crc_val <<= 8;
	crc_val += rtn;
	cn = _hj212_crc_value(recv_data_p->pdata + HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN, data_len);
	if((crc_val == cn) || (crc_val == HJ212_DEF_CRC_VAL))
	{
		rtn = HJ212_QNRTN_EXCUTE;
	}
	else
	{
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]֡��ʽ����-У����,[%04X] != [%04X]", crc_val, cn));
		rtn = HJ212_QNRTN_ERR_CRC;
	}
	i += HJ212_BYTES_FRAME_CRC;
	//��β
	if(memcmp((void *)HJ212_FRAME_BAOWEI, (void *)(recv_data_p->pdata + i), HJ212_BYTES_FRAME_BAOWEI))
	{
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]֡��ʽ����-��β"));
		return RT_FALSE;
	}
	//���յ�������
	DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����֡:"));
	DEBUG_INFO_OUTPUT(DEBUG_INFO_TYPE_HJ212, (recv_data_p->pdata, recv_data_p->data_len));
	//У�����Ƿ����
	if(HJ212_QNRTN_ERR_CRC == rtn)
	{
		goto __qnrtn;
	}
	//���ݶη���
	i = HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN;
	//QN
	crc_val = _hj212_frm_is_qn(recv_data_p->pdata + i, data_len);
	if(!crc_val)
	{
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]֡��ʽ����-QN"));
		rtn = HJ212_QNRTN_ERR_QN;
		goto __qnrtn;
	}
	i += crc_val;
	data_len -= crc_val;
	qn_en = RT_TRUE;
	//ST
	crc_val = _hj212_frm_is_st(recv_data_p->pdata + i, data_len);
	if(!crc_val)
	{
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]֡��ʽ����-ST"));
		rtn = HJ212_QNRTN_ERR_ST;
		goto __qnrtn;
	}
	i += crc_val;
	data_len -= crc_val;
	//CN
	crc_val = _hj212_frm_is_cn(recv_data_p->pdata + i, data_len, &cn);
	if(!crc_val)
	{
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]֡��ʽ����-CN"));
		rtn = HJ212_QNRTN_ERR_CN;
		goto __qnrtn;
	}
	i += crc_val;
	data_len -= crc_val;
	//PW
	crc_val = _hj212_frm_is_pw(recv_data_p->pdata + i, data_len);
	if(!crc_val)
	{
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]֡��ʽ����-PW"));
		rtn = HJ212_QNRTN_ERR_PW;
		goto __qnrtn;
	}
	i += crc_val;
	data_len -= crc_val;
	//MN
	crc_val = _hj212_frm_is_mn(recv_data_p->pdata + i, data_len);
	if(!crc_val)
	{
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]֡��ʽ����-MN"));
		rtn = HJ212_QNRTN_ERR_MN;
		goto __qnrtn;
	}
	i += crc_val;
	data_len -= crc_val;
	//Flag
	crc_val = _hj212_frm_is_flag(recv_data_p->pdata + i, data_len, &flag);
	if(!crc_val)
	{
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]֡��ʽ����-Flag"));
		rtn = HJ212_QNRTN_ERR_FLAG;
		goto __qnrtn;
	}
	i += crc_val;
	data_len -= crc_val;
	if(flag & HJ212_FLAG_PACKET_DIV_EN)
	{
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����֡�ְ���֧��"));
		rtn = HJ212_QNRTN_ERR_UNKNOWN;
		goto __qnrtn;
	}
	//CP
	crc_val = _hj212_frm_is_cp(recv_data_p->pdata + i, data_len);
	if(!crc_val)
	{
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]֡��ʽ����-CP"));
		rtn = HJ212_QNRTN_ERR_UNKNOWN;
		goto __qnrtn;
	}
	i += crc_val;
	data_len -= (crc_val + strlen(HJ212_FRAME_CP_END));
	//�Ƿ�����Ӧ��
	if((0 == (flag & HJ212_FLAG_ACK_EN)) || (HJ212_CN_GET_INFORM_RTN == cn) || (HJ212_CN_STOP_RTD_DATA == cn) || (HJ212_CN_STOP_RS_DATA == cn) || (HJ212_CN_GET_DATA_RTN == cn))
	{
		goto __excute;
	}

__qnrtn:
	//����Ӧ���
	report_data_p->data_len = _hj212_frm_data_head(report_data_p->pdata, PTCL_BYTES_ACK_REPORT, (RT_FALSE == qn_en) ? (rt_uint8_t *)0 : (recv_data_p->pdata + HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN), RT_FALSE, HJ212_CN_GET_QNRTN, HJ212_FLAG_PTCL_VER, 0, 0);
	if(!report_data_p->data_len)
	{
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����Ӧ������ʧ��-data_head"));
		goto __exit;
	}
	//QNRTN
	crc_val = _hj212_frm_qnrtn(PTCL_BYTES_ACK_REPORT - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, rtn);
	if(crc_val)
	{
		report_data_p->data_len += crc_val;
	}
	else
	{
		report_data_p->data_len = 0;
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����Ӧ������ʧ��-qnrtn"));
		goto __exit;
	}
	//CP_END
	crc_val = _hj212_frm_cp_end(PTCL_BYTES_ACK_REPORT - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len);
	if(crc_val)
	{
		report_data_p->data_len += crc_val;
	}
	else
	{
		report_data_p->data_len = 0;
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����Ӧ������ʧ��-cp_end"));
		goto __exit;
	}
	//�Ƿ�ִ������
	if(HJ212_QNRTN_EXCUTE != rtn)
	{
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]ֻ��Ҫ�ظ�����Ӧ���-[%d]", rtn));
		goto __exit;
	}
	//��β����
	_hj212_frm_len(report_data_p->pdata, report_data_p->data_len);
	crc_val = _hj212_crc_value(report_data_p->pdata + HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN, report_data_p->data_len - HJ212_BYTES_FRAME_BAOTOU - HJ212_BYTES_FRAME_LEN);
	crc_val = _hj212_frm_baowei(PTCL_BYTES_ACK_REPORT - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, crc_val);
	if(crc_val)
	{
		report_data_p->data_len += crc_val;

		report_data_p->data_id			= 0;
		report_data_p->fun_csq_update	= (void *)0;
		report_data_p->need_reply		= RT_FALSE;
		report_data_p->fcb_value		= 0;
		report_data_p->ptcl_type		= PTCL_PTCL_TYPE_HJ212;
	}
	else
	{
		report_data_p->data_len = 0;
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����Ӧ������ʧ��-��β"));
		goto __exit;
	}
	//����Ӧ�������
	DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����Ӧ���:"));
	DEBUG_INFO_OUTPUT(DEBUG_INFO_TYPE_HJ212, (report_data_p->pdata, report_data_p->data_len));
	//��������Ӧ��
	ptcl_report_data_send(report_data_p, recv_data_p->comm_type, recv_data_p->ch, 0, 0, (rt_uint16_t *)0);

__excute:
	//�ظ�׼��
	report_data_p->data_len = 0;
	//ִ������
	switch(cn)
	{
	default:
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d][δ֪����]", cn));
		rtn = HJ212_EXERTN_ERR_CMD;
		break;
	//���ó�ʱʱ�估�ط�����
	case HJ212_CN_SET_OT_RECOUNT:
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d][���ó�ʱʱ�估�ط�����]", cn));
		//over_time
		param_info_p = ptcl_param_info_req(sizeof(rt_uint16_t), RT_WAITING_NO);
		if((PTCL_PARAM_INFO *)0 == param_info_p)
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]����ռ�ʧ��[over_time]", cn));
			rtn = HJ212_EXERTN_ERR_SYSTEM;
			goto __cn_set_ot_recount;
		}
		crc_val = _hj212_frm_is_ot(recv_data_p->pdata + i, data_len, (rt_uint16_t *)param_info_p->pdata);
		if(crc_val)
		{
			i += crc_val;
			data_len -= crc_val;
			param_info_p->param_type = PTCL_PARAM_INFO_HJ212_OVER_TIME;
			ptcl_param_info_add(param_info_pp, param_info_p);
		}
		else
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]֡��ʽ����[over_time]", cn));
			rt_mp_free((void *)param_info_p);
			rtn = HJ212_EXERTN_ERR_NO_DATA;
			goto __cn_set_ot_recount;
		}
		//recount
		param_info_p = ptcl_param_info_req(sizeof(rt_uint8_t), RT_WAITING_NO);
		if((PTCL_PARAM_INFO *)0 == param_info_p)
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]����ռ�ʧ��[recount]", cn));
			rtn = HJ212_EXERTN_ERR_SYSTEM;
			goto __cn_set_ot_recount;
		}
		crc_val = _hj212_frm_is_recount(recv_data_p->pdata + i, data_len, param_info_p->pdata);
		if(crc_val)
		{
			i += crc_val;
			data_len -= crc_val;
			param_info_p->param_type = PTCL_PARAM_INFO_HJ212_RECOUNT;
			ptcl_param_info_add(param_info_pp, param_info_p);
		}
		else
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]֡��ʽ����[recount]", cn));
			rt_mp_free((void *)param_info_p);
			rtn = HJ212_EXERTN_ERR_NO_DATA;
			goto __cn_set_ot_recount;
		}
		rtn = HJ212_EXERTN_OK;
__cn_set_ot_recount:
		break;
	//��ȡ�ֳ���ʱ��
	case HJ212_CN_GET_SYS_TIME:
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d][��ȡ�ֳ���ʱ��]", cn));
		if(data_len)
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]ֻ������ȡ�����ǵ�ʱ��", cn));
			rtn = HJ212_EXERTN_ERR_CMD;
			goto __cn_get_sys_time;
		}
		//�ظ�
		report_data_p->data_len = _hj212_frm_data_head(report_data_p->pdata, PTCL_BYTES_ACK_REPORT, (RT_FALSE == qn_en) ? (rt_uint8_t *)0 : (recv_data_p->pdata + HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN), RT_TRUE, cn, HJ212_FLAG_PTCL_VER, 0, 0);
		if(!report_data_p->data_len)
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]�ظ������ʧ��-data_head"));
			rtn = HJ212_EXERTN_ERR_SYSTEM;
			goto __cn_get_sys_time;
		}
		crc_val = _hj212_frm_sys_time(PTCL_BYTES_ACK_REPORT - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len);
		if(crc_val)
		{
			report_data_p->data_len += crc_val;
		}
		else
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]�ظ������ʧ��-sys_time"));
			rtn = HJ212_EXERTN_ERR_SYSTEM;
			goto __cn_get_sys_time;
		}
		rtn = HJ212_EXERTN_OK;
__cn_get_sys_time:
		break;
	//�����ֳ���ʱ��
	case HJ212_CN_SET_SYS_TIME:
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d][�����ֳ���ʱ��]", cn));
		//sys_time
		param_info_p = ptcl_param_info_req(sizeof(struct tm), RT_WAITING_NO);
		if((PTCL_PARAM_INFO *)0 == param_info_p)
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]����ռ�ʧ��[sys_time]", cn));
			rtn = HJ212_EXERTN_ERR_SYSTEM;
			goto __cn_set_sys_time;
		}
		crc_val = _hj212_frm_is_sys_time(recv_data_p->pdata + i, data_len, (struct tm *)param_info_p->pdata);
		if(crc_val)
		{
			i += crc_val;
			data_len -= crc_val;
			param_info_p->param_type = PTCL_PARAM_INFO_TT;
			ptcl_param_info_add(param_info_pp, param_info_p);
		}
		else
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]֡��ʽ����[sys_time]", cn));
			rt_mp_free((void *)param_info_p);
			rtn = HJ212_EXERTN_ERR_NO_DATA;
			goto __cn_set_sys_time;
		}
		rtn = HJ212_EXERTN_OK;
__cn_set_sys_time:
		break;
	//֪ͨӦ��
	case HJ212_CN_GET_INFORM_RTN:
	//����Ӧ��
	case HJ212_CN_GET_DATA_RTN:
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d][Ӧ��]", cn));
		if(data_len)
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]֡��ʽ����[��������Ϊ��]", cn));
			goto __exit;
		}
		crc_val = _hj212_data_id(recv_data_p->pdata, recv_data_p->data_len);
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]Ӧ��id[%d]", cn, crc_val));
		ptcl_reply_post(recv_data_p->comm_type, recv_data_p->ch, crc_val, 0);
		
		goto __exit;
	//��ȡʵʱ���ݼ��
	case HJ212_CN_GET_RTD_INTERVAL:
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d][��ȡʵʱ���ݼ��]", cn));
		if(data_len)
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]��������Ϊ��", cn));
			rtn = HJ212_EXERTN_ERR_CMD;
			goto __cn_get_rtd_interval;
		}
		//�ظ�
		report_data_p->data_len = _hj212_frm_data_head(report_data_p->pdata, PTCL_BYTES_ACK_REPORT, (RT_FALSE == qn_en) ? (rt_uint8_t *)0 : (recv_data_p->pdata + HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN), RT_TRUE, cn, HJ212_FLAG_PTCL_VER, 0, 0);
		if(!report_data_p->data_len)
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]�ظ������ʧ��-data_head"));
			rtn = HJ212_EXERTN_ERR_SYSTEM;
			goto __cn_get_rtd_interval;
		}
		crc_val = _hj212_frm_rtd_interval(PTCL_BYTES_ACK_REPORT - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len);
		if(crc_val)
		{
			report_data_p->data_len += crc_val;
		}
		else
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]�ظ������ʧ��-rtd_interval"));
			rtn = HJ212_EXERTN_ERR_SYSTEM;
			goto __cn_get_rtd_interval;
		}
		rtn = HJ212_EXERTN_OK;
__cn_get_rtd_interval:
		break;
	//����ʵʱ���ݼ��
	case HJ212_CN_SET_RTD_INTERVAL:
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d][����ʵʱ���ݼ��]", cn));
		//rtd_interval
		param_info_p = ptcl_param_info_req(sizeof(rt_uint16_t), RT_WAITING_NO);
		if((PTCL_PARAM_INFO *)0 == param_info_p)
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]����ռ�ʧ��[rtd_interval]", cn));
			rtn = HJ212_EXERTN_ERR_SYSTEM;
			goto __cn_set_rtd_interval;
		}
		crc_val = _hj212_frm_is_rtd_interval(recv_data_p->pdata + i, data_len, (rt_uint16_t *)param_info_p->pdata);
		if(crc_val)
		{
			i += crc_val;
			data_len -= crc_val;
			param_info_p->param_type = PTCL_PARAM_INFO_HJ212_RTD_INTERVAL;
			ptcl_param_info_add(param_info_pp, param_info_p);
		}
		else
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]֡��ʽ����[rtd_interval]", cn));
			rt_mp_free((void *)param_info_p);
			rtn = HJ212_EXERTN_ERR_NO_DATA;
			goto __cn_set_rtd_interval;
		}
		rtn = HJ212_EXERTN_OK;
__cn_set_rtd_interval:
		break;
	//��ȡ�������ݼ��
	case HJ212_CN_GET_MIN_INTERVAL:
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d][��ȡ�������ݼ��]", cn));
		if(data_len)
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]��������Ϊ��", cn));
			rtn = HJ212_EXERTN_ERR_CMD;
			goto __cn_get_min_interval;
		}
		//�ظ�
		report_data_p->data_len = _hj212_frm_data_head(report_data_p->pdata, PTCL_BYTES_ACK_REPORT, (RT_FALSE == qn_en) ? (rt_uint8_t *)0 : (recv_data_p->pdata + HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN), RT_TRUE, cn, HJ212_FLAG_PTCL_VER, 0, 0);
		if(!report_data_p->data_len)
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]�ظ������ʧ��-data_head"));
			rtn = HJ212_EXERTN_ERR_SYSTEM;
			goto __cn_get_min_interval;
		}
		crc_val = _hj212_frm_min_interval(PTCL_BYTES_ACK_REPORT - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len);
		if(crc_val)
		{
			report_data_p->data_len += crc_val;
		}
		else
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]�ظ������ʧ��-min_interval"));
			rtn = HJ212_EXERTN_ERR_SYSTEM;
			goto __cn_get_min_interval;
		}
		rtn = HJ212_EXERTN_OK;
__cn_get_min_interval:
		break;
	//���÷������ݼ��
	case HJ212_CN_SET_MIN_INTERVAL:
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d][���÷������ݼ��]", cn));
		//min_interval
		param_info_p = ptcl_param_info_req(sizeof(rt_uint16_t), RT_WAITING_NO);
		if((PTCL_PARAM_INFO *)0 == param_info_p)
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]����ռ�ʧ��[min_interval]", cn));
			rtn = HJ212_EXERTN_ERR_SYSTEM;
			goto __cn_set_min_interval;
		}
		crc_val = _hj212_frm_is_min_interval(recv_data_p->pdata + i, data_len, (rt_uint16_t *)param_info_p->pdata);
		if(crc_val)
		{
			i += crc_val;
			data_len -= crc_val;
			param_info_p->param_type = PTCL_PARAM_INFO_HJ212_MIN_INTERVAL;
			ptcl_param_info_add(param_info_pp, param_info_p);
		}
		else
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]֡��ʽ����[min_interval]", cn));
			rt_mp_free((void *)param_info_p);
			rtn = HJ212_EXERTN_ERR_NO_DATA;
			goto __cn_set_min_interval;
		}
		rtn = HJ212_EXERTN_OK;
__cn_set_min_interval:
		break;
	//�����ֳ�����������
	case HJ212_CN_SET_NEW_PW:
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d][�����ֳ�����������]", cn));
		//new_pw
		param_info_p = ptcl_param_info_req(HJ212_BYTES_PW, RT_WAITING_NO);
		if((PTCL_PARAM_INFO *)0 == param_info_p)
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]����ռ�ʧ��[pw]", cn));
			rtn = HJ212_EXERTN_ERR_SYSTEM;
			goto __cn_set_new_pw;
		}
		crc_val = _hj212_frm_is_new_pw(recv_data_p->pdata + i, data_len, param_info_p->pdata);
		if(crc_val)
		{
			i += crc_val;
			data_len -= crc_val;
			param_info_p->param_type = PTCL_PARAM_INFO_HJ212_PW;
			ptcl_param_info_add(param_info_pp, param_info_p);
		}
		else
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]֡��ʽ����[new_pw]", cn));
			rt_mp_free((void *)param_info_p);
			rtn = HJ212_EXERTN_ERR_NO_DATA;
			goto __cn_set_new_pw;
		}
		rtn = HJ212_EXERTN_OK;
__cn_set_new_pw:
		break;
	//ȡ��Ⱦ��ʵʱ����
	case HJ212_CN_RTD_DATA:
	//ȡ�豸����״̬����
	case HJ212_CN_RS_DATA:
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d][�����ϱ���Ⱦ��ʵʱ���ݻ򹤿�����]", cn));
		if(data_len)
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]��������Ϊ��", cn));
			rtn = HJ212_EXERTN_ERR_CMD;
			goto __cn_set_rtd_rs_en;
		}
		//ִ��
		param_info_p = ptcl_param_info_req(sizeof(rt_uint8_t), RT_WAITING_NO);
		if((PTCL_PARAM_INFO *)0 == param_info_p)
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]����ռ�ʧ��[rtd_rs_en]", cn));
			rtn = HJ212_EXERTN_ERR_SYSTEM;
			goto __cn_set_rtd_rs_en;
		}
		param_info_p->param_type	= (HJ212_CN_RTD_DATA == cn) ? PTCL_PARAM_INFO_HJ212_RTD_EN : PTCL_PARAM_INFO_HJ212_RS_EN;
		*param_info_p->pdata		= RT_TRUE;
		ptcl_param_info_add(param_info_pp, param_info_p);
		rtn = HJ212_EXERTN_OK;
__cn_set_rtd_rs_en:
		break;
	//ֹͣ�鿴��Ⱦ��ʵʱ����
	case HJ212_CN_STOP_RTD_DATA:
	//ֹͣ�鿴�豸����״̬
	case HJ212_CN_STOP_RS_DATA:
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d][ֹͣ�ϱ���Ⱦ��ʵʱ���ݻ��豸����״̬����]", cn));
		if(data_len)
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]��������Ϊ��", cn));
			rtn = HJ212_EXERTN_ERR_CMD;
			goto __cn_set_rtd_rs_dis;
		}
		//ִ��
		param_info_p = ptcl_param_info_req(sizeof(rt_uint8_t), RT_WAITING_NO);
		if((PTCL_PARAM_INFO *)0 == param_info_p)
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]����ռ�ʧ��[rtd_rs_en]", cn));
			rtn = HJ212_EXERTN_ERR_SYSTEM;
			goto __cn_set_rtd_rs_dis;
		}
		param_info_p->param_type	= (HJ212_CN_STOP_RTD_DATA == cn) ? PTCL_PARAM_INFO_HJ212_RTD_EN : PTCL_PARAM_INFO_HJ212_RS_EN;
		*param_info_p->pdata		= RT_FALSE;
		ptcl_param_info_add(param_info_pp, param_info_p);
		//�ظ�
		report_data_p->data_len = _hj212_frm_data_head(report_data_p->pdata, PTCL_BYTES_ACK_REPORT, (RT_FALSE == qn_en) ? (rt_uint8_t *)0 : (recv_data_p->pdata + HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN), RT_FALSE, HJ212_CN_GET_INFORM_RTN, HJ212_FLAG_PTCL_VER, 0, 0);
		if(!report_data_p->data_len)
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]�ظ������ʧ��-data_head"));
			rtn = HJ212_EXERTN_ERR_SYSTEM;
			goto __cn_set_rtd_rs_dis;
		}
		rtn = HJ212_EXERTN_OK;
__cn_set_rtd_rs_dis:
		break;
	//ȡ������ʷ����
	case HJ212_CN_MIN_DATA:
	//ȡСʱ��ʷ����
	case HJ212_CN_HOUR_DATA:
	//ȡ����ʷ����
	case HJ212_CN_DAY_DATA:
	//ȡ��ʱ����ʷ����
	case HJ212_CN_DAY_RS_DATA:
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d][��ѯ��ʷ����]", cn));
		//begin_time
		crc_val = _hj212_frm_is_begin_time(recv_data_p->pdata + i, data_len, &unix_low);
		if(crc_val)
		{
			i += crc_val;
			data_len -= crc_val;
		}
		else
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]֡��ʽ����[begin_time]", cn));
			rtn = HJ212_EXERTN_ERR_NO_DATA;
			goto __cn_get_old_data;
		}
		//end_time
		crc_val = _hj212_frm_is_end_time(recv_data_p->pdata + i, data_len, &unix_high);
		if(crc_val)
		{
			i += crc_val;
			data_len -= crc_val;
		}
		else
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]֡��ʽ����[end_time]", cn));
			rtn = HJ212_EXERTN_ERR_NO_DATA;
			goto __cn_get_old_data;
		}
		//ʱ���ж�
		if(unix_low > unix_high)
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]���ݴ���[ʱ�����]", cn));
			rtn = HJ212_EXERTN_ERR_CMD;
			goto __cn_get_old_data;
		}
		//���벢��ʼ����������
		pmb = (rt_mailbox_t)mempool_req(sizeof(struct rt_mailbox), RT_WAITING_NO);
		if((rt_mailbox_t)0 == pmb)
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]��ѯʧ��[������������ʧ��]", cn));
			rtn = HJ212_EXERTN_ERR_SYSTEM;
			goto __cn_get_old_data;
		}
		if(RT_EOK != rt_mb_init(pmb, "hj212", (void *)&msg_pool, 1, RT_IPC_FLAG_PRIO))
		{
			while(1);
		}
		//����ʷ����
		if(RT_FALSE == sample_query_data_ex(cn, unix_low, unix_high, pmb))
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����[%d]��ѯʧ��[����ʧ��]", cn));
			rt_mp_free((void *)pmb);
			rtn = HJ212_EXERTN_ERR_SYSTEM;
			goto __cn_get_old_data;
		}
		//��������
		while(1)
		{
			if(RT_EOK != rt_mb_recv(pmb, (rt_ubase_t *)&param_info_p, RT_WAITING_FOREVER))
			{
				while(1);
			}

			if(0 == (rt_ubase_t)param_info_p)
			{
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]��ʷ���ݲ�ѯ����"));
				break;
			}

			//��ͷ
			report_data_p->data_len = _hj212_frm_data_head(report_data_p->pdata, PTCL_BYTES_ACK_REPORT, (rt_uint8_t *)0, RT_TRUE, cn, HJ212_FLAG_PTCL_VER, 0, 0);
			if(!report_data_p->data_len)
			{
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]��ʷ�����ʧ��-��ͷ"));
				goto __next_old_data;
			}
			//������
			crc_val = *(rt_uint16_t *)param_info_p;
			if((report_data_p->data_len + crc_val) > PTCL_BYTES_ACK_REPORT)
			{
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]��ʷ�����ʧ��-������[%d]", crc_val));
				goto __next_old_data;
			}
			memcpy((void *)(report_data_p->pdata + report_data_p->data_len), (void *)((rt_uint8_t *)param_info_p + sizeof(rt_uint16_t)), crc_val);
			report_data_p->data_len += crc_val;
			//CP_END
			crc_val = _hj212_frm_cp_end(PTCL_BYTES_ACK_REPORT - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len);
			if(crc_val)
			{
				report_data_p->data_len += crc_val;
			}
			else
			{
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]��ʷ�����ʧ��-cp_end"));
				goto __next_old_data;
			}
			//��β����
			_hj212_frm_len(report_data_p->pdata, report_data_p->data_len);
			crc_val = _hj212_crc_value(report_data_p->pdata + HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN, report_data_p->data_len - HJ212_BYTES_FRAME_BAOTOU - HJ212_BYTES_FRAME_LEN);
			crc_val = _hj212_frm_baowei(PTCL_BYTES_ACK_REPORT - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, crc_val);
			if(crc_val)
			{
				report_data_p->data_len += crc_val;

				report_data_p->data_id			= 0;
				report_data_p->fun_csq_update	= (void *)0;
				report_data_p->need_reply		= RT_FALSE;
				report_data_p->fcb_value		= 0;
				report_data_p->ptcl_type		= PTCL_PTCL_TYPE_HJ212;
			}
			else
			{
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]��ʷ�����ʧ��-��β"));
				goto __next_old_data;
			}
			//��ʷ������
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]��ʷ��:"));
			DEBUG_INFO_OUTPUT(DEBUG_INFO_TYPE_HJ212, (report_data_p->pdata, report_data_p->data_len));
			//������ʷ��
			ptcl_report_data_send(report_data_p, recv_data_p->comm_type, recv_data_p->ch, 0, 0, (rt_uint16_t *)0);
			
__next_old_data:
			rt_mp_free((void *)param_info_p);
		}
		rt_mp_free((void *)pmb);
		report_data_p->data_len = 0;
		rtn = HJ212_EXERTN_OK;
__cn_get_old_data:
		break;
	}

	//�ظ�
	if((HJ212_EXERTN_OK == rtn) && report_data_p->data_len)
	{
		//CP_END
		crc_val = _hj212_frm_cp_end(PTCL_BYTES_ACK_REPORT - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len);
		if(crc_val)
		{
			report_data_p->data_len += crc_val;
		}
		else
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]�ظ������ʧ��-cp_end"));
			rtn = HJ212_EXERTN_ERR_SYSTEM;
			goto __exertn;
		}
		//��β����
		_hj212_frm_len(report_data_p->pdata, report_data_p->data_len);
		crc_val = _hj212_crc_value(report_data_p->pdata + HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN, report_data_p->data_len - HJ212_BYTES_FRAME_BAOTOU - HJ212_BYTES_FRAME_LEN);
		crc_val = _hj212_frm_baowei(PTCL_BYTES_ACK_REPORT - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, crc_val);
		if(crc_val)
		{
			report_data_p->data_len += crc_val;

			report_data_p->data_id			= 0;
			report_data_p->fun_csq_update	= (void *)0;
			report_data_p->need_reply		= RT_FALSE;
			report_data_p->fcb_value		= 0;
			report_data_p->ptcl_type		= PTCL_PTCL_TYPE_HJ212;
		}
		else
		{
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]�ظ������ʧ��-��β"));
			rtn = HJ212_EXERTN_ERR_SYSTEM;
			goto __exertn;
		}
		//�ظ�������
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]�ظ���:"));
		DEBUG_INFO_OUTPUT(DEBUG_INFO_TYPE_HJ212, (report_data_p->pdata, report_data_p->data_len));
		//���ͻظ���
		ptcl_report_data_send(report_data_p, recv_data_p->comm_type, recv_data_p->ch, 0, 0, (rt_uint16_t *)0);
		//�������Ҫ�ظ�֪ͨ�����cn
		if((HJ212_CN_STOP_RTD_DATA == cn) || (HJ212_CN_STOP_RS_DATA == cn))
		{
			report_data_p->data_len = 0;
			goto __exit;
		}
	}

__exertn:
	//ִ��Ӧ���
	report_data_p->data_len = _hj212_frm_data_head(report_data_p->pdata, PTCL_BYTES_ACK_REPORT, (RT_FALSE == qn_en) ? (rt_uint8_t *)0 : (recv_data_p->pdata + HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN), RT_FALSE, HJ212_CN_GET_EXERTN, HJ212_FLAG_PTCL_VER, 0, 0);
	if(!report_data_p->data_len)
	{
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]ִ��Ӧ������ʧ��-data_head"));
		goto __exit;
	}
	//EXERTN
	crc_val = _hj212_frm_exertn(PTCL_BYTES_ACK_REPORT - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, rtn);
	if(crc_val)
	{
		report_data_p->data_len += crc_val;
	}
	else
	{
		report_data_p->data_len = 0;
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]ִ��Ӧ������ʧ��-exertn"));
		goto __exit;
	}
	//CP_END
	crc_val = _hj212_frm_cp_end(PTCL_BYTES_ACK_REPORT - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len);
	if(crc_val)
	{
		report_data_p->data_len += crc_val;
	}
	else
	{
		report_data_p->data_len = 0;
		DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]ִ��Ӧ������ʧ��-cp_end"));
		goto __exit;
	}

__exit:
	//��β����
	if(report_data_p->data_len)
	{
		//����
		_hj212_frm_len(report_data_p->pdata, report_data_p->data_len);
		//��β
		crc_val = _hj212_crc_value(report_data_p->pdata + HJ212_BYTES_FRAME_BAOTOU + HJ212_BYTES_FRAME_LEN, report_data_p->data_len - HJ212_BYTES_FRAME_BAOTOU - HJ212_BYTES_FRAME_LEN);
		crc_val = _hj212_frm_baowei(PTCL_BYTES_ACK_REPORT - report_data_p->data_len, report_data_p->pdata + report_data_p->data_len, crc_val);
		if(crc_val)
		{
			report_data_p->data_len += crc_val;

			report_data_p->data_id			= 0;
			report_data_p->fun_csq_update	= (void *)0;
			report_data_p->need_reply		= RT_FALSE;
			report_data_p->fcb_value		= 0;
			report_data_p->ptcl_type		= PTCL_PTCL_TYPE_HJ212;
			
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HJ212, ("\r\n[hj212]����֡:"));
			DEBUG_INFO_OUTPUT(DEBUG_INFO_TYPE_HJ212, (report_data_p->pdata, report_data_p->data_len));
		}
		else
		{
			report_data_p->data_len = 0;
		}
	}

	return RT_TRUE;
}

