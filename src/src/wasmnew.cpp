#include "wasmnew.h"
#include "wasmmalloc.h"

void* operator new(size_t count)
{
    return malloc(count);
}

void* operator new[](size_t count)
{
    return malloc(count);
}

void operator delete(void* ptr) noexcept
{
    free(ptr);
}

void operator delete[](void *ptr) noexcept
{
    free(ptr);
}
