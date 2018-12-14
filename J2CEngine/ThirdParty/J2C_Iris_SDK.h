#pragma once
#include "stdafx.h"


#define WIDTH	(2592)
#define HEIGHT	(1944)
#define MAX_FRAME_SIZE			(2592*1944)


typedef void * IRIS_HANDLE;

typedef void(*PNP_CALLBACK)(BOOL);
typedef void(*FRAME_RECEIVE_CALLBACK)(PBYTE, DWORD);

#pragma pack(1)
typedef struct _USB_REQUEST
{
	unsigned short	Length;
	unsigned char	Request;
	unsigned short	Value;
	unsigned short	Index;
}USB_REQUEST, *PUSB_REQUEST;

typedef struct _USB_SETREQUEST
{
	USB_REQUEST		UsbRequest;
	unsigned char	Data[1];
}USB_SETREQUEST, *PUSB_SETREQUEST;

typedef struct _USB_GETREQUEST
{
	USB_REQUEST		UsbRequest;
}USB_GETREQUEST, *PUSB_GETREQUEST;

typedef struct _COMPLEX_GPIO_PARAM
{
	unsigned int  Period;
	unsigned int  ThresHold;
	unsigned int  BlinkingTime; // msec
}COMPLEX_GPIO_PARAM, *PCOMPLEX_GPIO_PARAM;

#pragma pack()


extern "C"
{
	BOOL			bJ2CIris_Init();
	IRIS_HANDLE		hJ2CIris_Open();
	void			J2CIris_Close(IRIS_HANDLE);
	void			J2CIris_DeInit();

	void			J2CIris_RegisterPnpCallback(PNP_CALLBACK);
	void			J2CIris_RegisterFrameReceiveCallback(IRIS_HANDLE, FRAME_RECEIVE_CALLBACK);
	BOOL			bJ2CIris_Start(IRIS_HANDLE);
	BOOL			bJ2CIris_Stop(IRIS_HANDLE);

	BOOL			bJ2CIris_InitCam(IRIS_HANDLE);
	BOOL			bJ2CIris_StartCam(IRIS_HANDLE);
	BOOL			bJ2CIris_StopCam(IRIS_HANDLE);
	BOOL			bJ2CIris_CaptureCam(IRIS_HANDLE);

	BOOL			bJ2CIris_SetSimpleGPIO(IRIS_HANDLE, BYTE, BOOL);
	BOOL			bJ2CIris_GetSimpleGPIO(IRIS_HANDLE, BYTE, PBOOL);
	BOOL			bJ2CIris_SetComplexGPIO(IRIS_HANDLE, BYTE, PCOMPLEX_GPIO_PARAM);
	BOOL			bJ2CIris_SetI2C(IRIS_HANDLE, BYTE, BOOL, UINT, UINT, PVOID);
	BOOL			bJ2CIris_GetI2C(IRIS_HANDLE, BYTE, BOOL, UINT, UINT, PVOID);
}
