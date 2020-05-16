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

#include "cptl-rmap-cache.h"

#include "jcontext.h"
#include "jmem-config.h"
#include "jmem-heap-segmented-rmap-rb.h"
#include "jmem-profiler.h"
#include "jmem.h"

// It is assumed that access_and_check_rmap_cache() is followed by
// update_rmap_cache().
#if defined(SEG_RMAP_CACHE_SIZE) && (SEG_RMAP_CACHE_SIZE != 1)
static uint32_t rmap_cache_tag = 0;
#endif

void init_rmap_cache(void) {
#ifdef SEG_RMAP_CACHE
  // Initialize reverse map cache
#if SEG_RMAP_CACHE_SIZE == 1
  // Case 1. Singleton cache
  JERRY_HEAP_CONTEXT(rmc_single_base_addr) = NULL;
  JERRY_HEAP_CONTEXT(rmc_single_sidx) = SEG_NUM_SEGMENTS;
#else  /* SEG_RMAP_CACHE_SIZE == 1 */
  // Case 2. Multi-entry cache
  for (int tag = 0; tag < SEG_RMAP_CACHE_SIZE; tag++) {
    JERRY_HEAP_CONTEXT(rmc_table_base_addr[tag]) = NULL;
    JERRY_HEAP_CONTEXT(rmc_table_sidx[tag]) = SEG_NUM_SEGMENTS;
  }
#endif /* SEG_RMAP_CACHE_SIZE != 1 */
#endif /* defined(SEG_RMAP_CACHE) */
}

inline uint32_t __attr_always_inline___
access_and_check_rmap_cache(uint8_t *addr, uint8_t **saddr_out) {
// Access and check reverse map cache
#ifdef SEG_RMAP_CACHE
#if SEG_RMAP_CACHE_SIZE == 1
  // Case 1. Singleton cache
  uint8_t *cached_base_addr = JERRY_HEAP_CONTEXT(rmc_single_base_addr);

  intptr_t result = (intptr_t)addr - (intptr_t)cached_base_addr;
  if (result < (intptr_t)SEG_SEGMENT_SIZE && result >= 0) {
    *saddr_out = cached_base_addr;
    uint32_t cached_sidx = JERRY_HEAP_CONTEXT(rmc_single_sidx);
    return cached_sidx;
  } else {
    return SEG_NUM_SEGMENTS;
  }
#elif SEG_RMAP_CACHE_SET_SIZE == 1 /* SEG_RMAP_CACHE_SIZE == 1 */
  // Case 2. Multi-entry cache (Direct-mapped)
  rmap_cache_tag = ((uint32_t)addr >> SEG_SEGMENT_SHIFT) % SEG_RMAP_CACHE_SIZE;
  uint8_t *cached_base_addr =
      JERRY_HEAP_CONTEXT(rmc_table_base_addr[rmap_cache_tag]);

  intptr_t result = (intptr_t)addr - (intptr_t)cached_base_addr;
  if (result < (intptr_t)SEG_SEGMENT_SIZE && result >= 0) {
    *saddr_out = cached_base_addr;
    uint32_t cached_sidx = JERRY_HEAP_CONTEXT(rmc_table_sidx[rmap_cache_tag]);
    return cached_sidx;
  } else {
    return SEG_NUM_SEGMENTS;
  }
#else  /* SEG_RMAP_CACHE_SIZE != 1 && SEG_RMAP_CACHE_SET_SIZE == 1 */
  // Case 3. Multi-entry cache (Set-associative or Fully-associative)
  // TODO: not yet implemented
#endif /* SEG_RMAP_CACHE_SIZE != && SEG_RMAP_CACHE_SET_SIZE == 1 */
#else  /* defined(SEG_RMAP_CACHE) */
  // It will never be called
  JERRY_UNUSED(addr);
  JERRY_UNUSED(saddr_out);
  return NULL;
#endif /* !defined(SEG_RMAP_CACHE) */
}

inline void __attr_always_inline___ update_rmap_cache(uint8_t *saddr,
                                                      uint32_t sidx) {
// Update reverse map cache on slow path (miss)
#ifdef SEG_RMAP_CACHE
#if SEG_RMAP_CACHE_SIZE == 1
  // Case 1. Singleton cache
  JERRY_HEAP_CONTEXT(rmc_single_base_addr) = saddr;
  JERRY_HEAP_CONTEXT(rmc_single_sidx) = sidx;
#elif SEG_RMAP_CACHE_SET_SIZE == 1 /* SEG_RMAP_CACHE_SIZE == 1 */
  // Case 2. Multi-entry cache (Direct-mapped)
  JERRY_HEAP_CONTEXT(rmc_table_base_addr[rmap_cache_tag]) = saddr;
  JERRY_HEAP_CONTEXT(rmc_table_sidx[rmap_cache_tag]) = sidx;
#else  /* SEG_RMAP_CACHE_SIZE != 1 && SEG_RMAP_CACHE_SET_SIZE == 1 */
  // Case 3. Multi-entry cache (Set-associative or Fully-associative)
  // TODO: not yet implemented
#endif /* SEG_RMAP_CACHE_SIZE != && SEG_RMAP_CACHE_SET_SIZE == 1 */
#else  /* defined(SEG_RMAP_CACHE) */
  // It will never be called
  JERRY_UNUSED(saddr);
  JERRY_UNUSED(sidx);
#endif /* !defined(SEG_RMAP_CACHE) */
}

inline void __attr_always_inline___ invalidate_rmap_cache_entry(uint32_t sidx) {
// Access, check and invalidate reverse map cache
#ifdef SEG_RMAP_CACHE
#if SEG_RMAP_CACHE_SIZE == 1
  // Case 1. Singleton cache
  if (JERRY_HEAP_CONTEXT(rmc_single_sidx) == sidx) {
    JERRY_HEAP_CONTEXT(rmc_single_base_addr) = 0;
    JERRY_HEAP_CONTEXT(rmc_single_sidx) = 0;
  }
#elif SEG_RMAP_CACHE_SET_SIZE == 1 /* SEG_RMAP_CACHE_SIZE == 1 */
  // Case 2. Multi-entry cache (Direct-mapped)
  for (int tag = 0; tag < SEG_RMAP_CACHE_SIZE; tag++) {
    if (JERRY_HEAP_CONTEXT(rmc_table_sidx[tag]) == sidx) {
      JERRY_HEAP_CONTEXT(rmc_table_base_addr[tag]) = 0;
      JERRY_HEAP_CONTEXT(rmc_table_sidx[tag]) = 0;
    }
  }
#else  /* SEG_RMAP_CACHE_SIZE != 1 && SEG_RMAP_CACHE_SET_SIZE == 1 */
  // Case 3. Multi-entry cache (Set-associative or Fully-associative)
  // TODO: not yet implemented
#endif /* SEG_RMAP_CACHE_SIZE != && SEG_RMAP_CACHE_SET_SIZE == 1 */
#else  /* defined(SEG_RMAP_CACHE) */
  // It will never be called
  JERRY_UNUSED(addr);
  return NULL;
#endif /* !defined(SEG_RMAP_CACHE) */
}