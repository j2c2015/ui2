//

#include "stdafx.h"
#include "J2CEngine_dll.h"
#include "controller.h"
#include "scale.h"
#include "eyedetector.h"
#include "eyeselector.h"

J2CEngineDLL_API void initJ2CEngine(int nPartialWidth, int nPartialHeight)
{
	Controller &c = getController();
	c.initEngine(nPartialWidth, nPartialHeight);
}

J2CEngineDLL_API void LoadJ2CConfiguration()
{
	Controller &c = getController();
	c.LoadConfiguration();
}

J2CEngineDLL_API void registerRealTimeRenderCb(J2CRenderCb cb)
{
	Controller &c = getController();
	c.setRealTimeRenderCb(cb);
}

J2CEngineDLL_API bool J2CStart()
{
	Controller &c = getController();
	bool bIrisStart = c.startCamera();
	return bIrisStart;
}

J2CEngineDLL_API bool J2CStop()
{
	Controller &c = getController();
	bool bIrisStop = c.stopCamera();
	return bIrisStop;
}

J2CEngineDLL_API bool J2CCheckDevOpen()
{
	Controller &c = getController();
	bool bIrisDevOpen = c.CheckIrisDevOpen();
	return bIrisDevOpen;
}

J2CEngineDLL_API void enrollRequest(char *id, J2CEnrollCb cb)
{
	getController().enrollRequest(id, cb);
}

J2CEngineDLL_API void registerEnrollRenderCb(J2CRenderCb cb)
{
	getController().setEnrollRenderCb(cb);
}

J2CEngineDLL_API void registerIdentifyRenderCb(J2CRenderCb cb)
{
	getController().setIdentifyRenderCb(cb);
}

J2CEngineDLL_API void identifyRequest(J2CIdentifyCb  cb)
{
	getController().identifyRequest(cb);
}

J2CEngineDLL_API void registerDebugOutputCb(J2CDebugOuputCb cb)
{
	// to do
}

J2CEngineDLL_API void saveCameraCropFrames(bool save, char *directory)
{
	getScaler().save(save, directory);
}
J2CEngineDLL_API void saveEyeDetectFrames(bool save, char *directory)
{
	getEyeDetectorPtr()->save(save, directory);
}
J2CEngineDLL_API void saveEyeSelectFrames(bool save, char *directory)
{
	getEyeSelector().save(save, directory);
}

J2CEngineDLL_API void setNeuroWorkDirectory(char *directory)
{
	getController().setNeuroWorkDirectory(directory);
}

J2CEngineDLL_API void setEnrollCount(int count)
{
	getController().setEnrollCount(count);
}

J2CEngineDLL_API void partialRender(int x, int y, J2CRenderCb cb)
{
	getController().partialRender(x, y, cb);
}

J2CEngineDLL_API void J2CLogInit(void* hInstance)
{
	g_hInstApp = (HINSTANCE)hInstance;
	LOG_INIT((HANDLE)hInstance);
}

J2CEngineDLL_API void J2CEyeFindTest(char* pszFilePath)
{
	getController().CallEyeFindTestFile(pszFilePath);
}
