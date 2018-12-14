#pragma once
#include <iostream>
#include <string>
#include "J2CEngine_dll.h"
#include <windows.h>
#include "NeuroTask.h"

#define DEVICE_ID_LEN 10
#define PARITAL_WIDTH	64
#define PARTIAL_HEIGHT	64

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
};

extern Controller gController;
static inline Controller &getController()
{
	return gController;
}
