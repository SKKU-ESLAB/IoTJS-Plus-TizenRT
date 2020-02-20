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

#include "jmem-gc-profiler.h"

#define UNUSED(x) (void)(x)

/* Object lifespan profiling */

inline void __attr_always_inline___ profile_gc_inc_total_count(void) {
#if defined(JMEM_PROFILE) && defined(JMEM_PROFILE_OBJECT_LIFESPAN)
  JERRY_HEAP_CONTEXT(gc_total_count)++;
#endif
}

inline void __attr_always_inline___
profile_gc_set_object_birth_time(uintptr_t compressed_pointer) {
#if defined(JMEM_PROFILE) && defined(JMEM_PROFILE_OBJECT_LIFESPAN)
  struct timeval tv;
  gettimeofday(&tv, NULL);
  JERRY_HEAP_CONTEXT(gc_obj_birth)
  [compressed_pointer] = JERRY_HEAP_CONTEXT(gc_total_count);
  JERRY_HEAP_CONTEXT(gc_obj_birth_time)[compressed_pointer][0] = tv.tv_sec;
  JERRY_HEAP_CONTEXT(gc_obj_birth_time)[compressed_pointer][1] = tv.tv_usec;
#else
  UNUSED(compressed_pointer);
#endif
}

inline void __attr_always_inline___
profile_gc_set_object_birth_count(uintptr_t compressed_pointer) {
#if defined(JMEM_PROFILE) && defined(JMEM_PROFILE_OBJECT_LIFESPAN)
  JERRY_HEAP_CONTEXT(gc_obj_birth)
  [compressed_pointer] = JERRY_HEAP_CONTEXT(gc_total_count);
#else
  UNUSED(compressed_pointer);
#endif
}

inline void __attr_always_inline___
profile_gc_print_object_lifespan(uintptr_t compressed_pointer) {
#if defined(JMEM_PROFILE) && defined(JMEM_PROFILE_OBJECT_LIFESPAN)
  FILE *fp = stdout;
#ifdef JMEM_PROFILE_OBJECT_LIFESPAN_FILENAME
  fp = fopen(JMEM_PROFILE_OBJECT_LIFESPAN_FILENAME, "a");
#endif

  fprintf(fp, "OL, %u, %u, %lu.%lu\n", compressed_pointer,
         JERRY_HEAP_CONTEXT(gc_total_count) - JERRY_HEAP_CONTEXT(gc_obj_birth)[compressed_pointer],
         JERRY_HEAP_CONTEXT(gc_obj_birth_time)[compressed_pointer][0],
         JERRY_HEAP_CONTEXT(gc_obj_birth_time)[compressed_pointer)][1]);
  JERRY_HEAP_CONTEXT(gc_obj_birth)[compressed_pointer] = 0;

#ifdef JMEM_PROFILE_OBJECT_LIFESPAN_FILENAME
  fflush(fp);
  fclose(fp);
#endif
#else
  UNUSED(compressed_pointer);
#endif
}