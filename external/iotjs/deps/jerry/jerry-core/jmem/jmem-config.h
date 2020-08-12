/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
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

#ifndef JMEM_CONFIG_H
#define JMEM_CONFIG_H

#include "jerry-board-config.h"
#include "config.h"

/* Garbage collection type configs */
// #define JMEM_LAZY_GC

/* Segmented heap allocation configs */
#define SEG_SEGMENT_SIZE 2048
#define SEG_SEGMENT_SHIFT 11

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

// #define SEG_FULLBIT_ADDRESS_ALLOC // fullbit-address allocation

// Fast path
#define SEG_RMAP_CACHE             // caching in reverse map
// Slow path
#define SEG_RMAP_BINSEARCH         // binary search for reverse map
// #define SEG_RMAP_2LEVEL_SEARCH     // 2-level search for reverse map

#elif defined(JMEM_DYNAMIC_HEAP_EMUL) // 3) Dynamic heap emulation
#define DE_SLAB // dynamic heap emulation with slab segment

#else
#define JMEM_STATIC_HEAP // 4) Static heap
// no options

#endif

/* Reverse map caching config */
#ifdef SEG_RMAP_CACHE
#define SEG_RMAP_CACHE_SIZE 16     // cache size (unit: # of entries)
#define SEG_RMAP_CACHE_SET_SIZE 1 // set size (unit: # of entries)
#define SEG_RMAP_CACHE_WAYS (SEG_RMAP_CACHE_SIZE / SEG_RMAP_CACHE_SET_SIZE)
#endif /* defined(SEG_RMAP_CACHE) */

/* 2-level search config */
#ifdef SEG_RMAP_2LEVEL_SEARCH
#define SEG_RMAP_2LEVEL_SEARCH_FIFO_CACHE_SIZE 4 // FIFO cache size
#endif /* defined(SEG_RMAP_2LEVEL_SEARCH) */

/* Profiler configs */
#define JMEM_PROFILE

#ifdef JMEM_PROFILE
// #define PROF_SIZE
#define PROF_TIME
// #define PROF_PMU
// #define PROF_CPTL // It may degrade performance
// #define PROF_SEGMENT // It may degrade performance
// #define PROF_JSOBJECT // It may degrade performance
#define PROF_COUNT // It may degrade performance

/* jmem-profiler-size.c */
#ifdef PROF_SIZE
// #define PROF_SIZE__PERIOD_USEC (100 * 1000) // It may degrade performance harshly
#define PROF_SIZE__PERIOD_USEC (1000 * 1000) // It may degrade performance harshly
#endif /* defined(PROF_SIZE) */

/* jmem-profiler-time.c */
#ifdef PROF_TIME

// #define PROF_TIME__ALLOC         // It may degrade performance
// #define PROF_TIME__FREE          // It may degrade performance
// #define PROF_TIME__GC // It may degrade performance
// #define PROF_TIME__DECOMPRESSION // It may degrade performance
// #define PROF_TIME__COMPRESSION   // It may degrade performance

#ifdef PROF_TIME__COMPRESSION
#define PROF_TIME__COMPRESSION_DETAILED
#endif

#endif /* defined(PROF_TIME) */

/* jmem-profiler-pmu.c */
#ifdef PROF_PMU
#define PROF_PMU__DECOMPRESSION_CYCLES // It may degrade performance
#define PROF_PMU__COMPRESSION_CYCLES   // It may degrade performance
#endif

#ifdef PROF_PMU__COMPRESSION_CYCLES
#define PROF_PMU__COMPRESSION_CYCLES_DETAILED
#endif

/* jmem-profiler-cptl.c */
#ifdef PROF_CPTL
// #define PROF_CPTL_RMC_HIT_RATIO
#define PROF_CPTL_ACCESS
// #define PROF_CPTL_ACCESS_SIMUL // sidx + de-/comp + hit/miss + miss penalty

#ifdef PROF_CPTL_ACCESS_SIMUL
#define PROF_CPTL_ACCESS_SIMUL_SET_SIZE 2
#define PROF_CPTL_ACCESS_SIMUL_LRU // LRU or FIFO
#endif
#endif /* defined(PROF_CPTL) */

/* jmem-profiler-segment.c */
#ifdef PROF_SEGMENT
#define PROF_SEGMENT_UTILIZATION__ABSOLUTE
// #define PROF_SEGMENT_UTILIZATION__AFTER_FREE_BLOCK
// #define PROF_SEGMENT_UTILIZATION__BEFORE_ADD_SEGMENT
#define PROF_SEGMENT_UTILIZATION__BEFORE_GC
#define PROF_SEGMENT_UTILIZATION__AFTER_GC
// #define PROF_SEGMENT_UTILIZATION__PERIOD_USEC (100 * 1000)
#endif /* defined(PROF_SEGMENT) */

/* jmem-profiler-jsobject.c */
#ifdef PROF_JSOBJECT
#define PROF_JSOBJECT_ALLOCATION
#define PROF_JSOBJECT_ALLOCATION__MAX_SIZE 1024
#endif /* defined(PROF_JSOBJECT) */

/* jmem-profiler-count.c */
#ifdef PROF_COUNT
#define PROF_COUNT__MAX_TYPES 10 // default type count

// #define PROF_COUNT__COMPRESSION_CALLERS
#define PROF_COUNT__SIZE_DETAILED // it requires PROF_SIZE and PROF_SIZE__PERIOD_USEC

// Compression callers
#ifdef PROF_COUNT__COMPRESSION_CALLERS
#undef PROF_COUNT__MAX_TYPES
#define PROF_COUNT__MAX_TYPES 19
#endif

// Size detailed
#ifdef PROF_COUNT__SIZE_DETAILED
#undef PROF_COUNT__MAX_TYPES
#define PROF_COUNT__MAX_TYPES 33
#endif

#endif

#endif /* defined(JMEM_PROFILE) */

#endif /* !defined(JMEM_CONFIG_H) */
