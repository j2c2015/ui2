#pragma once

#include "multicorequeue.h"
#include "bufferpoolmanager.h"
#include "colorbufferinfo.h"
#include "controller.h"
#include "multicorequeuetemplate.h"
#include <thread>
#include <queue>
#include "opencv/cv.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "imagesaver.h"
#include "identify.h"


#define EYE_CANDIDATE_MAX_NUM	1
#define EYE_CANDIDATE_MAX_INTERVAL (500)	// 500msec 
#define EYE_CANDIDATE_TIMEOUT	(2*1000)	// 3sec
class EyeSelector
{
public:
	EyeSelector();

	void startJob()
	{
		stopFlag_ = false;
		jobThread_ = new std::thread(std::bind(&EyeSelector::__internalJob, this));
		jobThread_->detach();
	}


	bool isStableEyeDetected()
	{
		std::queue< SharedBuffer > latestDetectedTmp_;
		int currentTick = GetTickCount();
		int min = currentTick - EYE_CANDIDATE_MAX_INTERVAL;
		SharedBuffer buf = nullptr;
		bool expired = false;
		while (latestDetected_.size() > 0)
		{
			buf = latestDetected_.front();
			latestDetected_.pop();
			if (buf->getTick() < min)
				expired = true;
			latestDetectedTmp_.push(buf);
		}

		while (latestDetectedTmp_.size() > 0)
		{
			buf = latestDetectedTmp_.front();
			latestDetectedTmp_.pop();
			latestDetected_.push(buf);
		}

		return !expired;
	}

	void cleanOldDetectedEye()
	{
		int currentTick = GetTickCount();
		int min = currentTick - EYE_CANDIDATE_TIMEOUT;
		SharedBuffer buf = nullptr;
		std::queue< SharedBuffer > latestDetectedTmp_;

		while (latestDetected_.size() > 0)
		{
			buf = latestDetected_.front();
			latestDetected_.pop();
			if (buf->getTick() < min)
				latestDetectedTmp_.push(buf);
		}

		while (latestDetectedTmp_.size() > 0)
		{
			buf = latestDetectedTmp_.front();
			latestDetectedTmp_.pop();
			latestDetected_.push(buf);
		}
	}

	void clearAllDetectedEye()
	{
		while (latestDetected_.size() > 0)
		{
			latestDetected_.pop();
		}
	}

	void stopJob()
	{
		stopFlag_ = true;
	}

	void setTimeout(int timeout)
	{
		timeout_ = timeout;
	}

	bool findContour(SharedBuffer &buf, int &x, int &y, int centerX, int centerY);

	void __internalJob()
	{
		FUNCTION_NAME_IS(_T("EyeSelector::__internalJob"));
		LOG_PUTS(0, _T("EyeSelector thread starts..."));

		int x, y;
		while (stopFlag_ == false)
		{
			//SharedBuffer buf = getMultiCoreQueueForEyeSelector().getSharedBufferWithTimeout(timeout_);
			//if (buf == nullptr)
			//	continue;

			EyeSelectSharedBuffer argBuffer = getEyeSelectMultiCoreQueue().getSharedBufferWithTimeout(timeout_);
			if (argBuffer.opencvSharedBuffer == nullptr)
				continue;

			// buf  opencv frame buffer
			SharedBuffer buf = argBuffer.opencvSharedBuffer;
			argBuffer.opencvSharedBuffer = nullptr;

			if ((getController().isEnrollUsage() == false) && (getController().isIdentifyUsage() == false))
			{
				getBufferPoolManager().deleteBufferWithName(CAMERA_CROP_BUFFER_POOL_ID, buf->getName());
				getBufferPoolManager().deleteBufferWithName(OPENCV_BUFFER_POOL_ID, buf->getName());
				buf = nullptr;
				continue;
			}

#if 0
			bool enrollBlocked = false;
			bool identifyBlocked = false;

			//eye_cascade_.detectMultiScale(image, *eyes, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, cv::Size(50, 50));
			if (getController().isEnrollUsage() && (getController().isEnrollBlocked() == false)) {
				getController().enrollBlock();
				enrollBlocked = true;
			}

			if (getController().isIdentifyUsage() && (getController().isIdentifyBlocked() == false))
			{
				getController().identifyBlock();
				identifyBlocked = true;
			}
#endif
			bool bRet = findContour(buf, x, y, argBuffer.x, argBuffer.y);

#if 0
			if (enrollBlocked == true)
				getController().enrollUnBlock();
			if (identifyBlocked == true)
				getController().identifyUnBlock();
#endif
			//if (::fabsl(x - OPENCV_FRAME_CENTER_X) < 10 && ::fabsl(y - OPENCV_FRAME_CENTER_Y) < 10)
			//bRet = true;
			x = argBuffer.x;
			y = argBuffer.y;
			bRet = true;
			if (bRet == true)
			{
				// center aligned specular found
				//std::cout << "********** contour fond ***********" << std::endl;
				while (latestDetected_.size() > EYE_CANDIDATE_MAX_NUM - 1)
				{
					latestDetected_.pop();
				}
				latestDetected_.push(buf);

				if (latestDetected_.size() >= EYE_CANDIDATE_MAX_NUM) {

					//if (isStableEyeDetected() == true)
					{
						std::string name = buf->getName();
						std::cout << "------------ Eye detected confirmed ------------- " << name << std::endl;
						// use latest eye detected buffer
						// get crop frame image matching name got by above statement;
						//std::cout << "Buffer name " << name << std::endl;
						SharedBuffer cropFrame = getBufferPoolManager().getSavedBufferWithName(CAMERA_CROP_BUFFER_POOL_ID, (char *)buf->getName());
						//getMultiCoreQueueForKind7().putSharedBuffer(cropFrame);
						if (cropFrame != nullptr) {
							if (getController().isEnrollUsage() /*&& (getController().isEnrollBlocked() == false)*/ ) {
								std::cout << "Sent to enroll task " << std::endl;
								getController().enrollBlock();
								getNeuroTask().clearQueue();
								std::string id = getController().getEnrollId();
								saveToFile(buf, id);
								EnrollSharedBuffer enroll;
								enroll.id = id;
								enroll.opencvSharedBuffer = buf;
								enroll.cropSharedBuffer = cropFrame;
								enroll.x = x;
								enroll.y = y;
								getEnrollMultiCoreQueue().putSharedBuffer(enroll);
								//cropFrame = nullptr;
								//getController().blockCameraInput(5000); // 5 sec camera input inhibit
								clearAllDetectedEye();
								if (saveFlag_ == true)
								{
									std::string path = savePath_ + "\\" + "to_enroll_" + buf->getName() + ".png";
									getImageSaver().saveGrayImage(cropFrame, path, WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP);
								}
								J2CRenderCb render = getController().getEnrollRenderCb();
								render(buf->getBuffer(), WIDTH_FOR_OPENCV, HEIGHT_FOR_OPENCV, COLOR_DEPTH_FOR_OPENCV);
								enroll.opencvSharedBuffer = nullptr;
								enroll.cropSharedBuffer = nullptr;

							}
							else if (getController().isIdentifyUsage() /*&&  (getController().isIdentifyBlocked()) == false*/)
							{
								std::cout << "Sent to identify task" << std::endl;
								getController().identifyBlock();
								getNeuroTask().clearQueue();
								EnrollSharedBuffer identify;
								identify.id = "identify";
								identify.opencvSharedBuffer = buf;
								identify.cropSharedBuffer = cropFrame;
								identify.x = x;
								identify.y = y;
								std::cout << "x,y " << x << " " << y << " " << "Arg x,y " << argBuffer.x << " " << argBuffer.y << std::endl;
								getIdentifyMultiCoreQueue().putSharedBuffer(identify);

								J2CRenderCb render = getController().getIdentifyRenderCb();
								render(buf->getBuffer(), WIDTH_FOR_OPENCV, HEIGHT_FOR_OPENCV, COLOR_DEPTH_FOR_OPENCV);
								identify.opencvSharedBuffer = nullptr;
								identify.cropSharedBuffer = nullptr;
							}
							else
							{
							if (getController().isIdentifyUsage())
									std::cout << "Current mode identify usage" << std::endl;
								std::cout << "Eye-Selector received frame but no usage mode" << std::endl;
							}
							getBufferPoolManager().deleteBufferWithName(CAMERA_CROP_BUFFER_POOL_ID, cropFrame->getName());
							cropFrame = nullptr;
						}
						else
						{
							std::cout << "No crop frame" << std::endl;
						}
					}
					getBufferPoolManager().deleteBufferWithName(OPENCV_BUFFER_POOL_ID, buf->getName());
				}
				else
				{
					getBufferPoolManager().deleteBufferWithName(CAMERA_CROP_BUFFER_POOL_ID, buf->getName());
					getBufferPoolManager().deleteBufferWithName(OPENCV_BUFFER_POOL_ID, buf->getName());
				}

			}
			else
			{
				// contour not found
				std::cout << "Contour not found " << std::endl;
				getBufferPoolManager().deleteBufferWithName(CAMERA_CROP_BUFFER_POOL_ID, buf->getName());
				getBufferPoolManager().deleteBufferWithName(OPENCV_BUFFER_POOL_ID, buf->getName());
			}
			//cleanOldDetectedEye();
			buf = nullptr;
		}

	}

	void save(bool save, char *path)
	{
		saveFlag_ = save;
		savePath_ = path;
	}

	void saveToFile(SharedBuffer &buf, std::string &id);

	bool stopFlag_;
	std::thread *jobThread_;
	int timeout_;
	std::queue< SharedBuffer > latestDetected_;
	int seqId_;
	bool saveFlag_ = false;
	std::string savePath_ = "";

};

extern EyeSelector	gEyeSelector;

static inline EyeSelector &getEyeSelector()
{
	return gEyeSelector;
}