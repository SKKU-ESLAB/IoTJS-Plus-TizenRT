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

void finalize_cptl_profiler(void) {
#if defined(JMEM_PROFILE) && defined(PROF_CPTL)
#if defined(SEG_RMAP_CACHE)
  print_cptl_profile_rmc_hit_ratio();
#endif
#endif
}

inline void __attr_always_inline___ print_cptl_profile_rmc_hit_ratio(void) {
#if defined(SEG_RMAP_CACHE) && defined(JMEM_PROFILE) && defined(PROF_CPTL)
  CHECK_LOGGING_ENABLED();
  FILE *fp = fopen(PROF_CPTL_FILENAME, "w");
  float rmc_access_count_fp = (float)JERRY_CONTEXT(cptl_rmc_access_count);
  float rmc_miss_count_fp = (float)JERRY_CONTEXT(cptl_rmc_miss_count);
  float rmc_hit_ratio = 1.0f - (rmc_miss_count_fp / rmc_access_count_fp);

  fprintf(fp, "Category, Value\n");
  fprintf(fp, "RMC Hit Ratio, %2.3f\n", rmc_hit_ratio);
  fprintf(fp, "RMC Miss Count, %u\n", JERRY_CONTEXT(cptl_rmc_miss_count));
  fprintf(fp, "RMC Access Count, %u\n", JERRY_CONTEXT(cptl_rmc_access_count));

  fflush(fp);
  fclose(fp);
#endif
}

inline void __attr_always_inline___ profile_inc_rmc_access_count(void) {
#if defined(SEG_RMAP_CACHE) && defined(JMEM_PROFILE) && defined(PROF_CPTL)
  JERRY_CONTEXT(cptl_rmc_access_count)++;
#endif
}

inline void __attr_always_inline___ profile_inc_rmc_miss_count(void) {
#if defined(SEG_RMAP_CACHE) && defined(JMEM_PROFILE) && defined(PROF_CPTL)
  JERRY_CONTEXT(cptl_rmc_miss_count)++;
#endif
}