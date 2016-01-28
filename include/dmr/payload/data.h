/**
 * @file
 * @brief  Data Payload
 * @author Wijnand Modderman-Lenstra PD0MZ
 */
#ifndef _DMR_PAYLOAD_DATA_H
#define _DMR_PAYLOAD_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <dmr/packet.h>

/* Table 9.30: Data packet format (DPF) information element content */
typedef enum {
	DMR_DPF_UDT 	   	  = 0x00, /* 8.2.1.8 Unified Data Transport (UDT) data header */
	DMR_DPF_RESPONSE 	  = 0x01, /* 8.2.1.3 Response data header */
	DMR_DPF_UNCONFIRMED	  =	0x02, /* 8.2.1.1 Unconfirmed data Header */
	DMR_DPF_CONFIRMED  	  = 0x03, /* 8.2.1.2 Confirmed data header */
	DMR_DPF_DEFINED_SHORT = 0x0d, /* 8.2.1.7 Defined short data header */
	DMR_DPF_RAW_SHORT     = 0x0e, /* 8.2.1.6 Raw short data header */
	DMR_DPF_PROPRIETARY   = 0x0f, /* 8.2.1.4 Proprietary data header */
} dmr_dpf_t;

/* Table 9.31: Service Access Point ID information element content */
typedef enum {
	DMR_SAP_UDT 					 = 0x00,
	DMR_SAP_TCPIP_HEADER_COMPRESSION = 0x02,
	DMR_SAP_UDPIP_HEADER_COMPRESSION = 0x03,
	DMR_SAP_IP_BASED_PACKET_DATA 	 = 0x04,
	DMR_SAP_ARP 					 = 0x05,
	DMR_SAP_PROPRIETARY_DATA 		 = 0x09,
	DMR_SAP_SHORT_DATA 				 = 0x0a,
} dmr_sap_t;

/* Table 9.50: Defined Data format (DD) information element content */
typedef enum {
	DMR_DD_BINARY 		= 0x00,
	DMR_DD_BCD 			= 0x01,
	DMR_DD_7BIT 		= 0x02, /* 7-bit character */
	DMR_DD_ISO_8859_1 	= 0x03, /* 8-bit ISO 8859-1 */
	DMR_DD_ISO_8859_2 	= 0x04, /* 8-bit ISO 8859-2 */
	DMR_DD_ISO_8859_3 	= 0x05, /* 8-bit ISO 8859-3 */
	DMR_DD_ISO_8859_4 	= 0x06, /* 8-bit ISO 8859-4 */
	DMR_DD_ISO_8859_5 	= 0x07, /* 8-bit ISO 8859-5 */
	DMR_DD_ISO_8859_6 	= 0x08, /* 8-bit ISO 8859-6 */
	DMR_DD_ISO_8859_7 	= 0x09, /* 8-bit ISO 8859-7 */
	DMR_DD_ISO_8859_8 	= 0x0a, /* 8-bit ISO 8859-8 */
	DMR_DD_ISO_8859_9 	= 0x0b, /* 8-bit ISO 8859-9 */
	DMR_DD_ISO_8859_10 	= 0x0c, /* 8-bit ISO 8859-10 */
	DMR_DD_ISO_8859_11 	= 0x0d, /* 8-bit ISO 8859-11 */
	DMR_DD_ISO_8859_13 	= 0x0e, /* 8-bit ISO 8859-13 */
	DMR_DD_ISO_8859_14 	= 0x0f, /* 8-bit ISO 8859-14 */
	DMR_DD_ISO_8859_15 	= 0x10, /* 8-bit ISO 8859-15 */
	DMR_DD_ISO_8859_16 	= 0x11, /* 8-bit ISO 8859-16 */
	DMR_DD_UTF_8 		= 0x12, /* Unicode UTF-8 */
	DMR_DD_UTF_16 		= 0x13, /* Unicode UTF-16 */
	DMR_DD_UTF_16BE 	= 0x14, /* Unicode UTF-16BE */
	DMR_DD_UTF_16LE 	= 0x15, /* Unicode UTF-16LE */
	DMR_DD_UTF_32 		= 0x16, /* Unicode UTF-32 */
	DMR_DD_UTF_32BE		= 0x17, /* Unicode UTF-32BE */
	DMR_DD_UTF_32LE		= 0x18, /* Unicode UTF-32LE */
} dmr_dd_t;

/* Table 9.51: Unified Data Transport Format information element content */
typedef enum {
	DMR_UDTF_BINARY 		= 0x00,
	DMR_UDTF_MS_ADDRESS 	= 0x01,
	DMR_UDTF_4_BIT_BCD  	= 0x02,
	DMR_UDTF_ISO_7_BIT  	= 0x03, /* ISO 7-bit coded characters */
	DMR_UDTF_ISO_8_BIT  	= 0x04, /* ISO 8-bit coded characters */
	DMR_UDTF_NMEA 			= 0x05, /* NMEA location coded */
	DMR_UDTF_IP 			= 0x06, /* IP address */
	DMR_UDTF_16_BIT_UNICODE = 0x07,
	DMR_UDTF_CUSTOM1 		= 0x08,
	DMR_UDTF_CUSTOM2 		= 0x09,
} dmr_udtf_t;

typedef struct {
	dmr_dpf_t dpf;
	bool 	  group;
	bool 	  response_requested;
	bool 	  header_compression;
	dmr_sap_t sap;
	uint32_t  dst_id;
	uint32_t  src_id;
	uint16_t  crc;
	union {
		struct {
			uint8_t pad_octet_count;
			bool 	full_message;
			uint8_t blocks_to_follow;
			uint8_t fragment_seq;
		} unconfirmed;
		struct {
			uint8_t pad_octet_count;
			bool 	full_message;
			uint8_t blocks_to_follow;
			bool 	resync;
			uint8_t send_seq;
			uint8_t fragment_seq;
		} confirmed;
		struct {
			uint8_t blocks_to_follow;
			uint8_t class_type;
			uint8_t status;
		} response;
		struct {
			uint8_t appended_blocks;
			uint8_t src_port;
			uint8_t dst_port;
			bool    resync;
			bool 	full_message;
			uint8_t bit_padding;
		} raw_short;
		struct {
			uint8_t  appended_blocks;
			dmr_dd_t dd;
			bool 	 resync;
			bool 	 full_message;
			uint8_t  bit_padding;
		} defined_short;
		struct {
			dmr_udtf_t udtf;
			uint8_t    pad_nibble;
			uint8_t    appended_blocks;
			bool 	   supplementary;
			uint8_t    opcode;
		} udt;
		struct {
			uint8_t    mfid;
		} proprietary;
	} data;
} dmr_data_header_t;

typedef struct {
	uint8_t  serial;
	uint16_t crc;
	bool 	 ok;
	uint8_t  data[24];
	uint8_t  length;
} dmr_data_block_t;

typedef enum {
	DMR_DATA_RATE1  = 1,
	DMR_DATA_RATE12 = 12,
	DMR_DATA_RATE34 = 34
} dmr_data_rate_t;

extern char *dmr_dpf_name(dmr_dpf_t dpf);
extern char *dmr_sap_name(dmr_sap_t sap);
extern int dmr_data_header_decode(dmr_data_header_t *header, dmr_packet_t *packet);
extern int dmr_data_block_decode(dmr_data_block_t *block, bool confirmed, dmr_packet_t *packet);
extern uint8_t dmr_data_block_size(dmr_data_rate_t rate, bool confirmed);

#ifdef __cplusplus
}
#endif

#endif // _DMR_PAYLOAD_DATA_H
