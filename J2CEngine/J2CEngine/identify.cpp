#include "identify.h"
#include "opencv/cv.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;
IdentifyTask gIdentifyTask;

SharedBuffer IdentifyTask::resizeForNeruoTechnology(SharedBuffer &buf, int centerX, int centerY)
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