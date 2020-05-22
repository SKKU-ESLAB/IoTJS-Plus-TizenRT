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

#include "jmem-heap-segmented-cptl.h"

#include "jcontext.h"
#include "jmem-config.h"
#include "jmem-heap-segmented-rmap-rb.h"
#include "jmem-profiler.h"
#include "jmem.h"

#include "cptl-rmap-cache.h"

#define JMEM_HEAP_GET_OFFSET_FROM_PTR(p, seg_ptr) \
  ((uint32_t)((uint8_t *)(p) - (uint8_t *)(seg_ptr)))

// Initialize compressed pointer translation layer (CPTL)
void init_cptl(void) {
  // Initialize segment base table
  for (uint32_t sidx = 0; sidx < SEG_NUM_SEGMENTS; sidx++) {
    JERRY_HEAP_CONTEXT(area[sidx]) = (uint8_t *)NULL;
  }

  // Initialize reverse map
#ifdef SEG_RMAP_BINSEARCH
  JERRY_HEAP_CONTEXT(segment_rmap_rb_root).rb_node = NULL;
#endif /* SEG_RMAP_BINSEARCH */

  // Initialize FIFO cache
#ifdef SEG_RMAP_2LEVEL_SEARCH
  for (int i = 0; i < SEG_RMAP_2LEVEL_SEARCH_FIFO_CACHE_SIZE; i++) {
    JERRY_HEAP_CONTEXT(fc_table_sidx[i]) = SEG_NUM_SEGMENTS;
    JERRY_HEAP_CONTEXT(fc_table_base_addr[i]) = NULL;
  }
  JERRY_HEAP_CONTEXT(fc_table_eviction_header) = 0;
  JERRY_HEAP_CONTEXT(fc_table_valid_count) = 0;
#endif

#ifdef SEG_RMAP_CACHE
  init_rmap_cache();
#endif /* defined(SEG_RMAP_CACHE) */
}


// Address compression (Full-bitwidth pointer -> compressed pointer)
// * jmem.h
inline uint32_t __attribute__((hot))
cptl_compress_pointer_internal(jmem_heap_free_t *p) {
  uint32_t cp;
  uint32_t sidx;
  uint8_t *saddr;
  JERRY_ASSERT(p != NULL);

  profile_compression_start();
  if (p == (uint8_t *)JMEM_HEAP_END_OF_LIST) {
    cp = (uint32_t)JMEM_HEAP_END_OF_LIST_UINT32;
  } else if (p == (uint8_t *)&JERRY_HEAP_CONTEXT(first)) {
    cp = 0;
  } else {
    sidx = addr_to_saddr_and_sidx((uint8_t *)p, &saddr);
    cp = (uint32_t)(JMEM_HEAP_GET_OFFSET_FROM_PTR(p, saddr) +
                    (uint32_t)SEG_SEGMENT_SIZE * sidx);
  }
  profile_compression_end();

#ifdef PROF_COUNT__COMPRESSION_CALLERS
  profile_inc_count_of_a_type(0); // compression callers
#endif
  return cp;
}

// Address decompression (Compressed pointer -> full-bitwidth pointer)
// * jmem.h
inline jmem_heap_free_t *__attribute__((hot))
cptl_decompress_pointer_internal(uint32_t cp) {
  jmem_heap_free_t *p;
  profile_decompression_start();
  if (cp == JMEM_HEAP_END_OF_LIST_UINT32) {
    p = JMEM_HEAP_END_OF_LIST;
  } else {
    p = (jmem_heap_free_t *)((uintptr_t)sidx_to_addr(cp >> SEG_SEGMENT_SHIFT) +
                             (uintptr_t)(cp % SEG_SEGMENT_SIZE));
  }
  profile_decompression_end();
  return p;
}

// Raw function to access segment base table
// * Segment index -> Segment base address(Full-bitwidth pointer)
// * Core part of decompression
// * jmem-heap-segmented-cptl.h
inline uint8_t *__attribute__((hot)) sidx_to_addr(uint32_t sidx) {
  uint8_t *addr = JERRY_HEAP_CONTEXT(area[sidx]);
  print_cptl_access(sidx, -1); // CPTL access profiling
  return addr;
}

#if defined(SEG_RMAP_BINSEARCH)
static inline uint32_t binary_search(uint8_t *addr, uint8_t **saddr_out) {
  seg_rmap_node_t *node =
      segment_rmap_lookup(&JERRY_HEAP_CONTEXT(segment_rmap_rb_root), addr);
  *saddr_out = node->base_addr;
  return node->sidx;
}
#else /* defined(SEG_RMAP_BINSEARCH) */
static inline uint32_t linear_search(uint8_t *addr, uint8_t **saddr_out) {
  for (uint32_t sidx = 0; sidx < SEG_NUM_SEGMENTS; sidx++) {
    INCREASE_LOOKUP_DEPTH();
    uint8_t *saddr = JERRY_HEAP_CONTEXT(area[sidx]);
    if (saddr != NULL && (uint32_t)(addr - saddr) < (uint32_t)SEG_SEGMENT_SIZE)
      *saddr_out = saddr;
    return sidx;
  }
  return SEG_NUM_SEGMENTS; // It should never be called.
}

#if defined(SEG_RMAP_2LEVEL_SEARCH)
static inline uint32_t two_level_search(uint8_t *addr, uint8_t **saddr_out) {
  uint32_t sidx = SEG_NUM_SEGMENTS;
  // 1st-level search: FIFO cache search
  for (uint32_t i = 0; i < JERRY_HEAP_CONTEXT(fc_table_valid_count); i++) {
    INCREASE_LOOKUP_DEPTH();
    uint8_t *saddr = JERRY_HEAP_CONTEXT(fc_table_base_addr[i]);
    if (saddr != NULL &&
        (uint32_t)(addr - saddr) < (uint32_t)SEG_SEGMENT_SIZE) {
      // FIFO cache saerch succeeds
      sidx = JERRY_HEAP_CONTEXT(fc_table_sidx[i]);
      break; // It should be called at least once.
    }
  }

  if (sidx >= SEG_NUM_SEGMENTS) {
    // FIFO cache search fails
    // 2nd-level search: linear search
    sidx = linear_search(addr, saddr_out);

    // Update FIFO cache
    uint32_t eviction_header = JERRY_HEAP_CONTEXT(fc_table_eviction_header);
    JERRY_HEAP_CONTEXT(fc_table_base_addr[eviction_header]) = addr;
    JERRY_HEAP_CONTEXT(fc_table_sidx[eviction_header]) = sidx;
    if (JERRY_HEAP_CONTEXT(fc_table_valid_count) <
        SEG_RMAP_2LEVEL_SEARCH_FIFO_CACHE_SIZE)
      JERRY_HEAP_CONTEXT(fc_table_valid_count)++;

    JERRY_HEAP_CONTEXT(fc_table_eviction_header) =
        (eviction_header + 1) % SEG_RMAP_2LEVEL_SEARCH_FIFO_CACHE_SIZE;
  }

  return sidx;
}
#endif /* defined(SEG_RMAP_2LEVEL_SEARCH) */
#endif /* !defined(SEG_RMAP_BINSEARCH) */

// Raw function to access segment reverse map
// * Full-bitwidth pointer -> Segment index
// * Core part of compression
// * jmem-heap-segmented-cptl.h
inline uint32_t __attribute__((hot))
addr_to_saddr_and_sidx(uint8_t *addr, uint8_t **saddr_out) {
  uint32_t sidx;
  CLEAR_LOOKUP_DEPTH();

  profile_inc_rmc_access_count(); // CPTL reverse map caching profiling

  // Fast path
#ifdef SEG_RMAP_CACHE
  sidx = access_and_check_rmap_cache(addr, saddr_out);
  if (sidx < SEG_NUM_SEGMENTS) {
    print_cptl_access(sidx, 0); // CPTL access profiling
    return sidx;
  }
#endif /* defined(SEG_RMAP_CACHE) */

  // Slow path
#if defined(SEG_RMAP_BINSEARCH)
  // Binary search
  sidx = binary_search(addr, saddr_out);
#elif defined(SEG_RMAP_2LEVEL_SEARCH) /* defined(SEG_RMAP_BINSEARCH) */
  sidx = two_level_search(addr, saddr_out);
  // 2-level search
#else  /* !defined(SEG_RMAP_BINSEARCH) && defined(SEG_RMAP_2LEVEL_SEARCH) */
  // Linear search
  sidx = linear_search(addr, saddr_out);
#endif /* !defined(SEG_RMAP_BINSEARCH) && !defined(SEG_RMAP_2LEVEL_SEARCH) */

#ifdef SEG_RMAP_CACHE
  update_rmap_cache(*saddr_out, sidx);
#endif /* defined(SEG_RMAP_CACHE) */

  profile_inc_rmc_miss_count(); // CPTL reverse map caching profiling
  print_cptl_access(sidx, 0);   // CPTL access profiling
  return sidx;
}