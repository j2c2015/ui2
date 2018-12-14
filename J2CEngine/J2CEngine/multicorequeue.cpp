#include "multicorequeue.h"

MultiCoreQueue  gMultiCoreQueueForCameraInput;
MultiCoreQueue  gMultiCoreQueueForCameraCrop;
MultiCoreQueue  gMultiCoreQueueForScaler;
MultiCoreQueue  gMultiCoreQueueForOpenCV;
MultiCoreQueue  gMultiCoreQueueForNeuro;
MultiCoreQueue  gMultiCoreQueueForEyeDetectViewer;
MultiCoreQueue  gMultiCoreQueueForEyeViewer;
MultiCoreQueue  gMultiCoreQueueForEyeSelector;
MultiCoreQueue  gMultiCoreQueueForKind7;
MultiCoreQueue  gMultiCoreQueueForNetowrk;
MultiCoreQueue  gMultiCoreQueueForIdentify;

MultiCoreQueue::MultiCoreQueue()
{
	ready_ = false;
	waiting_ = false;
}
