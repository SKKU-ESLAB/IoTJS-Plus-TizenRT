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

#ifndef JMEM_PROFILER_COMMON_H
#define JMEM_PROFILER_COMMON_H

#include "jcontext.h"
#include "jmem-config.h"
#include "jrt.h"

#define CHECK_LOGGING_ENABLED()                                         \
  if (!(JERRY_CONTEXT(jerry_init_flags) & ECMA_INIT_JMEM_LOGS_ENABLED)) \
  return

/* jmem-profiler-common.c */
extern void get_js_uptime(struct timeval *js_uptime);
extern long get_timeval_diff_usec(struct timeval *prior, struct timeval *post);

/* jmem-profiler-size.c */
extern void init_size_profiler(void);

/* jmem-profiler-segment.c */
extern void init_segment_profiler(void);

/* jmem-profiler-time.c */
extern void init_time_profiler(void);

/* jmem-profiler-pmu.c */
extern void init_pmu_profiler(void);

/* jmem-profiler-cptl.c */
extern void init_cptl_profiler(void);

/* jmem-profiler-count.c */
extern void init_count_profile(void);

/* Profiler output configs */
/* If config is defined, output is stored to the specified file.
 * Otherwise, output is printed to stdout.
 */
#ifdef PROF_MODE_ARTIK053
#define PROF_TOTAL_SIZE_FILENAME "/mnt/total_size.log"
#define PROF_SEGMENT_UTILIZATION_FILENAME "/mnt/segment_utilization.log"
#define PROF_TIME_FILENAME "/mnt/time.log"
#define PROF_PMU_FILENAME "/mnt/pmu.log"
#define PROF_JSOBJECT_ALLOCATION_FILENAME "/mnt/object_allocation.log"
#define PROF_CPTL_FILENAME "/mnt/cptl.log"
#define PROF_CPTL_ACCESS_FILENAME "/mnt/cptl_access.log"
#define PROF_COUNT_FILENAME "/mnt/count.log"
#else
#define PROF_TOTAL_SIZE_FILENAME "total_size.log"
#define PROF_SEGMENT_UTILIZATION_FILENAME "segment_utilization.log"
#define PROF_TIME_FILENAME "time.log"
#define PROF_PMU_FILENAME "pmu.log"
#define PROF_JSOBJECT_ALLOCATION_FILENAME "object_allocation.log"
#define PROF_CPTL_FILENAME "cptl.log"
#define PROF_CPTL_ACCESS_FILENAME "cptl_access.log"
#define PROF_COUNT_FILENAME "count.log"
#endif

#endif /* !defined(JMEM_PROFILER_COMMON_H) */