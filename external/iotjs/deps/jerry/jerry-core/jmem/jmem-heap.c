/* Copyright JS Foundation and other contributors, http://js.foundation
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

/**
 * Heap implementation
 */

#include "jcontext.h"
#include "jmem.h"
#include "jrt-bit-fields.h"
#include "jrt-libc-includes.h"

#include "jmem-heap-segmented.h"
#define JMEM_ALLOCATOR_INTERNAL
#include "jmem-allocator-internal.h"
#include "jmem-heap-dynamic-emul-slab.h"
#include "jmem-heap-profiler.h"
#include "jmem-jsobject-profiler.h"
#include "jmem-time-profiler.h"

/** \addtogroup mem Memory allocation
 * @{
 *
 * \addtogroup heap Heap
 * @{
 */

#ifndef JERRY_SYSTEM_ALLOCATOR
/**
 * Get end of region
 */
static inline jmem_heap_free_t *__attr_always_inline___ __attr_pure___
jmem_heap_get_region_end(jmem_heap_free_t *curr_p) /**< current region */
{
  return (jmem_heap_free_t *)((uint8_t *)curr_p + curr_p->size);
} /* jmem_heap_get_region_end */
#endif /* !JERRY_SYSTEM_ALLOCATOR */

#ifndef JERRY_ENABLE_EXTERNAL_CONTEXT
/**
 * Check size of heap is corresponding to configuration
 */
JERRY_STATIC_ASSERT(
    sizeof(jmem_heap_t) <= JMEM_HEAP_SIZE,
    size_of_mem_heap_must_be_less_than_or_equal_to_MEM_HEAP_SIZE);
#endif /* !JERRY_ENABLE_EXTERNAL_CONTEXT */

#ifdef JMEM_STATS

#ifdef JERRY_SYSTEM_ALLOCATOR
/* TODO: Implement mem-stat support for system allocator */
#error Memory statistics (JMEM_STATS) are not supported
#endif

static void jmem_heap_stat_init(void);
static void jmem_heap_stat_alloc(size_t num);
static void jmem_heap_stat_free(size_t num);
static void jmem_heap_stat_skip(void);
static void jmem_heap_stat_nonskip(void);
static void jmem_heap_stat_alloc_iter(void);
static void jmem_heap_stat_free_iter(void);

#define JMEM_HEAP_STAT_INIT() jmem_heap_stat_init()
#define JMEM_HEAP_STAT_ALLOC(v1) jmem_heap_stat_alloc(v1)
#define JMEM_HEAP_STAT_FREE(v1) jmem_heap_stat_free(v1)
#define JMEM_HEAP_STAT_SKIP() jmem_heap_stat_skip()
#define JMEM_HEAP_STAT_NONSKIP() jmem_heap_stat_nonskip()
#define JMEM_HEAP_STAT_ALLOC_ITER() jmem_heap_stat_alloc_iter()
#define JMEM_HEAP_STAT_FREE_ITER() jmem_heap_stat_free_iter()
#else /* !JMEM_STATS */
#define JMEM_HEAP_STAT_INIT()
#define JMEM_HEAP_STAT_ALLOC(v1)
#define JMEM_HEAP_STAT_FREE(v1)
#define JMEM_HEAP_STAT_SKIP()
#define JMEM_HEAP_STAT_NONSKIP()
#define JMEM_HEAP_STAT_ALLOC_ITER()
#define JMEM_HEAP_STAT_FREE_ITER()
#endif /* JMEM_STATS */

// Calculating system allocator's effect
#define SYSTEM_ALLOCATOR_METADATA_SIZE 8
#define SYSTEM_ALLOCATOR_ALIGN_BYTES 8

/**
 * Startup initialization of heap
 */
void jmem_heap_init(void) {
  /* Print allocator type */
#if defined(JERRY_SYSTEM_ALLOCATOR)
  printf("Dynamic allocation // JS heap area size: %dB\n", JMEM_HEAP_AREA_SIZE);
#elif defined(JMEM_SEGMENTED_HEAP)    /* JERRY_SYSTEM_ALLOCATOR */
  printf("Segmented allocation // Segment size: %dB * %d\n", SEG_SEGMENT_SIZE,
         SEG_NUM_SEGMENTS);
#elif defined(JMEM_DYNAMIC_HEAP_EMUL) /* JMEM_SEGMENTED_HEAP */
  printf("Emulated dynamic allocation // JS heap area size: %dB\n",
         JMEM_HEAP_AREA_SIZE);
#if defined(DE_SLAB)
  printf("Slab enabled\n");
#endif
#else /* JMEM_DYNAMIC_HEAP_EMUL */
  printf("Static allocation // JS heap area size: %dB\n", JMEM_HEAP_AREA_SIZE);
#endif

  /* Check validity of configs */
#ifndef JERRY_CPOINTER_32_BIT
  /* the maximum heap size for 16bit compressed pointers should be 512K */
  JERRY_ASSERT(((UINT16_MAX + 1) << JMEM_ALIGNMENT_LOG) >= JMEM_HEAP_SIZE);
#endif /* !JERRY_CPOINTER_32_BIT */

#ifndef JERRY_SYSTEM_ALLOCATOR
  JERRY_ASSERT((uintptr_t)JERRY_HEAP_CONTEXT(area) % JMEM_ALIGNMENT == 0);

  /* Initialize segmented heap */
#ifdef JMEM_SEGMENTED_HEAP
  jmem_segmented_init_segments();
#endif /* JMEM_SEGMENTED_HEAP */

  /* Initialize heap's common variables */
  JERRY_CONTEXT(jmem_heap_limit) = CONFIG_MEM_HEAP_DESIRED_LIMIT;

/* Initialize first free region */
#ifdef JMEM_SEGMENTED_HEAP
  jmem_heap_free_t *const region_p =
      (jmem_heap_free_t *)(JERRY_HEAP_CONTEXT(area[0]) + JMEM_ALIGNMENT);
  region_p->size = SEG_SEGMENT_SIZE - JMEM_ALIGNMENT;
#else
  jmem_heap_free_t *const region_p =
      (jmem_heap_free_t *)JERRY_HEAP_CONTEXT(area);
  region_p->size = JMEM_HEAP_AREA_SIZE;
#endif
  region_p->next_offset = JMEM_HEAP_END_OF_LIST;
  JERRY_HEAP_CONTEXT(first).size = 0;
  JERRY_HEAP_CONTEXT(first).next_offset =
      JMEM_HEAP_GET_OFFSET_FROM_ADDR(region_p);
  JERRY_CONTEXT(jmem_heap_list_skip_p) = &JERRY_HEAP_CONTEXT(first);

#endif /* !JERRY_SYSTEM_ALLOCATOR */

  /* Initialize profiling */
  profile_set_js_start_time(); /* Total size profiling */
  profile_init_times();        /* Time profiling */

  JMEM_HEAP_STAT_INIT();
} /* jmem_heap_init */

/**
 * Finalize heap
 */
void jmem_heap_finalize(void) {
  profile_print_times();                       /* Time profiling */
  profile_print_total_size_finally();          /* Total size profiling */
  profile_print_segment_utilization_finally(); /* Segment utilization profiling
                                                */
  profile_jsobject_print_allocation(); /* JS object allocation profiling */

#ifdef JMEM_SEGMENTED_HEAP
  free_empty_segments();
  free_first_empty_segment();

  JERRY_ASSERT(JERRY_HEAP_CONTEXT(segments_count) == 0);
#endif
  JERRY_ASSERT(JERRY_CONTEXT(jmem_heap_allocated_size) == 0);
} /* jmem_heap_finalize */

/**
 * Allocation of memory region.
 *
 * See also:
 *          jmem_heap_alloc_block
 *
 * @return pointer to allocated memory block - if allocation is successful,
 *         NULL - if there is not enough memory.
 */
static __attr_hot___ void *jmem_heap_alloc_block_internal(const size_t size,
                                                          bool is_no_aas) {
  profile_alloc_start(); /* Time profiling */

#ifndef JERRY_SYSTEM_ALLOCATOR
  /* Align size. */
  const size_t required_size =
      ((size + JMEM_ALIGNMENT - 1) / JMEM_ALIGNMENT) * JMEM_ALIGNMENT;

  /* Fast path for 8 byte chunks, first region is guaranteed to be sufficient.
   */
  jmem_heap_free_t *data_space_p = NULL;
  if (required_size == JMEM_ALIGNMENT &&
      likely(JERRY_HEAP_CONTEXT(first).next_offset != JMEM_HEAP_END_OF_LIST)) {
    data_space_p =
        JMEM_HEAP_GET_ADDR_FROM_OFFSET(JERRY_HEAP_CONTEXT(first).next_offset);
    JERRY_ASSERT(jmem_is_heap_pointer(data_space_p));


    JERRY_CONTEXT(jmem_heap_allocated_size) += JMEM_ALIGNMENT;
    if (!is_no_aas) {
#ifdef JMEM_DYNAMIC_HEAP_EMUL
      // Dynamic heap emulation
      JERRY_CONTEXT(jmem_heap_actually_allocated_size) +=
          JMEM_ALIGNMENT + SYSTEM_ALLOCATOR_METADATA_SIZE;
#else
      // Static heap or segmented heap
      JERRY_CONTEXT(jmem_heap_actually_allocated_size) += JMEM_ALIGNMENT;
#endif
    }
#ifdef JMEM_SEGMENTED_HEAP
    JERRY_HEAP_CONTEXT(
        segments[JERRY_HEAP_CONTEXT(first).next_offset / SEG_SEGMENT_SIZE])
        .occupied_size += JMEM_ALIGNMENT;
#endif
    JERRY_CONTEXT(jmem_heap_allocated_blocks_count)++;
    JMEM_HEAP_STAT_ALLOC_ITER();

    if (data_space_p->size == JMEM_ALIGNMENT) {
      JERRY_HEAP_CONTEXT(first).next_offset = data_space_p->next_offset;
    } else {
      JERRY_ASSERT(data_space_p->size > JMEM_ALIGNMENT);
      jmem_heap_free_t *remaining_p;
      remaining_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET(
                        JERRY_HEAP_CONTEXT(first).next_offset) +
                    1;
      remaining_p->size = data_space_p->size - JMEM_ALIGNMENT;
      remaining_p->next_offset = data_space_p->next_offset;
      JERRY_HEAP_CONTEXT(first).next_offset =
          JMEM_HEAP_GET_OFFSET_FROM_ADDR(remaining_p);
    }

    if (unlikely(data_space_p == JERRY_CONTEXT(jmem_heap_list_skip_p))) {
      JERRY_CONTEXT(jmem_heap_list_skip_p) =
          JMEM_HEAP_GET_ADDR_FROM_OFFSET(JERRY_HEAP_CONTEXT(first).next_offset);
    }
  }
  /* Slow path for larger regions. */
  else {
    uint32_t current_offset = JERRY_HEAP_CONTEXT(first).next_offset;
    jmem_heap_free_t *prev_p = &JERRY_HEAP_CONTEXT(first);

    while (current_offset != JMEM_HEAP_END_OF_LIST) {
      jmem_heap_free_t *current_p =
          JMEM_HEAP_GET_ADDR_FROM_OFFSET(current_offset);
      JERRY_ASSERT(jmem_is_heap_pointer(current_p));
      JMEM_HEAP_STAT_ALLOC_ITER();

      const uint32_t next_offset = current_p->next_offset;
#ifdef JMEM_SEGMENTED_HEAP
      jmem_heap_free_t *next_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET(next_offset);
      JERRY_ASSERT(next_offset == JMEM_HEAP_END_OF_LIST_UINT32 ||
                   jmem_is_heap_pointer(next_p));
#else
      JERRY_ASSERT(
          next_offset == JMEM_HEAP_END_OF_LIST ||
          jmem_is_heap_pointer(JMEM_HEAP_GET_ADDR_FROM_OFFSET(next_offset)));
#endif

      if (current_p->size >= required_size) {
        /* Region is sufficiently big, store address. */
        data_space_p = current_p;

        JERRY_CONTEXT(jmem_heap_allocated_size) += required_size;
        JERRY_CONTEXT(jmem_heap_allocated_blocks_count)++;
        if (!is_no_aas) {
#ifdef JMEM_DYNAMIC_HEAP_EMUL
          // Dynamic heap emulation
          JERRY_CONTEXT(jmem_heap_actually_allocated_size) +=
              required_size + SYSTEM_ALLOCATOR_METADATA_SIZE;
#else
          // Static heap or segmented heap
          JERRY_CONTEXT(jmem_heap_actually_allocated_size) += required_size;
#endif
        }

#ifdef JMEM_SEGMENTED_HEAP
        uint32_t start_segment = current_offset / SEG_SEGMENT_SIZE;
        uint32_t end_segment =
            (current_offset + (uint32_t)required_size - JMEM_ALIGNMENT) /
            SEG_SEGMENT_SIZE;
        if (start_segment == end_segment) {
          // Update metadata of single segment
          JERRY_HEAP_CONTEXT(segments[start_segment]).occupied_size +=
              required_size;
        } else {
          // Update metadata of double segment
          JERRY_ASSERT(start_segment + 1 == end_segment);
          uint32_t first_size =
              SEG_SEGMENT_SIZE - current_offset % SEG_SEGMENT_SIZE;
          JERRY_ASSERT(required_size > first_size);
          uint32_t following_size = (uint32_t)required_size - first_size;
          JERRY_HEAP_CONTEXT(segments[start_segment]).occupied_size +=
              first_size;
          JERRY_HEAP_CONTEXT(segments[end_segment]).occupied_size +=
              following_size;
        }
#endif

        /* Region was larger than necessary. */
        if (current_p->size > required_size) {
          /* Get address of remaining space. */
          jmem_heap_free_t *const remaining_p =
              (jmem_heap_free_t *)((uint8_t *)current_p + required_size);

          /* Update metadata. */
          remaining_p->size = current_p->size - (uint32_t)required_size;
          remaining_p->next_offset = next_offset;

          /* Update list. */
#ifdef JMEM_SEGMENTED_HEAP
          prev_p->next_offset = current_offset + (uint32_t)required_size;
#else
          prev_p->next_offset = JMEM_HEAP_GET_OFFSET_FROM_ADDR(remaining_p);
#endif
        }
        /* Block is an exact fit. */
        else {
          /* Remove the region from the list. */
          prev_p->next_offset = next_offset;
        }

        JERRY_CONTEXT(jmem_heap_list_skip_p) = prev_p;

        /* Found enough space. */
        break;
      }

      /* Next in list. */
      prev_p = current_p;
      current_offset = next_offset;
    }
  }

  while (JERRY_CONTEXT(jmem_heap_allocated_size) >=
         JERRY_CONTEXT(jmem_heap_limit)) {
    JERRY_CONTEXT(jmem_heap_limit) += CONFIG_MEM_HEAP_DESIRED_LIMIT;
  }

  if (unlikely(!data_space_p)) {
    profile_alloc_end(); /* Time profiling */
    return NULL;
  }

  JERRY_ASSERT((uintptr_t)data_space_p % JMEM_ALIGNMENT == 0);
  JMEM_HEAP_STAT_ALLOC(size);

  profile_alloc_end(); /* Time profiling */
  return (void *)data_space_p;
#else  /* JERRY_SYSTEM_ALLOCATOR */
  void *data_space_p = malloc(size);

  JERRY_CONTEXT(jmem_heap_allocated_size) += size;
  JERRY_CONTEXT(jmem_heap_allocated_blocks_count)++;

  // Dynamic heap
  size_t aligned_size = size + SYSTEM_ALLOCATOR_METADATA_SIZE;
  aligned_size = ((aligned_size + SYSTEM_ALLOCATOR_ALIGN_BYTES - 1) /
                  SYSTEM_ALLOCATOR_ALIGN_BYTES) *
                 SYSTEM_ALLOCATOR_ALIGN_BYTES;
  if (!is_no_aas) {
    JERRY_CONTEXT(jmem_heap_actually_allocated_size) += aligned_size;
  }

  profile_alloc_end(); /* Time profiling */
  return data_space_p;
#endif /* !JERRY_SYSTEM_ALLOCATOR */
} /* jmem_heap_alloc_block_internal */

/**
 * Allocation of memory block, running 'try to give memory back' callbacks, if
 * there is not enough memory.
 *
 * Note:
 *      if there is still not enough memory after running the callbacks
 *        - NULL value will be returned if parmeter 'ret_null_on_error' is true
 *        - the engine will terminate with ERR_OUT_OF_MEMORY if
 * 'ret_null_on_error' is false
 *
 * @return NULL, if the required memory size is 0
 *         also NULL, if 'ret_null_on_error' is true and the allocation fails
 * because of there is not enough memory
 */
static void *jmem_heap_gc_and_alloc_block(
    const size_t size,      /**< required memory size */
    bool ret_null_on_error, /**< indicates whether return null or terminate
                                 with ERR_OUT_OF_MEMORY on out of memory */
    bool is_no_aas)         /**< do not update on actually allocated size
                                 in order to */
{
#ifdef JMEM_SEGMENTED_HEAP
  // Disallow too large block bigger than two segments
  if (size >= SEG_SEGMENT_SIZE * 2) {
    printf("Requested size: %lu, but allowed maximum allocation size is: %lu\n",
           (uint32_t)size, (uint32_t)SEG_SEGMENT_SIZE * 2);
  }
  JERRY_ASSERT(size < SEG_SEGMENT_SIZE * 2);
#endif

  if (unlikely(size == 0)) {
    return NULL;
  }

#ifdef JMEM_GC_BEFORE_EACH_ALLOC
  // GC before each alloc: not enabled in most cases
  jmem_run_free_unused_memory_callbacks(JMEM_FREE_UNUSED_MEMORY_SEVERITY_HIGH);
#endif /* JMEM_GC_BEFORE_EACH_ALLOC */

#if defined(JMEM_DYNAMIC_HEAP_EMUL) || defined(JERRY_SYSTEM_ALLOCATOR)
  // Dynamic heap or dynamic heap emulation: add segment overhead to the max
  // size for the fair comparison
  size_t max_size = JMEM_HEAP_SIZE + SEG_NUM_SEGMENTS * 32;
#else
  // Static heap or segmented heap
  size_t max_size = JMEM_HEAP_SIZE;
#endif
  size_t allocated_size = JERRY_CONTEXT(jmem_heap_actually_allocated_size);
  if (!is_no_aas) {
    allocated_size += size;
  }
  if (allocated_size > max_size) {
    profile_print_segment_utilization_before_gc(
        size); /* Segment utilization profiling */
    jmem_run_free_unused_memory_callbacks(JMEM_FREE_UNUSED_MEMORY_SEVERITY_LOW);
    profile_print_segment_utilization_after_gc(
        size); /* Segment utilization profiling */
  }
  void *data_space_p =
      jmem_heap_alloc_block_internal(size, is_no_aas); // BLOCK ALLOC
  if (likely(data_space_p != NULL)) {
    profile_print_total_size_each_time(); /* Total size profiling */
#ifndef JERRY_SYSTEM_ALLOCATOR
    profile_jsobject_set_object_birth_time(
        jmem_compress_pointer(data_space_p)); /* JS object lifespan profiling */
#endif
    profile_jsobject_inc_allocation(size); /* JS object allocation profiling */
    return data_space_p;
  }
  // Segment Allocation before GC
#ifdef JMEM_SEGMENTED_HEAP
#ifdef JMEM_SEGMENTED_AGGRESSIVE_GC
  {
    profile_print_segment_utilization_before_gc(
        size); /* Segment utilization profiling */
    jmem_run_free_unused_memory_callbacks(JMEM_FREE_UNUSED_MEMORY_SEVERITY_LOW);
    profile_print_segment_utilization_after_gc(
        size); /* Segment utilization profiling */
    void *data_space_p2 =
        jmem_heap_alloc_block_internal(size, is_no_aas); // BLOCK ALLOC
    if (likely(data_space_p2 != NULL)) {
      profile_print_total_size_each_time(); /* Total size profiling */
      profile_jsobject_set_object_birth_time(jmem_compress_pointer(
          data_space_p2)); /* JS object lifespan profiling */
      profile_jsobject_inc_allocation(
          size); /* JS object allocation profiling */
      return data_space_p2;
    }
  }
#endif
#ifdef SEG_SEGALLOC_FIRST
  {
    /* Try one or two segments -> try to alloc a block */
    bool is_two_segs = false;
    if (size > SEG_SEGMENT_SIZE) {
      is_two_segs = true;
    }
    /* Segment utilization profiling */
    profile_print_segment_utilization_before_add_segment(size);
    if (jmem_heap_add_segment(is_two_segs) != NULL) {
      data_space_p =
          jmem_heap_alloc_block_internal(size, is_no_aas); // BLOCK ALLOC
      JERRY_ASSERT(data_space_p != NULL);
      profile_jsobject_set_object_birth_count(jmem_compress_pointer(
          data_space_p)); /* JS object lifespan profiling */
      return data_space_p;
    }
  }
#endif /* SEG_SEGALLOC_FIRST */
#endif /* JMEM_SEGMENTED_HEAP */
  for (jmem_free_unused_memory_severity_t severity =
           JMEM_FREE_UNUSED_MEMORY_SEVERITY_LOW;
       severity <= JMEM_FREE_UNUSED_MEMORY_SEVERITY_HIGH;
       severity = (jmem_free_unused_memory_severity_t)(severity + 1)) {
    /* Garbage collection -> try to alloc a block */
    profile_print_segment_utilization_before_gc(
        size); /* Segment utilization profiling */
    jmem_run_free_unused_memory_callbacks(severity);
    profile_print_segment_utilization_after_gc(
        size); /* Segment utilization profiling */
    data_space_p =
        jmem_heap_alloc_block_internal(size, is_no_aas); // BLOCK ALLOC
    if (likely(data_space_p != NULL)) {
      profile_print_total_size_each_time(); /* Total size profiling */
      profile_jsobject_set_object_birth_time(jmem_compress_pointer(
          data_space_p)); /* JS object lifespan profiling */
      profile_jsobject_inc_allocation(
          size); /* JS object allocation profiling */
      return data_space_p;
    }
  }
  // Segment allocation after GC
#ifdef JMEM_SEGMENTED_HEAP
  {
    bool is_two_segs = false;
    /* Segment utilization profiling */
    profile_print_segment_utilization_before_add_segment(size);
    if (jmem_heap_add_segment(is_two_segs) != NULL) {
      data_space_p =
          jmem_heap_alloc_block_internal(size, is_no_aas); // BLOCK ALLOC
      JERRY_ASSERT(data_space_p != NULL);
      profile_jsobject_set_object_birth_count(jmem_compress_pointer(
          data_space_p)); /* JS object lifespan profiling */
    }
    return data_space_p;
  }
#endif /* JMEM_SEGMENTED_HEAP */
  JERRY_ASSERT(data_space_p == NULL);

  if (!ret_null_on_error) {
    jerry_fatal(ERR_OUT_OF_MEMORY);
  }
  return data_space_p;
} /* jmem_heap_gc_and_alloc_block */

/**
 * Allocation of memory block, running 'try to give memory back' callbacks, if
 * there is not enough memory.
 *
 * Note:
 *      If there is still not enough memory after running the callbacks, then
 * the engine will be terminated with ERR_OUT_OF_MEMORY.
 *
 * @return NULL, if the required memory is 0
 *         pointer to allocated memory block, otherwise
 */
inline void *__attr_hot___ __attr_always_inline___
jmem_heap_alloc_block(const size_t size) /**< required memory size */
{
  return jmem_heap_gc_and_alloc_block(size, false, false);
} /* jmem_heap_alloc_block */

/**
 * Allocation of memory block, running 'try to give memory back' callbacks, if
 * there is not enough memory.
 *
 * Note:
 *      If there is still not enough memory after running the callbacks, NULL
 * will be returned.
 *
 * @return NULL, if the required memory size is 0
 *         also NULL, if the allocation has failed
 *         pointer to the allocated memory block, otherwise
 */
inline void *__attr_hot___ __attr_always_inline___
jmem_heap_alloc_block_null_on_error(
    const size_t size) /**< required memory size */
{
  return jmem_heap_gc_and_alloc_block(size, true, false);
} /* jmem_heap_alloc_block_null_on_error */

/**
 * Free the memory block.
 */
static void __attr_hot___ jmem_heap_free_block_internal(
    void *ptr,         /**< pointer to beginning of data space of the block */
    const size_t size, /**< size of allocated region */
    bool is_no_aas)    /**< do not update on actually allocated size
                            in order to emulate dynamic heap with slab */
{
#ifndef JERRY_SYSTEM_ALLOCATOR
  profile_free_start(); /* Time profiling */

  /* checking that ptr points to the heap */
  JERRY_ASSERT(jmem_is_heap_pointer(ptr));
  JERRY_ASSERT(size > 0);
  JERRY_ASSERT(JERRY_CONTEXT(jmem_heap_limit) >=
               JERRY_CONTEXT(jmem_heap_allocated_size));

  JMEM_HEAP_STAT_FREE_ITER();

  jmem_heap_free_t *block_p = (jmem_heap_free_t *)ptr;
  jmem_heap_free_t *prev_p;
  jmem_heap_free_t *next_p;

#ifdef JMEM_SEGMENTED_HEAP
  uint32_t boffset = JMEM_HEAP_GET_OFFSET_FROM_ADDR(block_p);
  uint32_t skip_offset =
      JMEM_HEAP_GET_OFFSET_FROM_ADDR(JERRY_CONTEXT(jmem_heap_list_skip_p));
  bool is_skip_ok = boffset > skip_offset;
#else  /* JMEM_SEGMENTED_HEAP */
  bool is_skip_ok = block_p > JERRY_CONTEXT(jmem_heap_list_skip_p);
#endif /* !JMEM_SEGMENTED_HEAP */
  if (is_skip_ok) {
    prev_p = JERRY_CONTEXT(jmem_heap_list_skip_p);
    JMEM_HEAP_STAT_SKIP();
  } else {
    prev_p = &JERRY_HEAP_CONTEXT(first);
    JMEM_HEAP_STAT_NONSKIP();
  }

  JERRY_ASSERT(jmem_is_heap_pointer(block_p));
#ifdef JMEM_SEGMENTED_HEAP
  const uint32_t block_offset = boffset;
#else  /* JMEM_SEGMENTED_HEAP */
  const uint32_t block_offset = JMEM_HEAP_GET_OFFSET_FROM_ADDR(block_p);
#endif /* !JMEM_SEGMENTED_HEAP */

  /* Find position of region in the list. */
  while (prev_p->next_offset < block_offset) {
    next_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET(prev_p->next_offset);
    JERRY_ASSERT(jmem_is_heap_pointer(next_p));

    prev_p = next_p;

    JMEM_HEAP_STAT_FREE_ITER();
  }

  next_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET(prev_p->next_offset);

  /* Realign size */
  const size_t aligned_size =
      (size + JMEM_ALIGNMENT - 1) / JMEM_ALIGNMENT * JMEM_ALIGNMENT;

  /* Update prev. */
  if (jmem_heap_get_region_end(prev_p) == block_p) {
    /* Can be merged. */
    prev_p->size += (uint32_t)aligned_size;
    block_p = prev_p;
  } else {
    block_p->size = (uint32_t)aligned_size;
    prev_p->next_offset = block_offset;
  }

  /* Update next. */
  if (jmem_heap_get_region_end(block_p) == next_p) {
    /* Can be merged. */
    block_p->size += next_p->size;
    block_p->next_offset = next_p->next_offset;
  } else {
    block_p->next_offset = JMEM_HEAP_GET_OFFSET_FROM_ADDR(next_p);
  }

  JERRY_CONTEXT(jmem_heap_list_skip_p) = prev_p;

#ifdef JMEM_SEGMENTED_HEAP
  uint32_t start_segment = block_offset / SEG_SEGMENT_SIZE;
  uint32_t end_segment =
      (block_offset + (uint32_t)aligned_size - JMEM_ALIGNMENT) /
      SEG_SEGMENT_SIZE;
  JERRY_ASSERT(JERRY_HEAP_CONTEXT(segments[start_segment]).occupied_size > 0);
  JERRY_ASSERT(JERRY_HEAP_CONTEXT(segments[end_segment]).occupied_size > 0);
  if (start_segment == end_segment) {
    // Update metadata of single segment
    JERRY_HEAP_CONTEXT(segments[start_segment]).occupied_size -= aligned_size;
  } else {
    // Update metadata of double segment
    JERRY_ASSERT(start_segment + 1 == end_segment);
    uint32_t first_size = SEG_SEGMENT_SIZE - block_offset % SEG_SEGMENT_SIZE;
    JERRY_ASSERT(aligned_size > first_size);
    uint32_t following_size = (uint32_t)aligned_size - first_size;
    JERRY_HEAP_CONTEXT(segments[start_segment]).occupied_size -= first_size;
    JERRY_HEAP_CONTEXT(segments[end_segment]).occupied_size -= following_size;
  }
#endif

  // Static heap or segmented heap
  JERRY_ASSERT(JERRY_CONTEXT(jmem_heap_allocated_size) > 0);
  JERRY_CONTEXT(jmem_heap_allocated_size) -= aligned_size;
  JERRY_CONTEXT(jmem_heap_allocated_blocks_count)--;

  if (!is_no_aas) {
#ifdef JMEM_DYNAMIC_HEAP_EMUL
    // Dynamic heap emulation
    JERRY_CONTEXT(jmem_heap_actually_allocated_size) -=
        aligned_size + SYSTEM_ALLOCATOR_METADATA_SIZE;
#else
    // Static heap or segmented heap
    JERRY_CONTEXT(jmem_heap_actually_allocated_size) -= aligned_size;
#endif
  }

  while (JERRY_CONTEXT(jmem_heap_allocated_size) +
             CONFIG_MEM_HEAP_DESIRED_LIMIT <=
         JERRY_CONTEXT(jmem_heap_limit)) {
    JERRY_CONTEXT(jmem_heap_limit) -= CONFIG_MEM_HEAP_DESIRED_LIMIT;
  }

  JERRY_ASSERT(JERRY_CONTEXT(jmem_heap_limit) >=
               JERRY_CONTEXT(jmem_heap_allocated_size));
  JMEM_HEAP_STAT_FREE(size);

  profile_print_total_size_each_time(); /* Total size profiling */
  profile_print_segment_utilization_after_free_block(size); /* Segment
                                                    utilization profiling */

  profile_jsobject_print_object_lifespan(
      jmem_compress_pointer(ptr)); /* JS object lifespan profiling */
  profile_free_end();              /* Time profiling */

#else  /* JERRY_SYSTEM_ALLOCATOR */
  JERRY_UNUSED(size);
  free(ptr);

  JERRY_CONTEXT(jmem_heap_allocated_size) -= size;
  JERRY_CONTEXT(jmem_heap_allocated_blocks_count)--;

  // Dynamic heap
  if (!is_no_aas) {
    size_t aligned_size = size + SYSTEM_ALLOCATOR_METADATA_SIZE;
    aligned_size = ((aligned_size + SYSTEM_ALLOCATOR_ALIGN_BYTES - 1) /
                    SYSTEM_ALLOCATOR_ALIGN_BYTES) *
                   SYSTEM_ALLOCATOR_ALIGN_BYTES;
    JERRY_CONTEXT(jmem_heap_actually_allocated_size) -= aligned_size;
  }

  profile_print_total_size_each_time(); /* Total size profiling */
  profile_free_end();                   /* Time profiling */
#endif /* !JERRY_SYSTEM_ALLOCATOR */
} /* jmem_heap_free_block_internal */

/**
 * Free the memory block.
 */
void __attr_hot___ jmem_heap_free_block(
    void *ptr,         /**< pointer to beginning of data space of the block */
    const size_t size) /**< size of allocated region */
{
  jmem_heap_free_block_internal(ptr, size, false);
} /* jmem_heap_free_block */

#if defined(JMEM_DYNAMIC_HEAP_EMUL) && defined(DE_SLAB)
inline void *__attr_hot___ __attr_always_inline___
jmem_heap_alloc_block_no_aas(const size_t size) {
  return jmem_heap_gc_and_alloc_block(size, false, true);
}
inline void __attr_hot___ __attr_always_inline___
jmem_heap_free_block_no_aas(void *ptr, const size_t size) {
  jmem_heap_free_block_internal(ptr, size, true);
}
#endif /* defined(JMEM_DYNAMIC_HEAP_EMUL) && \
          defined(DE_SLAB) */


#ifndef JERRY_NDEBUG
/**
 * Check whether the pointer points to the heap
 *
 * Note:
 *      the routine should be used only for assertion checks
 *
 * @return true - if pointer points to the heap,
 *         false - otherwise
 */
bool jmem_is_heap_pointer(const void *pointer) /**< pointer */
{
  bool is_heap_pointer;
#ifndef JERRY_SYSTEM_ALLOCATOR
#ifdef JMEM_SEGMENTED_HEAP
  /* Not yet implemented */
  is_heap_pointer = pointer != NULL;
#else  /* JMEM_SEGMENTED_HEAP */
  is_heap_pointer =
      ((uint8_t *)pointer >= JERRY_HEAP_CONTEXT(area) &&
       (uint8_t *)pointer <= (JERRY_HEAP_CONTEXT(area) + JMEM_HEAP_AREA_SIZE));
#endif /* !JMEM_SEGMENTED_HEAP */
#else  /* JERRY_SYSTEM_ALLOCATOR */
  JERRY_UNUSED(pointer);
  is_heap_pointer = true;
#endif /* !JERRY_SYSTEM_ALLOCATOR */
  return is_heap_pointer;
} /* jmem_is_heap_pointer */
#endif /* !JERRY_NDEBUG */

#ifdef JMEM_STATS
/**
 * Get heap memory usage statistics
 */
void jmem_heap_get_stats(
    jmem_heap_stats_t *out_heap_stats_p) /**< [out] heap stats */
{
  JERRY_ASSERT(out_heap_stats_p != NULL);

  *out_heap_stats_p = JERRY_CONTEXT(jmem_heap_stats);
} /* jmem_heap_get_stats */

/**
 * Print heap memory usage statistics
 */
void jmem_heap_stats_print(void) {
  jmem_heap_stats_t *heap_stats = &JERRY_CONTEXT(jmem_heap_stats);

  JERRY_DEBUG_MSG(
      "Heap stats:\n"
      "  Heap size = %zu bytes\n"
      "  Allocated = %zu bytes\n"
      "  Peak allocated = %zu bytes\n"
      "  Waste = %zu bytes\n"
      "  Peak waste = %zu bytes\n"
      "  Allocated byte code data = %zu bytes\n"
      "  Peak allocated byte code data = %zu bytes\n"
      "  Allocated string data = %zu bytes\n"
      "  Peak allocated string data = %zu bytes\n"
      "  Allocated object data = %zu bytes\n"
      "  Peak allocated object data = %zu bytes\n"
      "  Allocated property data = %zu bytes\n"
      "  Peak allocated property data = %zu bytes\n"
      "  Skip-ahead ratio = %zu.%04zu\n"
      "  Average alloc iteration = %zu.%04zu\n"
      "  Average free iteration = %zu.%04zu\n"
      "\n",
      heap_stats->size, heap_stats->allocated_bytes,
      heap_stats->peak_allocated_bytes, heap_stats->waste_bytes,
      heap_stats->peak_waste_bytes, heap_stats->byte_code_bytes,
      heap_stats->peak_byte_code_bytes, heap_stats->string_bytes,
      heap_stats->peak_string_bytes, heap_stats->object_bytes,
      heap_stats->peak_object_bytes, heap_stats->property_bytes,
      heap_stats->peak_property_bytes,
      heap_stats->skip_count / heap_stats->nonskip_count,
      heap_stats->skip_count % heap_stats->nonskip_count * 10000 /
          heap_stats->nonskip_count,
      heap_stats->alloc_iter_count / heap_stats->alloc_count,
      heap_stats->alloc_iter_count % heap_stats->alloc_count * 10000 /
          heap_stats->alloc_count,
      heap_stats->free_iter_count / heap_stats->free_count,
      heap_stats->free_iter_count % heap_stats->free_count * 10000 /
          heap_stats->free_count);
} /* jmem_heap_stats_print */

/**
 * Initalize heap memory usage statistics account structure
 */
static void jmem_heap_stat_init(void) {
  JERRY_CONTEXT(jmem_heap_stats).size = JMEM_HEAP_AREA_SIZE;
} /* jmem_heap_stat_init */

/**
 * Account allocation
 */
static void jmem_heap_stat_alloc(size_t size) /**< Size of allocated block */
{
  const size_t aligned_size =
      (size + JMEM_ALIGNMENT - 1) / JMEM_ALIGNMENT * JMEM_ALIGNMENT;
  const size_t waste_bytes = aligned_size - size;

  jmem_heap_stats_t *heap_stats = &JERRY_CONTEXT(jmem_heap_stats);

  heap_stats->allocated_bytes += aligned_size;
  heap_stats->waste_bytes += waste_bytes;
  heap_stats->alloc_count++;

  if (heap_stats->allocated_bytes > heap_stats->peak_allocated_bytes) {
    heap_stats->peak_allocated_bytes = heap_stats->allocated_bytes;
  }

  if (heap_stats->waste_bytes > heap_stats->peak_waste_bytes) {
    heap_stats->peak_waste_bytes = heap_stats->waste_bytes;
  }
} /* jmem_heap_stat_alloc */

/**
 * Account freeing
 */
static void jmem_heap_stat_free(size_t size) /**< Size of freed block */
{
  const size_t aligned_size =
      (size + JMEM_ALIGNMENT - 1) / JMEM_ALIGNMENT * JMEM_ALIGNMENT;
  const size_t waste_bytes = aligned_size - size;

  jmem_heap_stats_t *heap_stats = &JERRY_CONTEXT(jmem_heap_stats);

  heap_stats->free_count++;
  heap_stats->allocated_bytes -= aligned_size;
  heap_stats->waste_bytes -= waste_bytes;
} /* jmem_heap_stat_free */

/**
 * Counts number of skip-aheads during insertion of free block
 */
static void jmem_heap_stat_skip(void) {
  JERRY_CONTEXT(jmem_heap_stats).skip_count++;
} /* jmem_heap_stat_skip  */

/**
 * Counts number of times we could not skip ahead during free block insertion
 */
static void jmem_heap_stat_nonskip(void) {
  JERRY_CONTEXT(jmem_heap_stats).nonskip_count++;
} /* jmem_heap_stat_nonskip */

/**
 * Count number of iterations required for allocations
 */
static void jmem_heap_stat_alloc_iter(void) {
  JERRY_CONTEXT(jmem_heap_stats).alloc_iter_count++;
} /* jmem_heap_stat_alloc_iter */

/**
 * Counts number of iterations required for inserting free blocks
 */
static void jmem_heap_stat_free_iter(void) {
  JERRY_CONTEXT(jmem_heap_stats).free_iter_count++;
} /* jmem_heap_stat_free_iter */
#endif /* JMEM_STATS */

/**
 * @}
 * @}
 */
