#ifndef BUFFERPOOLMANAGER_H
#define BUFFERPOOLMANAGER_H
#include "bufferpool.h"
#include <unordered_map>
class BufferPoolManager
{
public:
	BufferPoolManager();

	void registerBufferPool(int buffer_pool_size, int buffer_size, int type, int poolid)
	{
		BufferPool *pool = new BufferPool();

		pool->alloc(buffer_pool_size, buffer_size, type, poolid);
		pools_.insert({ poolid, pool });
		//qDebug() << "register buffer " << buffer_size;
		//for ( auto &&k : pools_)
		//    qDebug() << k.first << " " << k.second ;
	}

	SharedBuffer getBuffer(int poolid, char *name = 0)
	{
		SharedBuffer buf = nullptr;
		BufferPool *pool = nullptr;
		//qDebug() << ">>> getBuffer called ";
		//qDebug() << ">>> pool size " << pools_.size() ;
		//for ( auto &&k : pools_)
		//    qDebug() << k.first << " " << k.second ;

		auto iter = pools_.find(poolid);
		if (iter == pools_.end()) {
			// qDebug() << "No suitable pool found for " << poolid  ;
			return buf;
		}

		pool = iter->second;
		if (name == 0)
			buf = pool->getBuffer();
		else
			buf = pool->getBuffer(name);
		return buf;
	}

	SharedBuffer getSavedBufferWithName(int poolid, char *name)
	{
		SharedBuffer buf = nullptr;
		BufferPool *pool = nullptr;

		auto iter = pools_.find(poolid);
		if (iter == pools_.end())
			return buf;

		pool = iter->second;
		buf = pool->getSavedBufferWithName(name);
		return buf;
	}

	void saveBufferWithName(int poolid, SharedBuffer &buf)
	{
		BufferPool *pool = nullptr;

		auto iter = pools_.find(poolid);
		if (iter == pools_.end())
			return;

		pool = iter->second;
		pool->saveBufferWithName(buf);
	}

	void deleteBufferWithName(int poolid, char *name)
	{
		BufferPool *pool = nullptr;

		auto iter = pools_.find(poolid);
		if (iter == pools_.end())
			return;

		pool = iter->second;
		pool->deleteSavedBufferWithName(name);
	}

	void deleteAllBufferWithName(int poolid)
	{
		BufferPool *pool = nullptr;

		auto iter = pools_.find(poolid);
		if (iter == pools_.end())
			return;

		pool = iter->second;
		pool->deleteAllBufferWithName();
	}

#if 0
	void returnBufferToPool(SharedBuffer &buf)
	{
		//int size = buf->getSize();
		BufferPool *pool = nullptr;

		auto iter = pools_.find(buf->getPoolId());
		if (iter == pools_.end())
			return;

		pool = iter->second;
		pool->returnBufferToPool(buf);
	}
#endif

	std::unordered_map<int, BufferPool *> pools_;

};

extern BufferPoolManager gBufferPoolManager;

static inline BufferPoolManager &getBufferPoolManager()
{
	return gBufferPoolManager;
}


#endif // BUFFERPOOLMANAGER_H
