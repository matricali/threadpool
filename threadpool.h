/*
 * This file is part of the buscatukas distribution
 * (https://github.com/matricali/buscatukas).
 *
 * Copyright (c) 2019 Jorge Matricali.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __THREADPOOL_THREADPOOL_H
#define __THREADPOOL_THREADPOOL_H

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

typedef void (*thread_func_t)(void *arg);

typedef struct threadpool_work {
	thread_func_t func;
	void *arg;
	struct threadpool_work *next;
} threadpool_work_t;

typedef struct tpool {
	threadpool_work_t *work_first;
	threadpool_work_t *work_last;
	pthread_mutex_t work_mutex;
	pthread_cond_t work_cond;
	pthread_cond_t working_cond;
	size_t working_count;
	size_t thread_count;
	bool stop;
} threadpool_t;

threadpool_t *threadpool_create(size_t size);

void threadpool_destroy(threadpool_t *tp);

bool threadpool_add_work(threadpool_t *tp, thread_func_t func, void *arg);

void threadpool_wait(threadpool_t *tp);

#endif /* __THREADPOOL_THREADPOOL_H */
