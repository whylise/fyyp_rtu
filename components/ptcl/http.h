#ifndef __HTTP_H__
#define __HTTP_H__



#include "rtthread.h"
#include "user_priority.h"



//������Ϣ
#define HTTP_RECV_DATA_TIMEOUT		10			//�������ݳ�ʱʱ�䣬��λ��
#define HTTP_BYTES_GET_REQ_DATA		200			//GET���������ֽ���
#define HTTP_URL_HEAD				"http://"
#define HTTP_DEF_SERVER_PORT		":80"
#define HTTP_RESP_CODE_OK			200
#define HTTP_CONTENT_LENGTH			"Content-Length: "
#define HTTP_CONTENT_SYMBOL			"\r\n\r\n"

//�ļ�����
#define HTTP_FILE_TYPE_FIRMWARE		0
#define HTTP_FILE_TYPE_PARAM		1
#define HTTP_NUM_FILE_TYPE			2

//�̼�firmware
#define HTTP_BYTES_FIRMWARE_INFO	8
#define HTTP_BYTES_FIRMWARE_MAX		(1024 * 1024)
#define HTTP_FIRMWARE_IS_VALID		0xa55a5aa5
#define HTTP_FIRMWARE_STORE_ADDR	0

//����param
#define HTTP_BYTES_PARAM			(4 * 1024)
#define HTTP_PARAM_STORE_ADDR		0

//����
#define HTTP_STK_GET_REQ			640
#ifndef HTTP_PRIO_GET_REQ
#define HTTP_PRIO_GET_REQ			4
#endif

//URL��Ϣ
typedef struct http_url_info
{
	rt_uint8_t		file_type;
	rt_uint16_t		url_len;
	rt_uint8_t		*purl;
} HTTP_URL_INFO;



rt_uint8_t http_get_req_trigger(rt_uint8_t file_type, rt_uint8_t const *purl, rt_uint16_t url_len);



#endif
