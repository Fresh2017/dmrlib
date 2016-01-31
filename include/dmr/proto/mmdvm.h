/**
 * @file
 * @brief Multi-Mode Digital Voice Modem protocol. Protocol implementation as
 * specified by G4KLX.
 */
#ifndef _DMR_PROTO_MMDVM
#define _DMR_PROTO_MMDVM

#include <dmr/config.h>
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

#include <dmr/type.h>
#include <dmr/thread.h>
#include <dmr/proto.h>
#include <dmr/thread.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define DMR_MMDVM_BAUD_RATE     115200

#define DMR_MMDVM_FRAME_START   0xE0

#define DMR_MMDVM_GET_VERSION   0x00
#define DMR_MMDVM_GET_STATUS    0x01
#define DMR_MMDVM_SET_CONFIG    0x02
#define DMR_MMDVM_SET_MODE      0x03
#define DMR_MMDVM_SET_RF_CONFIG 0x04

#define DMR_MMDVM_DSTAR_HEADER  0x10
#define DMR_MMDVM_DSTAR_DATA    0x11
#define DMR_MMDVM_DSTAR_LOST    0x12
#define DMR_MMDVM_DSTAR_EOT     0x13

#define DMR_MMDVM_DMR_DATA1     0x18
#define DMR_MMDVM_DMR_LOST1     0x19
#define DMR_MMDVM_DMR_DATA2     0x1a
#define DMR_MMDVM_DMR_LOST2     0x1b
#define DMR_MMDVM_DMR_SHORTLC   0x1c
#define DMR_MMDVM_DMR_START     0x1d

#define DMR_MMDVM_DMR_TS        0x80
#define DMR_MMDVM_DMR_DATA_SYNC 0x40
#define DMR_MMDVM_DMR_VOICE_SYNC 0x20

#define DMR_MMDVM_YSF_DATA      0x20
#define DMR_MMDVM_YSF_LOST      0x21

#define DMR_MMDVM_ACK           0x70
#define DMR_MMDVM_NAK           0x7f

#define DMR_MMDVM_DUMP          0xf0
#define DMR_MMDVM_DEBUG1        0xf1
#define DMR_MMDVM_DEBUG2        0xf2
#define DMR_MMDVM_DEBUG3        0xf3
#define DMR_MMDVM_DEBUG4        0xf4
#define DMR_MMDVM_DEBUG5        0xf5
#define DMR_MMDVM_SAMPLES       0xf8

#define DMR_MMDVM_DELAY_MS      200UL
#define DMR_MMDVM_DELAY_US      (DMR_MMDVM_DELAY_MS*1000UL)

#define DMR_MMDVM_MODE_IDLE     0x00
#define DMR_MMDVM_MODE_DSTAR    0x01
#define DMR_MMDVM_MODE_DMR      0x02
#define DMR_MMDVM_MODE_YSF      0x03
#define DMR_MMDVM_MODE_INVALID  0xff

#define DMR_MMDVM_TAG_HEADER    0x00
#define DMR_MMDVM_TAG_DATA      0x01
#define DMR_MMDVM_TAG_LOST      0x02
#define DMR_MMDVM_TAG_EOT       0x03

typedef enum {
    DMR_MMDVM_MODEL_GENERIC = 0x00,
    DMR_MMDVM_MODEL_G4KLX   = 0x01,
    DMR_MMDVM_MODEL_DVMEGA  = 0x02,
    DMR_MMDVM_MODEL_INVALID = 0xff,
} dmr_mmdvm_model_t;

/* Model feature flags */
#define DMR_MMDVM_HAS_NONE               (0)
#define DMR_MMDVM_HAS_GET_VERSION        (1 << 0)
#define DMR_MMDVM_HAS_GET_STATUS         (1 << 1)
#define DMR_MMDVM_HAS_SET_CONFIG         (1 << 2)
#define DMR_MMDVM_HAS_SET_MODE           (1 << 3)
#define DMR_MMDVM_HAS_SET_RF_CONFIG      (1 << 4)
#define DMR_MMDVM_HAS_DSTAR              (1 << 8)
#define DMR_MMDVM_HAS_DSTAR_EOT          (1 << 9)
#define DMR_MMDVM_HAS_DMR                (1 << 10)
#define DMR_MMDVM_HAS_DMR_EOT            (1 << 11)
#define DMR_MMDVM_HAS_DMR_SHORT_LC       (1 << 12)
#define DMR_MMDVM_HAS_YSF                (1 << 14)
#define DMR_MMDVM_HAS_YSF_EOT            (1 << 15)
#define DMR_MMDVM_HAS_ALL               ((1 << 16) - 1)

typedef enum {
    dmr_mmdvm_ok,
    dmr_mmdvm_timeout,
    dmr_mmdvm_error
} dmr_mmdvm_response_t;

typedef struct __attribute__((packed)) {
    uint8_t frame_start;
    uint8_t length;
    uint8_t command;
} dmr_mmdvm_header_t;

typedef struct __attribute__((packed)) {
    dmr_mmdvm_header_t header;
    uint8_t            modes;
    uint8_t            state;
    uint8_t            flags;
    uint8_t            dstar_buffers;
    uint8_t            dmr_buffers[2];
    uint8_t            ysf_buffers;
} dmr_mmdvm_status_t;

typedef struct __attribute__((packed)) {
    dmr_mmdvm_header_t header;
    uint8_t            version;
    char               *description;
} dmr_mmdvm_version_t;

typedef struct {
    dmr_proto_t       proto;
    dmr_mmdvm_model_t model;
    uint16_t          flags;
    int               error;
    long              baud;
    char              *port;
    void              *serial;      /* internal serial port struct */
    uint8_t           buffer[200];  /* receive buffer */
    dmr_packet_t      **queue;      /* send buffer */
    size_t            queue_size;
    size_t            queue_used;
    dmr_mutex_t       *queue_lock;
    bool              rx;
    bool              adc_overflow;
    uint8_t           version;
    char              *firmware;
    uint8_t           last_mode;
    struct timeval    *last_status_received;
    struct timeval    *last_status_requested;
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
        uint32_t        stream_id;
    } dmr_ts[3];
    dmr_ts_t last_dmr_ts;
} dmr_mmdvm_t;

DMR_API extern dmr_mmdvm_t *dmr_mmdvm_open(char *port, long baud, dmr_mmdvm_model_t model);
DMR_API extern int dmr_mmdvm_sync(dmr_mmdvm_t *modem);
DMR_API extern int dmr_mmdvm_poll(dmr_mmdvm_t *modem);
DMR_API extern int dmr_mmdvm_send(dmr_mmdvm_t *modem, dmr_packet_t *packet);
DMR_API extern int dmr_mmdvm_queue(dmr_mmdvm_t *modem, dmr_packet_t *packet);
DMR_API extern dmr_packet_t *dmr_mmdvm_queue_shift(dmr_mmdvm_t *modem);
DMR_API extern dmr_mmdvm_response_t dmr_mmdvm_get_response(dmr_mmdvm_t *modem, uint8_t *length, unsigned int timeout_ms, int retries);
DMR_API extern bool dmr_mmdvm_get_status(dmr_mmdvm_t *modem);
DMR_API extern bool dmr_mmdvm_get_version(dmr_mmdvm_t *modem);
DMR_API extern bool dmr_mmdvm_set_config(dmr_mmdvm_t *modem);
DMR_API extern bool dmr_mmdvm_set_mode(dmr_mmdvm_t *modem, uint8_t mode);
DMR_API extern bool dmr_mmdvm_set_rf_config(dmr_mmdvm_t *modem, uint32_t rx_freq, uint32_t tx_freq);
DMR_API extern int dmr_mmdvm_free(dmr_mmdvm_t *modem);
DMR_API extern int dmr_mmdvm_close(dmr_mmdvm_t *modem);

#if defined(__cplusplus)
}
#endif

#endif // _DMR_PROTO_MMDVM
