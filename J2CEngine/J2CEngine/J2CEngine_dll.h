#pragma once
#ifdef J2CEngine_EXPORTS
#define J2CEngineDLL_API	extern "C" __declspec(dllexport)
#else
#define J2CEngineDLL_API	extern "C" __declspec(dllimport)
#endif

typedef void(__stdcall *J2CRenderCb)(void *buffer, int width, int height, int depth);
typedef void(__stdcall *J2CEnrollCb)(bool success);
typedef void(__stdcall *J2CIdentifyCb)(char *id,bool success);
typedef void(__stdcall *J2CDebugOuputCb)(char *str);

#define WIDTH_EYE_VIEW	800
#define HEIGHT_EYE_VIEW 600

#define WIDTH_ENROLL_EYE_VIEW	320
#define HEIGHT_ENROLL_EYE_VIEW	240

#define WIDTH_IDENTIFY_EYE_VIEW	320
#define HEIGHT_IDENTIFY_EYE_VIEW	240

J2CEngineDLL_API void initJ2CEngine(int nPartialWidth, int nPartialHeight);
J2CEngineDLL_API void registerRealTimeRenderCb(J2CRenderCb cb);
J2CEngineDLL_API bool J2CStart();
J2CEngineDLL_API bool J2CStop();
J2CEngineDLL_API bool J2CCheckDevOpen();
J2CEngineDLL_API void enrollRequest(char *id, J2CEnrollCb cb);
J2CEngineDLL_API void registerEnrollRenderCb(J2CRenderCb cb);
J2CEngineDLL_API void registerIdentifyRenderCb(J2CRenderCb cb);
J2CEngineDLL_API void identifyRequest(J2CIdentifyCb  cb);
J2CEngineDLL_API void registerDebugOutputCb(J2CDebugOuputCb cb);
J2CEngineDLL_API void saveCameraCropFrames(bool save,char *directory);
J2CEngineDLL_API void saveEyeDetectFrames(bool save,char *directory);
J2CEngineDLL_API void saveEyeSelectFrames(bool save,char *directory);
J2CEngineDLL_API void setNeuroWorkDirectory(char *directory);
J2CEngineDLL_API void setEnrollCount(int count);
J2CEngineDLL_API void partialRender(int x,int y, J2CRenderCb cb);
J2CEngineDLL_API void J2CLogInit(void* hInstance);

