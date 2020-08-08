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

static size_t* extern_heap_size_ptr = NULL;

extern void set_extern_heap_size_ptr(size_t* size_ptr) {
  extern_heap_size_ptr = size_ptr;
}

#if !defined(PROF_MODE_ARTIK053) && defined(PROF_SIZE)
/* Prototypes for our hooks.  */
static void jmem_profiler_init_malloc_hook(void);
static void *jmem_profiler_malloc_hook(size_t, const void *);
static void jmem_profiler_free_hook(void *, const void *);
static void *jmem_profiler_realloc_hook(void *, size_t, const void *);

/* Variables to save original hooks. */
static void *(*old_malloc_hook)(size_t, const void *);
static void (*old_free_hook)(void *, const void *);
static void *(*old_realloc_hook)(void *, size_t, const void *);

/* Override initializing hook from the C library. */
void (*__MALLOC_HOOK_VOLATILE __malloc_initialize_hook)(void) =
    jmem_profiler_init_malloc_hook;

// Hash table to store heap objects: key=<void* ptr> value=<size_t size>
static HashTable g_heap_objects_ht;

static void jmem_profiler_init_malloc_hook(void) {
  ht_setup(&g_heap_objects_ht, sizeof(void *), sizeof(size_t), 10);
  ht_reserve(&g_heap_objects_ht, 100);

  old_malloc_hook = __malloc_hook;
  old_free_hook = __free_hook;
  old_realloc_hook = __realloc_hook;
  __malloc_hook = jmem_profiler_malloc_hook;
  __free_hook = jmem_profiler_free_hook;
  __realloc_hook = jmem_profiler_realloc_hook;
}

static void *jmem_profiler_malloc_hook(size_t size, const void *caller) {
  void *result;

  // Restore old hooks
  __malloc_hook = old_malloc_hook;
  __free_hook = old_free_hook;
  __realloc_hook = old_realloc_hook;

  // Inner call of malloc
  result = malloc(size);

  ht_insert(&g_heap_objects_ht, (void *)&result, (void *)&size);
  JERRY_CONTEXT(jmem_total_heap_size) += size;


  // Restore our hooks
  old_malloc_hook = __malloc_hook;
  old_free_hook = __free_hook;
  old_realloc_hook = __realloc_hook;
  __malloc_hook = jmem_profiler_malloc_hook;
  __free_hook = jmem_profiler_free_hook;
  __realloc_hook = jmem_profiler_realloc_hook;

  JERRY_UNUSED(caller);
  return result;
}

//static int free_error_count = 0;
//static int realloc_error_count = 0;

static void jmem_profiler_free_hook(void *ptr, const void *caller) {
  // Restore old hooks
  __malloc_hook = old_malloc_hook;
  __free_hook = old_free_hook;
  __realloc_hook = old_realloc_hook;

  // Inner call of free
  free(ptr);

  if (ht_contains(&g_heap_objects_ht, &ptr)) {
    size_t size = *(size_t *)ht_lookup(&g_heap_objects_ht, (void *)&ptr);
    ht_erase(&g_heap_objects_ht, (void *)&ptr);
    JERRY_CONTEXT(jmem_total_heap_size) -= size;
  } else {
    // printf("free error: %x\n", free_error_count++);
  }

  // Restore our hooks
  old_malloc_hook = __malloc_hook;
  old_free_hook = __free_hook;
  old_realloc_hook = __realloc_hook;
  __malloc_hook = jmem_profiler_malloc_hook;
  __free_hook = jmem_profiler_free_hook;
  __realloc_hook = jmem_profiler_realloc_hook;

  JERRY_UNUSED(caller);
}

static void *jmem_profiler_realloc_hook(void *ptr, size_t new_size,
                                        const void *caller) {
  void *result;

  // Restore old hooks
  __malloc_hook = old_malloc_hook;
  __free_hook = old_free_hook;
  __realloc_hook = old_realloc_hook;

  // Inner call of free
  result = realloc(ptr, new_size);

  if(ptr == NULL) {
    // Equivalent to malloc
    ht_insert(&g_heap_objects_ht, (void *)&result, (void *)&new_size);
    JERRY_CONTEXT(jmem_total_heap_size) += new_size;
  } else if(new_size == 0 && ptr != NULL) {
    // Equivalent to free
    if (ht_contains(&g_heap_objects_ht, &ptr)) {
      size_t old_size = *(size_t *)ht_lookup(&g_heap_objects_ht, (void *)&ptr);
      ht_erase(&g_heap_objects_ht, (void *)&ptr);
      JERRY_CONTEXT(jmem_total_heap_size) -= old_size;
    } else {
      //printf("realloc error 1: %d\n", realloc_error_count++);
    }
  } else {
    // Equivalent to realloc
    if (ht_contains(&g_heap_objects_ht, &ptr)) {
      size_t old_size = *(size_t *)ht_lookup(&g_heap_objects_ht, (void *)&ptr);
      ht_erase(&g_heap_objects_ht, (void *)&ptr);
      JERRY_CONTEXT(jmem_total_heap_size) -= old_size;

      ht_insert(&g_heap_objects_ht, (void *)&result, (void *)&new_size);
      JERRY_CONTEXT(jmem_total_heap_size) += new_size;
    } else {
      //printf("realloc error 2: %d\n", realloc_error_count++);
    }
  }

  // Restore our hooks
  old_malloc_hook = __malloc_hook;
  old_free_hook = __free_hook;
  old_realloc_hook = __realloc_hook;
  __malloc_hook = jmem_profiler_malloc_hook;
  __free_hook = jmem_profiler_free_hook;
  __realloc_hook = jmem_profiler_realloc_hook;

  JERRY_UNUSED(caller);
  return result;
}
#endif /* defined(PROF_MODE_ARTIK053) && defined(PROF_SIZE) */

/* Total size profiling */
inline void __attr_always_inline___ init_size_profiler(void) {
#if defined(JMEM_PROFILE)
  CHECK_LOGGING_ENABLED();
#if defined(PROF_SIZE)

  FILE *fp1 = fopen(PROF_TOTAL_SIZE_FILENAME, "w");
  fprintf(fp1,
          "Timestamp (s), Blocks Size (B), Full-bitwidth Pointer Overhead (B), "
          "Allocated Heap Size (B), System Allocator Metadata Size (B), "
          "Segment Metadata Size (B), Snapshot Size (B), Total Heap Size (B), "
          "GC Threshold (B), GC Count\n");
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
  uint32_t gc_threshold = (uint32_t)JERRY_CONTEXT(jmem_heap_limit);
  uint32_t gc_count = (uint32_t)JERRY_CONTEXT(jmem_size_profiler_gc_count);
  uint32_t total_heap_size = (uint32_t)JERRY_CONTEXT(jmem_total_heap_size);
  if(extern_heap_size_ptr != NULL) {
    // Tizen RT
    total_heap_size = alloc_heap_size + (uint32_t)*extern_heap_size_ptr;
  }

  fprintf(fp, "%lu.%06lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu\n",
          js_uptime.tv_sec, js_uptime.tv_usec, blocks_size, full_bw_oh,
          alloc_heap_size, sysalloc_meta_size, segment_meta_size, snapshot_size,
          total_heap_size, gc_threshold, gc_count);
  fflush(fp);
  fclose(fp);
#endif /* defined(PROF_SIZE) */
}
