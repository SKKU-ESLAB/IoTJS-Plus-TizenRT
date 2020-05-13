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

static void __print_segment_utilization_profile(const char *type,
                                                size_t jsobject_size);

inline void __attr_always_inline___ init_segment_profiler(void) {
#if defined(JMEM_PROFILE)
  CHECK_LOGGING_ENABLED();
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

inline void __attr_always_inline___
print_segment_utilization_profile_after_free(size_t jsobject_size) {
#if defined(PROF_SEGMENT_UTILIZATION__AFTER_FREE_BLOCK)
#if defined(PROF_SEGMENT_UTILIZATION__PERIOD_USEC)
  CHECK_LOGGING_ENABLED();
  struct timeval js_uptime;
  get_js_uptime(&js_uptime);
  long timeval_diff_in_usec =
      get_timeval_diff_usec(&JERRY_CONTEXT(jsuptime_recent_segutil_print),
                            &js_uptime);
  if (timeval_diff_in_usec > PROF_SEGMENT_UTILIZATION__PERIOD_USEC) {
    JERRY_CONTEXT(jsuptime_recent_segutil_print) = js_uptime;
    __print_segment_utilization_profile("AFB", jsobject_size);
  }
#else  /* defined(PROF_SEGMENT_UTILIZATION__PERIOD_USEC) */
  __print_segment_utilization_profile("AFB", jsobject_size);
#endif /* !defined(PROF_SEGMENT_UTILIZATION__PERIOD_USEC) */
#else  /* defined(PROF_SEGMENT_UTILIZATION__AFTER_FREE_BLOCK) */
  JERRY_UNUSED(jsobject_size);
#endif /* !defined(PROF_SEGMENT_UTILIZATION__AFTER_FREE_BLOCK) */
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
    defined(PROF_SEGMENT)
  CHECK_LOGGING_ENABLED();
  struct timeval js_uptime;
  get_js_uptime(&js_uptime);

  FILE *fp = fopen(PROF_SEGMENT_UTILIZATION_FILENAME, "a");
  fprintf(fp, "%s %lu, %lu.%06lu", header, (unsigned long)jsobject_size,
          js_uptime.tv_sec, js_uptime.tv_usec);

  for (uint32_t segment_idx = 0; segment_idx < SEG_NUM_SEGMENTS;
       segment_idx++) {
    jmem_segment_t *segment = &(JERRY_HEAP_CONTEXT(segments[segment_idx]));
#ifdef PROF_SEGMENT_UTILIZATION__ABSOLUTE
    if (JERRY_HEAP_CONTEXT(area[segment_idx]) != NULL) {
      fprintf(fp, ", %5d", segment->occupied_size);
    } else {
      fprintf(fp, ", -");
    }
#else
    if (JERRY_HEAP_CONTEXT(area[segment_idx]) != NULL) {
      float utilization =
          (float)segment->occupied_size / (float)SEG_SEGMENT_SIZE * 100.0f;
      fprintf(fp, ", %2.02f", utilization);
    } else {
      fprintf(fp, ", -");
    }
#endif /* defined(PROF_SEGMENT_UTILIZATION__ABSOLUTE) */
  }
  fprintf(fp, "\n");

  fflush(fp);
  fclose(fp);
#else  /* defined(JMEM_SEGMENTED_HEAP) && defined(JMEM_PROFILE) && \
        * defined(PROF_SEGMENT)                        \
        */
  JERRY_UNUSED(header);
  JERRY_UNUSED(jsobject_size);
#endif /* !defined(JMEM_SEGMENTED_HEAP) || !defined(JMEM_PROFILE) || \
        * !defined(PROF_SEGMENT)                         \
        */
}