/* Copyright 2016-2020 Gyeonghwan Hong, Eunsoo Park, Sungkyunkwan University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <sys/time.h>

#include "jmem-profiler-common-internal.h"
#include "jmem-profiler.h"

#if defined(JMEM_PROFILE) && defined(PROF_TIME)
static void __check_watch(struct timeval *timer);
static void __stop_watch(struct timeval *timer, struct timeval *t);
#endif

/* Time profiling */
inline void __attr_always_inline___ init_time_profiler(void) {
#if defined(JMEM_PROFILE) && defined(PROF_TIME)
#define INIT_PROFILER_TIME_ENTRY(x)   \
  JERRY_CONTEXT(x##_time).tv_sec = 0; \
  JERRY_CONTEXT(x##_time).tv_usec = 0

  CHECK_LOGGING_ENABLED();
  __check_watch(&JERRY_CONTEXT(timeval_uptime));
  INIT_PROFILER_TIME_ENTRY(alloc);
  INIT_PROFILER_TIME_ENTRY(free);
  INIT_PROFILER_TIME_ENTRY(gc);
  INIT_PROFILER_TIME_ENTRY(compression_rmc_hit);
  INIT_PROFILER_TIME_ENTRY(compression_fifo_hit);
  INIT_PROFILER_TIME_ENTRY(compression_final_miss);
#endif
}

#define SEC_IN_US (1000 * 1000)
#define SEC_IN_NS (1000 * 1000 * 1000)
#define US_IN_NS (1000)

#if defined(JMEM_PROFILE) && defined(PROF_TIME)
static inline unsigned long long get_uptime_ns(void) {
  struct timeval uptime;
  uptime.tv_sec = 0;
  uptime.tv_usec = 0;
  __stop_watch(&JERRY_CONTEXT(timeval_uptime), &uptime);
  unsigned long long uptime_ns =
      ((unsigned long long)uptime.tv_sec * SEC_IN_NS +
       (unsigned long long)uptime.tv_usec * US_IN_NS);
  return uptime_ns;
}
#endif

void print_time_profile(void) {
#if defined(JMEM_PROFILE) && defined(PROF_TIME)
#define TOTAL_TIME(x)                                               \
  ((unsigned long long)JERRY_CONTEXT(x##_time.tv_sec) * SEC_IN_NS + \
   (unsigned long long)JERRY_CONTEXT(x##_time.tv_usec) * US_IN_NS)
#define AVG_TIME(x)                                                  \
  (JERRY_CONTEXT(x##_count) > 0)                                     \
      ? TOTAL_TIME(x) / (unsigned long long)JERRY_CONTEXT(x##_count) \
      : 0
#define AVG_TIME3(x, y, z)                                           \
  ((unsigned long long)JERRY_CONTEXT(x##_count)                      \
    + (unsigned long long)JERRY_CONTEXT(y##_count)                   \
    + (unsigned long long)JERRY_CONTEXT(z##_count) > 0)              \
      ? (TOTAL_TIME3(x,y,z))                                         \
        / ((unsigned long long)JERRY_CONTEXT(x##_count) +            \
            (unsigned long long)JERRY_CONTEXT(y##_count) +           \
            (unsigned long long)JERRY_CONTEXT(z##_count)) : 0
#define TOTAL_TIME3(x, y, z) (TOTAL_TIME(x) + TOTAL_TIME(y)          \
                              + TOTAL_TIME(z))
#define PRINT_TIME_HEADER(fp) fprintf(fp, "Time")
#define PRINT_TIME(fp, x) fprintf(fp, ", %llu", x)
#define PRINT_AVG_TIME(fp, x) fprintf(fp, ", %llu", AVG_TIME(x))
#define PRINT_AVG_TIME3(fp, x, y, z) fprintf(fp, ", %llu", AVG_TIME3(x, y, z))
#define PRINT_TOTAL_TIME(fp, x) fprintf(fp, ", %llu", TOTAL_TIME(x))
#define PRINT_TOTAL_TIME3(fp, x, y, z) fprintf(fp, ", %llu", TOTAL_TIME3(x, y, z))
#define PRINT_TIME_TAIL(fp) fprintf(fp, "\n")

  CHECK_LOGGING_ENABLED();
  FILE *fp = fopen(PROF_TIME_FILENAME, "a");

  PRINT_TIME_HEADER(fp);
  PRINT_TIME(fp, get_uptime_ns());
  PRINT_AVG_TIME(fp, alloc);
  PRINT_AVG_TIME(fp, free);
  PRINT_AVG_TIME(fp, gc);
  PRINT_AVG_TIME(fp, decompression);
  PRINT_AVG_TIME3(fp, compression_rmc_hit, compression_fifo_hit, compression_final_miss);
  PRINT_AVG_TIME(fp, compression_rmc_hit);
  PRINT_AVG_TIME(fp, compression_fifo_hit);
  PRINT_AVG_TIME(fp, compression_final_miss);
  PRINT_TOTAL_TIME(fp, alloc);
  PRINT_TOTAL_TIME(fp, free);
  PRINT_TOTAL_TIME(fp, gc);
  PRINT_TOTAL_TIME(fp, decompression);
  PRINT_TOTAL_TIME3(fp, compression_rmc_hit, compression_fifo_hit, compression_final_miss);
  PRINT_TOTAL_TIME(fp, compression_rmc_hit);
  PRINT_TOTAL_TIME(fp, compression_fifo_hit);
  PRINT_TOTAL_TIME(fp, compression_final_miss);
  PRINT_TIME_TAIL(fp);
  fflush(fp);
  fclose(fp);
#endif
}

void __attr_always_inline___ profile_alloc_start(void) {
#if defined(PROF_TIME__ALLOC)
  CHECK_LOGGING_ENABLED();
  JERRY_CONTEXT(alloc_count)++;
  __check_watch(&JERRY_CONTEXT(timeval_alloc));
#endif
}
void __attr_always_inline___ profile_alloc_end(void) {
#if defined(PROF_TIME__ALLOC)
  CHECK_LOGGING_ENABLED();
  __stop_watch(&JERRY_CONTEXT(timeval_alloc), &JERRY_CONTEXT(alloc_time));
#endif
}

void __attr_always_inline___ profile_free_start(void) {
#if defined(PROF_TIME__FREE)
  CHECK_LOGGING_ENABLED();
  JERRY_CONTEXT(free_count)++;
  __check_watch(&JERRY_CONTEXT(timeval_free));
#endif
}
void __attr_always_inline___ profile_free_end(void) {
#if defined(PROF_TIME__FREE)
  CHECK_LOGGING_ENABLED();
  __stop_watch(&JERRY_CONTEXT(timeval_free), &JERRY_CONTEXT(free_time));
#endif
}

void __attr_always_inline___ profile_compression_start(void) {
#if defined(PROF_TIME__COMPRESSION)
  CHECK_LOGGING_ENABLED();
  __check_watch(&JERRY_CONTEXT(timeval_compression));
#endif
}
inline void __attr_always_inline___ profile_compression_end(int type) {
#if defined(PROF_TIME__COMPRESSION)
  CHECK_LOGGING_ENABLED();
  if (type == 0) { // COMPRESSION_RMC_HIT
    JERRY_CONTEXT(compression_rmc_hit_count)++;
    __stop_watch(&JERRY_CONTEXT(timeval_compression),
                 &JERRY_CONTEXT(compression_rmc_hit_time));
  } else if (type == 1) { // COMPRESSION_FIFO_HIT
    JERRY_CONTEXT(compression_fifo_hit_count)++;
    __stop_watch(&JERRY_CONTEXT(timeval_compression),
                 &JERRY_CONTEXT(compression_fifo_hit_time));
  } else if (type == 2) { // COMPRESSION_FINAL_MISS
    JERRY_CONTEXT(compression_final_miss_count)++;
    __stop_watch(&JERRY_CONTEXT(timeval_compression),
                 &JERRY_CONTEXT(compression_final_miss_time));
  } else {
    printf("Invalid argument to profile_compression_end: %d\n", type);
  }
#else
  JERRY_UNUSED(type);
#endif
}

inline void __attr_always_inline___ profile_decompression_start(void) {
#if defined(PROF_TIME__DECOMPRESSION)
  CHECK_LOGGING_ENABLED();
  JERRY_CONTEXT(decompression_count)++;
  __check_watch(&JERRY_CONTEXT(timeval_decompression));
#endif
}
inline void __attr_always_inline___ profile_decompression_end(void) {
#if defined(PROF_TIME__DECOMPRESSION)
  CHECK_LOGGING_ENABLED();
  __stop_watch(&JERRY_CONTEXT(timeval_decompression),
               &JERRY_CONTEXT(decompression_time));
#endif
}

inline void __attr_always_inline___ profile_gc_start(void) {
#if defined(PROF_TIME__GC)
  CHECK_LOGGING_ENABLED();
  JERRY_CONTEXT(gc_count)++;
  __check_watch(&JERRY_CONTEXT(timeval_gc));
#endif
}
inline void __attr_always_inline___ profile_gc_end(void) {
#if defined(PROF_TIME__GC)
  CHECK_LOGGING_ENABLED();
  __stop_watch(&JERRY_CONTEXT(timeval_gc), &JERRY_CONTEXT(gc_time));
#endif
}

/* Internal functions */
#if defined(JMEM_PROFILE) && defined(PROF_TIME)
void __check_watch(struct timeval *timer) {
  gettimeofday(timer, NULL);
}
void __stop_watch(struct timeval *timer, struct timeval *t) {
  struct timeval curr_time;

  gettimeofday(&curr_time, NULL);

  curr_time.tv_usec -= timer->tv_usec;
  if (curr_time.tv_usec < 0) {
    curr_time.tv_usec += 1000000;
    curr_time.tv_sec -= 1;
  }
  curr_time.tv_sec -= timer->tv_sec;
  JERRY_ASSERT(curr_time.tv_sec >= 0 && curr_time.tv_usec >= 0);

  t->tv_sec += curr_time.tv_sec;
  t->tv_usec += curr_time.tv_usec;

  if (t->tv_usec > 1000000) {
    t->tv_sec += 1;
    t->tv_usec -= 1000000;
  }
}
#endif
