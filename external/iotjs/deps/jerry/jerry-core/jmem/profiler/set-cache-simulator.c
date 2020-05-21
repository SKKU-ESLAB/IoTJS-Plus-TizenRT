/* Copyright 2016-2020 Gyeonghwan Hong, Sungkyunkwan University
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

#include "set-cache-simulator.h"
#include "jcontext.h"
#include "jmem-config.h"
#include "jrt.h"

#if defined(PROF_CPTL_ACCESS_SIMUL)
static uint32_t simulate_cache_internal(uint32_t sidx, int* set_entries,
                                        int* set_ages, int* set_header,
                                        uint32_t* hit_count);
#endif

uint32_t simulate_cache(uint32_t sidx, int type) {
#if defined(PROF_CPTL_ACCESS_SIMUL)
  if (type < 0) {
    // Decompression
    return simulate_cache_internal(sidx,
                                   JERRY_CONTEXT(
                                       cptl_access_set_entries_decomp),
                                   JERRY_CONTEXT(cptl_access_set_ages_decomp),
                                   &JERRY_CONTEXT(
                                       cptl_access_set_header_decomp),
                                   &JERRY_CONTEXT(
                                       cptl_access_hit_count_decomp));
  } else {
    // Compression
    return simulate_cache_internal(sidx,
                                   JERRY_CONTEXT(cptl_access_set_entries_comp),
                                   JERRY_CONTEXT(cptl_access_set_ages_comp),
                                   &JERRY_CONTEXT(cptl_access_set_header_comp),
                                   &JERRY_CONTEXT(cptl_access_hit_count_comp));
  }
#else  /* defined(PROF_CPTL_ACCESS_SIMUL) */
  JERRY_UNUSED(sidx);
  JERRY_UNUSED(type);
  return 0;
#endif /* !defined(PROF_CPTL_ACCESS_SIMUL) */
}

#if defined(PROF_CPTL_ACCESS_SIMUL)
uint32_t simulate_cache_internal(uint32_t sidx, int* set_entries, int* set_ages,
                                 int* set_header, uint32_t* hit_count) {
  uint32_t consec_hit_count = 0;
  // Check set entries
  int hit_index = -1;
  for (int i = 0; i < PROF_CPTL_ACCESS_SIMUL_SET_SIZE; i++) {
    if ((int)sidx == set_entries[i]) {
      hit_index = i;
      break;
    }
  }
  // Update entry table
#ifdef PROF_CPTL_ACCESS_SIMUL_LRU
  if (hit_index >= 0) {
    // Just update ages
    set_ages[hit_index] = 0;
  }
#endif
  if (hit_index < 0) {
    // Evict an entry
#ifdef PROF_CPTL_ACCESS_SIMUL_LRU
    // LRU
    int max_age = -1;
    int max_age_index = 0;
    for (int i = 0; i < PROF_CPTL_ACCESS_SIMUL_SET_SIZE; i++) {
      if (max_age < set_ages[i]) {
        max_age = set_ages[i];
        max_age_index = i;
      }
    }
    set_entries[max_age_index] = (int)sidx;
    set_ages[max_age_index] = 0;
    JERRY_UNUSED(set_header);
#else
    // FIFO
    int header_index = *set_header;
    set_entries[header_index] = (int)sidx;
    *set_header = (header_index + 1) % PROF_CPTL_ACCESS_SIMUL_SET_SIZE;
#endif
  }

#ifdef PROF_CPTL_ACCESS_SIMUL_LRU
  // Update ages
  for (int i = 0; i < PROF_CPTL_ACCESS_SIMUL_SET_SIZE; i++) {
    set_ages[i]++;
  }
#endif

  // Update consecutive hit count
  if (hit_index >= 0) {
    *hit_count = *hit_count + 1;
  } else {
    *hit_count = 0;
  }
  consec_hit_count = *hit_count;

  return consec_hit_count;
}
#endif /* defined(PROF_CPTL_ACCESS_SIMUL) */