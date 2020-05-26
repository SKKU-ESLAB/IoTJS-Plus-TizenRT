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
  CHECK_LOGGING_ENABLED();
  __check_watch(&JERRY_CONTEXT(timeval_uptime));
  JERRY_CONTEXT(alloc_time).tv_sec = 0;
  JERRY_CONTEXT(alloc_time).tv_usec = 0;
  JERRY_CONTEXT(free_time).tv_sec = 0;
  JERRY_CONTEXT(free_time).tv_usec = 0;
  JERRY_CONTEXT(compression_time).tv_sec = 0;
  JERRY_CONTEXT(compression_time).tv_usec = 0;
  JERRY_CONTEXT(decompression_time).tv_sec = 0;
  JERRY_CONTEXT(decompression_time).tv_usec = 0;
  JERRY_CONTEXT(gc_time).tv_sec = 0;
  JERRY_CONTEXT(gc_time).tv_usec = 0;
  JERRY_CONTEXT(alloc_count) = 0;
  JERRY_CONTEXT(free_count) = 0;
  JERRY_CONTEXT(compression_count) = 0;
  JERRY_CONTEXT(decompression_count) = 0;
  JERRY_CONTEXT(gc_count) = 0;
#endif
}

void print_time_profile(void) {
#if defined(JMEM_PROFILE) && defined(PROF_TIME)
  CHECK_LOGGING_ENABLED();
  FILE *fp = fopen(PROF_TIME_FILENAME, "a");

  struct timeval total_time;
  total_time.tv_sec = 0;
  total_time.tv_usec = 0;
  __stop_watch(&JERRY_CONTEXT(timeval_uptime), &total_time);

#ifdef PROF_TIME__PRINT_HEADER
  fprintf(fp, "Category, Total, Alloc, Free, Compression, Decompression, GC\n");
#endif /* defined(PROF_TIME__PRINT_HEADER) */
#ifdef PROF_TIME__PRINT_AVERAGE
#define SEC_IN_US (1000 * 1000)
#define SEC_IN_NS (1000 * 1000 * 1000)
#define US_IN_NS (1000)
#define AVG_TIME(x)                                                        \
  (JERRY_CONTEXT(x##_count) > 0)                                           \
      ? ((unsigned long long)JERRY_CONTEXT(x##_time.tv_sec) * SEC_IN_NS +  \
         (unsigned long long)JERRY_CONTEXT(x##_time.tv_usec) * US_IN_NS) / \
            (unsigned long long)JERRY_CONTEXT(x##_count)                   \
      : 0
  unsigned long long total_time_ns =
      ((unsigned long long)total_time.tv_sec * SEC_IN_NS +
       (unsigned long long)total_time.tv_usec * US_IN_NS);
  unsigned long long avg_alloc_time_ns = AVG_TIME(alloc);
  unsigned long long avg_free_time_ns = AVG_TIME(free);
  unsigned long long avg_compression_time_ns = AVG_TIME(compression);
  unsigned long long avg_decompression_time_ns = AVG_TIME(decompression);
  unsigned long long avg_gc_time_ns = AVG_TIME(gc);
  fprintf(fp, "Time, %llu, %llu, %llu, %llu, %llu, %llu\n", total_time_ns,
          avg_alloc_time_ns, avg_free_time_ns, avg_compression_time_ns,
          avg_decompression_time_ns, avg_gc_time_ns);
#else
  fprintf(fp, "Count, 0, %u, %u, %u, %u, %u\n", JERRY_CONTEXT(alloc_count),
          JERRY_CONTEXT(free_count), JERRY_CONTEXT(compression_count),
          JERRY_CONTEXT(decompression_count), JERRY_CONTEXT(gc_count));
  fprintf(fp,
          "Time, %ld.%06ld, %ld.%06ld, %ld.%06ld, %ld.%06ld, %ld.%06ld, "
          "%ld.%06ld\n",
          (long)(total_time.tv_sec), (long)(total_time.tv_usec),
          JERRY_CONTEXT(alloc_time).tv_sec, JERRY_CONTEXT(alloc_time).tv_usec,
          JERRY_CONTEXT(free_time).tv_sec, JERRY_CONTEXT(free_time).tv_usec,
          JERRY_CONTEXT(compression_time).tv_sec,
          JERRY_CONTEXT(compression_time).tv_usec,
          JERRY_CONTEXT(decompression_time).tv_sec,
          JERRY_CONTEXT(decompression_time).tv_usec,
          JERRY_CONTEXT(gc_time).tv_sec, JERRY_CONTEXT(gc_time).tv_usec);
#endif

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
  JERRY_CONTEXT(compression_count)++;
  __check_watch(&JERRY_CONTEXT(timeval_compression));
#endif
}
inline void __attr_always_inline___ profile_compression_end(void) {
#if defined(PROF_TIME__COMPRESSION)
  CHECK_LOGGING_ENABLED();
  __stop_watch(&JERRY_CONTEXT(timeval_compression),
               &JERRY_CONTEXT(compression_time));
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