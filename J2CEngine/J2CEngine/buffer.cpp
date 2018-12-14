#include "buffer.h"
//test comment
Buffer::Buffer()
{
	memory_ = 0;
	name_[0] = 0;
	size_ = -1;
	type_ = -1;
	available_ = -1;
	poolId_ = -1;
	tick_ = 0;
}


