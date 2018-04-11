/*
 * https://github.com/matricali/threadpool
 *
 * Copyright (c) 2018 Jorge Matricali.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "threadpool.h"

static threadpool_work_t *threadpool_work_create(thread_func_t func, void *arg)
{
	threadpool_work_t *work;

	if (func == NULL)
		return NULL;

	work = malloc(sizeof(*work));
	work->func = func;
	work->arg = arg;
	work->next = NULL;
	return work;
}

static void threadpool_work_destroy(threadpool_work_t *work)
{
	if (work == NULL)
		return;
	free(work);
}

static threadpool_work_t *threadpool_work_get(threadpool_t *tp)
{
	threadpool_work_t *work;

	if (tp == NULL)
		return NULL;

	work = tp->work_first;
	if (work == NULL)
		return NULL;

	if (work->next == NULL) {
		tp->work_first = NULL;
		tp->work_last = NULL;
	} else {
		tp->work_first = work->next;
	}

	return work;
}

static void *threadpool_worker(void *arg)
{
	threadpool_t *tp = arg;
	threadpool_work_t *work;

	for (;;) {
		pthread_mutex_lock(&(tp->work_mutex));
		if (tp->stop)
			break;

		if (tp->work_first == NULL)
			pthread_cond_wait(&(tp->work_cond), &(tp->work_mutex));

		work = threadpool_work_get(tp);
		tp->working_count++;
		pthread_mutex_unlock(&(tp->work_mutex));

		if (work != NULL) {
			work->func(work->arg);
			threadpool_work_destroy(work);
		}

		pthread_mutex_lock(&(tp->work_mutex));
		tp->working_count--;
		if (!tp->stop && tp->working_count == 0 &&
		    tp->work_first == NULL)
			pthread_cond_signal(&(tp->working_cond));
		pthread_mutex_unlock(&(tp->work_mutex));
	}

	tp->thread_count--;
	pthread_cond_signal(&(tp->working_cond));
	pthread_mutex_unlock(&(tp->work_mutex));
	return NULL;
}

threadpool_t *threadpool_create(size_t num)
{
	threadpool_t *tp;
	pthread_t thread;
	size_t i;

	if (num == 0)
		num = 2;

	tp = calloc(1, sizeof(*tp));
	tp->thread_count = num;

	pthread_mutex_init(&(tp->work_mutex), NULL);
	pthread_cond_init(&(tp->work_cond), NULL);
	pthread_cond_init(&(tp->working_cond), NULL);

	tp->work_first = NULL;
	tp->work_last = NULL;

	for (i = 0; i < num; i++) {
		pthread_create(&thread, NULL, threadpool_worker, tp);
		pthread_detach(thread);
	}

	return tp;
}

void threadpool_destroy(threadpool_t *tp)
{
	threadpool_work_t *work;
	threadpool_work_t *work2;

	if (tp == NULL)
		return;

	pthread_mutex_lock(&(tp->work_mutex));
	work = tp->work_first;
	while (work != NULL) {
		work2 = work->next;
		threadpool_work_destroy(work);
		work = work2;
	}
	tp->stop = true;
	pthread_cond_broadcast(&(tp->work_cond));
	pthread_mutex_unlock(&(tp->work_mutex));

	threadpool_wait(tp);

	pthread_mutex_destroy(&(tp->work_mutex));
	pthread_cond_destroy(&(tp->work_cond));
	pthread_cond_destroy(&(tp->working_cond));

	free(tp);
}

bool threadpool_add_work(threadpool_t *tp, thread_func_t func, void *arg)
{
	threadpool_work_t *work;

	if (tp == NULL)
		return false;

	work = threadpool_work_create(func, arg);
	if (work == NULL)
		return false;

	pthread_mutex_lock(&(tp->work_mutex));
	if (tp->work_first == NULL) {
		tp->work_first = work;
		tp->work_last = tp->work_first;
	} else {
		tp->work_last->next = work;
		tp->work_last = work;
	}

	pthread_cond_broadcast(&(tp->work_cond));
	pthread_mutex_unlock(&(tp->work_mutex));

	return true;
}

void threadpool_wait(threadpool_t *tp)
{
	if (tp == NULL)
		return;

	pthread_mutex_lock(&(tp->work_mutex));
	for (;;) {
		if ((!tp->stop && tp->working_count != 0) ||
		    (tp->stop && tp->thread_count != 0)) {
			pthread_cond_wait(&(tp->working_cond),
					  &(tp->work_mutex));
		} else {
			break;
		}
	}
	pthread_mutex_unlock(&(tp->work_mutex));
}
