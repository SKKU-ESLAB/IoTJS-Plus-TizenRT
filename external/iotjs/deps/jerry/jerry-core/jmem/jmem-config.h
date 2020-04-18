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
#define SEG_SEGMENT_SIZE 4096
#define SEG_SEGMENT_SHIFT 12


/* Heap allocation type */
// 1) Real dynamic heap - if JMEM_SYSTEM_ALLOCATOR in CMakeLists.txt is enabled
#define JMEM_SEGMENTED_HEAP // 2) Segmented heap
// #define JMEM_DYNAMIC_HEAP_EMUL // 3) Dynamic heap emulation
// 4) Static heap - else


/* For each allocation type configs */
#if defined(JMEM_SYSTEM_ALLOCATOR)
#define JMEM_DYNAMIC_HEAP_REAL // 1) Real dynamic heap - not used
// no options

#elif defined(JMEM_SEGMENTED_HEAP) // 2) Segmented heap
#define SEG_RMAP_BINSEARCH // binary search for reverse map

#elif defined(JMEM_DYNAMIC_HEAP_EMUL) // 3) Dynamic heap emulation
#define DE_SLAB // dynamic heap emulation with slab segment

#else
#define JMEM_STATIC_HEAP // 4) Static heap
// no options

#endif


/* Profiler configs */
#define JMEM_PROFILE
/* jmem-size-profiler.c */
#define PROF_TOTAL_SIZE
#define PROF_TOTAL_SIZE__PERIOD_USEC (100 * 1000)
// #define PROF_SEGMENT_UTILIZATION
// #define PROF_SEGMENT_UTILIZATION__ABSOLUTE
// // #define PROF_SEGMENT_UTILIZATION__AFTER_FREE_BLOCK
// // #define PROF_SEGMENT_UTILIZATION__BEFORE_ADD_SEGMENT
// #define PROF_SEGMENT_UTILIZATION__BEFORE_GC
// #define PROF_SEGMENT_UTILIZATION__AFTER_GC
// // #define PROF_SEGMENT_UTILIZATION__PERIOD_USEC (100 * 1000)

/* jmem-time-profiler.c */
#define PROF_TIME

/* jmem-jsobject-profiler.c */
// #define PROF_JSOBJECT_LIFESPAN
// #define PROF_JSOBJECT_ALLOCATION
// #define PROF_JSOBJECT_ALLOCATION__MAX_SIZE 1024 // 8B ~ 1024B

/* Profiler output configs */
/* If config is defined, output is stored to the specified file.
 * Otherwise, output is printed to stdout.
 */
#define PROF_MODE_ARTIK053

#ifdef PROF_MODE_ARTIK053
#define PROF_TOTAL_SIZE_FILENAME "/mnt/total_size.log"
#define PROF_SEGMENT_UTILIZATION_FILENAME "/mnt/segment_utilization.log"
#define PROF_TIME_FILENAME "/mnt/time.log"
#define PROF_JSOBJECT_LIFESPAN_FILENAME "/mnt/object_lifespan.log"
#define PROF_JSOBJECT_ALLOCATION_FILENAME "/mnt/object_allocation.log"
#else
#define PROF_TOTAL_SIZE_FILENAME "total_size.log"
#define PROF_SEGMENT_UTILIZATION_FILENAME "segment_utilization.log"
#define PROF_TIME_FILENAME "time.log"
#define PROF_JSOBJECT_LIFESPAN_FILENAME "object_lifespan.log"
#define PROF_JSOBJECT_ALLOCATION_FILENAME "object_allocation.log"
#endif

#endif /* !JMEM_CONFIG_H */