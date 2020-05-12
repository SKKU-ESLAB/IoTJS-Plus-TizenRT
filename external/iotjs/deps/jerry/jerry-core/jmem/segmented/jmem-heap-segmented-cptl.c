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

#define JMEM_HEAP_GET_OFFSET_FROM_PTR(p, seg_ptr) \
  ((uint32_t)((uint8_t *)(p) - (uint8_t *)(seg_ptr)))

inline uint32_t __attribute__((hot))
jmem_heap_get_offset_from_addr_segmented(jmem_heap_free_t *addr) {
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
inline jmem_heap_free_t *__attribute__((hot))
jmem_heap_get_addr_from_offset_segmented(uint32_t u) {
  if (u == JMEM_HEAP_END_OF_LIST_UINT32)
    return JMEM_HEAP_END_OF_LIST;
  return (jmem_heap_free_t *)((uintptr_t)JERRY_HEAP_CONTEXT(
                                  area[u >> SEG_SEGMENT_SHIFT]) +
                              (uintptr_t)(u % SEG_SEGMENT_SIZE));
}

/**
 * Addr <-> offset translation
 */
inline uint8_t *__attribute__((hot)) sidx_to_addr(uint32_t sidx) {
  return JERRY_HEAP_CONTEXT(area[sidx]);
}
inline uint32_t __attribute__((hot))
addr_to_saddr_and_sidx(uint8_t *addr, uint8_t **saddr_out) {
  uint8_t *saddr = NULL;
  uint32_t sidx;

  profile_inc_rmc_access_count(); // CPTL reverse map caching profiling

#ifdef SEG_RMAP_CACHING
  // Caching
  uint8_t *curr_addr = JERRY_HEAP_CONTEXT(recent_base_addr);
  intptr_t result = (intptr_t)addr - (intptr_t)curr_addr;
  if (result < (intptr_t)SEG_SEGMENT_SIZE && result >= 0) {
    *saddr_out = curr_addr;
    return JERRY_HEAP_CONTEXT(recent_sidx);
  }
#endif

#ifndef SEG_RMAP_BINSEARCH
  for (sidx = 0; sidx < SEG_NUM_SEGMENTS; sidx++) {
    saddr = JERRY_HEAP_CONTEXT(area[sidx]);
    if (saddr != NULL && (uint32_t)(addr - saddr) < (uint32_t)SEG_SEGMENT_SIZE)
      break;
  }
#else  /* SEG_RMAP_BINSEARCH */
  seg_rmap_node_t *node =
      segment_rmap_lookup(&JERRY_HEAP_CONTEXT(segment_rmap_rb_root), addr);
  sidx = node->sidx;
  saddr = node->base_addr;
#endif /* !SEG_RMAP_BINSEARCH */

#ifdef SEG_RMAP_CACHING
  JERRY_HEAP_CONTEXT(recent_sidx) = sidx;
  JERRY_HEAP_CONTEXT(recent_base_addr) = saddr;
#endif

  *saddr_out = saddr;

  profile_inc_rmc_miss_count(); // CPTL reverse map caching profiling
  return sidx;
}