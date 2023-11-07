#pragma once
/*
	ver 2023-04-23
*/

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t strlen(const char *str);
char* strncpy(char* destination, const char* source, size_t num);

void* memcpy(void *dest, const void *src, size_t n);
void* memmove(void *dst, const void *src, size_t len);
void* memset(void *s, int c, size_t len);

#ifdef __cplusplus
}
#endif
