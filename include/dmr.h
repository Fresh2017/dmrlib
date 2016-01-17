#ifndef _DMR_H
#define _DMR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
* @file
* @mainpage dmrlib API Reference
*
* @section intro_sec Introduction
* dmrlib is a portable implementation of the Digital Mobile Radio protocol for
* the C programming language.
*
* @section port_sec Portability
* The three major operating systems Linux, Mac OS X and Windows are supported
* by dmrlib.
**
* For more detailed information, browse the different sections of this
* documentation. A good place to start is:
* proto.h.
*/

#include <dmr/config.h>
#include <dmr/packet.h>
#include <dmr/payload.h>
#include <dmr/payload/emb.h>
#include <dmr/payload/lc.h>
#include <dmr/payload/sync.h>

#ifdef __cplusplus
}
#endif

#endif // _DMR_H
