#include "controller.h"
#include "bufferpoolmanager.h"
#include "imagesaver.h"
#include "scale.h"
#include "crop.h"
#include "eyedetector.h"
#include "eyeselector.h"
#include "NeuroTask.h"
#include "enroll.h"
#include "J2C_Iris_SDK.h"
#include <thread>
#include <chrono>
using namespace std::chrono_literals;

IRIS_HANDLE handle = 0;
Controller gController;
long long frameSequenceId = 0;
void OnPnPEvent(BOOL bPlugIn)
{
	FUNCTION_NAME_IS(_T("OnPnPEvent"));
	LOG_PRINTF(0, _T("Plug-In: %d"), bPlugIn);

	if (bPlugIn == TRUE)
	{
		//
	}
	else
	{
		//
	}
}

void OnReceiveFrame(PBYTE pFrame, DWORD dwReceived)
{
	Controller::frame_receive_callback(pFrame, dwReceived);
}

void Controller::initBufferPoolManager()
{
	BufferPoolManager &gpm = getBufferPoolManager();
	gpm.registerBufferPool(16, FRAMESIZE_FOR_OPENCV, COLOR_BUFFER_RGBA, OPENCV_BUFFER_POOL_ID);   // 320 * 240 * 1 RGBA	
	gpm.registerBufferPool(60, FRAMESIZE_FOR_EYE_VIEW, COLOR_BUFFER_LUMINANCE, EYE_VIEW_BUFFER_POOL_ID); // 640 * 480 *4 RGBA
	gpm.registerBufferPool(15, FRAMESIZE_FOR_CAMERA_FRAME, COLOR_BUFFER_LUMINANCE, CAMERA_FRAME_BUFFER_POOL_ID); // 2608 * 1960 * 1 ,Luminance only 15 frame/sec 
	gpm.registerBufferPool(8, FRAMESIZE_FOR_EYEDETECT_VIEW, COLOR_BUFFER_LUMINANCE, EYEDETECT_VIEW_BUFFER_POOL_ID);
	gpm.registerBufferPool(60, FRAMESIZE_FOR_CAMERA_CROP, COLOR_BUFFER_LUMINANCE, CAMERA_CROP_BUFFER_POOL_ID); // camera pool 
}

void Controller::initCamera()
{
	//bJ2CIris_Init();
	//J2CIris_RegisterPnpCallback(OnPnPEvent);
	//handle = hJ2CIris_Open();
	//J2CIris_RegisterFrameReceiveCallback(handle, OnReceiveFrame);
}

BOOL Controller::CheckIrisDevOpen()
{
	return (handle == 0) ? FALSE : TRUE;
}

BOOL Controller::startCamera()
{
	BOOL bIrisStart = FALSE;
	BOOL bIrisInit = bJ2CIris_Init();
	FUNCTION_NAME_IS(_T("Controller::startCamera"));
	LOG_PRINTF(0, _T("IRIS init: %d"), bIrisInit);

	//if (bIrisInit)
	{
		J2CIris_RegisterPnpCallback(OnPnPEvent);
		handle = hJ2CIris_Open();
		if (handle)
		{
			J2CIris_RegisterFrameReceiveCallback(handle, OnReceiveFrame);
			bIrisStart = bJ2CIris_Start(handle);
		}
	}
	//else
	{
		//
	}
	LOG_PRINTF(0, _T("IRIS start: %d"), bIrisStart);

	return bIrisStart;
}

BOOL Controller::stopCamera()
{
	BOOL bIrisStop = FALSE;
	FUNCTION_NAME_IS(_T("Controller::stopCamera"));
	if (handle)
	{
		bIrisStop = bJ2CIris_Stop(handle);
		J2CIris_Close(handle);
		handle = 0;
		J2CIris_DeInit();
		LOG_PUTS(0, _T("IRIS deinit."));
	}	
	LOG_PRINTF(0, _T("IRIS stop: %d"), bIrisStop);

	return bIrisStop;
}

void Controller::initComponents()
{
	getScaler().startJob();
	getCrop().startJob();
	EyeDetector *eyeDetector = new EyeDetector();
	setEyeDetectorPtr(eyeDetector);
	std::thread *eyeThread = new std::thread(*eyeDetector);
	getImageSaver().startJob();
	getEyeSelector().startJob();
	getEnrollTask().startJob();
	getIdentifyTask().startJob();
	getNeuroTask().startJob();	
	std::this_thread::sleep_for(3s);
	
	setReadyToDraw(true);

	getNeuroTask().reloadEnroll();
}

void Controller::setNeuroWorkDirectory(char *directory)
{
	std::string dir(directory);
	getNeuroTask().setWorkDir(dir);
}

void Controller::reloadEnroll()
{
	getNeuroTask().reloadEnroll();
}

void Controller::saveCropImage(SharedBuffer& buf)
{
	Mat  _src = cv::Mat(cvSize(WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP), CV_8UC1, buf->getBuffer(), cv::Mat::AUTO_STEP);
	Mat src;
	cvtColor(_src, src, cv::COLOR_GRAY2BGR);
	char path[256];
	sprintf(path, "c:\\jtwoc\\crop\\crop_%s.png", buf->getName());
	imwrite(path, src);
}

void Controller::frame_receive_callback(unsigned char * buffer, DWORD in)
{	
	if (gController.isReadyToDraw() == false)
		return;

	if (gController.isBlocked() == true)
	{
		//std::cout << "Camera frame is blocked " << std::endl;
		LOG_PRINTF(0, _T("Camera frame is blocked. (BlockTime= %lld)"), gController.getBlockTime());
		return;
	}

	if (getController().deviceId_ == "")
	{
		char _tmp[12];
		memset(_tmp, 0, sizeof(_tmp));
		memcpy(_tmp, buffer, DEVICE_ID_LEN);
		_tmp[DEVICE_ID_LEN - 1] = 0;
		getController().deviceId_ = _tmp;
	}
	char sequenceIdBuffer[MAX_BUFFER_NAME];
	sprintf(sequenceIdBuffer, "%lld", frameSequenceId++);

	/////////////////////////////////////////////////////////////////////////////
	// cscho (2018-12.11)
	
	SharedBuffer origin = getBufferPoolManager().getBuffer(CAMERA_FRAME_BUFFER_POOL_ID, sequenceIdBuffer);
	if (origin != nullptr)
	{
		memcpy(origin->getBuffer(), buffer, FRAMESIZE_FOR_CAMERA_FRAME);
		origin->setAvailable(FRAMESIZE_FOR_CAMERA_FRAME);
		origin->setTick(GetTickCount());
		getMultiCoreQueueForCameraInput().putSharedBuffer(origin);
		origin = nullptr;
	}
	/*
	SharedBuffer origin = getBufferPoolManager().getBuffer(CAMERA_CROP_BUFFER_POOL_ID, sequenceIdBuffer);
	if (origin != nullptr)
	{
		char* dst = (char*)origin->getBuffer();
		char* start = (char*)buffer;
		start += (WIDTH_FOR_CAMERA_FRAME * ROI_Y);
		start += ROI_X;

		for (int i = 0; i < HEIGHT_FOR_CAMERA_CROP; i++)
		{
			memcpy(dst, start, WIDTH_FOR_CAMERA_CROP);
			dst += WIDTH_FOR_CAMERA_CROP;
			start += WIDTH_FOR_CAMERA_FRAME;
		}
		//getController().saveCropImage(origin);
		getMultiCoreQueueForScaler().putSharedBuffer(origin);

		if (getController().isEnrollUsage() != true && getController().isIdentifyUsage() != true)
		{
			getBufferPoolManager().deleteBufferWithName(CAMERA_CROP_BUFFER_POOL_ID, sequenceIdBuffer);
		}
	}
	*/
	/////////////////////////////////////////////////////////////////////////////
	else
	{
		//std::cout << "Camera frame buffer get failed" << std::endl;
		LOG_PRINTF(0, _T("Failed to get Camera frame buffer. (Receive Frame= %d bytes)"), in);
	}

	return;

}