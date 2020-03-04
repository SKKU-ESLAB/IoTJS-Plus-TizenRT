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
static void __profile_print_segment_utilization(void);

inline void __attr_always_inline___ profile_set_js_start_time(void) {
#if defined(JMEM_PROFILE)
  gettimeofday(&JERRY_CONTEXT(timeval_js_start), NULL);
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
  __profile_print_total_size();
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
profile_print_segment_utilization_each_time(void) {
  __profile_print_segment_utilization();
}

inline void __attr_always_inline___
profile_print_segment_utilization_finally(void) {
  __profile_print_segment_utilization();
}

inline void __attr_always_inline___ __profile_print_segment_utilization(void) {
#if defined(JMEM_SEGMENTED_HEAP)
#if defined(JMEM_PROFILE) && defined(JMEM_PROFILE_SEGMENT_UTILIZATION)
  struct timeval js_uptime;
  __get_js_uptime(&js_uptime);
#if defined(JMEM_PROFILE_SEGMENT_UTILIZATION__PERIOD_USEC)
  long timeval_diff_in_usec =
      __get_timeval_diff_usec(&JERRY_CONTEXT(jsuptime_recent_segutil_print),
                              &js_uptime);
  if (timeval_diff_in_usec > JMEM_PROFILE_SEGMENT_UTILIZATION__PERIOD_USEC) {
    JERRY_CONTEXT(jsuptime_recent_segutil_print) = js_uptime;
#endif /* defined(JMEM_PROFILE_SEGMENT_UTILIZATION__PERIOD_USEC) */

    FILE *fp = stdout;
#ifdef JMEM_PROFILE_SEGMENT_UTILIZATION_FILENAME
    fp = fopen(JMEM_PROFILE_SEGMENT_UTILIZATION_FILENAME, "a");
#endif
    fprintf(fp, "SU, %lu.%06lu", js_uptime.tv_sec, js_uptime.tv_usec);

    for (uint32_t segment_idx = 0; segment_idx < JMEM_SEGMENT; segment_idx++) {
      jmem_segment_t *segment = &(JERRY_HEAP_CONTEXT(segments[segment_idx]));
#ifdef JMEM_PROFILE_SEGMENT_UTILIZATION__ABSOLUTE
      fprintf(fp, ", %5d/%5d", segment->occupied_size, segment->total_size);
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
#if defined(JMEM_PROFILE_SEGMENT_UTILIZATION__PERIOD_USEC)
  }
#endif /* defined(JMEM_PROFILE_SEGMENT_UTILIZATION__PERIOD_USEC) */
#endif /* defined(JMEM_PROFILE) && defined(JMEM_PROFILE_SEGMENT_UTILIZATION) \
        */
#endif /* defined(JMEM_SEGMENTED_HEAP) */
}