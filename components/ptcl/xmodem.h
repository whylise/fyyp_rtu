#ifndef __XMODEM_H__
#define __XMODEM_H__



#include "rtthread.h"
#include "user_priority.h"
#include "ptcl.h"



#define XMCS_KW_SOH					0x01	//������������
#define XMCS_KW_REQ					0x15	//��������
#define XMCS_KW_EOT					0x04	//��ȷ��ɴ���
#define XMCS_KW_CAN					0x18	//�д���ȡ������
#define XMCS_KW_EOF					0x1a	//��������
#define XMCS_KW_REQ_EX				0x02	//�������ݣ���չͷ

//������Ϣ
#define XM_BYTES_FRAME_FILE_LEN			4
#define XM_BYTES_FRAME_PACKET_INFO		2
#define XM_BYTES_FRAME_CRC				1
#define XM_BYTES_PER_TRANS_MIN			900
#define XM_BYTES_PER_TRANS_MAX			900
#define XM_TRANS_TIMEOUT_MIN			2	//��
#define XM_TRANS_TIMEOUT_MAX			10	//��
#define XM_TRANS_TRY_TIMES				5
#define XM_BYTES_PER_TRANS_PLATFORM		1000

//�ļ����䷽��
#define XM_DIR_TO_DEVICE			0
#define XM_DIR_FROM_DEVICE			1
#define XM_DIR_FROM_PLATFORM		2
#define XM_NUM_TRANS_DIR			3

//�ļ�����
#define XM_FILE_TYPE_PARAM			0
#define XM_FILE_TYPE_FIRMWARE		1
#define XM_NUM_FILE_TYPE			2

//�̼�firmware
#define XM_BYTES_FIRMWARE_INFO		8
#define XM_BYTES_FIRMWARE_MAX		(1024 * 1024)
#define XM_FIRMWARE_IS_VALID		0xa55a5aa5
#define XM_FIRMWARE_STORE_ADDR		0

//����param
#define XM_BYTES_PARAM				(4 * 1024)
#define XM_PARAM_STORE_ADDR			0

//����
#define XM_STK_FILE_TRANS			720
#ifndef XM_PRIO_FILE_TRANS
#define XM_PRIO_FILE_TRANS			20
#endif



typedef void (*XM_FUN_SHELL)(PTCL_REPORT_DATA **report_data_ptr_ptr);

typedef struct xm_file_trans_info
{
	rt_uint8_t		comm_type;
	rt_uint8_t		ch;
	rt_uint8_t		trans_dir;
	rt_uint8_t		file_type;
	XM_FUN_SHELL	fun_shell;
} XM_FILE_TRANS_INFO;



rt_uint8_t xm_data_decoder(PTCL_RECV_DATA const *recv_data_ptr);
rt_uint8_t xm_file_trans_trigger(rt_uint8_t comm_type, rt_uint8_t ch, rt_uint8_t trans_dir, rt_uint8_t file_type, XM_FUN_SHELL fun_shell);



#endif

