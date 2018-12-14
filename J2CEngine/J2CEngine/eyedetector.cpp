#include "eyedetector.h"
#include <opencv/cv.h>
#include "colorbufferinfo.h"
#include "bufferpoolmanager.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <iostream>

EyeDetector *gEyeDetector;


EyeDetector::EyeDetector()
{
	stopFlag_ = false;
	timeout_ = 0;
	bool ret = eye_cascade_.load("C:\\opencv\\build\\etc\\haarcascades\\haarcascade_eye.xml");
	
	if (ret == false)
	{
		std::cout << "@@@ harr load failed @@@" << std::endl;
	}
	else
	{
		std::cout << "@@@harr load success @@@" << std::endl;
	}
}

std::vector<cv::Rect> *eyes = nullptr;
void EyeDetector::openCVEyeDetect(SharedBuffer &buf, std::vector<EyePosition> &positions)
{
	static int cnt = 0;
	EyePosition pos = { -1,-1,0,0 };

	//std::cout << "Get eye buffer" << std::endl;
	//cv::Mat image = cv::Mat(cvSize(WIDTH_FOR_OPENCV,HEIGHT_FOR_OPENCV), CV_8UC4, buf->getBuffer(), cv::Mat::AUTO_STEP );
	cv::Mat image = cv::Mat(cvSize(WIDTH_FOR_OPENCV, HEIGHT_FOR_OPENCV), CV_8UC1, buf->getBuffer(), cv::Mat::AUTO_STEP);
	char __safe[4096];
	if (eyes == nullptr) {
		eyes = new std::vector<cv::Rect>();
	}

	eyes->clear();
	if (image.data == nullptr)
	{
		OutputDebugString(L"error");
	}


	//cv::Mat gray= cv::Mat(WIDTH_FOR_EYEDETECT_VIEW , HEIGHT_FOR_EYEDETECT_VIEW , CV_8UC4, grayBuffer->getBuffer(),cv::Mat::AUTO_STEP);
	//cv::Mat gray = image;
	//cv::cvtColor(image, gray, CV_BGRA2GRAY);
	//cv::Rect  rect(100,100,100,100);
	//cv::rectangle(gray , rect, cv::Scalar(255,125,125));
	//imwrite("C:\\work\\gray.jpg",gray );

#if 1
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

	eye_cascade_.detectMultiScale(image, *eyes);

	if (enrollBlocked == true)
		getController().enrollUnBlock();
	if (identifyBlocked == true)
		getController().identifyUnBlock();

	int _cnt = 0;
#if 1
	if (eyes->size())
	{
		//std::cout << "Eyes detected" << std::endl;
		//OutputDebugString(L"Eyes detected");
		//qDebug() << "Eyes detected ";
		for (auto &&k : *eyes)
		{
			EyePosition pos = { k.x, k.y , k.width , k.height };
			positions.push_back(pos);
			//qDebug() << k.x << " " << k.y << " " << k.width << " " << k.height;
			//std::cout << "* eye detected " << k.x << " " << k.y << " " << k.width << " " << k.height << std::endl;;
			//cv::rectangle(image, k, cv::Scalar(255, 255, 0));
			if (_cnt == 0) {
				int xOffset = pos.width / 2;
				int yOffset = pos.height / 2;
				int centerX = pos.x + xOffset;
				int centerY = pos.y + yOffset;
				//cv::circle(image, {centerX,centerY},5, cv::Scalar(255,0,0),2);
			   // qDebug() << "@@@ center " << centerX << " " << centerY ;
			}
			++_cnt;
		}

		//sprintf(__safe, "c:\\work\\eye\\image%d.png", cnt++);
		//imwrite(__safe, image);

	}
#endif
#endif
	//delete eyes;
}
