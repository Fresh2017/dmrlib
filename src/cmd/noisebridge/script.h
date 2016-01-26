#ifndef _NOISEBRIDGE_SCRIPT_H
#define _NOISEBRIDGE_SCRIPT_H

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <dmr/proto.h>
#include <dmr/packet.h>

int lua_pass_proto(lua_State *L, dmr_proto_t *proto);
int lua_pass_packet(lua_State *L, dmr_packet_t *packet);
void lua_modify_packet(lua_State *L, dmr_packet_t *packet);
int init_script(void);

#endif // _NOISEBRIDGE_SCRIPT_H
