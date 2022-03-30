#include <stdio.h>
#include <stdlib.h>		/*free*/
#include <string.h>
#include <unistd.h>		/* getopt */
#include "Pdu.h"
#include "sms_pdu.h"


void usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s -n PHONE_NUMBER MSG_01 MSG_02 ... \n", argv0);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int opt;
	char *pdu = NULL;
	const char *number = "+8617316602003";

	while ((opt = getopt(argc, argv, "n:h")) != -1) {
		switch (opt) {
			case 'n':
				number = optarg;
				break;
			case 'h':
				/*fall throught*/
			default: /* '?' */
				usage(argv[0]);
		}
	}

	if (optind < argc) {
		while (optind < argc) {
			mk_send_pdu(number, argv[optind++], &pdu);
			printf("%s\n", pdu);
			if(pdu)
				free(pdu);
		}
	}

    return 0;
}
