/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
 * Copyright 2016-2020 Gyeonghwan Hong, Eunsoo Park, Sungkyunkwan University
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

#include "jcontext.h"
#include "jmem-profiler-common-internal.h"
#include "jmem-profiler.h"
#include "jmem.h"

/* JS object allocation profiling */
inline void __attr_always_inline___
profile_jsobject_inc_allocation(size_t jsobject_size) {
#if defined(JMEM_PROFILE) && defined(PROF_JSOBJECT_ALLOCATION)
  CHECK_LOGGING_ENABLED();
  size_t aligned_size =
      (jsobject_size + JMEM_ALIGNMENT - 1) / JMEM_ALIGNMENT * JMEM_ALIGNMENT;

  uint32_t index = (uint32_t)(aligned_size / JMEM_ALIGNMENT) - 1;
  if (index >= PROF_JSOBJECT_ALLOCATION__MAX_SIZE / JMEM_ALIGNMENT) {
    index = PROF_JSOBJECT_ALLOCATION__MAX_SIZE / JMEM_ALIGNMENT - 1;
    printf("Large JS object allocation: %lu\n", (uint32_t)aligned_size);
  }
  JERRY_CONTEXT(jsobject_count[index])++;
#else
  JERRY_UNUSED(jsobject_size);
#endif
}

inline void __attr_always_inline___ print_jsobject_allocation_profile(void) {
#if defined(JMEM_PROFILE) && defined(PROF_JSOBJECT_ALLOCATION)
  CHECK_LOGGING_ENABLED();
  FILE *fp = fopen(PROF_JSOBJECT_ALLOCATION_FILENAME, "a");

  for (uint32_t index = 0;
       index < PROF_JSOBJECT_ALLOCATION__MAX_SIZE / JMEM_ALIGNMENT; index++) {
    fprintf(fp, "OA, %lu, %lu\n", (unsigned long)((index + 1) * JMEM_ALIGNMENT),
            (unsigned long)(JERRY_CONTEXT(jsobject_count[index])));
  }

  fflush(fp);
  fclose(fp);
#endif
}