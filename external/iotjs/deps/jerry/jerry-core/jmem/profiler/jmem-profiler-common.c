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

inline void __attr_always_inline___ init_profilers(void) {
#if defined(JMEM_PROFILE)
  CHECK_LOGGING_ENABLED();
  gettimeofday(&JERRY_CONTEXT(timeval_js_start), NULL);
  init_size_profiler();
  init_segment_profiler();
  init_time_profiler();
  init_cptl_profiler();
  init_pmu_profiler();
#endif
}

inline void __attr_always_inline___ finalize_profilers(void) {
#if defined(JMEM_PROFILE)
  CHECK_LOGGING_ENABLED();
  print_time_profile();                        /* Time profiling */
  print_total_size_profile_finally();          /* Total size profiling */
  print_segment_utilization_profile_finally(); /* Segment-util profiling */
  print_jsobject_allocation_profile(); /* JS object allocation profiling */
  print_cptl_profile_rmc_hit_ratio();  /* CPTL profiling */
  print_count_profile();               /* Count profiling */
  print_pmu_profile();                 /* PMU profiling */
#endif
}

inline long __attr_always_inline___
get_timeval_diff_usec(struct timeval *prior, struct timeval *post) {
  long timeval_diff_usec = (post->tv_sec - prior->tv_sec) * 1000 * 1000 +
                           (post->tv_usec - prior->tv_usec);
  return timeval_diff_usec;
}

inline void __attr_always_inline___ get_js_uptime(struct timeval *js_uptime) {
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