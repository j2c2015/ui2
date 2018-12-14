#include "eyeselector.h"
#include "opencv/cv.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "colorbufferinfo.h"
#include <vector>
#include <iostream>
using namespace cv;
using namespace std;
#define CONTOUR_DRAW 0
EyeSelector	gEyeSelector;
std::vector< std::vector< cv::Point> > contours_;
EyeSelector::EyeSelector()
{
	stopFlag_ = false;
	timeout_ = 100;
	seqId_ = 0;
	//contours_ = new std::vector< std::vector< cv::Point > >();
}
RNG rng(12345);

bool EyeSelector::findContour(SharedBuffer &buf, int &x, int &y, int centerX, int centerY)
{
	int thresh = 127;
	Mat src_gray = cv::Mat(cvSize(WIDTH_FOR_OPENCV, HEIGHT_FOR_OPENCV), CV_8UC1, buf->getBuffer(), cv::Mat::AUTO_STEP);
	Mat canny_output;
	Canny(src_gray, canny_output, thresh, thresh * 2);

	vector<Vec4i> hierarchy;
	contours_.clear();
	findContours(canny_output, contours_, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
	// we assume 1 contour will be found.

	int size = contours_.size();
	bool found = false;
	for (int i = 0; i < size; i++)
	{
		Rect r = boundingRect((contours_)[i]);
		Point center = (r.br() + r.tl()) * 0.5;
		x = center.x;
		y = center.y;

		//std::cout << i << " th contour at " << x << " " << y << std::endl;
		if (fabs(centerX - x) > (OPENCV_FRAME_CENTER_X *0.08))
			continue;
		if (fabs(centerY - y) > (OPENCV_FRAME_CENTER_Y * 0.08))
			continue;

		found = true;
		break;
	}
	//std::cout << "***** Contour found at " << x << ", " << y;
	return found;

#if CONTOUR_DRAW
	Mat drawing = Mat::zeros(canny_output.size(), CV_8UC3);
	for (size_t i = 0; i < contours.size(); i++)
	{
		Scalar color = Scalar(rng.uniform(0, 256), rng.uniform(0, 256), rng.uniform(0, 256));
		drawContours(drawing, contours, (int)i, color, 2, LINE_8, hierarchy, 0);
	}
#endif
}

void EyeSelector::saveToFile(SharedBuffer &buf, std::string &id)
{
	Mat  _src = cv::Mat(cvSize(WIDTH_FOR_OPENCV, HEIGHT_FOR_OPENCV), CV_8UC1, buf->getBuffer(), cv::Mat::AUTO_STEP);
	Mat src;
	cvtColor(_src, src, cv::COLOR_GRAY2BGR);
	char path[256];
	sprintf(path, "c:\\jtwoc\\eyeselect\\%s_%d.png", id.c_str(), seqId_++);
	imwrite(path, src);
}