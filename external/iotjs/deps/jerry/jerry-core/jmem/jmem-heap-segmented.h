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

#ifndef JMEM_HEAP_SEGMENTED_H
#define JMEM_HEAP_SEGMENTED_H

#include "jcontext.h"
#include "jmem-config.h"
#include "jmem-heap-segmented-rb.h"
#include "jrt.h"

#ifdef JMEM_SEGMENTED_HEAP
extern void jmem_segmented_init_segments(void);
extern void *jmem_segmented_alloc_a_segment(const size_t size, bool is_alloc_before);

extern void *jmem_heap_add_segment(bool is_two_segs);
extern void free_empty_segments(void);

typedef struct {
  // If Heap DMUX is ON, this points to the heap segment, otherwise NULL
  uint8_t *heap_segment;
  // jmem_tree_node_t *root;
} jmem_merge_tree_t;
#endif /* JMEM_SEGMENTED_HEAP */

#endif /* !defined(JMEM_HEAP_SEGMENTED_H) */