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

#ifndef __ULIB_THREAD_H
#define __ULIB_THREAD_H

#include <pthread.h>

namespace ulib {

class thread {
public:
	thread()
	: _running(false) { }

	int
	start()
	{
		if (_running)
			return 0;
		_running = true;
		if (pthread_create(&_tid, NULL, _thread, (void *)this)) {
			_running = false;
			return -1;
		}
		return 0;
	}

	int
	join()
	{
		if (_running) {
			if (pthread_join(_tid, NULL))
				return -1;
			_running = false;
		}
		return 0;
	}

	// the difference between join() and stop_and_join() is that
	// the later changes the status flag to tell the thread
	// routine to voluntarily exit.
	int
	stop_and_join()
	{
		if (!_running)
			return 0;
		_running = false;
		if (pthread_join(_tid, NULL)) {
			_running = true;
			return -1;
		}
		return 0;
	}

	void
	set_state(bool started)
	{ _running = started; }

	bool
	is_running() const
	{ return _running; }

	// ATTENTION: inherented classes must implement a custom
	// destructor function and call join there.
	virtual
	~thread()
	{ stop_and_join(); }

	// optionally performs initialization for thread
	virtual int
	before_run()
	{ return 0; }

	// thread routine
	virtual int
	run() = 0;

	// optionally performs clean up for thread
	virtual int
	after_run()
	{ return 0; }

private:
	pthread_t     _tid;
	volatile bool _running;
	static void * _thread(void *param);
};

void *thread::_thread(void *param)
{
	thread *parent = (thread *) param;

	int ret = parent->before_run();
	if (ret) {
		// if initialization fails, we won't proceed
		return (void *)(unsigned long)-1;
	}
	ret = parent->run();
	ret = parent->after_run();
	return NULL;
}

}  // namespace ulib

#endif  /* __ULIB_THREAD_H */
