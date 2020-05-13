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

#if defined(PROF_CPTL_COMPRESSION_CALL_COUNT)
  for (int i = 0; i < PROF_CPTL_COMPRESSION_CALL_COUNT_TYPES; i++) {
    JERRY_CONTEXT(cptl_compression_call_count[i]) = 0;
  }
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

inline void __attr_always_inline___ print_cptl_compression_call_count(void) {
#if defined(JMEM_SEGMENTED_HEAP) && defined(PROF_CPTL_COMPRESSION_CALL_COUNT)
  CHECK_LOGGING_ENABLED();
  FILE *fp = fopen(PROF_CPTL_FILENAME, "a");

  for (int i = 0; i < PROF_CPTL_COMPRESSION_CALL_COUNT_TYPES; i++) {
    if (i > 0) {
      fprintf(fp, ", ");
    }
    fprintf(fp, "%d", i);
  }
  fprintf(fp, "\n");
  for (int i = 0; i < PROF_CPTL_COMPRESSION_CALL_COUNT_TYPES; i++) {
    if (i > 0) {
      fprintf(fp, ", ");
    }
    fprintf(fp, "%d", JERRY_CONTEXT(cptl_compression_call_count[i]));
  }
  fprintf(fp, "\n");

  fflush(fp);
  fclose(fp);
#endif
}

inline void __attr_always_inline___
profile_inc_compression_call_count(int type) {
#if defined(JMEM_SEGMENTED_HEAP) && defined(PROF_CPTL_COMPRESSION_CALL_COUNT)
  JERRY_ASSERT(type < PROF_CPTL_COMPRESSION_CALL_COUNT_TYPES && type >= 0);
  JERRY_CONTEXT(cptl_compression_call_count[type])++;
#else
  JERRY_UNUSED(type);
#endif
}