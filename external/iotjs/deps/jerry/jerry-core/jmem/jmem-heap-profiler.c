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

static void __get_js_uptime(struct timeval *js_uptime);
static long __get_timeval_diff_usec(struct timeval *prior,
                                    struct timeval *post);
static void __profile_print_total_size(void);
static void __profile_print_segment_utilization(const char *type,
                                                size_t jsobject_size);

inline void __attr_always_inline___ profile_set_js_start_time(void) {
#if defined(JMEM_PROFILE)
  gettimeofday(&JERRY_CONTEXT(timeval_js_start), NULL);
#if defined(JMEM_PROFILE_TOTAL_SIZE__PERIOD_USEC)
  JERRY_CONTEXT(jsuptime_recent_total_size_print).tv_sec = 0;
  JERRY_CONTEXT(jsuptime_recent_total_size_print).tv_usec = 0;
#endif
#if defined(JMEM_PROFILE_SEGMENT_UTILIZATION__PERIOD_USEC)
  JERRY_CONTEXT(jsuptime_recent_segutil_print).tv_sec = 0;
  JERRY_CONTEXT(jsuptime_recent_segutil_print).tv_usec = 0;
#endif
#endif
}

inline long __attr_always_inline___
__get_timeval_diff_usec(struct timeval *prior, struct timeval *post) {
  long timeval_diff_usec = (post->tv_sec - prior->tv_sec) * 1000 * 1000 +
                           (post->tv_usec - prior->tv_usec);
  return timeval_diff_usec;
}

inline void __attr_always_inline___ __get_js_uptime(struct timeval *js_uptime) {
  gettimeofday(js_uptime, NULL);

  if (js_uptime->tv_usec < JERRY_CONTEXT(timeval_js_start).tv_usec) {
    js_uptime->tv_usec += 1000000;
    js_uptime->tv_sec -= 1;
  }
  js_uptime->tv_usec -= JERRY_CONTEXT(timeval_js_start).tv_usec;
  js_uptime->tv_sec -= JERRY_CONTEXT(timeval_js_start).tv_sec;
}

/* Total size profiling */


inline void __attr_always_inline___ profile_print_total_size_each_time(void) {
#if defined(JMEM_PROFILE_TOTAL_SIZE__PERIOD_USEC)
  struct timeval js_uptime;
  __get_js_uptime(&js_uptime);
  long timeval_diff_in_usec =
      __get_timeval_diff_usec(&JERRY_CONTEXT(jsuptime_recent_total_size_print),
                              &js_uptime);
  if (timeval_diff_in_usec > JMEM_PROFILE_TOTAL_SIZE__PERIOD_USEC) {
    JERRY_CONTEXT(jsuptime_recent_total_size_print) = js_uptime;
    __profile_print_total_size();
  }
#else
  __profile_print_total_size();
#endif /* defined(JMEM_PROFILE_TOTAL_SIZE__PERIOD_USEC) */
}

inline void __attr_always_inline___ profile_print_total_size_finally(void) {
  __profile_print_total_size();
}

inline void __attr_always_inline___ __profile_print_total_size(void) {
#if defined(JMEM_PROFILE) && defined(JMEM_PROFILE_TOTAL_SIZE)
  FILE *fp = stdout;
#ifdef JMEM_PROFILE_TOTAL_SIZE_FILENAME
  fp = fopen(JMEM_PROFILE_TOTAL_SIZE_FILENAME, "a");
#endif

  struct timeval js_uptime;
  __get_js_uptime(&js_uptime);

  uint32_t segments_in_bytes = 0;
#ifdef JMEM_SEGMENTED_HEAP
  segments_in_bytes =
      (uint32_t)JERRY_HEAP_CONTEXT(segments_count) * JMEM_SEGMENT_SIZE;
#endif
  fprintf(fp, "TS, %lu.%06lu, %lu, %lu\n", js_uptime.tv_sec, js_uptime.tv_usec,
          (uint32_t)JERRY_CONTEXT(jmem_heap_allocated_size), segments_in_bytes);
#ifdef JMEM_PROFILE_TOTAL_SIZE_FILENAME
  fflush(fp);
  fclose(fp);
#endif
#endif
}

/* Segment utilization profiling */
inline void __attr_always_inline___
profile_print_segment_utilization_after_free_block(size_t jsobject_size) {
#if defined(JMEM_PROFILE_SEGMENT_UTILIZATION__AFTER_FREE_BLOCK)
#if defined(JMEM_PROFILE_SEGMENT_UTILIZATION__PERIOD_USEC)
  struct timeval js_uptime;
  __get_js_uptime(&js_uptime);
  long timeval_diff_in_usec =
      __get_timeval_diff_usec(&JERRY_CONTEXT(jsuptime_recent_segutil_print),
                              &js_uptime);
  if (timeval_diff_in_usec > JMEM_PROFILE_SEGMENT_UTILIZATION__PERIOD_USEC) {
    JERRY_CONTEXT(jsuptime_recent_segutil_print) = js_uptime;
    __profile_print_segment_utilization("AFB", jsobject_size);
  }
#else
  __profile_print_segment_utilization("AFB", jsobject_size);
#endif /* defined(JMEM_PROFILE_SEGMENT_UTILIZATION__PERIOD_USEC) */
#else
  UNUSED(jsobject_size);
#endif
}

inline void __attr_always_inline___
profile_print_segment_utilization_before_add_segment(size_t jsobject_size) {
#if defined(JMEM_PROFILE_SEGMENT_UTILIZATION__BEFORE_ADD_SEGMENT)
  __profile_print_segment_utilization("BAS", jsobject_size);
#else
  UNUSED(jsobject_size);
#endif
}

inline void __attr_always_inline___
profile_print_segment_utilization_before_gc(size_t jsobject_size) {
#if defined(JMEM_PROFILE_SEGMENT_UTILIZATION__BEFORE_GC)
  __profile_print_segment_utilization("BGC", jsobject_size);
#else
  UNUSED(jsobject_size);
#endif
}

inline void __attr_always_inline___
profile_print_segment_utilization_after_gc(size_t jsobject_size) {
#if defined(JMEM_PROFILE_SEGMENT_UTILIZATION__AFTER_GC)
  __profile_print_segment_utilization("AGC", jsobject_size);
#else
  UNUSED(jsobject_size);
#endif
}

inline void __attr_always_inline___
profile_print_segment_utilization_finally(void) {
  __profile_print_segment_utilization("F", 0);
}

inline void __attr_always_inline___
__profile_print_segment_utilization(const char *header, size_t jsobject_size) {
#if defined(JMEM_SEGMENTED_HEAP) && defined(JMEM_PROFILE) && \
    defined(JMEM_PROFILE_SEGMENT_UTILIZATION)
  struct timeval js_uptime;
  __get_js_uptime(&js_uptime);

  FILE *fp = stdout;
#ifdef JMEM_PROFILE_SEGMENT_UTILIZATION_FILENAME
  fp = fopen(JMEM_PROFILE_SEGMENT_UTILIZATION_FILENAME, "a");
#endif
  fprintf(fp, "%s %lu, %lu.%06lu", header, (unsigned long)jsobject_size,
          js_uptime.tv_sec, js_uptime.tv_usec);

  for (uint32_t segment_idx = 0; segment_idx < JMEM_SEGMENT; segment_idx++) {
    jmem_segment_t *segment = &(JERRY_HEAP_CONTEXT(segments[segment_idx]));
#ifdef JMEM_PROFILE_SEGMENT_UTILIZATION__ABSOLUTE
    fprintf(fp, ", %5d / %5d", segment->occupied_size, segment->total_size);
#else
    float utilization =
        (float)segment->occupied_size / (float)segment->total_size * 100.0f;
    fprintf(fp, ", %2.02f", utilization);
#endif
  }
  fprintf(fp, "\n");

#if defined(JMEM_PROFILE_SEGMENT_UTILIZATION_FILENAME)
  fflush(fp);
  fclose(fp);
#endif
#else
  UNUSED(header);
  UNUSED(jsobject_size);
#endif /* defined(JMEM_SEGMENTED_HEAP) &&defined(JMEM_PROFILE) && \
        * defined(JMEM_PROFILE_SEGMENT_UTILIZATION)               \
        */
}