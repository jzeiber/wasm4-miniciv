#include "wasmmalloc.h"
#include "tinyalloc.h"
#include <stdint.h>

static uint8_t init=0;

void* malloc( size_t size )
{
    if(init==0)
    {
        extern void* __heap_base;
        ta_init(&__heap_base,(void *)0xffff,256,16,8);
        init=1;
    }
    return ta_alloc(size);
}

void* calloc( size_t num, size_t size )
{
    if(init==0)
    {
        extern void* __heap_base;
        ta_init(&__heap_base,(void *)0xffff,256,16,8);
        init=1;
    }
    return ta_calloc(num,size);
}

void free( void *ptr )
{
    ta_free(ptr);
}
