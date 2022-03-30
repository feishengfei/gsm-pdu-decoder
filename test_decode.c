#include <stdio.h>
#include <stdlib.h>		/*free*/
#include <string.h>
#include <unistd.h>		/* getopt */
#include "Pdu.h"
#include "sms_pdu.h"

#ifndef USE_HOSTCC

void usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s -s PDU_STR_01 PDU_STR_02 ... \n", argv0);
	fprintf(stderr, " -s \tmeans treat PDU_STR_01 PDU_STR_02 as concatenated msg.\n");
	exit(EXIT_FAILURE);
}

void dump_sms_info(st_sms_info_t *sms_info_p)
{
	if(NULL == sms_info_p) return;

	printf("%d %d %d %d\n",
		sms_info_p->long_info.is_long_msg,
		sms_info_p->long_info.msgid,
		sms_info_p->long_info.sum_items,
		sms_info_p->long_info.index);
	printf("%s\n",sms_info_p->str_PhoneNumber);
	printf("%s\n",sms_info_p->time_stamp);
	printf("%s\n",sms_info_p->message_content);
		
}

int main(int argc, char *argv[])
{
	int flags = 0;
	int opt;
	st_sms_info_t sms_info;

	/*encoding*/
	if(2 > argc) {
		usage(argv[0]);
	}

	while ((opt = getopt(argc, argv, "sh")) != -1) {
		switch (opt) {
			case 's':
				flags = 1;
				break;
			case 'h':
				/*fall throught*/
			default: /* '?' */
				usage(argv[0]);
		}
	}

	if(0 == flags) {
		if (optind < argc) {
			while (optind < argc) {
				memset(&sms_info, 0, sizeof(sms_info));
				mk_recv_pdu(argv[optind++], &sms_info);
				dump_sms_info(&sms_info);
			}
		}
	}
	else {
		//TODO
	}

	return 0;
}
#endif /* USE_HOSTCC */
