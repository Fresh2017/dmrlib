#ifndef _NOISEBRIDGE_REPEATER_H
#define _NOISEBRIDGE_REPEATER_H

#include <dmr/proto/repeater.h>

dmr_repeater_t *load_repeater(void);
int init_repeater(void);
int loop_repeater(void);

#endif // _NOISEBRIDGE_REPEATER_H
