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

#include <stdio.h>

#include "jmem-profiler-common-internal.h"
#include "jmem-profiler.h"

#if defined(JMEM_PROFILE) && defined(PROF_PMU)
void __start_pmu_cycles_watch(unsigned long long *cycles_watch);
void __stop_pmu_cycles_watch(unsigned long long *cycles_watch,
                             unsigned long long *cycles,
                             unsigned int *count);
#endif

/* PMU profiling */
inline void __attr_always_inline___ init_pmu_profiler(void) {
#if defined(JMEM_PROFILE) && defined(PROF_PMU)
#define INIT_PROFILER_PMU_ENTRY(x)   \
  JERRY_CONTEXT(x##_cycles) = 0;     \
  JERRY_CONTEXT(x##_pmu_count) = 0

  CHECK_LOGGING_ENABLED();
  JERRY_CONTEXT(compression_cycles_val) = 0;
  JERRY_CONTEXT(decompression_cycles_val) = 0;
  INIT_PROFILER_PMU_ENTRY(decompression);
  INIT_PROFILER_PMU_ENTRY(compression_rmc_hit);
  INIT_PROFILER_PMU_ENTRY(compression_fifo_hit);
  INIT_PROFILER_PMU_ENTRY(compression_final_miss);
#endif
}

void print_pmu_profile(void) {
#if defined(JMEM_PROFILE) && defined(PROF_PMU)
#define TOTAL_CYCLES(x) (unsigned long long)JERRY_CONTEXT(x##_cycles)
#define AVG_CYCLES(x)                                                      \
  (JERRY_CONTEXT(x##_pmu_count) > 0)                                       \
      ? TOTAL_CYCLES(x) / (unsigned long long)JERRY_CONTEXT(x##_pmu_count) \
      : 0
#define AVG_CYCLES3(x, y, z)                                    \
  ((unsigned long long)JERRY_CONTEXT(x##_pmu_count) +           \
       (unsigned long long)JERRY_CONTEXT(y##_pmu_count) +       \
       (unsigned long long)JERRY_CONTEXT(z##_pmu_count) >       \
   0)                                                           \
      ? (TOTAL_CYCLES3(x, y, z)) /                              \
            ((unsigned long long)JERRY_CONTEXT(x##_pmu_count) + \
             (unsigned long long)JERRY_CONTEXT(y##_pmu_count) + \
             (unsigned long long)JERRY_CONTEXT(z##_pmu_count))  \
      : 0
#define TOTAL_CYCLES3(x, y, z) \
  (TOTAL_CYCLES(x) + TOTAL_CYCLES(y) + TOTAL_CYCLES(z))
#define PRINT_PMU_HEADER(fp) fprintf(fp, "PMU")
#define PRINT_CYCLES(fp, x) fprintf(fp, ", %llu", x)
#define PRINT_AVG_CYCLES(fp, x) fprintf(fp, ", %llu", AVG_CYCLES(x))
#define PRINT_AVG_CYCLES3(fp, x, y, z) \
  fprintf(fp, ", %llu", AVG_CYCLES3(x, y, z))
#define PRINT_TOTAL_CYCLES(fp, x) fprintf(fp, ", %llu", TOTAL_CYCLES(x))
#define PRINT_TOTAL_CYCLES3(fp, x, y, z) \
  fprintf(fp, ", %llu", TOTAL_CYCLES3(x, y, z))
#define PRINT_PMU_TAIL(fp) fprintf(fp, "\n")

  CHECK_LOGGING_ENABLED();
  FILE *fp = fopen(PROF_PMU_FILENAME, "a");

  PRINT_PMU_HEADER(fp);
  PRINT_AVG_CYCLES(fp, decompression);
  PRINT_AVG_CYCLES3(fp, compression_rmc_hit, compression_fifo_hit,
                    compression_final_miss);
  PRINT_AVG_CYCLES(fp, compression_rmc_hit);
  PRINT_AVG_CYCLES(fp, compression_fifo_hit);
  PRINT_AVG_CYCLES(fp, compression_final_miss);
  PRINT_TOTAL_CYCLES(fp, decompression);
  PRINT_TOTAL_CYCLES3(fp, compression_rmc_hit, compression_fifo_hit,
                      compression_final_miss);
  PRINT_TOTAL_CYCLES(fp, compression_rmc_hit);
  PRINT_TOTAL_CYCLES(fp, compression_fifo_hit);
  PRINT_TOTAL_CYCLES(fp, compression_final_miss);
  PRINT_PMU_TAIL(fp);
  fflush(fp);
  fclose(fp);
#endif
}

void __attr_always_inline___ profile_compression_cycles_start(void) {
#if defined(PROF_PMU__COMPRESSION_CYCLES)
  CHECK_LOGGING_ENABLED();
  __start_pmu_cycles_watch(&JERRY_CONTEXT(compression_cycles_val));
#endif
}
inline void __attr_always_inline___ profile_compression_cycles_end(int type) {
#if defined(PROF_PMU__COMPRESSION_CYCLES)
  CHECK_LOGGING_ENABLED();
  if (type == 0) { // COMPRESSION_RMC_HIT
    __stop_pmu_cycles_watch(&JERRY_CONTEXT(compression_cycles_val),
                            &JERRY_CONTEXT(compression_rmc_hit_cycles),
                            &JERRY_CONTEXT(compression_rmc_hit_pmu_count));
  } else if (type == 1) { // COMPRESSION_FIFO_HIT
    __stop_pmu_cycles_watch(&JERRY_CONTEXT(compression_cycles_val),
                            &JERRY_CONTEXT(compression_fifo_hit_cycles),
                            &JERRY_CONTEXT(compression_fifo_hit_pmu_count));
  } else if (type == 2) { // COMPRESSION_FINAL_MISS
    __stop_pmu_cycles_watch(&JERRY_CONTEXT(compression_cycles_val),
                            &JERRY_CONTEXT(compression_final_miss_cycles),
                            &JERRY_CONTEXT(compression_final_miss_pmu_count));
  } else {
    printf("Invalid argument to profile_compression_end: %d\n", type);
  }
#else
  JERRY_UNUSED(type);
#endif
}

inline void __attr_always_inline___ profile_decompression_cycles_start(void) {
#if defined(PROF_PMU__DECOMPRESSION_CYCLES)
  CHECK_LOGGING_ENABLED();
  __start_pmu_cycles_watch(&JERRY_CONTEXT(decompression_cycles_val));
#endif
}
inline void __attr_always_inline___ profile_decompression_cycles_end(void) {
#if defined(PROF_PMU__DECOMPRESSION_CYCLES)
  CHECK_LOGGING_ENABLED();
  __stop_pmu_cycles_watch(&JERRY_CONTEXT(decompression_cycles_val),
                          &JERRY_CONTEXT(decompression_cycles),
                          &JERRY_CONTEXT(decompression_pmu_count));
#endif
}

/* Internal functions */
#if defined(JMEM_PROFILE) && defined(PROF_PMU)
static inline unsigned long arm_pmu_read_cycles(void) {
#if defined(__GNUC__) && defined(__ARM_ARCH_7A__)
  unsigned long r = 0;
  __asm__ volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(r));
  return r;
#else
  return 0;
#endif
}

void __start_pmu_cycles_watch(unsigned long long *cycles_watch) {
  *cycles_watch = (unsigned long long)arm_pmu_read_cycles();
}
void __stop_pmu_cycles_watch(unsigned long long *cycles_watch,
                             unsigned long long *cycles,
                             unsigned int *count) {
  unsigned long long curr_cycles;
  curr_cycles = (unsigned long long)arm_pmu_read_cycles();
  unsigned long long my_cycles = (curr_cycles - *cycles_watch);
  my_cycles = (curr_cycles - *cycles_watch);
  if(my_cycles < 10000) {
    *cycles += my_cycles;
    *count += 1;
  } else {
    JERRY_UNUSED(cycles);
    JERRY_UNUSED(count);
  }
}
#endif /* defined(JMEM_PROFILE) && defined(PROF_PMU) */
