/*
Copyright (c) 2011-2014 Roger Light <roger@atchoo.org>

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

#include <config.h>

#ifndef WIN32
#include <unistd.h>
#else
#include <process.h>
#endif

#include <mosquitto_internal.h>
#include <net_mosq.h>

#ifdef _WIN32
static unsigned WINAPI _mosquitto_thread_main(void *obj);
#else
static void *_mosquitto_thread_main(void *obj);
#endif

int mosquitto_loop_start(struct mosquitto *mosq)
{
#ifdef WITH_THREADING
	if(!mosq || mosq->threaded != mosq_ts_none) return MOSQ_ERR_INVAL;

	mosq->threaded = mosq_ts_self;
#ifdef _WIN32
	mosq->thread = (HANDLE) _beginthreadex(NULL, 0, &_mosquitto_thread_main, mosq, 0, NULL);
	if(mosq->thread == INVALID_HANDLE_VALUE){
		errno = GetLastError();
		return MOSQ_ERR_ERRNO;
	}else{
		return MOSQ_ERR_SUCCESS;
	}
#else
	if(!pthread_create(&mosq->thread_id, NULL, _mosquitto_thread_main, mosq)){
		return MOSQ_ERR_SUCCESS;
	}else{
		return MOSQ_ERR_ERRNO;
	}
#endif
#else
	return MOSQ_ERR_NOT_SUPPORTED;
#endif
}

int mosquitto_loop_stop(struct mosquitto *mosq, bool force)
{
#ifdef WITH_THREADING
#  ifndef WITH_BROKER
	char sockpair_data = 0;
#  endif

	if(!mosq || mosq->threaded != mosq_ts_self) return MOSQ_ERR_INVAL;


	/* Write a single byte to sockpairW (connected to sockpairR) to break out
	 * of select() if in threaded mode. */
	if(mosq->sockpairW != INVALID_SOCKET){
#ifndef WIN32
		if(write(mosq->sockpairW, &sockpair_data, 1)){
		}
#else
		send(mosq->sockpairW, &sockpair_data, 1, 0);
#endif
	}
	
#ifdef _WIN32
	if(force){
		SetEvent(mosq->loop_cancel);
	}
	WaitForSingleObject(mosq->thread, INFINITE);
	mosq->thread = GetCurrentThread();
	mosq->threaded = mosq_ts_none;
#else
	if(force){
		pthread_cancel(mosq->thread_id);
	}
	pthread_join(mosq->thread_id, NULL);
	mosq->thread_id = pthread_self();
	mosq->threaded = mosq_ts_none;
#endif

	return MOSQ_ERR_SUCCESS;
#else
	return MOSQ_ERR_NOT_SUPPORTED;
#endif
}

#ifdef WITH_THREADING
#ifdef _WIN32
static unsigned WINAPI _mosquitto_thread_main(void *obj)
#else
static void *_mosquitto_thread_main(void *obj)
#endif
{
	struct mosquitto *mosq = obj;

#ifdef _WIN32
	if(!mosq) return 0;
#else
	if(!mosq) return NULL;
#endif

	_mosquitto_mutex_acquire(&mosq->state_mutex);
	if(mosq->state == mosq_cs_connect_async){
		_mosquitto_mutex_release(&mosq->state_mutex);
		mosquitto_reconnect(mosq);
	}else{
		_mosquitto_mutex_release(&mosq->state_mutex);
	}

	if(!mosq->keepalive){
		/* Sleep for a day if keepalive disabled. */
		mosquitto_loop_forever(mosq, 1000*86400, 1);
	}else{
		/* Sleep for our keepalive value. publish() etc. will wake us up. */
		mosquitto_loop_forever(mosq, mosq->keepalive*1000, 1);
	}

#ifdef _WIN32
	return 0;
#else
	return obj;
#endif
}
#endif

int mosquitto_threaded_set(struct mosquitto *mosq, bool threaded)
{
	if(!mosq) return MOSQ_ERR_INVAL;

	if(threaded){
		mosq->threaded = mosq_ts_external;
	}else{
		mosq->threaded = mosq_ts_none;
	}

	return MOSQ_ERR_SUCCESS;
}
