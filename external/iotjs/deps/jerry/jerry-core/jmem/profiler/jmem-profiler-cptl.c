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

#include "jmem-profiler-common-internal.h"
#include "jmem-profiler.h"
#include "jrt.h"

void init_cptl_profiler(void) {
#if defined(PROF_CPTL)
  CHECK_LOGGING_ENABLED();

#if defined(PROF_CPTL_RMC_HIT_RATIO)
  JERRY_CONTEXT(cptl_rmc_access_count) = 0;
  JERRY_CONTEXT(cptl_rmc_miss_count) = 0;
#endif

#if defined(PROF_CPTL_ACCESS)
  JERRY_CONTEXT(cptl_access_count) = 0;
  JERRY_CONTEXT(cptl_access_hit_count_comp) = 0;
  JERRY_CONTEXT(cptl_access_hit_count_decomp) = 0;
  for (int i = 0; i < PROF_CPTL_ACCESS_SET_SIZE; i++) {
    JERRY_CONTEXT(cptl_access_set_entries_comp[i]) = -1;
    JERRY_CONTEXT(cptl_access_set_entries_decomp[i]) = -1;
    JERRY_CONTEXT(cptl_access_set_ages_comp[i]) = -1;
    JERRY_CONTEXT(cptl_access_set_ages_decomp[i]) = -1;
  }
  JERRY_CONTEXT(cptl_access_set_header_comp) = 0;
  JERRY_CONTEXT(cptl_access_set_header_decomp) = 0;

  FILE *fp = fopen(PROF_CPTL_ACCESS_FILENAME, "w");
  // Same segment: old(legacy) name of lru cache success count
  fprintf(fp, "CPTL Access Number, Segment Index, Type/Depth, Same Segment\n");
  fflush(fp);
  fclose(fp);
#endif

#endif
}

inline void __attr_always_inline___ print_cptl_profile_rmc_hit_ratio(void) {
#if defined(SEG_RMAP_CACHE) && defined(PROF_CPTL_RMC_HIT_RATIO)
  CHECK_LOGGING_ENABLED();
  FILE *fp = fopen(PROF_CPTL_FILENAME, "a");

  float rmc_access_count_fp = (float)JERRY_CONTEXT(cptl_rmc_access_count);
  float rmc_miss_count_fp = (float)JERRY_CONTEXT(cptl_rmc_miss_count);
  float rmc_hit_ratio = 1.0f - (rmc_miss_count_fp / rmc_access_count_fp);

  fprintf(fp, "RMC Hit Ratio, RMC Miss Count, RMC Access\n");
  fprintf(fp, "%2.3f, %u, %u\n", rmc_hit_ratio,
          JERRY_CONTEXT(cptl_rmc_miss_count),
          JERRY_CONTEXT(cptl_rmc_access_count));

  fflush(fp);
  fclose(fp);
#endif
}

inline void __attr_always_inline___ profile_inc_rmc_access_count(void) {
#if defined(SEG_RMAP_CACHE) && defined(PROF_CPTL_RMC_HIT_RATIO)
  JERRY_CONTEXT(cptl_rmc_access_count)++;
#endif
}

inline void __attr_always_inline___ profile_inc_rmc_miss_count(void) {
#if defined(SEG_RMAP_CACHE) && defined(PROF_CPTL_RMC_HIT_RATIO)
  JERRY_CONTEXT(cptl_rmc_miss_count)++;
#endif
}

inline void __attr_always_inline___ print_cptl_access(uint32_t sidx,
                                                      int type_depth) {
  // type_depth: -1 = decompression, 0 = compression hit, 1~ = compression miss
#if defined(PROF_CPTL_ACCESS)
  CHECK_LOGGING_ENABLED();
  FILE *fp = fopen(PROF_CPTL_ACCESS_FILENAME, "a");
  uint32_t consec_hit_count = 0;
  if (type_depth < 0) {
    // Decompression
    // Check set entries
    int hit_index = -1;
    for (int i = 0; i < PROF_CPTL_ACCESS_SET_SIZE; i++) {
      if ((int)sidx == JERRY_CONTEXT(cptl_access_set_entries_decomp[i])) {
        hit_index = i;
        break;
      }
    }
    // Update entry table
#ifdef PROF_CPTL_ACCESS_LRU
    if (hit_index >= 0) {
      // Just update ages
      JERRY_CONTEXT(cptl_access_set_ages_decomp[hit_index]) = 0;
    }
#endif
    if (hit_index < 0) {
      // Evict an entry
#ifdef PROF_CPTL_ACCESS_LRU
      // LRU
      int max_age = -1;
      int max_age_index = 0;
      for (int i = 0; i < PROF_CPTL_ACCESS_SET_SIZE; i++) {
        if (max_age < JERRY_CONTEXT(cptl_access_set_ages_decomp[i])) {
          max_age = JERRY_CONTEXT(cptl_access_set_ages_decomp[i]);
          max_age_index = i;
        }
      }
      JERRY_CONTEXT(cptl_access_set_entries_decomp[max_age_index]) = (int)sidx;
      JERRY_CONTEXT(cptl_access_set_ages_decomp[max_age_index]) = 0;
#else
      // FIFO
      int header_index = JERRY_CONTEXT(cptl_access_set_header_decomp);
      JERRY_CONTEXT(cptl_access_set_entries_decomp[header_index]) = (int)sidx;
      JERRY_CONTEXT(cptl_access_set_header_decomp) =
          (header_index + 1) % PROF_CPTL_ACCESS_SET_SIZE;
#endif
    }

#ifdef PROF_CPTL_ACCESS_LRU
    // Update ages
    for (int i = 0; i < PROF_CPTL_ACCESS_SET_SIZE; i++) {
      JERRY_CONTEXT(cptl_access_set_ages_decomp[i])++;
    }
#endif

    // Update consecutive hit count
    if (hit_index >= 0) {
      JERRY_CONTEXT(cptl_access_hit_count_decomp)++;
    } else {
      JERRY_CONTEXT(cptl_access_hit_count_decomp) = 0;
    }
    consec_hit_count = JERRY_CONTEXT(cptl_access_hit_count_decomp);

  } else {
    // Compression
    // Check set entries
    int hit_index = -1;
    for (int i = 0; i < PROF_CPTL_ACCESS_SET_SIZE; i++) {
      if ((int)sidx == JERRY_CONTEXT(cptl_access_set_entries_comp[i])) {
        hit_index = i;
        break;
      }
    }
    // Update entry table
#ifdef PROF_CPTL_ACCESS_LRU
    if (hit_index >= 0) {
      // Just update ages
      JERRY_CONTEXT(cptl_access_set_ages_comp[hit_index]) = 0;
    }
#endif
    if (hit_index < 0) {
      // Evict an entry
#ifdef PROF_CPTL_ACCESS_LRU
      // LRU
      int max_age = -1;
      int max_age_index = 0;
      for (int i = 0; i < PROF_CPTL_ACCESS_SET_SIZE; i++) {
        if (max_age < JERRY_CONTEXT(cptl_access_set_ages_comp[i])) {
          max_age = JERRY_CONTEXT(cptl_access_set_ages_comp[i]);
          max_age_index = i;
        }
      }
      JERRY_CONTEXT(cptl_access_set_entries_comp[max_age_index]) = (int)sidx;
      JERRY_CONTEXT(cptl_access_set_ages_comp[max_age_index]) = 0;
#else
      // FIFO
      int header_index = JERRY_CONTEXT(cptl_access_set_header_comp);
      JERRY_CONTEXT(cptl_access_set_entries_comp[header_index]) = (int)sidx;
      JERRY_CONTEXT(cptl_access_set_header_comp) =
          (header_index + 1) % PROF_CPTL_ACCESS_SET_SIZE;
#endif
    }

#ifdef PROF_CPTL_ACCESS_LRU
    // Update ages
    for (int i = 0; i < PROF_CPTL_ACCESS_SET_SIZE; i++) {
      JERRY_CONTEXT(cptl_access_set_ages_comp[i])++;
    }
#endif

    // Update consecutive hit count
    if (hit_index >= 0) {
      JERRY_CONTEXT(cptl_access_hit_count_comp)++;
    } else {
      JERRY_CONTEXT(cptl_access_hit_count_comp) = 0;
    }
    consec_hit_count = JERRY_CONTEXT(cptl_access_hit_count_comp);
  }
  fprintf(fp, "%u, %u, %d, %u\n", JERRY_CONTEXT(cptl_access_count)++, sidx,
          type_depth, consec_hit_count);
  fflush(fp);
  fclose(fp);
#else
  JERRY_UNUSED(sidx);
  JERRY_UNUSED(type_depth);
#endif
}