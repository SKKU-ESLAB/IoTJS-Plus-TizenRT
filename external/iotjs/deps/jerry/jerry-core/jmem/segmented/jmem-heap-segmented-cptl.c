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

#ifdef SEG_RMAP_CACHE
  init_rmap_cache();
#endif /* defined(SEG_RMAP_CACHE) */
}

// Address decompression (Compressed pointer -> full-bitwidth pointer)
// * jmem.h
inline jmem_heap_free_t *__attribute__((hot))
cptl_decompress_pointer_internal(uint32_t u) {
  if (u == JMEM_HEAP_END_OF_LIST_UINT32)
    return JMEM_HEAP_END_OF_LIST;
  return (jmem_heap_free_t *)((uintptr_t)sidx_to_addr(u >> SEG_SEGMENT_SHIFT) +
                              (uintptr_t)(u % SEG_SEGMENT_SIZE));
}

// Address compression (Full-bitwidth pointer -> compressed pointer)
// * jmem.h
inline uint32_t __attribute__((hot))
cptl_compress_pointer_internal(jmem_heap_free_t *addr) {
  uint32_t sidx;
  uint8_t *saddr;

  JERRY_ASSERT(addr != NULL);

  if (addr == (uint8_t *)JMEM_HEAP_END_OF_LIST)
    return (uint32_t)JMEM_HEAP_END_OF_LIST_UINT32;

  if (addr == (uint8_t *)&JERRY_HEAP_CONTEXT(first))
    return 0;

  sidx = addr_to_saddr_and_sidx((uint8_t *)addr, &saddr);

  return (uint32_t)(JMEM_HEAP_GET_OFFSET_FROM_PTR(addr, saddr) +
                    (uint32_t)SEG_SEGMENT_SIZE * sidx);
}

// Raw function to access segment base table
// * Segment index -> Segment base address(Full-bitwidth pointer)
// * Core part of decompression
// * jmem-heap-segmented-cptl.h
inline uint8_t *__attribute__((hot)) sidx_to_addr(uint32_t sidx) {
  return JERRY_HEAP_CONTEXT(area[sidx]);
}

// Raw function to access segment reverse map
// * Full-bitwidth pointer -> Segment index
// * Core part of compression
// * jmem-heap-segmented-cptl.h
inline uint32_t __attribute__((hot))
addr_to_saddr_and_sidx(uint8_t *addr, uint8_t **saddr_out) {
  uint8_t *saddr = NULL;
  uint32_t sidx;

  profile_inc_rmc_access_count(); // CPTL reverse map caching profiling

#ifdef SEG_RMAP_CACHE
  uint32_t result_cache = access_and_check_rmap_cache(addr, saddr_out);
  if (result_cache < SEG_NUM_SEGMENTS)
    return result_cache;
#endif /* defined(SEG_RMAP_CACHE) */

#ifndef SEG_RMAP_BINSEARCH
  for (sidx = 0; sidx < SEG_NUM_SEGMENTS; sidx++) {
    saddr = JERRY_HEAP_CONTEXT(area[sidx]);
    if (saddr != NULL && (uint32_t)(addr - saddr) < (uint32_t)SEG_SEGMENT_SIZE)
      break;
  }
#else  /* defined(SEG_RMAP_BINSEARCH) */
  seg_rmap_node_t *node =
      segment_rmap_lookup(&JERRY_HEAP_CONTEXT(segment_rmap_rb_root), addr);
  sidx = node->sidx;
  saddr = node->base_addr;
#endif /* !defined(SEG_RMAP_BINSEARCH) */

#ifdef SEG_RMAP_CACHE
  update_rmap_cache(saddr, sidx);
#endif /* defined(SEG_RMAP_CACHE) */

  *saddr_out = saddr;

  profile_inc_rmc_miss_count(); // CPTL reverse map caching profiling
  return sidx;
}