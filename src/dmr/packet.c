#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <talloc.h>
#include "dmr/bits.h"
#include "dmr/error.h"
#include "dmr/log.h"
#include "dmr/packet.h"
#include "dmr/payload/lc.h"
#include "dmr/payload/sync.h"
#include "dmr/fec/golay_20_8.h"
#include "common/byte.h"

DMR_PRV static char *flco_name[] = {
    "group",
    "unit to unit",
    "invalid",
    NULL
};

DMR_PRV static struct {
    dmr_fid fid;
    char    *name;
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

DMR_PRV static char *ts_name[] = {
    "TS1",
    "TS2",
    "invalid",
    NULL
};

#define PACKET_DUMP_COLS 11
DMR_API void dmr_dump_packet(dmr_packet packet)
{
    dmr_sync_pattern pattern = dmr_sync_pattern_decode(packet);
    if (pattern != DMR_SYNC_PATTERN_UNKNOWN) {
        dmr_log_debug("packet: sync pattern: %s", dmr_sync_pattern_name(pattern));
    }

    dmr_data_type data_type;
    dmr_slot_type_decode(packet, NULL, &data_type);

    size_t i, j, len = DMR_PACKET_LEN;
    char *mem = (char *)packet;
    for(i = 0; i < len + ((len % PACKET_DUMP_COLS) ? (PACKET_DUMP_COLS - len % PACKET_DUMP_COLS) : 0); i++) {
        /* print offset */
        if (i % PACKET_DUMP_COLS == 0) {
            printf("0x%06zx  ", i);
        }

        bool print_hex = true;
        if (pattern == DMR_SYNC_PATTERN_UNKNOWN) {
            switch (data_type) {
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
            if (i < DMR_PACKET_LEN) {
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

}

DMR_API void dmr_dump_parsed_packet(dmr_parsed_packet *parsed)
{
    if (parsed == NULL) {
        dmr_log_error("dump: parsed is NULL");
        return;
    }

    char buf[8];
    memset(buf, 0, sizeof(buf));
    if (parsed->data_type == DMR_DATA_TYPE_VOICE) {
        sprintf(buf, "%c", 'A' + (char)parsed->voice_frame);
    } else {
        sprintf(buf, "%d", parsed->data_type);
    }
    dmr_log_debug("packet: [%lu->%lu] via %lu, seq 0x%02x, slot %d, flco %s (%d), data type %s (%s), stream id 0x%08lx:",
        parsed->src_id,
        parsed->dst_id,
        parsed->repeater_id,
        parsed->sequence,
        parsed->ts,
        dmr_flco_name(parsed->flco),
        parsed->flco,
        dmr_data_type_name_short(parsed->data_type),
        buf,
        parsed->stream_id);

    if (parsed->data_type == DMR_DATA_TYPE_VOICE_LC) {
        dmr_full_lc full_lc;
        if (dmr_full_lc_decode(parsed->packet, &full_lc, parsed->data_type) != 0) {
            dmr_log_error("full LC decode failed: %s", dmr_error_get());
            return;
        }
        dmr_log_info("full LC: flco=%d, %u->%u\n",
            full_lc.flco_pdu, full_lc.src_id, full_lc.dst_id);
    }
    dmr_dump_packet(parsed->packet);

    fflush(stdout);
}

dmr_parsed_packet *dmr_packet_decode(dmr_packet packet)
{
    dmr_parsed_packet *parsed;
    if ((parsed = talloc_zero(NULL, dmr_parsed_packet)) == NULL) {
        dmr_error(DMR_ENOMEM);
        return NULL;
    }
    byte_copy(parsed->packet, packet, DMR_PACKET_LEN);

    dmr_sync_pattern pattern = dmr_sync_pattern_decode(packet);
    switch (pattern) {
    case DMR_SYNC_PATTERN_BS_SOURCED_DATA:
    case DMR_SYNC_PATTERN_MS_SOURCED_DATA:
        parsed->data_type = DMR_DATA_TYPE_DATA_HEADER;
        break;

    case DMR_SYNC_PATTERN_BS_SOURCED_VOICE:
    case DMR_SYNC_PATTERN_MS_SOURCED_VOICE:
        parsed->data_type = DMR_DATA_TYPE_VOICE_SYNC;
        parsed->voice_frame = 0;
        break;

    case DMR_SYNC_PATTERN_DIRECT_DATA_TS1:
        parsed->data_type = DMR_DATA_TYPE_DATA_HEADER;
        parsed->ts = DMR_TS1;
        break;

    case DMR_SYNC_PATTERN_DIRECT_DATA_TS2:
        parsed->data_type = DMR_DATA_TYPE_DATA_HEADER;
        parsed->ts = DMR_TS2;
        break;

    case DMR_SYNC_PATTERN_DIRECT_VOICE_TS1:
        parsed->data_type = DMR_DATA_TYPE_VOICE_SYNC;
        parsed->ts = DMR_TS1;
        break;

    case DMR_SYNC_PATTERN_DIRECT_VOICE_TS2:
        parsed->data_type = DMR_DATA_TYPE_VOICE_SYNC;
        parsed->ts = DMR_TS2;
        break;

    case DMR_SYNC_PATTERN_UNKNOWN:
    default:
        parsed->data_type = DMR_DATA_TYPE_INVALID;
        dmr_slot_type_decode(packet, &parsed->color_code, &parsed->data_type);
        break;
    }

    parsed->parsed = true;
    return parsed;
}

DMR_API int dmr_payload_bits(dmr_packet packet, void *bits)
{
    if (packet == NULL || bits == NULL)
        return dmr_error(DMR_EINVAL);

    bool data[DMR_PACKET_BITS];
    dmr_bytes_to_bits(packet, DMR_PACKET_LEN, data, sizeof(data));
    memcpy(bits + 0 , &data[0],   98);
    memcpy(bits + 98, &data[166], 98);
    return 0;
}

DMR_API char *dmr_fid_name(dmr_fid fid)
{
    uint8_t i;
    for (i = 0; fid_name[i].name != NULL; i++) {
        if (fid_name[i].fid == fid)
            return fid_name[i].name;
    }
    return NULL;
}

DMR_API char *dmr_flco_name(dmr_flco flco)
{
    flco = min(flco, DMR_FLCO_INVALID);
    return flco_name[flco];
}

DMR_API char *dmr_ts_name(dmr_ts ts)
{
    ts = min(ts, DMR_TS_INVALID);
    return ts_name[ts];
}

DMR_API char *dmr_data_type_name(dmr_data_type data_type)
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
    case DMR_DATA_TYPE_MBC_HEADER:
        return "multi block control";
    case DMR_DATA_TYPE_MBC_CONTINUATION:
        return "multi block control continuation";
    case DMR_DATA_TYPE_DATA_HEADER:
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

char *dmr_data_type_name_short(dmr_data_type data_type)
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
    case DMR_DATA_TYPE_MBC_HEADER:
        return "mbc";
    case DMR_DATA_TYPE_MBC_CONTINUATION:
        return "mbcc";
    case DMR_DATA_TYPE_DATA_HEADER:
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

DMR_API int dmr_slot_type_decode(dmr_packet packet, dmr_color_code *color_code, dmr_data_type *data_type)
{
    dmr_log_trace("packet: slot type decode");
    if (packet == NULL)
        return dmr_error(DMR_EINVAL);

    uint8_t bytes[3];
    memset(bytes, 0, sizeof(bytes));
    /* See Table E.1: Transmit bit order for BPTC general data burst with SYNC */
    bytes[0]  = (packet[12] << 2) & B11111100;
    bytes[0] |= (packet[13] >> 6) & B00000011;
    bytes[1]  = (packet[13] << 2) & B11000000;
    bytes[1] |= (packet[19] << 2) & B11110000;
    bytes[1] |= (packet[20] >> 6) & B00000011;
    bytes[2]  = (packet[20] << 2) & B11110000;

    uint8_t code = dmr_golay_20_8_decode(bytes);
    dmr_log_debug("packet: slot type Golay(20, 8) code: 0x%02x%02x%02x -> 0x%02x",
        bytes[0], bytes[1], bytes[2], code);
    
    if (color_code) {
        *color_code = (code & B11110000) >> 4;
        dmr_log_debug("packet: color code %d", *color_code);
    }
    if (data_type) {
        *data_type  = (code & B00001111);
        dmr_log_debug("packet: data type decoded as %s (%d)",
            dmr_data_type_name(*data_type), *data_type);
    }

    return 0;
}

DMR_API int dmr_slot_type_encode(dmr_packet packet, dmr_color_code color_code, dmr_data_type data_type)
{
    dmr_log_trace("packet: slot type encode");
    if (packet == NULL)
        return dmr_error(DMR_EINVAL);

    if (data_type >= DMR_DATA_TYPE_INVALID) {
        dmr_log_debug("packet: can't encode slot type of invalid data type 0x%02x",
            data_type);
        return dmr_error(DMR_EINVAL);
    }
    if (color_code < 1 || color_code > 15) {
        dmr_log_debug("packet: can't encode slot type of invalid color code %d",
            color_code);
        return dmr_error(DMR_EINVAL);
    }

    uint8_t bytes[3];
    memset(bytes, 0, sizeof(bytes));
    bytes[0] = (color_code << 4) | (data_type & 0x0f);
    dmr_golay_20_8_encode(bytes);

    packet[12] = (packet[12] & 0xc0) | ((bytes[0] >> 2) & 0x3f);
    packet[13] = (packet[13] & 0x0f) | ((bytes[0] << 6) & 0xc0) | ((bytes[1] >> 2) & 0x30);
    packet[19] = (packet[19] & 0xf0) | ((bytes[1] >> 2) & 0x0f);
    packet[20] = (packet[20] & 0x03) | ((bytes[1] << 6) & 0xc0) | ((bytes[2] >> 2) & 0x3c);

    return 0;
}
