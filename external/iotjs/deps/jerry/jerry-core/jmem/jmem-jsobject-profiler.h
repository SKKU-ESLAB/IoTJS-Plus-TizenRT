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

#ifndef JMEM_GC_PROFILER_H
#define JMEM_GC_PROFILER_H

#include "jmem-config.h"
#include "jrt.h"

/* JS object lifespan profiling */
extern void profile_jsobject_inc_total_count(void);
extern void profile_jsobject_set_object_birth_time(uintptr_t compressed_pointer);
extern void profile_jsobject_set_object_birth_count(uintptr_t compressed_pointer);
extern void profile_jsobject_print_object_lifespan(uintptr_t compressed_pointer);

/* JS object allocation profiling */
extern void profile_jsobject_inc_allocation(size_t jsobject_size);
extern void profile_jsobject_print_allocation(void);

#endif /* !defined(JMEM_GC_PROFILER_H) */