#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dmr/bits.h"
#include "dmr/error.h"
#include "dmr/log.h"
#include "dmr/packet.h"
#include "dmr/payload/lc.h"
#include "dmr/payload/sync.h"
#include "dmr/fec/golay_20_8.h"

static char *flco_name[] = {
    "group",
    "unit to unit",
    "invalid",
    NULL
};

static struct {
    dmr_fid_t fid;
    char      *name;
} fid_name[] = {
    { DMR_FID_ETSI, "ETSI"                                      },
    { 0x01,         "RESERVED"                                  },
    { 0x02,         "RESERVED"                                  },
    { 0x03,         "RESERVED"                                  },
    { 0x04,         "Flyde Micro Ltd."                          },
    { 0x05,         "PROD-EL SPA"                               },
    { 0x06,         "Trident Datacom DBA Trident Micro Systems" },
    { 0x07,         "RADIODATA GmbH"                            },
    { 0x08,         "HYT science tech"                          },
    { 0x09,         "ASELSAN Elektronik Sanayi ve Ticaret A.S." },
    { 0x0a,         "Kirisun Communications Co. Ltd"            },
    { 0x0b,         "DMR Association Ltd."                      },
    { 0x10,         "Motorola Ltd."                             },
    { 0x13,         "EMC S.p.A. (Electronic Marketing Company)" },
    { 0x1c,         "EMC S.p.A. (Electronic Marketing Company)" },
    { 0x20,         "JVCKENWOOD Corporation"                    },
    { 0x33,         "Radio Activity Srl"                        },
    { 0x3c,         "Radio Activity Srl"                        },
    { 0x58,         "Tait Electronics Ltd",                     },
    { 0x68,         "HYT science tech"                          },
    { 0x77,         "Vertex Standard"                           },
    { 0,            NULL                                        }
};

static char *ts_name[] = {
    "TS1",
    "TS2",
    "invalid",
    NULL
};

#define PACKET_DUMP_COLS 11
void dmr_dump_packet(dmr_packet_t *packet)
{
    if (packet == NULL) {
        dmr_log_error("dump: packet is NULL");
        return;
    }

    char buf[8];
    memset(buf, 0, sizeof(buf));
    if (packet->data_type == DMR_DATA_TYPE_VOICE) {
        sprintf(buf, "%c", 'A' + (char)packet->meta.voice_frame);
    } else {
        sprintf(buf, "%d", packet->data_type);
    }
    dmr_log_debug("packet: [%lu->%lu] via %lu, seq %#02x, slot %d, flco %s (%d), data type %s (%s), stream id 0x%08lx:",
        packet->src_id,
        packet->dst_id,
        packet->repeater_id,
        packet->meta.sequence,
        packet->ts,
        dmr_flco_name(packet->flco),
        packet->flco,
        dmr_data_type_name_short(packet->data_type),
        buf,
        packet->meta.stream_id);

    dmr_sync_pattern_t pattern = dmr_sync_pattern_decode(packet);
    if (packet->data_type < DMR_DATA_TYPE_INVALID) {
        dmr_log_debug("packet: sync pattern: %s", dmr_sync_pattern_name(pattern));
    }

    size_t i, j, len = DMR_PAYLOAD_BYTES;
    char *mem = (char *)packet->payload;
    for(i = 0; i < len + ((len % PACKET_DUMP_COLS) ? (PACKET_DUMP_COLS - len % PACKET_DUMP_COLS) : 0); i++) {
        /* print offset */
        if (i % PACKET_DUMP_COLS == 0) {
            printf("0x%06zx  ", i);
        }

        bool print_hex = true;
        if (pattern == DMR_SYNC_PATTERN_UNKNOWN) {
            switch (packet->data_type) {
            /* highlight emb bits and emb lc */
            case DMR_DATA_TYPE_VOICE:
                switch (i) {
                case 13:
                    print_hex = false;
                    printf("%01x\x1b[1;32m%01x\x1b[0m ", (mem[i] & 0xf0) >> 4, mem[i] & 0x0f);
                    break;
                case 14:
                    print_hex = false;
                    printf("\x1b[1;32m%01x\x1b[1;33m%01x\x1b[0m ", (mem[i] & 0xf0) >> 4, mem[i] & 0x0f);
                    break;
                case 15:
                case 16:
                case 17:
                    print_hex = false;
                    printf("\x1b[1;33m%02x\x1b[0m ", mem[i] & 0xff);
                    break;
                case 18:
                    print_hex = false;
                    printf("\x1b[1;33m%01x\x1b[1;32m%01x\x1b[0m ", (mem[i] & 0xf0) >> 4, mem[i] & 0x0f);
                    break;
                case 19:
                    print_hex = false;
                    printf("\x1b[1;32m%01x\x1b[0m%01x ", (mem[i] & 0xf0) >> 4, mem[i] & 0x0f);
                    break;
                }
                break;

            default:
                break;
            }

        } else {

            /* highlight sync pattern, if found */
            switch (i) {
            case 13:
                print_hex = false;
                printf("%01x\x1b[1;34m%01x\x1b[0m ", (mem[i] & 0xf0) >> 4, mem[i] & 0x0f);
                break;
            case 14:
            case 15:
            case 16:
            case 17:
            case 18:
                print_hex = false;
                printf("\x1b[1;34m%02x\x1b[0m ", mem[i] & 0xff);
                break;
            case 19:
                print_hex = false;
                printf("\x1b[1;34m%01x\x1b[0m%01x ", (mem[i] & 0xf0) >> 4, mem[i] & 0x0f);
                break;
            default:
                break;
            }
        }

        if (print_hex) {
            /* print hex data */
            if (i < DMR_PAYLOAD_BYTES) {
                printf("%02x ", mem[i] & 0xff);
            } else { /* end of block, just aligning for ASCII dump */
                printf("   ");
            }
        }

        /* print ASCII dump */
        if (i % PACKET_DUMP_COLS == (PACKET_DUMP_COLS - 1)) {
            putchar('|');
            for (j = i - (PACKET_DUMP_COLS - 1); j <= i; j++) {
                if (j >= len) { /* end of block, not really printing */
                    putchar(' ');
                } else if (isprint(mem[j])) { /* printable char */
                    putchar(0xff & mem[j]);
                } else { /* other char */
                    putchar('.');
                }
            }
            putchar('|');
            putchar('\n');
        }
    }

    if (packet->data_type == DMR_DATA_TYPE_VOICE_LC) {
        dmr_full_lc_t full_lc;
        if (dmr_full_lc_decode(&full_lc, packet) != 0) {
            dmr_log_error("full LC decode failed: %s", dmr_error_get());
            return;
        }
        dmr_log_info("full LC: flco=%d, %u->%u\n",
            full_lc.flco_pdu, full_lc.src_id, full_lc.dst_id);
    }

    fflush(stdout);
}


dmr_packet_t *dmr_packet_decode(uint8_t *buf, size_t len)
{
    dmr_packet_t *packet;
    if (len != DMR_PAYLOAD_BYTES) {
        dmr_log_error("dmr: can't decode packet of %d bytes, need %d", len, DMR_PAYLOAD_BYTES);
        return NULL;
    }

    packet = malloc(sizeof(dmr_packet_t));
    if (packet == NULL) {
        dmr_error(DMR_ENOMEM);
        return NULL;
    }
    memset(packet, 0, sizeof(dmr_packet_t));
    memcpy(packet->payload, buf, DMR_PAYLOAD_BYTES);
    return packet;
}

int dmr_payload_bits(dmr_packet_t *packet, void *bits)
{
    if (packet == NULL || bits == NULL)
        return dmr_error(DMR_EINVAL);

    bool data[33 * 8];
    dmr_bytes_to_bits(packet->payload, 33, data, sizeof(data));
    memcpy(bits + 0 , &data[0],   98);
    memcpy(bits + 98, &data[166], 98);
    return 0;
}

char *dmr_fid_name(dmr_fid_t fid)
{
    uint8_t i;
    for (i = 0; fid_name[i].name != NULL; i++) {
        if (fid_name[i].fid == fid)
            return fid_name[i].name;
    }
    return NULL;
}

char *dmr_flco_name(dmr_flco_t flco)
{
    flco = min(flco, DMR_FLCO_INVALID);
    return flco_name[flco];
}

char *dmr_ts_name(dmr_ts_t ts)
{
    ts = min(ts, DMR_TS_INVALID);
    return ts_name[ts];
}

char *dmr_data_type_name(dmr_data_type_t data_type)
{
    switch (data_type) {
    case DMR_DATA_TYPE_VOICE_PI:
        return "privacy indicator";
    case DMR_DATA_TYPE_VOICE_LC:
        return "voice lc";
    case DMR_DATA_TYPE_TERMINATOR_WITH_LC:
        return "terminator with lc";
    case DMR_DATA_TYPE_CSBK:
        return "csbk";
    case DMR_DATA_TYPE_MBC:
        return "multi block control";
    case DMR_DATA_TYPE_MBCC:
        return "multi block control continuation";
    case DMR_DATA_TYPE_DATA:
        return "data";
    case DMR_DATA_TYPE_RATE12_DATA:
        return "rate 1/2 data";
    case DMR_DATA_TYPE_RATE34_DATA:
        return "rate 3/4 data";
    case DMR_DATA_TYPE_IDLE:
        return "idle";
    case DMR_DATA_TYPE_VOICE:
        return "voice";
    case DMR_DATA_TYPE_VOICE_SYNC:
        return "voice sync";
    case DMR_DATA_TYPE_INVALID:
    default:
        return "invalid";
    }
}

char *dmr_data_type_name_short(dmr_data_type_t data_type)
{
    switch (data_type) {
    case DMR_DATA_TYPE_VOICE_PI:
        return "voice pi";
    case DMR_DATA_TYPE_VOICE_LC:
        return "voice lc";
    case DMR_DATA_TYPE_TERMINATOR_WITH_LC:
        return "term lc";
    case DMR_DATA_TYPE_CSBK:
        return "csbk";
    case DMR_DATA_TYPE_MBC:
        return "mbc";
    case DMR_DATA_TYPE_MBCC:
        return "mbcc";
    case DMR_DATA_TYPE_DATA:
        return "data";
    case DMR_DATA_TYPE_RATE12_DATA:
        return "r 1/2";
    case DMR_DATA_TYPE_RATE34_DATA:
        return "r 3/4";
    case DMR_DATA_TYPE_IDLE:
        return "idle";
    case DMR_DATA_TYPE_VOICE:
        return "voice";
    case DMR_DATA_TYPE_VOICE_SYNC:
        return "vc sync";
    case DMR_DATA_TYPE_INVALID:
    default:
        return "invalid";
    }
}

int dmr_slot_type_decode(dmr_packet_t *packet)
{
    dmr_log_trace("packet: slot type decode");
    if (packet == NULL)
        return dmr_error(DMR_EINVAL);

    uint8_t bytes[3];
    memset(bytes, 0, sizeof(bytes));
    /* See Table E.1: Transmit bit order for BPTC general data burst with SYNC */
    bytes[0]  = (packet->payload[12] << 2) & B11111100;
    bytes[0] |= (packet->payload[13] >> 6) & B00000011;
    bytes[1]  = (packet->payload[13] << 2) & B11000000;
    bytes[1] |= (packet->payload[19] << 2) & B11110000;
    bytes[1] |= (packet->payload[20] >> 6) & B00000011;
    bytes[2]  = (packet->payload[20] << 2) & B11110000;

    uint8_t code = dmr_golay_20_8_decode(bytes);
    dmr_log_debug("packet: slot type Golay(20, 8) code: 0x%02x%02x%02x -> 0x%02x",
        bytes[0], bytes[1], bytes[2], code);
    packet->color_code = (code & B11110000) >> 4;
    packet->data_type  = (code & B00001111);

    dmr_log_debug("packet: data type decoded as %s (%d), color code %d",
        dmr_data_type_name(packet->data_type), packet->data_type,
        packet->color_code);

    return 0;
}

int dmr_slot_type_encode(dmr_packet_t *packet)
{
    dmr_log_trace("packet: slot type encode");
    if (packet == NULL)
        return dmr_error(DMR_EINVAL);

    if (packet->data_type >= DMR_DATA_TYPE_INVALID) {
        dmr_log_debug("packet: can't encode slot type of invalid data type 0x%02x",
            packet->data_type);
        return dmr_error(DMR_EINVAL);
    }
    if (packet->color_code < 1 || packet->color_code > 15) {
        dmr_log_debug("packet: can't encode slot type of invalid color code %d",
            packet->color_code);
        return dmr_error(DMR_EINVAL);
    }

    uint8_t bytes[3];
    memset(bytes, 0, sizeof(bytes));
    bytes[0]  = (packet->color_code << 4) & 0xf0;
    bytes[0] |= (packet->data_type  << 0) & 0x0f;

    dmr_golay_20_8_encode(bytes);

    packet->payload[12] |=                   B11000000;
    packet->payload[12] |= (bytes[0] >> 2) & B00111111;
    packet->payload[13] |=                   B00001111;
    packet->payload[13] |= (bytes[0] << 6) & B11000000;
    packet->payload[13] |= (bytes[1] >> 2) & B00110000;

    packet->payload[19] |=                   B11110000;
    packet->payload[19] |= (bytes[1] >> 2) & B00001111;
    packet->payload[20] |=                   B00000011;
    packet->payload[20] |= (bytes[1] << 6) & B11000000;
    packet->payload[20] |= (bytes[2] >> 2) & B00111100;

    return 0;
}
