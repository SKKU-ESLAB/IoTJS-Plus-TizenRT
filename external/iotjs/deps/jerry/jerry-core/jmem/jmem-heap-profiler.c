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
inline void __attr_always_inline___ profile_set_init_time(void) {
#if defined(JMEM_PROFILE)
  gettimeofday(&JERRY_CONTEXT(timeval_init), NULL);
#endif
}
void _get_curr_time(struct timeval *curr_time);
inline void __attr_always_inline___ _get_curr_time(struct timeval *curr_time) {
  gettimeofday(curr_time, NULL);

  if (curr_time->tv_usec < JERRY_CONTEXT(timeval_init).tv_usec) {
    curr_time->tv_usec += 1000000;
    curr_time->tv_sec -= 1;
  }
  curr_time->tv_usec -= JERRY_CONTEXT(timeval_init).tv_usec;
  curr_time->tv_sec -= JERRY_CONTEXT(timeval_init).tv_sec;
}

inline void __attr_always_inline___ profile_print_total_size(void) {
#if defined(JMEM_PROFILE) && defined(JMEM_PROFILE_TOTAL_SIZE)
  FILE *fp = stdout;
#ifdef JMEM_PROFILE_TOTAL_SIZE_FILENAME
  fp = fopen(JMEM_PROFILE_TOTAL_SIZE_FILENAME, "a");
#endif

  struct timeval curr_time;
  _get_curr_time(&curr_time);

  uint32_t segments_in_bytes = 0;
#ifdef JMEM_SEGMENTED_HEAP
  segments_in_bytes =
      (uint32_t)JERRY_HEAP_CONTEXT(segments_count) * JMEM_SEGMENT_SIZE;
#endif
  fprintf(fp, "TS, %lu.%06lu, %lu, %lu\n", curr_time.tv_sec, curr_time.tv_usec,
          (uint32_t)JERRY_CONTEXT(jmem_heap_allocated_size), segments_in_bytes);
#ifdef JMEM_PROFILE_TOTAL_SIZE_FILENAME
  fflush(fp);
  fclose(fp);
#endif
#endif
}

/* Segment utilization profiling */
inline void __attr_always_inline___ profile_print_segment_utilization(void) {
#if defined(JMEM_SEGMENTED_HEAP)
#if defined(JMEM_PROFILE) && defined(JMEM_PROFILE_SEGMENT_UTILIZATION)
  FILE *fp = stdout;
#ifdef JMEM_PROFILE_SEGMENT_UTILIZATION_FILENAME
  fp = fopen(JMEM_PROFILE_SEGMENT_UTILIZATION_FILENAME, "a");
#endif

  struct timeval curr_time;
  _get_curr_time(&curr_time);
  fprintf(fp, "SU, %lu.%06lu", curr_time.tv_sec, curr_time.tv_usec);

  for (uint32_t segment_idx = 0; segment_idx < JMEM_SEGMENT; segment_idx++) {
    jmem_segment_t *segment = &(JERRY_HEAP_CONTEXT(segments[segment_idx]));
#ifdef JMEM_PROFILE_SEGMENT_UTILIZATION_ABSOLUTE
    fprintf(fp, ", %5d/%5d", segment->occupied_size, segment->total_size);
#else
    float utilization =
        (float)segment->occupied_size / (float)segment->total_size * 100.0f;
    fprintf(fp, ", %2.02f", utilization);
#endif
  }
  fprintf(fp, "\n");
#endif

#ifdef JMEM_PROFILE_SEGMENT_UTILIZATION_FILENAME
  fflush(fp);
  fclose(fp);
#endif
#endif
}