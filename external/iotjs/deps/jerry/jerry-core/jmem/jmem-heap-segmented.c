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

#include <stdlib.h>
#define MALLOC(size) ((void *)malloc(size))
#define FREE(ptr) (free(ptr))

#include "jmem-heap-segmented.h"

#include "jmem-heap-segmented-rmap-rb.h"
#include "jmem-heap-segmented-translation.h"

#ifdef JMEM_SEGMENTED_HEAP

/* Declaration of internal functions */
static void *alloc_a_segment(jmem_segment_t *seg_info);
static void free_a_segment(void *seg_ptr, bool is_following_node);

/* External functions */
void init_segmented_heap(void) {
  /* Initialize first segment */
  JERRY_HEAP_CONTEXT(area[0]) =
      (uint8_t *)alloc_a_segment(&JERRY_HEAP_CONTEXT(segments[0]));
  JERRY_HEAP_CONTEXT(segments_count)++;

  /* Initialize other segments' metadata */
  {
    uint32_t sidx;
    for (sidx = 1; sidx < SEG_NUM_SEGMENTS; sidx++) {
      JERRY_HEAP_CONTEXT(area[sidx]) = (uint8_t *)NULL;
      JERRY_HEAP_CONTEXT(segments[sidx]).occupied_size = 0;
      JERRY_HEAP_CONTEXT(segments[sidx]).total_size = 0;
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

static uint32_t __find_proper_segment_entry(bool is_two_segs) {
  for (uint32_t sidx = 0; sidx < SEG_NUM_SEGMENTS; sidx++) {
    if (is_two_segs) {
      if (JERRY_HEAP_CONTEXT(area[sidx] == NULL) &&
          JERRY_HEAP_CONTEXT(area[sidx + 1] == NULL)) {
        return sidx;
      }
    } else {
      if (JERRY_HEAP_CONTEXT(area[sidx] == NULL)) {
        return sidx;
      }
    }
  }
  return SEG_NUM_SEGMENTS;
}

void *alloc_a_segment_group(bool is_two_segs) {
  // Find empty entry or double empty entries in segment translation table
  uint32_t sidx = __find_proper_segment_entry(is_two_segs);
  if (sidx >= SEG_NUM_SEGMENTS) {
    return NULL;
  }

  /* Allocate and initialize a segment or double segments */
  jmem_heap_free_t *allocated_segment = NULL;
  JERRY_ASSERT(sidx < SEG_NUM_SEGMENTS && sidx_to_addr(sidx) == NULL);

  if (likely(!is_two_segs)) {
    // One segment
    JERRY_HEAP_CONTEXT(area[sidx]) =
        (uint8_t *)alloc_a_segment(&JERRY_HEAP_CONTEXT(segments[sidx]));
    allocated_segment = (jmem_heap_free_t *)JERRY_HEAP_CONTEXT(area[sidx]);
  } else {
    // Double segments
    JERRY_HEAP_CONTEXT(area[sidx]) = (uint8_t *)MALLOC(SEG_SEGMENT_SIZE * 2);
    JERRY_HEAP_CONTEXT(area[sidx + 1]) =
        ((uint8_t *)JERRY_HEAP_CONTEXT(area[sidx])) + SEG_SEGMENT_SIZE;

    jmem_heap_free_t *const region_p =
        (jmem_heap_free_t *)JERRY_HEAP_CONTEXT(area[sidx]);
    region_p->size = (size_t)SEG_SEGMENT_SIZE * 2;
    region_p->next_offset =
        JMEM_HEAP_GET_OFFSET_FROM_ADDR(JMEM_HEAP_END_OF_LIST);
    JERRY_HEAP_CONTEXT(segments[sidx]).total_size = SEG_SEGMENT_SIZE * 2;
    JERRY_HEAP_CONTEXT(segments[sidx]).occupied_size = 0;
    JERRY_HEAP_CONTEXT(segments[sidx + 1]).total_size = 0;
    JERRY_HEAP_CONTEXT(segments[sidx + 1]).occupied_size = 0;
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
      (uint32_t)sidx * (uint32_t)SEG_SEGMENT_SIZE;
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
  new_rmap_node->seg_idx = sidx;
  segment_rmap_insert(&JERRY_HEAP_CONTEXT(segment_rmap_rb_root), new_rmap_node);
  if (unlikely(is_two_segs)) {
    new_rmap_node = (seg_rmap_node_t *)MALLOC(sizeof(seg_rmap_node_t));
    new_rmap_node->base_addr =
        ((uint8_t *)allocated_segment) + SEG_SEGMENT_SIZE;
    new_rmap_node->seg_idx = sidx + 1;
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
      free_a_segment(JERRY_HEAP_CONTEXT(area[seg_iter + 1]), true);
      JERRY_HEAP_CONTEXT(area[seg_iter + 1]) = NULL;
      JERRY_HEAP_CONTEXT(segments[seg_iter + 1]).total_size = 0;
      JERRY_HEAP_CONTEXT(segments[seg_iter + 1]).occupied_size = 0;
      JERRY_HEAP_CONTEXT(segments_count)--;
    }
    free_a_segment(JERRY_HEAP_CONTEXT(area[seg_iter]), false);
    JERRY_HEAP_CONTEXT(area[seg_iter]) = NULL;
    JERRY_HEAP_CONTEXT(segments[seg_iter]).total_size = 0;
    JERRY_HEAP_CONTEXT(segments[seg_iter]).occupied_size = 0;
    JERRY_HEAP_CONTEXT(segments_count)--;
  }
}

/* Internal functions */

void free_initial_segment_group(void) {
  free_a_segment(JERRY_HEAP_CONTEXT(area[0]), false);
  JERRY_HEAP_CONTEXT(segments_count)--;
}

/**
 * Segment management
 */
static void *alloc_a_segment(jmem_segment_t *seg_info) {
  JERRY_ASSERT(seg_info != NULL);
  void *seg_ptr = MALLOC(SEG_SEGMENT_SIZE);

  // Update allocated heap area size, system memory allocator
  JERRY_CONTEXT(jmem_allocated_heap_size) += SEG_SEGMENT_SIZE;
  JERRY_CONTEXT(jmem_system_allocator_metadata_size) +=
      SYSTEM_ALLOCATOR_METADATA_SIZE;
  JERRY_ASSERT(seg_ptr != NULL);

  // Update free region
  jmem_heap_free_t *const region_p = (jmem_heap_free_t *)seg_ptr;
  region_p->size = (size_t)SEG_SEGMENT_SIZE;
  region_p->next_offset = JMEM_HEAP_GET_OFFSET_FROM_ADDR(JMEM_HEAP_END_OF_LIST);

  // Update segment metadata
  seg_info->total_size = (size_t)SEG_SEGMENT_SIZE;
  seg_info->occupied_size = 0;
  return seg_ptr;
}

static void free_a_segment(void *seg_ptr, bool is_following_node) {
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