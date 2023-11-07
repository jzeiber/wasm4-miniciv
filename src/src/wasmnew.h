#pragma once
/*
	ver 2023-02-17
*/

#include <stddef.h>

void* operator new(size_t count);
void* operator new[](size_t count);
void operator delete(void* ptr) noexcept;
void operator delete[](void *ptr) noexcept;
