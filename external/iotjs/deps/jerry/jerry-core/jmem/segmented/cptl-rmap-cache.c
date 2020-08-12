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

void init_rmap_cache(void) {
#ifdef SEG_RMAP_CACHE
  // Initialize reverse map cache
#if SEG_RMAP_CACHE_SIZE == 1
  // Case 1. Singleton cache
  JERRY_HEAP_CONTEXT(rmc_single_base_addr) = NULL;
  JERRY_HEAP_CONTEXT(rmc_single_sidx) = SEG_NUM_SEGMENTS;
#else  /* SEG_RMAP_CACHE_SIZE == 1 */
  // Case 2. Multi-entry cache
  for (int index = 0; index < SEG_RMAP_CACHE_SIZE; index++) {
    JERRY_HEAP_CONTEXT(rmc_table_base_addr[index]) = NULL;
    JERRY_HEAP_CONTEXT(rmc_table_sidx[index]) = SEG_NUM_SEGMENTS;
  }
#endif /* SEG_RMAP_CACHE_SIZE != 1 */
#endif /* defined(SEG_RMAP_CACHE) */
}

inline uint32_t __attr_always_inline___
access_and_check_rmap_cache(uint8_t *addr) {
// access_and_check_rmap_cache(uint8_t *addr, uint8_t **saddr_out) {
// Access and check reverse map cache
#ifdef SEG_RMAP_CACHE
#if SEG_RMAP_CACHE_SIZE == 1
  // Case 1. Singleton cache
  uint8_t *cached_base_addr = JERRY_HEAP_CONTEXT(rmc_single_base_addr);

  // intptr_t result = (intptr_t)addr - (intptr_t)cached_base_addr;
  // if (result < (intptr_t)SEG_SEGMENT_SIZE && result >= 0) {
  uint32_t result = (uint32_t)addr - (uint32_t)cached_base_addr;
  if (result < (uint32_t)SEG_SEGMENT_SIZE) {
    JERRY_HEAP_CONTEXT(comp_i_offset) = result;
    JERRY_HEAP_CONTEXT(comp_i_saddr) = cached_base_addr;
    // *saddr_out = cached_base_addr;
    uint32_t cached_sidx = JERRY_HEAP_CONTEXT(rmc_single_sidx);
    return cached_sidx;
  } else {
    return SEG_NUM_SEGMENTS;
  }
#elif SEG_RMAP_CACHE_SET_SIZE == 1
  // Case 2. Multi-entry cache (Direct-mapped)
  JERRY_HEAP_CONTEXT(rmc_index) =
      ((uint32_t)addr >> SEG_SEGMENT_SHIFT) % SEG_RMAP_CACHE_WAYS;
  uint32_t index = JERRY_HEAP_CONTEXT(rmc_index);
  uint8_t *cached_base_addr = JERRY_HEAP_CONTEXT(rmc_table_base_addr[index]);

  // intptr_t result = (intptr_t)addr - (intptr_t)cached_base_addr;
  // if (result < (intptr_t)SEG_SEGMENT_SIZE && result >= 0) {
  uint32_t result = (uint32_t)addr - (uint32_t)cached_base_addr;
  if (likely(result < (uint32_t)SEG_SEGMENT_SIZE)) {
    JERRY_HEAP_CONTEXT(comp_i_offset) = result;
    JERRY_HEAP_CONTEXT(comp_i_saddr) = cached_base_addr;
    // *saddr_out = cached_base_addr;
    uint32_t cached_sidx = JERRY_HEAP_CONTEXT(rmc_table_sidx[index]);
    return cached_sidx; // Cache hit
  } else {
    return SEG_NUM_SEGMENTS; // Cache miss
  }
#elif SEG_RMAP_CACHE_SIZE == SEG_RMAP_CACHE_SET_SIZE
  // Case 3. Multi-entry cache (Fully-associative)
  for (int i = 0; i < SEG_RMAP_CACHE_SIZE; i++) {
    uint8_t *cached_base_addr = JERRY_HEAP_CONTEXT(rmc_table_base_addr[i]);
    // intptr_t result = (intptr_t)addr - (intptr_t)cached_base_addr;
    // if (result < (intptr_t)SEG_SEGMENT_SIZE && result >= 0) {
    uint32_t result = (uint32_t)addr - (uint32_t)cached_base_addr;
    if (likely(result < (uint32_t)SEG_SEGMENT_SIZE)) {
      JERRY_HEAP_CONTEXT(comp_i_offset) = result;
      JERRY_HEAP_CONTEXT(comp_i_saddr) = cached_base_addr;
      // *saddr_out = cached_base_addr;
      uint32_t cached_sidx = JERRY_HEAP_CONTEXT(rmc_table_sidx[i]);
      return cached_sidx; // Cache hit
    }
  }
  return SEG_NUM_SEGMENTS; // Cache miss
#elif SEG_RMAP_CACHE_SIZE > SEG_RMAP_CACHE_SET_SIZE
  // Case 4. Multi-entry cache (Set-associative)
  JERRY_HEAP_CONTEXT(rmc_index) =
      ((uint32_t)addr >> SEG_SEGMENT_SHIFT) % SEG_RMAP_CACHE_WAYS;
  uint32_t index = JERRY_HEAP_CONTEXT(rmc_index);
  int i_from = (int)index * SEG_RMAP_CACHE_SET_SIZE;
  int i_to = ((int)index + 1) * SEG_RMAP_CACHE_SET_SIZE;
  for (int i = i_from; i < i_to; i++) {
    uint8_t *cached_base_addr = JERRY_HEAP_CONTEXT(rmc_table_base_addr[i]);
    // intptr_t result = (intptr_t)addr - (intptr_t)cached_base_addr;
    // if (result < (intptr_t)SEG_SEGMENT_SIZE && result >= 0) {
    uint32_t result = (uint32_t)addr - (uint32_t)cached_base_addr;
    if (likely(result < (uint32_t)SEG_SEGMENT_SIZE)) {
      JERRY_HEAP_CONTEXT(comp_i_offset) = result;
      JERRY_HEAP_CONTEXT(comp_i_saddr) = cached_base_addr;
      // *saddr_out = cached_base_addr;
      uint32_t cached_sidx = JERRY_HEAP_CONTEXT(rmc_table_sidx[i]);
      return cached_sidx; // Cache hit
    }
  }
  return SEG_NUM_SEGMENTS; // Cache miss
#else
  // Invalid option -- It will never be called
  JERRY_UNUSED(addr);
  // JERRY_UNUSED(saddr_out);
  return SEG_NUM_SEGMENTS;
#endif /* SEG_RMAP_CACHE_SIZE, SEG_RMAP_CACHE_SET_SIZE */
#else  /* defined(SEG_RMAP_CACHE) */
  JERRY_UNUSED(addr);
  // JERRY_UNUSED(saddr_out);
  return NULL;
#endif /* !defined(SEG_RMAP_CACHE) */
}

inline void __attr_always_inline___ update_rmap_cache(uint32_t sidx) {
// inline void __attr_always_inline___ update_rmap_cache(uint8_t *saddr,
//                                                       uint32_t sidx) {
// Update reverse map cache on slow path (miss)
#ifdef SEG_RMAP_CACHE
#if SEG_RMAP_CACHE_SIZE == 1
  // Case 1. Singleton cache
  JERRY_HEAP_CONTEXT(rmc_single_base_addr) = JERRY_HEAP_CONTEXT(comp_i_saddr);
  // JERRY_HEAP_CONTEXT(rmc_single_base_addr) = saddr;
  JERRY_HEAP_CONTEXT(rmc_single_sidx) = sidx;
#elif SEG_RMAP_CACHE_SET_SIZE == 1
  // Case 2. Multi-entry cache (Direct-mapped)
  uint32_t index = JERRY_HEAP_CONTEXT(rmc_index);
  JERRY_HEAP_CONTEXT(rmc_table_base_addr[index]) =
      JERRY_HEAP_CONTEXT(comp_i_saddr);
  // JERRY_HEAP_CONTEXT(rmc_table_base_addr[index]) = saddr;
  JERRY_HEAP_CONTEXT(rmc_table_sidx[index]) = sidx;
#elif SEG_RMAP_CACHE_SIZE == SEG_RMAP_CACHE_SET_SIZE
  // Case 3. Multi-entry cache (Fully-associative)
  uint32_t eviction_header = JERRY_HEAP_CONTEXT(rmc_table_eviction_header);
  JERRY_HEAP_CONTEXT(rmc_table_eviction_header) =
      (eviction_header + 1) % SEG_RMAP_CACHE_SET_SIZE;
  JERRY_HEAP_CONTEXT(rmc_table_base_addr[eviction_header]) =
      JERRY_HEAP_CONTEXT(comp_i_saddr);
  // JERRY_HEAP_CONTEXT(rmc_table_base_addr[eviction_header]) = saddr;
  JERRY_HEAP_CONTEXT(rmc_table_sidx[eviction_header]) = sidx;
#elif SEG_RMAP_CACHE_SIZE > SEG_RMAP_CACHE_SET_SIZE
  // Case 4. Multi-entry cache (Set-associative)
  uint32_t index = JERRY_HEAP_CONTEXT(rmc_index);
  uint32_t way_eviction_header =
      JERRY_HEAP_CONTEXT(rmc_table_eviction_headers[index]);
  uint32_t eviction_header =
      index * SEG_RMAP_CACHE_SET_SIZE + way_eviction_header;
  JERRY_HEAP_CONTEXT(rmc_table_eviction_headers[index]) =
      (way_eviction_header + 1) % SEG_RMAP_CACHE_SET_SIZE;
  JERRY_HEAP_CONTEXT(rmc_table_base_addr[eviction_header]) =
      JERRY_HEAP_CONTEXT(comp_i_saddr);
  // JERRY_HEAP_CONTEXT(rmc_table_base_addr[eviction_header]) = saddr;
  JERRY_HEAP_CONTEXT(rmc_table_sidx[eviction_header]) = sidx;
#else
  // Invalid option -- It will never be called
  // JERRY_UNUSED(saddr);
  JERRY_UNUSED(sidx);
#endif /* SEG_RMAP_CACHE_SIZE, SEG_RMAP_CACHE_SET_SIZE */
#else  /* defined(SEG_RMAP_CACHE) */
  // JERRY_UNUSED(saddr);
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
#else  /* SEG_RMAP_CACHE_SIZE == 1 */
  // Case 2. Multi-entry cache
  for (int i = 0; i < SEG_RMAP_CACHE_SIZE; i++) {
    if (JERRY_HEAP_CONTEXT(rmc_table_sidx[i]) == sidx) {
      JERRY_HEAP_CONTEXT(rmc_table_base_addr[i]) = 0;
      JERRY_HEAP_CONTEXT(rmc_table_sidx[i]) = 0;
    }
  }
#endif /* SEG_RMAP_CACHE_SIZE != 1 */
#else  /* defined(SEG_RMAP_CACHE) */
  JERRY_UNUSED(sidx);
  return NULL;
#endif /* !defined(SEG_RMAP_CACHE) */
}