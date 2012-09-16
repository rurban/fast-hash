/* The MIT License

   Copyright (C) 2012 Zilong Tan (eric.zltan@gmail.com)

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without
   restriction, including without limitation the rights to use, copy,
   modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#include <algorithm>
#include "periodic.h"

namespace ulib {

static bool timespec_less(const timespec &a, const timespec &b)
{
	if (a.tv_sec != b.tv_sec)
		return a.tv_sec < b.tv_sec;
	else
		return a.tv_nsec < b.tv_nsec;
}

bool periodic::task_less(const periodic::task_t &a, const periodic::task_t &b)
{
	return timespec_less(a.next_run_time, b.next_run_time);
}

void *periodic::timer_routine(void *arg)
{
	periodic *timer_thread = (periodic *) arg;
	timer_thread->run();
	return NULL;
}

periodic::periodic()
	: _started(false), _stop(false),
	  _next_id(0), _running_task_id(0)
{
	pthread_mutex_init(&_mutex, NULL);
	pthread_cond_init(&_cond, NULL);
}

periodic::~periodic()
{
	if (_started && !_stop)
		stop_and_join();
	pthread_mutex_destroy(&_mutex);
	pthread_cond_destroy(&_cond);
}

int periodic::start()
{
	if (_started)
		return false;

	if (!pthread_create(&_thread, NULL, &periodic::timer_routine, this)) {
		_started = true;
		return 0;
	}
	return -1;
}

inline periodic::taskid_t periodic::schedule_task(const task_t &task)
{
	pthread_mutex_lock(&_mutex);

	// NOTE: here use upper_bound instead of lower_bound to avoid starving.
	std::list<task_t>::iterator it = 
		upper_bound(_tasks.begin(), _tasks.end(), task, task_less);
	// If the new task become the next task, then wake up the timer thread to
	// recalculate sleeping time.
	bool should_wake_up = (it == _tasks.begin());

	// ++_next_id, so _task_id will start with 1, since we use 0 for special
	// purpose.
	// NOTE: currently we don't check overflow of _next_id, since we are using
	// uint64, should be big enough.
	taskid_t task_id = ++_next_id;
	_tasks.insert(it, task)->id = task_id;

	if (should_wake_up)
		pthread_cond_signal(&_cond);
	pthread_mutex_unlock(&_mutex);
	return task_id;
}

periodic::taskid_t periodic::schedule(timespec run_time, task_func_t routine, void *arg)
{
	task_t task;
	task.next_run_time = run_time;
	task.interval = 0;  // only run once
	task.routine = routine;
	task.arg = arg;
	return schedule_task(task);
}

periodic::taskid_t periodic::schedule_repeated(timespec run_time, long interval,
					       task_func_t routine, void *arg)
{
	task_t task;
	task.next_run_time = run_time;
	task.interval = interval;
	task.routine = routine;
	task.arg = arg;
	return schedule_task(task);
}

void periodic::unschedule(taskid_t task_id)
{
	pthread_mutex_lock(&_mutex);
	if (_running_task_id == task_id) {
		// current running task is not in the list, so special treatment
		_running_task_id = 0;
	} else {
		for (std::list <task_t>::iterator it = _tasks.begin(); it != _tasks.end(); ++it) {
			if (it->id == task_id) {
				_tasks.erase(it);
				break;
			}
		}
	}
	pthread_mutex_unlock(&_mutex);
}

void periodic::run()
{
	bool lock_hold = false;
	while (!_stop) {
		if (!lock_hold) {
			pthread_mutex_lock(&_mutex);
			lock_hold = true;
		}
		if (_tasks.empty()) {
			// no task to run, sleep
			pthread_cond_wait(&_cond, &_mutex);
			continue;
		}
		timespec current_time;
		clock_gettime(CLOCK_REALTIME, &current_time);
		const task_t & task = _tasks.front();
		if (!timespec_less(current_time, task.next_run_time)) {
			// ok to run the first task, remove it from the list.
			task_t to_run = _tasks.front();
			_tasks.pop_front();
			_running_task_id = to_run.id;
			// unlock it, so when running the routine, the lock is open.
			pthread_mutex_unlock(&_mutex);
			lock_hold = false;
			to_run.routine(to_run.arg);
			if (to_run.interval > 0) {
				// this is a repeated task, calculate its next running time.
				to_run.next_run_time = us_from_now(to_run.interval);

				pthread_mutex_lock(&_mutex);
				lock_hold = true;
				if (_running_task_id != 0) {
					// _running_task_id != 0 means the task hasn't been
					// unscheduled.
					_running_task_id = 0;
					std::list <task_t>::iterator it =
						upper_bound(_tasks.begin(), _tasks.end(), to_run, task_less);
					_tasks.insert(it, to_run);
				} else {
					// this task has been unschedule while it is running
					// skip adding it back.
				}
			}
			continue;
		} else {
			// need sleep more until the time is ready.
			pthread_cond_timedwait(&_cond, &_mutex, &task.next_run_time);
			continue;
		}
	}
	if (lock_hold)
		pthread_mutex_unlock(&_mutex);
}

void periodic::stop_and_join()
{
	if (pthread_self() == _thread) {
		// stop_and_join was called from a running task.
		_stop = true;
	} else {
		pthread_mutex_lock(&_mutex);
		_stop = true;
		// wake up the timer thread in case it is sleeping.
		pthread_cond_signal(&_cond);
		pthread_mutex_unlock(&_mutex);
		if (_started) {
			pthread_join(_thread, NULL);
		}
	}
}

}  // end namespace
