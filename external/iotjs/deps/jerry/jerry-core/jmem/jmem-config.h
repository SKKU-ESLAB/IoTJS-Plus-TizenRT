/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
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

#ifndef JMEM_CONFIG_H
#define JMEM_CONFIG_H

#include "config.h"

/* Segmented heap allocation configs */
#define JMEM_SEGMENT_SIZE 8192
#define JMEM_SEGMENT_SHIFT 13


/* Heap allocation type */
// 1) Dynamic heap - if JMEM_SYSTEM_ALLOCATOR in CMakeLists.txt is enabled
#define JMEM_SEGMENTED_HEAP // 2) Segmented heap
// #define JMEM_DYNAMIC_HEAP_EMUL // 3) Dynamic heap emulation
// 4) Static heap - else


/* For each allocation type configs */
#if defined(JMEM_SYSTEM_ALLOCATOR)

// 1) Dynamic heap
// no options

#elif defined(JMEM_SEGMENTED_HEAP)

// 2) Segmented heap
#define JMEM_SEGMENTED_RMAP_BINSEARCH // binary search for reverse map
#define JMEM_SEGMENTED_SEGALLOC_FIRST // segment alloc at first before GC
// #define JMEM_SEGMENTED_AGGRESSIVE_GC

#elif defined(JMEM_DYNAMIC_HEAP_EMUL)

// 3) Dynamic heap emulation
#define JMEM_DYNAMIC_HEAP_EMUL_SLAB // dynamic heap emulation with slab segment

#else

// 4) Static heap
// no options

#endif


/* Profiler configs */
#define JMEM_PROFILE
/* jmem-heap-profiler.c */
#define JMEM_PROFILE_TOTAL_SIZE
#define JMEM_PROFILE_TOTAL_SIZE__PERIOD_USEC (100 * 1000)
// #define JMEM_PROFILE_SEGMENT_UTILIZATION
// #define JMEM_PROFILE_SEGMENT_UTILIZATION__ABSOLUTE
// // #define JMEM_PROFILE_SEGMENT_UTILIZATION__AFTER_FREE_BLOCK
// #define JMEM_PROFILE_SEGMENT_UTILIZATION__BEFORE_ADD_SEGMENT
// #define JMEM_PROFILE_SEGMENT_UTILIZATION__BEFORE_GC
// #define JMEM_PROFILE_SEGMENT_UTILIZATION__AFTER_GC
// #define JMEM_PROFILE_SEGMENT_UTILIZATION__PERIOD_USEC (100 * 1000)

/* jmem-time-profiler.c */
#define JMEM_PROFILE_TIME

/* jmem-jsobject-profiler.c */
// #define JMEM_PROFILE_JSOBJECT_LIFESPAN
// #define JMEM_PROFILE_JSOBJECT_ALLOCATION
// #define JMEM_PROFILE_JSOBJECT_ALLOCATION__MAX_SIZE 1024 // 8B ~ 1024B

/* Profiler output configs */
/* If config is defined, output is stored to the specified file.
 * Otherwise, output is printed to stdout.
 */
#define JMEM_PROFILE_MODE_ARTIK053

#ifdef JMEM_PROFILE_MODE_ARTIK053
#define JMEM_PROFILE_TOTAL_SIZE_FILENAME "/mnt/total_size.log"
#define JMEM_PROFILE_SEGMENT_UTILIZATION_FILENAME "/mnt/segment_utilization.log"
#define JMEM_PROFILE_TIME_FILENAME "/mnt/time.log"
#define JMEM_PROFILE_JSOBJECT_LIFESPAN_FILENAME "/mnt/object_lifespan.log"
#define JMEM_PROFILE_JSOBJECT_ALLOCATION_FILENAME "/mnt/object_allocation.log"
#else
#define JMEM_PROFILE_TOTAL_SIZE_FILENAME "total_size.log"
#define JMEM_PROFILE_SEGMENT_UTILIZATION_FILENAME "segment_utilization.log"
#define JMEM_PROFILE_TIME_FILENAME "time.log"
#define JMEM_PROFILE_JSOBJECT_LIFESPAN_FILENAME "object_lifespan.log"
#define JMEM_PROFILE_JSOBJECT_ALLOCATION_FILENAME "object_allocation.log"
#endif

#endif /* !JMEM_CONFIG_H */