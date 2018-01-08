/*
Copyright (c) 2013,2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#ifdef WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif
#include <time.h>

#include "mosquitto.h"
#include "time_mosq.h"

#if defined(_WIN32_WINNT) && (_WIN32_WINNT < _WIN32_WINNT_VISTA)
typedef DWORD (WINAPI* GetTickCountFunc)(void);
typedef ULONGLONG (WINAPI* GetTickCount64Func)(void);

static GetTickCountFunc fnGetTickCount = NULL;
static GetTickCount64Func fnGetTickCount64 = NULL;

static ULONGLONG ticks64 = 0;
static LONG Ticks_Initialized = 0;

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#include <intrin.h>

#pragma intrinsic(_InterlockedCompareExchange)
/* the non-intrinsic version is only available since Vista */
#pragma intrinsic(_InterlockedCompareExchange64)
#endif
#endif

void _mosquitto_time_lib_init()
{
#if defined(_WIN32_WINNT) && (_WIN32_WINNT < _WIN32_WINNT_VISTA)
	if (_InterlockedCompareExchange(&Ticks_Initialized, 1, 0) == 0) {
		HMODULE kernel32_dll = GetModuleHandle(TEXT("kernel32.dll"));
		if (kernel32_dll) {
			fnGetTickCount64 = (GetTickCount64Func) GetProcAddress(kernel32_dll, "GetTickCount64");
			if (!fnGetTickCount64) {
				fnGetTickCount = (GetTickCountFunc) GetProcAddress(kernel32_dll, "GetTickCount");
				ticks64 = fnGetTickCount();
			}
		}
		Ticks_Initialized = -1;
	} else {
		do {
			SwitchToThread();
		} while (Ticks_Initialized != -1);
	}
#endif
}

time_t mosquitto_time(void)
{
#ifdef WIN32
#if defined(_WIN32_WINNT) && (_WIN32_WINNT < _WIN32_WINNT_VISTA)
	LARGE_INTEGER old_ticks64, new_ticks64;
	long abs_difference;

	if (Ticks_Initialized != -1) {
		_mosquitto_time_lib_init();
	}

	if (fnGetTickCount64) {
		return (time_t) ((LONGLONG) fnGetTickCount64() / 1000);
	}

	old_ticks64.QuadPart = _InterlockedCompareExchange64((LONGLONG volatile*) &ticks64, 0, 0);
	abs_difference = (long) fnGetTickCount() - old_ticks64.LowPart;
	new_ticks64.QuadPart = old_ticks64.QuadPart + abs_difference;

	if (abs_difference < INT_MIN/2) {
		new_ticks64.QuadPart = old_ticks64.QuadPart + (DWORD) abs_difference;
		_InterlockedCompareExchange64((LONGLONG volatile*) &ticks64, new_ticks64.QuadPart, old_ticks64.QuadPart);
	}

	return (time_t) ((LONGLONG) new_ticks64.QuadPart / 1000);
#else
	return (time_t) ((LONGLONG) GetTickCount64() / 1000);
#endif
#elif _POSIX_TIMERS>0 && defined(_POSIX_MONOTONIC_CLOCK)
	struct timespec tp;

	clock_gettime(CLOCK_MONOTONIC, &tp);
	return tp.tv_sec;
#elif defined(__APPLE__)
	static mach_timebase_info_data_t tb;
    uint64_t ticks;
	uint64_t sec;

	ticks = mach_absolute_time();

	if(tb.denom == 0){
		mach_timebase_info(&tb);
	}
	sec = ticks*tb.numer/tb.denom/1000000000;

	return (time_t)sec;
#else
	return time(NULL);
#endif
}

