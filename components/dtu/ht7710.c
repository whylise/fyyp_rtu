/*ʹ��˵��
**1�����DTU���õ��ǣ����������ߡ����������ߡ����绰�����ݻ��ѡ���������������0����
**2���Ƽ�ʹ��ģʽ����DTU����ΪDTU_MODE_ONLINE����DTU������Ϊ0��,�ص����£�
**		-DTU�����������ߣ���������ʱ�Ͳ�������IP������IP��ʵ�������ú��IP��ַ����������Ҫ������ʱ�䣻
**		-DTU�������ᷢ��������������ڿ���״̬�¿����������ߣ�������ʵ���˵���ģʽ��
**		-����Ҫʵ�����ߣ���DTU���������ò�Ϊ0���ɣ�
**		-������ģʽ����ʵ�ֺ����繦�ܣ���Ҫ��DTU����ΪDTU_MODE_PWRģʽ�����ʺ������ݷ��ͼ���ϴ���������1Сʱ
**3������������ʵ��ֻ�ǽ�����״̬��0��DTU_STA_IP״̬���������ִ�е�������ʱ�����DTU��δ�������ߣ��������DTU������������ߵģ���ѡ��ر�IP�ķ�ʽ�Ͽ����ӣ�����Ϊ�绰���ѹ��ܣ��е绰��ʱ����DTU��ֱ�����ߣ���������ر�IP�����˵绰���޷������ˣ�
*/

/*�������
**ʹ�ú���������m2mtoolsbox�������ã����DTUĬ�ϲ�������57600����������RX��TX��DTU�ϱ��RX��TX�ǽ������ӵġ�
**1������������Ϊ9600��
**2������RDPЭ�飬��ѡ��En��
**3������RDP����Э�飬��ѡ��GPRS+SMS��
**4���������ĺ����Ϊ0001��
**5��������Ϣ�رգ�
**6���������ͨ�����Զ���ע������Զ�����������ͨ��������롣
*/



/*��ʱ����
**
*/
#define DTU_TIME_PWR_OFF			30u						//�ϵ�ʱ��
#define DTU_TIME_PWR_ON				50u						//�ϵ�ʱ��
#define DTU_TIME_STA_OFF			3u						//�ػ�ʱ��
#define DTU_TIME_STA_ON				10u						//����ʱ��
#define DTU_TIME_SET_PARAM			3u						//���ò���ָ�ʱʱ��
#define DTU_TIME_SAVE_PARAM			10u						//�������ָ�ʱʱ��
#define DTU_TIME_RESET_START		10u						//��ʼ��λ��ʱʱ��
#define DTU_TIME_RESET_END			60u						//������λ��ʱʱ��
#define DTU_TIME_SMS_SEND			15u						//SMS���ͳ�ʱʱ��
#define DTU_TIME_IP_SEND			1u						//IP���ͳ�ʱʱ��
#define DTU_TIME_BOOT_SMS			10u						//BOOT_SMS��ɺ����ʱʱ��
#define DTU_TIME_QUERY_STA			3u						//��ѯ״̬��ʱʱ��

/*�ֽڿռ�
**
*/
#define DTU_BYTES_AT_SEND			65u						//AT����ָ��

/*�¼���־��
**
*/
#define DTU_FLAG_NONE_YES			0x01u
#define DTU_FLAG_NONE_NO			0x02u
#define DTU_FLAG_SET_OK				0x04u
#define DTU_FLAG_SET_ERROR			0x08u
#define DTU_FLAG_SAVE_OK			0x10u
#define DTU_FLAG_SAVE_ERROR			0x20u
#define DTU_FLAG_RESET_OK			0x40u
#define DTU_FLAG_RESET_ERROR		0x80u
#define DTU_FLAG_SMS_OK				0x100u
#define DTU_FLAG_SMS_ERROR			0x200u
#define DTU_FLAG_IP_OK				0x400u
#define DTU_FLAG_IP_ERROR			0x800u
#define DTU_FLAG_MODULE_OK			0x1000u
#define DTU_FLAG_MODULE_ERROR		0x2000u
#define DTU_FLAG_CSQ				0x4000u



static INT8U			m_atSend[DTU_BYTES_AT_SEND];		//AT����ָ��ռ�
static INT8U			m_atSendLen;						//AT����ָ���



/*֡�ṹ
**
*/
#define HT_PACKET_HEAD				0x7Du		//��ͷ
#define HT_PACKET_END				0x7Fu		//��β
#define HT_BYTES_HEAD				3u			//��ͷ�ֽ���
#define HT_BYTES_LEN				2u			//�����ֽ���
#define HT_BYTES_AFN				2u			//�������ֽ���
#define HT_BYTES_CRC				1u			//У�����ֽ���
#define HT_BYTES_END				3u			//��β�ֽ���
#define HT_BYTES_BASIC_FRAME		11u			//����֡�ṹ�ֽ���HT_BYTES_HEAD + HT_BYTES_LEN + HT_BYTES_AFN + HT_BYTES_CRC + HT_BYTES_END
#define HT_BYTES_SMS_ADDR			32u			//���������к�����ֽ���

/*������
**
*/
#define HT_AFNU_OPEN_RDP			0x00u		//��ʱ��RDP
#define HT_AFNU_QUERY_PARAM			0x01u		//��ѯ����
#define HT_AFNU_SET_PARAM			0x02u		//���ò���
#define HT_AFNU_SAVE_PARAM			0x03u		//�������
#define HT_AFNU_RESET				0x04u		//��λ
#define HT_AFNU_SEND_GPRS			0x05u		//����GPRS
#define HT_AFNU_SEND_SMS			0x06u		//����SMS
#define HT_AFNU_QUERY_STA			0x07u		//��ѯ״̬
#define HT_AFND_OPEN_RDP			0x80u		//��ʱ��RDP
#define HT_AFND_QUERY_PARAM			0x81u		//��ѯ����
#define HT_AFND_SET_PARAM			0x82u		//���ò���
#define HT_AFND_SAVE_PARAM			0x83u		//�������
#define HT_AFND_RESET				0x84u		//��λ
#define HT_AFND_SEND_GPRS			0x85u		//����GPRS
#define HT_AFND_SEND_SMS			0x86u		//����SMS
#define HT_AFND_QUERY_STA			0x87u		//��ѯ״̬

/*����ID
**
*/
#define HT_IDH_PARAM_RTU			0x03u		//RTU����
#define HT_IDH_PARAM_SMS			0x04u		//���Ų���
#define HT_IDH_PARAM_RUN			0x05u		//���в���
#define HT_IDH_PARAM_CH1			0xC8u		//ͨ��1����
#define HT_IDH_PARAM_CH2			0xC9u		//ͨ��2����
#define HT_IDH_PARAM_CH3			0xCAu		//ͨ��3����
#define HT_IDH_PARAM_CH4			0xCBu		//ͨ��4����
#define HT_IDH_PARAM_STA			0xF0u		//״̬����

#define HT_IDL_DATA_MAX				0x07u		//������ݰ�
#define HT_IDL_DATA_TIME			0x08u		//���ݰ����ʱ��
#define HT_IDL_SMS_TYPE				0x02u		//���ű��뷽ʽ
#define HT_IDL_ONLINE_TYPE			0x03u		//���߷�ʽ
#define HT_IDL_WAKE_TYPE			0x04u		//���ѷ�ʽ
#define HT_IDL_OFFLINE_TIME			0x06u		//����ʱ��
#define HT_IDL_OFFLINE_TYPE			0x07u		//���߷�ʽ
#define HT_IDL_CSQ_MIN				0x08u		//����ź�ǿ��
#define HT_IDL_CONN_TIMES			0x0Au		//�����Ӵ���
#define HT_IDL_CONN_PERIOD			0x0Bu		//����ʱ����
#define HT_IDL_RUN_MODE				0x0Cu		//����ģʽ
#define HT_IDL_CONN_TYPE			0x01u		//���ӷ�ʽ
#define HT_IDL_IP_ADDR				0x02u		//IP��ַ
#define HT_IDL_IP_PORT				0x03u		//IP�˿�
#define HT_IDL_DOMAIN_NAME			0x04u		//����
#define HT_IDL_HEART_PERIOD			0x06u		//����������
#define HT_IDL_SERVICE_SMS			0x0Cu		//ͨ���ķ������
#define HT_IDL_CSQ_VALUE			0x04u		//�ź�ǿ��
#define HT_IDL_MODULE_STA			0x01u		//ģ��״̬
#define HT_IDL_SMS_SEND_STA			0x0Eu		//���ŷ���״̬

/*��������
**
*/
#define HT_LEN_DATA_MAX				2u			//������ݰ�
#define HT_LEN_DATA_TIME			2u			//���ݰ����ʱ��
#define HT_LEN_SMS_TYPE				1u			//���ű��뷽ʽ
#define HT_LEN_ONLINE_TYPE			1u			//���߷�ʽ
#define HT_LEN_WAKE_TYPE			1u			//���ѷ�ʽ
#define HT_LEN_OFFLINE_TIME			2u			//����ʱ��
#define HT_LEN_OFFLINE_TYPE			1u			//���߷�ʽ
#define HT_LEN_CSQ_MIN				1u			//��������ź�ǿ��
#define HT_LEN_CONN_TIMES			1u			//�����Ӵ���
#define HT_LEN_CONN_PERIOD			2u			//������ʱ��
#define HT_LEN_RUN_MODE				1u			//����ģʽ
#define HT_LEN_HEART_PERIOD			2u			//������ʱ����
#define HT_LEN_CONN_TYPE			1u			//���ӷ�ʽ
#define HT_LEN_IP_ADDR				4u			//IP��ַ
#define HT_LEN_IP_PORT				2u			//�˿ں�

/*���ű��뷽ʽ
**
*/
#define HT_SMS_TYPE_7BIT			0u			//7BIT
#define HT_SMS_TYPE_8BIT			1u			//8BIT
#define HT_SMS_TYPE_UCS2			2u			//UCS2

/*���߷�ʽ
**
*/
#define HT_ONLINE_TYPE_AUTO			0u			//�Զ�����
#define HT_ONLINE_TYPE_WAKE			1u			//�ȴ�����

/*���ѷ�ʽ
**
*/
#define HT_WAKE_TYPE_SMS			1u			//SMS����
#define HT_WAKE_TYPE_CALL			2u			//CALL����
#define HT_WAKE_TYPE_DATA			4u			//DATA����

/*���߷�ʽ
**
*/
#define HT_OFFLINE_TYPE_IDLE		0u			//��������
#define HT_OFFLINE_TYPE_TIME		1u			//��ʱ����

/*����ģʽ
**
*/
#define HT_RUN_MODE_MUL				0u			//��ͨ��
#define HT_RUN_MODE_MULB			1u			//��ͨ������

/*���ӷ�ʽ
**
*/
#define HT_CONN_TYPE_UDP			0u			//UDP
#define HT_CONN_TYPE_TCP			1u			//TCP

/*��������
**
*/
#define HT_RTU_DATA_MAX				1024u		//������ݰ�
#define HT_RTU_DATA_TIME			500u		//���ݰ����ʱ�䣬����
#define HT_RUN_OFFLINE_TIME			60u			//����ʱ�䣬��
#define HT_RUN_CSQ_MIN				5u			//��������ź�ǿ��
#define HT_RUN_CONN_TIMES			3u			//�����Ӵ���
#define HT_RUN_CONN_PERIOD			30u			//�����Ӽ��
#define HT_CH_HEART_PERIOD			0u			//���������



/*������ݰ�
**
*/
static INT8U CMD_DataMax(INT8U *pCMD, INT16U dataMax)
{
	INT8U cmdLen = 0u;
	
	if((INT8U *)0u == pCMD)
	{
		return 0u;
	}
	
	pCMD[cmdLen++] = HT_IDH_PARAM_RTU;
	pCMD[cmdLen++] = HT_IDL_DATA_MAX;
	pCMD[cmdLen++] = (INT8U)(HT_LEN_DATA_MAX >> 8u);
	pCMD[cmdLen++] = (INT8U)HT_LEN_DATA_MAX;
	pCMD[cmdLen++] = (INT8U)dataMax;
	pCMD[cmdLen++] = (INT8U)(dataMax >> 8u);
	
	return cmdLen;
}

/*���ݰ����ʱ��
**
*/
static INT8U CMD_DataTime(INT8U *pCMD, INT16U dataTime)
{
	INT8U cmdLen = 0u;
	
	if((INT8U *)0u == pCMD)
	{
		return 0u;
	}
	
	pCMD[cmdLen++] = HT_IDH_PARAM_RTU;
	pCMD[cmdLen++] = HT_IDL_DATA_TIME;
	pCMD[cmdLen++] = (INT8U)(HT_LEN_DATA_TIME >> 8u);
	pCMD[cmdLen++] = (INT8U)HT_LEN_DATA_TIME;
	pCMD[cmdLen++] = (INT8U)dataTime;
	pCMD[cmdLen++] = (INT8U)(dataTime >> 8u);
	
	return cmdLen;
}

/*���ű��뷽ʽ
**
*/
static INT8U CMD_SmsType(INT8U *pCMD, INT8U smsType)
{
	INT8U cmdLen = 0u;
	
	if((INT8U *)0u == pCMD)
	{
		return 0u;
	}
	
	pCMD[cmdLen++] = HT_IDH_PARAM_SMS;
	pCMD[cmdLen++] = HT_IDL_SMS_TYPE;
	pCMD[cmdLen++] = (INT8U)(HT_LEN_SMS_TYPE >> 8u);
	pCMD[cmdLen++] = (INT8U)HT_LEN_SMS_TYPE;
	pCMD[cmdLen++] = smsType;
	
	return cmdLen;
}

/*���߷�ʽ
**
*/
static INT8U CMD_OnlineType(INT8U *pCMD, INT8U onlineType)
{
	INT8U cmdLen = 0u;
	
	if((INT8U *)0u == pCMD)
	{
		return 0u;
	}
	
	pCMD[cmdLen++] = HT_IDH_PARAM_RUN;
	pCMD[cmdLen++] = HT_IDL_ONLINE_TYPE;
	pCMD[cmdLen++] = (INT8U)(HT_LEN_ONLINE_TYPE >> 8u);
	pCMD[cmdLen++] = (INT8U)HT_LEN_ONLINE_TYPE;
	pCMD[cmdLen++] = onlineType;
	
	return cmdLen;
}

/*���ѷ�ʽ
**
*/
static INT8U CMD_WakeType(INT8U *pCMD, INT8U wakeType)
{
	INT8U cmdLen = 0u;
	
	if((INT8U *)0u == pCMD)
	{
		return 0u;
	}
	
	pCMD[cmdLen++] = HT_IDH_PARAM_RUN;
	pCMD[cmdLen++] = HT_IDL_WAKE_TYPE;
	pCMD[cmdLen++] = (INT8U)(HT_LEN_WAKE_TYPE >> 8u);
	pCMD[cmdLen++] = (INT8U)HT_LEN_WAKE_TYPE;
	pCMD[cmdLen++] = wakeType;
	
	return cmdLen;
}

/*����ʱ��
**
*/
static INT8U CMD_OfflineTime(INT8U *pCMD, INT16U offlineTime)
{
	INT8U cmdLen = 0u;
	
	if((INT8U *)0u == pCMD)
	{
		return 0u;
	}
	
	pCMD[cmdLen++] = HT_IDH_PARAM_RUN;
	pCMD[cmdLen++] = HT_IDL_OFFLINE_TIME;
	pCMD[cmdLen++] = (INT8U)(HT_LEN_OFFLINE_TIME >> 8u);
	pCMD[cmdLen++] = (INT8U)HT_LEN_OFFLINE_TIME;
	pCMD[cmdLen++] = (INT8U)offlineTime;
	pCMD[cmdLen++] = (INT8U)(offlineTime >> 8u);
	
	return cmdLen;
}

/*���߷�ʽ
**
*/
static INT8U CMD_OfflineType(INT8U *pCMD, INT8U offlineType)
{
	INT8U cmdLen = 0u;
	
	if((INT8U *)0u == pCMD)
	{
		return 0u;
	}
	
	pCMD[cmdLen++] = HT_IDH_PARAM_RUN;
	pCMD[cmdLen++] = HT_IDL_OFFLINE_TYPE;
	pCMD[cmdLen++] = (INT8U)(HT_LEN_OFFLINE_TYPE >> 8u);
	pCMD[cmdLen++] = (INT8U)HT_LEN_OFFLINE_TYPE;
	pCMD[cmdLen++] = offlineType;
	
	return cmdLen;
}

/*��������ź�ǿ��
**
*/
static INT8U CMD_CsqMin(INT8U *pCMD, INT8U csqValue)
{
	INT8U cmdLen = 0u;
	
	if((INT8U *)0u == pCMD)
	{
		return 0u;
	}
	
	pCMD[cmdLen++] = HT_IDH_PARAM_RUN;
	pCMD[cmdLen++] = HT_IDL_CSQ_MIN;
	pCMD[cmdLen++] = (INT8U)(HT_LEN_CSQ_MIN >> 8u);
	pCMD[cmdLen++] = (INT8U)HT_LEN_CSQ_MIN;
	pCMD[cmdLen++] = csqValue;
	
	return cmdLen;
}

/*�����Ӵ���
**
*/
static INT8U CMD_ConnTimes(INT8U *pCMD, INT8U connTimes)
{
	INT8U cmdLen = 0u;
	
	if((INT8U *)0u == pCMD)
	{
		return 0u;
	}
	
	pCMD[cmdLen++] = HT_IDH_PARAM_RUN;
	pCMD[cmdLen++] = HT_IDL_CONN_TIMES;
	pCMD[cmdLen++] = (INT8U)(HT_LEN_CONN_TIMES >> 8u);
	pCMD[cmdLen++] = (INT8U)HT_LEN_CONN_TIMES;
	pCMD[cmdLen++] = connTimes;
	
	return cmdLen;
}

/*������ʱ��
**
*/
static INT8U CMD_ConnPeriod(INT8U *pCMD, INT16U connPeriod)
{
	INT8U cmdLen = 0u;
	
	if((INT8U *)0u == pCMD)
	{
		return 0u;
	}
	
	pCMD[cmdLen++] = HT_IDH_PARAM_RUN;
	pCMD[cmdLen++] = HT_IDL_CONN_PERIOD;
	pCMD[cmdLen++] = (INT8U)(HT_LEN_CONN_PERIOD >> 8u);
	pCMD[cmdLen++] = (INT8U)HT_LEN_CONN_PERIOD;
	pCMD[cmdLen++] = (INT8U)connPeriod;
	pCMD[cmdLen++] = (INT8U)(connPeriod >> 8u);
	
	return cmdLen;
}

/*����ģʽ
**
*/
static INT8U CMD_RunMode(INT8U *pCMD, INT8U runMode)
{
	INT8U cmdLen = 0u;
	
	if((INT8U *)0u == pCMD)
	{
		return 0u;
	}
	
	pCMD[cmdLen++] = HT_IDH_PARAM_RUN;
	pCMD[cmdLen++] = HT_IDL_RUN_MODE;
	pCMD[cmdLen++] = (INT8U)(HT_LEN_RUN_MODE >> 8u);
	pCMD[cmdLen++] = (INT8U)HT_LEN_RUN_MODE;
	pCMD[cmdLen++] = runMode;
	
	return cmdLen;
}

/*���������
**
*/
static INT8U CMD_HeartPeriod(INT8U *pCMD, INT8U ipCh, INT16U heartPeriod)
{
	INT8U cmdLen = 0u;
	
	if((INT8U *)0u == pCMD)
	{
		return 0u;
	}
	
	pCMD[cmdLen++] = HT_IDH_PARAM_CH1 + ipCh;
	pCMD[cmdLen++] = HT_IDL_HEART_PERIOD;
	pCMD[cmdLen++] = (INT8U)(HT_LEN_HEART_PERIOD >> 8u);
	pCMD[cmdLen++] = (INT8U)HT_LEN_HEART_PERIOD;
	pCMD[cmdLen++] = (INT8U)heartPeriod;
	pCMD[cmdLen++] = (INT8U)(heartPeriod >> 8u);
	
	return cmdLen;
}

/*����ָ��
**
*/
static INT8U CMD_ParamSet(INT8U *pCMD, INT8U cmdNum)
{
	INT8U cmdLen = 0u, posLen;
	
	if((INT8U *)0u == pCMD)
	{
		return 0u;
	}
	
	//��ͷ
	pCMD[cmdLen++] = HT_PACKET_HEAD;
	pCMD[cmdLen++] = HT_PACKET_HEAD;
	pCMD[cmdLen++] = HT_PACKET_HEAD;
	//����
	posLen = cmdLen;
	pCMD[cmdLen++] = 0u;
	pCMD[cmdLen++] = 0u;
	//������
	pCMD[cmdLen++] = HT_AFNU_SET_PARAM;
	pCMD[cmdLen++] = 0u;
	switch(cmdNum)
	{
	case 0u:
		//������ݰ�6
		cmdLen += CMD_DataMax(&pCMD[cmdLen], HT_RTU_DATA_MAX);
		//���ݰ����ʱ��6
		cmdLen += CMD_DataTime(&pCMD[cmdLen], HT_RTU_DATA_TIME);
		//���ű��뷽ʽ5
		cmdLen += CMD_SmsType(&pCMD[cmdLen], HT_SMS_TYPE_8BIT);
		//���߷�ʽ5
		cmdLen += CMD_OnlineType(&pCMD[cmdLen], HT_ONLINE_TYPE_WAKE);
		//���ѷ�ʽ5
		cmdLen += CMD_WakeType(&pCMD[cmdLen], HT_WAKE_TYPE_DATA);
		//����ʱ��6
		cmdLen += CMD_OfflineTime(&pCMD[cmdLen], HT_RUN_OFFLINE_TIME);
		//���߷�ʽ5
		cmdLen += CMD_OfflineType(&pCMD[cmdLen], HT_OFFLINE_TYPE_IDLE);
		//��������ź�ǿ��5
		cmdLen += CMD_CsqMin(&pCMD[cmdLen], HT_RUN_CSQ_MIN);
		//�����Ӵ���5
		cmdLen += CMD_ConnTimes(&pCMD[cmdLen], HT_RUN_CONN_TIMES);
		break;
	case 1u:
		//������ʱ��6
		cmdLen += CMD_ConnPeriod(&pCMD[cmdLen], HT_RUN_CONN_PERIOD);
		//����ģʽ5
		cmdLen += CMD_RunMode(&pCMD[cmdLen], HT_RUN_MODE_MUL);
		//���������24
		cmdLen += CMD_HeartPeriod(&pCMD[cmdLen], 0u, HT_CH_HEART_PERIOD);
		cmdLen += CMD_HeartPeriod(&pCMD[cmdLen], 1u, HT_CH_HEART_PERIOD);
		cmdLen += CMD_HeartPeriod(&pCMD[cmdLen], 2u, HT_CH_HEART_PERIOD);
		cmdLen += CMD_HeartPeriod(&pCMD[cmdLen], 3u, HT_CH_HEART_PERIOD);
		break;
	default:
		return 0u;
	}
	//��β
	pCMD[cmdLen++] = 0u;
	pCMD[cmdLen++] = HT_PACKET_END;
	pCMD[cmdLen++] = HT_PACKET_END;
	pCMD[cmdLen++] = HT_PACKET_END;
	//����
	pCMD[posLen++] = (INT8U)(cmdLen >> 8u);
	pCMD[posLen++] = (INT8U)cmdLen;
	
	return cmdLen;
}

/*�������
**
*/
static INT8U CMD_SaveParam(INT8U *pCMD)
{
	INT8U cmdLen = 0u, posLen;
	
	if((INT8U *)0u == pCMD)
	{
		return 0u;
	}
	
	//��ͷ
	pCMD[cmdLen++] = HT_PACKET_HEAD;
	pCMD[cmdLen++] = HT_PACKET_HEAD;
	pCMD[cmdLen++] = HT_PACKET_HEAD;
	//����
	posLen = cmdLen;
	pCMD[cmdLen++] = 0u;
	pCMD[cmdLen++] = 0u;
	//������
	pCMD[cmdLen++] = HT_AFNU_SAVE_PARAM;
	pCMD[cmdLen++] = 0u;
	//��β
	pCMD[cmdLen++] = 0u;
	pCMD[cmdLen++] = HT_PACKET_END;
	pCMD[cmdLen++] = HT_PACKET_END;
	pCMD[cmdLen++] = HT_PACKET_END;
	//����
	pCMD[posLen++] = (INT8U)(cmdLen >> 8u);
	pCMD[posLen++] = (INT8U)cmdLen;
	
	return cmdLen;
}

/*����ĳIPͨ��
**
*/
static INT8U CMD_ConnIP(INT8U *pCMD, INT8U ipCh, DTU_IP_ADDR const *pIpAddr, INT8U isUDP)
{
	INT8U	cmdLen = 0u, posLen, pos;
	INT16U	port;
	
	if((INT8U *)0u == pCMD)
	{
		return 0u;
	}
	
	if((DTU_IP_ADDR *)0u == pIpAddr)
	{
		return 0u;
	}
	
	//��ͷ
	pCMD[cmdLen++] = HT_PACKET_HEAD;
	pCMD[cmdLen++] = HT_PACKET_HEAD;
	pCMD[cmdLen++] = HT_PACKET_HEAD;
	//����
	posLen = cmdLen;
	pCMD[cmdLen++] = 0u;
	pCMD[cmdLen++] = 0u;
	//������
	pCMD[cmdLen++] = HT_AFNU_SET_PARAM;
	pCMD[cmdLen++] = 0u;
	//������
	//���ӷ�ʽ
	pCMD[cmdLen++] = HT_IDH_PARAM_CH1 + ipCh;
	pCMD[cmdLen++] = HT_IDL_CONN_TYPE;
	pCMD[cmdLen++] = (INT8U)(HT_LEN_CONN_TYPE >> 8u);
	pCMD[cmdLen++] = (INT8U)HT_LEN_CONN_TYPE;
	pCMD[cmdLen++] = (OS_TRUE == isUDP) ? HT_CONN_TYPE_UDP : HT_CONN_TYPE_TCP;
	//IP��ַ
	pCMD[cmdLen++] = HT_IDH_PARAM_CH1 + ipCh;
	pCMD[cmdLen++] = HT_IDL_IP_ADDR;
	pCMD[cmdLen++] = (INT8U)(HT_LEN_IP_ADDR >> 8u);
	pCMD[cmdLen++] = (INT8U)HT_LEN_IP_ADDR;
	pCMD[cmdLen++] = 0u;
	pCMD[cmdLen++] = 0u;
	pCMD[cmdLen++] = 0u;
	pCMD[cmdLen++] = 0u;
	//�����Ͷ˿ں�
	for(pos = 0u; pos < pIpAddr->addrLen; pos++)
	{
		if(':' == pIpAddr->addrData[pos])
		{
			break;
		}
	}
	pCMD[cmdLen++] = HT_IDH_PARAM_CH1 + ipCh;
	pCMD[cmdLen++] = HT_IDL_DOMAIN_NAME;
	pCMD[cmdLen++] = (INT8U)(pos >> 8u);
	pCMD[cmdLen++] = (INT8U)pos;
	for(port = 0u; port < pos; port++)
	{
		pCMD[cmdLen++] = pIpAddr->addrData[port];
	}
	pos++;
	port = 0u;
	for(; pos < pIpAddr->addrLen; pos++)
	{
		if((pIpAddr->addrData[pos] >= '0') && (pIpAddr->addrData[pos] <= '9'))
		{
			port *= 10u;
			port += (pIpAddr->addrData[pos] - '0');
		}
		else
		{
			port = 0u;
			break;
		}
	}
	pCMD[cmdLen++] = HT_IDH_PARAM_CH1 + ipCh;
	pCMD[cmdLen++] = HT_IDL_IP_PORT;
	pCMD[cmdLen++] = (INT8U)(HT_LEN_IP_PORT >> 8u);
	pCMD[cmdLen++] = (INT8U)HT_LEN_IP_PORT;
	pCMD[cmdLen++] = (INT8U)port;
	pCMD[cmdLen++] = (INT8U)(port >> 8u);
	//��β
	pCMD[cmdLen++] = 0u;
	pCMD[cmdLen++] = HT_PACKET_END;
	pCMD[cmdLen++] = HT_PACKET_END;
	pCMD[cmdLen++] = HT_PACKET_END;
	//����
	pCMD[posLen++] = (INT8U)(cmdLen >> 8u);
	pCMD[posLen++] = (INT8U)cmdLen;
	
	return cmdLen;
}

/*��ѯ�ź�ǿ��
**
*/
static INT8U CMD_QueryCSQ(INT8U *pCMD)
{
	INT8U cmdLen = 0u, posLen;
	
	if((INT8U *)0u == pCMD)
	{
		return 0u;
	}
	
	//��ͷ
	pCMD[cmdLen++] = HT_PACKET_HEAD;
	pCMD[cmdLen++] = HT_PACKET_HEAD;
	pCMD[cmdLen++] = HT_PACKET_HEAD;
	//����
	posLen = cmdLen;
	pCMD[cmdLen++] = 0u;
	pCMD[cmdLen++] = 0u;
	//������
	pCMD[cmdLen++] = HT_AFNU_QUERY_STA;
	pCMD[cmdLen++] = 0u;
	//������
	//�ź�ǿ��
	pCMD[cmdLen++] = HT_IDH_PARAM_STA;
	pCMD[cmdLen++] = HT_IDL_CSQ_VALUE;
	//��β
	pCMD[cmdLen++] = 0u;
	pCMD[cmdLen++] = HT_PACKET_END;
	pCMD[cmdLen++] = HT_PACKET_END;
	pCMD[cmdLen++] = HT_PACKET_END;
	//����
	pCMD[posLen++] = (INT8U)(cmdLen >> 8u);
	pCMD[posLen++] = (INT8U)cmdLen;
	
	return cmdLen;
}

/*��ѯģ��״̬
**
*/
static INT8U CMD_ModuleSta(INT8U *pCMD)
{
	INT8U cmdLen = 0u, posLen;
	
	if((INT8U *)0u == pCMD)
	{
		return 0u;
	}
	
	//��ͷ
	pCMD[cmdLen++] = HT_PACKET_HEAD;
	pCMD[cmdLen++] = HT_PACKET_HEAD;
	pCMD[cmdLen++] = HT_PACKET_HEAD;
	//����
	posLen = cmdLen;
	pCMD[cmdLen++] = 0u;
	pCMD[cmdLen++] = 0u;
	//������
	pCMD[cmdLen++] = HT_AFNU_QUERY_STA;
	pCMD[cmdLen++] = 0u;
	//������
	//ģ��״̬
	pCMD[cmdLen++] = HT_IDH_PARAM_STA;
	pCMD[cmdLen++] = HT_IDL_MODULE_STA;
	//��β
	pCMD[cmdLen++] = 0u;
	pCMD[cmdLen++] = HT_PACKET_END;
	pCMD[cmdLen++] = HT_PACKET_END;
	pCMD[cmdLen++] = HT_PACKET_END;
	//����
	pCMD[posLen++] = (INT8U)(cmdLen >> 8u);
	pCMD[posLen++] = (INT8U)cmdLen;
	
	return cmdLen;
}

/*��λ
**
*/
static INT8U CMD_Reset(INT8U *pCMD)
{
	INT8U cmdLen = 0u, posLen;
	
	if((INT8U *)0u == pCMD)
	{
		return 0u;
	}
	
	//��ͷ
	pCMD[cmdLen++] = HT_PACKET_HEAD;
	pCMD[cmdLen++] = HT_PACKET_HEAD;
	pCMD[cmdLen++] = HT_PACKET_HEAD;
	//����
	posLen = cmdLen;
	pCMD[cmdLen++] = 0u;
	pCMD[cmdLen++] = 0u;
	//������
	pCMD[cmdLen++] = HT_AFNU_RESET;
	pCMD[cmdLen++] = 0u;
	//��β
	pCMD[cmdLen++] = 0u;
	pCMD[cmdLen++] = HT_PACKET_END;
	pCMD[cmdLen++] = HT_PACKET_END;
	pCMD[cmdLen++] = HT_PACKET_END;
	//����
	pCMD[posLen++] = (INT8U)(cmdLen >> 8u);
	pCMD[posLen++] = (INT8U)cmdLen;
	
	return cmdLen;
}



/*ִ��һ��ָ��
**
*/
static INT8U AT_CmdExcute(INT8U const *pCMD, INT16U cmdLen, OS_FLAGS yes, OS_FLAGS no, INT8U errTimes, INT8U timeOutTimes, INT32U timeOut, INT32U waitTime)
{
	INT8U		err;
	OS_FLAGS	flagsRdy;
	
	//������֤
	if((INT8U *)0u == pCMD)
	{
		return OS_FALSE;
	}
	//ִ��
	while(OS_TRUE)
	{
		//����¼���־λ
		OSFlagPost(m_pFlagGrpDtu, yes + no, OS_FLAG_CLR, &err);
		while(OS_ERR_NONE != err);
		//����ָ��
		SendToModule(pCMD, cmdLen);
		//�ж��¼���־λ
		flagsRdy = OSFlagPend(m_pFlagGrpDtu, yes + no, OS_FLAG_CONSUME + OS_FLAG_WAIT_SET_ANY, timeOut * OS_TICKS_PER_SEC, &err);
		if(flagsRdy & yes)
		{
			return OS_TRUE;
		}
		else if(flagsRdy & no)
		{
			if(errTimes)
			{
				errTimes--;
			}
			if(errTimes)
			{
				OSTimeDly(waitTime * OS_TICKS_PER_SEC);
			}
			else
			{
				return OS_FALSE;
			}
		}
		else
		{
			if(timeOutTimes)
			{
				timeOutTimes--;
			}
			if(0u == timeOutTimes)
			{
				return OS_FALSE;
			}
		}
	}
}

/*�Ͽ�IPͨ��
**
*/
static INT8U AT_CloseIP(INT8U ipCh)
{
	//������֤
	if(ipCh >= DTU_NUM_IP_CH)
	{
		return OS_FALSE;
	}
	
	return OS_TRUE;
}

/*����IPͨ��
**
*/
static INT8U AT_ConnIP(INT8U ipCh)
{
	//������֤
	if(ipCh >= DTU_NUM_IP_CH)
	{
		return OS_FALSE;
	}
	//����IP��ַ
	DtuParamPend();
	m_atSendLen = CMD_ConnIP(m_atSend, ipCh, &m_dtuIpAddr[ipCh], (m_dtuIpType & (1u << ipCh)) ? OS_TRUE : OS_FALSE);
	DtuParamPost();
	if(OS_FALSE == AT_CmdExcute(m_atSend, m_atSendLen, DTU_FLAG_SET_OK, DTU_FLAG_SET_ERROR, 3u, 3u, DTU_TIME_SET_PARAM, 2u))
	{
		return OS_FALSE;
	}
	//�������
	m_atSendLen = CMD_SaveParam(m_atSend);
	if(OS_FALSE == AT_CmdExcute(m_atSend, m_atSendLen, DTU_FLAG_SAVE_OK, DTU_FLAG_SAVE_ERROR, 3u, 3u, DTU_TIME_SAVE_PARAM, 2u))
	{
		return OS_FALSE;
	}
	//��λ
	m_atSendLen = CMD_Reset(m_atSend);
	if(OS_FALSE == AT_CmdExcute(m_atSend, m_atSendLen, DTU_FLAG_RESET_OK, DTU_FLAG_RESET_ERROR, 3u, 3u, DTU_TIME_RESET_START, 2u))
	{
		return OS_FALSE;
	}
	//��ʱ
	OSTimeDly(DTU_TIME_RESET_END * OS_TICKS_PER_SEC);
	
	return OS_TRUE;
}

/*��������
**
*/
static INT8U AT_SendIP(INT8U ipCh, INT8U const *pData, INT16U dataLen)
{
	INT16U lenTotal;
	
	//������֤
	if(ipCh >= DTU_NUM_IP_CH)
	{
		return OS_FALSE;
	}
	if((INT8U *)0u == pData)
	{
		return OS_FALSE;
	}
	if(0u == dataLen)
	{
		return OS_FALSE;
	}
	
	//�ܳ���
	lenTotal = dataLen + HT_BYTES_HEAD + HT_BYTES_LEN + HT_BYTES_AFN + HT_BYTES_CRC + HT_BYTES_END;
	//��ͷ
	m_atSendLen = 0u;
	m_atSend[m_atSendLen++] = HT_PACKET_HEAD;
	m_atSend[m_atSendLen++] = HT_PACKET_HEAD;
	m_atSend[m_atSendLen++] = HT_PACKET_HEAD;
	m_atSend[m_atSendLen++] = (INT8U)(lenTotal >> 8u);
	m_atSend[m_atSendLen++] = (INT8U)lenTotal;
	m_atSend[m_atSendLen++] = HT_AFNU_SEND_GPRS;
	m_atSend[m_atSendLen++] = ipCh + 1u;
	SendToModule(m_atSend, m_atSendLen);
	//����
	SendToModule(pData, dataLen);
	//��β
	m_atSendLen = 0u;
	m_atSend[m_atSendLen++] = 0u;
	m_atSend[m_atSendLen++] = HT_PACKET_END;
	m_atSend[m_atSendLen++] = HT_PACKET_END;
	m_atSend[m_atSendLen++] = HT_PACKET_END;
	SendToModule(m_atSend, m_atSendLen);
	
	return OS_TRUE;
}

/*��������
**
*/
static INT8U AT_SendSMS(INT8U smsCh, INT8U const *pData, INT16U dataLen)
{
	INT8U			err;
	INT16U			lenTotal;
	OS_FLAGS		flagsRdy;
	DTU_SMS_ADDR	*pSmsAddr;
	
	//������֤
	if((INT8U *)0u == pData)
	{
		return OS_FALSE;
	}
	if(0u == dataLen)
	{
		return OS_FALSE;
	}
	
	//����¼���־λ
	OSFlagPost(m_pFlagGrpDtu, DTU_FLAG_SMS_OK + DTU_FLAG_SMS_ERROR, OS_FLAG_CLR, &err);
	while(OS_ERR_NONE != err);
	//�ܳ���
	lenTotal = dataLen + HT_BYTES_HEAD + HT_BYTES_LEN + HT_BYTES_AFN + HT_BYTES_CRC + HT_BYTES_END + HT_BYTES_SMS_ADDR;
	//��ͷ
	m_atSendLen = 0u;
	m_atSend[m_atSendLen++] = HT_PACKET_HEAD;
	m_atSend[m_atSendLen++] = HT_PACKET_HEAD;
	m_atSend[m_atSendLen++] = HT_PACKET_HEAD;
	m_atSend[m_atSendLen++] = (INT8U)(lenTotal >> 8u);
	m_atSend[m_atSendLen++] = (INT8U)lenTotal;
	m_atSend[m_atSendLen++] = HT_AFNU_SEND_SMS;
	m_atSend[m_atSendLen++] = 0u;
	if(smsCh < DTU_NUM_SMS_CH)
	{
		pSmsAddr = &m_dtuSmsAddr[smsCh];
	}
	else
	{
		pSmsAddr = &m_dtuSuperPhone;
	}
	DtuParamPend();
	for(err = 0u; err < pSmsAddr->addrLen; err++)
	{
		m_atSend[m_atSendLen++] = pSmsAddr->addrData[err];
	}
	DtuParamPost();
	for(; err < HT_BYTES_SMS_ADDR; err++)
	{
		m_atSend[m_atSendLen++] = 0u;
	}
	SendToModule(m_atSend, m_atSendLen);
	//����
	SendToModule(pData, dataLen);
	//��β
	m_atSendLen = 0u;
	m_atSend[m_atSendLen++] = 0u;
	m_atSend[m_atSendLen++] = HT_PACKET_END;
	m_atSend[m_atSendLen++] = HT_PACKET_END;
	m_atSend[m_atSendLen++] = HT_PACKET_END;
	SendToModule(m_atSend, m_atSendLen);
	//�ж��¼���־λ
	flagsRdy = OSFlagPend(m_pFlagGrpDtu, DTU_FLAG_SMS_OK + DTU_FLAG_SMS_ERROR, OS_FLAG_CONSUME + OS_FLAG_WAIT_SET_ANY, DTU_TIME_SMS_SEND * OS_TICKS_PER_SEC, &err);
	if(flagsRdy & DTU_FLAG_SMS_OK)
	{
		return OS_TRUE;
	}
	else if(flagsRdy & DTU_FLAG_SMS_ERROR)
	{
		return OS_FALSE;
	}
	else
	{
		return OS_FALSE;
	}
}

/*���ų�ʼ��
**
*/
static INT8U AT_BootSMS(void)
{
	INT8U i = 0u;
	
	//���ò���
	while(OS_TRUE)
	{
		m_atSendLen = CMD_ParamSet(m_atSend, i++);
		if(0u == m_atSendLen)
		{
			break;
		}
		if(OS_FALSE == AT_CmdExcute(m_atSend, m_atSendLen, DTU_FLAG_SET_OK, DTU_FLAG_SET_ERROR, 3u, 3u, DTU_TIME_SET_PARAM, 2u))
		{
			return OS_FALSE;
		}
	}
	
	//�������
	m_atSendLen = CMD_SaveParam(m_atSend);
	if(OS_FALSE == AT_CmdExcute(m_atSend, m_atSendLen, DTU_FLAG_SAVE_OK, DTU_FLAG_SAVE_ERROR, 3u, 3u, DTU_TIME_SAVE_PARAM, 2u))
	{
		return OS_FALSE;
	}
	
	//��ʱ
	OSTimeDly(DTU_TIME_BOOT_SMS * OS_TICKS_PER_SEC);
	
	return OS_TRUE;
}

/*������ʼ��
**
*/
static INT8U AT_BootIP(void)
{
	INT8U sta;
	
	m_atSendLen = CMD_ModuleSta(m_atSend);
	sta = AT_CmdExcute(m_atSend, m_atSendLen, DTU_FLAG_MODULE_OK, DTU_FLAG_MODULE_ERROR, 3u, 3u, DTU_TIME_QUERY_STA, 2u);
	
	return sta;
}

/*�ź�ǿ�ȸ���
**
*/
static INT8U AT_CsqUpdate(void)
{
	INT8U sta;
	
	m_atSendLen = CMD_QueryCSQ(m_atSend);
	sta = AT_CmdExcute(m_atSend, m_atSendLen, DTU_FLAG_CSQ, DTU_FLAG_NONE_NO, 3u, 3u, DTU_TIME_QUERY_STA, 2u);
	
	return sta;
}

/*�ر�����
**
*/
static INT8U AT_CloseNET(void)
{
	return OS_TRUE;
}

/*�ҵ绰
**
*/
static INT8U AT_HangUp(void)
{
	return OS_TRUE;
}



/*Ĭ�ϵ��������������
**
*/
static INT16U HeartEncoder(INT8U *pData, INT8U ipCh, INT16U lenLimit)
{
	INT16U i = 0u;
	
	if((INT8U *)0u == pData)
	{
		return 0u;
	}
	
	if(ipCh >= DTU_NUM_IP_CH)
	{
		return 0u;
	}
	
	if(lenLimit < 10u)
	{
		return 0u;
	}
	
	pData[i++] = 'H';
	pData[i++] = 'T';
	pData[i++] = '7';
	pData[i++] = '7';
	pData[i++] = '1';
	pData[i++] = '0';
	pData[i++] = '-';
	pData[i++] = 'C';
	pData[i++] = 'H';
	pData[i++] = '0' + ipCh;
	
	return i;
}

/*SMS����ת��
**
*/
static void SmsDataConv(DTU_RECV_DATA *pRecvData)
{
	INT8U					i, valid;
	static DTU_SMS_ADDR		smsAddr;
	
	//��������ָ��Ϊ��
	if((DTU_RECV_DATA *)0u == pRecvData)
	{
		return;
	}
	//��SMS�ŵ�������������
	if(DTU_COMM_TYPE_SMS != pRecvData->commType)
	{
		return;
	}
	//���ݳ������ֻ���ɶ��ź���
	if(pRecvData->dataLen <= HT_BYTES_SMS_ADDR)
	{
		pRecvData->dataLen = 0u;
		return;
	}
	//��ȡ���ź���
	smsAddr.addrLen = 0u;
	for(i = 0u; i < HT_BYTES_SMS_ADDR; i++)
	{
		if(0u == pRecvData->pData[i])
		{
			break;
		}
		else if(smsAddr.addrLen < DTU_BYTES_SMS_ADDR)
		{
			smsAddr.addrData[smsAddr.addrLen++] = pRecvData->pData[i];
		}
		else
		{
			break;
		}
	}
	//�Ƚ϶��ź���
	valid = OS_FALSE;
	DtuParamPend();
	if(OS_TRUE == ComparePhone(&smsAddr, &m_dtuSuperPhone))
	{
		valid = OS_TRUE;
		pRecvData->ch = DTU_NUM_SMS_CH;
	}
	else
	{
		for(i = 0u; i < DTU_NUM_SMS_CH; i++)
		{
			if(m_dtuSmsCh & (1u << i))
			{
				if(OS_TRUE == ComparePhone(&smsAddr, &m_dtuSmsAddr[i]))
				{
					valid = OS_TRUE;
					pRecvData->ch = i;
					break;
				}
			}
		}
	}
	DtuParamPost();
	//���ݱȽϽ��������Ӧ�Ĳ���
	if(OS_FALSE == valid)
	{
		pRecvData->dataLen = 0u;
	}
	else
	{
		pRecvData->dataLen -= HT_BYTES_SMS_ADDR;
		pRecvData->pData += HT_BYTES_SMS_ADDR;
	}
}



/*DTUӦ�����
**
*/
void DtuRecvHandler(INT8U recvData)
{
	static INT8U			step = 0u, afnH, afnL;
	static INT16U			dataLen, recvCount;
	static DTU_RECV_DATA	*pRecvData;
	
	switch(step)
	{
	case 0u:
		if(HT_PACKET_HEAD == recvData)
		{
			step = 1u;
		}
		break;
	case 1u:
		if(HT_PACKET_HEAD == recvData)
		{
			step = 2u;
		}
		else
		{
			step = 0u;
		}
		break;
	case 2u:
		if(HT_PACKET_HEAD == recvData)
		{
			step = 3u;
		}
		else
		{
			step = 0u;
		}
		break;
	case 3u:
		dataLen = recvData;
		dataLen <<= 8u;
		step = 4u;
		break;
	case 4u:
		dataLen += recvData;
		step = 5u;
		break;
	case 5u:
		afnH = recvData;
		step = 6u;
		break;
	case 6u:
		afnL = recvData;
		switch(afnH)
		{
		//���ò���
		case HT_AFND_SET_PARAM:
			if((0u == afnL) && (HT_BYTES_BASIC_FRAME == dataLen))
			{
				OSFlagPost(m_pFlagGrpDtu, DTU_FLAG_SET_OK, OS_FLAG_SET, &step);
			}
			else
			{
				OSFlagPost(m_pFlagGrpDtu, DTU_FLAG_SET_ERROR, OS_FLAG_SET, &step);
			}
			step = 0u;
			break;
		//�������
		case HT_AFND_SAVE_PARAM:
			if((0u == afnL) && (HT_BYTES_BASIC_FRAME == dataLen))
			{
				OSFlagPost(m_pFlagGrpDtu, DTU_FLAG_SAVE_OK, OS_FLAG_SET, &step);
			}
			else
			{
				OSFlagPost(m_pFlagGrpDtu, DTU_FLAG_SAVE_ERROR, OS_FLAG_SET, &step);
			}
			step = 0u;
			break;
		//��λ
		case HT_AFND_RESET:
			if((0u == afnL) && (HT_BYTES_BASIC_FRAME == dataLen))
			{
				OSFlagPost(m_pFlagGrpDtu, DTU_FLAG_RESET_OK, OS_FLAG_SET, &step);
			}
			else
			{
				OSFlagPost(m_pFlagGrpDtu, DTU_FLAG_RESET_ERROR, OS_FLAG_SET, &step);
			}
			step = 0u;
			break;
		//����GPRS
		case HT_AFND_SEND_GPRS:
			if((afnL > 0u) && (afnL < 5u) && (dataLen > HT_BYTES_BASIC_FRAME))
			{
				afnL--;
				dataLen -= HT_BYTES_BASIC_FRAME;
				pRecvData = RecvDataReq(dataLen);
				if((DTU_RECV_DATA *)0u == pRecvData)
				{
					step = 0u;
				}
				else
				{
					pRecvData->commType		= DTU_COMM_TYPE_IP;
					pRecvData->ch			= afnL;
					pRecvData->dataLen		= dataLen;
					recvCount = 0u;
					step = 7u;
				}
			}
			else
			{
				step = 0u;
			}
			break;
		//����SMS
		case HT_AFND_SEND_SMS:
			if((0u == afnL) && (dataLen > HT_BYTES_BASIC_FRAME))
			{
				dataLen -= HT_BYTES_BASIC_FRAME;
				pRecvData = RecvDataReq(dataLen);
				if((DTU_RECV_DATA *)0u == pRecvData)
				{
					step = 0u;
				}
				else
				{
					pRecvData->commType		= DTU_COMM_TYPE_SMS;
					pRecvData->ch			= DTU_NUM_SMS_CH;
					pRecvData->dataLen		= dataLen;
					recvCount = 0u;
					step = 7u;
				}
			}
			else
			{
				step = 0u;
			}
			break;
		//��ѯ״̬
		case HT_AFND_QUERY_STA:
			if((0u == afnL) && (dataLen > HT_BYTES_BASIC_FRAME))
			{
				step = 8u;
			}
			else
			{
				step = 0u;
			}
			break;
		default:
			step = 0u;
			break;
		}
		break;
	//GPRS��SMS���ݽ���
	case 7u:
		pRecvData->pData[recvCount++] = recvData;
		if(recvCount >= pRecvData->dataLen)
		{
			if(OS_ERR_NONE != OSQPost(m_pQRecvData, (void *)pRecvData))
			{
				OSMemPut(pRecvData->pMem, (void *)pRecvData);
			}
			step = 0u;
		}
		break;
	case 8u:
		if(HT_IDH_PARAM_STA == recvData)
		{
			step = 9u;
		}
		else
		{
			step = 0u;
		}
		break;
	case 9u:
		afnL = recvData;
		step = 10u;
		break;
	case 10u:
		dataLen = recvData;
		dataLen <<= 8u;
		step = 11u;
		break;
	case 11u:
		dataLen += recvData;
		step = 12u;
		break;
	case 12u:
		switch(afnL)
		{
		//�ź�ǿ��
		case HT_IDL_CSQ_VALUE:
			if(1u == dataLen)
			{
				m_csqValue = recvData;
				OSFlagPost(m_pFlagGrpDtu, DTU_FLAG_CSQ, OS_FLAG_SET, &step);
			}
			step = 0u;
			break;
		//SMS����״̬
		case HT_IDL_SMS_SEND_STA:
			if(1u == dataLen)
			{
				if(1u == recvData)
				{
					OSFlagPost(m_pFlagGrpDtu, DTU_FLAG_SMS_OK, OS_FLAG_SET, &step);
				}
				else
				{
					OSFlagPost(m_pFlagGrpDtu, DTU_FLAG_SMS_ERROR, OS_FLAG_SET, &step);
				}
			}
			step = 0u;
			break;
		//ģ��״̬
		case HT_IDL_MODULE_STA:
			if(1u == dataLen)
			{
				if(1u == recvData)
				{
					OSFlagPost(m_pFlagGrpDtu, DTU_FLAG_MODULE_OK, OS_FLAG_SET, &step);
				}
				else
				{
					OSFlagPost(m_pFlagGrpDtu, DTU_FLAG_MODULE_ERROR, OS_FLAG_SET, &step);
				}
			}
			step = 0u;
			break;
		default:
			step = 0u;
			break;
		}
		break;
	}
}

