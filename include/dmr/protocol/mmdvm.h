#ifndef _DMR_PROTOCOL_MMDVM
#define _DMR_PROTOCOL_MMDVM

#include <inttypes.h>
#include <dmr/io.h>
#include <dmr/packet.h>
#include <dmr/packetq.h>
#include <dmr/protocol.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define DMR_MMDVM_FRAME_START   	0xE0

#define DMR_MMDVM_DMR_TS        	0x80
#define DMR_MMDVM_DMR_DATA_SYNC 	0x40
#define DMR_MMDVM_DMR_VOICE_SYNC 	0x20

/* Maximum size of a single MMDVM frame */
#define DMR_MMDVM_FRAME_MAX         0xff

typedef enum {
    DMR_MMDVM_GET_VERSION   	= 0x00,
    DMR_MMDVM_GET_STATUS    	= 0x01,
    DMR_MMDVM_SET_CONFIG    	= 0x02,
    DMR_MMDVM_SET_MODE      	= 0x03,
    DMR_MMDVM_SET_RF_CONFIG 	= 0x04,

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

typedef struct dmr_mmdvm_frameq {
    dmr_mmdvm_frame                frame;
    STAILQ_ENTRY(dmr_mmdvm_frameq) entries;
} dmr_mmdvm_frameq;

typedef struct {
    char             *port;
    int              baud;
    dmr_mmdvm_model  model;
    /* these come from the modem */
    int              protocol_version;
    char             *description;
    uint8_t          modes;
    uint8_t          status;
    uint8_t          buffer_size[4];     /* see dmr_mmdvm_bufsize for offsets */
    bool             tx_on, rx_on;
    /* private */
    void             *serial;            /* internal serial struct */
    dmr_packetq      *rxq;               /* packets received from the modem */
    dmr_packetq      *txq;               /* packets to be transmitted by the modem */
    dmr_mmdvm_frame  frame;
    size_t           pos;
    STAILQ_HEAD(,dmr_mmdvm_frameq) frameq;
} dmr_mmdvm;

#if defined(__cplusplus)
}
#endif

#endif
