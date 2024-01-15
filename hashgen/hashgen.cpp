/* The MIT License


   Copyright (C) 2012 Zilong Tan (eric.zltan@gmail.com)
   Copyright (C) 2024 Reini Urban (reini.urban@gmail.com)

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

#include <cstring>
#include <utility>
#include <vector>

#include "avalanche.h"
#include "../fasthash.h"
#include "xxhash.h"
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <ulib/common.h>
#include <ulib/console.h>
#include <ulib/hash.h>
#include <ulib/log.h>
#include <ulib/rand_tpl.h>
#include <ulib/rdtsc.h>
#include <ulib/thread.h>
#include <ulib/timer.h>

#define unlikely(x) __builtin_expect(!!(x), 0)

// Feistel structure round
#define FS_ROUND(a, b, F)                                                      \
  ({                                                                           \
    (a) ^= F(b);                                                               \
    swap(a, b);                                                                \
  })
#define ARR_SIZE(x) sizeof(x)/sizeof(x[0])
#define RAND_PICK(arr,_u,_v,_w) arr[RAND_NR_NEXT(_u, _v, _w) % ARR_SIZE(arr)]

using namespace std;
using namespace ulib;

int volatile g_aval_len = 47;
int volatile g_aval_times = 5000;
float volatile g_time_r = 1;
#define BUF_SIZE 32

// see also https://github.com/skeeto/hash-prospector
// which is optimized for 32bit and adds a bias score.
class hashgen {
public:
  enum op_type {
    // | and & are not-reversible. mul by even neither.
    OP_MUL = 0, // multiplication
    OP_XSL = 1, // xorshift left
    OP_XSR = 2, // xorshift right
    OP_ROR = 3, // rotate right
    // worse ops:
    OP_ADD = 4,  // add
    OP_XOR = 5,  // xor
    OP_NOT = 6,  // ~
    OP_SWP = 7,  // swap
    OP_ASL = 8,  // addshift left
    OP_SSL = 9,  // subshift left
    OP_SUB = 10,  // sub
    OP_LOR = 11, // rotate left
    OP_XQO = 12, // xorsquare
    OP_NUM       // number of operations
  };

  struct op {
    op_type type;
    uint64_t arg;
    hashgen *gen;

    class arg_mutate : public thread {
    public:
      arg_mutate(op *p) : _op(p) {
        uint64_t now = rdtsc();
        RAND_NR_INIT(_u, _v, _w, now);
      }

      ~arg_mutate() { stop_and_join(); }

      int run() {
        while (is_running()) {
          uint64_t old = _op->arg;
          if (_op->type == OP_MUL) {
            // some known good mult constants from other hashes
            const uint64_t mul_constants[6] = {
              UINT64_C(0x2127599bf4325c37),
              UINT64_C(0xbf58476d1ce4e5b9),
              UINT64_C(0x94d049bb133111eb),
              UINT64_C(0x9743d1e18d4481c7),
              UINT64_C(0xe4adbc73edb87283),
              UINT64_C(0xff51afd7ed558ccd)
            };
            // pick from some changed fixed constants
            do {
              _op->arg = RAND_PICK(mul_constants, _u, _v, _w);
            } while (_op->arg == old);  
          }
          else {
            _op->update(RAND_NR_NEXT(_u, _v, _w));
          }
          if (!_op->gen->evolve()) {
            ULIB_DEBUG("attempt to evolve with arg:%016llx -> %016llx was "
                       "cancelled",
                       (unsigned long long)old, (unsigned long long)_op->arg);
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

    void update(uint64_t v) {
      switch (type) {
      case OP_ADD: // by 0 makes not much sense
      case OP_SUB:
      case OP_XQO:
        v = v ? v : 1;
        break;
      case OP_XSL: // limited to 64 bits
      case OP_XSR:
      case OP_ROR:
      case OP_XOR:
      case OP_ASL:
      case OP_SSL:
      case OP_LOR:
        v = v % 63 + 1;
        break;
      case OP_MUL: // ignored, picked from constants
      case OP_NOT: // no args
      case OP_SWP:
        break;
      case OP_NUM:
        ULIB_FATAL("unknown op_type: %d", type);
      }
      arg = v;
    }

    void start() {
      if (ctrl)
        ctrl->start();
    }

    void stop_and_join() {
      if (ctrl)
        ctrl->stop_and_join();
    }

    op(op_type t, hashgen *g) : type(t), gen(g) {
      uint64_t now = rdtsc();
      update(RAND_INT4_MIX64(now));
      ctrl = new arg_mutate(this);
    }

    ~op() { delete ctrl; }
  };

  hashgen()
      : _min_seq(2), _max_seq(6), // murmur has 5, rrmxmx has 6
        _best_seen_score(-1)      // negative value for uninitialized
  {
    pthread_mutex_init(&_mutex, NULL);
    ctrl_add_op = new thread_add_op(this);
    ctrl_del_op = new thread_del_op(this);
    ctrl_mod_op = new thread_mod_op(this);
    ctrl_swap_op = new thread_swap_op(this);
  }

  ~hashgen() {
    delete ctrl_add_op;
    delete ctrl_del_op;
    delete ctrl_mod_op;
    delete ctrl_swap_op;
    for (vector<op *>::iterator it = _op_seq.begin(); it != _op_seq.end(); ++it)
      (*it)->stop_and_join();
    for (vector<op *>::iterator it = _op_seq.begin(); it != _op_seq.end(); ++it)
      delete (*it);
    pthread_mutex_destroy(&_mutex);
  }

  int start() {
    ctrl_add_op->start();
    ctrl_del_op->start();
    ctrl_mod_op->start();
    ctrl_swap_op->start();
    return 0;
  }

  void lock() { pthread_mutex_lock(&_mutex); }

  void unlock() { pthread_mutex_unlock(&_mutex); }

  int get_min_seq() const { return _min_seq; }

  int get_max_seq() const { return _max_seq; }

  void set_min_seq(int min) { _min_seq = min; }

  void set_max_seq(int max) { _max_seq = max; }

  // may these ops be adjacent?
  int adjacent(enum op_type a, enum op_type b) {
    switch (a) {
    case OP_XQO:
    case OP_LOR:
    case OP_NOT:
    case OP_SWP:
    case OP_ADD:
    case OP_SUB:
    case OP_XOR:
    case OP_ROR:
    case OP_MUL:
      return a != b;
    case OP_XSL:
    case OP_XSR:
    case OP_ASL:
    case OP_SSL:
      return 1;
    case OP_NUM:
      ULIB_FATAL("unknown op_type: %d", a);
    }
    return 0;
  }

  void add_op(uint64_t rnd) {
    uint32_t r1 = (uint32_t)(rnd >> 32);
    uint32_t r2 = (uint32_t)rnd;

    lock();

    if (_op_seq.size() == 0) {
      op *new_op = new op((op_type)(r2 % (uint32_t)OP_NUM), this);
      while (new_op->type == OP_ADD) // don't start with the worst op
        new_op = new op((op_type)(r2 % (uint32_t)OP_NUM), this);
      _op_seq.push_back(new_op);
      new_op->start();
#ifndef UNDEBUG
      char buf[BUF_SIZE];
      pair<op_type, uint64_t> it = {new_op->type, new_op->arg};
      ULIB_DEBUG("new op %s added", _print_op(it, buf));
#endif
    } else if (_op_seq.size() < (unsigned)_max_seq) {
      op *new_op = new op((op_type)(r2 % (uint32_t)OP_NUM), this);
      uint32_t pos = r1 % (_op_seq.size() + 1);
      op *next_op = pos < _op_seq.size() ? _op_seq[pos] : _op_seq[0];
      while (!adjacent(new_op->type, next_op->type)) {
#ifndef UNDEBUG
        char buf1[BUF_SIZE];
        char buf2[BUF_SIZE];
        pair<op_type, uint64_t> p1 = {new_op->type, new_op->arg};
        pair<op_type, uint64_t> p2 = {next_op->type, next_op->arg};
        ULIB_DEBUG("new op %s adjacent to %s cancelled", _print_op(p1, buf1),
                   _print_op(p2, buf2));
#endif
        new_op = new op((op_type)(r2 % (uint32_t)OP_NUM), this);
        pos = r1 % (_op_seq.size() + 1);
        next_op = pos < _op_seq.size() ? _op_seq[pos] : _op_seq[0];
      }
      // before
      vector<op *>::iterator it = _op_seq.begin() + pos;
      _op_seq.insert(it, new_op);
      if (!_evolve()) {
#ifndef UNDEBUG
        char buf[BUF_SIZE];
        pair<op_type, uint64_t> p = {new_op->type, new_op->arg};
        ULIB_DEBUG("attempt to add new op %s to pos=%u was"
                   "cancelled",
                   _print_op(p, buf), pos);
#endif
        _op_seq.erase(it);
        delete new_op;
      } else {
#ifndef UNDEBUG
        char buf[BUF_SIZE];
        pair<op_type, uint64_t> p = {new_op->type, new_op->arg};
        ULIB_DEBUG("new op %s added to pos=%u", _print_op(p, buf), pos);
#endif
        new_op->start();
      }
    }

    unlock();
  }

  void del_op(uint64_t rnd) {
    lock();

    if (_op_seq.size() > (unsigned)_max_seq) {
      uint32_t pos = rnd % _op_seq.size();
      op *tmp = _op_seq[pos];
      op *tmp1 = pos < _op_seq.size() - 1 ? _op_seq[pos + 1] : _op_seq[0];
      while (!adjacent(tmp->type, tmp1->type)) {
#ifndef UNDEBUG
        char buf1[BUF_SIZE];
        char buf2[BUF_SIZE];
        pair<op_type, uint64_t> p1 = {tmp->type, tmp->arg};
        pair<op_type, uint64_t> p2 = {tmp1->type, tmp1->arg};
        ULIB_DEBUG("new op %s adjacent to %s cancelled", _print_op(p1, buf1),
                   _print_op(p2, buf2));
#endif
        pos = rnd % _op_seq.size();
        tmp = _op_seq[pos];
      }
      _op_seq.erase(_op_seq.begin() + pos);
      if (!_evolve()) {
#ifndef UNDEBUG
        char buf[BUF_SIZE];
        pair<op_type, uint64_t> it = {tmp->type, tmp->arg};
        ULIB_DEBUG("attempt to erase op %s at pos=%u was cancelled",
                   _print_op(it, buf), pos);
#endif
        _op_seq.insert(_op_seq.begin() + pos, tmp);
      } else {
#ifndef UNDEBUG
        char buf[BUF_SIZE];
        pair<op_type, uint64_t> it = {tmp->type, tmp->arg};
        ULIB_DEBUG("erased op %s at pos=%u", _print_op(it, buf), pos);
#endif
        unlock();
        delete tmp;
        return;
      }
    }

    unlock();
  }

  void mod_op(uint64_t rnd) {
    lock();

    if (_op_seq.size()) {
      uint32_t pos = rnd % _op_seq.size();
      op_type old_type = _op_seq[pos]->type;
      uint64_t old_arg = _op_seq[pos]->arg;
      _op_seq[pos]->type = (op_type)(RAND_INT_MIX64(rnd) % (unsigned)OP_NUM);
      _op_seq[pos]->update(rnd ^ rdtsc());

      if (!_evolve()) {
#ifndef UNDEBUG
        char buf[BUF_SIZE];
        pair<op_type, uint64_t> it = {_op_seq[pos]->type, _op_seq[pos]->arg};
        ULIB_DEBUG("attempt to mod op %s at pos=%u was cancelled",
                   _print_op(it, buf), pos);
#endif
        _op_seq[pos]->type = old_type;
        _op_seq[pos]->arg = old_arg;
      } else {
#ifndef UNDEBUG
        char buf[BUF_SIZE];
        pair<op_type, uint64_t> it = {_op_seq[pos]->type, _op_seq[pos]->arg};
        ULIB_DEBUG("modified op %s at pos=%u", _print_op(it, buf), pos);
#endif
      }
    }

    unlock();
  }

  void swap_op(uint64_t rnd) {
    lock();

    if (_op_seq.size() > 1) {
      uint32_t pos1 = (rnd >> 32) % _op_seq.size();
      uint32_t pos2 = (uint32_t)rnd % _op_seq.size();
      if (pos1 != pos2) {
        op_type tmp_type = _op_seq[pos1]->type;
        uint64_t tmp_arg = _op_seq[pos1]->arg;
        _op_seq[pos1]->type = _op_seq[pos2]->type;
        _op_seq[pos2]->type = tmp_type;
        _op_seq[pos1]->arg = _op_seq[pos2]->arg;
        _op_seq[pos2]->arg = tmp_arg;
        if (!_evolve()) {
          ULIB_DEBUG("attempt to swap pos1=%u and pos2=%u was cancelled", pos1,
                     pos2);
          tmp_type = _op_seq[pos1]->type;
          tmp_arg = _op_seq[pos1]->arg;
          _op_seq[pos1]->type = _op_seq[pos2]->type;
          _op_seq[pos2]->type = tmp_type;
          _op_seq[pos1]->arg = _op_seq[pos2]->arg;
          _op_seq[pos2]->arg = tmp_arg;
        } else {
          ULIB_DEBUG("swapped pos1=%u and pos2=%u", pos1, pos2);
        }
      }
    }

    unlock();
  }

  bool evolve() {
    lock();
    bool ret = _evolve();
    unlock();
    return ret;
  }

  void print_best_seen() {
    lock();
    _print_best_seen();
    unlock();
  }

  // merkle damgard construction
  // this function is DANGEROUS because it is not protected by the lock
  uint64_t hash_value(const void *buf, size_t len) {
    const uint64_t m1 = 0xd36463187cc70d7bULL;
    const uint64_t m2 = 0xb597d0ceca3f6e07ULL;
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

      pc = (const unsigned char *)pos;

      switch (len & 7) {
      case 7:
        t ^= (uint64_t)pc[6] << 48;
      // fallthrough
      case 6:
        t ^= (uint64_t)pc[5] << 40;
      // fallthrough
      case 5:
        t ^= (uint64_t)pc[4] << 32;
      // fallthrough
      case 4:
        t ^= (uint64_t)pc[3] << 24;
      // fallthrough
      case 3:
        t ^= (uint64_t)pc[2] << 16;
      // fallthrough
      case 2:
        t ^= (uint64_t)pc[1] << 8;
      // fallthrough
      case 1:
        t ^= (uint64_t)pc[0];
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
    //const uint64_t   m1 = 0xd36463187cc70d7bULL;
    //const uint64_t   m2 = 0xb597d0ceca3f6e07ULL;
    const uint64_t *pos = (const uint64_t *)buf;
    const uint64_t *end = (const uint64_t *)((char *)pos + (len &
  ~15));
    const unsigned char *pc; uint64_t s = len; uint64_t t = len;

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

  static uint64_t gen_hash(const void *buf, size_t len) {
    return instance->hash_value(buf, len);
  }

private:
  uint64_t _process(uint64_t init) {
    for (vector<op *>::const_iterator it = _op_seq.begin(); it != _op_seq.end();
         ++it) {
      switch ((*it)->type) {
      case OP_MUL:
        init *= (*it)->arg;
        break;
      case OP_ADD:
        init += (*it)->arg;
        break;
      case OP_XOR:
        init ^= (*it)->arg;
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
      case OP_NOT:
        init = ~init;
        break;
      case OP_SWP:
        init = __builtin_bswap64(init);
        break;
      case OP_ASL:
        init += init << (*it)->arg;
        break;
      case OP_SSL:
        init -= init << (*it)->arg;
        break;
      case OP_SUB:
        init -= (*it)->arg;
        break;
      case OP_LOR:
        init <<= (*it)->arg;
        break;
      case OP_XQO:
        // xorsquare from https://github.com/skeeto/hash-prospector/issues/23
        {
          uint64_t n = (*it)->arg;
          init = (n|1ull)^(n*n);
        }
        break;
      // init += ~(init << (*it)->arg)
      // init -= ~(init << (*it)->arg)
      case OP_NUM:
        ULIB_FATAL("unknown op type:%d", (*it)->type);
      }
    }
    return init;
  }

  bool _evolve() {
    avalanche aval;
    timespec timer;
    float time_score;
    bool ret = true;

    if (unlikely(_best_seen_score < 0)) {
      // first time
      _init_with_latest();
      // warmup
      for (int i = 0; i < 10; i++) {
        (void)aval(gen_hash, g_aval_len, g_aval_times);
      }
      timer_start(&timer);
      _best_seen_score = aval(gen_hash, g_aval_len, g_aval_times);
      time_score = timer_stop(&timer) * g_time_r;
      _best_seen_score += time_score;
      printf("Best seen score: aval_score=%f, time_score=%f, overall=%f\n",
             _best_seen_score - time_score, time_score, _best_seen_score);
    } else {
      timer_start(&timer);
      float new_score = aval(gen_hash, g_aval_len, g_aval_times);
      time_score = timer_stop(&timer) * g_time_r;
      new_score += time_score;
      if (new_score < _best_seen_score) {
        // new score is better
        _update_best_seen();
        _best_seen_score = new_score;
        printf("Updated best seen score: aval_score=%f, time_score=%f, "
               "overall=%f\n",
               _best_seen_score - time_score, time_score, _best_seen_score);
      } else
        ret = false;
    }

    return ret;
  }

  char *_print_op(pair<op_type, uint64_t> it, char *buf) {
    switch (it.first) {
    case OP_MUL:
      snprintf(buf, BUF_SIZE, "MUL(%016llx)", (unsigned long long)it.second);
      break;
    case OP_XSL:
      snprintf(buf, BUF_SIZE, "XSL(%u)", (unsigned)it.second);
      break;
    case OP_XSR:
      snprintf(buf, BUF_SIZE, "XSR(%u)", (unsigned)it.second);
      break;
    case OP_ROR:
      snprintf(buf, BUF_SIZE, "ROR(%u)", (unsigned)it.second);
      break;
    case OP_ADD:
      snprintf(buf, BUF_SIZE, "ADD(%016llx)", (unsigned long long)it.second);
      break;
    case OP_XOR:
      snprintf(buf, BUF_SIZE, "XOR(%u)", (unsigned)it.second);
      break;
    case OP_NOT:
      snprintf(buf, BUF_SIZE, "NOT");
      break;
    case OP_SWP:
      snprintf(buf, BUF_SIZE, "SWP");
      break;
    case OP_ASL:
      snprintf(buf, BUF_SIZE, "ASL(%u)", (unsigned)it.second);
      break;
    case OP_SSL:
      snprintf(buf, BUF_SIZE, "SSL(%u)", (unsigned)it.second);
      break;
    case OP_SUB:
      snprintf(buf, BUF_SIZE, "SUB(%u)", (unsigned)it.second);
      break;
    case OP_LOR:
      snprintf(buf, BUF_SIZE, "LOR(%u)", (unsigned)it.second);
      break;
    case OP_XQO:
      snprintf(buf, BUF_SIZE, "XQO(%u)", (unsigned)it.second);
      break;
    case OP_NUM:
      snprintf(buf, BUF_SIZE, "UNKNOWN");
    }
    return buf;
  }

  void _print_best_seen() {
    printf("Best seen combination: ");
    for (vector<pair<op_type, uint64_t>>::const_iterator it =
             _best_seen.begin();
         it != _best_seen.end(); ++it) {
      char buf[BUF_SIZE];
      printf("%s ", _print_op(*it, buf));
    }
    printf("\t%f\n", _best_seen_score);
  }

  // start with a good baseline, what Zilong Tan computed as best in 2012.
  // and then get at least 2x as good.
  void _init_with_latest() {
    _best_seen.clear();
    _op_seq.clear();

    pair<op_type, uint64_t> ts[] = {
#ifdef START_WITH_FASTHASH
        {OP_XSR, 23},
        {OP_MUL, 0x2127599bf4325c37ULL},
        {OP_XSR, 47},
#elif defined START_WITH_PROSPECTOR
        {OP_MUL, 0xe4adbc73edb87283ULL},
        {OP_XSR, 25},
        {OP_NOT, 0},
        {OP_SWP, 0},
        {OP_MUL, 0x9743d1e18d4481c7ULL},
        {OP_XSR, 30},
#else
        {OP_ROR, 48}, // or 18
        {OP_ROR, 40}, //    38
        {OP_MUL, 0x2127599bf4325c37ULL},
        {OP_XSR, 34},
#endif
    };
    for (unsigned i = 0; i < ARR_SIZE(ts); i++) {
      auto t = ts[i];
      op *new_op = new op(t.first, hashgen::instance);
      new_op->arg = t.second;
      _op_seq.push_back(new_op);
      _best_seen.push_back(t);
    }

    _print_best_seen();
  }

  void _update_best_seen() {
    _best_seen.clear();
    for (vector<op *>::const_iterator it = _op_seq.begin(); it != _op_seq.end();
         ++it) {
      pair<op_type, uint64_t> t;
      t.first = (*it)->type;
      t.second = (*it)->arg;
      _best_seen.push_back(t);
    }
    _print_best_seen();
  }

  class thread_add_op : public thread {
  public:
    thread_add_op(hashgen *gen) : _gen(gen) {
      uint64_t now = rdtsc();
      RAND_NR_INIT(_u, _v, _w, now);
    }

    ~thread_add_op() { stop_and_join(); }

    int run() {
      while (is_running())
        _gen->add_op(RAND_NR_NEXT(_u, _v, _w));
      return 0;
    }

  private:
    hashgen *_gen;
    uint64_t _u, _v, _w;
  } * ctrl_add_op;

  class thread_del_op : public thread {
  public:
    thread_del_op(hashgen *gen) : _gen(gen) {
      uint64_t now = rdtsc();
      RAND_NR_INIT(_u, _v, _w, now);
    }

    ~thread_del_op() { stop_and_join(); }

    int run() {
      while (is_running())
        _gen->del_op(RAND_NR_NEXT(_u, _v, _w));
      return 0;
    }

  private:
    hashgen *_gen;
    uint64_t _u, _v, _w;
  } * ctrl_del_op;

  class thread_mod_op : public thread {
  public:
    thread_mod_op(hashgen *gen) : _gen(gen) {
      uint64_t now = rdtsc();
      RAND_NR_INIT(_u, _v, _w, now);
    }

    ~thread_mod_op() { stop_and_join(); }

    int run() {
      while (is_running())
        _gen->mod_op(RAND_NR_NEXT(_u, _v, _w));
      return 0;
    }

  private:
    hashgen *_gen;
    uint64_t _u, _v, _w;
  } * ctrl_mod_op;

  class thread_swap_op : public thread {
  public:
    thread_swap_op(hashgen *gen) : _gen(gen) {
      uint64_t now = rdtsc();
      RAND_NR_INIT(_u, _v, _w, now);
    }

    ~thread_swap_op() { stop_and_join(); }

    int run() {
      while (is_running())
        _gen->swap_op(RAND_NR_NEXT(_u, _v, _w));
      return 0;
    }

  private:
    hashgen *_gen;
    uint64_t _u, _v, _w;
  } * ctrl_swap_op;

  int volatile _min_seq;
  int volatile _max_seq;

  pthread_mutex_t _mutex;
  vector<op *> _op_seq;
  float volatile _best_seen_score;
  vector<pair<op_type, uint64_t>> _best_seen; // best seen result
};

hashgen *hashgen::instance = NULL;

int cmd_start(int, const char **) {
  if (hashgen::instance == NULL) {
    ULIB_FATAL("instance is NULL");
    return -1;
  }
  hashgen::instance->start();
  return 0;
}

int cmd_aval_rate(int argc, const char *argv[]) {
  if (argc > 1)
    g_aval_r = atof(argv[1]);
  printf("%f\n", g_aval_r);
  return 0;
}

int cmd_indep_rate(int argc, const char *argv[]) {
  if (argc > 1)
    g_indep_r = atof(argv[1]);
  printf("%f\n", g_indep_r);
  return 0;
}

int cmd_time_rate(int argc, const char *argv[]) {
  if (argc > 1)
    g_time_r = atof(argv[1]);
  printf("%f\n", g_time_r);
  return 0;
}

int cmd_aval_byte(int argc, const char *argv[]) {
  if (argc > 1)
    g_aval_len = atoi(argv[1]);
  printf("%d\n", g_aval_len);
  return 0;
}

int cmd_aval_times(int argc, const char *argv[]) {
  if (argc > 1)
    g_aval_times = atoi(argv[1]);
  printf("%d\n", g_aval_times);
  return 0;
}

int cmd_min_seq(int argc, const char *argv[]) {
  if (hashgen::instance == NULL) {
    ULIB_FATAL("instance is NULL");
    return -1;
  }
  if (argc > 1)
    hashgen::instance->set_min_seq(atoi(argv[1]));
  printf("%d\n", hashgen::instance->get_min_seq());
  return 0;
}

int cmd_max_seq(int argc, const char *argv[]) {
  if (hashgen::instance == NULL) {
    ULIB_FATAL("instance is NULL");
    return -1;
  }
  if (argc > 1)
    hashgen::instance->set_max_seq(atoi(argv[1]));
  printf("%d\n", hashgen::instance->get_max_seq());
  return 0;
}

int cmd_best_seen(int, const char **) {
  if (hashgen::instance == NULL) {
    ULIB_FATAL("instance is NULL");
    return -1;
  }
  hashgen::instance->print_best_seen();
  return 0;
}

static uint64_t hash_xxhash_noseed(const void *buf, size_t len) {
  uint64_t low, high;
  low = XXH_fast32(buf, len, 0);
  high = XXH_fast32(buf, len, 1);
  return low | (high << 32);
}

static uint64_t hash_jenkins_noseed(const void *buf, size_t len) {
  uint64_t hash = 0x0000000100000001ULL;
  uint32_t *ph = (uint32_t *)&hash;
  hash_jenkins2(buf, len, ph, ph + 1);
  return hash;
}

static uint64_t fasthash64_noseed(const void *buf, size_t len) {
  return fasthash64(buf, len, 0);
}

int cmd_standard(int, const char **) {
  avalanche aval;
  timespec timer;
  float ascore, tscore;

  timer_start(&timer);
  ascore = aval(hash_jenkins_noseed, g_aval_len, g_aval_times);
  tscore = timer_stop(&timer) * g_time_r;
  printf("JenkinsHash: aval_score=%f, time_score=%f, overall=%f\n", ascore,
         tscore, ascore + tscore);

  timer_start(&timer);
  ascore = aval(hash_xxhash_noseed, g_aval_len, g_aval_times);
  tscore = timer_stop(&timer) * g_time_r;
  printf("XXHash     : aval_score=%f, time_score=%f, overall=%f\n", ascore,
         tscore, ascore + tscore);

  timer_start(&timer);
  ascore = aval(fasthash64_noseed, g_aval_len, g_aval_times);
  tscore = timer_stop(&timer) * g_time_r;
  printf("fasthash64 : aval_score=%f, time_score=%f, overall=%f\n", ascore,
         tscore, ascore + tscore);

  return 0;
}

int cmd_help(int, const char **) {
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

int main(int argc, const char *argv[]) {
  hashgen::instance = new hashgen;
  console_t con;

  printf("Non-cryptographic Hash Function Generator 1.1 alpha\n");
  printf("Zilong Tan (eric.zltan@gmail.com)\n");

  if (argc == 2 && !strcmp(argv[1], "start")) {
    cmd_standard(argc, argv);
    cmd_start(argc, argv);
  } else {
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
  }

  delete hashgen::instance;

  return 0;
}
