#include "stdio.h"
#include "drv_fyyp.h"



//8��byte���7��byte������ǰ8��byte��56bit�������7��byteҲ��56��bit
static rt_uint16_t _pdu_7bit_encoder(rt_uint8_t *pdst, rt_uint8_t const *psrc, rt_uint16_t src_len)
{
	rt_uint16_t	dst_len = 0, i = 0;
	rt_uint8_t	bit_num = 0;

	while(i < src_len)
	{
		pdst[dst_len] = psrc[i] >> bit_num;
		if((i + 1) < src_len)
		{
			pdst[dst_len] += (psrc[i + 1] << (7 - bit_num));
		}
		dst_len++;
		i++;
		bit_num++;
		if(7 == bit_num)
		{
			bit_num = 0;
			i++;
		}
	}

	return dst_len;
}

//7��byte���8��byte
static rt_uint16_t _pdu_7bit_decoder(rt_uint8_t *pdst, rt_uint8_t const *psrc, rt_uint16_t src_len)
{
	rt_uint16_t	dst_len = 0, i = 0;
	rt_uint8_t	bit_num = 0, fill_value = 0;

	while(i < src_len)
	{
		pdst[dst_len]	= psrc[i] << bit_num;
		pdst[dst_len]	+= fill_value;
		pdst[dst_len++]	&= 0x7f;
		fill_value = psrc[i++] >> (7 - bit_num);
		bit_num++;
		if(7 == bit_num)
		{
			bit_num = 0;
			pdst[dst_len++] = fill_value;
			fill_value = 0;
		}
	}

	return dst_len;
}

//psrc�������������λȫ��Ϊ0�Ͳ���7-bit���룬����Ͳ���8-bit����
static rt_uint16_t _pdu_encoder(rt_uint8_t *pdst, DTU_SMS_ADDR const *sms_addr_ptr, rt_uint8_t const *psrc, rt_uint8_t src_len)
{
	rt_uint16_t	dst_len = 0;
	rt_uint8_t	i, *pmem;

	if(0 == sms_addr_ptr->addr_len)
	{
		return 0;
	}
	if(0 == src_len)
	{
		return 0;
	}
	
	//ʹ�ñ������õĶ������ĺ���
	pdst[dst_len++] = '0';
	pdst[dst_len++] = '0';
	//�ļ�ͷ�ֽں���Ϣ���ͣ��ǹ̶���
	pdst[dst_len++] = '1';
	pdst[dst_len++] = '1';
	pdst[dst_len++] = '0';
	pdst[dst_len++] = '0';
	//���ŷ���ַ����
	dst_len += sprintf((char *)(pdst + dst_len), "%02X", sms_addr_ptr->addr_len);
	//���ŷ��ǹ��ڵ绰
	pdst[dst_len++] = '8';
	pdst[dst_len++] = '1';
	//���ŷ���ַ
	for(i = 0; i < sms_addr_ptr->addr_len;)
	{
		if((i + 1) < sms_addr_ptr->addr_len)
		{
			pdst[dst_len++] = sms_addr_ptr->addr_data[i + 1];
		}
		else
		{
			pdst[dst_len++] = 'F';
		}
		pdst[dst_len++] = sms_addr_ptr->addr_data[i];
		i += 2;
	}
	//00 Э���ʶ(TP-PID) ����ͨGSM ���ͣ��㵽�㷽ʽ
	pdst[dst_len++] = '0';
	pdst[dst_len++] = '0';
	//���뷽ʽ
	for(i = 0; i < src_len; i++)
	{
		if(psrc[i] & 0x80)
		{
			break;
		}
	}
	pdst[dst_len++] = '0';
	if(i < src_len)
	{
		//8bit
		pdst[dst_len++] = '4';
	}
	else
	{
		//7bit
		pdst[dst_len++] = '0';
	}
	//��ϢʱЧ��
	pdst[dst_len++] = '0';
	pdst[dst_len++] = '1';
	//���ų��ȣ����������ԭ�������ݵĳ��ȣ����Ǳ����ĳ���
	dst_len += sprintf((char *)(pdst + dst_len), "%02X", src_len);
	//��������
	if(i >= src_len)
	{
		pmem = (rt_uint8_t *)mempool_req(src_len, RT_WAITING_FOREVER);
		if((rt_uint8_t *)0 == pmem)
		{
			return 0;
		}
		src_len = _pdu_7bit_encoder(pmem, psrc, src_len);
		for(i = 0; i < src_len; i++)
		{
			dst_len += sprintf((char *)(pdst + dst_len), "%02X", pmem[i]);
		}
		rt_mp_free((void *)pmem);
	}
	else
	{
		for(i = 0; i < src_len; i++)
		{
			dst_len += sprintf((char *)(pdst + dst_len), "%02X", psrc[i]);
		}
	}
	
	return dst_len;
}

static rt_uint16_t _pdu_decoder(rt_uint8_t *pdst, DTU_SMS_ADDR *sms_addr_ptr, rt_uint8_t const *psrc, rt_uint16_t src_len)
{
	rt_uint16_t	i = 0, addr_len;
	rt_uint8_t	byte, tp_udl, *pmem;

	if(0 == src_len)
	{
		return 0;
	}

	//�������ĺ��볤��
	if((i + 2) > src_len)
	{
		return 0;
	}
	fyyp_str_to_array(&byte, psrc + i, 2);
	i += 2;
	addr_len = byte;
	addr_len *= 2;
	//�������ĺ�������
	if((i + addr_len) > src_len)
	{
		return 0;
	}
	i += addr_len;
	//��������
	if((i + 2) > src_len)
	{
		return 0;
	}
	i += 2;
	//�ظ���ַ����
	if((i + 2) > src_len)
	{
		return 0;
	}
	fyyp_str_to_array(&sms_addr_ptr->addr_len, psrc + i, 2);
	if((0 == sms_addr_ptr->addr_len) || (sms_addr_ptr->addr_len > DTU_BYTES_SMS_ADDR))
	{
		return 0;
	}
	i += 2;
	//��ַ��ʽ
	if((i + 2) > src_len)
	{
		return 0;
	}
	fyyp_str_to_array(&byte, psrc + i, 2);
	i += 2;
	addr_len = 0;
	if(0x91 == byte)
	{
		byte = sms_addr_ptr->addr_len;
		sms_addr_ptr->addr_len++;
		if(sms_addr_ptr->addr_len > DTU_BYTES_SMS_ADDR)
		{
			return 0;
		}
		sms_addr_ptr->addr_data[addr_len++] = '+';
	}
	else
	{
		byte = sms_addr_ptr->addr_len;
	}
	//��ַ����
	if(byte % 2)
	{
		byte++;
	}
	if((i + byte) > src_len)
	{
		return 0;
	}
	while(byte)
	{
		//������������жϣ��ɽ�����ŵ�ַ��F������
		if(addr_len < sms_addr_ptr->addr_len)
		{
			sms_addr_ptr->addr_data[addr_len++] = psrc[i + 1];
		}
		if(addr_len < sms_addr_ptr->addr_len)
		{
			sms_addr_ptr->addr_data[addr_len++] = psrc[i];
		}
		i += 2;
		byte -= 2;
	}
	//Э���ʶ
	if((i + 2) > src_len)
	{
		return 0;
	}
	i += 2;
	//���뷽ʽ
	if((i + 2) > src_len)
	{
		return 0;
	}
	fyyp_str_to_array(&byte, psrc + i, 2);
	i += 2;
	byte &= 0x0c;
	//ʱ���
	if((i + 14) > src_len)
	{
		return 0;
	}
	i += 14;
	//���ݳ���(����ʵ�ʳ��ȣ�������ǰ�ĳ��ȣ����Ǻ������ݵĳ��ȣ������Ǳ���������)
	if((i + 2) > src_len)
	{
		return 0;
	}
	fyyp_str_to_array(&tp_udl, psrc + i, 2);
	addr_len = tp_udl;
	i += 2;
	if(0 == byte)
	{
		//������7bit����ʱ��ÿ8���ֽڻ�����7���ֽڣ���Ϊ���Ǳ���ǰ��ʵ�ʳ��ȣ�������Ҫ����һ��
		addr_len -= (addr_len / 8);
	}
	addr_len *= 2;
	//����
	if((i + addr_len) != src_len)
	{
		return 0;
	}
	if(0 == byte)
	{
		pmem = (rt_uint8_t *)mempool_req(addr_len / 2, RT_WAITING_FOREVER);
		if((rt_uint8_t *)0 == pmem)
		{
			return 0;
		}
		addr_len = fyyp_str_to_array(pmem, psrc + i, addr_len);
		i = _pdu_7bit_decoder(pdst, pmem, addr_len);
		rt_mp_free((void *)pmem);
	}
	else
	{
		i = fyyp_str_to_array(pdst, psrc + i, addr_len);
	}

	return i;
}

