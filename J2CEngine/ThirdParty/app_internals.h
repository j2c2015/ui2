#include <Windows.h>

#ifndef _INTERNALS_H_
#define _INTERNALS_H_

#include "common.h"
#include "kernel.h"
#include "engine.h"

typedef void (FN_ENGINE_CALLBACK)(IN void_t *, IN SIZE_T);

typedef FN_ENGINE_CALLBACK *PFN_ENGINE_CALLBACK;


	void internal_prepareJob(PFN_ENGINE_CALLBACK);
	void internal_postJob();


#endif // _INTERNALS_H_
