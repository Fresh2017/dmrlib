#include <string.h>
#include <talloc.h>
#include "dmr/crc.h"
#include "dmr/error.h"
#include "dmr/fec/bptc_196_96.h"
#include "dmr/fec/trellis.h"
#include "dmr/payload/data.h"

static struct {
    dmr_dpf dpf;
    char    *name;
} dpf_names[] = {
    { DMR_DPF_UDT,           "Unified Data Transport (UDT)" },
    { DMR_DPF_RESPONSE,      "response"                     },
    { DMR_DPF_UNCONFIRMED,   "unconfirmed"                  },
    { DMR_DPF_CONFIRMED,     "confirmed"                    },
    { DMR_DPF_DEFINED_SHORT, "defined short"                },
    { DMR_DPF_RAW_SHORT,     "raw short"                    },
    { DMR_DPF_PROPRIETARY,   "proprietary"                  },
    { 0, NULL } /* sentinel */
};

static struct {
    dmr_sap sap;
    char    *name;
} sap_names[] = {
    { DMR_SAP_UDT,                      "Unified Data Transport (UDT)" },
    { DMR_SAP_TCPIP_HEADER_COMPRESSION, "TCP/IP header compression"    },
    { DMR_SAP_UDPIP_HEADER_COMPRESSION, "UDP/IP header compression"    },
    { DMR_SAP_IP_BASED_PACKET_DATA,     "IP based packet data"         },
    { DMR_SAP_ARP,                      "ARP"                          },
    { DMR_SAP_PROPRIETARY_DATA,         "proprietary data"             },
    { DMR_SAP_SHORT_DATA,               "short data"                   },
    { 0, NULL } /* sentinel */
};

char *dmr_dpf_name(dmr_dpf dpf)
{
    uint8_t i;
    for (i = 0; dpf_names[i].name != NULL; i++) {
        if (dpf_names[i].dpf == dpf) {
            return dpf_names[i].name;
        }
    }
    return NULL;
}

char *dmr_sap_name(dmr_sap sap)
{
    uint8_t i;
    for (i = 0; sap_names[i].name != NULL; i++) {
        if (sap_names[i].sap == sap) {
            return sap_names[i].name;
        }
    }
    return NULL;
}

static uint16_t data_header_crc(uint8_t bytes[10])
{
    uint16_t crc = 0;
    uint8_t i;
    for (i = 0; i < 10; i++) {
        dmr_crc16(&crc, bytes[i]);
    }
    dmr_crc16_finish(&crc);
    // Inverting according to the inversion polynomial.
    // Apply CRC mask, Table B.21: Data Type CRC Mask
    return (~crc) ^ 0xcccc;
}

int dmr_data_header_decode(dmr_packet packet, dmr_data_header *header, dmr_data_type data_type)
{
    if (packet == NULL || header == NULL || data_type >= DMR_DATA_TYPE_INVALID)
        return dmr_error(DMR_EINVAL);

    uint8_t bytes[12];
    memset(bytes, 0, sizeof(bytes));

    // BPTC(196, 96) decode data
    dmr_bptc_196_96 *bptc = talloc_zero(NULL, dmr_bptc_196_96);
    if (bptc == NULL) {
        dmr_log_error("data: BPTC(196,96) init failed: out of memory");
        return dmr_error(DMR_ENOMEM);
    }

    dmr_log_trace("data: decoding BPTC(196, 96)");
    if (dmr_bptc_196_96_decode(packet, bptc, bytes) != 0) {
        dmr_log_error("data: BPTC(196,96) decode failed: %s", dmr_error_get());
        TALLOC_FREE(bptc);
        return dmr_error(DMR_LASTERROR);
    }
    TALLOC_FREE(bptc);
    dmr_dump_hex(bytes, 12);

    /* Bit field order is not guaranteed, we still have to parse */
    header->group              = (bytes[0] & 0x80) != 0;
    header->response_requested = (bytes[0] & 0x40) != 0;
    header->header_compression = (bytes[0] & 0x20) != 0;
    header->dpf                = (bytes[0] & 0x0f);
    header->sap                = (bytes[1] & 0xf0) >> 4;
    header->src_id             = (bytes[2] << 16) | (bytes[3] << 8) | (bytes[4]);
    header->dst_id             = (bytes[5] << 16) | (bytes[6] << 8) | (bytes[7]);
    header->crc                = (bytes[10] << 8) | (bytes[11]);

    uint16_t crc = data_header_crc(bytes);
    if (header->crc != crc) {
        dmr_log_warn("data: %s CRC-16 checksum failed, %04x<>%04x",
            dmr_dpf_name(header->dpf), header->crc, crc);
    }
    
    switch (header->dpf) {
    case DMR_DPF_PROPRIETARY:
        header->data.proprietary.mfid              = (bytes[1] & 0x7f);
        break;
    case DMR_DPF_UDT:
        header->data.udt.udtf                      = (bytes[1] & 0x0f);
        header->data.udt.pad_nibble                = (bytes[8] & 0xf8) >> 3;
        header->data.udt.appended_blocks           = (bytes[8] & 0x03);
        header->data.udt.supplementary             = (bytes[9] & 0x80) != 0;
        header->data.udt.opcode                    = (bytes[9] & 0x3f);
        break;
    case DMR_DPF_UNCONFIRMED:
        header->data.unconfirmed.pad_octet_count   = (bytes[0] & 0x10) | (bytes[1] & 0x0f);
        header->data.unconfirmed.full_message      = (bytes[8] & 0x80) != 0;
        header->data.unconfirmed.blocks_to_follow  = (bytes[8] & 0x7f);
        header->data.unconfirmed.fragment_seq      = (bytes[9] & 0x0f);
        break;
    case DMR_DPF_CONFIRMED:
        header->data.confirmed.pad_octet_count     = (bytes[0] & 0x10) | (bytes[1] & 0x0f);
        header->data.confirmed.full_message        = (bytes[8] & 0x80) != 0;
        header->data.confirmed.blocks_to_follow    = (bytes[8] & 0x7f);
        header->data.confirmed.resync              = (bytes[9] & 0x80) != 0;
        header->data.confirmed.send_seq            = (bytes[9] & 0x70) >> 4;
        header->data.confirmed.fragment_seq        = (bytes[9] & 0x0f);
        break;
    case DMR_DPF_RESPONSE:
        header->data.response.blocks_to_follow     = (bytes[8] & 0x7f);
        header->data.response.class_type           = (bytes[9] & 0xf8) >> 3;
        header->data.response.status               = (bytes[9] & 0x07);
        break;
    case DMR_DPF_RAW_SHORT:
        header->data.raw_short.appended_blocks     = (bytes[0] & 0x30) | (bytes[1] & 0x0f);
        header->data.raw_short.src_port            = (bytes[8] & 0xe0) >> 5;
        header->data.raw_short.dst_port            = (bytes[8] & 0x1c) >> 2;
        header->data.raw_short.resync              = (bytes[8] & 0x02) != 0;
        header->data.raw_short.full_message        = (bytes[8] & 0x01) != 0;
        header->data.raw_short.bit_padding         = (bytes[9]);
        break;
    case DMR_DPF_DEFINED_SHORT:
        header->data.defined_short.appended_blocks = (bytes[0] & 0x30) | (bytes[1] & 0x0f);
        header->data.defined_short.dd              = (bytes[8] & 0xfc) >> 2;
        header->data.defined_short.resync          = (bytes[8] & 0x02) != 0;
        header->data.defined_short.full_message    = (bytes[8] & 0x01) != 0;
        header->data.defined_short.bit_padding     = (bytes[9]);
        break;
    }

    return 0;
}

int dmr_data_block_decode(dmr_packet packet, dmr_data_block *block, bool confirmed, dmr_data_type data_type)
{
    if (block == NULL || packet == NULL)
        return dmr_error(DMR_EINVAL);

    uint16_t crc = 0;
    uint8_t i;
    memset(block, 0, sizeof(dmr_data_block));
    if (data_type == DMR_DATA_TYPE_RATE34_DATA) {
        block->length = dmr_data_block_size(DMR_DATA_RATE34, confirmed);

        uint8_t bytes[block->length + 2];
        memset(bytes, 0, sizeof(bytes));
        if (dmr_trellis_rate_34_decode(packet, bytes) != 0)
            return dmr_error(DMR_LASTERROR);

        if (confirmed) {
            block->serial = ((bytes[0] >> 1));
            block->crc    = ((bytes[0] & 0x01) << 8) | bytes[1];
            memcpy(block->data, bytes + 2, min(sizeof(block->data), sizeof(bytes)));

            for (i = 0; i < block->length; i++) {
                dmr_crc9(&crc, block->data[i], 8);
            }
            dmr_crc9(&crc, block->serial, 7);
            dmr_crc9_finish(&crc, 8);
            crc  = ~crc;
            crc &= 0x01ff;
            crc ^= 0x01ff;

            if (!(block->ok = (crc == block->crc))) {
                dmr_log_error("data: rate 3/4 data CRC9 check failed %04x<>%04x",
                    block->crc, crc);
                return -1;
            }
        }
    } else {
        /* Not implemented */
        return dmr_error(DMR_EINVAL);
    }

    return 0;
}

uint8_t dmr_data_block_size(dmr_data_rate rate, bool confirmed)
{
    switch (rate) {
    case DMR_DATA_RATE1:  return confirmed ? 22 : 24;
    case DMR_DATA_RATE12: return confirmed ? 10 : 12;
    case DMR_DATA_RATE34: return confirmed ? 16 : 18;
    default:              return 0;
    }
}

