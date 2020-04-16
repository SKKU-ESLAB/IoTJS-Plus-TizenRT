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

#ifndef JMEM_HEAP_PROFILER_H
#define JMEM_HEAP_PROFILER_H

#include "jcontext.h"
#include "jmem-config.h"
#include "jrt.h"

#include <sys/time.h>

/* Initialize the profiling start time */
extern void init_size_profiler(void);

/* Total size profiling: configurable each_time */
extern void print_total_size_profile_on_alloc(void);
extern void print_total_size_profile_finally(void);

/* Segment utilization profiling */
extern void print_segment_utilization_profile_after_free(size_t jsobject_size);
extern void print_segment_utilization_profile_before_segalloc(size_t jsobject_size);
extern void print_segment_utiliaztion_profile_before_gc(size_t jsobject_size);
extern void print_segment_utiliaztion_profile_after_gc(size_t jsobject_size);
extern void print_segment_utilization_profile_finally(void);

#endif /* !defined(JMEM_HEAP_PROFILER_H) */