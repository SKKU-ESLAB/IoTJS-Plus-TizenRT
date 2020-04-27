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

#endif /* !defined(JMEM_PROFILER_COMMON_H) */