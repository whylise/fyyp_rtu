#include "http.h"
#include "drv_rtcee.h"
#include "drv_w25qxxx.h"
#include "drv_lpm.h"
#include "drv_fyyp.h"
#include "dtu.h"
#include "drv_mempool.h"
#include "drv_debug.h"
#include "stdio.h"
#include "string.h"
#include "stm32f4xx.h"



static rt_ubase_t			m_http_msgpool_url_info;
static struct rt_mailbox	m_http_mb_url_info;

static struct rt_thread		m_http_thread_get_req;
static rt_uint8_t			m_http_stk_get_req[HTTP_STK_GET_REQ];



static void _http_firmware_erase(void)
{
	rt_uint32_t addr = 0;

	while(addr < HTTP_BYTES_FIRMWARE_MAX)
	{
		w25qxxx_block_erase_ex(addr + HTTP_FIRMWARE_STORE_ADDR);
		addr += W25QXXX_BYTES_PER_BLOCK;
	}
}

static HTTP_URL_INFO *_http_url_info_req(rt_uint16_t bytes_req, rt_int32_t ticks)
{
	HTTP_URL_INFO *url_info_ptr;

	if(0 == bytes_req)
	{
		return (HTTP_URL_INFO *)0;
	}
	
	bytes_req += sizeof(HTTP_URL_INFO);
	url_info_ptr = (HTTP_URL_INFO *)mempool_req(bytes_req, ticks);
	if((HTTP_URL_INFO *)0 != url_info_ptr)
	{
		url_info_ptr->purl = (rt_uint8_t *)(url_info_ptr + 1);
	}
	
	return url_info_ptr;
}

/*url����
"http://www.rt-thread.com/service/rt-thread.txt"
*/
static void _http_task_get_req(void *parg)
{
	rt_uint8_t		*ptr;
	HTTP_URL_INFO	*url_info_ptr;
	DTU_RECV_DATA	*resp_data_ptr;
	rt_uint32_t		start_addr, file_len, len, host_len;
	DTU_IP_ADDR		ip_addr;
	rt_uint8_t (*file_data_write)(rt_uint32_t addr, rt_uint8_t const *pdata, rt_uint16_t data_len);
	
	while(1)
	{
		if(RT_EOK != rt_mb_recv(&m_http_mb_url_info, (rt_ubase_t *)&url_info_ptr, RT_WAITING_FOREVER))
		{
			while(1);
		}

		lpm_cpu_ref(RT_TRUE);

		if((HTTP_URL_INFO *)0 != url_info_ptr)
		{
			if(HTTP_FILE_TYPE_FIRMWARE == url_info_ptr->file_type)
			{
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]�̼�url:"));
				DEBUG_INFO_OUTPUT(DEBUG_INFO_TYPE_HTTP, (url_info_ptr->purl, url_info_ptr->url_len));
				
				start_addr = HTTP_FIRMWARE_STORE_ADDR + HTTP_BYTES_FIRMWARE_INFO;
				file_data_write = w25qxxx_program_ex;
				w25qxxx_open_ex();
			}
			else if(HTTP_FILE_TYPE_PARAM == url_info_ptr->file_type)
			{
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]����url:"));
				DEBUG_INFO_OUTPUT(DEBUG_INFO_TYPE_HTTP, (url_info_ptr->purl, url_info_ptr->url_len));
				
				start_addr = HTTP_PARAM_STORE_ADDR;
				file_data_write = rtcee_eeprom_write;
			}
			else
			{
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]δ֪url:"));
				DEBUG_INFO_OUTPUT(DEBUG_INFO_TYPE_HTTP, (url_info_ptr->purl, url_info_ptr->url_len));
				
				goto __exit;
			}
			//URLͷ(http://)
			len = strlen(HTTP_URL_HEAD);
			if(url_info_ptr->url_len <= len)
			{
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]url���ȹ���"));
				goto __exit;
			}
			if(memcmp((void *)url_info_ptr->purl, (void *)HTTP_URL_HEAD, len))
			{
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]urlЭ���ʶ����"));
				goto __exit;
			}
			url_info_ptr->purl		+= len;
			url_info_ptr->url_len	-= len;
			//��������ַ�Ľ�β(/)
			ptr = fyyp_mem_find(url_info_ptr->purl, url_info_ptr->url_len, "/", 1);
			if((rt_uint8_t *)0 == ptr)
			{
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]url��ʽ����"));
				goto __exit;
			}
			ip_addr.addr_len = ptr - url_info_ptr->purl;
			if((0 == ip_addr.addr_len) || (ip_addr.addr_len > DTU_BYTES_IP_ADDR))
			{
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]��������ַ���ȴ���"));
				goto __exit;
			}
			memcpy((void *)ip_addr.addr_data, (void *)url_info_ptr->purl, ip_addr.addr_len);
			url_info_ptr->purl		+= ip_addr.addr_len;
			url_info_ptr->url_len	-= ip_addr.addr_len;
			//�жϷ�������ַ���Ƿ�����˿ںţ����û�������Ĭ��80�˿�
			ptr = fyyp_mem_find(ip_addr.addr_data, ip_addr.addr_len, ":", 1);
			if((rt_uint8_t *)0 == ptr)
			{
				host_len = ip_addr.addr_len;
				len = snprintf((char *)(ip_addr.addr_data + ip_addr.addr_len), DTU_BYTES_IP_ADDR - ip_addr.addr_len, HTTP_DEF_SERVER_PORT);
				if(len >= (DTU_BYTES_IP_ADDR - ip_addr.addr_len))
				{
					DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]��������ַ���ȴ���"));
					goto __exit;
				}
				ip_addr.addr_len += len;
			}
			else
			{
				host_len = ptr - ip_addr.addr_data;
			}

			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]������:"));
			DEBUG_INFO_OUTPUT(DEBUG_INFO_TYPE_HTTP, (ip_addr.addr_data, ip_addr.addr_len));
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]�ļ�Ŀ¼:"));
			DEBUG_INFO_OUTPUT(DEBUG_INFO_TYPE_HTTP, (url_info_ptr->purl, url_info_ptr->url_len));

			//��GET�������ݰ�
			ptr = (rt_uint8_t *)mempool_req(HTTP_BYTES_GET_REQ_DATA, RT_WAITING_FOREVER);
			if((rt_uint8_t *)0 == ptr)
			{
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET������������ʧ��"));
				goto __exit;
			}
			//GET_
			len = snprintf((char *)ptr, HTTP_BYTES_GET_REQ_DATA, "GET ");
			if(len >= HTTP_BYTES_GET_REQ_DATA)
			{
				rt_mp_free((void *)ptr);
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET�������ݿռ䲻��"));
				goto __exit;
			}
			//�ļ�Ŀ¼
			if((len + url_info_ptr->url_len) > HTTP_BYTES_GET_REQ_DATA)
			{
				rt_mp_free((void *)ptr);
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET�������ݿռ䲻��"));
				goto __exit;
			}
			memcpy((void *)(ptr + len), (void *)url_info_ptr->purl, url_info_ptr->url_len);
			len += url_info_ptr->url_len;
			//_HTTP/1.1\r\nHost:_
			file_len = snprintf((char *)(ptr + len), HTTP_BYTES_GET_REQ_DATA - len, " HTTP/1.1\r\nHost: ");
			if(file_len >= (HTTP_BYTES_GET_REQ_DATA - len))
			{
				rt_mp_free((void *)ptr);
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET�������ݿռ䲻��"));
				goto __exit;
			}
			len += file_len;
			//��������ַ
			if((len + host_len) > HTTP_BYTES_GET_REQ_DATA)
			{
				rt_mp_free((void *)ptr);
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET�������ݿռ䲻��"));
				goto __exit;
			}
			memcpy((void *)(ptr + len), (void *)ip_addr.addr_data, host_len);
			len += host_len;
			//\r\nUser-Agent: RT-Thread HTTP Agent\r\nAccept: */*\r\n\r\n
			file_len = snprintf((char *)(ptr + len), HTTP_BYTES_GET_REQ_DATA - len, "\r\nUser-Agent: RT-Thread HTTP Agent\r\nAccept: */*\r\n\r\n");
			if(file_len >= (HTTP_BYTES_GET_REQ_DATA - len))
			{
				rt_mp_free((void *)ptr);
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET�������ݿռ䲻��"));
				goto __exit;
			}
			len += file_len;
			
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET��������:\r\n"));
			DEBUG_INFO_OUTPUT(DEBUG_INFO_TYPE_HTTP, (ptr, len));

			//����GET����֮ǰ��׼��
			dtu_http_data_clear();
			if(HTTP_FILE_TYPE_FIRMWARE == url_info_ptr->file_type)
			{
				_http_firmware_erase();
			}
			//����GET����
			host_len = dtu_http_send(&ip_addr, ptr, len);
			rt_mp_free((void *)ptr);
			if(RT_FALSE == host_len)
			{
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET�������ݷ���ʧ��"));
				goto __exit;
			}
			//���յ�һ����Ӧ
			resp_data_ptr = dtu_http_data_pend(HTTP_RECV_DATA_TIMEOUT * RT_TICK_PER_SECOND);
			if((DTU_RECV_DATA *)0 == resp_data_ptr)
			{
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET��Ӧ���ݽ��ճ�ʱ"));
				goto __exit;
			}
			//��Ӧ��
			ptr = fyyp_mem_find(resp_data_ptr->pdata, resp_data_ptr->data_len, " ", 1);
			if((rt_uint8_t *)0 == ptr)
			{
				rt_mp_free((void *)resp_data_ptr);
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET��Ӧ���ݴ���-δ�ҵ���1���ո�"));
				goto __exit;
			}
			ptr++;
			resp_data_ptr->data_len	-= (ptr - resp_data_ptr->pdata);
			resp_data_ptr->pdata	= ptr;
			if(0 == resp_data_ptr->data_len)
			{
				rt_mp_free((void *)resp_data_ptr);
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET��Ӧ���ݴ���-����Ӧ��"));
				goto __exit;
			}
			ptr = fyyp_mem_find(resp_data_ptr->pdata, resp_data_ptr->data_len, " ", 1);
			if((rt_uint8_t *)0 == ptr)
			{
				rt_mp_free((void *)resp_data_ptr);
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET��Ӧ���ݴ���-δ�ҵ���2���ո�"));
				goto __exit;
			}
			len = fyyp_str_to_hex(resp_data_ptr->pdata, ptr - resp_data_ptr->pdata);
			if(HTTP_RESP_CODE_OK != len)
			{
				rt_mp_free((void *)resp_data_ptr);
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET��Ӧ���ݴ���-��Ӧ��[%d]", len));
				goto __exit;
			}
			//�ļ�����Content-Length
			ptr = fyyp_mem_find(resp_data_ptr->pdata, resp_data_ptr->data_len, HTTP_CONTENT_LENGTH, strlen(HTTP_CONTENT_LENGTH));
			if((rt_uint8_t *)0 == ptr)
			{
				rt_mp_free((void *)resp_data_ptr);
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET��Ӧ���ݴ���-δ�ҵ�������"));
				goto __exit;
			}
			ptr += strlen(HTTP_CONTENT_LENGTH);
			resp_data_ptr->data_len	-= (ptr - resp_data_ptr->pdata);
			resp_data_ptr->pdata	= ptr;
			if(0 == resp_data_ptr->data_len)
			{
				rt_mp_free((void *)resp_data_ptr);
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET��Ӧ���ݴ���-���ļ�����"));
				goto __exit;
			}
			ptr = fyyp_mem_find(resp_data_ptr->pdata, resp_data_ptr->data_len, "\r\n", strlen("\r\n"));
			if((rt_uint8_t *)0 == ptr)
			{
				rt_mp_free((void *)resp_data_ptr);
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET��Ӧ���ݴ���-���ļ����Ƚ�β"));
				goto __exit;
			}
			file_len = fyyp_str_to_hex(resp_data_ptr->pdata, ptr - resp_data_ptr->pdata);
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET��Ӧ�����ļ�����[%d]", file_len));
			//�ж��ļ������Ƿ�Ϸ�
			if(HTTP_FILE_TYPE_FIRMWARE == url_info_ptr->file_type)
			{
				if((0 == file_len) || ((file_len + HTTP_BYTES_FIRMWARE_INFO) > HTTP_BYTES_FIRMWARE_MAX))
				{
					rt_mp_free((void *)resp_data_ptr);
					DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET��Ӧ���ݴ���-�ļ����ȳ���"));
					goto __exit;
				}
			}
			else if(HTTP_FILE_TYPE_PARAM == url_info_ptr->file_type)
			{
				if(HTTP_BYTES_PARAM != file_len)
				{
					rt_mp_free((void *)resp_data_ptr);
					DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET��Ӧ���ݴ���-�ļ����ȺͲ����ļ�����"));
					goto __exit;
				}
			}
			//����λ��
			ptr = fyyp_mem_find(resp_data_ptr->pdata, resp_data_ptr->data_len, HTTP_CONTENT_SYMBOL, strlen(HTTP_CONTENT_SYMBOL));
			if((rt_uint8_t *)0 == ptr)
			{
				rt_mp_free((void *)resp_data_ptr);
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET��Ӧ���ݴ���-δ�ҵ�����"));
				goto __exit;
			}
			ptr += strlen(HTTP_CONTENT_SYMBOL);
			resp_data_ptr->data_len	-= (ptr - resp_data_ptr->pdata);
			resp_data_ptr->pdata	= ptr;
			if(0 == resp_data_ptr->data_len)
			{
				rt_mp_free((void *)resp_data_ptr);
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET��Ӧ���ݴ���-������"));
				goto __exit;
			}
			//д���һ������
			len = 0;
			if((len + resp_data_ptr->data_len) > file_len)
			{
				rt_mp_free((void *)resp_data_ptr);
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET��Ӧ���ݴ���-����ʵ�ʳ��ȳ���"));
				goto __exit;
			}
			file_data_write(start_addr + len, resp_data_ptr->pdata, resp_data_ptr->data_len);
			len += resp_data_ptr->data_len;
			rt_mp_free((void *)resp_data_ptr);
			DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]��д��[%d]/�ܳ���[%d]", len, file_len));
			//д��ʣ�������
			while(len < file_len)
			{
				resp_data_ptr = dtu_http_data_pend(HTTP_RECV_DATA_TIMEOUT * RT_TICK_PER_SECOND);
				if((DTU_RECV_DATA *)0 == resp_data_ptr)
				{
					DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET��Ӧ���ݽ��ճ�ʱ"));
					break;
				}
				if((len + resp_data_ptr->data_len) > file_len)
				{
					rt_mp_free((void *)resp_data_ptr);
					DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]GET��Ӧ���ݴ���-����ʵ�ʳ��ȳ���"));
					break;
				}
				file_data_write(start_addr + len, resp_data_ptr->pdata, resp_data_ptr->data_len);
				len += resp_data_ptr->data_len;
				rt_mp_free((void *)resp_data_ptr);
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]��д��[%d]/�ܳ���[%d]", len, file_len));
			}
			//�ж��ļ��Ƿ���
			if(len >= file_len)
			{
				DEBUG_INFO_OUTPUT_STR(DEBUG_INFO_TYPE_HTTP, ("\r\n[http]�ļ��������"));
				if(HTTP_FILE_TYPE_FIRMWARE == url_info_ptr->file_type)
				{
					ptr = (rt_uint8_t *)mempool_req(HTTP_BYTES_FIRMWARE_INFO, RT_WAITING_FOREVER);
					*(rt_uint32_t *)ptr			= HTTP_FIRMWARE_IS_VALID;
					*((rt_uint32_t *)ptr + 1)	= file_len;
					file_data_write(HTTP_FIRMWARE_STORE_ADDR, ptr, HTTP_BYTES_FIRMWARE_INFO);
					rt_mp_free((void *)ptr);
					w25qxxx_close_ex();
					HAL_NVIC_SystemReset();
				}
				else if(HTTP_FILE_TYPE_PARAM == url_info_ptr->file_type)
				{
					//��̬�洢
//					sample_clear_old_data();
				}
			}

__exit:
			if(HTTP_FILE_TYPE_FIRMWARE == url_info_ptr->file_type)
			{
				w25qxxx_close_ex();
			}
			rt_mp_free((void *)url_info_ptr);
		}

		lpm_cpu_ref(RT_FALSE);
	}
}

static int _http_component_init(void)
{
	if(RT_EOK != rt_mb_init(&m_http_mb_url_info, "http", (void *)&m_http_msgpool_url_info, 1, RT_IPC_FLAG_PRIO))
	{
		while(1);
	}
	
	return 0;
}
INIT_COMPONENT_EXPORT(_http_component_init);

static int _http_app_init(void)
{
	if(RT_EOK != rt_thread_init(&m_http_thread_get_req, "http", _http_task_get_req, (void *)0, (void *)m_http_stk_get_req, HTTP_STK_GET_REQ, HTTP_PRIO_GET_REQ, 2))
	{
		while(1);
	}
	if(RT_EOK != rt_thread_startup(&m_http_thread_get_req))
	{
		while(1);
	}
	
	return 0;
}
INIT_APP_EXPORT(_http_app_init);



rt_uint8_t http_get_req_trigger(rt_uint8_t file_type, rt_uint8_t const *purl, rt_uint16_t url_len)
{
	HTTP_URL_INFO *url_info_ptr;
	
	if(file_type >= HTTP_NUM_FILE_TYPE)
	{
		return RT_FALSE;
	}
	if((rt_uint8_t *)0 == purl)
	{
		return RT_FALSE;
	}
	if(0 == url_len)
	{
		return RT_FALSE;
	}

	url_info_ptr = _http_url_info_req(url_len, RT_WAITING_FOREVER);
	if((HTTP_URL_INFO *)0 == url_info_ptr)
	{
		return RT_FALSE;
	}

	url_info_ptr->file_type	= file_type;
	url_info_ptr->url_len	= url_len;
	memcpy((void *)url_info_ptr->purl, (void *)purl, url_len);

	if(RT_EOK == rt_mb_send_wait(&m_http_mb_url_info, (rt_ubase_t)url_info_ptr, RT_WAITING_NO))
	{
		return RT_TRUE;
	}

	rt_mp_free((void *)url_info_ptr);
	return RT_FALSE;
}
