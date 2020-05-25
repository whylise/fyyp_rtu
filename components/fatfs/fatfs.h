#ifndef __FATFS_H__
#define __FATFS_H__



#include "rtthread.h"
#include "drv_pin.h"
#include "stm32f4xx.h"



//Ӳ����Դ��ʽ
#if 0
#define FATFS_HW_PWR_ALWAYS_ON
#endif

//Ӳ������
#define FATFS_HW_PIN_PWREN				PIN_INDEX_63
#define FATFS_HW_PIN_CS					PIN_INDEX_64
#define FATFS_HW_PIN_SCK				PIN_INDEX_65
#define FATFS_HW_PIN_MISO				PIN_INDEX_66
#define FATFS_HW_PIN_MOSI				PIN_INDEX_67
#define FATFS_HW_PIN_AF_INDEX			GPIO_AF5_SPI4
#define FATFS_HW_SPIX					SPI4
#define FATFS_HW_SPI_CLK_EN				__HAL_RCC_SPI4_CLK_ENABLE
#define FATFS_HW_SPI_CLK_DIS			__HAL_RCC_SPI4_CLK_DISABLE();__HAL_RCC_SPI4_FORCE_RESET();__HAL_RCC_SPI4_RELEASE_RESET

//������
#define FATFS_ERR_NONE					0
#define FATFS_ERR_ARG					1
#define FATFS_ERR_MKFS					2
#define FATFS_ERR_MOUNT					3
#define FATFS_ERR_OPEN					4
#define FATFS_ERR_WRITE					5
#define FATFS_ERR_WRLEN					6
#define FATFS_ERR_MEM_MKFS				7
#define FATFS_ERR_MEM_FATFS				8
#define FATFS_ERR_MEM_FIL				9

//�¼�
#define FATFS_EVENT_REF_TIMES			0x01
#define FATFS_EVENT_MODULE_PEND			0x02
#define FATFS_EVENT_INIT_VALUE			0x03

//SD������
#define FATFS_SD_TYPE_NONE				0		//�޿�
#define FATFS_SD_TYPE_MMC_V3			1		//MMC��Ver3
#define FATFS_SD_TYPE_SC_V1				2		//SDSC��Ver1
#define FATFS_SD_TYPE_SC_V2				3		//SDSC��Ver2
#define FATFS_SD_TYPE_HC_XC				4		//SDHC����SDXC��

//SD��CMD
#define FATFS_SD_CMD_0					0			//��λָ��
#define FATFS_SD_CMD_1					1			//MMC����ʼ��ָ��
#define FATFS_SD_CMD_8					8			//�鿴�������ѹ��ͨ��״ָ̬��
#define FATFS_SD_CMD_9					9			//SEND_CSD
#define FATFS_SD_CMD_12					12			//����������ָ��
#define FATFS_SD_ACMD_13				(13 + 0x80)	//SD_STATUS(SDC)
#define FATFS_SD_CMD_16					16			//���ÿ鳤��
#define FATFS_SD_CMD_17					17			//������ָ��
#define FATFS_SD_CMD_18					18			//�����ָ��
#define FATFS_SD_ACMD_23				(23 + 0x80)	//����дʱָ������
#define FATFS_SD_CMD_24					24			//д����ָ��
#define FATFS_SD_CMD_25					25			//д���ָ��
#define FATFS_SD_ACMD_41				(41 + 0x80)	//SD����ʼ��ָ��
#define FATFS_SD_CMD_55					55			//ָʾ����ָ��Ϊ����ָ��
#define FATFS_SD_CMD_58					58			//��OCR����ȡCCS��Ϣ
#define FATFS_SD_CMD_32					32			//ERASE_ER_BLK_START
#define FATFS_SD_CMD_33					33			//ERASE_ER_BLK_END
#define FATFS_SD_CMD_38					38			//ERASE

//Data Token
#define FATFS_SD_DATA_TOKEN_BLK_RD			0xfe	//����������
#define FATFS_SD_DATA_TOKEN_SINGLE_WR		0xfe	//����д
#define FATFS_SD_DATA_TOKEN_MULTI_WR		0xfc	//���д
#define FATFS_SD_DATA_TOKEN_STOP_TRAN		0xfd	//�������д

//������
#define FATFS_SD_ERR_NONE				0
#define FATFS_SD_ERR_NOT_RDY			1		//δ׼����
#define FATFS_SD_ERR_TOKEN_NONE			2		//��token
#define FATFS_SD_ERR_TOKEN_ERR			3		//����token
#define FATFS_SD_ERR_DATA_REJECT		4		//���ݱ��ܾ�



rt_uint8_t fatfs_mkfs(void);

rt_uint8_t fatfs_write(char const *file_name, rt_uint8_t const *pdata, rt_uint16_t data_len);



#endif

