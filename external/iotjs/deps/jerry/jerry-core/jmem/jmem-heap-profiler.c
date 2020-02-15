/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
 * Copyright 2016-2019 Gyeonghwan Hong, Eunsoo Park, Sungkyunkwan University
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

#include "jmem-heap-profiler.h"

#define UNUSED(x) (void)(x)

/* Total size profiling */
static struct timeval g_timeval_init;

inline void __attr_always_inline___ profile_set_init_time(void) {
#ifdef JMEM_PROFILE_TOTAL_SIZE
  gettimeofday(&g_timeval_init, NULL);
#endif
}
void profile_get_curr_time(struct timeval *curr_time);
inline void __attr_always_inline___
profile_get_curr_time(struct timeval *curr_time) {
  gettimeofday(curr_time, NULL);

  if (curr_time->tv_usec < g_timeval_init.tv_usec) {
    curr_time->tv_usec += 1000000;
    curr_time->tv_sec -= 1;
  }
  curr_time->tv_usec -= g_timeval_init.tv_usec;
  curr_time->tv_sec -= g_timeval_init.tv_sec;
}

inline void __attr_always_inline___ profile_print_total_size(void) {
#ifdef JMEM_PROFILE_TOTAL_SIZE
  struct timeval curr_time;
  profile_get_curr_time(&curr_time);
  uint32_t segments_in_bytes = 0;
#ifdef JMEM_SEGMENTED_HEAP
  segments_in_bytes = (uint32_t)JERRY_HEAP_CONTEXT(segments_count) * JMEM_SEGMENT_SIZE;
#endif
  printf("%lu.%06lu\t%lu\t%lu\n", curr_time.tv_sec, curr_time.tv_usec,
         (uint32_t)JERRY_HEAP_CONTEXT(jmem_heap_allocated_size), segments_in_bytes);
#endif
}

/* Time profiling */
static struct timeval g_timeval_alloc;
static struct timeval g_timeval_free;
static struct timeval g_timeval_compression;
static struct timeval g_timeval_decompression;
static struct timeval g_timeval_gc;

static struct timeval g_alloc_time;
static struct timeval g_free_time;
static struct timeval g_compression_time;
static struct timeval g_decompression_time;
static struct timeval g_gc_time;

static unsigned int g_alloc_count = 0;
static unsigned int g_free_count = 0;
static unsigned int g_compression_count = 0;
static unsigned int g_decompression_count = 0;
static unsigned int g_gc_count = 0;

void profile_init_times(void) {
#ifdef JMEM_PROFILE_TIME
  g_compression_time.tv_sec = 0;
  g_compression_time.tv_usec = 0;
#endif
}

void profile_print_times(void) {
#ifdef JMEM_PROFILE_TIME
  printf("Alloc(%u): %ld.%ld\n", g_alloc_count, g_alloc_time.tv_sec,
         g_alloc_time.tv_usec);
  printf("Free(%u): %ld.%ld\n", g_free_count, g_free_time.tv_sec,
         g_free_time.tv_usec);
  printf("Compression(%u): %ld.%ld\n", g_compression_count,
         g_compression_time.tv_sec, g_compression_time.tv_usec);
  printf("Decompression(%u): %ld.%ld\n", g_decompression_count,
         g_decompression_time.tv_sec, g_decompression_time.tv_usec);
  printf("GC(%u): %ld.%ld\n", g_gc_count, g_gc_time.tv_sec, g_gc_time.tv_usec);
#else
  UNUSED(g_alloc_count);
  UNUSED(g_free_count);
  UNUSED(g_compression_count);
  UNUSED(g_decompression_count);
  UNUSED(g_gc_count);
  UNUSED(g_alloc_time);
  UNUSED(g_free_time);
  UNUSED(g_compression_time);
  UNUSED(g_decompression_time);
  UNUSED(g_gc_time);
  UNUSED(g_timeval_alloc);
  UNUSED(g_timeval_free);
  UNUSED(g_timeval_compression);
  UNUSED(g_timeval_decompression);
  UNUSED(g_timeval_gc);
#endif
}

void check_watch(struct timeval *timer);
void stop_watch(struct timeval *timer, struct timeval *t);

void check_watch(struct timeval *timer) { gettimeofday(timer, NULL); }
void stop_watch(struct timeval *timer, struct timeval *t) {
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

void __attr_always_inline___ profile_alloc_start(void) {
#ifdef JMEM_PROFILE_TIME
  g_alloc_count++;
  check_watch(&g_timeval_alloc);
#endif
}
void __attr_always_inline___ profile_alloc_end(void) {
#ifdef JMEM_PROFILE_TIME
  stop_watch(&g_timeval_alloc, &g_alloc_time);
#endif
}

void __attr_always_inline___ profile_free_start(void) {
#ifdef JMEM_PROFILE_TIME
  g_free_count++;
  check_watch(&g_timeval_free);
#endif
}
void __attr_always_inline___ profile_free_end(void) {
#ifdef JMEM_PROFILE_TIME
  stop_watch(&g_timeval_free, &g_free_time);
#endif
}

void __attr_always_inline___ profile_compression_start(void) {
#ifdef JMEM_PROFILE_TIME
  g_compression_count++;
  check_watch(&g_timeval_compression);
#endif
}
inline void __attr_always_inline___ profile_compression_end(void) {
#ifdef JMEM_PROFILE_TIME
  stop_watch(&g_timeval_compression, &g_compression_time);
#endif
}

inline void __attr_always_inline___ profile_decompression_start(void) {
#ifdef JMEM_PROFILE_TIME
  g_decompression_count++;
  check_watch(&g_timeval_decompression);
#endif
}
inline void __attr_always_inline___ profile_decompression_end(void) {
#ifdef JMEM_PROFILE_TIME
  stop_watch(&g_timeval_decompression, &g_decompression_time);
#endif
}

inline void __attr_always_inline___ profile_gc_start(void) {
#ifdef JMEM_PROFILE_TIME
  g_gc_count++;
  check_watch(&g_timeval_gc);
#endif
}
inline void __attr_always_inline___ profile_gc_end(void) {
#ifdef JMEM_PROFILE_TIME
  stop_watch(&g_timeval_gc, &g_gc_time);
#endif
}