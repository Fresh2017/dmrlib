#ifndef _DMR_MALLOC_H
#define _DMR_MALLOC_H

extern void *_dmr_malloc(size_t size);
extern void *_dmr_malloc_zero(size_t size);
extern void _dmr_free(void *ptr);

#define dmr_malloc(parent,type) 		_dmr_malloc(sizeof(type))
#define dmr_malloc_zero(parent,type) 	_dmr_malloc_zero(sizeof(type))
#define dmr_malloc_size(parent,size)	_dmr_malloc_zero(size)
#define dmr_free(ptr) 					do { if (ptr != NULL) { _dmr_free(ptr); ptr = NULL; } } while(0)

#endif // _DMR_MALLOC_H
