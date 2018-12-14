#ifndef IMAGESAVER_H
#define IMAGESAVER_H
#include "bufferpoolmanager.h"
#include "multicorequeuetemplate.h"
#include <string>
#include <thread>
#include "LogMgr.h"

enum ImageFormatParam { GRAY = 0, RGB, RGBA };

struct ImageSaveParam {
	std::string filename;
	int width;
	int height;
	int format;

	SharedBuffer buffer;

	ImageSaveParam() {
		filename = "";
		buffer = nullptr;
		width = 0;
		height = 0;
		format = GRAY;
	}

	bool isEmpty()
	{
		if (filename == "")
			return true;
		return false;
	}
};

class ImageSaver
{
public:
	ImageSaver();

	void startJob()
	{
		stopFlag_ = false;
		jobThread_ = new std::thread(std::bind(&ImageSaver::__internalJob, this));
		jobThread_->detach();
	}

	void stopJob()
	{
		stopFlag_ = true;
	}

	void saveGrayImage(SharedBuffer &buf, std::string filename, int width, int height)
	{
		ImageSaveParam param;
		param.buffer = buf;
		param.filename = filename;
		param.width = width;
		param.height = height;
		param.format = GRAY;

		jobQueue_.putSharedBuffer(param);
	}

	void saveRGBImage(SharedBuffer &buf, std::string filename, int width, int height)
	{
		ImageSaveParam param;
		param.buffer = buf;
		param.filename = filename;
		param.width = width;
		param.height = height;
		param.format = RGB;

		jobQueue_.putSharedBuffer(param);
	}

	void saveRGBAImage(SharedBuffer &buf, std::string filename, int width, int height)
	{
		ImageSaveParam param;
		param.buffer = buf;
		param.filename = filename;
		param.width = width;
		param.height = height;
		param.format = RGBA;

		jobQueue_.putSharedBuffer(param);
	}

	void saveToFile(ImageSaveParam &param);

	void __internalJob()
	{
		FUNCTION_NAME_IS(_T("ImageSaver::__internalJob"));
		LOG_PUTS(0, _T("ImageSaver thread starts..."));

		while (stopFlag_ == false)
		{
			ImageSaveParam param = jobQueue_.getSharedBufferWithTimeout(50);
			if (param.isEmpty() == true)
				continue;

			saveToFile(param);
			param.buffer = nullptr;
		}
	}

	std::thread *jobThread_;
	bool stopFlag_;
	MultiCoreQueueTemplate<ImageSaveParam>  jobQueue_;
};

extern ImageSaver gImageSaver;
static inline ImageSaver &getImageSaver()
{
	return gImageSaver;
}
#endif // IMAGESAVER_H
