#ifndef __PDU_H__
#define __PDU_H__

//###########################################################################
// @INCLUDE
//###########################################################################
#include <stdint.h>

//###########################################################################
// @DEFINES
//###########################################################################
#define SMS_PDU_MAX_LEN					175

/*
	//6 octets for User Uata Header
	hexBuf[idx++] = UDH_CONCATENATED_MSG_LEN;
	hexBuf[idx++] = IE_CONCATENATED_MSG;
	hexBuf[idx++] = IE_CONCATENATED_MSG_LEN;
	hexBuf[idx++] = pPduEncodeDesc->concateMsgRefNo;
	hexBuf[idx++] = pPduEncodeDesc->concateTotalParts;
	hexBuf[idx++] = pPduEncodeDesc->concateCurntPart;
*/
#define SMS_PDU_USER_DATA_MAX_LEN		140  /* 140 octets */
#define TRUNCATED_PDU_DATA_LEN			134	 /* 140 - 6 octets , 134 octets 8bits or 67 UCS2 characters*/

//7 octets for User Uata Header
#define SMS_GSM7BIT_MAX_LEN				160		/* 140 * 8 / 7 = 160 charcters */
#define TRUNCATED_GSM_DATA_LEN			153		/* */

#define ADDR_OCTET_MAX_LEN				20	 /* 10 Octets */
#define TIME_STAMP_OCTET_MAX_LEN		14	 /* 7 Octets */
#define UTF8_CHAR_LEN					4	 /* 4 bytes */

#define TIME_STAMP_APP_MAX_LEN			32	/*[22/04/06,17:19:21+8]*/

#define GSM_7BIT						0x00
#define ANSI_8BIT						0x01  // TODO
#define UCS2_16BIT						0x02  // TODO

/* PDU Type */
//UDHI	Parameter indicating that the UD field contains a Header
#define USER_DATA_HEADER_INDICATION		0x40
//SRI	Parameter indicating if the Short Message Entity (SME) has requested a status report.
#define STATUS_REPORT_INDICATOR			0x20

#define MSG_REF_NO_DEFAULT				0x00
#define UDH_CONCATENATED_MSG_LEN		0x05
#define IE_CONCATENATED_MSG_LEN			0x03
#define MORE_MSG_TO_SEND				0x04
#ifndef BOOL
#define BOOL char
#endif
#define TRUE							 1
#define FALSE							 0
#define LONG_SMS_TEXT_MAX_LEN			700
//###########################################################################
// @ENUMERATOR
//###########################################################################
/* Error Values */
enum
{
	ERR_MSG_TYPE = 0,
	ERR_CHAR_SET,
	ERR_PHONE_TYPE_OF_ADDR,
	ERR_PHONE_NUM_PLAN,
	ERR_PROTOCOL_ID,
	ERR_DATA_CODE_SCHEME,
	ERR_CODE_CONVERT
};

/* Message Type indication */
enum
{
	MSG_TYPE_SMS_DELIVER = 0x00,
	MSG_TYPE_SMS_SUBMIT = 0x01,
	MSG_TYPE_SMS_STATUS_REPORT = 0x02
};

/* Type of Number */
enum
{
   NUM_TYPE_UNKNOWN = 0x00,
   NUM_TYPE_INTERNATIONAL = 0x01,
   NUM_TYPE_NATIONAL = 0x02,
   NUM_TYPE_ALPHANUMERIC = 0x05
};

/* Numbering Plan Indicator */
enum
{
   NUM_PLAN_UNKNOWN = 0x00,
   NUM_PLAN_ISDN = 0x01
};

/* Validity Period type */
enum
{
   VLDTY_PERIOD_DEFAULT = 0x00,
   VLDTY_PERIOD_RELATIVE = 0x10,
   VLDTY_PERIOD_ABSOLUTE = 0x18		/*FIXME*/
};

/* User Data Header Information Element identifier */
enum
{
   IE_CONCATENATED_MSG = 0x00,
   IE_PORT_ADDR_8BIT = 0x04,
   IE_PORT_ADDR_16BIT = 0x05
};

/* Message State */
enum
{
	MSG_DELIVERY_FAIL = 0,
	MSG_DELIVERY_SUCCESS
};

typedef struct
{
	uint8_t day;
	uint8_t month;
	uint8_t year;

} DATE_DESC;

typedef struct
{
	uint8_t hour;
	uint8_t minute;
	uint8_t second;

} TIME_DESC;

//###########################################################################
// @DATATYPE
//###########################################################################
/* PDU Decode Descriptor */
typedef struct
{
	/* Both SMS-SUBMIT & SMS-DELIVER fields */

	/* Service Center Address */
	uint8_t smscAddrLen;						/* Length of Service Center Number  */
	char smscAddr[ADDR_OCTET_MAX_LEN + 1];	/* Service Center Number */
	uint8_t smscNpi;							/* Numbering Plan Indicactor */
	uint8_t smscTypeOfAddr;					/* Type of Address of Service Center Number */

	/* Destination Address(DO) for SMS-DELIVER */
	/* Origination Address(DA) for SMS-SUBMIT */
	uint8_t phoneAddrLen;						/* Lenght of Phone Number */
	uint8_t phoneTypeOfAddr;					/* Type of Address of Phone Number */
	char phoneAddr[ADDR_OCTET_MAX_LEN + 1];	/* Phone Number */

	/* UDL */
	uint8_t usrDataLen;							    				/* User Data Length */
	/* UD */
	uint8_t usrData[SMS_GSM7BIT_MAX_LEN * UTF8_CHAR_LEN + 1];   		/* User Data for GSM_7bit, ANSI_8bit & UCS2_16bit*/
	//	bit 3,	bit 2			Alphabet
	//		0,		0			Default alphabe(7bit)		
	//		0,		1			8 bit data
	//		1,		0			UCS2 (16bit)
	//		1,		1			Reserved
	uint8_t usrDataFormat;					/* User Data Coding Format */
	uint8_t udhLen;									/* User Data Header Length */
	uint8_t udhInfoType;								/* Type of User Data Header */
	uint8_t udhInfoLen;								/* User Data Header information length */

	uint8_t firstOct;							/* First octet of PDU SMS */
	BOOL isHeaderPrsnt;						/* User data header indicator */
	uint8_t msgRefNo;							/* Message Reference Number */


	uint8_t protocolId;						/* Protocol Identifier */
	uint8_t dataCodeScheme; 					/* Data Coding scheme */
	uint8_t msgType;						    /* Message Type */
	BOOL isWapPushMsg;						/* WAP-PUSH SMS */
	BOOL isFlashMsg;						/* FLASH SMS */
	BOOL isStsReportReq;					/* Staus Report Flag */
	BOOL isMsgWait;							/* Message Waiting */

	/* SMS-SUBMIT */
	uint8_t vldtPrd;							 /* Validity Period */
	// [4:3] VPF
	uint8_t vldtPrdFrmt;						 /* Validity Period Format */

	/* SMS-DELIVER */
	uint8_t timeStampHex[7];
	char timeStampStr[TIME_STAMP_APP_MAX_LEN];
	char timeStamp[TIME_STAMP_OCTET_MAX_LEN + 1];		 /* Service Center Time Stamp */
	char dischrgTimeStamp[TIME_STAMP_OCTET_MAX_LEN + 1]; /* Discharge Time Stamp */


	BOOL isConcatenatedMsg;							/* Concatenated Msg or Not */
	uint8_t concateMsgRefNo; 							/* Concatenated Message Reference Number */
	uint8_t concateCurntPart;							/* Sequence Number of concatenated messages */
	uint8_t concateTotalParts;						/* Maximum Number of concatenated messages */

	uint8_t smsSts;					  				/* Status of SMS */
	uint16_t srcPortAddr;								/* Source Port Address */
	uint16_t destPortAddr;							/* Destination Port Address */
	BOOL isDeliveryReq;
	DATE_DESC date;
	TIME_DESC time;
	uint8_t timezone;
} PDU_DESC;


//###########################################################################
// @PROTOTYPE
//###########################################################################
BOOL DecodePduData(char *pGsmPduStr, PDU_DESC *pPduDecodeDesc, uint8_t *pError);
BOOL EncodePduData(PDU_DESC *pPduEncodeDesc, char *pGsmPduStr);
void Utf8StrToGsmStr(uint8_t *cIn, uint16_t cInLen, uint8_t *gsmOut, uint16_t *gsmLen);

void dumpPDU_SMS_SUBMIT(PDU_DESC *pPduEncodeDesc);
void dumpPDU_SMS_DELIVER(PDU_DESC *pPduEncodeDesc);
void hex_dump(const char *str, const char *pSrcBufVA, int SrcBufLen);

#endif //__PDU_H__
