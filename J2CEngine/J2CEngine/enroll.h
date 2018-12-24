#pragma once

#include "bufferpoolmanager.h"
#include "multicorequeuetemplate.h"
#include <iostream>
#include "NeuroTask.h"


class EnrollTask {
public:
	EnrollTask() {
		stopFlag_ = false;
		jobThread_ = nullptr;
		timeout_ = 1000;	// 1sec
		seqId_ = 0;
	}
	~EnrollTask() {}

	void stopJob()
	{
		stopFlag_ = true;
	}

	void startJob()
	{
		stopFlag_ = false;
		jobThread_ = new std::thread(std::bind(&EnrollTask::__internalJob, this));
		jobThread_->detach();
	}

	void __internalJob()
	{
		FUNCTION_NAME_IS(_T("EnrollTask::__internalJob"));
		LOG_PUTS(0, _T("EnrollTask thread starts..."));

		EnrollSharedBuffer buf;
		while (stopFlag_ == false)
		{
			buf = getEnrollMultiCoreQueue().getSharedBufferWithTimeout(timeout_);
			if (buf.id == "")
				continue;

			std::cout << "EnrollTask get Enroll Item" << std::endl;
			//
			// cscho
			//
			//SharedBuffer resized = resizeForNeruoTechnology(buf.cropSharedBuffer, buf.x, buf.y);
			SharedBuffer resized = resizeForNeruoTechnologySize(buf.cropSharedBuffer, buf.x, buf.y, 800, 600);
			if (resized != nullptr)
			{
				NeuroTask &neuro = getNeuroTask();
				neuro.setCurrentEnrollName(buf.id);
				neuro.enrollStart();
				neuro.pushBufferToJobQueue(resized, NEURO_ENROLL_COMMAND);

					
#if NETWORK_ENROLL
				std::cout << "Send enroll image to neurotechnology server" << std::endl;
				saveToFile(resized, buf.id);
				int ret = getNetwork().sendEnrollRequest(resized, buf.id, std::bind(&EnrollTask::enrollDone, this, std::placeholders::_1));

				if (ret == 0)
					getController().enrollDone(true);
				else
					getController().enrollDone(false);
#endif
				resized = nullptr;
			}
			else
			{
				int a = 0;
			}
			getBufferPoolManager().deleteBufferWithName(CAMERA_CROP_BUFFER_POOL_ID, buf.cropSharedBuffer->getName());
			getBufferPoolManager().deleteBufferWithName(OPENCV_BUFFER_POOL_ID, buf.opencvSharedBuffer->getName());
			buf.cropSharedBuffer = nullptr;
			buf.opencvSharedBuffer = nullptr;
		}
	}

	void enrollDone(bool isSuccess)
	{
		std::cout << "Enroll done : " << isSuccess << std::endl;
	}

	void saveToFile(SharedBuffer &buf, std::string &id);
	SharedBuffer resizeForNeruoTechnology(SharedBuffer &buf, int x, int y);
	SharedBuffer resizeForNeruoTechnologySize(SharedBuffer &buf, int x, int y, int width, int height);

	bool stopFlag_;
	std::thread *jobThread_;
	int timeout_;
	int seqId_;
};

extern EnrollTask gEnrollTask;
static inline EnrollTask &getEnrollTask()
{
	return gEnrollTask;
}