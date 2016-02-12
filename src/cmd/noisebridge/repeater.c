#include <common/config.h>
#include <signal.h>
#include <dmr/id.h>
#include <dmr/malloc.h>
#include <dmr/packet.h>
#include <dmr/packetq.h>
#include "common/format.h"
#include "common/scan.h"
#include "common/serial.h"
#include "common/socket.h"
#include "config.h"
#include "http.h"
#include "script.h"
#include "repeater.h"

#if defined(WITH_MBELIB) && defined(HAVE_LIBPORTAUDIO)
#include <portaudio.h>

static PaStream *stream = NULL;
static float stream_data[DMR_DECODED_AMBE_FRAME_SAMPLES];

static int stream_samples(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
    dmr_log_trace("noisebridge: reading %lu samples from stream", frameCount);
    float *data = (float *)userData;
    float *out = (float *)output;
    DMR_UNUSED(input);
    DMR_UNUSED(timeInfo);
    DMR_UNUSED(statusFlags);

    unsigned long i;
    for (i = 0; i < min(frameCount, DMR_DECODED_AMBE_FRAME_SAMPLES); i++) {
        *out++ = *data++;
    }

    return 0;
}

static void stream_write(float *samples, size_t len)
{
    dmr_log_trace("noisebridge: writing %lu samples to stream", len);
    size_t i;
    for (i = 0; i < min(len, DMR_DECODED_AMBE_FRAME_SAMPLES); i++) {
        stream_data[i] = samples[i];
    }
}

#endif

static repeater_t *repeater = NULL;
static volatile bool stopped = false;

repeater_t *load_repeater(void)
{
    return repeater;
}

void kill_handler(int sig)
{
    DMR_UNUSED(sig);
    if (stopped) {
        dmr_log_info("repeater: interrupt, killing");
        exit(2);
    } else {
        dmr_log_info("repeater: interrupt, stopping, hit again to kill");
        stopped = true;
    }
}

route_policy route(proto_t *src, proto_t *dst, dmr_parsed_packet *parsed)
{
    DMR_UNUSED(repeater);

    config_t *config = load_config();
    lua_State *L = config->L;
    route_policy policy = ROUTE_REJECT;

    dmr_log_trace("noisebridge: %s->route()", config->repeater.script);
    lua_getglobal(L, "route"); /* Call script.lua->route() */
    lua_pass_proto(L, src);
    lua_pass_proto(L, dst);
    lua_pass_packet(L, parsed);

    /* Call route(), 3 arguments, 1 return */
    if (lua_pcall(L, 3, 1, 0) != 0) {
        dmr_log_error("noisebridge: %s->route() failed: %s",
            config->repeater.script, lua_tostring(L, -1));
        goto bail;
    }

    /* Retrieve packet */
    switch (lua_type(L, lua_gettop(L))) {
        case LUA_TNIL: {
            // Packet is not modified
            dmr_log_debug("noisebridge: %s->route() returned nil packet (ok)",
                config->repeater.script);
            policy = ROUTE_PERMIT_UNMODIFIED;
            break;
        }
        case LUA_TBOOLEAN: {
            dmr_log_debug("noisebridge: %s->route() returned boolean %s (ok)",
                config->repeater.script, DMR_LOG_BOOL(lua_toboolean(L, 1)));
            policy = lua_toboolean(L, 1) ? ROUTE_PERMIT_UNMODIFIED : ROUTE_REJECT;
            break;
        }
        case LUA_TTABLE: {
            dmr_log_debug("noisebridge: %s->route() returned modified packet (ok)",
                config->repeater.script);
            policy = ROUTE_PERMIT;
            break;
        }
        default: {
            dmr_log_error("noisebridge: %s->route() did not return a table, boolean or nil",
                config->repeater.script);
            policy = ROUTE_REJECT;
            break;
        }
    }

    // Packet is modified
    if (policy == ROUTE_PERMIT) {
        lua_modify_packet(L, parsed);
    }

    lua_pop(L, 1); /* pop returned value from stack */

bail:
    switch (policy) {
    case ROUTE_REJECT:
        dmr_log_trace("noisebridge: %s->route(): reject", config->repeater.script);
        return ROUTE_REJECT;
    case ROUTE_PERMIT:
        dmr_log_trace("noisebridge: %s->route(): permit", config->repeater.script);
        return ROUTE_PERMIT;
    case ROUTE_PERMIT_UNMODIFIED:
        dmr_log_trace("noisebridge: %s->route(): permit (unmodified)", config->repeater.script);
        return ROUTE_PERMIT_UNMODIFIED;
    default:
        dmr_log_trace("noisebridge: %s->route(): invalid %u, rejecting", config->repeater.script, policy);
        return ROUTE_REJECT;
    }
}

static void end_voice_call(dmr_parsed_packet *packet);

static void new_data_call(dmr_parsed_packet *parsed)
{
    dmr_ts ts = parsed->ts;
    repeater_slot_t *rts = &repeater->ts[ts];
    if (rts->state == STATE_VOICE_CALL)
        end_voice_call(parsed);

    rts->state = STATE_DATA_CALL;

    const char *src_name = dmr_id_name(parsed->src_id);
    const char *dst_name = dmr_id_name(parsed->dst_id);
    dmr_log_info("noisebridge: new data call on %s from %u(%s) to %u(%s), flco=%u, repeater=%u",
        dmr_ts_name(ts),
        parsed->src_id, src_name == NULL ? "?" : src_name,
        parsed->dst_id, dst_name == NULL ? "?" : dst_name,
        parsed->flco, parsed->repeater_id);
}

static void end_data_call(dmr_parsed_packet *parsed)
{
    dmr_ts ts = parsed->ts;
    repeater_slot_t *rts = &repeater->ts[ts];
    rts->state = STATE_IDLE;

    const char *src_name = dmr_id_name(parsed->src_id);
    const char *dst_name = dmr_id_name(parsed->dst_id);
    dmr_log_info("noisebridge: end data call on %s from %u(%s) to %u(%s), flco=%u, repeater=%u",
        dmr_ts_name(ts),
        parsed->src_id, src_name == NULL ? "?" : src_name,
        parsed->dst_id, dst_name == NULL ? "?" : dst_name,
        parsed->flco, parsed->repeater_id);
}

static void new_voice_call(dmr_parsed_packet *parsed)
{
    dmr_ts ts = parsed->ts;
    repeater_slot_t *rts = &repeater->ts[ts];
    if (rts->state == STATE_DATA_CALL)
        end_data_call(parsed);

    rts->state = STATE_VOICE_CALL;

    const char *src_name = dmr_id_name(parsed->src_id);
    const char *dst_name = dmr_id_name(parsed->dst_id);
    dmr_log_info("noisebridge: new voice call on %s from %u(%s) to %u(%s), flco=%u, repeater=%u",
        dmr_ts_name(ts),
        parsed->src_id, src_name == NULL ? "?" : src_name,
        parsed->dst_id, dst_name == NULL ? "?" : dst_name,
        parsed->flco, parsed->repeater_id);
}

static void end_voice_call(dmr_parsed_packet *parsed)
{
    dmr_ts ts = parsed->ts;
    repeater_slot_t *rts = &repeater->ts[ts];
    rts->state = STATE_IDLE;

    const char *src_name = dmr_id_name(parsed->src_id);
    const char *dst_name = dmr_id_name(parsed->dst_id);
    dmr_log_info("noisebridge: end voice call on %s from %u(%s) to %u(%s), flco=%u, repeater=%u",
        dmr_ts_name(ts),
        parsed->src_id, src_name == NULL ? "?" : src_name,
        parsed->dst_id, dst_name == NULL ? "?" : dst_name,
        parsed->flco, parsed->repeater_id);
}

int push_proto(proto_t *src, dmr_parsed_packet *parsed)
{
    DMR_ERROR_IF_NULL(src, DMR_EINVAL);
    DMR_ERROR_IF_NULL(parsed, DMR_EINVAL);

    dmr_log_debug("noisebridge: pushing parsed packet");

    dmr_ts ts = parsed->ts;
    repeater_slot_t *rts = &repeater->ts[ts];

    switch (rts->state) {
    case STATE_IDLE:
        switch (parsed->data_type) {
        case DMR_DATA_TYPE_VOICE_LC:
        case DMR_DATA_TYPE_VOICE_SYNC:
        case DMR_DATA_TYPE_VOICE:
            rts->state = STATE_VOICE_CALL;
            rts->src_id = parsed->src_id;
            rts->dst_id = parsed->dst_id;
            rts->stream_id = parsed->stream_id;
            new_voice_call(parsed);
            break;

        case DMR_DATA_TYPE_TERMINATOR_WITH_LC:
            break;

        case DMR_DATA_TYPE_CSBK:
        case DMR_DATA_TYPE_MBC_HEADER:
        case DMR_DATA_TYPE_DATA_HEADER:
            rts->state = STATE_DATA_CALL;
            rts->src_id = parsed->src_id;
            rts->dst_id = parsed->dst_id;
            rts->stream_id = parsed->stream_id;
            new_data_call(parsed);
            break; 

        default:
            break;
        }
        break;
    
    case STATE_DATA_CALL:
        if (parsed->stream_id != rts->stream_id) {
            dmr_log_debug("noisebridge: ignored stream id 0x%08x",
                parsed->stream_id);
            return 0;
        }

        switch (parsed->data_type) {
        case DMR_DATA_TYPE_VOICE_PI:
        case DMR_DATA_TYPE_VOICE_LC:
        case DMR_DATA_TYPE_TERMINATOR_WITH_LC:
            end_data_call(parsed);
            break;

        default:
            break;
        }

        break;

    case STATE_VOICE_CALL:
        if (parsed->stream_id != rts->stream_id) {
            dmr_log_debug("noisebridge: ignored stream id 0x%08x",
                parsed->stream_id);
            return 0;
        }

        switch (parsed->data_type) {
        case DMR_DATA_TYPE_VOICE_LC:
        case DMR_DATA_TYPE_VOICE_SYNC:
        case DMR_DATA_TYPE_VOICE:
            break;

        case DMR_DATA_TYPE_TERMINATOR_WITH_LC:
            end_voice_call(parsed);
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }

    config_t *config = load_config();
    size_t i;
    for (i = 0; i < config->protos; i++) {
        proto_t *dst = config->proto[i];
        if (src == dst)
            continue;


        dmr_parsed_packet *cloned = dmr_malloc(dmr_parsed_packet);
        if (cloned == 0) {
            dmr_log_error("noisebridge: can't clone packet, out of memory");
            return dmr_error(DMR_ENOMEM);
        }
        byte_copy(cloned, parsed, sizeof(dmr_parsed_packet));

        int ret = 0;
        switch (route(src, dst, cloned)) {
        case ROUTE_PERMIT:
        case ROUTE_PERMIT_UNMODIFIED:
            switch (dst->type) {
            case DMR_PROTOCOL_HOMEBREW: {
                    dmr_homebrew *homebrew = (dmr_homebrew *)dst->instance;
                    ret = dmr_homebrew_send(homebrew, cloned);
                    break;
                }                  
            case DMR_PROTOCOL_MMDVM: {
                    dmr_mmdvm *mmdvm = (dmr_mmdvm *)dst->instance;
                    ret = dmr_mmdvm_send(mmdvm, cloned);
                    break;
                }
            default:
                break;
            }
            break;
        case ROUTE_REJECT:
        default:
            dmr_log_debug("noisebridge: route %s->%s rejected",
                src->name, dst->name);
            break;
        }
        if (ret != 0) {
            dmr_log_error("noisebridge: send to %s failed: %s",
                dst->name, dmr_error_get());
        }
    }

    return 0;
}

/* poll_* is triggered after a proto becomes readable and checks the received
 * parsed packet queue for new frames. */

int poll_proto_homebrew(dmr_io *io, void *homebrewptr, int fd)
{
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(homebrewptr, DMR_EINVAL);

    config_t *config = load_config();
    dmr_homebrew *homebrew = (dmr_homebrew *)homebrewptr;
    socket_t *sock = (socket_t *)homebrew->sock;
    if (sock->fd != fd)
        return 0;

    size_t i;
    for (i = 0; i < config->protos; i++) {
        proto_t *proto = config->proto[i];
        if (proto->fd == fd) {
            /* push packets */
            for (;;) {
                dmr_parsed_packet *parsed = NULL;
                dmr_packetq_shift(homebrew->rxq, &parsed);
                if (parsed == NULL)
                    break;
                push_proto(proto, parsed);
            }
        }
    }
    
    return 0;
}

int poll_proto_mmdvm(dmr_io *io, void *mmdvmptr, int fd)
{
    DMR_ERROR_IF_NULL(io, DMR_EINVAL);
    DMR_ERROR_IF_NULL(mmdvmptr, DMR_EINVAL);

    dmr_log_debug("noisebridge: poll after mmdvm read");

    config_t *config = load_config();
    dmr_mmdvm *mmdvm = (dmr_mmdvm *)mmdvmptr;
    serial_t *serial = (serial_t *)mmdvm->serial;
    if (serial->fd != fd)
        return 0;

    proto_t *src = NULL;
    size_t i;
    for (i = 0; i < config->protos; i++) {
        proto_t *proto = config->proto[i];
        if (proto->fd == fd) {
            src = proto;
            break;
        }
    }

    if (src == NULL) {
        dmr_log_error("noisebridge: proto for fd %d not found!", fd);
        return 1;
    }

    /* push packets */
    for (i = 0;; i++) {
        dmr_parsed_packet *parsed = NULL;
        dmr_packetq_shift(mmdvm->rxq, &parsed);
        if (parsed == NULL)
            break;
        push_proto(src, parsed);
    }
    
    dmr_log_debug("noisebridge: poll after mmdvm read relayed %lu packets", i);
    return 0;
}

int init_proto_homebrew(config_t *config, proto_t *proto)
{
    DMR_UNUSED(config);

    int ret = 0;
    dmr_log_info("noisebridge: connect to homebrew repeater");
    dmr_homebrew *homebrew = dmr_homebrew_new(
        proto->settings.homebrew.repeater_id,
        proto->settings.homebrew.peer_ip, proto->settings.homebrew.peer_port,
        proto->settings.homebrew.bind_ip, proto->settings.homebrew.bind_port);
    if (homebrew == NULL) {
        return dmr_error(DMR_LASTERROR);
    }

    DMR_ERROR_IF_NULL(homebrew->config.call = dmr_strdup(homebrew, proto->settings.homebrew.call), DMR_ENOMEM);
    homebrew->config.repeater_id = proto->settings.homebrew.repeater_id;
    homebrew->config.rx_freq     = proto->settings.homebrew.rx_freq;
    homebrew->config.tx_freq     = proto->settings.homebrew.tx_freq;
    homebrew->config.tx_power    = proto->settings.homebrew.tx_power;
    homebrew->config.altitude    = proto->settings.homebrew.height;
    homebrew->config.latitude    = proto->settings.homebrew.latitude;
    homebrew->config.longitude   = proto->settings.homebrew.longitude;

    dmr_log_debug("ret: %d", ret);
    if ((ret = dmr_homebrew_auth(homebrew, proto->settings.homebrew.auth)) != 0) {
        dmr_log_critical("homebrew: authentication failed: %s", dmr_error_get());
        dmr_log_debug("ret: %d", ret);
        goto bail;
    }

    socket_t *sock  = (socket_t *)homebrew->sock;
    proto->protocol = dmr_homebrew_protocol;
    proto->instance = homebrew;
    proto->fd       = sock->fd;

    /* Listen in on received packets */
    dmr_io_reg_read(repeater->io, proto->fd, poll_proto_homebrew, homebrew, false); 

bail:
    dmr_log_debug("ret: %d", ret);
    return ret;
}

int init_proto_mmdvm(config_t *config, proto_t *proto)
{
    DMR_UNUSED(config);
    DMR_UNUSED(proto);

    int ret = 0;
    dmr_log_info("noisebridge: connect to MMDVM modem \"%s\" on %s (%d baud)",
        proto->name,
        proto->settings.mmdvm.port,
        proto->settings.mmdvm.baud);
    dmr_mmdvm *mmdvm = dmr_mmdvm_new(
        proto->settings.mmdvm.port,
        proto->settings.mmdvm.baud,
        proto->settings.mmdvm.model);

    mmdvm->rx_freq = proto->settings.mmdvm.rx_freq;
    mmdvm->tx_freq = proto->settings.mmdvm.tx_freq;

    if (mmdvm == NULL) {
        dmr_log_critical("repeater: mmdvm open failed: %s", dmr_error_get());
        return dmr_error(DMR_LASTERROR);
    }

    serial_t *serial = (serial_t *)mmdvm->serial;
    proto->protocol  = dmr_mmdvm_protocol;
    proto->instance  = mmdvm;
    proto->fd        = serial->fd;

    /* Listen in on received packets */
    dmr_io_reg_read(repeater->io, proto->fd, poll_proto_mmdvm, mmdvm, false); 

    return ret;
}

int stop_repeater(dmr_io *io, void *unused, int sig)
{
    DMR_UNUSED(unused);
    DMR_UNUSED(sig);
    dmr_log_info("noisebridge: received interrupt, stopping");
    return dmr_io_close(io);
}

int slot_timer(dmr_io *io, void *unused)
{
    DMR_UNUSED(io);
    DMR_UNUSED(unused);

    config_t *config = load_config();
    dmr_ts ts;
    for (ts = 0; ts < DMR_TS_INVALID; ts++) {
        repeater_slot_t *rts = &repeater->ts[ts];

        if (rts->state == STATE_IDLE)
            continue;

        struct timeval delta;
        timersub(&io->wallclock, &rts->last_frame_received, &delta);
        uint32_t ms = dmr_time_ms_since(delta);
        if (ms > config->repeater.timeout) {
            dmr_log_info("noisebridge: timeout on %s after %ums",
                dmr_ts_name(ts), ms);
            rts->state = STATE_IDLE;
        }
    }

    return 0;
}

int init_repeater(void)
{
    int ret = 0;
    config_t *config = load_config();
    repeater = dmr_malloc(repeater_t);
    if (repeater == NULL) {
        dmr_log_critical("noisebridge: out of memory");
        ret = DMR_OOM();
        goto bail;
    }
    if ((repeater->io = dmr_io_new()) == NULL) {
        dmr_log_critical("noisebridge: out of memory");
        ret = DMR_OOM();
        goto bail;
    }

    /* Timeouts on timeslots */
    struct timeval slottimeout;
    slottimeout.tv_sec = (config->repeater.timeout / 1000);
    slottimeout.tv_usec = ((config->repeater.timeout % 1000) * 1000);
    dmr_io_reg_timer(repeater->io, slottimeout, slot_timer, NULL, false);

    /* Close repeater on SIGINT (^C) */
    dmr_io_reg_signal(repeater->io, SIGINT, stop_repeater, NULL, true);

    /* Default timeout: duration of one DMR frame on-air */
    repeater->io->timeout.tv_sec = 0;
    repeater->io->timeout.tv_usec = 30 * 1000;

    dmr_log_debug("noisebridge: init repeater");

    uint8_t i;
    for (i = 0; i < config->protos; i++) {
        proto_t *proto = config->proto[i];
        switch (proto->type) {
            case DMR_PROTOCOL_HOMEBREW: {
                ret = init_proto_homebrew(config, proto);
                break;
            }
            case DMR_PROTOCOL_MBE: {
                ret = 0; // init_proto_mbe(config, proto);
                break;
            }
            case DMR_PROTOCOL_MMDVM: {
                ret = init_proto_mmdvm(config, proto);
                break;
            }
            default: {
                dmr_log_critical("noisebridge: unknown proto %u", proto->type);
                ret = -1;
                goto bail;
            }
        }

        if (ret != 0) {
            dmr_log_critical("noisebridge: protocol init failed: %s", dmr_error_get());
            goto bail;
        }

        dmr_log_debug("ret: %d", ret);
        if (proto->instance == NULL) {
            dmr_log_critical("noisebridge: no instance returned from %s", proto->name);
            ret = -1;
            goto bail;
        }

        if ((ret = dmr_io_add_protocol(repeater->io, proto->protocol, proto->instance)) != 0) {
            dmr_log_critical("noisebridge: io add protocol failed: %s", dmr_error_get());
            goto bail;
        }
    }

    goto done;

bail:
    dmr_free(repeater);
    repeater = NULL;

done:
    return ret;
}

int loop_repeater(void)
{
    int ret = 0;
    config_t *config = load_config();

    dmr_log_info("noisebridge: running repeater");
    if ((ret = dmr_io_loop(repeater->io)) != 0) {
        dmr_log_critical("noisebridge: io loop returned error");
    }

    dmr_log_info("noisebridge: stopping repeater");
    stop_http();

    size_t i;
    for (i = 0; i < config->protos; i++) {
        proto_t *proto = config->proto[i];
        if (proto->instance == NULL)
            continue;

        switch (proto->type) {
        case DMR_PROTOCOL_MMDVM:
            dmr_mmdvm_close((dmr_mmdvm *)proto->instance);
            break;
        case DMR_PROTOCOL_HOMEBREW:
            dmr_homebrew_close((dmr_homebrew *)proto->instance);
            break;
        default:
            break;
        }

        dmr_free(proto->instance);
        proto->instance = NULL;
    }

    if (repeater->io != NULL)
        dmr_io_free(repeater->io);

    dmr_free(repeater);
    repeater = NULL;
    return ret;
}
