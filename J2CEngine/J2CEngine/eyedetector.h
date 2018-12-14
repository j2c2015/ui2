#ifndef EYEDETECT_H
#define EYEDETECT_H

#include "multicorequeue.h"
#include "multicorequeuetemplate.h"
#include <opencv/cv.h>
#include "colorbufferinfo.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include "controller.h"
#include <stdint.h>
#include <iostream>
#include "imagesaver.h"

typedef struct _EyePosition {
	int x;
	int y;
	int width;
	int height;

} EyePosition;

class EyeDetector
{
public:
	EyeDetector();

	void openCVEyeDetect(SharedBuffer &buf, std::vector<EyePosition> &positions);

	// thread entry function
	void operator()()
	{
		FUNCTION_NAME_IS(_T("EyeDetector::operator()"));
		LOG_PUTS(0, _T("EyeDetector thread starts..."));

		SharedBuffer buf = nullptr;
		while (stopFlag_ == false)
		{
			buf = getMultiCoreQueueForOpenCV().getSharedBufferWithTimeout(timeout_);
			if (buf == nullptr)
				continue;

			std::cout << "OpenCV queue size " << getMultiCoreQueueForOpenCV().getSize() << std::endl;

			eyePositions_.clear();
			openCVEyeDetect(buf, eyePositions_);

			if (eyePositions_.size() > 0)
			{
				// eye detected. send this frame to eye selector
				std::cout << "Eye detected !!!" << std::endl;
				int centerX, centerY;
				bool pass = isCenteredEye(eyePositions_[0], centerX, centerY);
				if (pass == true)
				{

					std::cout << "Center aligned eye detected. send it to eye selector " << centerX << " " << centerY << std::endl;
					EyeSelectSharedBuffer argBuffer = { centerX, centerY , buf };
					//getMultiCoreQueueForEyeSelector().putSharedBuffer(buf);
					//std::cout << ">>> sent to eye selector " << argBuffer.opencvSharedBuffer->getName() << std::endl ;
					getEyeSelectMultiCoreQueue().putSharedBuffer(argBuffer);
					if (saveFlag_ == true)
					{
						std::string path = savePath_ + "\\" + "detect" + buf->getName() + ".png";
						getImageSaver().saveGrayImage(buf, path, WIDTH_FOR_OPENCV, HEIGHT_FOR_OPENCV);
					}
				}
				else
				{
					// no eye detected
					getBufferPoolManager().deleteBufferWithName(CAMERA_CROP_BUFFER_POOL_ID, buf->getName());
					getBufferPoolManager().deleteBufferWithName(OPENCV_BUFFER_POOL_ID, buf->getName());
				}
			}
			else
			{
				std::cout << "Eye not detected" << std::endl;
				// no eye detected
				getBufferPoolManager().deleteBufferWithName(CAMERA_CROP_BUFFER_POOL_ID, buf->getName());
				getBufferPoolManager().deleteBufferWithName(OPENCV_BUFFER_POOL_ID, buf->getName());
			}


			buf = nullptr;
		}
	}

	// always return true
	bool isCenteredEye(EyePosition &pos, int &centerX, int &centerY)
	{
		centerX = pos.x + pos.width / 2;
		centerY = pos.y + pos.height / 2;

		//std::cout << "CenterX  : " << centerX << " " << "Diff " << fabs(centerX - OPENCV_FRAME_CENTER_X) << std::endl;
		//std::cout << "CenterY  : " << centerY << " " << "Diff " << fabs(centerY - OPENCV_FRAME_CENTER_Y) << std::endl;
#if 1
		if (fabs(centerX - OPENCV_FRAME_CENTER_X) > (WIDTH_FOR_OPENCV *0.1))
			return false;

		if (fabs(centerY - OPENCV_FRAME_CENTER_Y) > (HEIGHT_FOR_OPENCV *0.1))
			return false;
#endif
		return true;
	}

	SharedBuffer getEyeCenterFrame(SharedBuffer eye, int centerX, int centerY)
	{
		SharedBuffer center = getBufferPoolManager().getBuffer(EYE_VIEW_BUFFER_POOL_ID);
		if (center == nullptr)
		{
			// qDebug() << "### eye center frame get failed ###";
			return nullptr;
		}

		float xRatio = (float)centerX / (float)WIDTH_FOR_OPENCV;
		float yRatio = (float)centerY / (float)HEIGHT_FOR_OPENCV;
		char * p = (char *)eye->getBuffer();
		//qDebug() << "Ratio x,y " << xRatio << " " << yRatio ;
		int cx = (int)(WIDTH_FOR_CAMERA_FRAME * xRatio) - (WIDTH_FOR_EYE_VIEW / 2);
		if (cx < 0)
			cx = 0;

		int cy = (int)(HEIGHT_FOR_CAMERA_FRAME * yRatio) - (HEIGHT_FOR_EYE_VIEW / 2);
		if (cy < 0)
			cy = 0;

		//qDebug() << "Center x,y " << cx << " " << cy << "Origin " << (int)( WIDTH_FOR_CAMERA_FRAME * xRatio ) << " " << (int)( HEIGHT_FOR_CAMERA_FRAME * yRatio ) ;
		char *dst = (char *)center->getBuffer();

		p += (cy * WIDTH_FOR_CAMERA_FRAME);
		p += (cx);

		for (int i = 0; i < HEIGHT_FOR_EYE_VIEW; i++)
		{
			memcpy(dst, p, WIDTH_FOR_EYE_VIEW);
			dst += WIDTH_FOR_EYE_VIEW;
			p += WIDTH_FOR_CAMERA_FRAME;
		}

		//SharedBuffer hist = getEqulizeHist(center);
		//center = nullptr;
		//return hist;
		return center;
	}

	SharedBuffer getEqulizeHist(SharedBuffer input)
	{
		cv::Mat image = cv::Mat(cvSize(WIDTH_FOR_EYE_VIEW, HEIGHT_FOR_EYE_VIEW), CV_8UC1, input->getBuffer(), cv::Mat::AUTO_STEP);
		SharedBuffer hist = getBufferPoolManager().getBuffer(EYE_VIEW_BUFFER_POOL_ID);
		if (hist == nullptr)
		{
			//qDebug() << "getEqualizeHist failed (getBuffer)" ;
			return input;
		}
		cv::Mat hist_image = cv::Mat(cvSize(WIDTH_FOR_EYE_VIEW, HEIGHT_FOR_EYE_VIEW), CV_8UC1, hist->getBuffer(), cv::Mat::AUTO_STEP);
		IplImage i = image;
		IplImage h = hist_image;
		cvEqualizeHist(&i, &h);

		return hist;
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

	void save(bool saveFlag, char *path)
	{
		saveFlag_ = saveFlag;
		savePath_ = path;
	}

	bool stopFlag_;
	int timeout_;
	cv::CascadeClassifier face_cascade_;
	cv::CascadeClassifier eye_cascade_;
	std::vector<EyePosition> eyePositions_;
	bool saveFlag_=false;
	std::string savePath_ = "";
};

extern EyeDetector *gEyeDetector;
static inline EyeDetector *getEyeDetectorPtr()
{
	return gEyeDetector;
}

static inline void setEyeDetectorPtr(EyeDetector *eye)
{
	gEyeDetector = eye;
}
#endif // EYEDETECT_H
