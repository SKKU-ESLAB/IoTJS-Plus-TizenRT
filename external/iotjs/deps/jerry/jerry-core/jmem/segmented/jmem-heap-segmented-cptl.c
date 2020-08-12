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

#if defined(JMEM_SEGMENTED_HEAP)
// Initialize compressed pointer translation layer (CPTL)
void init_cptl(void) {
  // Initialize segment base table
  for (uint32_t sidx = 0; sidx < SEG_NUM_SEGMENTS; sidx++) {
    JERRY_HEAP_CONTEXT(area[sidx]) = (uint8_t *)NULL;
  }
  JERRY_HEAP_CONTEXT(area[SEG_NUM_SEGMENTS]) = (uint8_t *)JMEM_HEAP_END_OF_LIST;

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
  // JERRY_HEAP_CONTEXT(fc_table_valid_count) = 0;
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

#if defined(PROF_TIME__COMPRESSION_DETAILED) || \
    defined(PROF_PMU__COMPRESSION_CYCLES_DETAILED)
  JERRY_CONTEXT(recent_compression_type) = 0; // COMPRESSION_RMC_HIT
#endif
  profile_compression_start();
  profile_compression_cycles_start();
  sidx = addr_to_saddr_and_sidx((uint8_t *)p);
#if defined(PROF_TIME__COMPRESSION_DETAILED) || \
    defined(PROF_PMU__COMPRESSION_CYCLES_DETAILED)
  profile_compression_cycles_end(JERRY_CONTEXT(recent_compression_type));
  profile_compression_end(JERRY_CONTEXT(recent_compression_type));
#else
  profile_compression_cycles_end(0);
  profile_compression_end(0); // COMPRESSION_RMC_HIT
#endif
  if (likely(sidx < SEG_NUM_SEGMENTS)) {
    cp = JERRY_HEAP_CONTEXT(comp_i_offset) + (sidx << SEG_SEGMENT_SHIFT);
  } else if (p == (uint8_t *)&JERRY_HEAP_CONTEXT(first)) {
    cp = 0;
  } else {
    cp = (uint32_t)JMEM_HEAP_END_OF_LIST_UINT32;
  }

#ifdef PROF_COUNT__COMPRESSION_CALLERS
  profile_inc_count_compression_callers(0); // compression callers
#endif
  return cp;
}

// Address decompression (Compressed pointer -> full-bitwidth pointer)
// * jmem.h
inline jmem_heap_free_t *__attribute__((hot))
cptl_decompress_pointer_internal(uint32_t cp) {
  jmem_heap_free_t *p;
  profile_decompression_start();
  profile_decompression_cycles_start();
  // if (likely(cp != JMEM_HEAP_END_OF_LIST_UINT32)) {
  p = (jmem_heap_free_t *)((uintptr_t)sidx_to_addr(cp >> SEG_SEGMENT_SHIFT) +
                           (uintptr_t)(cp & (SEG_SEGMENT_SIZE - 1)));
  // } else {
  //   p = JMEM_HEAP_END_OF_LIST;
  // }
  profile_decompression_cycles_end();
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

// JERRY_HEAP_CONTEXT(comp_i_saddr)
#if defined(SEG_RMAP_BINSEARCH)
static inline uint32_t __attr_always_inline___ binary_search(uint8_t *addr) {
  // binary_search(uint8_t *addr, uint8_t **saddr_out) {
  seg_rmap_node_t *node =
      segment_rmap_lookup(&JERRY_HEAP_CONTEXT(segment_rmap_rb_root), addr);
  if (node == NULL) {
    return SEG_NUM_SEGMENTS;
  } else {
    JERRY_HEAP_CONTEXT(comp_i_saddr) = node->base_addr;
    return node->sidx;
  }
}
#else /* defined(SEG_RMAP_BINSEARCH) */
static inline uint32_t __attr_always_inline___ linear_search(uint8_t *addr) {
  // linear_search(uint8_t *addr, uint8_t **saddr_out) {
  for (uint32_t sidx = 0; sidx < SEG_NUM_SEGMENTS; sidx++) {
    INCREASE_LOOKUP_DEPTH();
    uint8_t *saddr = JERRY_HEAP_CONTEXT(area[sidx]);
    uint32_t result = (uint32_t)addr - (uint32_t)saddr;
    if (result < (uint32_t)SEG_SEGMENT_SIZE) {
      JERRY_HEAP_CONTEXT(comp_i_offset) = (uint32_t)result;
      JERRY_HEAP_CONTEXT(comp_i_saddr) = saddr;
      return sidx;
    }
  }
  return SEG_NUM_SEGMENTS; // It should never be called.
}

#if defined(SEG_RMAP_2LEVEL_SEARCH)
static inline uint32_t __attr_always_inline___ two_level_search(uint8_t *addr) {
  // two_level_search(uint8_t *addr, uint8_t **saddr_out) {
  uint32_t sidx = SEG_NUM_SEGMENTS;
  // 1st-level search: FIFO cache search
  for (int i = 0; i < SEG_RMAP_2LEVEL_SEARCH_FIFO_CACHE_SIZE; i++) {
    if (i == JERRY_HEAP_CONTEXT(fc_table_eviction_header))
      continue;
    INCREASE_LOOKUP_DEPTH();
    uint8_t *saddr = JERRY_HEAP_CONTEXT(fc_table_base_addr[i]);
    uint32_t result = (uint32_t)addr - (uint32_t)saddr;
    if (result < (uint32_t)SEG_SEGMENT_SIZE) {
      // FIFO cache saerch succeeds
      sidx = JERRY_HEAP_CONTEXT(fc_table_sidx[i]);
      JERRY_HEAP_CONTEXT(comp_i_offset) = (uint32_t)result;
      JERRY_HEAP_CONTEXT(comp_i_saddr) = saddr;
#if defined(PROF_TIME__COMPRESSION_DETAILED) || \
    defined(PROF_PMU__COMPRESSION_CYCLES_DETAILED)
      JERRY_CONTEXT(recent_compression_type) = 1; // COMPRESSION_FIFO_HIT
#endif
      return sidx; // It should be called at least once.
    }
  }

  // FIFO cache search fails
  // 2nd-level search: linear search
  sidx = linear_search(addr);
  // sidx = linear_search(addr, saddr_out);

  // Update FIFO cache
  if (sidx < SEG_NUM_SEGMENTS) {
    JERRY_HEAP_CONTEXT(fc_table_eviction_header) =
        (JERRY_HEAP_CONTEXT(fc_table_eviction_header) + 1) %
        SEG_RMAP_2LEVEL_SEARCH_FIFO_CACHE_SIZE;
    int eviction_header = JERRY_HEAP_CONTEXT(fc_table_eviction_header);
    JERRY_HEAP_CONTEXT(fc_table_base_addr[eviction_header]) =
        JERRY_HEAP_CONTEXT(comp_i_saddr);
    JERRY_HEAP_CONTEXT(fc_table_sidx[eviction_header]) = sidx;
  }

  return sidx;
}

inline void __attr_always_inline___ invalidate_fifo_cache_entry(uint32_t sidx) {
  // Access, check and invalidate reverse map cache
  // uint8_t *new_fc_base_addr[SEG_RMAP_2LEVEL_SEARCH_FIFO_CACHE_SIZE];
  // uint32_t new_fc_sidx[SEG_RMAP_2LEVEL_SEARCH_FIFO_CACHE_SIZE];
  // int header = 0;

  for (int i = 0; i < SEG_RMAP_2LEVEL_SEARCH_FIFO_CACHE_SIZE; i++) {
    if (JERRY_HEAP_CONTEXT(fc_table_sidx[i]) == sidx) {
      JERRY_HEAP_CONTEXT(fc_table_base_addr[i]) = NULL;
      JERRY_HEAP_CONTEXT(fc_table_sidx[i]) = SEG_NUM_SEGMENTS;
      // header++;
      //     new_fc_base_addr[header] =
      //     JERRY_HEAP_CONTEXT(fc_table_base_addr[i]); new_fc_sidx[header] =
      //     JERRY_HEAP_CONTEXT(fc_table_sidx[i]);
      //   }
      // }

      // for (int i = 0; i < SEG_RMAP_2LEVEL_SEARCH_FIFO_CACHE_SIZE; i++) {
      //   if (i < header) {
      //     JERRY_HEAP_CONTEXT(fc_table_base_addr[i]) = new_fc_base_addr[i];
      //     JERRY_HEAP_CONTEXT(fc_table_sidx[i]) = new_fc_sidx[i];
      //   } else {
      //     JERRY_HEAP_CONTEXT(fc_table_base_addr[i]) = NULL;
      //     JERRY_HEAP_CONTEXT(fc_table_sidx[i]) = SEG_NUM_SEGMENTS;
    }
  }

  // int valid_index = header;
  // JERRY_HEAP_CONTEXT(fc_table_valid_count) = (uint32_t)valid_index;
}

#endif /* defined(SEG_RMAP_2LEVEL_SEARCH) */
#endif /* !defined(SEG_RMAP_BINSEARCH) */

// Raw function to access segment reverse map
// * Full-bitwidth pointer -> Segment index
// * Core part of compression
// * jmem-heap-segmented-cptl.h
inline uint32_t __attribute__((hot)) addr_to_saddr_and_sidx(uint8_t *addr) {
  uint32_t sidx;
  CLEAR_LOOKUP_DEPTH();

  profile_inc_rmc_access_count(); // CPTL reverse map caching profiling

  // Fast path
#ifdef SEG_RMAP_CACHE
  sidx = access_and_check_rmap_cache(addr);
  if (likely(sidx < SEG_NUM_SEGMENTS)) {
    print_cptl_access(sidx, 0); // CPTL access profiling
    return sidx;
  }
#endif /* defined(SEG_RMAP_CACHE) */

  // Slow path
#if defined(PROF_TIME__COMPRESSION_DETAILED) || \
    defined(PROF_PMU__COMPRESSION_CYCLES_DETAILED)
  JERRY_CONTEXT(recent_compression_type) = 2; // COMPRESSION_FINAL_MISS
#endif
#if defined(SEG_RMAP_BINSEARCH)
  // Binary search
  sidx = binary_search(addr);
#elif defined(SEG_RMAP_2LEVEL_SEARCH) /* defined(SEG_RMAP_BINSEARCH) */
  sidx = two_level_search(addr);
  // 2-level search
#else  /* !defined(SEG_RMAP_BINSEARCH) && defined(SEG_RMAP_2LEVEL_SEARCH) */
  // Linear search
  sidx = linear_search(addr);
#endif /* !defined(SEG_RMAP_BINSEARCH) && !defined(SEG_RMAP_2LEVEL_SEARCH) */

#ifdef SEG_RMAP_CACHE
  if (likely(sidx < SEG_NUM_SEGMENTS)) {
    update_rmap_cache(sidx);
  }
#endif /* defined(SEG_RMAP_CACHE) */

  profile_inc_rmc_miss_count(); // CPTL reverse map caching profiling
  print_cptl_access(sidx, 1);   // CPTL access profiling
  return sidx;
}
#endif /* defined(JMEM_SEGMENTED_HEAP) */