#include "common.h"

#ifndef _KERNEL_H_
#define _KERNEL_H_

UTIL_API uint32_t	Kern_GetLastError();
UTIL_API void_t		Kern_SetLastError(IN uint32_t LastError);
UTIL_API void_t		Kern_DebugMonStart();
UTIL_API void_t		Kern_DebugMonStop();
UTIL_API void_t		Kern_DebugMonIncrement(uint32_t Issued, uint32_t Completed, uint32_t Inserted, uint32_t Removed, uint32_t Error);

#endif // _KERNEL_H_
