#include "scale.h"

Scaler gScaler;
Scaler::Scaler()
{
	timeout_ = 10000;
	index_ = 0;
}