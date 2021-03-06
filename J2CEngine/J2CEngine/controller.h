#pragma once
#include <iostream>
#include <string>
#include "J2CEngine_dll.h"
#include <windows.h>
#include "NeuroTask.h"
#include <opencv/cv.h>

#define DEVICE_ID_LEN				(10)
#define PARITAL_WIDTH				(64)
#define PARTIAL_HEIGHT				(64)

#define INDICATE_LED_NOBLINKING		(unsigned int)(0xFFFFFFFF)
#define INDICATE_LED_ON				(unsigned int)(0x00000001)
#define INDICATE_LED_OFF			(unsigned int)(0x00000000)
#define CY_FX_PWM_PERIOD            (201600 - 1)  /* PWM time period. */
#define CY_FX_PWM_HOLD_A0			(130446)  /* A0% of PWM time period. */

#define GPIO_VSYNC					(17)
#define GPIO_B                      (18)
#define GPIO_G                      (19)
#define GPIO_R                      (20)
#define GPIO_850                    (21)
#define GPIO_730                    (22)

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

typedef struct _roi_result
{
	TCHAR szName[64];
	RECT roi;
	float fMaxDeviation;
	SharedBuffer enrollSharedBuffer;
	_roi_result()
	{
		memset(szName, 0x00, sizeof(szName));
		roi.left = roi.right = roi.top = roi.bottom = 0;
		fMaxDeviation = 0.0f;
		enrollSharedBuffer = nullptr;
	}
} ROI_RESULT, *PROI_RESULT;

typedef enum _led_state
{
	eLED_NONE,
	eLED_BLINKING,
	eLED_RED,
	eLED_BLUE,
	eLED_GREEN,
	eLED_YELLOW,
} eLED_STATE;

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
		m_eLEDBState = eLED_NONE;
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
	BOOL testCamera();
	BOOL CheckIrisDevOpen();

	void allIndicationLedOff();
	void redColorLedOn();
	void blueColorLedOn();
	void greenColorLedOn();
	void yellowColorLedOn();
	void blinkingRedColorLedOn(int value);	
	void setSimpleGPIO(BYTE port, BOOL value);
	bool getSimpleGPIO(BYTE port, BOOL* pbValue);
	void setI2C(BYTE slaveAddress, BOOL regAddrHigh_true, UINT RegAddr, UINT dataLength, char* data);
	bool getI2C(BYTE slaveAddress, BOOL regAddrHigh_true, UINT RegAddr, UINT dataLength, char* pbValue);

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
		{
			int a = 0;
			return;
		}

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
	eLED_STATE m_eLEDBState;
	eLED_STATE GetLEDState() { return m_eLEDBState; }
	void SetLEDState(eLED_STATE eLEDState) { m_eLEDBState = eLEDState; }
	bool m_bEyeFindPixelPctCheck;
	bool GetEyeFindPixelPctCheck() { return m_bEyeFindPixelPctCheck; }
	int m_nEyeFindPixelPctMin;
	int GetEyeFindPixelPctMin() { return m_nEyeFindPixelPctMin; }
	int m_nEyeFindPixelPctMax;
	int GetEyeFindPixelPctMax() { return m_nEyeFindPixelPctMax; }
	int m_nEyeFindDevValueRef;
	int GetEyeFindDevValueRef() { return m_nEyeFindDevValueRef; }
	int m_nEyeFindDevValueGroupCnt;
	int GetEyeFindDevValueGroupCnt() { return m_nEyeFindDevValueGroupCnt; }
	int m_nEyeFindResultAdded;
	int GetEyeFindScanResultAdded() { return m_nEyeFindResultAdded; }
	int m_nEyeFindScanBaseValue;
	int GetEyeFindScanBaseValue() { return m_nEyeFindScanBaseValue; }
	int m_nEyeFindLineMin;
	int m_nEyeFindLineMax;	
	int GetEyeFindScanLineMin() { return m_nEyeFindLineMin; }
	int GetEyeFindScanLineMax() { return m_nEyeFindLineMax; }
	int m_nEyeFindLineGroupMin;
	int m_nEyeFindLineGroupMax;
	int GetEyeFindScanLineGroupMin() { return m_nEyeFindLineGroupMin; }
	int GetEyeFindScanLineGroupMax() { return m_nEyeFindLineGroupMax; }
	int m_nEyeFindRatioMin;
	int GetEyeFindScanRatioMin() { return m_nEyeFindRatioMin; }
	bool m_bEyeFindUseOpenCV;
	bool GetEyeFindUseOpenCV() { return m_bEyeFindUseOpenCV; }
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
	bool m_bEyeFindViewSPAll;
	bool GetEyeFindScanViewSPAll() { return m_bEyeFindViewSPAll; }
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
	int MaskingChunkFromBuf(unsigned char* pBufSrc, unsigned char* pBufDest, int nRowStart, int nRowEnd, int nColStart, int nColEnd, int nThreshold, int nPixelContrast, int& nCntAll, int& nCntUpSide, int& nCntDnSide, bool bTimeLogging = false);
	int FindSpecularCross(unsigned char* dest, char* pszName, int nRowFindStart, int nRowFindEnd, int nColFindStart, int nColFindEnd, int nBaseValue, std::vector<RECT>* pVecRoiSP, std::vector<RECT>* pVecRoiSPNot, int nResultAdded, bool bCheckCond, FILE* fpLog, bool bTimeLogging = false);
	bool CheckSpecularCond(RECT* pRt, TCHAR* pszName, FILE* fpLog, bool bCondLogging = false);
	float CalculateDistance(unsigned char* src, RECT& rtROI, int nExcludeThreshold, bool bSaveDist, char* pszName, int nBaseValue, int nPixelContrast, int& nCntSPAll, int& nCntSPUp, int& nCntSPDn, bool bTimeLogging = false);
	float CalculateDistanceAll(unsigned char* src, std::vector<RECT>* pVecRoi, int nExcludeThreshold, bool bSaveDist, char* pszName, int nBaseValue, int nPixelContrast, int& nCntSPAll, int& nCntSPUp, int& nCntSPDn, RECT& rtRoiRet, bool bTimeLogging = false);
	void CopyRoiValueToClipboard(unsigned char** copyValue, int row, int col, bool bTimeLogging = false);
	float CallEyeFindTest(unsigned char* src, char* pszName, int& nCntSPAll, int& nCntSPUp, int& nCntSPDn, ROI_RESULT* pRoiResult, FILE* fpLog = NULL);
	void CallEyeFindTestFile(char* pszFilePath);
	/////////////////////////////////////////////////////////////////////////////
};

extern Controller gController;
static inline Controller &getController()
{
	return gController;
}

static inline bool sortByRoiDeviation(const ROI_RESULT& lhs, const ROI_RESULT& rhs)
{
	return lhs.fMaxDeviation < rhs.fMaxDeviation;
}

extern int gFindSpecularIdx;
extern std::vector<RECT> vecRoiSP;
extern std::vector<RECT> vecRoiSPNot;
extern int gEnrollIdx;
extern std::vector<ROI_RESULT> vecRoiSPDeviation;
extern std::vector<ROI_RESULT> vecRoiSPCandidate;
extern MultiCoreQueue gMultiCoreQueueForRoiSP;
