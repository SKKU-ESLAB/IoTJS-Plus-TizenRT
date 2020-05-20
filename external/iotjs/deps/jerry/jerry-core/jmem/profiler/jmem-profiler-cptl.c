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

  FILE *fp = fopen(PROF_CPTL_ACCESS_FILENAME, "w");
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
  uint32_t recent_sidx = 0;
  uint32_t is_same_segment = 0;
  if (type_depth < 0) {
    // decompression
    recent_sidx = JERRY_CONTEXT(cptl_access_recent_decompression_sidx);
    JERRY_CONTEXT(cptl_access_recent_decompression_sidx) = sidx;
    is_same_segment = (recent_sidx == sidx) ? 1 : 0;
    if (is_same_segment) {
      JERRY_CONTEXT(cptl_access_sameseg_decompression_count)++;
      is_same_segment = JERRY_CONTEXT(cptl_access_sameseg_decompression_count);
    } else {
      JERRY_CONTEXT(cptl_access_sameseg_decompression_count) = 0;
    }
  } else {
    // compression
    recent_sidx = JERRY_CONTEXT(cptl_access_recent_compression_sidx);
    JERRY_CONTEXT(cptl_access_recent_compression_sidx) = sidx;
    is_same_segment = (recent_sidx == sidx) ? 1 : 0;
    if (is_same_segment) {
      JERRY_CONTEXT(cptl_access_sameseg_compression_count)++;
      is_same_segment = JERRY_CONTEXT(cptl_access_sameseg_compression_count);
    } else {
      JERRY_CONTEXT(cptl_access_sameseg_compression_count) = 0;
    }
  }
  fprintf(fp, "%u, %u, %d, %u\n", JERRY_CONTEXT(cptl_access_count)++, sidx,
          type_depth, is_same_segment);
  fflush(fp);
  fclose(fp);
#else
  JERRY_UNUSED(sidx);
  JERRY_UNUSED(type_depth);
#endif
}
