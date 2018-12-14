#ifndef BUFFERPOOL_H
#define BUFFERPOOL_H
#include "buffer.h"
#include <utility>
#include <memory>
#include <list>
#include <queue>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <string>
#include "colorbufferinfo.h"
#include <iostream>
//namespace J2C {
class BufferPool
{
public:
	BufferPool();

	int alloc(int num, int size, int type, int poolid)
	{
		int cnt = 0;
		poolId_ = poolid;
		for (int i = 0; i < num; i++)
		{
			//std::shared_ptr<Buffer>  buffer = std::make_shared<Buffer>();
			Buffer *buffer = new Buffer();
			int ret = buffer->alloc(size);

			if (ret == size)
			{
				buffer->setType(type);
				buffer->setPoolId(poolid);
				queue_.push(buffer);
				++cnt;
			}
			buffer = nullptr;
		}
		size_ = size;
		return cnt;
	}


	SharedBuffer  getBuffer(char *name = 0)
	{
		SharedBuffer buf = nullptr;
		//std::lock_guard< std::mutex > lock(lock_);
		std::lock_guard< std::mutex > lock(queueLock_);
		if (queue_.size() == 0)
			return buf;

	
		Buffer *buffer = nullptr;
		buffer = queue_.front();
		queue_.pop();

		buf = std::make_shared<BufferHolder>();
		buf->attachBuffer(buffer);
		buf->setReturnToPoolFn(std::bind(&BufferPool::returnBufferToPool, this, std::placeholders::_1));

		if (name != 0) {
			//std::string str( buffer->getName());
			//deleteNameMap(str);
			buffer->clearName();
			buffer->setName(name);
			insertNameMap(name, buf);
		}
		else
			buffer->clearName();

		return buf;
	}

	void saveBufferWithName(SharedBuffer buffer)
	{
		std::lock_guard< std::mutex > lock(lock_);
		std::string str(buffer->getName());

		deleteNameMap(str);
		insertNameMap(str, buffer);
	}

	SharedBuffer getSavedBufferWithName(char *name)
	{
		std::lock_guard<std::mutex> lock(lock_);
		SharedBuffer buf = nullptr;
		std::string str(name);

		auto iter = nameMap_.find(str);
		if (iter == nameMap_.end())
			return buf;

		buf = iter->second;
		return buf;
	}

	void deleteSavedBufferWithName(char *name)
	{
		std::lock_guard<std::mutex> lock(lock_);
		std::string str(name);

		auto iter = nameMap_.find(str);
		if (iter != nameMap_.end())
			deleteNameMap(str);
	}

	void deleteAllBufferWithName()
	{
		std::lock_guard<std::mutex> lock(lock_);
		nameMap_.clear();
	}

#if 0
	void returnBufferToPool(SharedBuffer &buf)
	{
		std::lock_guard<std::mutex> lock(lock_);
		std::string name(buf->getName());

		SharedBuffer buf2 = getNameMap(name);
		qDebug() << "use count " << buf.use_count();
		if (buf2 == nullptr && buf.use_count() == 2)
		{
			if (size_ != FRAMESIZE_FOR_CAMERA_FRAME)
				qDebug() << "Buffer deallocating No name map ";
			queue_.push(buf);
			buf = nullptr;
			return;
		}

		// the buf is contained in name map.
		if (buf2.use_count() == 4)
		{
			// the buf is not referenced at anywhere.
			// parameter buf  --> reference counting 1
			// shared_from_this ( see buffer.h ) -> reference counting + 1
			// buf2           --> reference counting +1
			// buf in name map --> reference counting +1
			// reference counting sum is 4
			queue_.push(buf);
			deleteNameMap(name);
			if (size_ != FRAMESIZE_FOR_CAMERA_FRAME)
				qDebug() << "Buffer deallocating Name map ";
		}
		else {
			// the buf may be refenced at anywhere
			// so we can not enqueue the buf.
		}
		buf = nullptr;
		buf2 = nullptr;
	}
#endif

	void returnBufferToPool(Buffer *buffer)
	{
		//std::lock_guard<std::mutex> lock(lock_);
		std::lock_guard<std::mutex> lock(queueLock_);
		//if (poolId_ == CAMERA_CROP_BUFFER_POOL_ID)
		//	std::cout << "Cameraframe dealloc " << buffer->getName() << std::endl;
		queue_.push(buffer);

	}

	void clear()
	{

		while (1)
		{
			Buffer *buffer = nullptr;
			{
				std::lock_guard<std::mutex> lock(queueLock_);
				if (queue_.size() == 0)
					return;
				buffer = queue_.front();
				queue_.pop();
			}
			std::string name(buffer->getName());
			{
				std::lock_guard<std::mutex> lock(lock_);
				deleteNameMap(name);
			}

			buffer = nullptr;
		}
	}

	// Below function must be called with the lock locked.
	void insertNameMap(std::string str, SharedBuffer buf)
	{
		nameMap_.insert({ str,buf });
	}

	// Below function must be called with the lock locked.
	void deleteNameMap(std::string &str)
	{
		nameMap_.erase(str);
	}

	// Below function must be called with the lock locked.
	SharedBuffer getNameMap(std::string &str)
	{
		SharedBuffer buf = nullptr;
		auto iter = nameMap_.find(str);
		if (iter == nameMap_.end())
			return buf;
		buf = iter->second;
		return buf;
	}


	std::queue<Buffer *> queue_;
	std::unordered_map< std::string, SharedBuffer > nameMap_;
	std::mutex  lock_;
	std::mutex  queueLock_;
	int size_;
	int poolId_;
};
//}
#endif // BUFFERPOOL_H
