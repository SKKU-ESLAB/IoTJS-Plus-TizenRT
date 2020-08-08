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

#ifndef JMEM_PROFILER_H
#define JMEM_PROFILER_H

#include "jrt.h"

extern void init_profilers(void);
extern void finalize_profilers(void);

extern void set_extern_heap_size_ptr(size_t*);

/* jmem-profiler-size.c: size profiling */
extern void print_total_size_profile_on_alloc(void);
extern void print_total_size_profile_finally(void);

/* jmem-profiler-time.c: time profiling */
extern void print_time_profile(void);
extern void profile_alloc_start(void);
extern void profile_alloc_end(void);
extern void profile_free_start(void);
extern void profile_free_end(void);
extern void profile_compression_start(void);
extern void profile_compression_end(int type);
extern void profile_decompression_start(void);
extern void profile_decompression_end(void);
extern void profile_gc_start(void);
extern void profile_gc_end(void);

/* jmem-profiler-pmu.c: PMU profiling */
extern void print_pmu_profile(void);
extern void profile_compression_cycles_start(void);
extern void profile_compression_cycles_end(int type);
extern void profile_decompression_cycles_start(void);
extern void profile_decompression_cycles_end(void);

/* jmem-profiler-segment.c: segment utilization profiling */
extern void print_segment_utilization_profile_after_free(size_t jsobject_size);
extern void print_segment_utilization_profile_before_segalloc(
    size_t jsobject_size);
extern void print_segment_utiliaztion_profile_before_gc(size_t jsobject_size);
extern void print_segment_utiliaztion_profile_after_gc(size_t jsobject_size);
extern void print_segment_utilization_profile_finally(void);

/* jmem-profiler-jsobject.c: JS object allocation profiling */
extern void print_jsobject_allocation_profile(void);
extern void profile_jsobject_inc_allocation(size_t jsobject_size);

/* jmem-profiler-cptl.c: Compressed Pointer Translation Layer (CPTL) profiling
 */
extern void print_cptl_profile_rmc_hit_ratio(void);
extern void profile_inc_rmc_access_count(void);
extern void profile_inc_rmc_miss_count(void);
extern void print_cptl_access(uint32_t sidx, int type_depth);

/* jmem-profiler-count.c : Temporary count profiling for investigation */
extern void print_count_profile(void);
/** PROF_COUNT__COMPRESSION_CALLERS **/
extern void profile_inc_count_compression_callers(int type_miss_penalty);
/** PROF_COUNT__ **/
extern void profile_add_count_size_detailed(int type, size_t size);

#endif /* !defined(JMEM_PROFILER_H) */