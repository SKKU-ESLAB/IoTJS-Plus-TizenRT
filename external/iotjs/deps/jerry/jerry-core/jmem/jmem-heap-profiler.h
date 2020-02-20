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

#ifndef JMEM_HEAP_PROFILER_H
#define JMEM_HEAP_PROFILER_H

#include "jcontext.h"
#include "jmem-config.h"
#include "jrt.h"

#include <sys/time.h>

/* Initialize the profiling start time */
extern void profile_set_init_time(void);

/* Total size profiling */
extern void profile_print_total_size(void);

/* Segment utilization profiling */
extern void profile_print_segment_utilization(void);

/* Time profiling */
extern void profile_init_times(void);
extern void profile_print_times(void);

extern void profile_alloc_start(void);
extern void profile_alloc_end(void);
extern void profile_free_start(void);
extern void profile_free_end(void);
extern void profile_compression_start(void);
extern void profile_compression_end(void);
extern void profile_decompression_start(void);
extern void profile_decompression_end(void);
extern void profile_gc_start(void);
extern void profile_gc_end(void);

#endif /* !defined(JMEM_HEAP_PROFILER_H) */