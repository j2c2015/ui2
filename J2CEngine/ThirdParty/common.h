/*
이 파일은 운영체제에 따라서 종속적으로 작성되는 파일입니다
*/

#include <Windows.h>

#include "usbcommon.h"

#ifndef _COMMON_H_
#define _COMMON_H_

#define UTIL_API	
#define IN
#define OUT
#define INOUT

#define void_t		void
#define uint8_t		unsigned char
#define uint16_t	unsigned short
#define uint32_t	unsigned int
#define bool_t		BOOLEAN
#define bTRUE		TRUE
#define bFALSE		FALSE
#define CRITICAL_SECTION_t	CRITICAL_SECTION

#define STAT_SUCCESS				(uint32_t)(0x00000000)
#define STAT_INVALID_PARAMETER		(uint32_t)(0xFFFFFFF0)
#define STAT_NO_RESOURCE			(uint32_t)(0xFFFFFFF1)
#define STAT_DEVICE_NOT_EXIST		(uint32_t)(0xFFFFFFF2)
#define STAT_SIZE_MISMATCH			(uint32_t)(0xFFFFFFF3)
#define STAT_SEQUENCE_ERROR			(uint32_t)(0xFFFFFFF4)
#define STAT_USB_ERROR				(uint32_t)(0xFFFFFFF5)
#define STAT_UNDEFINED_ERROR		(uint32_t)(0xFFFFFFFF)

#define HANDLE_t void_t *

#endif // _COMMON_H_
