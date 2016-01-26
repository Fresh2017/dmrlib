#include <stdlib.h>
#include "dmr/malloc.h"

void *_dmr_malloc(size_t size)
{
	return (void *)malloc(size);
}

void *_dmr_malloc_zero(size_t size)
{
	return (void *)calloc(1, size);	
}

void _dmr_free(void *ptr)
{
	if (ptr != NULL) {
		free(ptr);
	}
}