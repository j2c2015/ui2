#pragma once
#pragma once

#include "multicorequeue.h"
#include <opencv/cv.h>
#include "colorbufferinfo.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include "controller.h"
#include <stdint.h>
#include "colorbufferinfo.h"


class Crop
{
public:
	Crop();
	
	void startJob()
	{
		stopFlag_ = false;
		jobThread_ = new std::thread(std::bind(&Crop::__internalJob, this));
	}

	// thread entry function
	void __internalJob()
	{
		FUNCTION_NAME_IS(_T("Crop::__internalJob"));
		LOG_PUTS(0, _T("Crop thread starts..."));

		++index_;
		SharedBuffer camera = nullptr;
		while (stopFlag_ == false)
		{
			camera = getMultiCoreQueueForCameraInput().getSharedBufferWithTimeout(timeout_);
			if (camera == nullptr)
				continue;

			SharedBuffer buf = getBufferPoolManager().getBuffer(CAMERA_CROP_BUFFER_POOL_ID, camera->getName());
			if (buf == nullptr) {
				camera = nullptr;
				continue;
			}

			char *dst = (char *)buf->getBuffer();
			char *start = (char *)camera->getBuffer();
			start += (WIDTH_FOR_CAMERA_FRAME * ROI_Y);
			start += ROI_X;

			for (int i = 0; i < HEIGHT_FOR_CAMERA_CROP; i++)
			{
				memcpy(dst, start, WIDTH_FOR_CAMERA_CROP);
				dst += WIDTH_FOR_CAMERA_CROP;
				start += WIDTH_FOR_CAMERA_FRAME;
			}
			//saveCropImage(buf);
			getMultiCoreQueueForScaler().putSharedBuffer(buf);

			if (getController().isEnrollUsage() != true && getController().isIdentifyUsage() != true)
			{
				getBufferPoolManager().deleteBufferWithName(CAMERA_FRAME_BUFFER_POOL_ID, camera->getName());
				getBufferPoolManager().deleteBufferWithName(CAMERA_CROP_BUFFER_POOL_ID, buf->getName());
				continue;
			}

#if 0
			if ((getController().isIdentifyUsage() && getController().isIdentifyBlocked()) ||
				(getController().isEnrollUsage() && getController().isEnrollBlocked()))
			{
				getBufferPoolManager().deleteBufferWithName(CAMERA_FRAME_BUFFER_POOL_ID, camera->getName());
				getBufferPoolManager().deleteBufferWithName(CAMERA_CROP_BUFFER_POOL_ID, buf->getName());
				camera = nullptr;
				buf = nullptr;
				continue;
			}
#endif

			SharedBuffer resized = getBufferPoolManager().getBuffer(OPENCV_BUFFER_POOL_ID, (char *)camera->getName());
			if (resized != nullptr) 
			{
				//cv::Mat dst = cv::Mat(cvSize(WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP), CV_8UC1 , resized->getBuffer(), cv::Mat::AUTO_STEP);
				cv::Mat src = cv::Mat(cvSize(WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP), CV_8UC1, buf->getBuffer(), cv::Mat::AUTO_STEP);
				cv::Mat dst;
				cv::resize(src, dst, cv::Size(WIDTH_FOR_OPENCV, HEIGHT_FOR_OPENCV));
				char buffer[256];

				memcpy(resized->getBuffer(), dst.data, dst.total());
				int out = (int) dst.total();
				sprintf(buffer, "c:\\jtwoc\\resize\\resize%d.img", index_);
				resized->setAvailable(FRAMESIZE_FOR_OPENCV);
				//resized->saveToFile(buffer);
				//saveOpenCVImage(resized);
				getMultiCoreQueueForOpenCV().putSharedBuffer(resized);
				dst.release();
				resized = nullptr;
			}
			else
			{
				std::cout << "############   opencv frame get failed ############" << std::endl;
				getBufferPoolManager().deleteBufferWithName(CAMERA_CROP_BUFFER_POOL_ID, buf->getName());
			}

			getBufferPoolManager().deleteBufferWithName(CAMERA_FRAME_BUFFER_POOL_ID, camera->getName());
			buf = nullptr;
			camera = nullptr;
			index_++;
		}
	}

	void stop()
	{
		stopFlag_ = true;
	}

	void setTimeout(int ms)
	{
		timeout_ = ms;
	}

	int getTimeout()
	{
		return timeout_;
	}

	void saveCropImage(SharedBuffer &buf);
	void saveOpenCVImage(SharedBuffer &buf);

	bool stopFlag_;
	int timeout_;
	std::thread *jobThread_;
	int index_;
	long seqId_;
};

extern Crop gCrop;
static inline Crop &getCrop()
{
	return gCrop;
}

