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

#ifndef MUTEX_MOSQ_H
#define MUTEX_MOSQ_H

#if defined(WITH_THREADING) && !defined(WITH_BROKER)
  #ifdef _WIN32
	typedef struct mosq_mutex_t {
	  void *ptr;
	} mosq_mutex_t;
	void _mosquitto_mutex_lib_init();
	int _mosquitto_mutex_init(mosq_mutex_t *mutex);
	int _mosquitto_mutex_destroy(mosq_mutex_t *mutex);
	int _mosquitto_mutex_acquire(mosq_mutex_t *mutex);
	int _mosquitto_mutex_release(mosq_mutex_t *mutex);
	int _mosquitto_mutex_acquire_shared(mosq_mutex_t *mutex);
	int _mosquitto_mutex_release_shared(mosq_mutex_t *mutex);
  #else
	#include <pthread.h>
	typedef pthread_mutex_t mosq_mutex_t;
	#define _mosquitto_mutex_lib_init()
	#define _mosquitto_mutex_init(M) pthread_mutex_init((M), NULL);
	#define _mosquitto_mutex_destroy(M) pthread_mutex_destroy((M));
	#define _mosquitto_mutex_acquire(M) pthread_mutex_lock((M));
	#define _mosquitto_mutex_release(M) pthread_mutex_unlock((M));
	#define _mosquitto_mutex_acquire_shared(M) pthread_mutex_lock((M));
	#define _mosquitto_mutex_release_shared(M) pthread_mutex_unlock((M));
  #endif
#else
  #define _mosquitto_mutex_init(M)
  #define _mosquitto_mutex_destroy(M)
  #define _mosquitto_mutex_acquire(M)
  #define _mosquitto_mutex_release(M)
  #define _mosquitto_mutex_acquire_shared(M)
  #define _mosquitto_mutex_release_shared(M)
#endif

#endif
