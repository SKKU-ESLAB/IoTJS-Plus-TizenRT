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

static void __print_total_size_profile(void);

/* Total size profiling */
inline void __attr_always_inline___ init_size_profiler(void) {
#if defined(JMEM_PROFILE)
  CHECK_LOGGING_ENABLED();
#if defined(PROF_SIZE__PERIOD_USEC)
  FILE *fp1 = fopen(PROF_TOTAL_SIZE_FILENAME, "w");
  fprintf(fp1,
          "Timestamp (s), Blocks Size (B), Full-bitwidth Pointer Overhead (B), "
          "Allocated Heap Size (B), System Allocator Metadata Size (B), "
          "Segment Metadata Size (B), Snapshot Size (B)\n");
  fflush(fp1);
  fclose(fp1);
  JERRY_CONTEXT(jsuptime_recent_total_size_print).tv_sec = 0;
  JERRY_CONTEXT(jsuptime_recent_total_size_print).tv_usec = 0;
#endif
#endif
}

inline void __attr_always_inline___ print_total_size_profile_on_alloc(void) {
#if defined(PROF_SIZE__PERIOD_USEC)
  CHECK_LOGGING_ENABLED();
  struct timeval js_uptime;
  get_js_uptime(&js_uptime);
  long timeval_diff_in_usec =
      get_timeval_diff_usec(&JERRY_CONTEXT(jsuptime_recent_total_size_print),
                            &js_uptime);
  if (timeval_diff_in_usec > PROF_SIZE__PERIOD_USEC) {
    JERRY_CONTEXT(jsuptime_recent_total_size_print) = js_uptime;
    __print_total_size_profile();
  }
#else
  __print_total_size_profile();
#endif /* defined(PROF_SIZE__PERIOD_USEC) */
}

inline void __attr_always_inline___ print_total_size_profile_finally(void) {
  CHECK_LOGGING_ENABLED();
  __print_total_size_profile();
}

inline void __attr_always_inline___ __print_total_size_profile(void) {
#if defined(JMEM_PROFILE) && defined(PROF_SIZE)
  CHECK_LOGGING_ENABLED();
  FILE *fp = fopen(PROF_TOTAL_SIZE_FILENAME, "a");

  struct timeval js_uptime;
  get_js_uptime(&js_uptime);
  uint32_t blocks_size = (uint32_t)JERRY_CONTEXT(jmem_heap_blocks_size);
  uint32_t full_bw_oh =
      (uint32_t)JERRY_CONTEXT(jmem_full_bitwidth_pointer_overhead);
  uint32_t alloc_heap_size = (uint32_t)JERRY_CONTEXT(jmem_allocated_heap_size);
  uint32_t sysalloc_meta_size =
      (uint32_t)JERRY_CONTEXT(jmem_system_allocator_metadata_size);
  uint32_t segment_meta_size =
      (uint32_t)JERRY_CONTEXT(jmem_segment_allocator_metadata_size);
  uint32_t snapshot_size = (uint32_t)JERRY_CONTEXT(jmem_snapshot_size);

  fprintf(fp, "%lu.%06lu, %lu, %lu, %lu, %lu, %lu, %lu\n", js_uptime.tv_sec,
          js_uptime.tv_usec, blocks_size, full_bw_oh, alloc_heap_size,
          sysalloc_meta_size, segment_meta_size, snapshot_size);
  fflush(fp);
  fclose(fp);
#endif
}