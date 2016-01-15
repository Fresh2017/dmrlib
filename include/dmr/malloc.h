#ifndef _DMR_MALLOC_H
#define _DMR_MALLOC_H

#include <stdlib.h>

#define dmr_malloc(_type)    	(_type *)malloc(sizeof(_type))
#define dmr_malloc_zero(_type) 	(_type *)calloc(1, sizeof(_type))
#define dmr_free(_ptr) 			do { if (_ptr != NULL) free(_ptr); } while(0)

#endif // _DMR_MALLOC_H