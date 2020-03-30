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

/* Segmented heap allocator */
#define JMEM_SEGMENT_SIZE 8192
#define JMEM_SEGMENT_SHIFT 13

/* Segmented heap allocator configs */
#define JMEM_DYNAMIC_ALLOCATOR_EMULATION
#ifndef JMEM_DYNAMIC_ALLOCATOR_EMULATION
#define JMEM_SEGMENTED_HEAP
#define JMEM_SEGMENT_RMAP_RBTREE

#define JMEM_SEG_ALLOC_BEFORE_GC
#endif

// #define JMEM_AGGRESSIVE_GC // obsolete option

// /* Profiler configs */
#define JMEM_PROFILE
// 
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