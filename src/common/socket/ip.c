#include "common/socket.h"

const ip4_t ip4loopback = {127, 0, 0, 1};
const ip4_t ip4any      = {  0, 0, 0, 0};

bool ip6disabled = false;
const uint8_t ip6mappedv4prefix[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff};
const ip6_t   ip6loopback           = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
const ip6_t   ip6any                = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
