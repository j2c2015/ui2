#pragma once
#include <iostream>
#include <string>
#include "J2CEngine_dll.h"
#include <windows.h>
#include "NeuroTask.h"
#include <opencv/cv.h>

#define DEVICE_ID_LEN 10
#define PARITAL_WIDTH	64
#define PARTIAL_HEIGHT	64

/////////////////////////////////////////////////////////////////////////////
//
// cscho (2018-12.20)
//
extern HINSTANCE g_hInstApp;
static inline HINSTANCE& getCtrlInstance()
{
	return g_hInstApp;
}
/////////////////////////////////////////////////////////////////////////////

enum Usage { IDLE_USAGE, ENROLL_USAGE, IDENTIFY_USAGE };
class Controller {
public:

	//char partialWindow_[PARITAL_WIDTH*PARITAL_WIDTH];
	char* partialWindow_ = 0;
	static void frame_receive_callback(unsigned char * buffer, DWORD in);
	Usage usage_;
	Controller()
	{
		usage_ = IDLE_USAGE;
		enrollId_ = "test";
		blockTime_ = 0;
	}
	~Controller()
	{
		if (partialWindow_);
		{
			free(partialWindow_);
			partialWindow_ = 0;
		}
	}

	void saveCropImage(SharedBuffer& buf);

	void initBufferPoolManager();
	void initComponents();
	/***************   API ***************/
	void initEngine(int nPartialWidth, int nPartialHeight)
	{
		/////////////////////////////////////////////////////////////////////////////
		//
		// cscho (2018-12.20)
		//
		LoadConfiguration();
		/////////////////////////////////////////////////////////////////////////////

		initBufferPoolManager();
		initComponents();
		initCamera();
		getNeuroTask().initLicense();
		getNeuroTask().setWorkDir("c:\\jtwoc");
		getNeuroTask().reloadEnroll();
		
		m_nPartialWidth = nPartialWidth;
		m_nPartialHeight = nPartialHeight;

		partialWindow_ = (char*)calloc(sizeof(char), m_nPartialWidth*m_nPartialHeight);
	}

	BOOL startCamera();
	BOOL stopCamera();
	BOOL CheckIrisDevOpen();


	J2CRenderCb realTimeRenderCb_ = [](void *, int, int, int) {};
	void setRealTimeRenderCb(J2CRenderCb cb)
	{
		realTimeRenderCb_ = cb;
	}

	J2CRenderCb getRealTimeRenderCb()
	{
		return realTimeRenderCb_;
	}

	J2CRenderCb pixelRenderCb_ = [](void *, int, int, int) {};
	void setPixelRenderCb(J2CRenderCb cb)
	{
		pixelRenderCb_ = cb;
	}

	J2CRenderCb getPixelRenderCb()
	{
		return pixelRenderCb_;
	}

	J2CRenderCb enrollRenderCb_ = [](void *, int, int, int) {};
	void setEnrollRenderCb(J2CRenderCb cb)
	{
		enrollRenderCb_ = cb;
	}

	J2CRenderCb getEnrollRenderCb()
	{
		return enrollRenderCb_;
	}

	J2CRenderCb identifyRenderCb_ = [](void *, int, int, int) {};
	void setIdentifyRenderCb(J2CRenderCb cb)
	{
		identifyRenderCb_ = cb;
	}

	J2CRenderCb getIdentifyRenderCb()
	{
		return identifyRenderCb_;
	}

	J2CIdentifyCb identifyCb_ = [](char *, bool) {};
	void identifyRequest(J2CIdentifyCb cb)
	{
		bool empty = true;
		bool matched = false;
		long long startTick = GetTickCount64();
		long long endTick = startTick + 3000;	// 3sec timeout
		getNeuroTask().clearMatchResultQueue();
		setIdentifyUsage();
		identifyUnBlock();
		identifyCb_ = cb;
		while (endTick > startTick) {
			NeuroTaskResult result = getNeuroTask().getMatchResult(200);	// 3 sec timeout
			if (result.isEmptyResult())
			{
				identifyUnBlock();
#if 0
				identifyCb_("", false);
				setIdleUsage();
				return;
#endif
			}
			else {
				matched = true;
				identifyCb_((char *)result.result.c_str(), true);
				identifyBlock();

				break;
			}
			
			startTick = GetTickCount64();
		}
		identifyBlock();
		setIdleUsage();
		
		if (matched == false)
		{
			identifyCb_("", false);
		}
	}

	void identifyProxyCb(char *id, bool success)
	{
		identifyCb_(id, success);
		identifyUnBlock();
		setIdleUsage();
	}

	J2CEnrollCb enrollCb_ = [](bool) {};
	int currentEnrollCount_ = 0;
	void enrollRequest(char *_id, J2CEnrollCb cb)
	{
		currentEnrollCount_ = 0;
		std::string id(_id);
		setEnrollId(id);
		enrollCb_ = cb;
		setEnrollUsage();
		enrollUnBlock();
		long long start = GetTickCount64();
		long long end = GetTickCount64() + 3000; 
		while (enrollDone_ == false && (end > start ) )
		{
			Sleep(1000);
			start = GetTickCount64();
		}
		std::cout << "*** Enroll control done ***" << std::endl;

		enrollBlock();
		setIdleUsage();	
		if (enrollDone_ == false)
			enrollCb_(false);
	}

	void enrollProxyCb(bool success)
	{
		++currentEnrollCount_;
		enrollUnBlock();
		if (currentEnrollCount_ < enrollCount_)
			return;

		enrollCb_(success);
		enrollDone_ = true;
		setIdleUsage();
		reloadEnroll();
	}

	void reloadEnroll();

	void setIdleUsage()
	{
		usage_ = IDLE_USAGE;
	}

	void setEnrollUsage()
	{
		usage_ = ENROLL_USAGE;
	}

	bool isEnrollUsage()
	{
		return usage_ == ENROLL_USAGE;
	}

	void setIdentifyUsage()
	{
		usage_ = IDENTIFY_USAGE;
	}

	bool isIdentifyUsage()
	{
		return usage_ == IDENTIFY_USAGE;
	}

	std::string enrollId_;
	void setEnrollId(std::string &id)
	{
		enrollId_ = id;
	}

	std::string getEnrollId()
	{
		return enrollId_;
	}

	bool enrollBlocked_ = false;
	void enrollBlock()
	{
		enrollBlocked_ = true;
	}

	void enrollUnBlock()
	{
		enrollBlocked_ = false;
	}

	bool isEnrollBlocked()
	{
		return enrollBlocked_;
	}

	bool identifyBlocked_ = false;
	void identifyBlock()
	{
		identifyBlocked_ = true;
	}

	bool isIdentifyBlocked()
	{
		return identifyBlocked_;
	}

	void identifyUnBlock()
	{
		identifyBlocked_ = false;
	}


	long long blockTime_;
	//long long enrollBlockTime_;
	void blockCameraInput(int ms)
	{
		blockTime_ = GetTickCount64() + ms;
	}

	long long getBlockTime()
	{
		return blockTime_;
	}

	bool isBlocked()
	{
		if (blockTime_ == 0)
			return false;

		long long current = GetTickCount64();
		if (current > blockTime_)
		{
			blockTime_ = 0;
			return false;
		}

		return true;
	}

	J2CRenderCb partialRenderCb_ = [](void *, int, int, int) {};
	bool partialRenderRequested_ = false;
	int partialRenderX_ = 0;
	int partialRenderY_ = 0;
	bool isPartialRenderRequested(int &x, int &y)
	{
		if (partialRenderRequested_ == true)
		{
			partialRenderRequested_ = false;
			x = partialRenderX_;
			y = partialRenderY_;
			return true;
		}
		return false;
	}

	J2CRenderCb getPartialRender()
	{
		return partialRenderCb_;
	}

	void partialRender(int x, int y, J2CRenderCb cb)
	{
		partialRenderRequested_ = true;
		partialRenderCb_ = cb;
		partialRenderX_ = x;
		partialRenderY_ = y;
		std::cout << "Partial Render " << partialRenderX_ << " " << partialRenderY_ << std::endl;

	}

	char *getPartialWindow()
	{
		return partialWindow_;
	}

	void setNeuroWorkDirectory(char *dir);
	void initCamera();

	void cameraPlugin(bool plugged)
	{
		if (plugged == true)
		{
			// TO DO
		}
		else
		{
			// TO DO
		}
	}
	bool isReadyToDraw()
	{
		return readyToDraw_;
	}

	void setReadyToDraw(bool bValue)
	{
		//readyToDraw_ = true;
		readyToDraw_ = bValue;
	}

	int enrollCount_ = 1;
	void setEnrollCount(int count)
	{
		enrollCount_ = count;
	}

	bool readyToDraw_=false;
	bool enrollDone_ = false;
	std::string deviceId_ = "";

	int m_nPartialWidth = PARITAL_WIDTH;
	int m_nPartialHeight = PARTIAL_HEIGHT;
	int getPartialWidth()
	{
		return m_nPartialWidth;
	}
	int getPartialHeight()
	{
		return m_nPartialHeight;
	}

	/////////////////////////////////////////////////////////////////////////////
	//
	// cscho (2018-12.20)
	//
	TCHAR m_szAppPath[_MAX_PATH];
	TCHAR m_szAppName[_MAX_PATH];
	TCHAR m_szConfPath[_MAX_PATH];
	TCHAR* GetAppPath() { return m_szAppPath; }
	TCHAR* GetAppName() { return m_szAppName; }
	TCHAR* GetConfPath() { return m_szConfPath; }
	int m_nEyeFindScanWidth;
	int m_nEyeFindScanHeight;
	int m_nEyeFindThreshold;
	int m_nEyeFindAreaMin;
	int m_nEyeFindAreaMax;
	int GetEyeFindScanWidth() { return m_nEyeFindScanWidth; }
	int GetEyeFindScanHeight() { return m_nEyeFindScanHeight; }
	int GetEyeFindScanThreshold() { return m_nEyeFindThreshold; }
	int GetEyeFindScanAreaMin() { return m_nEyeFindAreaMin; }
	int GetEyeFindScanAreaMax() { return m_nEyeFindAreaMax; }
	int m_nEyeFindResultAdded;
	int GetEyeFindScanResultAdded() { return m_nEyeFindResultAdded; }
	int m_nEyeFindScanBaseValue;
	int GetEyeFindScanBaseValue() { return m_nEyeFindScanBaseValue; }
	int m_nEyeFindLineMin;
	int m_nEyeFindLineMax;	
	int GetEyeFindScanLineMin() { return m_nEyeFindLineMin; }
	int GetEyeFindScanLineMax() { return m_nEyeFindLineMax; }
	int m_nEyeFindRatioMin;
	int GetEyeFindScanRatioMin() { return m_nEyeFindRatioMin; }
	int m_nEyeFindCheckInterval;
	int GetEyeFindScanCheckInterval() { return m_nEyeFindCheckInterval; }
	bool m_bEyeFindTimeLogging;
	bool GetEyeFindScanTimeLogging() { return m_bEyeFindTimeLogging; }
	bool m_bEyeFindCondLogging;
	bool GetEyeFindScanCondLogging() { return m_bEyeFindCondLogging; }
	bool m_bEyeFindCheckCond;
	bool GetEyeFindScanCheckCond() { return m_bEyeFindCheckCond; }
	bool m_bEyeFindSaveMask;
	bool GetEyeFindScanSaveMask() { return m_bEyeFindSaveMask; }
	bool m_bEyeFindSaveSpecular;
	bool GetEyeFindScanSaveSpecular() { return m_bEyeFindSaveSpecular; }
	int m_nEyeDistRoiDimension;
	int GetEyeDistRoiDimension() { return m_nEyeDistRoiDimension; }
	int m_nEyeDistRoiSample;
	int GetEyeDistRoiSample() { return m_nEyeDistRoiSample; }
	bool m_bEyeDistTimeLogging;
	bool GetEyeDistTimeLogging() { return m_bEyeDistTimeLogging; }
	int m_nEyeDistExcludeThreshold;
	int GetEyeDistExcludeThreshold() { return m_nEyeDistExcludeThreshold; }
	bool m_bEyeDistSaveImg;
	bool GetEyeDistSaveImage() { return m_bEyeDistSaveImg; }
	int m_nEyeFindScanLeftIdx;
	int m_nEyeFindScanTopIdx;
	int m_nEyeFindScanRightIdx;
	int m_nEyeFindScanBottomIdx;
	int GetEyeFindScanLeftIdx() { return m_nEyeFindScanLeftIdx; }
	int GetEyeFindScanTopIdx() { return m_nEyeFindScanTopIdx; }
	int GetEyeFindScanRightIdx() { return m_nEyeFindScanRightIdx; }
	int GetEyeFindScanBottomIdx() { return m_nEyeFindScanBottomIdx; }
	void LoadConfiguration();
	void FindSpecularRecurse(unsigned char* dest, int row, int col, unsigned char nThreshold, int* bufTest, cv::Rect& rtRet, int nCheckValue, int& nPixelCnt, int& nRecurseCnt);
	void FindSpecular(unsigned char* dest, int row, int col, unsigned char nThreshold, RECT& rtRet, int& nPixelCnt);
	void UpdateRoiRect(RECT& rtRet, int row, int col, int& nPixelCnt);
	int MaskingChunkFromBuf(unsigned char* pBufSrc, unsigned char* pBufDest, int nRowStart, int nRowEnd, int nColStart, int nColEnd, int nThreshold, bool bTimeLogging = false);
	int FindSpecularCross(unsigned char* dest, int nRowFindStart, int nRowFindEnd, int nColFindStart, int nColFindEnd, int nBaseValue, std::vector<RECT>* pVecRoiSP, int nResultAdded, bool bCheckCond, bool bTimeLogging = false);
	bool CheckSpecularCond(RECT* pRt, bool bCondLogging = false);
	float CalculateDistance(unsigned char* src, RECT& rtROI, int nExcludeThreshold, bool bSaveDist, char* pszName, bool bTimeLogging = false);
	void CopyRoiValueToClipboard(unsigned char** copyValue, int row, int col, bool bTimeLogging = false);
	void CallEyeFindTest(unsigned char* src, char* pszName);
	void CallEyeFindTestFile(char* pszFilePath);
	/////////////////////////////////////////////////////////////////////////////
};

extern Controller gController;
static inline Controller &getController()
{
	return gController;
}

extern int gFindSpecularIdx;
extern std::vector<RECT> vecRoiSP;
