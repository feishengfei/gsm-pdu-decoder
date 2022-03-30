/*
 * Copyright (c) 2022 Xi'an Gongjin Mobile Communication Co., Ltd.
 * All Rights Reserved.
 * Confidential and Proprietary - Gongjin Electronics, Inc.
 *
 * Author: Zhou, Xiang <zhouxiang@twsz.com>
 */


#ifndef  __SMS_PDU_H
#define  __SMS_PDU_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

#define SMS_MAX_PHONENUMBER_LENGTH      (252)    // max phone number length
#define SMS_MAX_MSG_CONTENT_LENGTH      (1440) //max message length
#define SMS_MAX_TIMESTAMP_LEN           (50)

typedef enum {
  SMS_TAG_TYPE_MT_READ_V01     = 0x00,
  SMS_TAG_TYPE_MT_NOT_READ_V01 = 0x01,
  SMS_TAG_TYPE_MO_SENT_V01     = 0x02,
  SMS_TAG_TYPE_MO_NOT_SENT_V01 = 0x03,
}SMS_MSG_TAG_TYPE_E;

typedef enum{
    SMS_STORAGE_TYPE_NONE = -1,//do not store
    SMS_STORAGE_TYPE_UIM  = 0,//store to sim
    SMS_STORAGE_TYPE_NV   = 1//store to device
}SMS_MSG_STORAGE_TYPE_E;

typedef struct _st_sms_long_sms_info{
    uint16_t    is_long_msg;
    uint16_t    msgid;
    uint16_t    sum_items;		// total Concatenated Msg count
    uint16_t    index;			// current index of concatenated msg
}st_sms_long_sms_info;

typedef struct _st_sms_info_t{
    SMS_MSG_TAG_TYPE_E       tag_type;//just be used when save or read sms.
    SMS_MSG_STORAGE_TYPE_E   storage_type;
    unsigned int             storage_index;
    st_sms_long_sms_info     long_info; // long message info
    char        time_stamp[SMS_MAX_TIMESTAMP_LEN];
    char        str_PhoneNumber[SMS_MAX_PHONENUMBER_LENGTH + 1];
    char        message_content[SMS_MAX_MSG_CONTENT_LENGTH + 1];      //utf-8 encoded
} st_sms_info_t;

typedef struct _st_sms_timestamp_s_type
{
    uint8_t      year;     /**< 00 through 99. */
    uint8_t      month;    /**< 01 through 12. */
    uint8_t      day;      /**< 01 through 31. */
    uint8_t      hour;     /**< 00 through 23. */
    uint8_t      minute;   /**< 00 through 59. */
    uint8_t      second;   /**< 00 through 59. */
    int8_t       timezone; /**< +/-, [-48,+48] number of 15 minutes; GSM/WCDMA only. */
} st_sms_timestamp_s_type;

typedef struct _st_sms_receipt_info_t{
    unsigned long int          receipt_message_id; //sms send  messages receipt
    st_sms_timestamp_s_type    timestamp;
}st_sms_receipt_info_t;

typedef struct _st_sms_cb_info{
    char                   msg_type[12];
    st_sms_info_t          sms_info;
    st_sms_receipt_info_t  receipt_info;
}st_sms_cb_info;

typedef void (*sms_callback)(st_sms_cb_info *sms_cb_info);


/**
 * @brief generate pdu by num(ascii, may prefixed '+') and cnt(utf-8 or pure ascii)
 *			caller should free pdu after use
 *
 * @param[in] num Destination Address
 * @param[in] cnt UTF-8 encoded string
 * @param[out] pdu hold hex2ascii string, split by '\n' if it is concatenate sms
 *
 * @return generated pdu string length if success, negative value for failure
 */
int mk_send_pdu(const char *num, const char *cnt, char **pdu);

/**
 * @brief 
 *
 * @param pdu
 * @param sms_cb_info
 *
 * @return 
 */
int mk_recv_pdu(const char *pdu, st_sms_info_t *sms_cb_info);


#ifdef __cplusplus
}
#endif

#endif
