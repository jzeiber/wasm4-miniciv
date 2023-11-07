#pragma once
/*
	ver 2023-02-17
*/

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* malloc( size_t size );
void free( void *ptr );
void* calloc( size_t num, size_t size );

#ifdef __cplusplus
}
#endif
