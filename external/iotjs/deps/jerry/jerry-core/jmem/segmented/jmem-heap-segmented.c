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
#define FREE(ptr) (free((void *)ptr))

#include "jmem-heap-segmented.h"

#include "cptl-rmap-cache.h"
#include "jmem-heap-segmented-cptl.h"
#include "jmem-heap-segmented-rmap-rb.h"

#ifdef JMEM_SEGMENTED_HEAP

/* Declaration of internal functions */
static void *alloc_a_segment_group_internal(size_t required_size,
                                            uint32_t *start_sidx_out);

/* External functions */
void init_segmented_heap(void) {
  // Initialize segment headers
  for (uint32_t sidx = 0; sidx < SEG_NUM_SEGMENTS; sidx++) {
    JERRY_HEAP_CONTEXT(segments[sidx]).occupied_size = 0;
    JERRY_HEAP_CONTEXT(segments[sidx]).group_num_segments = 0;
  }

  // Initialize compressed pointer translation layer (CPTL)
  init_cptl();

  // Initialize first segment
  uint32_t start_sidx;
  alloc_a_segment_group_internal(JMEM_ALIGNMENT, &start_sidx);
  JERRY_ASSERT(start_sidx == 0);
}

static uint32_t __find_new_segment_group_region(
    uint32_t required_num_segments) {
  uint32_t max_start_sidx = (uint32_t)SEG_NUM_SEGMENTS - required_num_segments;
  for (uint32_t start_sidx = 0; start_sidx <= max_start_sidx; start_sidx++) {
    bool is_new_area_found = true;
    uint32_t end_sidx = start_sidx + required_num_segments - 1;
    for (uint32_t sidx = start_sidx; sidx <= end_sidx; sidx++) {
      if (JERRY_HEAP_CONTEXT(area[sidx]) != NULL) {
        is_new_area_found = false;
        break;
      }
    }
    if (is_new_area_found) {
      return start_sidx;
    }
  }
  return SEG_NUM_SEGMENTS;
}

static uint32_t get_required_num_segments(size_t required_size) {
  return ((uint32_t)required_size + SEG_SEGMENT_SIZE - JMEM_ALIGNMENT) /
         SEG_SEGMENT_SIZE;
}

static void *alloc_a_segment_group_internal(size_t required_size,
                                            uint32_t *start_sidx_out) {
  // Calculate required number of segments
  uint32_t required_num_segments = get_required_num_segments(required_size);
  uint32_t segment_group_size = SEG_SEGMENT_SIZE * required_num_segments;

  // Check number of segments
  if (JERRY_HEAP_CONTEXT(segments_count) + required_num_segments >
      SEG_NUM_SEGMENTS)
    return NULL;

  // Find empty entry or double empty entries in segment translation table
  uint32_t start_sidx = __find_new_segment_group_region(required_num_segments);
  if (start_sidx >= SEG_NUM_SEGMENTS)
    return NULL;

  // Allocate segment group
  uint8_t *segment_group_area = MALLOC(segment_group_size);
  if (segment_group_area == NULL)
    return NULL;

  // Update segment metadata
  for (uint32_t seg_no = 0; seg_no < required_num_segments; seg_no++) {
    uint32_t sidx = start_sidx + seg_no;
    // Allocate an free region for a segment
    jmem_segment_t *segment_header = &JERRY_HEAP_CONTEXT(segments[sidx]);
    uint8_t *segment_area = segment_group_area + (SEG_SEGMENT_SIZE * seg_no);

    // Update segment base table
    JERRY_HEAP_CONTEXT(area[sidx]) = segment_area;

#ifdef SEG_RMAP_BINSEARCH
    // Update segment reverse map tree
    segment_rmap_insert(&JERRY_HEAP_CONTEXT(segment_rmap_rb_root), segment_area,
                        sidx);
#endif /* defined(SEG_RMAP_BINSEARCH) */

    // Update segment header
    if (sidx == start_sidx) {
      segment_header->group_num_segments = required_num_segments;
    } else {
      segment_header->group_num_segments = 0;
    }
    segment_header->occupied_size = 0;
  }

  // Update allocated heap area size, system memory allocator
  JERRY_CONTEXT(jmem_allocated_heap_size) +=
      SEG_SEGMENT_SIZE * required_num_segments;
  JERRY_CONTEXT(jmem_system_allocator_metadata_size) +=
      SYSTEM_ALLOCATOR_METADATA_SIZE;

  // Update segment count
  JERRY_HEAP_CONTEXT(segments_count) += required_num_segments;

  *start_sidx_out = start_sidx;
  return (void *)segment_group_area;
}

void *alloc_a_segment_group(size_t required_size) {
  uint32_t start_sidx;
  uint8_t *segment_group_area =
      (uint8_t *)alloc_a_segment_group_internal(required_size, &start_sidx);
  if (segment_group_area == NULL)
    return NULL;
  jmem_segment_t *segment_header = &JERRY_HEAP_CONTEXT(segments[start_sidx]);
  uint32_t required_num_segments = segment_header->group_num_segments;

  // Update free region for the segment group and free region list
  jmem_heap_free_t *prev_p = &JERRY_HEAP_CONTEXT(first);
  uint32_t curr_offset = JERRY_HEAP_CONTEXT(first).next_offset;
  jmem_heap_free_t *current_p =
      JMEM_DECOMPRESS_POINTER_INTERNAL(JERRY_HEAP_CONTEXT(first).next_offset);
  uint32_t segment_group_offset =
      (uint32_t)start_sidx * (uint32_t)SEG_SEGMENT_SIZE;
  while (current_p != JMEM_HEAP_END_OF_LIST &&
         curr_offset < segment_group_offset) {
    prev_p = current_p;
    curr_offset = current_p->next_offset;
    current_p = JMEM_DECOMPRESS_POINTER_INTERNAL(curr_offset);
  }
  jmem_heap_free_t *segment_group_region =
      (jmem_heap_free_t *)segment_group_area;
  segment_group_region->size = SEG_SEGMENT_SIZE * required_num_segments;
  segment_group_region->next_offset = prev_p->next_offset;
  prev_p->next_offset = segment_group_offset;

  return (void *)segment_group_region;
}

void free_empty_segment_groups(void) {
  // Free empty segment groups except initial segment
  for (uint32_t start_sidx = 1; start_sidx < SEG_NUM_SEGMENTS; start_sidx++) {
    // Filter leading segment of the segment group
    jmem_heap_free_t *segment_group_region =
        (jmem_heap_free_t *)JERRY_HEAP_CONTEXT(area[start_sidx]);
    jmem_segment_t *segment_group_header =
        &JERRY_HEAP_CONTEXT(segments[start_sidx]);
    if (segment_group_region == NULL ||
        segment_group_header->group_num_segments == 0)
      continue;

    // Check if the segment group is empty
    bool is_free_segment_group = true;
    uint32_t end_sidx =
        start_sidx + segment_group_header->group_num_segments - 1;
    for (uint32_t sidx = start_sidx; sidx <= end_sidx; sidx++) {
      jmem_segment_t *segment_header = &JERRY_HEAP_CONTEXT(segments[sidx]);
      if (segment_header->occupied_size > 0)
        is_free_segment_group = false;
    }

    if (!is_free_segment_group)
      continue;


    // Update segment metadata
    {
      uint32_t group_num_segments = segment_group_header->group_num_segments;
      // Check free region size of the segment group
      JERRY_ASSERT(segment_group_region->size ==
                   SEG_SEGMENT_SIZE * group_num_segments);

      // Update segment count
      JERRY_HEAP_CONTEXT(segments_count) -= group_num_segments;

      // Update free region list
      uint32_t curr_offset = JERRY_HEAP_CONTEXT(first).next_offset;
      jmem_heap_free_t *current_p =
          JMEM_DECOMPRESS_POINTER_INTERNAL(curr_offset);
      jmem_heap_free_t *prev_p = &JERRY_HEAP_CONTEXT(first);
      uint32_t segment_group_offset =
          (uint32_t)start_sidx * (uint32_t)SEG_SEGMENT_SIZE;
      while (current_p != JMEM_HEAP_END_OF_LIST &&
             curr_offset < segment_group_offset) {
        prev_p = current_p;
        curr_offset = current_p->next_offset;
        current_p = JMEM_DECOMPRESS_POINTER_INTERNAL(curr_offset);
      }
      JERRY_ASSERT(curr_offset == segment_group_offset);
      prev_p->next_offset = current_p->next_offset;

      // Update allocated heap area size, system memory allocator
      JERRY_CONTEXT(jmem_allocated_heap_size) -=
          SEG_SEGMENT_SIZE * group_num_segments;
      JERRY_CONTEXT(jmem_system_allocator_metadata_size) -=
          SYSTEM_ALLOCATOR_METADATA_SIZE;

      // Update segment metadata
      for (uint32_t sidx = start_sidx; sidx <= end_sidx; sidx++) {
        jmem_segment_t *segment_header = &JERRY_HEAP_CONTEXT(segments[sidx]);
        uint8_t *segment_area = sidx_to_addr(sidx);

        // Update segment header
        segment_header->group_num_segments = 0;
        segment_header->occupied_size = 0;

#ifdef SEG_RMAP_BINSEARCH
        // Update segment reverse map tree
        segment_rmap_remove(&JERRY_HEAP_CONTEXT(segment_rmap_rb_root),
                            segment_area);
#else
        JERRY_UNUSED(segment_area);
#endif /* defined(SEG_RMAP_BINSEARCH) */

#ifdef SEG_RMAP_CACHE
        // Invalidate reverse map cache entry
        invalidate_rmap_cache_entry(sidx);
#ifdef SEG_RMAP_2LEVEL_SEARCH
        invalidate_fifo_cache_entry(sidx);
#endif
#endif /* defined(SEG_RMAP_BINSEARCH) */

        // Update segment base table
        JERRY_HEAP_CONTEXT(area[sidx]) = NULL;
      }

      // Free the segment group
      FREE(segment_group_region);
    } /* update segment metadata END */
  }   /* for (start_sidx = 1 to (SEG_NUM_SEGMENTS - 1) by 1 */
}

void free_initial_segment_group(void) {
  jmem_heap_free_t *segment_group_region = JERRY_HEAP_CONTEXT(area[0]);
  jmem_segment_t *segment_header = &JERRY_HEAP_CONTEXT(segments[0]);
  // Free the segment group
  FREE(segment_group_region);

  // Update segment count
  JERRY_HEAP_CONTEXT(segments_count)--;

  // Update segment header
  segment_header->group_num_segments = 0;
  segment_header->occupied_size = 0;

#ifdef SEG_RMAP_BINSEARCH
  // Update segment reverse map tree
  segment_rmap_remove(&JERRY_HEAP_CONTEXT(segment_rmap_rb_root),
                      segment_group_region);
#endif /* defined(SEG_RMAP_BINSEARCH) */

  // Update allocated heap area size, system memory allocator
  JERRY_CONTEXT(jmem_allocated_heap_size) -= SEG_SEGMENT_SIZE;
  JERRY_CONTEXT(jmem_system_allocator_metadata_size) -=
      SYSTEM_ALLOCATOR_METADATA_SIZE;
}
#endif /* JMEM_SEGMENTED_HEAP */