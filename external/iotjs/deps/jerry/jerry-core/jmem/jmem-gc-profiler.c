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
#ifdef JMEM_PROFILE_OBJECT_LIFESPAN
static unsigned g_gc_total_count = 0;
static unsigned g_gc_obj_birth[65536] = {
    0,
};
static long g_gc_obj_birth_time[65536][2];
#endif

inline void __attr_always_inline___ profile_gc_inc_total_count() {
#ifdef JMEM_PROFILE_OBJECT_LIFESPAN
  g_gc_total_count++;
#endif
}

inline void __attr_always_inline___
profile_gc_set_object_birth_time(uintptr_t compressed_pointer) {
#ifdef JMEM_PROFILE_OBJECT_LIFESPAN
  struct timeval tv;
  gettimeofday(&tv, NULL);
  g_gc_obj_birth[compressed_pointer] = g_gc_total_count;
  g_gc_obj_birth_time[compressed_pointer][0] = tv.tv_sec;
  g_gc_obj_birth_time[compressed_pointer][1] = tv.tv_usec;
#else
  UNUSED(compressed_pointer);
#endif
}

inline void __attr_always_inline___
profile_gc_set_object_birth_count(uintptr_t compressed_pointer) {
#ifdef JMEM_PROFILE_OBJECT_LIFESPAN
  g_gc_obj_birth[compressed_pointer] = g_gc_total_count;
#else
  UNUSED(compressed_pointer);
#endif
}

inline void __attr_always_inline___
profile_gc_print_object_lifespan(uintptr_t compressed_pointer) {
#ifdef JMEM_PROFILE_OBJECT_LIFESPAN
  printf("%u\t%u\t%lu.%lu\n", compressed_pointer,
         g_gc_total_count - g_gc_obj_birth[compressed_pointer],
         g_gc_obj_birth_time[compressed_pointer][0],
         g_gc_obj_birth_time[compressed_pointer)][1]);
  g_gc_obj_birth[compressed_pointer] = 0;
#else
  UNUSED(compressed_pointer);
#endif
}