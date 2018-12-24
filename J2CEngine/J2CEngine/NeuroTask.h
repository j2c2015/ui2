#ifndef NUEROTASK_H
#define NUEROTASK_H
#define N_FRAMEWORK_NATIVE

#include "bufferpoolmanager.h"
#include "multicorequeue.h"
#include "multicorequeuetemplate.h"
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <NCore.hpp>
#include <NBiometricClient.hpp>
#include <NBiometrics.hpp>
#include <NMedia.hpp>
#include <NLicensing.hpp>
#include "LogMgr.h"


using namespace Neurotec;
using namespace Neurotec::Licensing;
using namespace Neurotec::Biometrics;
using namespace Neurotec::Biometrics::Client;
using namespace Neurotec::IO;
using namespace Neurotec::Devices;
using namespace Neurotec::Images;

struct NeuroTaskResult {
	std::string desc;
	std::string result;
	NeuroTaskResult()
	{
		result = "";
		desc = "";
	}

	bool operator!=(NeuroTaskResult &rhs)
	{
		int i = this->desc.compare(rhs.desc);
		int j = this->result.compare(rhs.result);
		if (i || j)
			return true;
		return false;
	}

	bool operator==(NeuroTaskResult &rhs)
	{
		int i = this->desc.compare(rhs.desc);
		int j = this->result.compare(rhs.result);
		if (i || j)
			return false;
		return true;
	}

	bool isEmptyResult()
	{
		if (result == "")
			return true;
		return false;
	}
};

enum NeuroTaskState { IDLE, ENROLL_PREPARE, ENROLL_START, ENROLL_END, MATCH_START, MATCH_END, JPEG_START, JPEG_END, CLEAR_JOB };
class NeuroTask
{
public:
	NeuroTask();

	void setEnrollResultFn(std::function< void(bool)> fn)
	{
		enrollResultFn_ = fn;
	}

	void initLicense()
	{
		const NChar * components[] = { N_T("Biometrics.IrisExtraction"),
									   N_T("Biometrics.IrisSegmentation"),
									   N_T("Biometrics.IrisMatching"),
									   N_T("Biometrics.IrisExtraction"),
		};
		try {
			for (int i = 0; i < sizeof(components) / sizeof(NChar *); i++)
				if (!NLicense::ObtainComponents(N_T("/local"), N_T("5000"), components[i]))
				{
					NThrowException(NString::Format(N_T("Could not obtain licenses for components: {S}"), components));
				}
		}
		catch (NError &ex)
		{
			//qDebug() << "License Failed";
		}
	}


	void setWorkDir(std::string dir)
	{
		workdir_ = dir;
	}

	void setCurrentEnrollName(std::string name)
	{
		currentEnrollName_ = name;
	}

	void enrollStart()
	{
		currentState_ = ENROLL_START;
		enrollCnt_ = 0;
	}

	void enrollEnd()
	{
		currentState_ = ENROLL_END;
	}

	void matchStart()
	{
		currentState_ = MATCH_START;
	}
	void matchEnd()
	{
		currentState_ = MATCH_END;
	}

	bool isMatching()
	{
		if (currentState_ == MATCH_START)
			return true;
		return false;
	}

	bool isEnrolling()
	{
		if (currentState_ == ENROLL_START)
			return true;
		return false;
	}

	void clearJob()
	{
		//while( resultQueue_.size() > 0 )
		//    resultQueue_.pop();
		SharedBuffer buf = nullptr;
		NeuroSharedBuffer neuro;
		while ( true )
		{
			neuro = getNeuroMultiCoreQueue().getSharedBufferWithTimeout(0);
			if (neuro.buf == nullptr)
				break;
		}

		NeuroTaskResult res;
		NeuroTaskResult empty = NeuroTaskResult();
		buf = nullptr;
		while ((res = matchResultQueue_.getSharedBufferWithTimeout(0)) != empty)
			;
	}

	void pushBufferToJobQueue(SharedBuffer &buf,int command =-1)
	{
		NeuroSharedBuffer buffer;
		buffer.buf = buf;
		buffer.command = command;
		getNeuroMultiCoreQueue().putSharedBuffer(buffer);
		//jobQueue_.putSharedBuffer(buf);
	}

	void clearMatchResultQueue()
	{
		NeuroTaskResult res;
		NeuroTaskResult empty = NeuroTaskResult();
		while ((res = matchResultQueue_.getSharedBufferWithTimeout(0)) != empty)
			;
	}

	NeuroTaskResult  getMatchResult(int timeout)
	{
		NeuroTaskResult res;
		res = matchResultQueue_.getSharedBufferWithTimeout(timeout);
		return res;
	}

	void startJob()
	{
		if (bioClient_ == nullptr)
		{
			bioClient_ = new NBiometricClient();
		}
		//matchStart();
		worker_ = new std::thread(std::bind(&NeuroTask::__internalJob, this));
		worker_->detach();
	}

	void clearQueue()
	{
		NeuroSharedBuffer neuroShared;
		while (1)
		{
			neuroShared = getNeuroMultiCoreQueue().getSharedBufferWithTimeout(30);
			if (neuroShared.buf == nullptr && neuroShared.command < 0)
				break;
		}
	}

	void reloadEnroll()
	{
		//__enrollSaveAndReload();
		NeuroSharedBuffer neuroShared;
		neuroShared.command = NEURO_ENROLL_RELOAD_COMMAND;
		getNeuroMultiCoreQueue().putSharedBuffer(neuroShared);
	}

	// neurotechnology api process
	void __enroll(SharedBuffer &buf);
	bool __match(SharedBuffer &buf);
	void __jpeg(SharedBuffer &buf);
	void __enrollSaveAndReload();
	std::string extract(std::string &id);
	void __internalJob()
	{
		FUNCTION_NAME_IS(_T("NeuroTask::__internalJob"));
		LOG_PUTS(0, _T("NeuroTask thread starts..."));

		NeuroSharedBuffer neuroShared ;
		SharedBuffer buf = nullptr;
		int command = -1;
		bool bRet = false;
		while (stopFlag_ == false)
		{
			//buf = jobQueue_.getSharedBufferWithTimeout(30); // 30msec
			//if (buf == nullptr)
			//	continue;

			neuroShared = getNeuroMultiCoreQueue().getSharedBufferWithTimeout(30);
			if ( neuroShared.buf  == nullptr && neuroShared.command < 0  )
				continue;

			buf = neuroShared.buf;
			neuroShared.buf = nullptr;
			command = neuroShared.command;
			if (command >= 0)
			{
				/*
				if (command == ENROLL_END)
				{
					__enrollSaveAndReload();
				}
				if (command == NEURO_ENROLL_COMMAND)
				{
					__enroll(buf);
				}
				if (command == NEURO_IDENTIFY_COMMAND)
				{
					bool bRet = false;
					bRet = __match(buf);
				}
				*/
				switch (command)
				{
				case NEURO_ENROLL_COMMAND:
					__enroll(buf);
					break;
				case NEURO_IDENTIFY_COMMAND:
					{
						bool bRet = false;
						bRet = __match(buf);
					}
					break;
				case NEURO_ENROLL_RELOAD_COMMAND:
					__enrollSaveAndReload();
					break;
				}
			}
			else
			{
				switch (currentState_)
				{
				case IDLE:
					// no work to do
					buf = nullptr;  // drop the buffer
					break;
				case ENROLL_START:
					__enroll(buf);
					break;

				case ENROLL_END:
					__enrollSaveAndReload();
					currentState_ = IDLE;
					enrolled_ = true;
					break;

				case MATCH_START:
					if (enrolled_ == false)
					{
						std::cout << "#### Enroll must be done ####" << std::endl;
						__enrollSaveAndReload();
						enrolled_ = true;
					}
					bRet = __match(buf);
					if (bRet == true) {
						std::cout << "@@@@ match found @@@@" << std::endl;
						currentState_ = IDLE;
						//getController().identifyBlock();
					}
					break;

				case MATCH_END:

					break;

				case JPEG_START:
					__jpeg(buf);
					break;

				case JPEG_END:

					break;

				default:
					buf = nullptr;
					break;
				}
			}

			buf = nullptr;
		} // end of while
	}

	//std::queue<NeuroTaskResult> resultQueue_;
	NeuroTaskState currentState_;
	//MultiCoreQueue  jobQueue_;
	
	MultiCoreQueueTemplate<NeuroTaskResult> matchResultQueue_;
	std::thread *worker_;
	bool stopFlag_;
	std::string argument_;
	NBiometricClient *bioClient_;
	std::string workdir_;
	std::string currentEnrollName_;
	int enrollCnt_;
	std::function< void(bool) > enrollResultFn_;
	bool enrolled_;
};

extern NeuroTask gNeuroTask;
static inline NeuroTask &getNeuroTask()
{
	return gNeuroTask;
}
#endif // NUEROTASK_H
