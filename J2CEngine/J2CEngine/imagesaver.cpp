#include "imagesaver.h"
#include <opencv/cv.h>
#include "colorbufferinfo.h"
#include "bufferpoolmanager.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/objdetect/objdetect.hpp>

ImageSaver gImageSaver;
ImageSaver::ImageSaver()
{
	jobThread_ = nullptr;
	stopFlag_ = false;
}

void ImageSaver::saveToFile(ImageSaveParam &param)
{
	if (param.format == GRAY) {
		cv::Mat image = cv::Mat(cvSize(param.width, param.height), CV_8UC1, param.buffer->getBuffer(), cv::Mat::AUTO_STEP);
		cv::imwrite(param.filename, image);
	}
	else if (param.format == RGB) {
		cv::Mat image = cv::Mat(cvSize(param.width, param.height), CV_8UC3, param.buffer->getBuffer(), cv::Mat::AUTO_STEP);
		cv::imwrite(param.filename, image);
	}
	else if (param.format == RGBA) {
		cv::Mat image = cv::Mat(cvSize(param.width, param.height), CV_8UC4, param.buffer->getBuffer(), cv::Mat::AUTO_STEP);
		cv::imwrite(param.filename, image);
	}
}
