   
                                                                                                                                                                                                                                                    #pragma once

#include "bufferpoolmanager.h"
#include "multicorequeuetemplate.h"
#include <iostream>
#include "colorbufferinfo.h"
#include "NeuroTask.h"


class IdentifyTask {
public:
	IdentifyTask() {
		stopFlag_ = false;
		jobThread_ = nullptr;
		timeout_ = 100;	// 100msec
		seqId_ = 0;
	}
	~IdentifyTask() {}

	void stopJob()
	{
		stopFlag_ = true;
	}

	void startJob()
	{
		stopFlag_ = false;
		jobThread_ = new std::thread(std::bind(&IdentifyTask::__internalJob, this));
		jobThread_->detach();
	}

	void __internalJob()
	{
		FUNCTION_NAME_IS(_T("IdentifyTask::__internalJob"));
		LOG_PUTS(0, _T("IdentifyTask thread starts..."));

		EnrollSharedBuffer buf;
		//SharedBuffer buf;
		while (stopFlag_ == false)
		{
#if 0
			buf = getMultiCoreQueueForIdentify().getSharedBufferWithTimeout(timeout_);
			if (buf == nullptr)
				continue;
#endif
			buf = getIdentifyMultiCoreQueue().getSharedBufferWithTimeout(timeout_);
			if (buf.id == "")
				continue;

			std::cout << "******  IdentifyTask get Identify candidate frame  *******" << std::endl;
			SharedBuffer resized = resizeForNeruoTechnology(buf.cropSharedBuffer , buf.x, buf.y);
			if (resized != nullptr)
			{
				NeuroTask &neuro = getNeuroTask();
				neuro.matchStart();
				neuro.pushBufferToJobQueue(resized, NEURO_IDENTIFY_COMMAND);

#if NETWORK_IDENTIFY
				std::cout << "Send enroll image to neurotechnology server" << std::endl;
				saveToFile(resized, buf.id);
				int ret = getNetwork().sendIdentifyRequest(resized, buf.id, std::bind(&EnrollTask::enrollDone, this, std::placeholders::_1));

				if (ret == 0)
					getController().identifyDone(true);
				else
					getController().identifyDone(false);
#endif
				resized = nullptr;
			}
			getBufferPoolManager().deleteBufferWithName(CAMERA_CROP_BUFFER_POOL_ID, buf.cropSharedBuffer->getName());
			getBufferPoolManager().deleteBufferWithName(OPENCV_BUFFER_POOL_ID, buf.opencvSharedBuffer->getName());
			buf.cropSharedBuffer= nullptr;
			buf.opencvSharedBuffer = nullptr;
		}
	}

	void enrollDone(bool isSuccess)
	{
		std::cout << "Enroll done : " << isSuccess << std::endl;
	}

	void saveToFile(SharedBuffer &buf, std::string &id);
	SharedBuffer resizeForNeruoTechnology(SharedBuffer &buf, int x, int y);

	bool stopFlag_;
	std::thread *jobThread_;
	int timeout_;
	int seqId_;
};

extern IdentifyTask gIdentifyTask;
static inline IdentifyTask &getIdentifyTask()
{
	return gIdentifyTask;
}