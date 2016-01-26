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
	DMR_DPF_UDT 	   	  = 0b0000, /* 8.2.1.8 Unified Data Transport (UDT) data header */
	DMR_DPF_RESPONSE 	  = 0b0001, /* 8.2.1.3 Response data header */
	DMR_DPF_UNCONFIRMED	  =	0b0010, /* 8.2.1.1 Unconfirmed data Header */
	DMR_DPF_CONFIRMED  	  = 0b0011, /* 8.2.1.2 Confirmed data header */
	DMR_DPF_DEFINED_SHORT = 0b1101, /* 8.2.1.7 Defined short data header */
	DMR_DPF_RAW_SHORT     = 0b1110, /* 8.2.1.6 Raw short data header */
	DMR_DPF_PROPRIETARY   = 0b1111, /* 8.2.1.4 Proprietary data header */
} dmr_dpf_t;

/* Table 9.31: Service Access Point ID information element content */
typedef enum {
	DMR_SAP_UDT 					 = 0b0000,
	DMR_SAP_TCPIP_HEADER_COMPRESSION = 0b0010,
	DMR_SAP_UDPIP_HEADER_COMPRESSION = 0b0011,
	DMR_SAP_IP_BASED_PACKET_DATA 	 = 0b0100,
	DMR_SAP_ARP 					 = 0b0101,
	DMR_SAP_PROPRIETARY_DATA 		 = 0b1001,
	DMR_SAP_SHORT_DATA 				 = 0b1010,
} dmr_sap_t;

/* Table 9.50: Defined Data format (DD) information element content */
typedef enum {
	DMR_DD_BINARY 		= 0b000000,
	DMR_DD_BCD 			= 0b000001,
	DMR_DD_7BIT 		= 0b000010, /* 7-bit character */
	DMR_DD_ISO_8859_1 	= 0b000011, /* 8-bit ISO 8859-1 */
	DMR_DD_ISO_8859_2 	= 0b000100, /* 8-bit ISO 8859-2 */
	DMR_DD_ISO_8859_3 	= 0b000101, /* 8-bit ISO 8859-3 */
	DMR_DD_ISO_8859_4 	= 0b000110, /* 8-bit ISO 8859-4 */
	DMR_DD_ISO_8859_5 	= 0b000111, /* 8-bit ISO 8859-5 */
	DMR_DD_ISO_8859_6 	= 0b001000, /* 8-bit ISO 8859-6 */
	DMR_DD_ISO_8859_7 	= 0b001001, /* 8-bit ISO 8859-7 */
	DMR_DD_ISO_8859_8 	= 0b001010, /* 8-bit ISO 8859-8 */
	DMR_DD_ISO_8859_9 	= 0b001011, /* 8-bit ISO 8859-9 */
	DMR_DD_ISO_8859_10 	= 0b001100, /* 8-bit ISO 8859-10 */
	DMR_DD_ISO_8859_11 	= 0b001101, /* 8-bit ISO 8859-11 */
	DMR_DD_ISO_8859_13 	= 0b001110, /* 8-bit ISO 8859-13 */
	DMR_DD_ISO_8859_14 	= 0b001111, /* 8-bit ISO 8859-14 */
	DMR_DD_ISO_8859_15 	= 0b010000, /* 8-bit ISO 8859-15 */
	DMR_DD_ISO_8859_16 	= 0b010001, /* 8-bit ISO 8859-16 */
	DMR_DD_UTF_8 		= 0b010010, /* Unicode UTF-8 */
	DMR_DD_UTF_16 		= 0b010011, /* Unicode UTF-16 */
	DMR_DD_UTF_16BE 	= 0b010100, /* Unicode UTF-16BE */
	DMR_DD_UTF_16LE 	= 0b010101, /* Unicode UTF-16LE */
	DMR_DD_UTF_32 		= 0b010110, /* Unicode UTF-32 */
	DMR_DD_UTF_32BE		= 0b010111, /* Unicode UTF-32BE */
	DMR_DD_UTF_32LE		= 0b011000, /* Unicode UTF-32LE */
} dmr_dd_t;

/* Table 9.51: Unified Data Transport Format information element content */
typedef enum {
	DMR_UDTF_BINARY 		= 0b0000,
	DMR_UDTF_MS_ADDRESS 	= 0b0001,
	DMR_UDTF_4_BIT_BCD  	= 0b0010,
	DMR_UDTF_ISO_7_BIT  	= 0b0011, /* ISO 7-bit coded characters */
	DMR_UDTF_ISO_8_BIT  	= 0b0100, /* ISO 8-bit coded characters */
	DMR_UDTF_NMEA 			= 0b0101, /* NMEA location coded */
	DMR_UDTF_IP 			= 0b0110, /* IP address */
	DMR_UDTF_16_BIT_UNICODE = 0b0111,
	DMR_UDTF_CUSTOM1 		= 0b1000,
	DMR_UDTF_CUSTOM2 		= 0b1001,
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
