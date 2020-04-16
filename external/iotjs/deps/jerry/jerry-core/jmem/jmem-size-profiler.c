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

#include "jmem-profiler-common.h"
#include "jmem-size-profiler.h"

static void __get_js_uptime(struct timeval *js_uptime);
static long __get_timeval_diff_usec(struct timeval *prior,
                                    struct timeval *post);
static void __print_total_size_profile(void);
static void __print_segment_utilization_profile(const char *type,
                                                size_t jsobject_size);

inline void __attr_always_inline___ init_size_profiler(void) {
#if defined(JMEM_PROFILE)
  CHECK_LOGGING_ENABLED();
  gettimeofday(&JERRY_CONTEXT(timeval_js_start), NULL);
#if defined(PROF_TOTAL_SIZE__PERIOD_USEC)
  FILE *fp1 = fopen(PROF_TOTAL_SIZE_FILENAME, "w");
  fprintf(fp1,
          "Timestamp (s), Blocks Size (B), Full-bitwidth Pointer Overhead (B), "
          "Over-provision Overhead (B), Allocated Heap Size (B), "
          "System Allocator Metadata Size (B), Segment Metadata Size (B), "
          "Total Size (B)\n");
  fflush(fp1);
  fclose(fp1);
  JERRY_CONTEXT(jsuptime_recent_total_size_print).tv_sec = 0;
  JERRY_CONTEXT(jsuptime_recent_total_size_print).tv_usec = 0;
#endif
#if defined(PROF_SEGMENT_UTILIZATION__PERIOD_USEC)
  FILE *fp2 = fopen(PROF_SEGMENT_UTILIZATION_FILENAME, "w");
  fprintf(fp2, "Timestamp (s), JSObject Size (s)");
  for (uint32_t segment_idx = 0; segment_idx < SEG_NUM_SEGMENTS;
       segment_idx++) {
    fprintf(fp2, ", S%lu", (unsigned long)segment_idx);
  }
  fprintf(fp2, "\n");
  fflush(fp2);
  fclose(fp2);
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
#if defined(JMEM_PROFILE)
  gettimeofday(js_uptime, NULL);

  if (js_uptime->tv_usec < JERRY_CONTEXT(timeval_js_start).tv_usec) {
    js_uptime->tv_usec += 1000000;
    js_uptime->tv_sec -= 1;
  }
  js_uptime->tv_usec -= JERRY_CONTEXT(timeval_js_start).tv_usec;
  js_uptime->tv_sec -= JERRY_CONTEXT(timeval_js_start).tv_sec;
#else
  JERRY_UNUSED(js_uptime);
#endif
}

/* Total size profiling */


inline void __attr_always_inline___ print_total_size_profile_on_alloc(void) {
#if defined(PROF_TOTAL_SIZE__PERIOD_USEC)
  CHECK_LOGGING_ENABLED();
  struct timeval js_uptime;
  __get_js_uptime(&js_uptime);
  long timeval_diff_in_usec =
      __get_timeval_diff_usec(&JERRY_CONTEXT(jsuptime_recent_total_size_print),
                              &js_uptime);
  if (timeval_diff_in_usec > PROF_TOTAL_SIZE__PERIOD_USEC) {
    JERRY_CONTEXT(jsuptime_recent_total_size_print) = js_uptime;
    __print_total_size_profile();
  }
#else
  __print_total_size_profile();
#endif /* defined(PROF_TOTAL_SIZE__PERIOD_USEC) */
}

inline void __attr_always_inline___ print_total_size_profile_finally(void) {
  CHECK_LOGGING_ENABLED();
  __print_total_size_profile();
}

inline void __attr_always_inline___ __print_total_size_profile(void) {
#if defined(JMEM_PROFILE) && defined(PROF_TOTAL_SIZE)
  CHECK_LOGGING_ENABLED();
  FILE *fp = fopen(PROF_TOTAL_SIZE_FILENAME, "a");

  struct timeval js_uptime;
  __get_js_uptime(&js_uptime);
  uint32_t blocks_size = (uint32_t)JERRY_CONTEXT(jmem_heap_blocks_size);
  uint32_t full_bw_oh =
      (uint32_t)JERRY_CONTEXT(jmem_full_bitwidth_pointer_overhead);
  uint32_t alloc_heap_size = (uint32_t)JERRY_CONTEXT(jmem_allocated_heap_size);
  uint32_t overprovision_oh = alloc_heap_size - blocks_size - full_bw_oh;
  uint32_t sysalloc_meta_size =
      (uint32_t)JERRY_CONTEXT(jmem_system_allocator_metadata_size);
  uint32_t segment_meta_size =
      (uint32_t)JERRY_CONTEXT(jmem_segment_allocator_metadata_size);
  uint32_t total_size =
      alloc_heap_size + sysalloc_meta_size + segment_meta_size;

  fprintf(fp, "%lu.%06lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu\n",
          js_uptime.tv_sec, js_uptime.tv_usec, blocks_size, full_bw_oh,
          overprovision_oh, alloc_heap_size, sysalloc_meta_size,
          segment_meta_size, total_size);
  fflush(fp);
  fclose(fp);
#endif
}

/* Segment utilization profiling */
inline void __attr_always_inline___
print_segment_utilization_profile_after_free(size_t jsobject_size) {
#if defined(PROF_SEGMENT_UTILIZATION__AFTER_FREE_BLOCK)
#if defined(PROF_SEGMENT_UTILIZATION__PERIOD_USEC)
  CHECK_LOGGING_ENABLED();
  struct timeval js_uptime;
  __get_js_uptime(&js_uptime);
  long timeval_diff_in_usec =
      __get_timeval_diff_usec(&JERRY_CONTEXT(jsuptime_recent_segutil_print),
                              &js_uptime);
  if (timeval_diff_in_usec > PROF_SEGMENT_UTILIZATION__PERIOD_USEC) {
    JERRY_CONTEXT(jsuptime_recent_segutil_print) = js_uptime;
    __print_segment_utilization_profile("AFB", jsobject_size);
  }
#else
  __print_segment_utilization_profile("AFB", jsobject_size);
#endif /* defined(PROF_SEGMENT_UTILIZATION__PERIOD_USEC) */
#else
  JERRY_UNUSED(jsobject_size);
#endif
}

inline void __attr_always_inline___
print_segment_utilization_profile_before_segalloc(size_t jsobject_size) {
#if defined(PROF_SEGMENT_UTILIZATION__BEFORE_ADD_SEGMENT)
  CHECK_LOGGING_ENABLED();
  __print_segment_utilization_profile("BAS", jsobject_size);
#else
  JERRY_UNUSED(jsobject_size);
#endif
}

inline void __attr_always_inline___
print_segment_utiliaztion_profile_before_gc(size_t jsobject_size) {
#if defined(PROF_SEGMENT_UTILIZATION__BEFORE_GC)
  CHECK_LOGGING_ENABLED();
  __print_segment_utilization_profile("BGC", jsobject_size);
#else
  JERRY_UNUSED(jsobject_size);
#endif
}

inline void __attr_always_inline___
print_segment_utiliaztion_profile_after_gc(size_t jsobject_size) {
#if defined(PROF_SEGMENT_UTILIZATION__AFTER_GC)
  CHECK_LOGGING_ENABLED();
  __print_segment_utilization_profile("AGC", jsobject_size);
#else
  JERRY_UNUSED(jsobject_size);
#endif
}

inline void __attr_always_inline___
print_segment_utilization_profile_finally(void) {
  CHECK_LOGGING_ENABLED();
  __print_segment_utilization_profile("F", 0);
}

inline void __attr_always_inline___
__print_segment_utilization_profile(const char *header, size_t jsobject_size) {
#if defined(JMEM_SEGMENTED_HEAP) && defined(JMEM_PROFILE) && \
    defined(PROF_SEGMENT_UTILIZATION)
  CHECK_LOGGING_ENABLED();
  struct timeval js_uptime;
  __get_js_uptime(&js_uptime);

  FILE *fp = fopen(PROF_SEGMENT_UTILIZATION_FILENAME, "a");
  fprintf(fp, "%s %lu, %lu.%06lu", header, (unsigned long)jsobject_size,
          js_uptime.tv_sec, js_uptime.tv_usec);

  for (uint32_t segment_idx = 0; segment_idx < SEG_NUM_SEGMENTS;
       segment_idx++) {
    jmem_segment_t *segment = &(JERRY_HEAP_CONTEXT(segments[segment_idx]));
#ifdef PROF_SEGMENT_UTILIZATION__ABSOLUTE
    fprintf(fp, ", %5d / %5d", segment->occupied_size, segment->total_size);
#else
    float utilization =
        (float)segment->occupied_size / (float)segment->total_size * 100.0f;
    fprintf(fp, ", %2.02f", utilization);
#endif
  }
  fprintf(fp, "\n");

  fflush(fp);
  fclose(fp);
#else
  JERRY_UNUSED(header);
  JERRY_UNUSED(jsobject_size);
#endif /* defined(JMEM_SEGMENTED_HEAP) &&defined(JMEM_PROFILE) && \
        * defined(PROF_SEGMENT_UTILIZATION)                       \
        */
}