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

#include "jcontext.h"
#include "jmem.h"
#include "jrt.h"

#define JMEM_ALLOCATOR_INTERNAL
#include "jmem-allocator-internal.h"

#include <stdlib.h>
#define MALLOC(size) ((void *)malloc(size))
#define FREE(ptr) (free(ptr))

#include "jmem-heap-segmented.h"

#ifdef JMEM_SEGMENTED_HEAP
#define JMEM_HEAP_GET_OFFSET_FROM_PTR(p, seg_ptr) \
  ((uint32_t)((uint8_t *)(p) - (uint8_t *)(seg_ptr)))

/* Declaration of internal functions */
static uint8_t *jmem_segment_get_addr(uint32_t segment_idx);
static uint32_t jmem_segment_lookup(uint8_t **seg_addr, uint8_t *p);
static void *jmem_segment_alloc(void);
static void jmem_segment_init(void *seg_ptr, jmem_segment_t *seg_info);
static void *jmem_segment_alloc_init(jmem_segment_t *seg_info);
static void jmem_segment_free(void *seg_ptr, bool is_following_node);

/* External functions */
void init_segmented_heap(void) {
  /* Initialize first segment */
  JERRY_HEAP_CONTEXT(area[0]) =
      (uint8_t *)jmem_segment_alloc_init(&JERRY_HEAP_CONTEXT(segments[0]));
  JERRY_HEAP_CONTEXT(segments_count)++;

  /* Initialize other segments' metadata */
  {
    uint32_t segment_idx;
    for (segment_idx = 1; segment_idx < SEG_NUM_SEGMENTS; segment_idx++) {
      JERRY_HEAP_CONTEXT(area[segment_idx]) = (uint8_t *)NULL;
      JERRY_HEAP_CONTEXT(segments[segment_idx]).occupied_size = 0;
      JERRY_HEAP_CONTEXT(segments[segment_idx]).total_size = 0;
    }
  }

  /* Initialize segment reverse map */
#ifdef SEG_RMAP_BINSEARCH
  JERRY_HEAP_CONTEXT(segment_rmap_rb_root).rb_node = NULL;
  seg_rmap_node_t *node = (seg_rmap_node_t *)MALLOC(sizeof(seg_rmap_node_t));
  node->seg_idx = 0;
  node->base_addr = JERRY_HEAP_CONTEXT(area[0]);
  segment_rmap_insert(&JERRY_HEAP_CONTEXT(segment_rmap_rb_root), node);
#endif /* SEG_RMAP_BINSEARCH */
}

inline uint32_t __attribute__((hot))
jmem_heap_get_offset_from_addr_segmented(jmem_heap_free_t *p) {
  uint32_t segment_idx;
  uint8_t *segment_addr;

  JERRY_ASSERT(p != NULL);

  if (p == (uint8_t *)JMEM_HEAP_END_OF_LIST)
    return (uint32_t)JMEM_HEAP_END_OF_LIST_UINT32;

  if (p == (uint8_t *)&JERRY_HEAP_CONTEXT(first))
    return 0;

  segment_idx = jmem_segment_lookup(&segment_addr, (uint8_t *)p);

  return (uint32_t)(JMEM_HEAP_GET_OFFSET_FROM_PTR(p, segment_addr) +
                    (uint32_t)SEG_SEGMENT_SIZE * segment_idx);
}
inline jmem_heap_free_t *__attribute__((hot))
jmem_heap_get_addr_from_offset_segmented(uint32_t u) {
  if (u == JMEM_HEAP_END_OF_LIST_UINT32)
    return JMEM_HEAP_END_OF_LIST;
  return (jmem_heap_free_t *)((uintptr_t)JERRY_HEAP_CONTEXT(
                                  area[u >> SEG_SEGMENT_SHIFT]) +
                              (uintptr_t)(u % SEG_SEGMENT_SIZE));
}

static uint32_t __find_proper_segment_entry(bool is_two_segs) {
  for (uint32_t segment_idx = 0; segment_idx < SEG_NUM_SEGMENTS;
       segment_idx++) {
    if (is_two_segs) {
      if (JERRY_HEAP_CONTEXT(area[segment_idx] == NULL) &&
          JERRY_HEAP_CONTEXT(area[segment_idx + 1] == NULL)) {
        return segment_idx;
      }
    } else {
      if (JERRY_HEAP_CONTEXT(area[segment_idx] == NULL)) {
        return segment_idx;
      }
    }
  }
  return SEG_NUM_SEGMENTS;
}

void *alloc_a_segment_group(bool is_two_segs) {
  // Find empty entry or double empty entries in segment translation table
  uint32_t segment_idx = __find_proper_segment_entry(is_two_segs);
  if (segment_idx >= SEG_NUM_SEGMENTS) {
    return NULL;
  }

  /* Allocate and initialize a segment or double segments */
  jmem_heap_free_t *allocated_segment = NULL;
  JERRY_ASSERT(segment_idx < SEG_NUM_SEGMENTS &&
               jmem_segment_get_addr(segment_idx) == NULL);

  if (likely(!is_two_segs)) {
    // One segment
    JERRY_HEAP_CONTEXT(area[segment_idx]) = (uint8_t *)jmem_segment_alloc_init(
        &JERRY_HEAP_CONTEXT(segments[segment_idx]));
    allocated_segment =
        (jmem_heap_free_t *)JERRY_HEAP_CONTEXT(area[segment_idx]);
  } else {
    // Double segments
    JERRY_HEAP_CONTEXT(area[segment_idx]) =
        (uint8_t *)MALLOC(SEG_SEGMENT_SIZE * 2);
    JERRY_HEAP_CONTEXT(area[segment_idx + 1]) =
        ((uint8_t *)JERRY_HEAP_CONTEXT(area[segment_idx])) + SEG_SEGMENT_SIZE;

    jmem_heap_free_t *const region_p =
        (jmem_heap_free_t *)JERRY_HEAP_CONTEXT(area[segment_idx]);
    region_p->size = (size_t)SEG_SEGMENT_SIZE * 2;
    region_p->next_offset =
        JMEM_HEAP_GET_OFFSET_FROM_ADDR(JMEM_HEAP_END_OF_LIST);
    JERRY_HEAP_CONTEXT(segments[segment_idx]).total_size = SEG_SEGMENT_SIZE * 2;
    JERRY_HEAP_CONTEXT(segments[segment_idx]).occupied_size = 0;
    JERRY_HEAP_CONTEXT(segments[segment_idx + 1]).total_size = 0;
    JERRY_HEAP_CONTEXT(segments[segment_idx + 1]).occupied_size = 0;
    allocated_segment = region_p;

    // Update allocated heap area size, system memory allocator
    JERRY_CONTEXT(jmem_allocated_heap_size) += SEG_SEGMENT_SIZE * 2;
    JERRY_CONTEXT(jmem_system_allocator_metadata_size) +=
        SYSTEM_ALLOCATOR_METADATA_SIZE;
  }

  // If malloc failed or all segments are full, return NULL
  if (allocated_segment == NULL) {
    return NULL;
  }

  // Update heap free list
  jmem_heap_free_t *prev_p = &JERRY_HEAP_CONTEXT(first);
  uint32_t curr_offset = JERRY_HEAP_CONTEXT(first).next_offset;
  jmem_heap_free_t *current_p =
      JMEM_HEAP_GET_ADDR_FROM_OFFSET(JERRY_HEAP_CONTEXT(first).next_offset);
  uint32_t allocated_segment_first_offset =
      (uint32_t)segment_idx * (uint32_t)SEG_SEGMENT_SIZE;
  while (current_p != JMEM_HEAP_END_OF_LIST &&
         curr_offset < allocated_segment_first_offset) {
    prev_p = current_p;
    curr_offset = current_p->next_offset;
    current_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET(curr_offset);
  }
  allocated_segment->next_offset = prev_p->next_offset;
  prev_p->next_offset = allocated_segment_first_offset;

#ifdef SEG_RMAP_BINSEARCH
  seg_rmap_node_t *new_rmap_node =
      (seg_rmap_node_t *)MALLOC(sizeof(seg_rmap_node_t));
  new_rmap_node->base_addr = (uint8_t *)allocated_segment;
  new_rmap_node->seg_idx = segment_idx;
  segment_rmap_insert(&JERRY_HEAP_CONTEXT(segment_rmap_rb_root), new_rmap_node);
  if (unlikely(is_two_segs)) {
    new_rmap_node = (seg_rmap_node_t *)MALLOC(sizeof(seg_rmap_node_t));
    new_rmap_node->base_addr =
        ((uint8_t *)allocated_segment) + SEG_SEGMENT_SIZE;
    new_rmap_node->seg_idx = segment_idx + 1;
    segment_rmap_insert(&JERRY_HEAP_CONTEXT(segment_rmap_rb_root),
                        new_rmap_node);
  }
#endif /* SEG_RMAP_BINSEARCH */

  if (unlikely(is_two_segs)) {
    JERRY_HEAP_CONTEXT(segments_count) += 2;
  } else {
    JERRY_HEAP_CONTEXT(segments_count) += 1;
  }
  return (void *)allocated_segment;
}

void free_empty_segment_groups(void) {
  for (uint32_t seg_iter = 1; seg_iter < SEG_NUM_SEGMENTS; seg_iter++) {
    jmem_heap_free_t *segment_to_free =
        (jmem_heap_free_t *)JERRY_HEAP_CONTEXT(area[seg_iter]);
    if (segment_to_free == NULL)
      continue;

    jmem_segment_t *segment = &(JERRY_HEAP_CONTEXT(segments[seg_iter]));
    if (segment->total_size == SEG_SEGMENT_SIZE) {
      // Single segment
      if (segment->occupied_size > 0) {
        continue;
      }
    } else if (segment->total_size == SEG_SEGMENT_SIZE * 2) {
      // Head segment in double segments
      jmem_segment_t *following_segment =
          &(JERRY_HEAP_CONTEXT(segments[seg_iter + 1]));
      if (segment->occupied_size > 0 || following_segment->occupied_size > 0) {
        continue;
      }
    } else {
      // Following segment in double segments
      continue;
    }

    JERRY_ASSERT(segment_to_free->size != 0);
    JERRY_ASSERT((segment_to_free->size % SEG_SEGMENT_SIZE) == 0);

    uint32_t curr_offset = JERRY_HEAP_CONTEXT(first).next_offset;
    jmem_heap_free_t *current_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET(curr_offset);
    jmem_heap_free_t *prev_p = &JERRY_HEAP_CONTEXT(first);
    uint32_t allocated_segment_first_offset =
        (uint32_t)seg_iter * (uint32_t)SEG_SEGMENT_SIZE;
    // if(allocated_segment_first_offset == 0) {
    //   allocated_segment_first_offset = JMEM_ALIGNMENT;
    // }
    while (current_p != JMEM_HEAP_END_OF_LIST &&
           curr_offset < allocated_segment_first_offset) {
      prev_p = current_p;
      curr_offset = current_p->next_offset;
      current_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET(curr_offset);
    }
    JERRY_ASSERT(curr_offset == allocated_segment_first_offset);
    prev_p->next_offset = current_p->next_offset;
    if (unlikely(segment_to_free->size > SEG_SEGMENT_SIZE)) {
      jmem_segment_free(JERRY_HEAP_CONTEXT(area[seg_iter + 1]), true);
      JERRY_HEAP_CONTEXT(area[seg_iter + 1]) = NULL;
      JERRY_HEAP_CONTEXT(segments[seg_iter + 1]).total_size = 0;
      JERRY_HEAP_CONTEXT(segments[seg_iter + 1]).occupied_size = 0;
      JERRY_HEAP_CONTEXT(segments_count)--;
    }
    jmem_segment_free(JERRY_HEAP_CONTEXT(area[seg_iter]), false);
    JERRY_HEAP_CONTEXT(area[seg_iter]) = NULL;
    JERRY_HEAP_CONTEXT(segments[seg_iter]).total_size = 0;
    JERRY_HEAP_CONTEXT(segments[seg_iter]).occupied_size = 0;
    JERRY_HEAP_CONTEXT(segments_count)--;
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

#ifndef SEG_RMAP_BINSEARCH
  for (segment_idx = 0; segment_idx < SEG_NUM_SEGMENTS; segment_idx++) {
    segment_addr = JERRY_HEAP_CONTEXT(area[segment_idx]);
    if (segment_addr != NULL &&
        (uint32_t)(p - segment_addr) < (uint32_t)SEG_SEGMENT_SIZE)
      break;
  }
#else  /* SEG_RMAP_BINSEARCH */
  seg_rmap_node_t *node =
      segment_rmap_lookup(&JERRY_HEAP_CONTEXT(segment_rmap_rb_root), p);
  segment_idx = node->seg_idx;
  segment_addr = node->base_addr;
#endif /* !SEG_RMAP_BINSEARCH */

  *seg_addr = segment_addr;

  return segment_idx;
}

void free_initial_segment_group(void) {
  jmem_segment_free(JERRY_HEAP_CONTEXT(area[0]), false);
  JERRY_HEAP_CONTEXT(segments_count)--;
}

/**
 * Segment management
 */
static void *jmem_segment_alloc(void) {
  void *ret = MALLOC(SEG_SEGMENT_SIZE);

  // Update allocated heap area size, system memory allocator
  JERRY_CONTEXT(jmem_allocated_heap_size) += SEG_SEGMENT_SIZE;
  JERRY_CONTEXT(jmem_system_allocator_metadata_size) +=
      SYSTEM_ALLOCATOR_METADATA_SIZE;

  JERRY_ASSERT(ret != NULL);

  return ret;
}

static void jmem_segment_init(void *seg_ptr, jmem_segment_t *seg_info) {
  JERRY_ASSERT(seg_ptr != NULL && seg_info != NULL);

  jmem_heap_free_t *const region_p = (jmem_heap_free_t *)seg_ptr;
  region_p->size = (size_t)SEG_SEGMENT_SIZE;
  region_p->next_offset = JMEM_HEAP_GET_OFFSET_FROM_ADDR(JMEM_HEAP_END_OF_LIST);

  seg_info->total_size = (size_t)SEG_SEGMENT_SIZE;
  seg_info->occupied_size = 0;
}
static void *jmem_segment_alloc_init(jmem_segment_t *seg_info) {
  JERRY_ASSERT(seg_info != NULL);
  void *seg_ptr = jmem_segment_alloc();
  jmem_segment_init(seg_ptr, seg_info);
  return seg_ptr;
}

static void jmem_segment_free(void *seg_ptr, bool is_following_node) {
  JERRY_ASSERT(seg_ptr != NULL);

  // Update allocated heap area size, system memory allocator
    JERRY_CONTEXT(jmem_allocated_heap_size) -= SEG_SEGMENT_SIZE;
    JERRY_CONTEXT(jmem_system_allocator_metadata_size) -=
        SYSTEM_ALLOCATOR_METADATA_SIZE;

  if (!is_following_node) {
    FREE(seg_ptr);
  }

#ifdef SEG_RMAP_BINSEARCH
  segment_rmap_remove(&JERRY_HEAP_CONTEXT(segment_rmap_rb_root),
                      (uint8_t *)seg_ptr);
#endif /* SEG_RMAP_BINSEARCH */
}
#endif /* JMEM_SEGMENTED_HEAP */