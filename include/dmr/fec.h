/**
 * @file
 * @author Wijnand Modderman-Lenstra
 */
#ifndef _DMR_FEC_H
#define _DMR_FEC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <dmr/fec/bptc_196_96.h>
#include <dmr/fec/qr_16_7.h>
#include <dmr/fec/reed_solomon.h>

/** Initialize the Forward Error Correction functions. */
extern int dmr_fec_init(void);

#ifdef __cplusplus
}
#endif

#endif // _DMR_FEC_H
