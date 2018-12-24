#include "enroll.h"
#include "opencv/cv.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;
EnrollTask gEnrollTask;
SharedBuffer EnrollTask::resizeForNeruoTechnology(SharedBuffer &buf, int centerX, int centerY)
{
	SharedBuffer resized = nullptr;
	// to do

	resized = getBufferPoolManager().getBuffer(EYE_VIEW_BUFFER_POOL_ID);
	if (resized == nullptr)
	{
		std::cout << "Failed to get resized buffer" << std::endl;
		return nullptr;
	}
	
	float xRatio = (float)centerX / (float)WIDTH_FOR_OPENCV;
	float yRatio = (float)centerY / (float)HEIGHT_FOR_OPENCV;

	int cx = (int)(WIDTH_FOR_CAMERA_CROP * xRatio) - (WIDTH_FOR_EYE_VIEW / 2);
	if (cx < 0)
		cx = 0;
	int cy = (int)(HEIGHT_FOR_CAMERA_CROP * yRatio) - (HEIGHT_FOR_EYE_VIEW / 2);
	if (cy < 0)
		cy = 0;

	char * p = (char *)buf->getBuffer();
	char *dst = (char *)resized->getBuffer();
	p += (cy * WIDTH_FOR_CAMERA_CROP);
	p += (cx);

	for (int i = 0; i < HEIGHT_FOR_EYE_VIEW; i++)
	{
		memcpy(dst, p, WIDTH_FOR_EYE_VIEW);
		dst += WIDTH_FOR_EYE_VIEW;
		p += WIDTH_FOR_CAMERA_CROP;
	}
	return resized;
}

SharedBuffer EnrollTask::resizeForNeruoTechnologySize(SharedBuffer &buf, int centerX, int centerY, int width, int height)
{
	SharedBuffer resized = nullptr;
	// to do

	resized = getBufferPoolManager().getBuffer(EYE_VIEW_BUFFER_POOL_ID);
	if (resized == nullptr)
	{
		std::cout << "Failed to get resized buffer" << std::endl;
		return nullptr;
	}

	float xRatio = (float)centerX / (float)width;
	float yRatio = (float)centerY / (float)height;
	
	int cx = (int)(WIDTH_FOR_CAMERA_CROP * xRatio) - (WIDTH_FOR_EYE_VIEW / 2);
	if (cx < 0)
		cx = 0;
	if ((cx + WIDTH_FOR_EYE_VIEW) > WIDTH_FOR_CAMERA_CROP)
		cx = (WIDTH_FOR_CAMERA_CROP - WIDTH_FOR_EYE_VIEW);
	int cy = (int)(HEIGHT_FOR_CAMERA_CROP * yRatio) - (HEIGHT_FOR_EYE_VIEW / 2);
	if (cy < 0)
		cy = 0;
	if ((cy + HEIGHT_FOR_EYE_VIEW) > HEIGHT_FOR_CAMERA_CROP)
		cy = (HEIGHT_FOR_CAMERA_CROP - HEIGHT_FOR_EYE_VIEW);

	char * p = (char *)buf->getBuffer();
	char *dst = (char *)resized->getBuffer();
	p += (cy * WIDTH_FOR_CAMERA_CROP);
	p += (cx);

	for (int i = 0; i < HEIGHT_FOR_EYE_VIEW; i++)
	{
		memcpy(dst, p, WIDTH_FOR_EYE_VIEW);
		dst += WIDTH_FOR_EYE_VIEW;
		p += WIDTH_FOR_CAMERA_CROP;
	}
	return resized;
}

void EnrollTask::saveToFile(SharedBuffer &buf, std::string &id)
{
	Mat  _src = cv::Mat(cvSize(WIDTH_FOR_OPENCV, HEIGHT_FOR_OPENCV), CV_8UC1, buf->getBuffer(), cv::Mat::AUTO_STEP);
	Mat src;
	cvtColor(_src, src, cv::COLOR_GRAY2BGR);
	char path[256];
	sprintf(path, "c:\\jtwoc\\enroll\\%s.png", id.c_str());
	imwrite(path, src);
}