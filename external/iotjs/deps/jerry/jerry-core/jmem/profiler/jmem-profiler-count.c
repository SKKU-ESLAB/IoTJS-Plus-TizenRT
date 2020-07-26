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

/* Count profiling */
inline void __attr_always_inline___ init_count_profile(void) {
#if defined(PROF_COUNT)
  CHECK_LOGGING_ENABLED();
  for (int type = 0; type < PROF_COUNT__MAX_TYPES; type++) {
    JERRY_CONTEXT(prof_count[type]) = 0;
  }
#endif /* defined(PROF_COUNT) */
}

inline void __attr_always_inline___ print_count_profile(void) {
#if defined(PROF_COUNT)
  CHECK_LOGGING_ENABLED();
  FILE *fp = fopen(PROF_COUNT_FILENAME, "a");
  for (int type = 0; type < PROF_COUNT__MAX_TYPES; type++) {
    if (type > 0) {
      fprintf(fp, ",");
    }
    fprintf(fp, "%d", JERRY_CONTEXT(prof_count[type]));
  }
  fprintf(fp, "\n");

  fflush(fp);
  fclose(fp);
#endif /* defined(PROF_COUNT) */
}
inline void __attr_always_inline___ profile_inc_count_compression_callers(int type) {
#if defined(PROF_COUNT)
  CHECK_LOGGING_ENABLED();
  JERRY_CONTEXT(prof_count[type])++;
#else  /* defined(PROF_COUNT) */
  JERRY_UNUSED(type);
#endif /* !defined(PROF_COUNT) */
}

inline void __attr_always_inline___ profile_add_count_size_detailed(int type, size_t size) {
#if defined(PROF_COUNT)
  CHECK_LOGGING_ENABLED();
  JERRY_CONTEXT(prof_count[type]) += (int)size;
#else  /* defined(PROF_COUNT) */
  JERRY_UNUSED(type);
  JERRY_UNUSED(size);
#endif /* !defined(PROF_COUNT) */
}