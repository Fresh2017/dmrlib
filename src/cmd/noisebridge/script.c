#include <dmr/id.h>
#include <dmr/packet.h>
#include "common/format.h"
#include "common/scan.h"
#include "config.h"

static void lua_set_fun(lua_State *L, const char *key, void *fun)
{
    lua_pushstring(L, key);
    lua_pushcfunction(L, fun);
    lua_settable(L, -3);
}

static void lua_set_int(lua_State *L, const char *key, int value)
{
    lua_pushstring(L, key);
    lua_pushinteger(L, value);
    lua_settable(L, -3);
}

static void lua_set_str(lua_State *L, const char *key, char *value)
{
    lua_pushstring(L, key);
    lua_pushstring(L, value);
    lua_settable(L, -3);
}

static int lua_log_priority(lua_State *L)
{
    switch (lua_gettop(L)) {
    case 0:
        break;
    case 1:
        dmr_log_priority_set(lua_tointeger(L, 1));
        break;
    default:
        return luaL_error(L, "expected at most 1 argument");
    }
    lua_pushinteger(L, dmr_log_priority());
    return 1;
}

static int lua_log_trace(lua_State *L)
{
    (void)L;
    dmr_log_trace(lua_tostring(L, -1));
    return 0;
}

static int lua_log_debug(lua_State *L)
{
    (void)L;
    dmr_log_debug(lua_tostring(L, -1));
    return 0;
}

static int lua_log_info(lua_State *L)
{
    dmr_log_info(lua_tostring(L, -1));
    return 0;
}

static int lua_log_warn(lua_State *L)
{
    dmr_log_warn(lua_tostring(L, -1));
    return 0;
}

static int lua_log_error(lua_State *L)
{
    dmr_log_error(lua_tostring(L, -1));
    return 0;
}

static int lua_log_critical(lua_State *L)
{
    dmr_log_critical(lua_tostring(L, -1));
    return 0;
}

int lua_pass_proto(lua_State *L, proto_t *proto)
{
    /*
    struct dmr_proto_s {
        const char                *name;
        dmr_proto_type_t          type;
        dmr_proto_init_t          init;
        bool                      init_done;
        dmr_proto_start_t         start;
        dmr_proto_stop_t          stop;
        dmr_proto_wait_t          wait;
        dmr_proto_active_t        active;
        dmr_proto_process_data_t  process_data;
        dmr_proto_process_voice_t process_voice;
        dmr_proto_io_t            tx;
        dmr_proto_io_cb_t         *tx_cb[DMR_PROTO_CB_MAX];
        size_t                    tx_cbs;
        dmr_proto_io_t            rx;
        dmr_proto_io_cb_t         *rx_cb[DMR_PROTO_CB_MAX];
        size_t                    rx_cbs;
        dmr_thread_t              *thread;
        bool                      is_active;
        dmr_mutex_t               *mutex;
    };
    */

    lua_createtable(L, 0, 2);
    lua_pushliteral(L, "name");         /* [1] */
    lua_pushstring(L, proto->name);
    lua_rawset(L, -3);
    lua_pushliteral(L, "type");         /* [2] */
    lua_pushinteger(L, proto->type);
    lua_rawset(L, -3);
    return 2;
}

int lua_pass_packet(lua_State *L, dmr_parsed_packet *packet)
{
    /*
    dmr_ts_t         ts;
    dmr_flco_t       flco;
    dmr_id_t         src_id;
    dmr_id_t         dst_id;
    dmr_id_t         repeater_id;
    dmr_data_type_t  data_type;
    dmr_color_code_t color_code;
    uint8_t          payload[DMR_PAYLOAD_BYTES];
    struct {
        uint8_t  sequence;
        uint8_t  voice_frame;
        uint32_t stream_id;
    } meta;
    */

    lua_createtable(L, 0, 7);
    lua_pushliteral(L, "ts");           /* [1] */
    lua_pushinteger(L, packet->ts);
    lua_rawset(L, -3);
    lua_pushliteral(L, "flco");         /* [2] */
    lua_pushinteger(L, packet->flco);
    lua_rawset(L, -3);
    lua_pushliteral(L, "src_id");       /* [3] */
    lua_pushinteger(L, packet->src_id);
    lua_rawset(L, -3);
    lua_pushliteral(L, "dst_id");       /* [4] */
    lua_pushinteger(L, packet->dst_id);
    lua_rawset(L, -3);
    lua_pushliteral(L, "repeater_id");  /* [5] */
    lua_pushinteger(L, packet->repeater_id);
    lua_rawset(L, -3);
    lua_pushliteral(L, "data_type");    /* [6] */
    lua_pushinteger(L, packet->data_type);
    lua_rawset(L, -3);
    lua_pushliteral(L, "color_code");   /* [7] */
    lua_pushinteger(L, packet->color_code);
    lua_rawset(L, -3);
    return 7;
}

void lua_modify_packet(lua_State *L, dmr_parsed_packet *packet)
{
    dmr_parsed_packet modify;
    byte_copy(&modify, packet, sizeof(dmr_parsed_packet));

    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "ts");
    lua_getfield(L, 1, "flco");
    lua_getfield(L, 1, "src_id");
    lua_getfield(L, 1, "dst_id");
    lua_getfield(L, 1, "repeater_id");
    lua_getfield(L, 1, "data_type");
    lua_getfield(L, 1, "color_code");
    modify.ts          = luaL_checkinteger(L, -7);
    modify.flco        = luaL_checkinteger(L, -6);
    modify.src_id      = luaL_checkinteger(L, -5);
    modify.dst_id      = luaL_checkinteger(L, -4);
    modify.repeater_id = luaL_checkinteger(L, -3);
    modify.data_type   = luaL_checkinteger(L, -2);
    modify.color_code  = luaL_checkinteger(L, -1);

#define CHECK(field,fmt,rep) do { \
    if (packet->field != modify.field) { \
        dmr_log_debug("script: update "#field" "fmt"->"fmt, rep(packet->field), rep(modify.field)); \
        packet->field = modify.field; \
    } \
} while(0)
    CHECK(ts, "%s", dmr_ts_name);
    CHECK(flco, "%s", dmr_flco_name);
    //CHECK(src_id, "%s", dmr_id_name);
    //CHECK(dst_id, "%s", dmr_id_name);
    CHECK(repeater_id, "%u", (dmr_id));
    //CHECK(data_type, "%s", dmr_data_type_name);
    CHECK(color_code, "%u", (dmr_color_code));
#undef CHECK
}

int init_script(void)
{
    config_t *config = load_config();
    lua_State *L = config->L;
    size_t i;

    if (L == NULL) {
        dmr_log_debug("noisebridge: setup lua state");
        L = luaL_newstate();
        if (L == NULL) {
            dmr_log_critical("config: failed to init new lua state");
            return dmr_error(DMR_EINVAL);
        }
        luaL_openlibs(L);
        lua_settop(L, 0);
    }

    /* Expose our API */
    dmr_log_debug("noisebridge: setup lua API");

    /* Global log table */
    lua_newtable(L);
    lua_set_fun(L, "priority", lua_log_priority);
    lua_set_fun(L, "trace", lua_log_trace);
    lua_set_fun(L, "debug", lua_log_debug);
    lua_set_fun(L, "info", lua_log_info);
    lua_set_fun(L, "warn", lua_log_warn);
    lua_set_fun(L, "error", lua_log_error);
    lua_set_fun(L, "critical", lua_log_critical);
    lua_setglobal(L, "log");

    /* Global config table */
    lua_newtable(L); /* create "config" */
    for (i = 0; i < config->protos; i++) {
        proto_t *proto = config->proto[i];
        lua_pushnumber(L, i + 1);
        lua_newtable(L);                /* child table */
        lua_set_int(L, "type", proto->type);
        lua_set_str(L, "name", proto->name);
        switch (proto->type) {
            case DMR_PROTOCOL_HOMEBREW: {
                lua_set_int(L, "repeater_id", proto->settings.homebrew.repeater_id);
                lua_set_str(L, "host", format_ip6s(proto->settings.homebrew.peer_ip));
                lua_set_int(L, "port", proto->settings.homebrew.peer_port);
                lua_set_str(L, "call", proto->settings.homebrew.call);
                break;
            }
            case DMR_PROTOCOL_MMDVM: {
                lua_set_str(L, "port", proto->settings.mmdvm.port);
                lua_set_int(L, "baud", proto->settings.mmdvm.baud);
                break;
            }
            default: {
                break;
            }
        }
        lua_settable(L, -3);        /* pop key, value pair from Lua VM stack */
    }
    lua_setglobal(L, "config");

    dmr_log_info("noisebridge: loading script %s", config->repeater.script);
    if (luaL_loadfile(L, config->repeater.script) != 0) {
        dmr_log_critical("noisebridge: failed to load %s: %s",
            config->repeater.script, lua_tostring(L, -1));
        return -1;
    }

    dmr_log_debug("noisebridge: init lua state");
    if (lua_pcall(L, 0, 0, 0) != 0) {
        dmr_log_critical("noisebridge: failed to init %s: %s",
            config->repeater.script, lua_tostring(L, -1));
        return -1;
    }

    dmr_log_debug("noisebridge: %s->setup()", config->repeater.script);
    lua_getglobal(L, "setup"); /* Call script.lua->setup(), expect no return */
    if (lua_pcall(L, 0, 0, 0) != 0) {
        dmr_log_error("noisebridge: %s->setup() failed: %s",
            config->repeater.script, lua_tostring(L, -1));
        return -1;
    }

    config->L = L;
    return 0;
}
