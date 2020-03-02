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

/* Segmented heap allocator configs */
#define JMEM_SEGMENTED_HEAP
#define SEG_ALLOC_BEFORE_GC
#define JMEM_SEGMENT_RMAP_RBTREE

/* Profiler configs */
#define JMEM_PROFILE
// #define JMEM_PROFILE_TOTAL_SIZE
// #define JMEM_PROFILE_SEGMENT_UTILIZATION
// #define JMEM_PROFILE_SEGMENT_UTILIZATION_ABSOLUTE
#define JMEM_PROFILE_TIME
//#define JMEM_PROFILE_OBJECT_LIFESPAN

/* Profiler output configs */
/* If config is defined, output is stored to the specified file.
 * Otherwise, output is printed to stdout.
 */
#define JMEM_PROFILE_TIZENRT

#ifdef JMEM_PROFILE_TIZENRT
// #define JMEM_PROFILE_TOTAL_SIZE_FILENAME "/mnt/total_size.log"
// #define JMEM_PROFILE_SEGMENT_UTILIZATION_FILENAME "/mnt/segment_utilization.log"
// #define JMEM_PROFILE_TIME_FILENAME "/mnt/time.log"
// #define JMEM_PROFILE_OBJECT_LIFESPAN_FILENAME "/mnt/object_lifespan.log"
#else
#define JMEM_PROFILE_TOTAL_SIZE_FILENAME "total_size.log"
#define JMEM_PROFILE_SEGMENT_UTILIZATION_FILENAME "segment_utilization.log"
#define JMEM_PROFILE_TIME_FILENAME "time.log"
#define JMEM_PROFILE_OBJECT_LIFESPAN_FILENAME "object_lifespan.log"
#endif

#endif /* !JMEM_CONFIG_H */