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

#ifndef __ULIB_PERIODIC_H
#define __ULIB_PERIODIC_H

#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <list>

namespace ulib {

/**
 * us_from - calculates timespec for us microseconds from ts
 * @ts: start timestamp
 * @us: microseconds
 */
static inline timespec us_from(struct timespec ts, uint64_t us)
{
	uint64_t ns = ts.tv_nsec + us * 1000;

	ts.tv_sec += ns / 1000000000u;
	ts.tv_nsec = ns % 1000000000u;
	return ts;
}

/**
 * us_from_now - calculates timespec for us microseconds from now
 * @ts: start timestamp
 * @us: microseconds
 */
static inline timespec us_from_now(uint64_t us)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return us_from(ts, us);
}

/**
 * sec_from_now - calculates timespec for sec seconds from now
 * @ts: start timestamp
 * @us: microseconds
 */
static inline timespec sec_from_now(uint64_t sec)
{
	return us_from_now(sec * 1000000);
}

class periodic {
public:
	typedef void *(*task_func_t) (void *);
	typedef uint64_t taskid_t;

	periodic();

	virtual
	~periodic();

	int
	start();

	void
	stop_and_join();

	// schedule a task to run once.
	// This method is thread-safe, it can be called before/after start()
	// params:
	//   run_time: the specified run time, system will guarantee to run the
	//             routine after this time.
	taskid_t
	schedule(timespec run_time, task_func_t routine, void *arg);

	// schedule a repeated task.
	// This method is thread-safe, it can be called before/after start()
	// params:
	//   run_time: see above
	//   interval: intervals in microsecond between each run of the routine
	taskid_t
	schedule_repeated(timespec run_time, long interval, task_func_t routine, void *arg);

	// unschedule a task
	void
	unschedule(taskid_t tid);

private:
	struct task_t {
		taskid_t    id;
		timespec    next_run_time;
		long        interval;  // less or equal than zero means run once
		task_func_t routine;
		void *      arg;
	};

	static void *
	timer_routine(void *arg);

	static bool
	task_less(const task_t &a, const task_t &b);

	taskid_t
	schedule_task(const task_t &task);

	// the timer thread will run this method.
	void run();

	bool _started; // whether the system thread already started.
	bool _stop;

	std::list<task_t> _tasks;  // list of tasks to be run
	pthread_t         _thread; // all scheduled task will be run on this thread
	pthread_cond_t    _cond;   // used to wake up the timer thread.
	pthread_mutex_t   _mutex;  // protect the _tasks list

	taskid_t          _next_id;
	taskid_t volatile _running_task_id;
};

}  // namespace ulib

#endif  /* __ULIB_PERIODIC_H */
