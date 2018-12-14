#ifndef MULTICOREQUEUETEMPLATE_H
#define MULTICOREQUEUETEMPLATE_H

#include <thread>
#include <mutex>
#include <queue>
#include "bufferpoolmanager.h"
#include <condition_variable>

template <typename MyType>
class MultiCoreQueueTemplate
{
public:
	MultiCoreQueueTemplate() {}


	void putSharedBuffer(MyType &buf)
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

	MyType getSharedBuffer()
	{
		MyType buf = MyType();
		std::lock_guard<std::mutex> lock(lock_);
		if (buffers_.size() == 0)
			return buf;

		buf = buffers_.front();
		buffers_.pop();
		if (buffers_.size() == 0)
			ready_ = false;

		return buf;
	}

	MyType getSharedBufferWithTimeout(int ms)
	{
		//qDebug() << "MultiCoreQueue getSharedBufferWithTimeout called";
		MyType buf = MyType();
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

private:
	std::queue<MyType> buffers_;
	bool ready_;
	bool waiting_;
	std::mutex lock_;
	std::condition_variable cond_;
};

typedef struct {
	int x;
	int y;
	SharedBuffer opencvSharedBuffer;
}  EyeSelectSharedBuffer;

typedef MultiCoreQueueTemplate< EyeSelectSharedBuffer > EyeSelectMultiCoreQueue;

extern EyeSelectMultiCoreQueue gEyeSelectMultiCoreQueue;

static inline EyeSelectMultiCoreQueue &getEyeSelectMultiCoreQueue()
{
	return gEyeSelectMultiCoreQueue;
}

typedef struct {
	std::string id;
	SharedBuffer opencvSharedBuffer;
	SharedBuffer cropSharedBuffer;
	int x;	// x-eye position 
	int y;  // y-eye position
} EnrollSharedBuffer;

typedef MultiCoreQueueTemplate< EnrollSharedBuffer >   EnrollMultiCoreQueue;

#define NEURO_ENROLL_COMMAND	1
#define NEURO_IDENTIFY_COMMAND	2
#define NEURO_ENROLL_RELOAD_COMMAND 3
typedef struct _NeuroSharedBuffer {
	SharedBuffer buf;
	int command;
	_NeuroSharedBuffer()
	{
		command = -1;
	}
} NeuroSharedBuffer;

typedef MultiCoreQueueTemplate<NeuroSharedBuffer> NeuroMultiCoreQueue;

extern NeuroMultiCoreQueue	gNeuroMultiCoreQueue;

static inline NeuroMultiCoreQueue &getNeuroMultiCoreQueue()
{
	return gNeuroMultiCoreQueue;
}

extern EnrollMultiCoreQueue gEnrollMultiCoreQueue;
extern EnrollMultiCoreQueue gIdentifyMultiCoreQueue;

static inline EnrollMultiCoreQueue &getEnrollMultiCoreQueue()
{
	return gEnrollMultiCoreQueue;
}

static inline EnrollMultiCoreQueue &getIdentifyMultiCoreQueue()
{
	return gIdentifyMultiCoreQueue;
}

#endif // MULTICOREQUEUETEMPLATE_H
