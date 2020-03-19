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

#ifndef JMEM_PROFILER_COMMON_H
#define JMEM_PROFILER_COMMON_H

#include "jcontext.h"
#include "jmem-config.h"
#include "jrt.h"

#define CHECK_LOGGING_ENABLED()                                         \
  if (!(JERRY_CONTEXT(jerry_init_flags) & ECMA_INIT_JMEM_LOGS_ENABLED)) \
  return

#endif /* !defined(JMEM_PROFILER_COMMON_H) */