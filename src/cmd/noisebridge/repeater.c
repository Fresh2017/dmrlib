#include <dmr/config.h>
#include <signal.h>
#include "config.h"
#include "script.h"

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

static dmr_repeater_t *repeater = NULL;
static dmr_cond_t *active = NULL;
static dmr_mutex_t *active_lock = NULL;
static volatile bool stopped = false;

void kill_handler(int sig)
{
    DMR_UNUSED(sig);
    if (stopped) {
        dmr_log_info("repeater: interrupt, killing");
        exit(2);
    } else {
        dmr_log_info("repeater: interrupt, stopping, hit again to kill");
        stopped = true;
        dmr_cond_signal(active);
    }
}

dmr_route_t route(dmr_repeater_t *repeater, dmr_proto_t *src, dmr_proto_t *dst, dmr_packet_t *packet)
{
    config_t *config = load_config();
    lua_State *L = config->L;
    dmr_route_t policy = DMR_ROUTE_REJECT;

    dmr_log_trace("noisebridge: %s->route()", config->script);
    lua_getglobal(L, "route"); /* Call script.lua->route() */
    lua_pass_proto(L, src);
    lua_pass_proto(L, dst);
    lua_pass_packet(L, packet);
    
    /* Call route(), 3 arguments, 1 return */
    if (lua_pcall(L, 3, 1, 0) != 0) {
        dmr_log_error("noisebridge: %s->route() failed: %s",
            config->script, lua_tostring(L, -1));
        goto bail;
    }

    /* Retrieve packet */
    switch (lua_type(L, lua_gettop(L))) {
        case LUA_TNIL: {
            // Packet is not modified
            dmr_log_debug("noisebridge: %s->route() returned nil packet (ok)",
                config->script);
            policy = DMR_ROUTE_PERMIT_UNMODIFIED;
            break;
        }
        case LUA_TBOOLEAN: {
            dmr_log_debug("noisebridge: %s->route() returned boolean %s (ok)",
                config->script, DMR_LOG_BOOL(lua_toboolean(L, 1)));
            policy = lua_toboolean(L, 1) ? DMR_ROUTE_PERMIT_UNMODIFIED : DMR_ROUTE_REJECT;
            break;
        }
        case LUA_TTABLE: {
            dmr_log_debug("noisebridge: %s->route() returned modified packet (ok)",
                config->script);
            policy = DMR_ROUTE_PERMIT;
            break;
        }
        default: {
            dmr_log_error("noisebridge: %s->route() did not return a table, boolean or nil",
                config->script);
            policy = DMR_ROUTE_REJECT;
            break;
        }
    }

    // Packet is modified
    if (policy == DMR_ROUTE_PERMIT) {
        lua_modify_packet(L, packet);
    }   

    lua_pop(L, 1); /* pop returned value from stack */

bail:
    switch (policy) {
    case DMR_ROUTE_REJECT:
        dmr_log_trace("noisebridge: %s->route(): reject", config->script);
        break;
    case DMR_ROUTE_PERMIT:
        dmr_log_trace("noisebridge: %s->route(): permit", config->script);
        break;
    case DMR_ROUTE_PERMIT_UNMODIFIED:
        dmr_log_trace("noisebridge: %s->route(): permit (unmodified)", config->script);
        break;
    default:
        dmr_log_trace("noisebridge: %s->route(): invalid %u, rejecting", config->script, policy);
        return DMR_ROUTE_REJECT;
    }
    

    return policy;
}

int init_proto_homebrew(config_t *config, proto_t *proto)
{
    int ret = 0;
    dmr_log_info("noisebridge: connect to BrandMeister homebrew on %s:%d",
        inet_ntoa(*proto->instance.homebrew.addr),
        proto->instance.homebrew.port);
    dmr_homebrew_t *homebrew = dmr_homebrew_new(
        proto->instance.homebrew.port,
        *proto->instance.homebrew.addr);
    if (homebrew == NULL) {
        return dmr_error(DMR_ENOMEM);
    }

    dmr_homebrew_config_callsign(&homebrew->config, proto->instance.homebrew.call);
    dmr_homebrew_config_repeater_id(&homebrew->config, proto->instance.homebrew.repeater_id);
    dmr_homebrew_config_rx_freq(&homebrew->config, proto->instance.homebrew.rx_freq);
    dmr_homebrew_config_tx_freq(&homebrew->config, proto->instance.homebrew.tx_freq);
    dmr_homebrew_config_tx_power(&homebrew->config, proto->instance.homebrew.tx_power);
    dmr_homebrew_config_height(&homebrew->config, proto->instance.homebrew.height);
    dmr_homebrew_config_latitude(&homebrew->config, proto->instance.homebrew.latitude);
    dmr_homebrew_config_longitude(&homebrew->config, proto->instance.homebrew.longitude);

    if ((ret = dmr_homebrew_auth(homebrew, proto->instance.homebrew.auth)) != 0) {
        goto bail;
    }

    proto->proto = &homebrew->proto;
    proto->mem = homebrew;

bail:
    return ret;
}

int init_proto_mbe(config_t *config, proto_t *proto)
{
    int ret = 0;
#if !defined(WITH_MBELIB) || !defined(HAVE_LIBPORTAUDIO)
    dmr_log_critical("noisebridge: mbe not available");
    return -1;
#else
    dmr_log_info("noisebridge: connect to MBE audio on %s",
        proto->instance.mbe.device == NULL
            ? "<auto detect>"
            : proto->instance.mbe.device);

    if (stream == NULL) {
        if ((ret = Pa_Initialize()) != paNoError) {
            dmr_log_critical("noisebridge: failed to initialize audio: %s",
                Pa_GetErrorText(ret));
            goto bail;
        }
        if ((ret = Pa_OpenDefaultStream(&stream, 0, 1, paFloat32, 8000, DMR_DECODED_AMBE_FRAME_SAMPLES, stream_samples, &stream_data)) != paNoError) {
            dmr_log_critical("noisebridge: failed to initialize stream: %s",
                Pa_GetErrorText(ret));
            goto bail;
        }
    }

    dmr_mbe_t *mbe = dmr_mbe_new(max(3, proto->instance.mbe.quality), stream_write);

    proto->proto = &mbe->proto;
    proto->mem = mbe;

bail:
#endif // WITH_MBELIB && HAVE_LIBPORTAUDIO
    return ret;
}

int init_proto_mmdvm(config_t *config, proto_t *proto)
{
    int ret = 0;
    dmr_log_info("noisebridge: connect to MMDVM modem on %s",
        proto->instance.mmdvm.port);
    dmr_mmdvm_t *mmdvm = dmr_mmdvm_open(proto->instance.mmdvm.port, 115200, 0);
    if (mmdvm == NULL) {
        return dmr_error(DMR_ENOMEM);
    }

    proto->proto = &mmdvm->proto;
    proto->mem = mmdvm;
    return ret;
}

int init_repeater(void)
{
    int ret = 0;
    config_t *config = load_config();
    repeater = dmr_repeater_new(route);
    if (repeater == NULL) {
        ret = -1;
        goto bail;
    }

    uint8_t i;
    for (i = 0; i < config->protos; i++) {
        proto_t *proto = config->proto[i];
        switch (proto->type) {
            case DMR_PROTO_HOMEBREW: {
                ret = init_proto_homebrew(config, proto);
                break;
            }
            case DMR_PROTO_MBE: {
                ret = init_proto_mbe(config, proto);
                break;
            }
            case DMR_PROTO_MMDVM: {
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
            goto bail;
        }

        if (proto->proto == NULL) {
            dmr_log_critical("noisebridge: no proto returned from %s", proto->name);
            ret = -1;
            goto bail;
        }

        if ((ret = dmr_proto_call_init(proto->mem) != 0)) {
            goto bail;
        }
        dmr_repeater_add(repeater, proto->mem, proto->proto);
        if ((ret = dmr_proto_call_start(proto->mem) != 0)) {
            goto bail;
        }
    }

    if ((ret = dmr_proto_call_init(repeater)) != 0) {
        goto bail;
    }
    if ((ret = dmr_proto_call_start(repeater)) != 0) {
        goto bail;
    }

    goto done;

bail:
    talloc_free(repeater);
    repeater = NULL;

done:
    return ret;
}

int loop_repeater(void)
{
    int ret = 0;

    if (active == NULL) {
        active = talloc_zero(NULL, dmr_cond_t);
    }
    if (active_lock == NULL) {
        active_lock = talloc_zero(NULL, dmr_mutex_t);
    }
    if (active == NULL || active_lock == NULL) {
        dmr_log_critical("noisebridge: out of memory");
        ret = dmr_error(DMR_ENOMEM);
        goto bail;
    }

    if (dmr_mutex_init(active_lock, dmr_mutex_plain) != dmr_thread_success) {
        dmr_log_critical("noisebridge: failed to setup lock");
        ret = -1;
        goto bail;
    }
    if (dmr_cond_init(active) != dmr_thread_success) {
        dmr_log_critical("noisebridge: failed to setup cond");
        ret = -1;
        goto bail;
    }
    signal(SIGINT, kill_handler);

    dmr_log_info("noisebridge: running repeater");
    dmr_cond_wait(active, active_lock);

    dmr_log_info("noisebridge: stopping repeater");
    dmr_cond_destroy(active);
    active = NULL;
    dmr_mutex_destroy(active_lock);
    active_lock = NULL;

    config_t *config = load_config();
    uint8_t i;
    for (i = 0; i < config->protos; i++) {
        proto_t *proto = config->proto[i];
        dmr_log_info("noisebridge: stop %s", proto->name);
        if (proto->proto->stop != NULL) {
            if ((ret = dmr_proto_call_stop(proto->mem)) != 0) {
                dmr_log_error("noisebridge: failed to stop %s: %s",
                    proto->name, dmr_error_get());
                ret = 0;
            }
        }
    }
    if ((ret = dmr_proto_call_stop(repeater)) != 0) {
        goto bail;
    }
    if ((ret = dmr_proto_call_wait(repeater)) != 0) {
        goto bail;
    }

bail:
    dmr_log_error("noisebridge: repeater stopped with error: %s", dmr_error_get());

done:
    talloc_free(repeater);
    repeater = NULL;
    return ret;
}
