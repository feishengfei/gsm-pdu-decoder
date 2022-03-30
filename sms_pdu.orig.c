#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include "Pdu.h"
#include "sms_pdu.h"


// 2位16进制字符串 转为 十进制
unsigned int hex_to_int(const char *hex, int n)
{
	unsigned int a[10] = {0};
	int i;
	for (i = 0; i < n && hex + i != NULL; i++) {
		if (hex[i] >= '0' && hex[i] <= '9') {
			a[i] = hex[i] - '0';
		} else if (hex[i] >= 'A' && hex[i] <= 'F'){
			a[i] = hex[i] - 'A' + 0x0a;
		} else if (hex[i] >= 'a' && hex[i] <= 'f'){
			a[i] = hex[i] - 'a' + 0x0a;
		}
	}

	unsigned int rtl = 0, multy = 1;
	for (i = n - 1; i >= 0; i--) {
		rtl += a[i] * multy;
		multy *= 0x10;
	}
	return rtl;
}


/**
 * @brief  utf-8转unicode(UCS-2)
 * @note   暂不支持UCS-4 (4字节)编码
 * @param  src: 输入utf-8指针
 * @param  dst: 输出Unicode缓冲区
 * @param  dlen: Unicode缓冲区长度
 * @retval <0:错误，>=0:输出uinicode字节数
 */
int utf82unicode(const char* src, char* dst, int dlen)
{
	// 输出Unicode字节数
	int olen = 0;

	while (*src){
		// 预留2个字节，不足则错误
		if(olen > (dlen-2)){
			olen = -1;
			break;
		}
		if (*src > 0x00 && *src <= 0x7F) {	// 单字节UTF8字符（英文字母、数字）
			*dst++ = 0; //小端法表示，在高地址填补0
			*dst++ = *src++;
		}
		else if (((*src) & 0xE0) == 0xC0) {	// 双字节UTF8字符
			char high = *src++;
			char low = *src++;
			if ((low & 0xC0) != 0x80) {		// 检查是否为合法的UTF8字符
				return -1;
			}

			*dst++ = (high << 6) + (low & 0x3F);
			*dst++ = (high >> 2) & 0x07;
		}else if (((*src) & 0xF0) == 0xE0) {	// 三字节UTF8字符
			char high 	= *src++;
			char middle = *src++;
			char low 	= *src++;
			if (((middle & 0xC0) != 0x80) || ((low & 0xC0) != 0x80)){ //检测是否合法
				return -1;
			}

			*dst++ = (high << 4) + ((middle >> 2) & 0x0F); 	// 取出high的低四位与middle的中间四位，组合成unicode字符的高8位
			*dst++ = (middle << 6) + (low & 0x3F);			// 取出middle的低两位与low的低6位，组合成unicode字符的低8位
		}else {	// 对于其他字节数的UTF8字符不进行处理
			printf("[%s](%d) err\n",__func__,__LINE__);
			return -1;
		}
		olen +=2;
	}
	// unicode字符串后面，有两个\0
	if(olen > (dlen-2)){
		olen = -1;
	}else{
		*dst++ = 0;
		*dst++ = 0;
	}
	return olen;
}


static int is_ascci_fun(const unsigned char *cnt)
{
	int ret = 1, i;
	for (i = 0; cnt[i] != '\0'; i++) {
		if (cnt[i] > 0x07f) {
			ret = 0; break;
		}
	}
	printf("\nis ascii [ret:%d]\n", ret);
	return ret;
}


// SEND PDU format :
// SCA   PDU-Type MR DA    PID DCS VP     UDL UD
// 1-12  1        1  2-12  1   1   0,1,7  1   0-140
//
// https://github.com/hu55a1n1/Multi-part-SMS-PDU-generator/blob/master/docs/Short%20Message%20in%20PDU%20Encoding.pdf
int mk_send_pdu(const char *num, const char *cnt, unsigned char *pdu)
{
	int is_ascci = 0; // 标记是否是ASCII编码

	{ // arg 合法性判定
		if (!num || !cnt || !pdu) {
			printf("args err");
			return -1;
		}
		int i, len;
		len = strlen(num);
		for (i = 0; num[i] != '\0'; i++) {
			if (i <= 10 && i > 0 && (num[i] >= '0' || num[i] <= '9')) {
				continue;
			} else if (num[0] == '+' || num[0] >='0' && num[0] <= '9') {
				continue;
			} else {
				printf("num format err");
				return -1;
			}
		}
	}

	{ // SCA(使用默认设为00)  + PDU-Type(send 11 / recv 24) + MR(00 or 64)
		char *set = "001100";
		strncpy(pdu, set, strlen(set));
	}

	// DA 接收方号码 带+号 type 为81 否则为91
	// Lenghth + Type + Address
	{
		char da[30] = {0};
		int len = strlen(num);

		if (num[0] == '+') len--;
		da[0] = (len % 0x100) / 0x10 + '0';
		if (len % 0x10 >= 10) da[1] = len % 0x10 - 10 + 'A';
		else da[1] = len % 0x10 + '0';

		int i, j;
		if (num[0] == '+') da[2] = '9', j = 1;
		else da[2] = '8', j = 0;
		da[3] = '1';
		for (i = 4; num[j] != '\0'; i += 2, j += 2) {
			da[i + 1] = num[j];
			if (num[j + 1] == '\0') {
				da[i] = 'F';
					break;
			} else {
				da[i] = num[j + 1];
			}
		}
		strncat(pdu, da, strlen(da));
	}

	// PID 协议标识默认 00
	// + DCS 数据编码方案 00:7bit 08:UCS2  F6:8bit
	// + VP 信息有效期 AA:4天
	is_ascci = is_ascci_fun(cnt);
	if (is_ascci) {
		strcat(pdu, "00F6AA");
	} else {
		strcat(pdu, "0008AA");
	}

	// UDL 数据编码长度 8位数组的个数   7bit为编码前的数据字节数
	// +UD 用户数据
	char dat[400] = {0};
	if (!is_ascci) {
		char uni[200] = {0};
		int num = utf82unicode(cnt, uni, 200);
		if (num == -1) {
			printf("Error!\n");
		} else {
			if (num < 0xa0) dat[0] = (num % 0x100) / 0x10 + '0';
			else dat[0] = (num % 0x100) / 0x10 - 0x0a + 'A';

			if ((num % 0x100) % 0x10 < 0x0a) dat[1] = (num % 0x100) % 0x10 + '0';
			else dat[1] = (num % 0x100) % 0x10 - 0x0a + 'A';

			int i;
			unsigned char d_test = 0;
			for (i = 0; i < num; i++) {
				d_test = (unsigned char)uni[i];
				if (d_test < 0xa0) dat[i * 2 + 2] = d_test / 0x10 + '0';
				else dat[i * 2 + 2] = d_test / 0x10 - 0x0a + 'A';

				if (d_test % 0x10 < 0x0a) dat[i * 2 + 3] = d_test % 0x10 + '0';
				else dat[i * 2 + 3] = d_test % 0x10 - 0x0a + 'A';
			}
			strcat(pdu, dat);
		}
	} else {
		int num = strlen(cnt);
		if (num < 0xa0) dat[0] = (num % 0x100) / 0x10 + '0';
		else dat[0] = (num % 0x100) / 0x10 - 0x0a + 'A';

		if ((num % 0x100) % 0x10 < 0x0a) dat[1] = (num % 0x100) % 0x10 + '0';
		else dat[1] = (num % 0x100) % 0x10 - 0x0a + 'A';
		int i;
		unsigned char d_test = 0;
		for (i = 0; i < num; i++) {
			d_test = (unsigned char)cnt[i];
			if (d_test < 0xa0) dat[i * 2 + 2] = d_test / 0x10 + '0';
			else dat[i * 2 + 2] = d_test / 0x10 - 0x0a + 'A';

			if (d_test % 0x10 < 0x0a) dat[i * 2 + 3] = d_test % 0x10 + '0';
			else dat[i * 2 + 3] = d_test % 0x10 - 0x0a + 'A';
		}
		strcat(pdu, dat);
	}
	return strlen(pdu);
}


/*=========================================================================
FUNCTION
  wms_ts_unpack_gw_7_bit_chars

DESCRIPTION
  Unpack raw data to GSM 7-bit characters.

DEPENDENCIES
  None

RETURN VALUE
  Number of bytes decoded.

SIDE EFFECTS
  None
=========================================================================*/
unsigned char wms_ts_unpack_gw_7_bit_chars
(
  const unsigned char       * in,
  unsigned char             in_len,        /* Number of 7-bit characters */
  unsigned char             out_len_max,   /* Number of maximum 7-bit characters after unpacking */
  unsigned short            shift,
  unsigned char             * out
)
{
    int      i=0;
    unsigned short  pos=0;

    if (in == NULL || out == NULL)
    {
        printf("null pointer in wms_ts_unpack_gw_7_bit_chars\n");
        return 0;
    }
#if 0
  /*If the number of fill bits != 0, then it would cause an additional shift*/
  if (shift != 0)
   pos = pos+1;

  if (shift ==7)
  {
    out[0] = in[0] >> 1; /*7 fillbits*/
    shift =0;            /*First Byte is a special case*/
    i=1;
  }
#endif
    int j = 0;
    int current_value = 0;
    int previous_value = 0;
    while(i < in_len && j < out_len_max)
    {
        shift = j & 0x07;
        current_value = in[i++];
        out[j++] =
           ((current_value << shift) | (previous_value >> (8 - shift))) & 0x7F;

       if (shift == 6)
       {
           if ((current_value >> 1) == 0x0D && i == in_len)
           {
               break;
           }

           out[j++] = current_value >> 1;
       }

       previous_value = current_value;

    }
    return 0;

} /* wms_ts_unpack_gw_7_bit_chars() */


static int GSM7Bit_convert(unsigned char *dest, int len)
{
    printf("++++++++in len:%d\n",len);
	char *tmp_rslt = NULL;
	if (dest == NULL || len <=0) {
		return 0;
	}
	tmp_rslt = calloc(3, len);
	if (tmp_rslt == NULL) {
		return 0;
	}
    const unsigned short def_alpha_to_ascii_table[] =
    {
	  '@',  /*'£'*/0xa3c2,  '$',  /*'¥'*/0xa5c2,  /*'è'*/0xa8c3,  /*'é'*/0xa9c3,  /*'ù'*/0xb9c3,
      /*'ì'*/0xacc3,  /*'ò'*/0xb2c3,  /*'Ç'*/0x87c3,  0x0A,  /*'Ø'*/0x98c3,  /*'ø'*/0xb8c3, 0x0D,  /*'Å'*/0x85c3,  /*'å'*/0xa5c3,
      /*'Δ'*/0x94ce,  '_',  /*'Φ'*/0xa6ce,  /*'Γ'*/0x93ce,  /*'Λ'*/0x9bce,  /*'Ω'*/0xa9ce,  /*'Π'*/0xa0ce,  /*'Ψ'*/0xa8ce,
      /*'Σ'*/0xa3ce,  /*'Θ'*/0x98ce,  /*'Ξ'*/0x9ece,  0x1B,  /*'Æ'*/0x86c3,  /*'æ'*/0xa6c3,  /*'β'*/0xb2ce,  /*'É'*/0x89c3,
      ' ',  '!', 0x22,  '#',  /*'¤'*/0xa4c2,  '%',  '&', 0x27,  '(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
      '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
      /*'¡'*/0xa1c2,  'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
      'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',  /*'Ä'*/0x84c3,  /*'Ö'*/0x96c3,  /*'Ñ'*/0x91c3,  /*'Ü'*/0x9cc3,  /*'§'*/0xa7c2,
      /*'¿'*/0xbfc2,  'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
      'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  /*'ä'*/0xa4c3,  /*'ö'*/0xb6c3,  /*'ñ'*/0xb1c3,  /*'ü'*/0xbcc3,  /*'à'*/0xa0c3
    };


    // 0x1b --> 转义
    const unsigned int def_alpha_to_ascii_table2[] =
    {
      0x14, '^', 0x28, '{', 0x29, '}',  0x2f, '\\', 0x3c, '[', 0x3d, '~', 0x3e, ']', 0x40, '|', 0x65, /*'€'*/0xac82e2,
    };

    int i, j, k;
    for (i = 0, k = 0; i < len; i++) {
        if (dest[i] != 0x1b) {
            tmp_rslt[k++] = def_alpha_to_ascii_table[(dest[i])] % 0x100;
			if (def_alpha_to_ascii_table[(dest[i])] > 0xff) {
				tmp_rslt[k++] = def_alpha_to_ascii_table[(dest[i])] / 0x100;
			}
        } else {
            i++;
            for (j = 0; j < sizeof(def_alpha_to_ascii_table2)/sizeof(def_alpha_to_ascii_table2[0]); j += 2) {
                if (dest[i] == def_alpha_to_ascii_table2[j]) {
                    tmp_rslt[k++] = def_alpha_to_ascii_table2[j + 1] % 0x100;
					if (dest[i] == 0x65) {
						tmp_rslt[k++] = def_alpha_to_ascii_table2[j + 1] / 0x100 % 0x100;
						tmp_rslt[k++] = def_alpha_to_ascii_table2[j + 1] / 0x10000 % 0x100;
					}
                    break;
                }
            }
        }
    }
	tmp_rslt[k] = '\0';
	memset(dest, 0, len);
	memcpy(dest, tmp_rslt, k + 1);
	free(tmp_rslt);

    return k + 1;
}

/**
 * @brief  常见unicode(UCS-2)转utf-8
 * @note   暂不支持UCS-4 (4字节)编码
 * @param  *src: 输入unicode指针(被转换成unsigned short类型进行识别)
 * @param  slen: 输出unicode字节数
 * @param  *dst: 输出utf-8缓冲区
 * @param  dlen: utf-8缓冲区长度
 * @retval <0:错误，>=0:输出utf-8字节数
 */
int unicode2utf8(const unsigned char *src, int slen, unsigned char *dst, int dlen)
{
	int olen = 0;
	if(!src || !dst || !dlen)
		return -1;

	// 长度必须为2的倍数，需转换成unsigned short类型
	if(slen%2)
		return -2;

	unsigned short *p = (unsigned short *)src;
	unsigned short unicode=0;
	int len=slen/2;
	while(len--)
	{
		unicode = *p++;
		// printf("[%s](%d) unicode=%x\n",__func__,__LINE__,unicode);
		if(unicode == 0)
			break;
		// 预留3个字节，不足则错误
		if(olen > (dlen-3)){
			olen = -3;
			break;
		}

		if ( unicode <= 0x0000007F ){	//1个字节
			// U-00000000 - U-0000007F:  0xxxxxxx
			*dst++	= (unicode & 0x7F);
			olen += 1;
		}else if ( unicode >= 0x00000080 && unicode <= 0x000007FF ){		//两个字节
			// U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
			*dst++	= ((unicode >> 6) & 0x1F) | 0xC0;
			*dst++ 	= (unicode & 0x3F) | 0x80;
			olen += 2;
		}else if ( unicode >= 0x00000800 && unicode <= 0x0000FFFF ){			//3个字节
			// U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
			*dst++	= ((unicode >> 12) & 0x0F) | 0xE0;
			*dst++	= ((unicode >>  6) & 0x3F) | 0x80;
			*dst++	= (unicode & 0x3F) | 0x80;
			olen += 3;
		}else{
			return -4;
		}
	};

    return olen;
}


// 解析接收短信的内容
// 格式：SAC + PDU-Type + OA + PID + DCS + SCTS + UDL + UD
//       1-12  1          2-12 1     1     7      1     0-140
int mk_recv_pdu(const char *pdu, st_sms_info_t *sms_cb_info)
{
	const char *ptr = pdu;
	unsigned char code_type = 0; // 0:7bite 8:UCS-2
	unsigned int udl = 0; // user date length
	sms_cb_info->long_info.is_long_msg = 0;
	sms_cb_info->tag_type = 0;
	sms_cb_info->storage_type = -1;
	// SAC ：短信中心号码 （可跳过）
	{
		if (pdu[1] == '0') {
			ptr += 2;
		} else if (pdu[1] <= '9') {
			ptr += (pdu[1] - '0') * 2 + 2;
		} else {
			ptr += (pdu[1] - 'A' + 0x0a) * 2 + 2;
		}
	}

	// 根据PDU-Type 判断是否是长短信
	unsigned char is_long_sms = 0;
	{
		unsigned char t = (unsigned char)(*ptr);
		if (t >= 'A') t = t -'A' + 0x0a;
		else t = t - '0';

		if (t & 0x04) is_long_sms = 1;
		ptr += 2;
	}

	// OA : Length + Type(国内A1/81 国外91) + 号码
	{
		char oa_num[30] = {0};
		int len, i,j = 0;
		len = hex_to_int(ptr, 2);
		if (len % 2) len++;
		len /= 2;
		ptr += 2; // Skip Lenghth
		if (*ptr == '9') {
			oa_num[0] = '+';
			j = 1;
		}
		ptr += 2; // Skip Type
		for (i = 0; i < len; i++) {
			oa_num[j + i * 2] = ptr[1];
			oa_num[j + i * 2 + 1] = ptr[0];
			ptr += 2;
		}
		if (oa_num[j + i * 2 - 1] == 'F') oa_num[j + i * 2 - 1] = '\0';
//		printf("oa_num : %s\n", oa_num);
		strcpy(sms_cb_info->str_PhoneNumber, oa_num);
	}

	// PID(跳过) + DCS 数据编码方案 00:7bit  08:UCS-2
	{
		ptr += 2;
		code_type = hex_to_int(ptr, 2);
		ptr += 2;
	}

	{ // SCTS 时间日期
		int i;
		char date[20] = {0};
		for (i = 0; i < 7; i++) {
			date[i * 3] = ptr[1];
			date[i * 3 + 1] = ptr[0];
			ptr += 2;
			if (i < 2) date[i * 3 + 2] = '/';
			else if (i == 2) date[i * 3 + 2] = ',';
			else if (i < 5) date[i * 3 + 2] = ':';
			else date[i * 3 + 2] = '+';
		}
		date[18] = '8';
		date[19] = '\0';
//		puts(date);
		strcpy(sms_cb_info->time_stamp, date);
	}

	{ // UDL
		udl = hex_to_int(ptr, 2);
		ptr += 2;
	}

	if (is_long_sms) { // UDH: long sms header
		int udhl = hex_to_int(ptr, 2);
		int id, sum, order;
		if (udhl == 5) {
			id = hex_to_int(ptr + 6, 2);
			ptr += 8;
		} else if (udhl == 6) {
			id = hex_to_int(ptr + 6, 4);
			ptr += 10;
		} else {
			printf("err pdu data\n");
			return -1;
		}
		sum = hex_to_int(ptr, 2);
		order = hex_to_int(ptr + 2, 2);
		ptr += 4;
		udl -= udhl + 1;
		if (udl % 2) {
			printf("err pdu data\n");
			return -1;
		}
//		printf("[long sms] id = %d  sum = %d  order = %d\n", id, sum, order);

		sms_cb_info->long_info.is_long_msg = 1;
		sms_cb_info->long_info.msgid = id;
		sms_cb_info->long_info.sum_items = sum;
		sms_cb_info->long_info.index = order;
	}

	{ // SM short message
		char sm[360] = {0};
		int i;
		if (code_type == 0) {
			printf("7bit decode\n");
			unsigned char gsm7[200] = {0};
			if (strlen(ptr) % 2 != 0) {
				// printf("err pdu data 552");
				return -1;
			}
			for (i = 0; i < strlen(ptr) / 2; i++) {
				gsm7[i] = hex_to_int(ptr + i * 2, 2);
			}
            int tmp = (((udl * 7 - 1) / 8)+1)+1;
            //printf("tmp: %d\n", tmp);
			wms_ts_unpack_gw_7_bit_chars(gsm7, tmp, udl, 0, sm);
			GSM7Bit_convert(sm, udl);
			puts(sm);
		} else if (code_type == 8) {
			unsigned char ucd[160] = {0};
			for (i = 0; i < udl; i += 2) {
				ucd[i + 1] = (unsigned char)hex_to_int(ptr, 2);
				ptr += 2;
				ucd[i] = (unsigned char)hex_to_int(ptr, 2);
				ptr += 2;
				// printf("%02hhx %02hhx ", ucd[i + 1], ucd[i]);
			}
			unicode2utf8(ucd, udl, sm, 360 - 1);
			//printf("\nUCS-2 decode %d: %s\n", udl, sm);
		} else {
			//printf("err pdu data\n");
			return -1;
		}
		strcpy(sms_cb_info->message_content, sm);
	}
}
