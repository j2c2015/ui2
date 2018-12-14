#ifndef BUFFER_H
#define BUFFER_H
#define MAX_BUFFER_NAME     64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <utility>
#include <functional>
#include <thread>
#include <mutex>
class Buffer
{
public:
	Buffer();
	~Buffer()
	{
		dealloc();
	}

	int getSize()
	{
		return size_;
	}
	void setName(char *name)
	{
		strcpy(name_, name);
	}

	char * getName()
	{
		return name_;
	}

	void clearName()
	{
		name_[0] = 0;
	}

	int alloc(int size)
	{
		memory_ = (void *)malloc(size);
		if (memory_ == 0)
			return -1;
		size_ = size;
		return size;
	}

	void dealloc()
	{
		if (memory_ == 0)
			return;
		free(memory_);
		memory_ = 0;
	}

	void setType(int type)
	{
		type_ = type;
	}

	void *getBuffer()
	{
		return memory_;
	}

	void setAvailable(int avail)
	{
		available_ = avail;
	}

	int getAvailable()
	{
		return available_;
	}

#if 0
	void saveToFile(char *path)
	{
		setMySelf();
		char *arg = (char *)malloc(strlen(path) + 1);
		strcpy(arg, path);
		std::thread saveJob([](std::shared_ptr<Buffer> buf, char *path)
		{
			buf->clearMySelf();
			FILE *f = fopen(path, "w");
			if (f == 0)
				return;

			int targt = buf->getAvailable();
			void *p = buf->getBuffer();
			fwrite(p, 1, targt, f);
			fclose(f);
			free(path);
			buf->returnToPool();
		}, shared_from_this(), arg);
		saveJob.detach();
	}
#endif

	void setPoolId(int id)
	{
		poolId_ = id;
	}

	int getPoolId()
	{
		return poolId_;
	}

	long getTick()
	{
		return tick_;
	}

	void setTick(long tick)
	{
		tick_ = tick;
	}

	char name_[MAX_BUFFER_NAME];
	void *memory_;
	int size_;
	int type_;
	int available_;
	int poolId_;
	long tick_;

};

class BufferHolder : public std::enable_shared_from_this<BufferHolder>
{
public:
	BufferHolder() {
		buffer_ = 0;
		myself_ = nullptr;
	}
	~BufferHolder() {
		fn_(buffer_);
	}

	void attachBuffer(Buffer *buf)
	{
		buffer_ = buf;
	}

	int getSize()
	{
		return buffer_->getSize();
	}

	void setName(char *name)
	{
		buffer_->setName(name);
	}

	char *getName()
	{
		return buffer_->getName();
	}

	void clearName()
	{
		buffer_->clearName();
	}

	void setType(int type)
	{
		buffer_->setType(type);
	}

	void setTick(long tick)
	{
		buffer_->setTick(tick);
	}

	long getTick()
	{
		return buffer_->getTick();
	}

	void *getBuffer()
	{
		return buffer_->getBuffer();
	}

	void setAvailable(int avail)
	{
		buffer_->setAvailable(avail);
	}

	int getAvailable()
	{
		return buffer_->getAvailable();
	}

	void setReturnToPoolFn(std::function<void(Buffer *)> fn)
	{
		fn_ = fn;
	}

	//void returnToPool()
	//{
	//    returnToPoolFn_();
	//}

	void setPoolId(int id)
	{
		buffer_->setPoolId(id);
	}

	int getPoolId()
	{
		return buffer_->getPoolId();
	}

	void saveToFile(char *path)
	{
		setMySelf();
		char *arg = (char *)malloc(strlen(path) + 1);
		strcpy(arg, path);
		std::thread saveJob([](std::shared_ptr<BufferHolder> buf, char *path)
		{
			buf->clearMySelf();
			FILE *f = fopen(path, "w");
			if (f == 0)
				return;

			int targt = buf->getAvailable();
			void *p = buf->getBuffer();
			fwrite(p, 1, targt, f);
			fclose(f);
			free(path);
			//buf->returnToPool();
			buf = nullptr;
		}, shared_from_this(), arg);
		saveJob.detach();
	}

	void clearMySelf()
	{
		myself_ = nullptr;
	}

	void setMySelf()
	{
		myself_ = shared_from_this();
	}

	Buffer *buffer_;
	std::shared_ptr<BufferHolder> myself_;
	std::function<void(Buffer *)> fn_;
};

using SharedBuffer = std::shared_ptr<BufferHolder>;

#endif // BUFFER_H


