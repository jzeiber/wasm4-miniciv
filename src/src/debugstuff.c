#include <stdint.h>
#include <stddef.h>

int __cxa_atexit(void (*func) (void *), void * arg, void * dso_handle)
{
	//trace("__cxa_atexit()");
	func;
	arg;
	dso_handle;
	return 0;
}
