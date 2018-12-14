#include "crop.h"
#include "opencv/cv.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <iostream>
using namespace cv;

Crop gCrop;
Crop::Crop()
{
	timeout_ = 10000;
	index_ = 0;
	seqId_ = 0;
}

void Crop::saveCropImage(SharedBuffer &buf)
{
	Mat  _src = cv::Mat(cvSize(WIDTH_FOR_CAMERA_CROP, HEIGHT_FOR_CAMERA_CROP), CV_8UC1, buf->getBuffer(), cv::Mat::AUTO_STEP);
	Mat src;
	cvtColor(_src, src, cv::COLOR_GRAY2BGR);
	char path[256];
	sprintf(path, "c:\\jtwoc\\crop\\crop_%s.png", buf->getName());
	imwrite(path, src);
}

void Crop::saveOpenCVImage(SharedBuffer &buf)
{
	Mat  _src = cv::Mat(cvSize(WIDTH_FOR_OPENCV, HEIGHT_FOR_OPENCV), CV_8UC1, buf->getBuffer(), cv::Mat::AUTO_STEP);
	Mat src;
	cvtColor(_src, src, cv::COLOR_GRAY2BGR);
	char path[256];
	sprintf(path, "c:\\jtwoc\\resize\\resize_%s.png", buf->getName());
	imwrite(path, src);
}



