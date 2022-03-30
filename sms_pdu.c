#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "SDL_iconv.h"
#include "sms_pdu.h"
#include "Pdu.h"

#define min(a,b) ((a) < (b) ? (a) : (b))

// SMS-SUBMIT
//
// SCA   PDU-Type MR DA    PID DCS VP     UDL UD
// 1-12  1        1  2-12  1   1   0,1,7  1   0-140
//
// https://github.com/hu55a1n1/Multi-part-SMS-PDU-generator/blob/master/docs/Short%20Message%20in%20PDU%20Encoding.pdf
int mk_send_pdu(const char *num, const char *cnt, char **pdu)
{
	int i, j, len;
	char *cnt_octet_p = NULL;
	size_t cnt_octet_len = 0;
	size_t pdu_len = 0;			/* total multi-seg pdu length, '\n' as delim */
	size_t pdu_seg_len = 0;		/* pdu segment length, 160 for 7bit, 140 for 8/16bit */
	int pdu_total = 0;			/* total generated pdu segment count */
	char pdu_buffer[2*SMS_PDU_MAX_LEN+1] = {0};
	PDU_DESC pdu_enc, *pPduEncodeDesc = &pdu_enc;

	//init
	memset(pPduEncodeDesc, 0, sizeof(PDU_DESC));

	//validation
	if (!num || !cnt || !pdu) {
		printf("invalid args\n");
		return -1;
	}


	//alloc 4 times space, prepare for utf-8 -> UCS2 conversion
	//although abundant for 7 bit
	cnt_octet_len = strlen(cnt) * UTF8_CHAR_LEN;
	cnt_octet_p = (char *)calloc(strlen(cnt), UTF8_CHAR_LEN);
	if(NULL == cnt_octet_p) {
		printf("cnt_octet_p calloc err:(%d):%s\n", errno, strerror(errno));
		return -1;
	}

	//validate num
	for (i = 0; i < strlen(num); i++) {
		if( isdigit(num[i]) ) {
			if( 0 == i )
				pPduEncodeDesc->phoneTypeOfAddr = NUM_TYPE_NATIONAL;
		}
		else if( 0 == i &&  '+' == num[i]){
			pPduEncodeDesc->phoneTypeOfAddr = NUM_TYPE_INTERNATIONAL;
		}
		else {
			printf("invalid number\n");
			if(NULL!=cnt_octet_p)free(cnt_octet_p);
			return -1;
		}
	}

	//fill destination phone number
	for (i = 0, j = 0; i < strlen(num); i++)
		if( isdigit(num[i]) ) {
			pPduEncodeDesc->phoneAddr[j] = num[i];
			pPduEncodeDesc->phoneAddrLen += 1;
			j++;
		}
	for ( ; j < ADDR_OCTET_MAX_LEN + 1; j++)
		pPduEncodeDesc->phoneAddr[j] = '\0';


#ifndef DISABLE_VPF
	pPduEncodeDesc->vldtPrdFrmt = VLDTY_PERIOD_RELATIVE;
	/*
	 * VP value			Validity period value
	 * 0 to 143			(VP + 1) x 5 minutes (i.e. 5 minutes intervals up to 12 hours)
	 * 144 to 167		12 hours + ((VP -143) x 30 minutes)
	 * 168 to 196		(VP - 166) x 1 day
	 * 197 to 255		(VP - 192) x 1 week
	 *
	 * Example: 0xAA: 170 in decimal: (170-166) x 1 day= 4 days
	 */
	pPduEncodeDesc->vldtPrd = 0xAA;
#endif

#ifdef SMS_SUBMIT_STATUS_REPORT
	pPduEncodeDesc->isDeliveryReq = 1;
#endif

	// User Data
	// default as 7BIT
	pPduEncodeDesc->usrDataFormat = ANSI_8BIT;
	for (i = 0; i < strlen(cnt); i++) {
		if(!isascii(cnt[i])) {
			pPduEncodeDesc->usrDataFormat = UCS2_16BIT;
			break;
		}
	}

	//calculate how much space need to hold encoded pdu
	if (ANSI_8BIT == pPduEncodeDesc->usrDataFormat)
	{
		//fill cnt_octet_p with cnt
		memcpy(cnt_octet_p, cnt, strlen(cnt));
		cnt_octet_len = strlen(cnt);

		pdu_total = 1 + cnt_octet_len/(SMS_GSM7BIT_MAX_LEN+1);
		pdu_seg_len = SMS_PDU_USER_DATA_MAX_LEN;

		/* alloc for output pdu
		 * each 8bit HEX take 2 bytes for ascii repr,
		 * (pdu_total -1) for '\n' delimiter
		 */
		pdu_len = pdu_total * (2*SMS_PDU_MAX_LEN) + (pdu_total - 1);
		*pdu = (char *)malloc(pdu_len);
	}
	else {
		//change into ucsii and fill into cnt_octet_p
		SDL_iconv_t cd = SDL_iconv_open("UCS2", "UTF-8");
		const char *  in_buf = cnt;
		size_t  in_len = strlen(cnt);
		char * out_buf = cnt_octet_p;
		size_t out_len = cnt_octet_len;
		len = SDL_iconv(cd, &in_buf, &in_len, &out_buf, &out_len);
		SDL_iconv_close(cd);
		if(len < 0) {
			printf("iconv err:%d\n", len);
			if(NULL!=cnt_octet_p)free(cnt_octet_p);
			return -1;
		}
		//iconv return total convert word;
		//each word occupy 4 bytes
		cnt_octet_len = 2 * len;
		//hex_dump("ucs2 cnt", cnt_octet_p, cnt_octet_len);

		pdu_total = 1 + cnt_octet_len/(SMS_PDU_USER_DATA_MAX_LEN+1);
		pdu_seg_len = SMS_PDU_USER_DATA_MAX_LEN;

		/* alloc for output pdu
		 * each 8bit HEX take 2 bytes for ascii repr,
		 * (pdu_total -1) for '\n' delimiter
		 */
		pdu_len = pdu_total * (2*SMS_PDU_MAX_LEN) + (pdu_total - 1);
		*pdu = (char *)malloc(pdu_len);
	}

	//check pdu
	if(NULL == *pdu) {
		printf("pdu malloc err:(%d):%s\n", errno, strerror(errno));
		if(NULL!=cnt_octet_p)free(cnt_octet_p);
		return -1;
	}
	//init pdu
	memset(*pdu	, 0, pdu_len);

	if(pdu_total > 1) {
		pPduEncodeDesc->isConcatenatedMsg = TRUE;
		pPduEncodeDesc->concateTotalParts = pdu_total;
	}

	//iter fill usrData and do encode
	for(len=0, i=0; len < cnt_octet_len;i++) {
		//clear buffer
		memset(pdu_buffer, 0, sizeof(pdu_buffer));
		memset(pPduEncodeDesc->usrData, 0, sizeof(pPduEncodeDesc->usrData));

		//delimiter
		if(len)strcat(*pdu, "\n");

		//usrData
		memcpy(pPduEncodeDesc->usrData, cnt_octet_p+len,
			min(pdu_seg_len, cnt_octet_len-len));
		pPduEncodeDesc->usrDataLen = min(pdu_seg_len, cnt_octet_len-len);
		pPduEncodeDesc->concateCurntPart = i+1;

		//iter
		len += min(pdu_seg_len, cnt_octet_len-len);

		//encoding
		EncodePduData(pPduEncodeDesc, pdu_buffer);

		//concatenate
		strcat(*pdu, (const char *)pdu_buffer);
	}

	if(NULL!=cnt_octet_p)free(cnt_octet_p);
	return strlen(*pdu);
}

// SMS-DELIVER
// SAC + PDU-Type + OA + PID + DCS + SCTS + UDL + UD
// 1-12  1          2-12 1     1     7      1     0-140
int mk_recv_pdu(const char *pdu, st_sms_info_t *sms_cb_info)
{
	BOOL ret = FALSE;
	char *pdu_dup = NULL;
	uint8_t error;
	PDU_DESC pdu_dec, *pPduDecodeDesc = &pdu_dec;

	//init
	memset(pPduDecodeDesc, 0, sizeof(PDU_DESC));

	//validation
	if ( !pdu || !sms_cb_info ) {
		printf("invalid args\n");
		return -1;
	}

	//duplicate pdu as pdu_dup
	pdu_dup = strdup(pdu);
	if(NULL == pdu_dup) {
		printf("pdu strdup err:(%d):%s\n", errno, strerror(errno));
		return -1;
	}

	//decode
	ret = DecodePduData(pdu_dup, pPduDecodeDesc, &error);
	if( TRUE != ret) {
		printf("ret:%d, error:%d\n",ret, error);
		free(pdu_dup);
		return -1;
	}

	//pojo
	if(pPduDecodeDesc->isConcatenatedMsg) {
		sms_cb_info->long_info.is_long_msg = 1;
		sms_cb_info->long_info.msgid = pPduDecodeDesc->concateMsgRefNo;
		sms_cb_info->long_info.sum_items = pPduDecodeDesc->concateTotalParts;
		sms_cb_info->long_info.index = pPduDecodeDesc->concateCurntPart;
	}
	else {
		sms_cb_info->long_info.is_long_msg = 0;
		sms_cb_info->long_info.msgid = 0;
		sms_cb_info->long_info.sum_items = 0;
		sms_cb_info->long_info.index = 0;
	}

	strcpy(sms_cb_info->str_PhoneNumber, pPduDecodeDesc->phoneAddr);
	strcpy(sms_cb_info->time_stamp, pPduDecodeDesc->timeStampStr);
	strcpy(sms_cb_info->message_content, (const char *)pPduDecodeDesc->usrData);

	//free
	free(pdu_dup);
#if 0
	const char *ptr = pdu;
	unsigned char code_type = 0; // 0:7bite 8:UCS-2
	unsigned int udl = 0; // user date length
	sms_cb_info->long_info.is_long_msg = 0;
	sms_cb_info->tag_type = 0;
	sms_cb_info->storage_type = -1;
	// SAC
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
#endif
	return 0;
}
