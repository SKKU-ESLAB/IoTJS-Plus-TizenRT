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

#include "jmem-allocator-internal.h"
#include "jmem.h"
#include "jrt.h"

#include <stdlib.h>
#define MALLOC(size) ((void *)malloc(size))
#define FREE(ptr) (free(ptr))

#include "jmem-heap-segmented.h"

#ifdef JMEM_SEGMENTED_HEAP
#define JMEM_HEAP_GET_OFFSET_FROM_PTR(p, seg_ptr) \
  ((uint32_t)((uint8_t *)(p) - (uint8_t *)(seg_ptr)))

/* global variables for segmented heap */
static uint32_t g_last_seg_idx = 0;
uint32_t g_segments_count = 0; /* extern access allowed */

#ifdef JMEM_SEGMENT_RB_LOOKUP
rb_root g_segment_rb_root;
#endif /* JMEM_SEGMENT_RB_LOOKUP */

/* Declaration of internal functions */
static uint8_t *jmem_segment_get_addr(uint32_t segment_idx);
static uint32_t jmem_segment_lookup(uint8_t **seg_addr, uint8_t *p);
static void *jmem_segment_alloc(void);
static void jmem_segment_init(void *seg_ptr, jmem_segment_t *seg_info);
static void *jmem_segment_alloc_init(jmem_segment_t *seg_info);
static void jmem_segment_free(void *seg_ptr);

/* External functions */
void jmem_segmented_init_segments(void) {
  /* Initialize segmented heap */
  JERRY_HEAP_CONTEXT(area[0]) =
      (uint8_t *)jmem_segment_alloc_init((&JERRY_HEAP_CONTEXT(segments[0]))) -
      JMEM_ALIGNMENT;
  g_segments_count++;
  {
    uint32_t segment_idx;
    for (segment_idx = 1; segment_idx < JMEM_SEGMENT; segment_idx++) {
      JERRY_HEAP_CONTEXT(area[segment_idx]) = (uint8_t *)NULL;
    }
  }

  /* Initialzie RB tree of segmented heap */
#ifdef JMEM_SEGMENT_RB_LOOKUP
  g_segment_rb_root.rb_node = NULL;
  seg_node_t *node = (seg_node_t *)MALLOC(sizeof(seg_node_t));
  node->seg_idx = 0;
  node->base_addr = JERRY_HEAP_CONTEXT(area[0]);
  segment_node_insert(&g_segment_rb_root, node);
#endif /* JMEM_SEGMENT_RB_LOOKUP */
}

inline uint32_t __attribute__((hot))
jmem_heap_get_offset_from_addr_segmented(uint8_t *p) {
  uint32_t segment_idx;
  uint8_t *segment_addr;

  JERRY_ASSERT(p != NULL);

  if (p == (uint8_t *)JMEM_HEAP_END_OF_LIST)
    return (uint32_t)JMEM_HEAP_END_OF_LIST_UINT32;

  if (p == (uint8_t *)&JERRY_HEAP_CONTEXT(first))
    return 0;

  segment_idx = jmem_segment_lookup(&segment_addr, p);

  return (uint32_t)(JMEM_HEAP_GET_OFFSET_FROM_PTR(p, segment_addr) +
                    (uint32_t)JMEM_SEGMENT_SIZE * segment_idx);
}
inline jmem_heap_free_t *__attribute__((hot))
jmem_heap_get_addr_from_offset_segmented(uint32_t u) {
  if (u == JMEM_HEAP_END_OF_LIST_UINT32)
    return JMEM_HEAP_END_OF_LIST;
  return (
      jmem_heap_free_t *)((uintptr_t)JERRY_HEAP_CONTEXT(area[u >> JMEM_SEGMENT_SHIFT]) +
                          (uintptr_t)(u % JMEM_SEGMENT_SIZE));
}

void *jmem_heap_add_segment(bool is_two_segs) {
  uint32_t segment_idx = 0;

  do {
    while (segment_idx < JMEM_SEGMENT && JERRY_HEAP_CONTEXT(area[segment_idx]) != NULL)
      segment_idx++;
  } while (unlikely(is_two_segs) && JERRY_HEAP_CONTEXT(area[segment_idx + 1]) != NULL);

  jmem_heap_free_t *allocated_segment = NULL;
  jmem_heap_free_t *prev_p = &JERRY_HEAP_CONTEXT(first);
  uint32_t curr_offset = JERRY_HEAP_CONTEXT(first).next_offset;
  jmem_heap_free_t *current_p =
      JMEM_HEAP_GET_ADDR_FROM_OFFSET(JERRY_HEAP_CONTEXT(first).next_offset);

  /**
   * If segment address points to NULL, we add a new segment
   * to expand the heap
   */
  if (segment_idx == JMEM_SEGMENT) {
    return NULL;
  }

  JERRY_ASSERT(segment_idx < JMEM_SEGMENT &&
               jmem_segment_get_addr(segment_idx) == NULL);

  if (unlikely(is_two_segs)) {
    JERRY_HEAP_CONTEXT(area[segment_idx]) = (uint8_t *)MALLOC(JMEM_SEGMENT_SIZE * 2);
    JERRY_HEAP_CONTEXT(area[segment_idx + 1]) =
        (uint8_t *)((uintptr_t)JERRY_HEAP_CONTEXT(area[segment_idx]) + JMEM_SEGMENT_SIZE);

    jmem_heap_free_t *const region_p =
        (jmem_heap_free_t *)JERRY_HEAP_CONTEXT(area[segment_idx]);
    region_p->size = (size_t)JMEM_SEGMENT_SIZE * 2;
    region_p->next_offset =
        JMEM_HEAP_GET_OFFSET_FROM_ADDR(JMEM_HEAP_END_OF_LIST);
    JERRY_HEAP_CONTEXT(segments[segment_idx]).allocated_size = 0;

    allocated_segment = region_p;
  } else {
    JERRY_HEAP_CONTEXT(area[segment_idx]) =
        (uint8_t *)jmem_segment_alloc_init(&JERRY_HEAP_CONTEXT(segments[segment_idx]));

    allocated_segment = (jmem_heap_free_t *)JERRY_HEAP_CONTEXT(area[segment_idx]);
  }

  // If malloc failed or all segments are full, return NULL
  if (allocated_segment == NULL) {
    return NULL;
  }

  uint32_t allocated_segment_first_offset =
      (uint32_t)segment_idx * (uint32_t)JMEM_SEGMENT_SIZE;
  while (current_p != JMEM_HEAP_END_OF_LIST &&
         curr_offset < allocated_segment_first_offset) {
    prev_p = current_p;
    // printf("get addr from offset %d->%d(%p)\n", curr_offset,
    // current_p->next_offset, current_p);
    curr_offset = current_p->next_offset;
    current_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET(curr_offset);
  }

  allocated_segment->next_offset = prev_p->next_offset;
  prev_p->next_offset = allocated_segment_first_offset;

#ifdef JMEM_SEGMENT_RB_LOOKUP
  seg_node_t *node_to_insert = (seg_node_t *)MALLOC(sizeof(seg_node_t));
  node_to_insert->base_addr = (uint8_t *)allocated_segment;
  node_to_insert->seg_idx = segment_idx;

  segment_node_insert(&g_segment_rb_root, node_to_insert);

  if (unlikely(is_two_segs)) {
    node_to_insert = (seg_node_t *)MALLOC(sizeof(seg_node_t));
    node_to_insert->base_addr = JERRY_HEAP_CONTEXT(area[segment_idx + 1]);
    node_to_insert->seg_idx = segment_idx + 1;
    segment_node_insert(&g_segment_rb_root, node_to_insert);
  }

#endif /* JMEM_SEGMENT_RB_LOOKUP */

  if (g_last_seg_idx < segment_idx)
    g_last_seg_idx = segment_idx;
  if (unlikely(is_two_segs)) {
    g_segments_count++;
    is_two_segs = 0;
  }
  g_segments_count++;

  return (void *)allocated_segment;
}

void free_empty_segments(void) {
  if (unlikely(g_last_seg_idx == 0))
    return;

  JERRY_ASSERT(JERRY_HEAP_CONTEXT(area[g_last_seg_idx]) != NULL);

  uint32_t seg_iter = 0;
  uint32_t max_seg_iter = 0;
  uint32_t curr_last_seg_idx = g_last_seg_idx;
  while (seg_iter <= curr_last_seg_idx) {
    jmem_heap_free_t *segment_to_free =
        (jmem_heap_free_t *)JERRY_HEAP_CONTEXT(area[seg_iter]);
    if (segment_to_free != NULL &&
        JERRY_HEAP_CONTEXT(segments[seg_iter]).allocated_size == 0) {
      JERRY_ASSERT(segment_to_free->size == JMEM_SEGMENT_SIZE);
      uint32_t allocated_segment_first_offset =
          (uint32_t)seg_iter * (uint32_t)JMEM_SEGMENT_SIZE;

      uint32_t curr_offset = JERRY_HEAP_CONTEXT(first).next_offset;
      jmem_heap_free_t *current_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET(curr_offset);
      jmem_heap_free_t *prev_p = &JERRY_HEAP_CONTEXT(first);

      while (current_p != JMEM_HEAP_END_OF_LIST &&
             curr_offset < allocated_segment_first_offset) {
        prev_p = current_p;
        curr_offset = current_p->next_offset;
        current_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET(curr_offset);
      }
      JERRY_ASSERT(curr_offset == allocated_segment_first_offset);
      prev_p->next_offset = current_p->next_offset;

      if (unlikely(segment_to_free->size > JMEM_SEGMENT_SIZE)) {
        jmem_segment_free(JERRY_HEAP_CONTEXT(area[seg_iter + 1]));
        JERRY_HEAP_CONTEXT(area[seg_iter + 1]) = NULL;
        g_segments_count--;

        if (seg_iter + 1 == g_last_seg_idx)
          g_last_seg_idx = max_seg_iter;
      }
      jmem_segment_free(JERRY_HEAP_CONTEXT(area[seg_iter]));
      JERRY_HEAP_CONTEXT(area[seg_iter]) = NULL;

      g_segments_count--;
      if (seg_iter == g_last_seg_idx)
        g_last_seg_idx = max_seg_iter;
    }
    max_seg_iter = (segment_to_free == NULL) ? max_seg_iter : seg_iter;
    seg_iter++;
  }
}

/* Internal functions */

/**
 * Addr <-> offset translation
 */
inline static uint8_t *__attribute__((hot))
jmem_segment_get_addr(uint32_t segment_idx) {
  return JERRY_HEAP_CONTEXT(area[segment_idx]);
}
inline static uint32_t __attribute__((hot))
jmem_segment_lookup(uint8_t **seg_addr, uint8_t *p) {
  uint8_t *segment_addr = NULL;
  uint32_t segment_idx;

#ifndef JMEM_SEGMENT_RB_LOOKUP
  for (segment_idx = 0; segment_idx < JMEM_SEGMENT; segment_idx++) {
    segment_addr = JERRY_HEAP_CONTEXT(area[segment_idx]);
    if (segment_addr != NULL &&
        (uint32_t)(p - segment_addr) < (uint32_t)JMEM_SEGMENT_SIZE)
      break;
  }
#else  /* JMEM_SEGMENT_RB_LOOKUP */
  seg_node_t *node = segment_node_lookup(&g_segment_rb_root, p);
  segment_idx = node->seg_idx;
  segment_addr = node->base_addr;
#endif /* !JMEM_SEGMENT_RB_LOOKUP */

  *seg_addr = segment_addr;

  return segment_idx;
}

/**
 * Segment management
 */
static void *jmem_segment_alloc(void) {
  void *ret = MALLOC(JMEM_SEGMENT_SIZE);
  JERRY_ASSERT(ret != NULL);

  return ret;
}

static void jmem_segment_init(void *seg_ptr, jmem_segment_t *seg_info) {
  JERRY_ASSERT(seg_ptr != NULL && seg_info != NULL);

  jmem_heap_free_t *const region_p = (jmem_heap_free_t *)seg_ptr;
  region_p->size = (size_t)JMEM_SEGMENT_SIZE;
  region_p->next_offset = JMEM_HEAP_GET_OFFSET_FROM_ADDR(JMEM_HEAP_END_OF_LIST);

  seg_info->flag = 0x00;
  seg_info->allocated_size = 0;
}
static void *jmem_segment_alloc_init(jmem_segment_t *seg_info) {
  JERRY_ASSERT(seg_info != NULL);

  void *seg_ptr = jmem_segment_alloc();

  jmem_segment_init(seg_ptr, seg_info);

  return seg_ptr;
}

static void jmem_segment_free(void *seg_ptr) {
  JERRY_ASSERT(seg_ptr != NULL);

  FREE(seg_ptr);

#ifdef JMEM_SEGMENT_RB_LOOKUP
  segment_node_remove(&g_segment_rb_root, (uint8_t *)seg_ptr);
#endif /* JMEM_SEGMENT_RB_LOOKUP */
}
#endif /* JMEM_SEGMENTED_HEAP */