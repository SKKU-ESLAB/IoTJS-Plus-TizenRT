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

static void __check_watch(struct timeval *timer);
static void __stop_watch(struct timeval *timer, struct timeval *t);

/* Time profiling */
inline void __attr_always_inline___ init_time_profiler(void) {
#if defined(JMEM_PROFILE) && defined(PROF_TIME)
  CHECK_LOGGING_ENABLED();
  gettimeofday(&JERRY_CONTEXT(timeval_start), NULL);
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

  struct timeval timeval_end;
  gettimeofday(&timeval_end, NULL);
  long long startUsec =
      (long long)JERRY_CONTEXT(timeval_start).tv_sec * 1000000 +
      (long long)JERRY_CONTEXT(timeval_start).tv_usec;
  long long endUsec =
      (long long)timeval_end.tv_sec * 1000000 + (long long)timeval_end.tv_usec;
  long long totalUsec = endUsec - startUsec;

  fprintf(fp, "Category, Total, Alloc, Free, Compression, Decompression, GC\n");
  fprintf(fp, "Count, 0, %u, %u, %u, %u, %u\n", JERRY_CONTEXT(alloc_count),
          JERRY_CONTEXT(free_count), JERRY_CONTEXT(compression_count),
          JERRY_CONTEXT(decompression_count), JERRY_CONTEXT(gc_count));
  fprintf(fp,
          "Time(Sec), %ld.%06ld, %ld.%06ld, %ld.%06ld, %ld.%06ld, %ld.%06ld, "
          "%ld.%06ld\n",
          (long)(totalUsec / 1000000), (long)(totalUsec % 1000000),
          JERRY_CONTEXT(alloc_time).tv_sec, JERRY_CONTEXT(alloc_time).tv_usec,
          JERRY_CONTEXT(free_time).tv_sec, JERRY_CONTEXT(free_time).tv_usec,
          JERRY_CONTEXT(compression_time).tv_sec,
          JERRY_CONTEXT(compression_time).tv_usec,
          JERRY_CONTEXT(decompression_time).tv_sec,
          JERRY_CONTEXT(decompression_time).tv_usec,
          JERRY_CONTEXT(gc_time).tv_sec, JERRY_CONTEXT(gc_time).tv_usec);

  fflush(fp);
  fclose(fp);
#endif
}

void __attr_always_inline___ profile_alloc_start(void) {
#if defined(JMEM_PROFILE) && defined(PROF_TIME)
  CHECK_LOGGING_ENABLED();
  JERRY_CONTEXT(alloc_count)++;
  __check_watch(&JERRY_CONTEXT(timeval_alloc));
#endif
}
void __attr_always_inline___ profile_alloc_end(void) {
#if defined(JMEM_PROFILE) && defined(PROF_TIME)
  CHECK_LOGGING_ENABLED();
  __stop_watch(&JERRY_CONTEXT(timeval_alloc), &JERRY_CONTEXT(alloc_time));
#endif
}

void __attr_always_inline___ profile_free_start(void) {
#if defined(JMEM_PROFILE) && defined(PROF_TIME)
  CHECK_LOGGING_ENABLED();
  JERRY_CONTEXT(free_count)++;
  __check_watch(&JERRY_CONTEXT(timeval_free));
#endif
}
void __attr_always_inline___ profile_free_end(void) {
#if defined(JMEM_PROFILE) && defined(PROF_TIME)
  CHECK_LOGGING_ENABLED();
  __stop_watch(&JERRY_CONTEXT(timeval_free), &JERRY_CONTEXT(free_time));
#endif
}

void __attr_always_inline___ profile_compression_start(void) {
#if defined(JMEM_PROFILE) && defined(PROF_TIME)
  CHECK_LOGGING_ENABLED();
  JERRY_CONTEXT(compression_count)++;
  __check_watch(&JERRY_CONTEXT(timeval_compression));
#endif
}
inline void __attr_always_inline___ profile_compression_end(void) {
#if defined(JMEM_PROFILE) && defined(PROF_TIME)
  CHECK_LOGGING_ENABLED();
  __stop_watch(&JERRY_CONTEXT(timeval_compression),
               &JERRY_CONTEXT(compression_time));
#endif
}

inline void __attr_always_inline___ profile_decompression_start(void) {
#if defined(JMEM_PROFILE) && defined(PROF_TIME)
  CHECK_LOGGING_ENABLED();
  JERRY_CONTEXT(decompression_count)++;
  __check_watch(&JERRY_CONTEXT(timeval_decompression));
#endif
}
inline void __attr_always_inline___ profile_decompression_end(void) {
#if defined(JMEM_PROFILE) && defined(PROF_TIME)
  CHECK_LOGGING_ENABLED();
  __stop_watch(&JERRY_CONTEXT(timeval_decompression),
               &JERRY_CONTEXT(decompression_time));
#endif
}

inline void __attr_always_inline___ profile_gc_start(void) {
#if defined(JMEM_PROFILE) && defined(PROF_TIME)
  CHECK_LOGGING_ENABLED();
  JERRY_CONTEXT(gc_count)++;
  __check_watch(&JERRY_CONTEXT(timeval_gc));
#endif
}
inline void __attr_always_inline___ profile_gc_end(void) {
#if defined(JMEM_PROFILE) && defined(PROF_TIME)
  CHECK_LOGGING_ENABLED();
  __stop_watch(&JERRY_CONTEXT(timeval_gc), &JERRY_CONTEXT(gc_time));
  printf("GC done (%u)\n", JERRY_CONTEXT(gc_count));
#endif
}

/* Internal functions */
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