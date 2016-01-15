/**
 * @file
 * @brief Multi-Mode Digital Voice Modem protocol. Protocol implementation as
 * specified by G4KLX.
 */
#ifndef _DMR_PROTO_MMDVM
#define _DMR_PROTO_MMDVM

#include <dmr/platform.h>

#include <inttypes.h>
#include <stdbool.h>
#if defined(DMR_PLATFORM_WINDOWS)
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#endif

#include <dmr/buffer/ring.h>
#include <dmr/type.h>
#include <dmr/thread.h>
#include <dmr/proto.h>
#include <dmr/thread.h>
#include <dmr/serial.h>

#define DMR_MMDVM_BAUD_RATE     115200

#define DMR_MMDVM_FRAME_START   0xE0U

#define DMR_MMDVM_GET_VERSION   0x00U
#define DMR_MMDVM_GET_STATUS    0x01U
#define DMR_MMDVM_SET_CONFIG    0x02U
#define DMR_MMDVM_SET_MODE      0x03U

#define DMR_MMDVM_DSTAR_HEADER  0x10U
#define DMR_MMDVM_DSTAR_DATA    0x11U
#define DMR_MMDVM_DSTAR_LOST    0x12U
#define DMR_MMDVM_DSTAR_EOT     0x13U

#define DMR_MMDVM_DMR_DATA1     0x18U
#define DMR_MMDVM_DMR_LOST1     0x19U
#define DMR_MMDVM_DMR_DATA2     0x1AU
#define DMR_MMDVM_DMR_LOST2     0x1BU
#define DMR_MMDVM_DMR_SHORTLC   0x1CU
#define DMR_MMDVM_DMR_START     0x1DU

#define DMR_MMDVM_DMR_TS        0x80U
#define DMR_MMDVM_DMR_DATA_SYNC 0x40U
#define DMR_MMDVM_DMR_VOICE_SYNC 0x20U

#define DMR_MMDVM_YSF_DATA      0x20U
#define DMR_MMDVM_YSF_LOST      0x21U

#define DMR_MMDVM_ACK           0x70U
#define DMR_MMDVM_NAK           0x7FU

#define DMR_MMDVM_DUMP          0xF0U
#define DMR_MMDVM_DEBUG1        0xF1U
#define DMR_MMDVM_DEBUG2        0xF2U
#define DMR_MMDVM_DEBUG3        0xF3U
#define DMR_MMDVM_DEBUG4        0xF4U
#define DMR_MMDVM_DEBUG5        0xF5U
#define DMR_MMDVM_SAMPLES       0xF8U

#define DMR_MMDVM_DELAY_MS      200UL
#define DMR_MMDVM_DELAY_US      (DMR_MMDVM_DELAY_MS*1000UL)

#define DMR_MMDVM_MODE_IDLE     B00000000
#define DMR_MMDVM_MODE_DSTAR    B00000001
#define DMR_MMDVM_MODE_DMR      B00000010
#define DMR_MMDVM_MODE_YSF      B00000011
#define DMR_MMDVM_MODE_INVALID  B11111111

#define DMR_MMDVM_TAG_HEADER    0b00
#define DMR_MMDVM_TAG_DATA      0b01
#define DMR_MMDVM_TAG_LOST      0b10
#define DMR_MMDVM_TAG_EOT       0b11

/*
typedef enum {
    dmr_mmdvm_timeout,
    dmr_mmdvm_error,
    dmr_mmdvm_unknown,
    dmr_mmdvm_get_status,
    dmr_mmdvm_get_version,
    dmr_mmdvm_dstar_header,
    dmr_mmdvm_dstar_data,
    dmr_mmdvm_dstar_eot,
    dmr_mmdvm_dstar_lost,
    dmr_mmdvm_ack,
    dmr_mmdvm_nak,
    dmr_mmdvm_dump,
    dmr_mmdvm_debug1,
    dmr_mmdvm_debug2,
    dmr_mmdvm_debug3,
    dmr_mmdvm_debug4,
    dmr_mmdvm_debug5
} dmr_mmdvm_response_type_t;
*/

/* Sync before connecting running a GET_VERSION command. */
#define DMR_MMDVM_FLAG_SYNC             (1U << 1)
/* Fixup voice frames replacing EMB and headers. */
#define DMR_MMDVM_FLAG_FIXUP_VOICE      (1U << 2)
/* Fixup voice frames adding a LC before the voice stream. */
#define DMR_MMDVM_FLAG_FIXUP_VOICE_LC   (1U << 3)

typedef enum {
    dmr_mmdvm_ok,
    dmr_mmdvm_timeout,
    dmr_mmdvm_error
} dmr_mmdvm_response_t;

typedef struct {
    dmr_proto_t  proto;
    dmr_serial_t fd;
    uint16_t     flag;
    int          error;
    long         baud;
    char         *port;
    uint8_t      buffer[200];
    bool         rx;
    bool         adc_overflow;
    uint8_t      version;
    char         *firmware;
    uint8_t      last_mode;
    struct {
        bool    enable_dstar;
        bool    enable_dmr;
        bool    enable_ysf;
        bool    rx_invert;
        bool    tx_invert;
        bool    ptt_invert;
        uint8_t tx_delay;
        uint8_t rx_level;
        uint8_t tx_level;
        uint8_t color_code;
    } config;
    struct {
        uint8_t dstar;
        uint8_t dmr_ts1;
        uint8_t dmr_ts2;
        uint8_t ysf;
    } space;

    // Book keeping
    struct timeval last_packet_received;
    struct {
        dmr_data_type_t last_data_type;
        struct timeval  last_packet_received;
        uint32_t        last_sequence;
        uint8_t         last_voice_frame;
    } dmr_ts[3];
    dmr_ts_t last_dmr_ts;
} dmr_mmdvm_t;

extern dmr_mmdvm_t *dmr_mmdvm_open(char *port, long baud, uint16_t flag, size_t buffer_sizes);
extern int dmr_mmdvm_sync(dmr_mmdvm_t *modem);
extern int dmr_mmdvm_poll(dmr_mmdvm_t *modem);
extern dmr_mmdvm_response_t dmr_mmdvm_get_response(dmr_mmdvm_t *modem, uint8_t *length, struct timeval *timeout, int retries);
extern int dmr_mmdvm_free(dmr_mmdvm_t *modem);
extern int dmr_mmdvm_close(dmr_mmdvm_t *modem);

#endif // _DMR_PROTO_MMDVM
