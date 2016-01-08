/**
 * @file
 * @author Wijnand Modderman-Lenstra
 */
#ifndef _DMRFEC_H
#define _DMRFEC_H

#include <dmrfec/bptc_196_96.h>
#include <dmrfec/quadres_16_7.h>
#include <dmrfec/rs_12_9.h>

/** Initialize the Forward Error Correction functions. */
extern void dmrfec_init(void);

#endif // _DMRFEC_H
