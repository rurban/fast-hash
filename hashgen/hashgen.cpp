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

#include <vector>
#include <utility>

#include <time.h>
#include <stdint.h>
#include <assert.h>
#include <pthread.h>
#include <ulib/log.h>
#include <ulib/timer.h>
#include <ulib/common.h>
#include <ulib/rdtsc.h>
#include <ulib/hash.h>
#include <ulib/thread.h>
#include <ulib/rand_tpl.h>
#include <ulib/console.h>
#include "avalanche.h"
#include "xxhash.h"

// Feistel structure round
#define FS_ROUND(a, b, F) ({			\
			(a) ^= F(b);		\
			swap(a, b); })

using namespace std;
using namespace ulib;

int volatile   g_aval_len   = 47;
int volatile   g_aval_times = 5000;
float volatile g_time_r     = 1;

class hashgen {
public:
	enum op_type {
		OP_MUL = 0,  // multiplication
		OP_XSL = 1,  // xorshift left
		OP_XSR = 2,  // xorshift right
		OP_ROR = 3,  // rotate right
		OP_NUM       // number of operations
	};

	struct op {
		op_type   type;
		uint64_t  arg;
		hashgen * gen;

		class arg_mutate : public thread {
		public:
			arg_mutate(op *p)
			: _op(p)
			{
				uint64_t now = rdtsc();
				RAND_NR_INIT(_u, _v, _w, now);
			}

			~arg_mutate()
			{ stop_and_join(); }

			int
			run()
			{
				while (is_running()) {
					uint64_t old = _op->arg;
					_op->update(RAND_NR_NEXT(_u, _v, _w));
					if (!_op->gen->evolve()) {
						//ULIB_DEBUG("attempt to evolve with arg:%016llx -> %016llx was cancelled",
						//	   (unsigned long long)old, (unsigned long long)_op->arg);
						_op->arg = old;
					} else {
						ULIB_DEBUG("arg optimized: %016llx -> %016llx",
							   (unsigned long long)old, (unsigned long long)_op->arg);
					}
				}
				return 0;
			}

		private:
			op *_op;
			uint64_t _u, _v, _w;
		} * ctrl;

		void
		update(uint64_t v)
		{
			switch (type) {
			case OP_MUL:
				v |= 1;
				break;
			case OP_XSL: 
			case OP_XSR:
			case OP_ROR:
				v = v % 63 + 1;
				break;
			default:
				ULIB_FATAL("unknown op_type: %d", type);
			}
			arg = v;
		}

		void
		start()
		{
			if (ctrl)
				ctrl->start();
		}

		void
		stop_and_join()
		{
			if (ctrl)
				ctrl->stop_and_join();
		}

		op(op_type t, hashgen *g)
		: type(t), gen(g)
		{
			uint64_t now = rdtsc();
			update(RAND_INT4_MIX64(now));
			ctrl = new arg_mutate(this);
		}

		~op()
		{ delete ctrl; }
	};

	hashgen()
	: _min_seq(3), _max_seq(3),
	  _best_seen_score(-1)  // negative value indicates it is uninitialized
	{
		pthread_mutex_init(&_mutex, NULL);
		ctrl_add_op = new thread_add_op(this);
		ctrl_del_op = new thread_del_op(this);
		ctrl_mod_op = new thread_mod_op(this);
		ctrl_swap_op = new thread_swap_op(this);
	}

	~hashgen()
	{
		delete ctrl_add_op;
		delete ctrl_del_op;
		delete ctrl_mod_op;
		delete ctrl_swap_op;
		for (vector<op *>::iterator it = _op_seq.begin();
		     it != _op_seq.end(); ++it)
			(*it)->stop_and_join();
		for (vector<op *>::iterator it = _op_seq.begin();
		     it != _op_seq.end(); ++it)
			delete (*it);
		pthread_mutex_destroy(&_mutex);
	}

	int
	start()
	{
		ctrl_add_op->start();
		ctrl_del_op->start();
		ctrl_mod_op->start();
		ctrl_swap_op->start();
		return 0;
	}

	void
	lock()
	{ pthread_mutex_lock(&_mutex); }

	void
	unlock()
	{ pthread_mutex_unlock(&_mutex); }

	int
	get_min_seq() const
	{ return _min_seq; }

	int
	get_max_seq() const
	{ return _max_seq; }

	void
	set_min_seq(int min)
	{ _min_seq = min; }

	void
	set_max_seq(int max)
	{ _max_seq = max; }

	void
	add_op(uint64_t rnd)
	{
		uint32_t r1 = (uint32_t)(rnd >> 32);
		uint32_t r2 = (uint32_t) rnd;

		lock();

		if (_op_seq.size() == 0) {
			op *new_op = new op((op_type)(r2 % (uint32_t)OP_NUM), this);
			_op_seq.push_back(new_op);
			new_op->start();
			//ULIB_DEBUG("new op with type=%d was successfully added", new_op->type);
		} else if (_op_seq.size() < (unsigned)_max_seq) {
			op *new_op = new op((op_type)(r2 % (uint32_t)OP_NUM), this);
			uint32_t pos = r1 % (_op_seq.size() + 1);
			_op_seq.insert(_op_seq.begin() + pos, new_op);
			if (!_evolve()) {
				//ULIB_DEBUG("attempt to add new op with type=%d to pos=%u was cancelled", new_op->type, pos);
				_op_seq.erase(_op_seq.begin() + pos);
				delete new_op;
			} else {
				ULIB_DEBUG("new op with type=%d was successfully added to pos=%u", new_op->type, pos);
				new_op->start();
			}
		}

		unlock();
	}

	void
	del_op(uint64_t rnd)
	{
		lock();

		if (_op_seq.size() > (unsigned)_min_seq) {
			uint32_t pos = rnd % _op_seq.size();
			op *tmp = _op_seq[pos];
			_op_seq.erase(_op_seq.begin() + pos);
			if (!_evolve()) {
				//ULIB_DEBUG("attempt to erase op at pos=%u was cancelled", pos);
				_op_seq.insert(_op_seq.begin() + pos, tmp);
			} else {
				ULIB_DEBUG("erased op at pos=%u successfully", pos);
				unlock();
				delete tmp;
				return;
			}
		}

		unlock();
	}

	void
	mod_op(uint64_t rnd)
	{
		lock();

		if (_op_seq.size()) {
			uint32_t pos      = rnd % _op_seq.size();
			op_type  old_type = _op_seq[pos]->type;
			uint64_t old_arg  = _op_seq[pos]->arg;
			_op_seq[pos]->type = (op_type)(RAND_INT_MIX64(rnd) % (unsigned)OP_NUM);
			_op_seq[pos]->update(rnd ^ rdtsc());

			if (!_evolve()) {
				//ULIB_DEBUG("attempt to mod op at pos=%u was cancelled", pos);
				_op_seq[pos]->type = old_type;
				_op_seq[pos]->arg  = old_arg;
			} else {
				ULIB_DEBUG("modified op at pos=%u successfully", pos);
			}
		}

		unlock();
	}

	void
	swap_op(uint64_t rnd)
	{
		lock();

		if (_op_seq.size() > 1) {
			uint32_t pos1     = (rnd >> 32) % _op_seq.size();
			uint32_t pos2     = (uint32_t)rnd % _op_seq.size();
			if (pos1 != pos2) {
				op_type  tmp_type = _op_seq[pos1]->type;
				uint64_t tmp_arg  = _op_seq[pos1]->arg;
				_op_seq[pos1]->type = _op_seq[pos2]->type;
				_op_seq[pos2]->type = tmp_type;
				_op_seq[pos1]->arg  = _op_seq[pos2]->arg;
				_op_seq[pos2]->arg  = tmp_arg;
				if (!_evolve()) {
					//ULIB_DEBUG("attempt to swap pos1=%u and pos2=%u was cancelled",
					//	   pos1, pos2);
					tmp_type = _op_seq[pos1]->type;
					tmp_arg  = _op_seq[pos1]->arg;
					_op_seq[pos1]->type = _op_seq[pos2]->type;
					_op_seq[pos2]->type = tmp_type;
					_op_seq[pos1]->arg  = _op_seq[pos2]->arg;
					_op_seq[pos2]->arg  = tmp_arg;
				} else {
					ULIB_DEBUG("swapped pos1=%u and pos2=%u successfully",
						   pos1, pos2);
				}
			}
		}

		unlock();
	}

	bool
	evolve()
	{
		lock();
		bool ret = _evolve();
		unlock();
		return ret;
	}

	void
	print_best_seen()
	{
		lock();
		_print_best_seen();
		unlock();
	}

	// merkle damgard construction
	// this function is DANGEROUS because it is not protected by the lock
	uint64_t
	hash_value(const void *buf, size_t len)
	{
		const uint64_t   m1 = 0xd36463187cc70d7bULL;
		const uint64_t   m2 = 0xb597d0ceca3f6e07ULL;
		const uint64_t *pos = (const uint64_t *)buf;
		const uint64_t *end = (const uint64_t *)((char *)pos + (len & ~15));
		const unsigned char *pc;
		uint64_t h = len * m2;
		uint64_t v = len;
		uint64_t t = 0;

		while (pos != end) {
			h = (ROR64(h, 33) + *pos++) * m1;
			v = (ROR64(v, 37) + *pos++) * m2;
		}

		if (len & 15) {
			if (len & 8)
				h = (ROR64(h, 33) + *pos++) * m1;

			pc = (const unsigned char*)pos;

			switch (len & 7) {
			case 7: t ^= (uint64_t)pc[6] << 48;
			case 6: t ^= (uint64_t)pc[5] << 40;
			case 5: t ^= (uint64_t)pc[4] << 32;
			case 4: t ^= (uint64_t)pc[3] << 24;
			case 3: t ^= (uint64_t)pc[2] << 16;
			case 2: t ^= (uint64_t)pc[1] << 8;
			case 1: t ^= (uint64_t)pc[0];
				v = (ROR64(v, 37) + *pos++) * m2;
			}
		}
	
		h = (ROR64(h, 33) + v) * m1;

		return _process(h);
	}

/*
	// Feistel Structure Hash Function
	uint64_t
	hash_value(const void *buf, size_t len)
	{
//		const uint64_t   m1 = 0xd36463187cc70d7bULL;
//		const uint64_t   m2 = 0xb597d0ceca3f6e07ULL;
		const uint64_t *pos = (const uint64_t *)buf;
		const uint64_t *end = (const uint64_t *)((char *)pos + (len & ~15));
		const unsigned char *pc;
		uint64_t s = len;
		uint64_t t = len;

		while (pos != end) {
			s ^= *pos++;
			t ^= *pos++;
			FS_ROUND(s, t, _process);
		}

		if (len & 15) {
			if (len & 8)
				s ^= *pos++;

			pc = (const unsigned char*)pos;

			switch (len & 7) {
			case 7: t ^= (uint64_t)pc[6] << 48;
			case 6: t ^= (uint64_t)pc[5] << 40;
			case 5: t ^= (uint64_t)pc[4] << 32;
			case 4: t ^= (uint64_t)pc[3] << 24;
			case 3: t ^= (uint64_t)pc[2] << 16;
			case 2: t ^= (uint64_t)pc[1] << 8;
			case 1: t ^= (uint64_t)pc[0];
			}

			FS_ROUND(s, t, _process);
		}
	
		FS_ROUND(s, t, _process);
		RAND_INT4_MIX64(t);

		return t;
	}
*/

	// global instance of this class, should be unique
	static hashgen *instance;

	static uint64_t
	gen_hash(const void *buf, size_t len)
	{ return instance->hash_value(buf, len); }

private:

	uint64_t
	_process(uint64_t init)
	{
		for (vector<op *>::const_iterator it = _op_seq.begin();
		     it != _op_seq.end(); ++it) {
			switch ((*it)->type) {
			case OP_MUL:
				init *= (*it)->arg;
				break;
			case OP_XSL:
				init ^= init << (*it)->arg;
				break;
			case OP_XSR:
				init ^= init >> (*it)->arg;
				break;
			case OP_ROR:
				init ^= ROR64(init, (*it)->arg);
				break;
			default:
				ULIB_FATAL("unknown op type:%d", (*it)->type);
			}
		}
		return init;
	}

	bool
	_evolve()
	{
		avalanche aval;
		timespec timer;
		float time_score;
		bool ret = true;

		timer_start(&timer);
		if (_best_seen_score < 0) {
			// first time
			_update_best_seen();
			_best_seen_score = aval(gen_hash, g_aval_len, g_aval_times);
			time_score = timer_stop(&timer) * g_time_r;
			_best_seen_score += time_score;
			printf("Updated best seen score: aval_score=%f, time_score=%f, overall=%f\n",
			       _best_seen_score - time_score, time_score, _best_seen_score);
		} else {
			float new_score = aval(gen_hash, g_aval_len, g_aval_times);
			time_score = timer_stop(&timer) * g_time_r;
			new_score += time_score;
			if (new_score < _best_seen_score) {
				// new score is better
				_update_best_seen();
				_best_seen_score = new_score;
				printf("Updated best seen score: aval_score=%f, time_score=%f, overall=%f\n",
				       _best_seen_score - time_score, time_score, _best_seen_score);
			} else
				ret = false;
		}

		return ret;
	}

	void
	_print_best_seen()
	{
		printf("Best seen combination:");
		for (vector< pair<op_type,uint64_t> >::const_iterator it = _best_seen.begin();
		     it != _best_seen.end(); ++it) {
			switch (it->first) {
			case OP_MUL:
				printf("MUL(%016llx) ", (unsigned long long)it->second);
				break;
			case OP_XSL:
				printf("XSL(%u) ", (unsigned)it->second);
				break;
			case OP_XSR:
				printf("XSR(%u) ", (unsigned)it->second);
				break;
			case OP_ROR:
				printf("ROR(%u) ", (unsigned)it->second);
				break;
			default:
				printf("UNKNOWN ");
			}
		}
		printf("\t%f\n", _best_seen_score);
	}

	void
	_update_best_seen()
	{
		_best_seen.clear();
		for (vector<op *>::const_iterator it = _op_seq.begin();
		     it != _op_seq.end(); ++it) {
			pair<op_type,uint64_t> t;
			t.first = (*it)->type;
			t.second = (*it)->arg;
			_best_seen.push_back(t);
		}
		_print_best_seen();
	}

	class thread_add_op : public thread {
	public:
		thread_add_op(hashgen *gen)
		: _gen(gen)
		{
			uint64_t now = rdtsc();
			RAND_NR_INIT(_u, _v, _w, now);
		}

		~thread_add_op()
		{ stop_and_join(); }

		int
		run()
		{
			while (is_running())
				_gen->add_op(RAND_NR_NEXT(_u, _v, _w));
			return 0;
		}

	private:
		hashgen *_gen;
		uint64_t _u, _v, _w;
	} *ctrl_add_op;

	class thread_del_op : public thread {
	public:
		thread_del_op(hashgen *gen)
		: _gen(gen)
		{
			uint64_t now = rdtsc();
			RAND_NR_INIT(_u, _v, _w, now);
		}

		~thread_del_op()
		{ stop_and_join(); }

		int
		run()
		{
			while (is_running())
				_gen->del_op(RAND_NR_NEXT(_u, _v, _w));
			return 0;
		}

	private:
		hashgen *_gen;
		uint64_t _u, _v, _w;
	} *ctrl_del_op;

	class thread_mod_op : public thread {
	public:
		thread_mod_op(hashgen *gen)
		: _gen(gen)
		{
			uint64_t now = rdtsc();
			RAND_NR_INIT(_u, _v, _w, now);
		}

		~thread_mod_op()
		{ stop_and_join(); }

		int
		run()
		{
			while (is_running())
				_gen->mod_op(RAND_NR_NEXT(_u, _v, _w));
			return 0;
		}

	private:
		hashgen *_gen;
		uint64_t _u, _v, _w;
	} *ctrl_mod_op;

	class thread_swap_op : public thread {
	public:
		thread_swap_op(hashgen *gen)
		: _gen(gen)
		{
			uint64_t now = rdtsc();
			RAND_NR_INIT(_u, _v, _w, now);
		}

		~thread_swap_op()
		{ stop_and_join(); }

		int
		run()
		{
			while (is_running())
				_gen->swap_op(RAND_NR_NEXT(_u, _v, _w));
			return 0;
		}

	private:
		hashgen *_gen;
		uint64_t _u, _v, _w;
	} *ctrl_swap_op;

	int volatile _min_seq;
	int volatile _max_seq;

	pthread_mutex_t _mutex;
	vector<op *>    _op_seq;
	float volatile  _best_seen_score;
	vector< pair<op_type,uint64_t> > _best_seen;   // best seen result
};

hashgen *hashgen::instance = NULL;

int cmd_start(int, const char **)
{
	if (hashgen::instance == NULL) {
		ULIB_FATAL("instance is NULL");
		return -1;
	}
	hashgen::instance->start();
	return 0;
}

int cmd_aval_rate(int argc, const char *argv[])
{
	if (argc > 1)
		g_aval_r = atof(argv[1]);
	printf("%f\n", g_aval_r);
	return 0;
}

int cmd_indep_rate(int argc, const char *argv[])
{
	if (argc > 1)
		g_indep_r = atof(argv[1]);
	printf("%f\n", g_indep_r);
	return 0;
}

int cmd_time_rate(int argc, const char *argv[])
{
	if (argc > 1)
		g_time_r = atof(argv[1]);
	printf("%f\n", g_time_r);
	return 0;
}

int cmd_aval_byte(int argc, const char *argv[])
{
	if (argc > 1)
		g_aval_len = atoi(argv[1]);
	printf("%d\n", g_aval_len);
	return 0;
}

int cmd_aval_times(int argc, const char *argv[])
{
	if (argc > 1)
		g_aval_times = atoi(argv[1]);
	printf("%d\n", g_aval_times);
	return 0;
}

int cmd_min_seq(int argc, const char *argv[])
{
	if (hashgen::instance == NULL) {
		ULIB_FATAL("instance is NULL");
		return -1;
	}
	if (argc > 1)
		hashgen::instance->set_min_seq(atoi(argv[1]));
	printf("%d\n", hashgen::instance->get_min_seq());
	return 0;
}

int cmd_max_seq(int argc, const char *argv[])
{
	if (hashgen::instance == NULL) {
		ULIB_FATAL("instance is NULL");
		return -1;
	}
	if (argc > 1)
		hashgen::instance->set_max_seq(atoi(argv[1]));
	printf("%d\n", hashgen::instance->get_max_seq());
	return 0;
}

int cmd_best_seen(int, const char **)
{
	if (hashgen::instance == NULL) {
		ULIB_FATAL("instance is NULL");
		return -1;
	}
	hashgen::instance->print_best_seen();
	return 0;
}

static uint64_t hash_xxhash_noseed(const void *buf, size_t len)
{
	uint64_t low, high;
	low  = XXH_fast32(buf, len, 0);
	high = XXH_fast32(buf, len, 1);
	return low | (high << 32);
}

static uint64_t hash_jenkins_noseed(const void *buf, size_t len)
{
	uint64_t hash = 0x0000000100000001ULL;
	uint32_t *ph  = (uint32_t *)&hash;
	hash_jenkins2(buf, len, ph, ph + 1);
	return hash;
}

int cmd_standard(int, const char **)
{
	avalanche aval;
	timespec timer;
	float ascore, tscore;

	timer_start(&timer);
	ascore = aval(hash_jenkins_noseed, g_aval_len, g_aval_times);
	tscore = timer_stop(&timer) * g_time_r;
	printf("JenkinsHash: aval_score=%f, time_score=%f, overall=%f\n",
	       ascore, tscore, ascore + tscore);
	timer_start(&timer);
	ascore = aval(hash_xxhash_noseed, g_aval_len, g_aval_times);
	tscore = timer_stop(&timer) * g_time_r;
	printf("XXHash     : aval_score=%f, time_score=%f, overall=%f\n",
	       ascore, tscore, ascore + tscore);

	return 0;
}

int cmd_help(int, const char **)
{
	printf("Basic commands:\n"
	       "start        -- start generation\n"
	       "std          -- see scores of some famous hash function\n"
	       "help         -- print this message\n"
	       "exit         -- exit program\n"
	       "\nSystem parameters:\n"
	       "min_seq      -- minimum sequence length\n"
	       "max_seq      -- maximum sequence length\n"
	       "aval_byte    -- buffer length for hash test\n"
	       "aval_times   -- sample size\n"
	       "best_seen    -- print best seen result so far\n"
	       "\nFitness parameters:\n"
	       "aval_rate    -- rate of avalanche score\n"
	       "indep_rate   -- rate of independence test score\n"
	       "time_rate    -- rate of speed score\n");
	return 0;
}

int main()
{
	hashgen::instance = new hashgen;
	console_t con;

	printf("Non-cryptographic Hash Function Generator 1.0 alpha\n");
	printf("Zilong Tan (eric.zltan@gmail.com)\n");
	printf("Type \'help\' for a list of commands; \'exit\' to quit.\n");

	assert(console_init(&con) == 0);
	assert(console_bind(&con, "start", cmd_start) == 0);
	assert(console_bind(&con, "aval_rate", cmd_aval_rate) == 0);
	assert(console_bind(&con, "indep_rate", cmd_indep_rate) == 0);
	assert(console_bind(&con, "time_rate", cmd_time_rate) == 0);
	assert(console_bind(&con, "aval_byte", cmd_aval_byte) == 0);
	assert(console_bind(&con, "aval_times", cmd_aval_times) == 0);
	assert(console_bind(&con, "min_seq", cmd_min_seq) == 0);
	assert(console_bind(&con, "max_seq", cmd_max_seq) == 0);
	assert(console_bind(&con, "best_seen", cmd_best_seen) == 0);
	assert(console_bind(&con, "std", cmd_standard) == 0);
	assert(console_bind(&con, "help", cmd_help) == 0);
	console_loop(&con, -1, "exit");
	console_destroy(&con);

	printf("\nExiting Now ...\n\n");

	delete hashgen::instance;

	return 0;
}
