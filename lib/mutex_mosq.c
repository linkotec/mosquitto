/*
Copyright (c) 2018 Jukka Ojanen <jukka.ojanen@linkotec.net>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Jukka Ojanen - initial implementation
*/

#include "mutex_mosq.h"

#if defined(WITH_THREADING) && defined(_WIN32)
#include <windows.h>
#include <assert.h>
#include <errno.h>

#if defined(_WIN32_WINNT) && (_WIN32_WINNT < _WIN32_WINNT_VISTA)
typedef void (WINAPI* SRWLockFunc)(PSRWLOCK);
static SRWLockFunc fnAcquireSRWLockExclusive = NULL;
static SRWLockFunc fnAcquireSRWLockShared = NULL;
static SRWLockFunc fnInitializeSRWLock = NULL;
static SRWLockFunc fnReleaseSRWLockExclusive =NULL;
static SRWLockFunc fnReleaseSRWLockShared = NULL;
static LONG SRW_Initialized = 0;

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#include <intrin.h>
#pragma intrinsic(_InterlockedCompareExchange)
#endif
#endif

void _mosquitto_mutex_lib_init()
{
#if defined(_WIN32_WINNT) && (_WIN32_WINNT < _WIN32_WINNT_VISTA)
	if (_InterlockedCompareExchange(&SRW_Initialized, 1, 0) == 0) {
		HMODULE kernel32_dll = GetModuleHandle(TEXT("kernel32.dll"));
		if (kernel32_dll) {
			fnInitializeSRWLock = (SRWLockFunc) GetProcAddress(kernel32_dll, "InitializeSRWLock");
			if (fnInitializeSRWLock) {
				fnAcquireSRWLockExclusive = (SRWLockFunc) GetProcAddress(kernel32_dll, "AcquireSRWLockExclusive");
				fnAcquireSRWLockShared    = (SRWLockFunc) GetProcAddress(kernel32_dll, "AcquireSRWLockShared");
				fnReleaseSRWLockExclusive = (SRWLockFunc) GetProcAddress(kernel32_dll, "ReleaseSRWLockExclusive");
				fnReleaseSRWLockShared    = (SRWLockFunc) GetProcAddress(kernel32_dll, "ReleaseSRWLockShared");
			}
		}
		SRW_Initialized = -1;
	} else {
		do {
			SwitchToThread();
		} while (SRW_Initialized != -1);
	}
#endif
}

int _mosquitto_mutex_init(mosq_mutex_t *mutex)
{
#if defined(_WIN32_WINNT) && (_WIN32_WINNT < _WIN32_WINNT_VISTA)
	if (SRW_Initialized != -1) {
		_mosquitto_mutex_lib_init();
	}
#endif
	if (!mutex) {
		return EINVAL;
	}
#if defined(_WIN32_WINNT) && (_WIN32_WINNT < _WIN32_WINNT_VISTA)
	if (fnInitializeSRWLock) {
		fnInitializeSRWLock((PSRWLOCK) mutex);
	} else {
		mutex->ptr = malloc(sizeof(CRITICAL_SECTION));
		if (!mutex->ptr) {
			return ENOMEM;
		}
		InitializeCriticalSection((LPCRITICAL_SECTION) mutex->ptr);
	}
#else
	InitializeSRWLock((PSRWLOCK) mutex);
#endif
	return 0;
}

int _mosquitto_mutex_destroy(mosq_mutex_t *mutex)
{
	if (!mutex) {
		return EINVAL;
	}
#if defined(_WIN32_WINNT) && (_WIN32_WINNT < _WIN32_WINNT_VISTA)
	if (!fnInitializeSRWLock) {
		DeleteCriticalSection((LPCRITICAL_SECTION) mutex->ptr);
		free(mutex->ptr);
		mutex->ptr = NULL;
	}
#endif
	return 0;
}

int _mosquitto_mutex_acquire(mosq_mutex_t *mutex)
{
	if (!mutex) {
		return EINVAL;
	}
#if defined(_WIN32_WINNT) && (_WIN32_WINNT < _WIN32_WINNT_VISTA)
	if (fnAcquireSRWLockExclusive) {
		fnAcquireSRWLockExclusive((PSRWLOCK) mutex);
	} else {
		EnterCriticalSection((LPCRITICAL_SECTION) mutex->ptr);
	}
#else
	AcquireSRWLockExclusive((PSRWLOCK) mutex);
#endif
	return 0;
}

int _mosquitto_mutex_release(mosq_mutex_t *mutex)
{
	if (!mutex) {
		return EINVAL;
	}
#if defined(_WIN32_WINNT) && (_WIN32_WINNT < _WIN32_WINNT_VISTA)
	if (fnReleaseSRWLockExclusive) {
		fnReleaseSRWLockExclusive((PSRWLOCK) mutex);
	} else {
		LeaveCriticalSection((LPCRITICAL_SECTION) mutex->ptr);
	}
#else
	ReleaseSRWLockExclusive((PSRWLOCK) mutex);
#endif
	return 0;
}

int _mosquitto_mutex_acquire_shared(mosq_mutex_t *mutex)
{
	if (!mutex) {
		return EINVAL;
	}
#if defined(_WIN32_WINNT) && (_WIN32_WINNT < _WIN32_WINNT_VISTA)
	if (fnAcquireSRWLockShared) {
		fnAcquireSRWLockShared((PSRWLOCK) mutex);
	} else {
		EnterCriticalSection((LPCRITICAL_SECTION) mutex->ptr);
	}
#else
	AcquireSRWLockShared((PSRWLOCK) mutex);
#endif
	return 0;
}

int _mosquitto_mutex_release_shared(mosq_mutex_t *mutex)
{
	if (!mutex) {
		return EINVAL;
	}
#if defined(_WIN32_WINNT) && (_WIN32_WINNT < _WIN32_WINNT_VISTA)
	if (fnReleaseSRWLockShared) {
		fnReleaseSRWLockShared((PSRWLOCK) mutex);
	} else {
		LeaveCriticalSection((LPCRITICAL_SECTION) mutex->ptr);
	}
#else
	ReleaseSRWLockShared((PSRWLOCK) mutex);
#endif
	return 0;
}
#endif
