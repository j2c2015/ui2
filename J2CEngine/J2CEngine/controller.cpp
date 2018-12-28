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
HINSTANCE g_hInstApp;
int gFindSpecularIdx = 0;
std::vector<RECT> vecRoiSP;
std::vector<RECT> vecRoiSPNot;
int gEnrollIdx = 0;
long long frameSequenceId = 0;
std::vector<ROI_RESULT> vecRoiSPDeviation;
std::vector<ROI_RESULT> vecRoiSPCandidate;
MultiCoreQueue gMultiCoreQueueForRoiSP;

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

	/////////////////////////////////////////////////////////////////////////////
	//
	// cscho (2018-12.20)
	//
	gpm.registerBufferPool(60, FRAMESIZE_FOR_MASK_CROP, COLOR_BUFFER_LUMINANCE, MASK_CROP_BUFFER_POOL_ID); // mask crop pool (800 * 600 * 1)
	gpm.registerBufferPool(60, FRAMESIZE_FOR_MASK_TEST, COLOR_BUFFER_LUMINANCE, MASK_TEST_BUFFER_POOL_ID); // mask crop pool (800 * 600 * 4)
	/////////////////////////////////////////////////////////////////////////////
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

	LoadConfiguration();

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

	// for print deviation value
	vecRoiSPDeviation.clear();

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

	// print deviation value
	if (vecRoiSPDeviation.size() > 0)
	{
		std::this_thread::sleep_for(5s);
		LOG_PRINTF(0, _T("Std-Deviation count= %d"), vecRoiSPDeviation.size());
	}
	_SYSTEMTIME	dt;
	GetLocalTime(&dt);
	TCHAR szDevResultPath[_MAX_PATH] = _T("");
	_stprintf(szDevResultPath, _T("%s%s%04d%02d%02d_%02d%02d%s"),
		g_szLogPath, _T("EyeTest_"),
		dt.wYear, dt.wMonth, dt.wDay, dt.wHour, dt.wMinute, g_szLogExt);

	FILE* fp = NULL;
	long lFLen = 0;
	if ((fp = _tfopen(szDevResultPath, _T("at"))) != NULL)
	{
		TCHAR szLogString[_MAX_PATH] = _T("");
		for (auto &&k : vecRoiSPDeviation)
		{
			TCHAR* pszName = k.szName;
			RECT roiMax = k.roi;
			float fDeviation = k.fMaxDeviation;
			int cx = (roiMax.left + roiMax.right) / 2;
			int cy = (roiMax.top + roiMax.bottom) / 2;
			_stprintf(szLogString, _T("%s\t%d\t%d\t%0.5f"), pszName, cx, cy, fDeviation);

			_ftprintf(fp, _T("%s\n"), szLogString);
		}		
		lFLen = ftell(fp);
		fclose(fp);
	}
	vecRoiSPDeviation.clear();

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
	// rollback	
	SharedBuffer origin = getBufferPoolManager().getBuffer(CAMERA_FRAME_BUFFER_POOL_ID, sequenceIdBuffer);
	if (origin != nullptr)
	{
		memcpy(origin->getBuffer(), buffer, FRAMESIZE_FOR_CAMERA_FRAME);
		origin->setAvailable(FRAMESIZE_FOR_CAMERA_FRAME);
		origin->setTick(GetTickCount());
		getMultiCoreQueueForCameraInput().putSharedBuffer(origin);
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

void Controller::LoadConfiguration()
{
	if (_tcslen(m_szConfPath) <= 0 || _taccess(m_szConfPath, 0) != 0)
	{
		RUN_MODULE_NAME(getCtrlInstance(), m_szAppPath, m_szAppName, NULL);
		_stprintf(m_szConfPath, _T("%s%s_conf.ini"), m_szAppPath, m_szAppName);
	}
	m_nEyeFindScanWidth = GetPrivateProfileInt(_T("EYEFIND"), _T("SCAN_WIDTH"), 800, m_szConfPath);
	m_nEyeFindScanHeight = GetPrivateProfileInt(_T("EYEFIND"), _T("SCAN_HEIGHT"), 600, m_szConfPath);
	m_nEyeFindThreshold = GetPrivateProfileInt(_T("EYEFIND"), _T("THRESHOLD"), 255, m_szConfPath);

	m_bEyeFindPixelPctCheck = (bool)GetPrivateProfileInt(_T("EYEFIND"), _T("PIXEL_PCT_CHECK"), 1, m_szConfPath);
	m_nEyeFindPixelPctMin = GetPrivateProfileInt(_T("EYEFIND"), _T("PIXEL_PCT_MIN"), 10, m_szConfPath);
	m_nEyeFindPixelPctMax = GetPrivateProfileInt(_T("EYEFIND"), _T("PIXEL_PCT_MAX"), 60, m_szConfPath);

	m_nEyeFindDevValueRef = GetPrivateProfileInt(_T("EYEFIND"), _T("DEV_VALUE_REF"), 100000, m_szConfPath);
	m_nEyeFindDevValueGroupCnt = GetPrivateProfileInt(_T("EYEFIND"), _T("DEV_VALUE_GROUP"), 5, m_szConfPath);

	m_nEyeFindScanBaseValue = GetPrivateProfileInt(_T("EYEFIND"), _T("BASE_VALUE"), 0, m_szConfPath);
	m_nEyeFindResultAdded = GetPrivateProfileInt(_T("EYEFIND"), _T("RESULT_ADDED"), 0, m_szConfPath);
	m_bEyeFindCheckCond = (bool)GetPrivateProfileInt(_T("EYEFIND"), _T("CHECK_COND"), 1, m_szConfPath);
	m_nEyeFindAreaMin = GetPrivateProfileInt(_T("EYEFIND"), _T("AREA_MIN"), 100, m_szConfPath);
	m_nEyeFindAreaMax = GetPrivateProfileInt(_T("EYEFIND"), _T("AREA_MAX"), 400, m_szConfPath);
	m_nEyeFindLineMin = GetPrivateProfileInt(_T("EYEFIND"), _T("LINE_MIN"), 10, m_szConfPath);
	m_nEyeFindLineMax = GetPrivateProfileInt(_T("EYEFIND"), _T("LINE_MAX"), 64, m_szConfPath);
	m_nEyeFindLineGroupMin = GetPrivateProfileInt(_T("EYEFIND"), _T("LINE_GROUP_MIN"), 10, m_szConfPath);
	m_nEyeFindLineGroupMax = GetPrivateProfileInt(_T("EYEFIND"), _T("LINE_GROUP_MAX"), 64, m_szConfPath);
	m_nEyeFindRatioMin = GetPrivateProfileInt(_T("EYEFIND"), _T("RATIO_MIN"), 80, m_szConfPath);
	m_bEyeFindUseOpenCV = (bool)GetPrivateProfileInt(_T("EYEFIND"), _T("USE_CVFIND"), 1, m_szConfPath);
	m_nEyeFindCheckInterval = GetPrivateProfileInt(_T("EYEFIND"), _T("CHECK_INTERVAL"), 15, m_szConfPath);
	m_bEyeFindViewSPAll = (bool)GetPrivateProfileInt(_T("EYEFIND"), _T("VIEWSP_ALL"), 0, m_szConfPath);
	m_bEyeFindSaveMask = (bool)GetPrivateProfileInt(_T("EYEFIND"), _T("SAVE_MASK"), 0, m_szConfPath);
	m_bEyeFindSaveSpecular = (bool)GetPrivateProfileInt(_T("EYEFIND"), _T("SAVE_SPECULAR"), 0, m_szConfPath);
	m_bEyeFindTimeLogging = (bool)GetPrivateProfileInt(_T("EYEFIND"), _T("TIME_LOGGING"), 0, m_szConfPath);

	m_bEyeFindTimeLogging = (bool)GetPrivateProfileInt(_T("EYEFIND"), _T("TIME_LOGGING"), 0, m_szConfPath);
	m_bEyeFindCondLogging = (bool)GetPrivateProfileInt(_T("EYEFIND"), _T("COND_LOGGING"), 0, m_szConfPath);	
	m_nEyeFindScanLeftIdx = (WIDTH_FOR_CAMERA_CROP - m_nEyeFindScanWidth) / 2;
	m_nEyeFindScanTopIdx = (HEIGHT_FOR_CAMERA_CROP - m_nEyeFindScanHeight) / 2;
	m_nEyeFindScanRightIdx = (m_nEyeFindScanLeftIdx + m_nEyeFindScanWidth);
	m_nEyeFindScanBottomIdx = (m_nEyeFindScanTopIdx + m_nEyeFindScanHeight);
	
	m_nEyeDistRoiDimension = GetPrivateProfileInt(_T("EYEDIST"), _T("ROI_DIMENSION"), 192, m_szConfPath);
	m_nEyeDistRoiSample = GetPrivateProfileInt(_T("EYEDIST"), _T("ROI_SAMPLE"), 3, m_szConfPath);
	m_nEyeDistExcludeThreshold = GetPrivateProfileInt(_T("EYEDIST"), _T("EXCLUDE_THRESHOLD"), 0, m_szConfPath);
	m_bEyeDistSaveImg = (bool)GetPrivateProfileInt(_T("EYEDIST"), _T("SAVE_DIST"), 0, m_szConfPath);
	m_bEyeDistTimeLogging = (bool)GetPrivateProfileInt(_T("EYEDIST"), _T("TIME_LOGGING"), 0, m_szConfPath);
}

void Controller::UpdateRoiRect(RECT& rtRet, int row, int col, int& nPixelCnt)
{
	nPixelCnt++;
	if (rtRet.left == 0 && rtRet.right == 0 && rtRet.top == 0 && rtRet.bottom == 0)
	{
		rtRet.left = col;
		rtRet.right = col;
		rtRet.top = row;
		rtRet.bottom = row;
	}
	else
	{
		// col
		if (col < rtRet.left)
		{
			rtRet.left = col;
		}
		else if (col > rtRet.right)
		{
			rtRet.right = col;
		}
		else
		{
			//
		}
		// row
		if (row < rtRet.top)
		{
			rtRet.top = row;
		}
		else if (row > rtRet.bottom)
		{
			rtRet.bottom = row;
		}
		else
		{
			//
		}
	}
}

void Controller::FindSpecular(unsigned char* dest, int row, int col, unsigned char nThreshold, RECT& rtRet, int& nPixelCnt)
{
	int nOffset = 1;
	int nIdxPx_Check = (((row + 0) * WIDTH_FOR_CAMERA_CROP) + (col + 0));

	int nOffsetRight = 0, nOffsetLeft = 0, nOffsetUp = 0, nOffsetDown = 0;
	int nIdxPx_Right = 0, nIdxPx_Left = 0, nIdxPx_Up = 0, nIdxPx_Down = 0;
	unsigned char nValueRight, nValueLeft, nValueUp, nValueDown;

	int lineMin = getController().GetEyeFindScanLineMin();
	int lineMax = getController().GetEyeFindScanLineMax();

	unsigned char nValueCheck = dest[nIdxPx_Check];
	if (nValueCheck >= nThreshold)
	{
		UpdateRoiRect(rtRet, row, col, nPixelCnt);

		// right
		int nOffsetRight = nOffset;
		nIdxPx_Right = (((row + 0) * WIDTH_FOR_CAMERA_CROP) + (col + nOffsetRight));
		nValueRight = dest[nIdxPx_Right];
		while (nValueRight >= nThreshold)
		{
			UpdateRoiRect(rtRet, row, col + nOffsetRight, nPixelCnt);
			nOffsetRight++;
			nIdxPx_Right = (((row + 0) * WIDTH_FOR_CAMERA_CROP) + (col + nOffsetRight));
			if (col + nOffsetRight > getController().GetEyeFindScanRightIdx())
			{
				break;
			}
			if (nOffsetRight > lineMax)
			{
				rtRet = { 0, };
				return;
			}
			nValueRight = dest[nIdxPx_Right];
		}

		// down
		int nOffsetDown = nOffset;
		nIdxPx_Down = (((row + nOffsetDown) * WIDTH_FOR_CAMERA_CROP) + (col + 0));
		nValueDown = dest[nIdxPx_Down];
		while (nValueDown >= nThreshold)
		{
			UpdateRoiRect(rtRet, row + nOffsetDown, col, nPixelCnt);

			// right
			int nOffsetRight = nOffset;
			nIdxPx_Right = (((row + nOffsetDown) * WIDTH_FOR_CAMERA_CROP) + (col + nOffsetRight));
			nValueRight = dest[nIdxPx_Right];
			while (nValueRight >= nThreshold)
			{
				UpdateRoiRect(rtRet, row + nOffsetDown, col + nOffsetRight, nPixelCnt);
				nOffsetRight++;
				if (col + nOffsetRight > getController().GetEyeFindScanRightIdx())
				{
					break;
				}
				if (nOffsetRight > lineMax)
				{
					rtRet = { 0, };
					return;
				}
				nIdxPx_Right = (((row + nOffsetDown) * WIDTH_FOR_CAMERA_CROP) + (col + nOffsetRight));
				nValueRight = dest[nIdxPx_Right];
			}

			// left
			int nOffsetLeft = nOffset;
			nIdxPx_Left = (((row + nOffsetDown) * WIDTH_FOR_CAMERA_CROP) + (col - nOffsetLeft));
			nValueLeft = dest[nIdxPx_Left];
			while (nValueLeft >= nThreshold)
			{
				UpdateRoiRect(rtRet, row + nOffsetDown, col - nOffsetLeft, nPixelCnt);
				nOffsetLeft++;
				if (col - nOffsetLeft < getController().GetEyeFindScanLeftIdx())
				{
					break;
				}
				if (nOffsetLeft > lineMax)
				{
					rtRet = { 0, };
					return;
				}
				nIdxPx_Left = (((row + nOffsetDown) * WIDTH_FOR_CAMERA_CROP) + (col - nOffsetLeft));
				nValueLeft = dest[nIdxPx_Left];
			}

			nOffsetDown++;
			if (row + nOffsetDown > getController().GetEyeFindScanBottomIdx())
			{
				break;
			}
			if (nOffsetDown > lineMax)
			{
				rtRet = { 0, };
				return;
			}
			nIdxPx_Down = (((row + nOffsetDown) * WIDTH_FOR_CAMERA_CROP) + (col + 0));
			nValueDown = dest[nIdxPx_Down];
		}

		// up
		int nOffsetUp = nOffset;
		nIdxPx_Up = (((row - nOffsetUp) * WIDTH_FOR_CAMERA_CROP) + (col + 0));
		nValueUp = dest[nIdxPx_Up];
		while (nValueUp >= nThreshold)
		{
			UpdateRoiRect(rtRet, row - nOffsetUp, col, nPixelCnt);

			// right
			int nOffsetRight = nOffset;
			nIdxPx_Right = (((row - nOffsetUp) * WIDTH_FOR_CAMERA_CROP) + (col + nOffsetRight));
			nValueRight = dest[nIdxPx_Right];
			while (nValueRight >= nThreshold)
			{
				UpdateRoiRect(rtRet, row - nOffsetUp, col + nOffsetRight, nPixelCnt);
				nOffsetRight++;
				if (col + nOffsetRight > getController().GetEyeFindScanRightIdx())
				{
					break;
				}
				if (nOffsetRight > lineMax)
				{
					rtRet = { 0, };
					return;
				}
				nIdxPx_Right = (((row - nOffsetUp) * WIDTH_FOR_CAMERA_CROP) + (col + nOffsetRight));
				nValueRight = dest[nIdxPx_Right];
			}

			// left
			int nOffsetLeft = nOffset;
			nIdxPx_Left = (((row - nOffsetUp) * WIDTH_FOR_CAMERA_CROP) + (col - nOffsetLeft));
			nValueLeft = dest[nIdxPx_Left];
			while (nValueLeft >= nThreshold)
			{
				UpdateRoiRect(rtRet, row - nOffsetUp, col - nOffsetLeft, nPixelCnt);
				nOffsetLeft++;
				if (col - nOffsetLeft < getController().GetEyeFindScanLeftIdx())
				{
					break;
				}
				if (nOffsetLeft > lineMax)
				{
					rtRet = { 0, };
					return;
				}
				nIdxPx_Left = (((row - nOffsetUp) * WIDTH_FOR_CAMERA_CROP) + (col - nOffsetLeft));
				nValueLeft = dest[nIdxPx_Left];
			}

			nOffsetUp++;
			if (row - nOffsetUp < getController().GetEyeFindScanTopIdx())
			{
				break;
			}
			if (nOffsetUp > lineMax)
			{
				rtRet = { 0, };
				return;
			}
			nIdxPx_Up = (((row - nOffsetUp) * WIDTH_FOR_CAMERA_CROP) + (col + 0));			
			nValueUp = dest[nIdxPx_Up];
		}
	}
}

void Controller::FindSpecularRecurse(unsigned char* dest, int row, int col, unsigned char nThreshold, int* bufTest, cv::Rect& rtRet, int nCheckValue, int& nPixelCnt, int& nRecurseCnt)
{
	nRecurseCnt++;

	int nIdxPx_Check = (((row + 0) * WIDTH_FOR_CAMERA_CROP) + (col + 0));
	int nTestCheck = bufTest[nIdxPx_Check];
	bufTest[nIdxPx_Check] = nCheckValue;

	unsigned char nValuePx = dest[nIdxPx_Check];
	if (nValuePx >= nThreshold)
	{
		nPixelCnt++;
		if (rtRet.empty())
		{
			rtRet.x = col;
			rtRet.width = 1;
			rtRet.y = row;
			rtRet.height = 1;
		}
		else
		{
			// col
			if (col > rtRet.x)
			{
				rtRet.width = (col - rtRet.x) + 1;
			}
			else
			{
				rtRet.x = col;
				rtRet.width = (rtRet.x - col) + 1;
			}
			// row
			if (row > rtRet.y)
			{
				rtRet.height = (row - rtRet.y) + 1;
			}
			else
			{
				rtRet.y = row;
				rtRet.height = (rtRet.y - row) + 1;
			}
		}

		// up
		nIdxPx_Check = (((row - 1) * WIDTH_FOR_CAMERA_CROP) + (col + 0));
		nTestCheck = bufTest[nIdxPx_Check];
		if (nTestCheck != nCheckValue && nRecurseCnt < 2000)
		{
			if ((row - 1) >= getController().GetEyeFindScanTopIdx())
			{
				getController().FindSpecularRecurse(dest, row - 1, col, nThreshold, bufTest, rtRet, nCheckValue, nPixelCnt, nRecurseCnt);
			}
		}
		// down
		nIdxPx_Check = (((row + 1) * WIDTH_FOR_CAMERA_CROP) + (col + 0));
		nTestCheck = bufTest[nIdxPx_Check];
		if (nTestCheck != nCheckValue && nRecurseCnt < 2000)
		{
			if ((row + 1) <= getController().GetEyeFindScanBottomIdx())
			{
				getController().FindSpecularRecurse(dest, row + 1, col, nThreshold, bufTest, rtRet, nCheckValue, nPixelCnt, nRecurseCnt);
			}
		}
		// left
		nIdxPx_Check = (((row + 0) * WIDTH_FOR_CAMERA_CROP) + (col - 1));
		nTestCheck = bufTest[nIdxPx_Check];
		if (nTestCheck != nCheckValue && nRecurseCnt < 2000)
		{
			if ((col - 1) >= getController().GetEyeFindScanLeftIdx())
			{
				getController().FindSpecularRecurse(dest, row, col - 1, nThreshold, bufTest, rtRet, nCheckValue, nPixelCnt, nRecurseCnt);
			}
		}
		// right
		nIdxPx_Check = (((row + 0) * WIDTH_FOR_CAMERA_CROP) + (col + 1));
		nTestCheck = bufTest[nIdxPx_Check];
		if (nTestCheck != nCheckValue && nRecurseCnt < 2000)
		{
			if ((col + 1) <= getController().GetEyeFindScanRightIdx())
			{
				getController().FindSpecularRecurse(dest, row, col + 1, nThreshold, bufTest, rtRet, nCheckValue, nPixelCnt, nRecurseCnt);
			}
		}
	}
	else
	{
		return;
	}
}

int Controller::MaskingChunkFromBuf(unsigned char* pBufSrc, unsigned char* pBufDest, int nRowStart, int nRowEnd, int nColStart, int nColEnd, int nThreshold, int nPixelContrast, int& nCntAll, int& nCntUpSide, int& nCntDnSide, bool bTimeLogging)
{
	CEllapsedTime timeLogging;
	if (bTimeLogging)
	{
		FUNCTION_NAME_IS(_T("Controller::MaskingChunkFromBuf"));
		LOG_PRINTF(0, _T("Starting masking. (Threshold= %d)"), nThreshold);
		timeLogging.StartEllapsedTime();
	}

	int nBaseValue = m_nEyeFindScanBaseValue;
	int nIdxPixel, nIdxPxUp, nIdxPxDn, nIdxPxLt, nIdxPxRt;
	int nCenterLine = nRowEnd / 2;
	for (int row = nRowStart; row < nRowEnd; row++)
	{
		for (int col = nColStart; col < nColEnd; col++)
		{
			nIdxPixel = ((row * WIDTH_FOR_CAMERA_CROP) + col);
			unsigned char pixel = pBufSrc[nIdxPixel];
			unsigned char value = 0;
			if (pixel < nThreshold)
			{
				value = 0;
			}
			else
			{
				value = 1;
				/*
				// up (base+=1)
				if (row - 1 >= nRowStart)
				{
					nIdxPxUp = (((row - 1) * WIDTH_FOR_CAMERA_CROP) + col);
					pBufDest[nIdxPxUp] += 1;
				}
				// down (base+=1)
				if (row + 1 < nRowEnd)
				{
					nIdxPxDn = (((row + 1) * WIDTH_FOR_CAMERA_CROP) + col);
					pBufDest[nIdxPxDn] += 1;
				}
				// left (base+=1)
				if (col - 1 >= nColStart)
				{
					nIdxPxLt = ((row * WIDTH_FOR_CAMERA_CROP) + (col - 1));
					pBufDest[nIdxPxLt] += 1;
				}
				// right (base+=1)
				if (col + 1 < nColEnd)
				{
					nIdxPxRt = ((row * WIDTH_FOR_CAMERA_CROP) + (col + 1));
					pBufDest[nIdxPxRt] += 1;
				}
				*/
			}
			pBufDest[nIdxPixel] += value;

			if (pixel >= nPixelContrast)
			{
				nCntAll++;
				if (row > nCenterLine)
					nCntDnSide++;
				else
					nCntUpSide++;
			}
		}
	}

	if (bTimeLogging)
	{
		double fTimeMask = timeLogging.EndEllapsedTime();
		LOG_PRINTF(0, _T("Finished masking. (Time= %0.5f)"), fTimeMask);
	}
	return nBaseValue;
}

int Controller::FindSpecularCross(unsigned char* dest, char* pszName, int nRowFindStart, int nRowFindEnd, int nColFindStart, int nColFindEnd, int nBaseValue, std::vector<RECT>* pVecRoiSP, std::vector<RECT>* pVecRoiSPNot, int nResultAdded, bool bCheckCond, FILE* fpLog, bool bTimeLogging)
{
	CEllapsedTime timeLogging;
	if (bTimeLogging)
	{
		FUNCTION_NAME_IS(_T("Controller::FindSpecularCross"));
		LOG_PRINTF(0, _T("Starting for finding specular. (BaseValue= %d)"), nBaseValue);
		timeLogging.StartEllapsedTime();
	}

	int nNameId = atoi(pszName);
	TCHAR szName[10] = { 0, };
	_stprintf(szName, _T("%d"), nNameId);

	int nIdxPixel = 0;
	int nOffsetDown = 0;

	for (int row = nRowFindStart; row <= nRowFindEnd; row++)
	{
		for (int col = nColFindStart; col <= nColFindEnd; col++)
		{
			nIdxPixel = ((row * WIDTH_FOR_CAMERA_CROP) + col);
			unsigned char value = dest[nIdxPixel];


			/////////////////////////////////////////////////////////////////////////////
			// already check
			if (value >= 100)
			{
				continue;
			}
			// lower than basevalue
			if (value != 1)
			{
				continue;
			}
			/////////////////////////////////////////////////////////////////////////////


			RECT rt = { col, row, col, row };
			int distance = 0;
			int baseCol = col;
			int nOffsetDown = 0;

			while (value == 1)
			{
				if ((row + nOffsetDown) > nRowFindEnd)
					break;

				nIdxPixel = (((row + nOffsetDown) * WIDTH_FOR_CAMERA_CROP) + baseCol);
				dest[nIdxPixel] += 100;

				int nOffsetRight = 0, nOffsetLeft = 0;

				// right
				do
				{
					if ((baseCol + nOffsetRight + 1) > nColFindEnd)
						break;
					nOffsetRight++;
					nIdxPixel = (((row + nOffsetDown) * WIDTH_FOR_CAMERA_CROP) + (baseCol + nOffsetRight));
					value = dest[nIdxPixel];
					if (value != 1)
						nOffsetRight--;
					dest[nIdxPixel] += 100;
				} while (value == 1);

				// left
				do
				{
					if ((baseCol - nOffsetLeft - 1) < nColFindStart)
						break;
					nOffsetLeft++;
					nIdxPixel = (((row + nOffsetDown) * WIDTH_FOR_CAMERA_CROP) + (baseCol - nOffsetLeft));
					value = dest[nIdxPixel];
					if (value != 1)
						nOffsetLeft--;
					dest[nIdxPixel] += 100;
				} while (value == 1);

				// line length check
				distance = (nOffsetRight + nOffsetLeft) + 1;
				if (distance < m_nEyeFindLineGroupMin || distance > m_nEyeFindLineGroupMax)
				{
					break;
				}

				if ((baseCol - nOffsetLeft) < rt.left)
				{
					rt.left = baseCol - nOffsetLeft;
				}
				if ((baseCol + nOffsetRight) > rt.right)
				{
					rt.right = baseCol + nOffsetRight;
				}
				
				if (distance > 1)
				{
					baseCol = (baseCol - nOffsetLeft + (distance / 2));
				}
				else
				{
					baseCol = baseCol;
				}

				nOffsetDown++;
				nIdxPixel = (((row + nOffsetDown) * WIDTH_FOR_CAMERA_CROP) + baseCol);
				value = dest[nIdxPixel];
				if (value == 1)
				{
					rt.bottom = row + nOffsetDown;
				}
			}
			if (::IsRectEmpty(&rt) == FALSE)
			{
				bool bCondPass = true;
				if (bCheckCond)
				{
					bool bCondLogging = GetEyeFindScanCondLogging();
					bCondPass = CheckSpecularCond(&rt, szName, fpLog, bCondLogging);
				}
				if (bCondPass)
				{
					if (nResultAdded > 0)
					{
						rt.left -= nResultAdded;
						rt.right += nResultAdded;
						rt.top -= nResultAdded;
						rt.bottom += nResultAdded;
					}
					pVecRoiSP->push_back(rt);
				}
				else
				{
					pVecRoiSPNot->push_back(rt);
				}
				// update pixel value to (BaseValue+1)
				/*
				for (int rowMask = rt.top; rowMask <= rt.bottom; rowMask++)
				{
					for (int colMask = rt.left; colMask <= rt.right; colMask++)
					{
						nIdxPixel = ((rowMask * WIDTH_FOR_CAMERA_CROP) + colMask);
						dest[nIdxPixel] = (nBaseValue + 1);
					}
				}
				*/
			}
		}
	}
	if (bTimeLogging)
	{
		double fTimeSpecular = timeLogging.EndEllapsedTime();
		LOG_PRINTF(0, _T("Finished finding specular. (Time= %0.5f)"), fTimeSpecular);
	}

	return pVecRoiSP->size();
}

bool Controller::CheckSpecularCond(RECT* pRt, TCHAR* pszName, FILE* fpLog, bool bCondLogging)
{
	int nWidth = pRt->right - pRt->left;
	int nHeight = pRt->bottom - pRt->top;

	TCHAR szHdr[64] = _T("");
	_stprintf(szHdr, _T("[Name= %s : %d,%d]"), pszName, pRt->left, pRt->top);

	// line
	int lineMin = getController().GetEyeFindScanLineMin();
	int lineMax = getController().GetEyeFindScanLineMax();	
	if (nWidth < lineMin || nWidth > lineMax)
	{
		if (bCondLogging)
			LOG_PRINTF(0, _T("%s Failed to pass condition. [Width= %d]"), szHdr, nWidth);
		//else if (fpLog)
		//	LOG_PRINTF_FP(0, fpLog, _T("%s Failed to pass condition. [Width= %d]"), szHdr, nWidth);
		return false;
	}
	if (nHeight < lineMin || nHeight > lineMax)
	{
		if (bCondLogging)
			LOG_PRINTF(0, _T("%s Failed to pass condition. [Height= %d]"), szHdr, nHeight);
		//else if (fpLog)
		//	LOG_PRINTF_FP(0, fpLog, _T("%s Failed to pass condition. [Height= %d]"), szHdr, nHeight);
		return false;
	}

	// area
	int nArea = (nWidth * nHeight);
	int areaMin = getController().GetEyeFindScanAreaMin();
	int areaMax = getController().GetEyeFindScanAreaMax();
	if (nArea < areaMin || nArea > areaMax)
	{
		if (bCondLogging)
			LOG_PRINTF(0, _T("%s Failed to pass condition. [Area= %d]"), szHdr, nArea);
		//else if (fpLog)
		//	LOG_PRINTF_FP(0, fpLog, _T("%s Failed to pass condition. [Area= %d]"), szHdr, nArea);
		return false;
	}

	// ratio
	int ratioMin = getController().GetEyeFindScanRatioMin();
	float fRatio = 0.0f;
	if (nWidth > nHeight)
		fRatio = ((float)nHeight / (float)nWidth);
	else
		fRatio = ((float)nWidth / (float)nHeight);
	fRatio *= 100;
	if (fRatio < ratioMin)
	{
		if (bCondLogging)
			LOG_PRINTF(0, _T("%s Failed to pass condition. [Ratio= %d]"), szHdr, (int)fRatio);
		//else if (fpLog)
		//	LOG_PRINTF_FP(0, fpLog, _T("%s Failed to pass condition. [Ratio= %d]"), szHdr, (int)fRatio);
		return false;
	}
	
	return true;
}

void Controller::CopyRoiValueToClipboard(unsigned char** copyValue, int row, int col, bool bTimeLogging)
{
	if (::OpenClipboard(NULL))
	{
		::EmptyClipboard();

		int nValueCount = (row * col);
		HGLOBAL hBlock = GlobalAlloc(GMEM_MOVEABLE, sizeof(WCHAR) * ((nValueCount * 5) + 1));
		if (hBlock)
		{
			WCHAR* pwszText = (WCHAR*)GlobalLock(hBlock);
			if (pwszText)
			{
				WCHAR wszValue[10] = _T("");
				WCHAR wszAllValue[_MAX_PATH * 1024] = _T("");
				int nStride = row;
				int nIdx = 0;
				for (int i = 0; i < row; i++)
				{
					WCHAR wszLine[1024] = _T("");
					for (int j = 0; j < row; j++)
					{
						unsigned char value = copyValue[i][j];
						if (((j + 1) % nStride) == 0)
						{
							wsprintf(wszValue, L"%d\n", value);
						}
						else
						{
							wsprintf(wszValue, L"%d\t", value);
						}
						wcscat(wszAllValue, wszValue);
						wcscat(wszLine, wszValue);
					}
					//LOG_PRINTF(0, _T("[Line: %d] %s"), i, wszLine);
				}
				wszAllValue[wcslen(wszAllValue) - 1] = L'\0';

				int nLenCopy = sizeof(WCHAR) * wcslen(wszAllValue);
				CopyMemory(pwszText, wszAllValue, nLenCopy);
				pwszText[wcslen(wszAllValue)] = L'\0';
				GlobalUnlock(hBlock);
			}
			SetClipboardData(CF_UNICODETEXT, hBlock);
		}
		CloseClipboard();

		if (hBlock)
		{
			GlobalFree(hBlock);
		}
	}
}

float Controller::CalculateDistance(unsigned char* src, RECT& rtROI, int nExcludeThreshold, bool bSaveDist, char* pszName, int nBaseValue, int nPixelContrast, int& nCntSPAll, int& nCntSPUp, int& nCntSPDn, bool bTimeLogging)
{
	RECT roiDeviation = rtROI;
	int nRoiDim = (m_nEyeDistRoiDimension / 2);
	int nRoiSample = m_nEyeDistRoiSample;

	char pathDist[256] = { 0, };
	int nNameId = atoi(pszName);
	TCHAR szName[10] = { 0, };
	_stprintf(szName, _T("%d"), nNameId);
	sprintf(pathDist, "c:\\jtwoc\\eyetest\\test_distance_%d.png", nNameId);

	CEllapsedTime timeLogging;
	if (bTimeLogging)
	{
		FUNCTION_NAME_IS(_T("Controller::CalculateDistance"));
		//LOG_PRINTF(0, _T("Starting caculating distance. (Name= %s)"), pszName);
		timeLogging.StartEllapsedTime();
	}
	
	int nROIWidth = (rtROI.right - rtROI.left);
	int nROIHeight = (rtROI.bottom - rtROI.top);
	int nROICenterX = rtROI.left + (nROIWidth / 2);
	int nROICenterY = rtROI.top + (nROIHeight / 2);

	roiDeviation.left = nROICenterX - nRoiDim;
	roiDeviation.right = nROICenterX + nRoiDim;
	roiDeviation.top = nROICenterY - nRoiDim;
	roiDeviation.bottom = nROICenterY + nRoiDim;

	unsigned char** ppCopyValue = NULL;
	if (bTimeLogging)
	{
		ppCopyValue = (unsigned char**)malloc(sizeof(unsigned char*) * m_nEyeDistRoiDimension);
		for (int pi = 0; pi < m_nEyeDistRoiDimension; pi++)
		{
			ppCopyValue[pi] = (unsigned char*)malloc(sizeof(unsigned char) * m_nEyeDistRoiDimension);
		}
	}
	for (int row = roiDeviation.top, i = 0; row < roiDeviation.bottom; row++, i++)
	{
		for (int col = roiDeviation.left, j = 0; col < roiDeviation.right; col++, j++)
		{
			int nIdxPixel = (((row + 0) * WIDTH_FOR_CAMERA_CROP) + (col + 0));
			unsigned char pixel = src[nIdxPixel];
			if (ppCopyValue)
			{
				ppCopyValue[i][j] = pixel;
			}
		}
	}

	if (ppCopyValue)
	{
		CopyRoiValueToClipboard(ppCopyValue, m_nEyeDistRoiDimension, m_nEyeDistRoiDimension, bTimeLogging);
		for (int pi = 0; pi < m_nEyeDistRoiDimension; pi++)
		{
			free(ppCopyValue[pi]);
		}
		free(ppCopyValue);
	}

	int nIdxPixel = 0;
	float fStdDeviationSum = 0.0f;
	int nCntInclude = 0, nCntExclude = 0;
	for (int row = roiDeviation.top; row < (roiDeviation.bottom - nRoiSample); row++)
	{
		for (int col = roiDeviation.left; col < (roiDeviation.right - nRoiSample); col++)
		{
			unsigned char value[3][3] = { 0, };
			double deviation[3][3] = { 0.0, };

			bool bExcludeThreshold = false;
			double fSumValue = 0.0;
			int nCntSum = 0;

			for (int roiY = 0; roiY < nRoiSample; roiY++)
			{
				for (int roiX = 0; roiX < nRoiSample; roiX++)
				{
					nIdxPixel = (((row + roiY) * WIDTH_FOR_CAMERA_CROP) + (col + roiX));
					unsigned char pixel = src[nIdxPixel];
					if (pixel >= nExcludeThreshold)
					{
						bExcludeThreshold = true;
						break;
					}
					value[roiY][roiX] = pixel;
					fSumValue += pixel;
					nCntSum++;
				}
				if (bExcludeThreshold)
					break;
			}
			if (bExcludeThreshold)
			{
				nCntExclude++;
				continue;
			}
			nCntInclude++;

			double fAvg = (fSumValue / (double)(nRoiSample * nRoiSample));
			if (bTimeLogging)
			{
				//LOG_PRINTF(0, _T("[Step.1] Sum= %0.1f, Average= %0.3f, (count= %d)"), fSumValue, fAvg, nCntSum);
			}

			double fSumDeviation = 0.0;
			int nCntSumDev = 0;
			for (int roiY = 0; roiY < nRoiSample; roiY++)
			{
				for (int roiX = 0; roiX < nRoiSample; roiX++)
				{
					double dev = (value[roiY][roiX] - fAvg);
					deviation[roiY][roiX] = (dev * dev);
					fSumDeviation += (dev * dev);
					nCntSumDev++;
				}
			}
			double fVariance = (fSumDeviation / (double)(nRoiSample * nRoiSample));

			if (bTimeLogging)
			{
				//LOG_PRINTF(0, _T("[Step.2] Deviation-Sum= %0.3f, Variance= %0.3f, (count= %d)"), fSumDeviation, fVariance, nCntSumDev);
			}

			float fStdDeviation = sqrt(fVariance);
			if (bTimeLogging)
			{
				//LOG_PRINTF(0, _T("[Step.3] Standard-Deviation= %0.3f"), fStdDeviation);
			}
			fStdDeviationSum += fStdDeviation;
		}
	}
	if (bTimeLogging)
	{
		double fTimeDistance = timeLogging.EndEllapsedTime();
		LOG_PRINTF(0, _T("Finished caculating distance[Name= %5.5s]. (Deviation= %0.3f, Time= %0.5f)"), szName, fStdDeviationSum, fTimeDistance);
	}

	// in the standard deviation calculation, IF Ex > In * (3/4), then “this frame OMIT
	bool bSampleOmit = false;
	float fStdDeviationOmit = 0.0f;
	if (nCntExclude > ((nCntInclude * 3) / 4))
	{
		bSampleOmit = true;
		fStdDeviationOmit = fStdDeviationSum;
		fStdDeviationSum = 0.0f;
	}

	// save distance
	if (bSaveDist)
	{
		cv::Rect roiDev;
		roiDev.x = roiDeviation.left - 2;
		roiDev.y = roiDeviation.top - 2;
		roiDev.width = (roiDeviation.right - roiDeviation.left) + 4;
		roiDev.height = (roiDeviation.bottom - roiDeviation.top) + 4;
		Mat _srcDist = cv::Mat(cvSize(WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP), CV_8UC1, src, cv::Mat::AUTO_STEP);
		Mat srcDist;
		cvtColor(_srcDist, srcDist, cv::COLOR_GRAY2BGR);
		rectangle(srcDist, roiDev, Scalar(0, 255, 0), 1);

		char szDeviation[64] = "";
		if (bSampleOmit)
			sprintf(szDeviation, "[%d,%d]= %0.3f [In= %d, Ex= %d] Omit", nROICenterX, nROICenterY, fStdDeviationOmit, nCntInclude, nCntExclude);
		else
			sprintf(szDeviation, "[%d,%d]= %0.3f [In= %d, Ex= %d]", nROICenterX, nROICenterY, fStdDeviationSum, nCntInclude, nCntExclude);
		int baseLine = 0;
		cv::Size szText = getTextSize(szDeviation, CV_FONT_HERSHEY_PLAIN, 1.0, 1, &baseLine);
		cv::Rect roiText;
		roiText.width = szText.width;
		roiText.height = szText.height + 10;
		roiText.x = roiDeviation.left;
		if (roiText.x + roiText.width > WIDTH_FOR_CAMERA_CROP)
		{
			roiText.x = (WIDTH_FOR_CAMERA_CROP - szText.width - 5);
		}
		roiText.y = roiDeviation.top - (szText.height + 15);
		if (roiText.y + roiText.height > HEIGHT_FOR_CAMERA_CROP)
		{
			roiText.y = (HEIGHT_FOR_CAMERA_CROP - roiText.height - 5);
		}
		cv::Mat textRect = srcDist(roiText);
		cv::Mat color(textRect.size(), CV_8UC3, Scalar(255, 255, 255));
		double alpha = 0.5;
		cv::addWeighted(color, alpha, textRect, 1.0 - alpha, 0.0, textRect);
		cv::Point ptDev = { roiText.x, roiDeviation.top - 10 };
		cv::putText(srcDist, szDeviation, ptDev, CV_FONT_HERSHEY_PLAIN, 1.0, Scalar(255, 0, 255));

		char szPixelCount[64] = "";
		int threshold = GetEyeFindScanThreshold();
		sprintf(szPixelCount, "Pixel[%d] All= %d, Up= %d, Down= %d", nPixelContrast, nCntSPAll, nCntSPUp, nCntSPDn);
		cv::Point ptPixel = { 0, 30 };
		cv::putText(srcDist, szPixelCount, ptPixel, CV_FONT_HERSHEY_PLAIN, 1.2, Scalar(255, 0, 0));

		imwrite(pathDist, srcDist);
	}

	return fStdDeviationSum;
}

float Controller::CalculateDistanceAll(unsigned char* src, std::vector<RECT>* pVecRoiSP, int nExcludeThreshold, bool bSaveDist, char* pszName, int nBaseValue, int nPixelContrast, int& nCntSPAll, int& nCntSPUp, int& nCntSPDn, RECT& rtRoiRet, bool bTimeLogging)
{
	int nRoiDim = (m_nEyeDistRoiDimension / 2);
	int nRoiSample = m_nEyeDistRoiSample;

	char pathDist[256] = { 0, };
	int nNameId = atoi(pszName);
	TCHAR szName[10] = { 0, };
	_stprintf(szName, _T("%d"), nNameId);
	sprintf(pathDist, "c:\\jtwoc\\eyetest\\test_distance_%d.png", nNameId);

	CEllapsedTime timeLogging;
	if (bTimeLogging)
	{
		FUNCTION_NAME_IS(_T("Controller::CalculateDistance"));
		//LOG_PRINTF(0, _T("Starting caculating distance. (Name= %s)"), pszName);
		timeLogging.StartEllapsedTime();
	}

	float fStdDeviationMax = 0.0f;
	RECT rtRoiMax = { 0, };

	Mat _srcDist = cv::Mat(cvSize(WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP), CV_8UC1, src, cv::Mat::AUTO_STEP);
	Mat srcDist;
	cvtColor(_srcDist, srcDist, cv::COLOR_GRAY2BGR);

	for (auto && roi : *pVecRoiSP)
	{
		RECT roiDeviation = roi;
		
		int nROIWidth = (roi.right - roi.left);
		int nROIHeight = (roi.bottom - roi.top);
		int nROICenterX = roi.left + (nROIWidth / 2);
		int nROICenterY = roi.top + (nROIHeight / 2);

		roiDeviation.left = nROICenterX - nRoiDim;
		roiDeviation.right = nROICenterX + nRoiDim;
		roiDeviation.top = nROICenterY - nRoiDim;
		roiDeviation.bottom = nROICenterY + nRoiDim;

		int nIdxPixel = 0;
		float fStdDeviationSum = 0.0f;
		int nCntInclude = 0, nCntExclude = 0;
		for (int row = roiDeviation.top; row < (roiDeviation.bottom - nRoiSample); row++)
		{
			for (int col = roiDeviation.left; col < (roiDeviation.right - nRoiSample); col++)
			{
				unsigned char value[3][3] = { 0, };
				double deviation[3][3] = { 0.0, };

				bool bExcludeThreshold = false;
				double fSumValue = 0.0;
				int nCntSum = 0;
				for (int roiY = 0; roiY < nRoiSample; roiY++)
				{
					for (int roiX = 0; roiX < nRoiSample; roiX++)
					{
						nIdxPixel = (((row + roiY) * WIDTH_FOR_CAMERA_CROP) + (col + roiX));
						unsigned char pixel = src[nIdxPixel];
						if (pixel >= nExcludeThreshold)
						{
							bExcludeThreshold = true;
							break;
						}
						value[roiY][roiX] = pixel;
						fSumValue += pixel;
						nCntSum++;
					}
					if (bExcludeThreshold)
						break;
				}
				if (bExcludeThreshold)
				{
					nCntExclude++;
					continue;
				}
				nCntInclude++;

				double fAvg = (fSumValue / (double)(nRoiSample * nRoiSample));
				if (bTimeLogging)
				{
					//LOG_PRINTF(0, _T("[Step.1] Sum= %0.1f, Average= %0.3f, (count= %d)"), fSumValue, fAvg, nCntSum);
				}

				double fSumDeviation = 0.0;
				int nCntSumDev = 0;
				for (int roiY = 0; roiY < nRoiSample; roiY++)
				{
					for (int roiX = 0; roiX < nRoiSample; roiX++)
					{
						double dev = (value[roiY][roiX] - fAvg);
						deviation[roiY][roiX] = (dev * dev);
						fSumDeviation += (dev * dev);
						nCntSumDev++;
					}
				}
				double fVariance = (fSumDeviation / (double)(nRoiSample * nRoiSample));

				if (bTimeLogging)
				{
					//LOG_PRINTF(0, _T("[Step.2] Deviation-Sum= %0.3f, Variance= %0.3f, (count= %d)"), fSumDeviation, fVariance, nCntSumDev);
				}

				float fStdDeviation = sqrt(fVariance);
				if (bTimeLogging)
				{
					//LOG_PRINTF(0, _T("[Step.3] Standard-Deviation= %0.3f"), fStdDeviation);
				}
				fStdDeviationSum += fStdDeviation;
			}
		}
		// in the standard deviation calculation, IF Ex > In * (3/4), then “this frame OMIT
		bool bSampleOmit = false;
		float fStdDeviationOmit = 0.0f;
		if (nCntExclude > ((nCntInclude * 3) / 4))
		{
			bSampleOmit = true;
			fStdDeviationOmit = fStdDeviationSum;
			fStdDeviationSum = 0.0f;
		}

		if (fStdDeviationSum > fStdDeviationMax)
		{
			fStdDeviationMax = fStdDeviationSum;
			rtRoiMax = roiDeviation;
		}

		if (bTimeLogging)
		{
			double fTimeDistance = timeLogging.EndEllapsedTime();
			LOG_PRINTF(0, _T("Finished caculating distance[Name= %5.5s]. (Deviation= %0.3f, Time= %0.5f)"), szName, fStdDeviationSum, fTimeDistance);
		}

		cv::Rect roiDev;
		roiDev.x = roiDeviation.left;
		roiDev.y = roiDeviation.top;
		roiDev.width = (roiDeviation.right - roiDeviation.left);
		roiDev.height = (roiDeviation.bottom - roiDeviation.top);
		rectangle(srcDist, roiDev, Scalar(0, 0, 255), 1);

		cv::Rect roiSP;
		roiSP.x = roi.left;
		roiSP.y = roi.top;
		roiSP.width = (roi.right - roi.left);
		roiSP.height = (roi.bottom - roi.top);
		rectangle(srcDist, roiSP, Scalar(255, 0, 0), 1);

		char szDeviation[64] = "";
		if (bSampleOmit)
			sprintf(szDeviation, "[%d,%d]= %0.3f [In= %d, Ex= %d] Omit", nROICenterX, nROICenterY, fStdDeviationOmit, nCntInclude, nCntExclude);
		else
			sprintf(szDeviation, "[%d,%d]= %0.3f [In= %d, Ex= %d]", nROICenterX, nROICenterY, fStdDeviationSum, nCntInclude, nCntExclude);
		int baseLine = 0;
		cv::Size szText = getTextSize(szDeviation, CV_FONT_HERSHEY_PLAIN, 1.0, 1, &baseLine);
		cv::Rect roiText;
		roiText.width = szText.width;
		roiText.height = szText.height + 10;
		roiText.x = roiDeviation.left;
		if (roiText.x + roiText.width > WIDTH_FOR_CAMERA_CROP)
		{
			roiText.x = (WIDTH_FOR_CAMERA_CROP - szText.width - 5);
		}
		roiText.y = roiDeviation.top - (szText.height + 15);
		if (roiText.y + roiText.height > HEIGHT_FOR_CAMERA_CROP)
		{
			roiText.y = (HEIGHT_FOR_CAMERA_CROP - roiText.height - 5);
		}
		cv::Mat textRect = srcDist(roiText);
		cv::Mat color(textRect.size(), CV_8UC3, Scalar(255, 255, 255));
		double alpha = 0.5;
		cv::addWeighted(color, alpha, textRect, 1.0 - alpha, 0.0, textRect);
		cv::Point ptDev = { roiText.x, roiDeviation.top - 10 };
		cv::putText(srcDist, szDeviation, ptDev, CV_FONT_HERSHEY_PLAIN, 1.0, Scalar(255, 0, 255));
	}

	cv::Rect roiDevMax;
	roiDevMax.x = rtRoiMax.left;
	roiDevMax.y = rtRoiMax.top;
	roiDevMax.width = (rtRoiMax.right - rtRoiMax.left);
	roiDevMax.height = (rtRoiMax.bottom - rtRoiMax.top);
	rectangle(srcDist, roiDevMax, Scalar(0, 255, 0), 1);

	// save distance
	if (bSaveDist)
	{
		char szPixelCount[64] = "";
		int threshold = GetEyeFindScanThreshold();
		sprintf(szPixelCount, "Pixel[%d] All= %d, Up= %d, Down= %d", nPixelContrast, nCntSPAll, nCntSPUp, nCntSPDn);
		cv::Point ptPixel = { 0, 30 };
		cv::putText(srcDist, szPixelCount, ptPixel, CV_FONT_HERSHEY_PLAIN, 1.2, Scalar(255, 0, 0));

		imwrite(pathDist, srcDist);
	}

	rtRoiRet = rtRoiMax;
	return fStdDeviationMax;
}

void Controller::CallEyeFindTestFile(char* pszFilePath)
{
	if (access(pszFilePath, 0) != 0)
		return;

	int cchDestChar = strlen((LPCSTR)pszFilePath) + 1;
	TCHAR wszDestPath[_MAX_PATH] = _T("");
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pszFilePath, -1, wszDestPath, cchDestChar - 1);

	FUNCTION_NAME_IS(_T("Controller::CallEyeFindTestFile"));
	LOG_PRINTF(0, _T("Calling [EyeFind] File: %s"), wszDestPath);

	cv::Mat image = cv::imread(pszFilePath, IMREAD_GRAYSCALE);
	unsigned char* src = image.data;
	if (src)
	{
		char szFName[_MAX_FNAME] = "";
		char* pszFName = NULL;
		_splitpath(pszFilePath, NULL, NULL, szFName, NULL);
		if (strlen(szFName) > 0)
		{
			pszFName = szFName;
			char* pch = strrchr(szFName, '_');
			if (pch)
			{
				pszFName = pch + 1;
			}
		}

		int nCntSPAll = 0, nCntSPUp = 0, nCntSPDn = 0;
		FILE* fp = NULL;
		//if (m_bEyeFindViewSPAll)
		//	fp = LOG_PRINTF_FP(0, NULL, _T(""));
		float fDeviationMax = 0.0f;
		fDeviationMax = CallEyeFindTest(src, pszFName, nCntSPAll, nCntSPUp, nCntSPDn, NULL, fp);
		if (fp)
		{
			fclose(fp);
		}
	}
}

float Controller::CallEyeFindTest(unsigned char* src, char* pszName, int& nCntSPAll, int& nCntSPUp, int& nCntSPDn, ROI_RESULT* pRoiResult, FILE* fpLog)
{
	int ScanHeight = GetEyeFindScanHeight();
	int ScanWidth = GetEyeFindScanWidth();
	int rowStart = (HEIGHT_FOR_CAMERA_CROP - ScanHeight) / 2;
	int colStart = (WIDTH_FOR_CAMERA_CROP - ScanWidth) / 2;
	int threshold = GetEyeFindScanThreshold();
	int resultAdded = GetEyeFindScanResultAdded();
	int areaMin = GetEyeFindScanAreaMin();
	int areaMax = GetEyeFindScanAreaMax();
	int lineMin = GetEyeFindScanLineMin();
	int lineMax = GetEyeFindScanLineMax();
	int ratioMin = GetEyeFindScanRatioMin();
	bool bSaveMask = GetEyeFindScanSaveMask();
	bool bSaveSpecular = GetEyeFindScanSaveSpecular();
	bool bTimeLogging = GetEyeFindScanTimeLogging();
	int nRowFindStart = GetEyeFindScanTopIdx();
	int nRowFindEnd = GetEyeFindScanBottomIdx();
	int nColFindStart = GetEyeFindScanLeftIdx();
	int nColFindEnd = GetEyeFindScanRightIdx();
	bool bCheckCond = GetEyeFindScanCheckCond();
	bool bDistTimeLogging = GetEyeDistTimeLogging();
	int nDistExcludeThreshold = GetEyeDistExcludeThreshold();
	bool bSaveDist = GetEyeDistSaveImage();

	int checkInterval = GetEyeFindScanCheckInterval();
	CEllapsedTime timeFind;

	SharedBuffer bufMask = getBufferPoolManager().getBuffer(MASK_CROP_BUFFER_POOL_ID);
	SharedBuffer bufTest = getBufferPoolManager().getBuffer(MASK_TEST_BUFFER_POOL_ID);
	gFindSpecularIdx++;
	if (gFindSpecularIdx == checkInterval)
	{
		gFindSpecularIdx = 0;
		vecRoiSP.clear();
		vecRoiSPNot.clear();
	}

	BOOL bCandidateOk = FALSE;
	float fDeviationMax = 0.0f;
	RECT roiMax = { 0, };

	if (bufMask != nullptr && gFindSpecularIdx == 0)
	{
		int nNameId = atoi(pszName);
		TCHAR szName[10] = { 0, };
		_stprintf(szName, _T("%d"), nNameId);

		unsigned char* dest = (unsigned char*)bufMask->getBuffer();
		memset(dest, 0x00, FRAMESIZE_FOR_MASK_CROP);

		int nRowStart = 0;
		int nRowEnd = HEIGHT_FOR_CAMERA_CROP;
		int nColStart = 0;
		int nColEnd = WIDTH_FOR_CAMERA_CROP;
		int nPixelContrast = 255;
		int nBaseValue = getController().MaskingChunkFromBuf(src, dest, nRowStart, nRowEnd, nColStart, nColEnd, threshold, nPixelContrast, nCntSPAll, nCntSPUp, nCntSPDn, bTimeLogging);
		// pixel pct. check
		if (m_bEyeFindPixelPctCheck)
		{
			int nAllPixelCount = (WIDTH_FOR_CAMERA_CROP * HEIGHT_FOR_CAMERA_CROP);
			float fPixelOkPct = (nCntSPAll * 100) / nAllPixelCount;
			if (fPixelOkPct < m_nEyeFindPixelPctMin || fPixelOkPct > m_nEyeFindPixelPctMax)
			{
				LOG_PRINTF_FP(0, fpLog, _T("[Name= %s] Pixel count Pct. = %0.1f"), szName, fPixelOkPct);
				// too close or too far
				blinkingRedColorLedOn(500);
				return 0.0f;
			}
			else
			{
				yellowColorLedOn();
			}
		}

		if (nBaseValue > 1)
		{
			//
		}
		
		int nRoiCount = getController().FindSpecularCross(dest, pszName, nRowFindStart, nRowFindEnd, nColFindStart, nColFindEnd, nBaseValue, &vecRoiSP, &vecRoiSPNot, resultAdded, bCheckCond, fpLog, bTimeLogging);
		if (nRoiCount <= 0)
		{
			LOG_PRINTF_FP(0, fpLog, _T("[Name= %s] ROI count = %d"), szName, nRoiCount);
		}
		else
		{
			// calculate all roi
			if (m_bEyeFindViewSPAll)
			{
				fDeviationMax = getController().CalculateDistanceAll(src, &vecRoiSP, nDistExcludeThreshold, bSaveDist, pszName, nBaseValue, nPixelContrast, nCntSPAll, nCntSPUp, nCntSPDn, roiMax, bDistTimeLogging);

				// save mask
				if (bSaveMask)
				{
					unsigned char* test = (unsigned char*)bufTest->getBuffer();
					memset(test, 0x00, FRAMESIZE_FOR_MASK_TEST);
					for (int row = 0; row < HEIGHT_FOR_CAMERA_CROP; row++)
					{
						for (int col = 0; col < WIDTH_FOR_CAMERA_CROP; col++)
						{
							int nIdxPixel = ((row * WIDTH_FOR_CAMERA_CROP) + col);
							unsigned char value = dest[nIdxPixel];
							if (value == 1 || value == 101)
								value = 255;
							test[nIdxPixel] = value;
						}
					}
					Mat _srcTest = cv::Mat(cvSize(WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP), CV_8UC1, test, cv::Mat::AUTO_STEP);
					char pathTest[256] = { 0, };
					sprintf(pathTest, "c:\\jtwoc\\eyetest\\test_mask_%s.png", pszName);
					imwrite(pathTest, _srcTest);
				}
			}
			else
			{
				bool bFindEye = false;
				bool bSaveDistRoi = false;

				bool bFirstRoi = true;
				for (auto &&roi : vecRoiSP)
				{
					int nCntRoiAll = 0, nCntRoiUp = 0, nCntRoiDn = 0;
					float fDeviation = getController().CalculateDistance(src, roi, nDistExcludeThreshold, bSaveDistRoi, pszName, nBaseValue, nPixelContrast, nCntRoiAll, nCntRoiUp, nCntRoiDn, bDistTimeLogging);
					int cx = (roi.left + roi.right) / 2;
					int cy = (roi.top + roi.bottom) / 2;
					if (bFirstRoi || (fDeviation > fDeviationMax))
					{
						fDeviationMax = fDeviation;
						roiMax = roi;
						bFirstRoi = false;
						bFindEye = true;
					}
				}

				if (bFindEye)
				{
					RECT rtROI = roiMax;
					// save mask
					if (bSaveMask)
					{
						unsigned char* test = (unsigned char*)bufTest->getBuffer();
						memset(test, 0x00, FRAMESIZE_FOR_MASK_TEST);
						for (int row = 0; row < HEIGHT_FOR_CAMERA_CROP; row++)
						{
							for (int col = 0; col < WIDTH_FOR_CAMERA_CROP; col++)
							{
								int nIdxPixel = ((row * WIDTH_FOR_CAMERA_CROP) + col);
								unsigned char value = dest[nIdxPixel];
								if (value == 1 || value == 101)
									value = 255;
								test[nIdxPixel] = value;
							}
						}
						Mat _srcTest = cv::Mat(cvSize(WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP), CV_8UC1, test, cv::Mat::AUTO_STEP);
						char pathTest[256] = { 0, };
						sprintf(pathTest, "c:\\jtwoc\\eyetest\\test_mask_%s.png", pszName);
						imwrite(pathTest, _srcTest);
					}
					// save specular display
					if (bSaveSpecular)
					{
						cv::Rect roiSP;
						roiSP.x = rtROI.left;
						roiSP.y = rtROI.top;
						roiSP.width = (rtROI.right - rtROI.left);
						roiSP.height = (rtROI.bottom - rtROI.top);
						Mat _srcSP = cv::Mat(cvSize(WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP), CV_8UC1, src, cv::Mat::AUTO_STEP);
						Mat srcSP;
						cvtColor(_srcSP, srcSP, cv::COLOR_GRAY2BGR);
						rectangle(srcSP, roiSP, Scalar(255, 0, 255), 1);
						char pathSP[256] = { 0, };
						sprintf(pathSP, "c:\\jtwoc\\eyetest\\test_specular_%s.png", pszName);
						imwrite(pathSP, srcSP);
					}
					// calculate distance
					float fDeviation = getController().CalculateDistance(src, rtROI, nDistExcludeThreshold, bSaveDist, pszName, nBaseValue, nPixelContrast, nCntSPAll, nCntSPUp, nCntSPDn, bDistTimeLogging);
				}
			}
			if (fDeviationMax > 0.0f)
			{
				//ROI_RESULT result;
				if (pRoiResult)
				{
					pRoiResult->fMaxDeviation = fDeviationMax;
					_tcscpy(pRoiResult->szName, szName);
					pRoiResult->roi = roiMax;
					vecRoiSPDeviation.push_back(*pRoiResult);
				}
			}
		}
		
		if (vecRoiSP.size() > 0)
		{
			//FUNCTION_NAME_IS(_T("Scaler::__internalJob"));
			//LOG_PRINTF(0, _T("[EyeFind] Finished finding specular. (Time= %0.5f)"), fTimeSpecular);
			/*
			char path[256] = "";
			sprintf(path, "c:\\jtwoc\\crop\\crop_roi_%s.png", buf->getName());
			char szFName[256] = "";
			sprintf(szFName, "crop_%s.png", buf->getName());
			std::string strPath = path;
			getImageSaver().saveGrayImage(buf, strPath, WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP);
			*/
			/*
			for (auto &&roi : vecRoiSP)
			{
			drawSpecular(forDisplay, roi);
			LOG_PRINTF(0, _T("[EyeFind] Resulting specular. (Name= %d) [Pos: %d,%d,%d,%d]"), atoi(buf->getName()), roi.x, roi.y, roi.x + roi.width, roi.y + roi.height);
			}
			*/
		}
		bufMask = nullptr;
		bufTest = nullptr;
	}
	return fDeviationMax;
}

void Controller::allIndicationLedOff()
{
	BOOL bSuccess = FALSE;

	COMPLEX_GPIO_PARAM ComplexGpioParam = { 0, };

	ComplexGpioParam.BlinkingTime = INDICATE_LED_OFF;
	ComplexGpioParam.Period = 0;
	ComplexGpioParam.ThresHold = 0;

	bSuccess = bJ2CIris_SetComplexGPIO(handle, GPIO_R, &ComplexGpioParam);
	bSuccess = bJ2CIris_SetComplexGPIO(handle, GPIO_G, &ComplexGpioParam);
	bSuccess = bJ2CIris_SetComplexGPIO(handle, GPIO_B, &ComplexGpioParam);
}

void Controller::redColorLedOn()
{
	BOOL bSuccess = FALSE;
	if (m_eLEDBState != eLED_RED)
	{
		m_eLEDBState = eLED_RED;

		allIndicationLedOff();

		COMPLEX_GPIO_PARAM ComplexGpioParam = { 0, };

		ComplexGpioParam.BlinkingTime = INDICATE_LED_ON;
		ComplexGpioParam.Period = 1;
		ComplexGpioParam.ThresHold = 1;

		bSuccess = bJ2CIris_SetComplexGPIO(handle, GPIO_R, &ComplexGpioParam);
	}
}

void Controller::blueColorLedOn()
{
	BOOL bSuccess = FALSE;
	if (m_eLEDBState != eLED_BLUE)
	{
		m_eLEDBState = eLED_BLUE;

		allIndicationLedOff();

		COMPLEX_GPIO_PARAM ComplexGpioParam = { 0, };

		ComplexGpioParam.BlinkingTime = INDICATE_LED_ON;
		ComplexGpioParam.Period = 1;
		ComplexGpioParam.ThresHold = 1;

		bSuccess = bJ2CIris_SetComplexGPIO(handle, GPIO_B, &ComplexGpioParam);
	}
}

void Controller::greenColorLedOn()
{
	BOOL bSuccess = FALSE;
	if (m_eLEDBState != eLED_GREEN)
	{
		m_eLEDBState = eLED_GREEN;

		allIndicationLedOff();

		COMPLEX_GPIO_PARAM ComplexGpioParam = { 0, };

		ComplexGpioParam.BlinkingTime = INDICATE_LED_ON;
		ComplexGpioParam.Period = 1;
		ComplexGpioParam.ThresHold = 1;

		bSuccess = bJ2CIris_SetComplexGPIO(handle, GPIO_G, &ComplexGpioParam);
	}
}

void Controller::yellowColorLedOn()
{
	BOOL bSuccess = FALSE;
	if (m_eLEDBState != eLED_YELLOW)
	{
		m_eLEDBState = eLED_YELLOW;

		allIndicationLedOff();

		COMPLEX_GPIO_PARAM rComplexGpioParam = { 0, };
		COMPLEX_GPIO_PARAM gComplexGpioParam = { 0, };

		rComplexGpioParam.BlinkingTime = INDICATE_LED_ON;
		rComplexGpioParam.Period = CY_FX_PWM_PERIOD;
		rComplexGpioParam.ThresHold = CY_FX_PWM_PERIOD;

		gComplexGpioParam.BlinkingTime = INDICATE_LED_ON;
		gComplexGpioParam.Period = CY_FX_PWM_PERIOD;
		gComplexGpioParam.ThresHold = CY_FX_PWM_HOLD_A0;

		bSuccess = bJ2CIris_SetComplexGPIO(handle, GPIO_R, &rComplexGpioParam);
		bSuccess = bJ2CIris_SetComplexGPIO(handle, GPIO_G, &gComplexGpioParam);
	}
}

void Controller::blinkingRedColorLedOn(int value)
{
	BOOL bSuccess = FALSE;
	if (m_eLEDBState != eLED_BLINKING)
	{
		m_eLEDBState = eLED_BLINKING;

		allIndicationLedOff();

		COMPLEX_GPIO_PARAM ComplexGpioParam = { 0, };

		ComplexGpioParam.BlinkingTime = value; //ms
		ComplexGpioParam.Period = CY_FX_PWM_PERIOD;
		ComplexGpioParam.ThresHold = CY_FX_PWM_PERIOD;

		bSuccess = bJ2CIris_SetComplexGPIO(handle, GPIO_R, &ComplexGpioParam);
	}
}

void Controller::setSimpleGPIO(BYTE port, BOOL value)
{
	BOOL bSuccess = FALSE;
	bSuccess = bJ2CIris_SetSimpleGPIO(handle, port, value);
}

bool Controller::getSimpleGPIO(BYTE port, BOOL* pbValue)
{
	BOOL bSuccess = FALSE;
	bSuccess = bJ2CIris_GetSimpleGPIO(handle, port, pbValue);
	return bSuccess;
}

void Controller::setI2C(BYTE slaveAddress, BOOL regAddrHigh_true, UINT RegAddr, UINT dataLength, char* data)
{
	BOOL bSuccess = FALSE;
	// regAddrHigh_true = 2byte, if 1 byte, false
	bSuccess = bJ2CIris_SetI2C(handle, slaveAddress, regAddrHigh_true, RegAddr, dataLength, (void*)data);
}

bool Controller::getI2C(BYTE slaveAddress, BOOL regAddrHigh_true, UINT RegAddr, UINT dataLength, char* pbValue)
{
	BOOL bSuccess = FALSE;
	bSuccess = bJ2CIris_GetI2C(handle, slaveAddress, regAddrHigh_true, RegAddr, dataLength, (void*)pbValue);
	return bSuccess;
}
