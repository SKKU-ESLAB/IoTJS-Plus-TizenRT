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

#ifndef JMEM_HEAP_SEGMENTED_H
#define JMEM_HEAP_SEGMENTED_H

#include "jcontext.h"
#include "jmem-config.h"
#include "jrt.h"

#ifdef JMEM_SEGMENTED_HEAP
extern void init_segmented_heap(void);
extern void *alloc_a_segment_group(size_t required_size);
extern void free_empty_segment_groups(void);
extern void free_initial_segment_group(void);
#endif /* JMEM_SEGMENTED_HEAP */

#endif /* !defined(JMEM_HEAP_SEGMENTED_H) */