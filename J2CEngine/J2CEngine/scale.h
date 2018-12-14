#pragma once

#include "stdafx.h"
#include "multicorequeue.h"
#include <opencv/cv.h>
#include "colorbufferinfo.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include "controller.h"
#include <stdint.h>
#include "imagesaver.h"
//#include "J2CIrisDemoDlg.h"
using namespace cv;


class Scaler
{
public:
	Scaler();

	void startJob()
	{
		stopFlag_ = false;
		jobThread_ = new std::thread(std::bind(&Scaler::__internalJob, this));
	}

	// thread entry function
	void __internalJob()
	{
		FUNCTION_NAME_IS(_T("Scaler::__internalJob"));
		LOG_PUTS(0, _T("Scaler thread starts..."));

		SharedBuffer buf = nullptr;
		while (stopFlag_ == false)
		{
			buf = getMultiCoreQueueForScaler().getSharedBufferWithTimeout(timeout_);
			if (buf == nullptr)
				continue;

			SharedBuffer forDisplay = getBufferPoolManager().getBuffer(CAMERA_CROP_BUFFER_POOL_ID);
			if (forDisplay == nullptr)
			{
				buf = nullptr;
				continue;
			}
			memcpy(forDisplay->getBuffer(), buf->getBuffer(), FRAMESIZE_FOR_CAMERA_CROP);
			//drawCircle(forDisplay);

			//g_pUI->Render((unsigned char *)buf->getBuffer());
			J2CRenderCb cb = getController().getRealTimeRenderCb();
			cb(forDisplay->getBuffer(), WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP, COLOR_DEPTH_FOR_CAMERA_CROP);

			int x, y;
			if (getController().isPartialRenderRequested(x, y) == true)
			{
				// save 800*600
				{
					char path[256] = "";
					sprintf(path, "c:\\jtwoc\\crop\\crop_%s.png", buf->getName());
					char szFName[256] = "";
					sprintf(szFName, "crop_%s.png", buf->getName());

					std::string strPath = path;					
					getImageSaver().saveGrayImage(buf, strPath, WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP);

					FUNCTION_NAME_IS(_T("Scaler::__internalJob"));
					LOG_PRINTF(0, _T("Save crop-image file.[%d * %d] (Name= %s)"), WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP, szFName);
				}

				//std::cout << "Partial Data Extracting " << x << " " << y << std::endl;
				FUNCTION_NAME_IS(_T("Scaler::__internalJob"));
				LOG_PRINTF(0, _T("Partial Data Extracting...(X= %d, Y= %d)"), x, y);

				char *start = (char *)buf->getBuffer();
				char *dst = (char *)getController().getPartialWindow();
				start += (WIDTH_FOR_CAMERA_CROP * y);
				start += x;
				char *result = dst;
				
				/////////////////////////////////////////////////////////////////////////////
				// cscho (2018-12.11)
				/*
				for (int i = 0; i < PARTIAL_HEIGHT; i++)
				{
					memcpy(dst, start, PARITAL_WIDTH);			
					dst += PARITAL_WIDTH;
					start += WIDTH_FOR_CAMERA_CROP;
				}
				J2CRenderCb partial = getController().getPartialRender();
				partial(result, PARITAL_WIDTH, PARTIAL_HEIGHT, 1);
				*/
				int nPartialWidth = getController().getPartialWidth();
				int nPartialHeight = getController().getPartialHeight();

				for (int i = 0; i < nPartialHeight; i++)
				{
					memcpy(dst, start, nPartialWidth);
					dst += nPartialWidth;
					start += WIDTH_FOR_CAMERA_CROP;
				}
				J2CRenderCb partial = getController().getPartialRender();
				partial(result, nPartialWidth, nPartialHeight, 1);
				
				// save 64*64
				{
					Mat  _src = cv::Mat(cvSize(nPartialWidth, nPartialHeight), CV_8UC1, result, cv::Mat::AUTO_STEP);
					Mat src;
					cvtColor(_src, src, cv::COLOR_GRAY2BGR);
					char path[256];
					sprintf(path, "c:\\jtwoc\\partial\\partial_%s.png", buf->getName());
					char szFName[256] = "";
					sprintf(szFName, "partial_%s.png", buf->getName());

					imwrite(path, src);

					FUNCTION_NAME_IS(_T("Scaler::__internalJob"));
					LOG_PRINTF(0, _T("Save partial-image file.[%d * %d] (Name= %s)"), nPartialWidth, nPartialHeight, szFName);
				}
				/////////////////////////////////////////////////////////////////////////////
			}

			if (saveFlag_ == true)
			{
				std::string path = savePath_ + "\\" + "crop" + buf->getName()+".png";
				getImageSaver().saveGrayImage(buf, path, WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP);
			}

			buf = nullptr;
			forDisplay = nullptr;

			index_++;
		}
	}

	void drawCircle(SharedBuffer &buf)
	{
		Mat src = cv::Mat(cvSize(WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP), CV_8UC1, buf->getBuffer(), cv::Mat::AUTO_STEP);
		circle(src, Point(CROP_FRAME_CENTER_X, CROP_FRAME_CENTER_Y), 150, Scalar(255, 255, 255), 5);
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

	void save(bool save, char *path)
	{
		saveFlag_ = save;
		savePath_ = path;
	}

	bool stopFlag_;
	int timeout_;
	std::thread *jobThread_;
	int index_;
	std::string savePath_ = "";
	bool saveFlag_ = false;
};

extern Scaler gScaler;
static inline Scaler &getScaler()
{
	return gScaler;
}

