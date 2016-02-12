#ifndef _DMR_PROTOCOL_MMDVM
#define _DMR_PROTOCOL_MMDVM

#include <inttypes.h>
#include <dmr/io.h>
#include <dmr/packet.h>
#include <dmr/packetq.h>
#include <dmr/raw.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define DMR_MMDVM_FRAME_START   	0xE0

#define DMR_MMDVM_DMR_TS        	0x80
#define DMR_MMDVM_DMR_DATA_SYNC 	0x40
#define DMR_MMDVM_DMR_VOICE_SYNC 	0x20

/** Maximum size of a single MMDVM frame. */
#define DMR_MMDVM_FRAME_MAX         0xff

/** Default baud rate. */
#define DMR_MMDVM_BAUD              115200

typedef enum {
    DMR_MMDVM_GET_VERSION   	= 0x00,
    DMR_MMDVM_GET_STATUS    	= 0x01,
    DMR_MMDVM_SET_CONFIG    	= 0x02,
    DMR_MMDVM_SET_MODE      	= 0x03,
    DMR_MMDVM_SET_RF_CONFIG 	= 0x04,
    DMR_MMDVM_SET_MAX           = 0x0f,

    DMR_MMDVM_DSTAR_HEADER  	= 0x10,
    DMR_MMDVM_DSTAR_DATA    	= 0x11,
    DMR_MMDVM_DSTAR_LOST    	= 0x12,
    DMR_MMDVM_DSTAR_EOT     	= 0x13,

    DMR_MMDVM_DMR_DATA1     	= 0x18,
    DMR_MMDVM_DMR_LOST1     	= 0x19,
    DMR_MMDVM_DMR_DATA2     	= 0x1a,
    DMR_MMDVM_DMR_LOST2     	= 0x1b,
    DMR_MMDVM_DMR_SHORTLC   	= 0x1c,
    DMR_MMDVM_DMR_START     	= 0x1d,

    DMR_MMDVM_YSF_DATA      	= 0x20,
    DMR_MMDVM_YSF_LOST      	= 0x21,

    DMR_MMDVM_ACK           	= 0x70,
    DMR_MMDVM_NAK           	= 0x7f,

    DMR_MMDVM_DUMP          	= 0xf0,
    DMR_MMDVM_DEBUG1        	= 0xf1,
    DMR_MMDVM_DEBUG2        	= 0xf2,
    DMR_MMDVM_DEBUG3        	= 0xf3,
    DMR_MMDVM_DEBUG4        	= 0xf4,
    DMR_MMDVM_DEBUG5        	= 0xf5,
    DMR_MMDVM_SAMPLES       	= 0xf8,
} dmr_mmdvm_command;

typedef enum {
    DMR_MMDVM_INVALID_VALUE = 0x01,
    DMR_MMDVM_WRONG_MODE,
    DMR_MMDVM_COMMAND_TOO_LONG,
    DMR_MMDVM_DATA_INCORRECT,
    DMR_MMDVM_NOT_ENOUGH_SPACE
} dmr_mmdvm_reason;

typedef enum {
    DMR_MMDVM_MODE_IDLE     = 0x00,
    DMR_MMDVM_MODE_DSTAR    = 0x01,
    DMR_MMDVM_MODE_DMR      = 0x02,
    DMR_MMDVM_MODE_YSF      = 0x03,
    DMR_MMDVM_MODE_INVALID  = 0xff
} dmr_mmdvm_mode;

typedef enum {
    DMR_MMDVM_STATE_IDLE      = 0x00,
    DMR_MMDVM_STATE_DSTAR     = 0x01,
    DMR_MMDVM_STATE_DMR       = 0x02,
    DMR_MMDVM_STATE_YSF       = 0x03,
    DMR_MMDVM_STATE_CALIBRATE = 0x63
} dmr_mmdvm_state;

typedef enum {
    DMR_MMDVM_MODEL_G4KLX = 0,
    DMR_MMDVM_MODEL_DVMEGA,
    DMR_MMDVM_MODELS
} dmr_mmdvm_model;

#define DMR_MMDVM_MODEL_DEFAULT DMR_MMDVM_MODEL_G4KLX

typedef uint8_t dmr_mmdvm_frame[DMR_MMDVM_FRAME_MAX];

typedef enum {
    DMR_MMDVM_BUFSIZE_DSTAR = 0,
    DMR_MMDVM_BUFSIZE_DMR_TS1,
    DMR_MMDVM_BUFSIZE_DMR_TS2,
    DMR_MMDVM_BUFSIZE_YSF
} dmr_mmdvm_bufsize;

typedef enum {
    DMR_MMDVM_INVERT_RX,
    DMR_MMDVM_INVERT_TX,
    DMR_MMDVM_INVERT_TRANSMIT
} dmr_mmdvm_invert;

#define DMR_MMDVM_ACK_BUF_MAX 0xff

typedef struct {
    char              *id;               /* ident */
    char              *port;
    int               baud;
    dmr_mmdvm_model   model;
    dmr_color_code    color_code;
    /* these come from the modem */
    int               protocol_version;
    char              *description;
    uint8_t           modes;
    uint8_t           status;
    uint8_t           buffer_size[4];     /* see dmr_mmdvm_bufsize for offsets */
    uint32_t          rx_freq, tx_freq;
    bool              rx_on, tx_on;
    /* private */
    uint32_t          sent;               /* sent DMR frame counter */
    volatile bool     ack[DMR_MMDVM_SET_MAX]; /* ack buffer for set commands */
    volatile uint8_t  set[DMR_MMDVM_SET_MAX]; /* set counter for set commands */
    bool              set_rf_config_sent; /* sent RF config frame */
    bool              set_rf_config_recv; /* received an RF config ACK/NAK */
    bool              started;
    void              *serial;            /* internal serial struct */
    dmr_packetq       *rxq;               /* packets received from the modem */
    dmr_packetq       *txq;               /* packets to be transmitted by the modem */
    dmr_mmdvm_frame   frame;              /* raw frame currently being decoded */
    dmr_rawq          *trq;               /* raw frame to be transmitted to the modem */
    size_t            pos;
} dmr_mmdvm;


extern const char *dmr_mmdvm_reason_name(dmr_mmdvm_reason reason);

extern const char *dmr_mmdvm_model_name(dmr_mmdvm_model model);

/** Setup serial communications with an MMDVM modem. */
extern dmr_mmdvm *dmr_mmdvm_new(const char *port, int baud, dmr_mmdvm_model model, dmr_color_code color_code);

/** Start serial communications with an MMDVM modem. */
extern int dmr_mmdvm_start(dmr_mmdvm *mmdvm);

/** Parse an MMDVM frame. */
extern int dmr_mmdvm_parse_frame(dmr_mmdvm *mmdvm, dmr_mmdvm_frame frame, dmr_parsed_packet **parsed_out);

/** Read from the serial line.
 * This also processes communications with the MMDVM modem, if the
 * received frame does not contain a DMR packet, the function will set the
 * destination packet pointer to NULL. */
extern int dmr_mmdvm_read(dmr_mmdvm *mmdvm, dmr_parsed_packet **packet_out);

/** Send a get status command to the modem.
 * The results will be collected in the modem structure. */
extern int dmr_mmdvm_get_status(dmr_mmdvm *mmdvm);

/** Send a get version command to the modem.
 * The results will be collected in the modem structure. */
extern int dmr_mmdvm_get_version(dmr_mmdvm *mmdvm);

/** Send configuration to the modem. */
extern int dmr_mmdvm_set_config(dmr_mmdvm *mmdvm, uint8_t invert, uint8_t mode, uint8_t delay_ms, dmr_mmdvm_state state, uint8_t rx_level, uint8_t tx_level, dmr_color_code color_code);

/** Tune the modem to a frequency in Hz. */
extern int dmr_mmdvm_set_rf_config(dmr_mmdvm *mmdvm, uint32_t rx_freq, uint32_t tx_freq);

/** Set modem mode. */
extern int dmr_mmdvm_set_mode(dmr_mmdvm *mmdvm, dmr_mmdvm_mode mode);

/** Send a DMR frame to the modem. */
extern int dmr_mmdvm_send(dmr_mmdvm *mmdvm, dmr_parsed_packet *packet);

/** Send a raw buffer to the mdoem. */
extern int dmr_mmdvm_send_buf(dmr_mmdvm *mmdmv, uint8_t *buf, size_t len);

/** Send a raw packet to the modem. */
extern int dmr_mmdvm_send_raw(dmr_mmdvm *mmdvm, dmr_raw *raw);

/** Write to the serial line.
 * This reads from the internal send buffer, if there are no frames queued to
 * be sent, the function returns immediately */
extern int dmr_mmdvm_write(dmr_mmdvm *mmdvm);

/** Send a parsed packet to the serial line. */
extern int dmr_mmdvm_send(dmr_mmdvm *mmdvm, dmr_parsed_packet *parsed);

/** Send a raw buffer to the serial line. */
extern int dmr_mmdvm_send_raw(dmr_mmdvm *mmdvm, dmr_raw *raw);

/** Stop communicaitons with the modem.
 * This also frees the mmdvm (internal) structure. */
extern int dmr_mmdvm_close(dmr_mmdvm *mmdvm);

#include <dmr/protocol.h>

/** Protocol specification */
extern dmr_protocol dmr_mmdvm_protocol;

#if defined(DMR_TRACE)
#define DMR_MM_TRACE(fmt,...) dmr_log_trace("%s: "fmt, mmdvm->id, ##__VA_ARGS__)
#else
#define DMR_MM_TRACE(fmt,...)
#endif
#if defined(DMR_DEBUG)
#define DMR_MM_DEBUG(fmt,...) dmr_log_debug("%s: "fmt, mmdvm->id, ##__VA_ARGS__)
#else
#define DMR_MM_DEBUG(fmt,...)
#endif
#define DMR_MM_INFO(fmt,...)  dmr_log_info ("%s: "fmt, mmdvm->id, ##__VA_ARGS__)
#define DMR_MM_WARN(fmt,...)  dmr_log_warn ("%s: "fmt, mmdvm->id, ##__VA_ARGS__)
#define DMR_MM_ERROR(fmt,...) dmr_log_error("%s: "fmt, mmdvm->id, ##__VA_ARGS__)
#define DMR_MM_FATAL(fmt,...) dmr_log_critical("%s: "fmt, mmdvm->id, ##__VA_ARGS__)

#if defined(__cplusplus)
}
#endif

#endif
