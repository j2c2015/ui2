#include "common.h"
#include "usbio.h"
#include "utility.h"

#ifndef _ENGINE_H_
#define _ENGINE_H_

#define CAMAPI

#define CAMERADEVICE_STS_INSERTED		(uint32_t)(0)
#define CAMERADEVICE_STS_REMOVED		(uint32_t)(1)

#define MAX_TRANSFER_OVERLAPPED_COUNT	(uint32_t)(32)
#define MAX_TRANSFER_ISSUED_COUNT		(uint32_t)(32)

typedef struct _CAMERA_DEVICEINFO
{
	USBIO_DEVICEHANDLE_t			DeviceHandle;
	USBIO_PIPEHANDLE_t				ActivePipeHandle;
	LOOPQUEUE_t						LoopQueue;
	HANDLE_t						InternalThread;
	HANDLE_t						InternalThreadKillEvent;
	uint32_t						State;
}CAMERA_DEVICEINFO_t, *PCAMERA_DEVICEINFO_t;

typedef PCAMERA_DEVICEINFO_t CAMERA_DEVICEHANDLE_t;

CAMAPI bool_t Engine_Start(IN bool_t bDebugStrace, IN PREPAREBUFFER_t PrepareBufferLists[], IN uint32_t ListCount);
CAMAPI void_t Engine_Stop();
CAMAPI bool_t Engine_GetFrame(IN HANDLE hCameraHandle, OUT uint8_t **pFrameBuffer, OUT SIZE_T *pSize);
CAMAPI bool_t Engine_AdvanceHead(IN HANDLE hCameraHandle);
CAMAPI bool_t Engine_GetDeviceHandle(OUT USBIO_DEVICEHANDLE_t * DeviceHandle);

#pragma pack(1)
typedef struct _UVC_HEADER
{
	uint8_t						Len; // 항상 12바이트
	uint8_t						Flag;  
	uint16_t					SeqNum; // 0 부터 증가
	uint32_t					DataLen; // 헤더를 제외한 실제 Payload 크기바이트
	uint8_t						Reserved[4];
}UVC_HEADER_t, *PUVC_HEADER_t;
#pragma pack()

#endif // _ENGINE_H_
