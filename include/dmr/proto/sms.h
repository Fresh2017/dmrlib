/**
 * @file
 * @brief  Short Message Service
 * @author Wijnand Modderman-Lenstra PD0MZ
 */
#ifndef _DMR_PROTO_SMS_H
#define _DMR_PROTO_SMS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <dmr/proto.h>
#include <dmr/payload/data.h>

typedef struct {
	dmr_proto_t proto;
	char 		*path;
	char 		*inbox;
	char 		*outbox;
	struct {
		dmr_data_header_t *header;
		uint8_t 		  blocks_received;
		uint8_t 		  selective_acks_sent;
		uint8_t 		  rx_seq;
		dmr_data_block_t  blocks[64];
		uint8_t 	 	  blocks_expected;
		uint8_t 		  blocks_full;
	} ts[2];
} dmr_sms_t;

extern dmr_sms_t *dmr_sms_new(char *path, char *inbox, char *outbox);

#ifdef __cplusplus
}
#endif

#endif // _DMR_PROTO_SMS_H
