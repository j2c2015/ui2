#include "common.h"

#ifndef _UTILITY_H_
#define _UTILITY_H_

// ȯ��ť�� �����մϴ�
#define MAX_QUEUE_ENTRYCOUNT	(32*4)

typedef struct _PREPAREBUFFER
{
	void_t *			Buffer;
	SIZE_T				YSize;
	SIZE_T				TotalSize;
}PREPAREBUFFER_t, *PPREPAREBUFFER_t;

typedef struct _QUEUEENTRYBUFFER
{
	bool_t				bIsUsing;
	void_t *			Buffer;
	SIZE_T				YSize;
	SIZE_T				TotalSize;
}QUEUEENTRYBUFFER_t, *PQUEUEENTRYBUFFER_t;

typedef struct _LOOPQUEUE
{
	uint32_t			dwHead;
	uint32_t			dwTail;
	CRITICAL_SECTION_t	lqCriticalSection;
	bool_t				bInitialized;
	uint32_t			ListCount;
	QUEUEENTRYBUFFER_t	QueueEntryBuffer[MAX_QUEUE_ENTRYCOUNT + 1];
	void_t *			QueueEntry[MAX_QUEUE_ENTRYCOUNT + 1];
	uint32_t			dwEntryAcceptedSize[MAX_QUEUE_ENTRYCOUNT + 1]; // YUV �� ��� �����ϴ� ũ�⸦ ��Ÿ����
}LOOPQUEUE_t, *PLOOPQUEUE_t;

UTIL_API bool_t	Util_ReadyResource();
UTIL_API bool_t	Util_FreeResource();
UTIL_API bool_t Util_InitializeLoopQueue(IN PLOOPQUEUE_t pLoopQueue, IN PREPAREBUFFER_t PrepareBufferLists[], IN uint32_t ListCount);
UTIL_API bool_t Util_TerminateLoopQueue(IN PLOOPQUEUE_t pLoopQueue);
UTIL_API bool_t Util_EmptyLoopQueue(IN PLOOPQUEUE_t pLoopQueue);
UTIL_API bool_t Util_IsEmptyLoopQueue(IN PLOOPQUEUE_t pLoopQueue, OUT bool_t *pbEmpty);
UTIL_API bool_t Util_IsFullLoopQueue(IN PLOOPQUEUE_t pLoopQueue, OUT bool_t *pbFull);

UTIL_API bool_t Util_AdvanceCurrentTail(IN PLOOPQUEUE_t pLoopQueue, IN uint8_t * pEntry, IN SIZE_T Size);
UTIL_API bool_t Util_GetEntryForCurrentTail(IN PLOOPQUEUE_t pLoopQueue, IN uint8_t ** ppEntry, SIZE_T * pdwSize);

UTIL_API bool_t Util_GetEntryCurrentHead(IN PLOOPQUEUE_t pLoopQueue, IN uint8_t ** pEntry, OUT SIZE_T *pSize);
UTIL_API bool_t Util_AdvanceCurrentHead(IN PLOOPQUEUE_t pLoopQueue);
UTIL_API bool_t Util_ReleaseLoopQueueEntryBuffer(IN PLOOPQUEUE_t pLoopQueue, IN void * pBuffer);

// ���ȵ�
//UTIL_API bool_t Util_CopyAndAdvanceTailToLoopQueue(IN PLOOPQUEUE_t pLoopQueue, IN uint8_t * pEntry, IN uint32_t Size);
//UTIL_API bool_t Util_GetHeadAndAdvanceFromLoopQueue(IN PLOOPQUEUE_t pLoopQueue, OUT uint8_t ** ppEntry, OUT uint32_t *pSize);

#endif //_UTILITY_H_