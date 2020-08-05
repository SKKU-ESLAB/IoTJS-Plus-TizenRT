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

#include <malloc.h>
#include <stdio.h>
#include <sys/time.h>

#include "hashtable.h"
#include "jmem-profiler-common-internal.h"
#include "jmem-profiler.h"

static void __print_total_size_profile(void);

/* Prototypes for our hooks.  */
static void jmem_profiler_init_malloc_hook(void);
static void *jmem_profiler_malloc_hook(size_t, const void *);

/* Variables to save original hooks. */
static void *(*old_malloc_hook)(size_t, const void *);
static void (*old_free_hook)(void *, const void *);

/* Override initializing hook from the C library. */
void (*__malloc_initialize_hook)(void) = jmem_profiler_init_malloc_hook;

// Hash table to store heap objects: key=<void* ptr> value=<size_t size>
static HashTable g_heap_objects_ht;

static void jmem_profiler_init_malloc_hook(void) {
#if defined(PROF_SIZE)
  ht_setup(&g_heap_objects_ht, sizeof(void *), sizeof(size_t), 10);
  ht_reserve(&table, 100);

  old_malloc_hook = __malloc_hook;
  old_free_hook = __free_hook;
  __malloc_hook = jmem_profiler_malloc_hook;
  __free_hook = jmem_profiler_free_hook;
#endif
}

static void *jmem_profiler_malloc_hook(size_t size, const void *caller) {
#if defined(PROF_SIZE)
  void *result;

  // Restore old hooks
  __malloc_hook = old_malloc_hook;
  __free_hook = old_free_hook;

  // Inner call of malloc
  result = malloc(size);

  ht_insert(&g_heap_objects_ht, (void *)&result, (void *)&size);
  JERRY_CONTEXT(jmem_total_heap_size) += size;
  

  // Restore our hooks
  old_malloc_hook = __malloc_hook;
  old_free_hook = __free_hook;
  __malloc_hook = jmem_profiler_malloc_hook;
  __free_hook = jmem_profiler_free_hook;

  JERRY_UNUSED(caller);
  return result;
#else
  JERRY_UNUSED(caller);
  return malloc(size);
#endif
}

static void (*old_free_hook)(void *ptr, const void *caller) {
#if defined(PROF_SIZE)
  // Restore old hooks
  __malloc_hook = old_malloc_hook;
  __free_hook = old_free_hook;

  // Inner call of free
  free(ptr);

  if (ht_contains(&g_heap_objects_ht, &ptr)) {
    size_t size = *(size_t *)ht_lookup(&g_heap_objects_ht, (void *)&ptr);
    ht_erase(&g_heap_objects_ht, (void *)&ptr);
    JERRY_CONTEXT(jmem_total_heap_size) -= size;
  }

  // Restore our hooks
  old_malloc_hook = __malloc_hook;
  old_free_hook = __free_hook;
  __malloc_hook = jmem_profiler_malloc_hook;
  __free_hook = jmem_profiler_free_hook;

  JERRY_UNUSED(caller);
#else
  free(ptr);
  JERRY_UNUSED(caller);
#endif
}

/* Total size profiling */
inline void __attr_always_inline___ init_size_profiler(void) {
#if defined(JMEM_PROFILE)
  CHECK_LOGGING_ENABLED();
#if defined(PROF_SIZE)

  FILE *fp1 = fopen(PROF_TOTAL_SIZE_FILENAME, "w");
  fprintf(fp1,
          "Timestamp (s), Blocks Size (B), Full-bitwidth Pointer Overhead (B), "
          "Allocated Heap Size (B), System Allocator Metadata Size (B), "
          "Segment Metadata Size (B), Snapshot Size (B)\n");
  fflush(fp1);
  fclose(fp1);
#if defined(PROF_SIZE__PERIOD_USEC)
  JERRY_CONTEXT(jsuptime_recent_total_size_print).tv_sec = 0;
  JERRY_CONTEXT(jsuptime_recent_total_size_print).tv_usec = 0;
#endif /* defined(PROF_SIZE__PERIOD_USEC) */
#endif
#endif
}

inline void __attr_always_inline___ print_total_size_profile_on_alloc(void) {
#if defined(PROF_SIZE)
#if defined(PROF_SIZE__PERIOD_USEC)
  CHECK_LOGGING_ENABLED();
  struct timeval js_uptime;
  get_js_uptime(&js_uptime);
  long timeval_diff_in_usec =
      get_timeval_diff_usec(&JERRY_CONTEXT(jsuptime_recent_total_size_print),
                            &js_uptime);
  if (timeval_diff_in_usec > PROF_SIZE__PERIOD_USEC) {
    JERRY_CONTEXT(jsuptime_recent_total_size_print) = js_uptime;
    __print_total_size_profile();
#if defined(PROF_COUNT__SIZE_DETAILED)
    print_count_profile();
#endif
  }
#else
  __print_total_size_profile();
#endif /* defined(PROF_SIZE__PERIOD_USEC) */
#endif /* defined(PROF_SIZE) */
}

inline void __attr_always_inline___ print_total_size_profile_finally(void) {
#if defined(PROF_SIZE)
  CHECK_LOGGING_ENABLED();
  __print_total_size_profile();
#endif /* defined(PROF_SIZE) */
}

inline void __attr_always_inline___ __print_total_size_profile(void) {
#if defined(PROF_SIZE)
  CHECK_LOGGING_ENABLED();
  FILE *fp = fopen(PROF_TOTAL_SIZE_FILENAME, "a");

  struct timeval js_uptime;
  get_js_uptime(&js_uptime);
  uint32_t blocks_size = (uint32_t)JERRY_CONTEXT(jmem_heap_blocks_size);
  uint32_t full_bw_oh =
      (uint32_t)JERRY_CONTEXT(jmem_full_bitwidth_pointer_overhead);
  uint32_t alloc_heap_size = (uint32_t)JERRY_CONTEXT(jmem_allocated_heap_size);
  uint32_t sysalloc_meta_size =
      (uint32_t)JERRY_CONTEXT(jmem_system_allocator_metadata_size);
  uint32_t segment_meta_size =
      (uint32_t)JERRY_CONTEXT(jmem_segment_allocator_metadata_size);
  uint32_t snapshot_size = (uint32_t)JERRY_CONTEXT(jmem_snapshot_size);

  fprintf(fp, "%lu.%06lu, %lu, %lu, %lu, %lu, %lu, %lu\n", js_uptime.tv_sec,
          js_uptime.tv_usec, blocks_size, full_bw_oh, alloc_heap_size,
          sysalloc_meta_size, segment_meta_size, snapshot_size);
  fflush(fp);
  fclose(fp);
#endif /* defined(PROF_SIZE) */
}