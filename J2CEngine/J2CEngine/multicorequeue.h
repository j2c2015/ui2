#ifndef MULTICOREQUEUE_H
#define MULTICOREQUEUE_H

#include <thread>
#include <mutex>
#include <queue>
#include "bufferpoolmanager.h"
#include <condition_variable>

class MultiCoreQueue
{
public:
	MultiCoreQueue();

	void putSharedBuffer(SharedBuffer &buf)
	{
		bool notify = false;
		{
			std::lock_guard<std::mutex> lock(lock_);
			if (buffers_.size() == 0)
				notify = true;
			buffers_.push(buf);
			ready_ = true;
			//qDebug() << ">>> Buf use count " << buf.use_count() ;
		}
		//if ( notify == true && waiting_ =  )
		cond_.notify_one();
		//qDebug() << "---- putBuffer done ----";
	}

	SharedBuffer getSharedBuffer()
	{
		SharedBuffer buf = nullptr;
		std::lock_guard<std::mutex> lock(lock_);
		if (buffers_.size() == 0)
			return buf;

		buf = buffers_.front();
		buffers_.pop();
		if (buffers_.size() == 0)
			ready_ = false;

		return buf;
	}

	SharedBuffer getSharedBufferWithTimeout(int ms)
	{
		//qDebug() << "MultiCoreQueue getSharedBufferWithTimeout called";
		SharedBuffer buf = nullptr;
		bool getFlag = false;
		std::unique_lock<std::mutex> lock(lock_);
		if (ready_ == false) {
			if (cond_.wait_for(lock, std::chrono::milliseconds(ms), [this] { return ready_; }))
			{
				buf = buffers_.front();
				buffers_.pop();
				if (buffers_.size() == 0)
					ready_ = false;
				waiting_ = false;
				getFlag = true;
			}
			else
			{
				// timeout, get buffer failed;
				//qDebug() << "getSharedBufferWithTimeout() get failed";
			}
		}
		else
		{
			buf = buffers_.front();
			buffers_.pop();
			if (buffers_.size() == 0)
				ready_ = false;
		}
		lock.unlock();
#if 0
		if (getFlag == true)
			qDebug() << "^^^^  getBuffer form MultiCoreQueue success ^^^";
#endif
		return buf;
	}

	int getSize()
	{
		std::unique_lock<std::mutex> lock(lock_);
		return buffers_.size();
	}

private:
	std::queue<SharedBuffer> buffers_;
	bool ready_;
	bool waiting_;
	std::mutex lock_;
	std::condition_variable cond_;
};

extern MultiCoreQueue  gMultiCoreQueueForOpenCV;
extern MultiCoreQueue  gMultiCoreQueueForCameraCrop;
extern MultiCoreQueue  gMultiCoreQueueForScaler;
extern MultiCoreQueue  gMultiCoreQueueForEyeDetectViewer;
extern MultiCoreQueue  gMultiCoreQueueForNeuro;
extern MultiCoreQueue  gMultiCoreQueueForCameraInput;
extern MultiCoreQueue  gMultiCoreQueueForEyeViewer;
extern MultiCoreQueue  gMultiCoreQueueForEyeSelector;
extern MultiCoreQueue  gMultiCoreQueueForKind7;
extern MultiCoreQueue  gMultiCoreQueueForNetwork;
extern MultiCoreQueue  gMultiCoreQueueForIdentify;

static inline MultiCoreQueue &getMultiCoreQueueForOpenCV()
{
	return gMultiCoreQueueForOpenCV;
}

static inline MultiCoreQueue &getMultiCoreQueueForIdentify()
{
	return gMultiCoreQueueForIdentify;
}

static inline MultiCoreQueue &getMultiCoreQueueForNetwork()
{
	return gMultiCoreQueueForNetwork;
}

static inline MultiCoreQueue &getMultiCoreQueueForEyeSelector()
{
	return gMultiCoreQueueForEyeSelector;
}

static inline MultiCoreQueue &getMultiCoreQueueForKind7()
{
	return gMultiCoreQueueForKind7;
}

static inline MultiCoreQueue &getMultiCoreQueueForCameraCrop()
{
	return gMultiCoreQueueForCameraCrop;
}

static inline MultiCoreQueue &getMultiCoreQueueForScaler()
{
	return gMultiCoreQueueForScaler;
}

static inline MultiCoreQueue &getMultiCoreQueueForEyeDetectViewer()
{
	return gMultiCoreQueueForEyeDetectViewer;
}

static inline MultiCoreQueue &getMultiCoreQueueForNeuro()
{
	return gMultiCoreQueueForNeuro;
}

static inline MultiCoreQueue &getMultiCoreQueueForCameraInput()
{
	return gMultiCoreQueueForCameraInput;
}

static inline MultiCoreQueue &getMultiCoreQueueForEyeViewer()
{
	return gMultiCoreQueueForEyeViewer;
}

#endif // MULTICOREQUEUE_H
