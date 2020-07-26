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

#ifndef JMEM_H
#define JMEM_H

#include "jrt.h"
#include "jmem-config.h"
#include "jmem-profiler.h"

/** \addtogroup mem Memory allocation
 * @{
 *
 * \addtogroup heap Heap
 * @{
 */

#ifndef JERRY_ENABLE_EXTERNAL_CONTEXT
/**
 * Size of heap
 */
#define JMEM_HEAP_SIZE ((size_t) (CONFIG_MEM_HEAP_AREA_SIZE))
#endif /* !JERRY_ENABLE_EXTERNAL_CONTEXT */

/* Segmented heap allocator */
#define SEG_NUM_SEGMENTS (JMEM_HEAP_SIZE / SEG_SEGMENT_SIZE)
#define SEG_METADATA_SIZE_PER_SEGMENT (32)

/* Dynamic heap with slab */
#define DE_SLAB_SLOT_SIZE (16)
#define DE_NUM_SLOTS_PER_SLAB (512)
#define DE_SLAB_SEGMENT_SIZE (DE_SLAB_SLOT_SIZE * DE_NUM_SLOTS_PER_SLAB)
#define DE_MAX_NUM_SLABS (JMEM_HEAP_SIZE / DE_SLAB_SEGMENT_SIZE)

/* Calculating system allocator's effect */
#define SYSTEM_ALLOCATOR_METADATA_SIZE 8
#define SYSTEM_ALLOCATOR_ALIGN_BYTES 8

#ifdef JMEM_SEGMENTED_HEAP
/**
 * Segment information
 */
typedef struct {
  // number of segments in segment group (valid only in leading segment of the group)
  uint32_t group_num_segments;

  // Occupied size of segment
  size_t occupied_size;

} jmem_segment_t;
#endif /* defined(JMEM_SEGMENTED_HEAP) */
/*******************************************************/

/**
 * Logarithm of required alignment for allocated units/blocks
 */
#define JMEM_ALIGNMENT_LOG   3

/**
 * Representation of NULL value for compressed pointers
 */
#define JMEM_CP_NULL ((jmem_cpointer_t) 0)

/**
 * Required alignment for allocated units/blocks
 */
#define JMEM_ALIGNMENT (1u << JMEM_ALIGNMENT_LOG)

/**
 * Compressed pointer representations
 *
 * 16 bit representation:
 *   The jmem_cpointer_t is defined as uint16_t
 *   and it can contain any sixteen bit value.
 *
 * 32 bit representation:
 *   The jmem_cpointer_t is defined as uint32_t.
 *   The lower JMEM_ALIGNMENT_LOG bits must be zero.
 *   The other bits can have any value.
 *
 * The 16 bit representation always encodes an offset from
 * a heap base. The 32 bit representation currently encodes
 * raw 32 bit JMEM_ALIGNMENT aligned pointers on 32 bit systems.
 * This can be extended to encode a 32 bit offset from a heap
 * base on 64 bit systems in the future. There are no plans
 * to support more than 4G address space for JerryScript.
 */

/**
 * Compressed pointer
 */
#ifdef JERRY_CPOINTER_32_BIT
typedef uint32_t jmem_cpointer_t;
#else /* !JERRY_CPOINTER_32_BIT */
typedef uint16_t jmem_cpointer_t;
#endif /* JERRY_CPOINTER_32_BIT */

/**
 * Severity of a 'try give memory back' request
 *
 * The request are posted sequentially beginning from
 * low to high until enough memory is freed.
 *
 * If not enough memory is freed upon a high request
 * then the engine is shut down with ERR_OUT_OF_MEMORY.
 */
typedef enum
{
  JMEM_FREE_UNUSED_MEMORY_SEVERITY_LOW,  /**< 'low' severity */
  JMEM_FREE_UNUSED_MEMORY_SEVERITY_HIGH, /**< 'high' severity */
} jmem_free_unused_memory_severity_t;

/**
 * Node for free chunk list
 */
typedef struct jmem_pools_chunk_t
{
  struct jmem_pools_chunk_t *next_p; /**< pointer to next pool chunk */
} jmem_pools_chunk_t;

/**
 *  Free region node
 */
typedef struct
{
  uint32_t next_offset; /**< Offset of next region in list */
  uint32_t size; /**< Size of region */
} jmem_heap_free_t;

void jmem_init (void);
void jmem_finalize (void);

void *jmem_heap_alloc_block (const size_t size);
void *jmem_heap_alloc_block_null_on_error (const size_t size);
void jmem_heap_free_block (void *ptr, const size_t size);

void *jmem_heap_alloc_block_small_object (const size_t size);
void jmem_heap_free_block_small_object (void *ptr, const size_t size);

/* Modification for unifying segmented heap allocator */
/**
 * End of list marker.
 */
#define JMEM_HEAP_END_OF_LIST ((uint32_t) JMEM_HEAP_SIZE)
#define JMEM_HEAP_END_OF_LIST_UINT32 ((uint32_t) JMEM_HEAP_SIZE)

// #define JMEM_HEAP_END_OF_LIST ((uint32_t) 0xffffffff)
// #define JMEM_HEAP_END_OF_LIST_UINT32 ((uint32_t) 0xffffffff)

#ifdef ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY
/* In this case we simply store the pointer, since it fits anyway. */
/* REDC: do not consider this case */
#define JMEM_COMPRESS_POINTER_INTERNAL(p) ((uint32_t) (p))
#define JMEM_DECOMPRESS_POINTER_INTERNAL(u) ((jmem_heap_free_t *) (u))

#else /* defined(ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY) */
#ifdef JMEM_SEGMENTED_HEAP
/* jmem-heap-segmented-cptl.c */
extern uint32_t cptl_compress_pointer_internal(jmem_heap_free_t *p);
extern jmem_heap_free_t *cptl_decompress_pointer_internal(uint32_t u);
#define JMEM_COMPRESS_POINTER_INTERNAL(p) cptl_compress_pointer_internal(p)
#define JMEM_DECOMPRESS_POINTER_INTERNAL(u) cptl_decompress_pointer_internal(u)
#else
/* jmem-heap.c */
extern uint32_t static_compress_pointer_internal(jmem_heap_free_t *p);
extern jmem_heap_free_t *static_decompress_pointer_internal(uint32_t u);
#define JMEM_COMPRESS_POINTER_INTERNAL(p) static_compress_pointer_internal(p)
#define JMEM_DECOMPRESS_POINTER_INTERNAL(u) static_decompress_pointer_internal(u)
#endif
#endif /* !defined(ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY) */
/*******************************************************/

#ifdef JMEM_STATS
/**
 * Heap memory usage statistics
 */
typedef struct
{
  size_t size; /**< heap total size */

  size_t allocated_bytes; /**< currently allocated bytes */
  size_t peak_allocated_bytes; /**< peak allocated bytes */

  size_t waste_bytes; /**< bytes waste due to blocks filled partially */
  size_t peak_waste_bytes; /**< peak wasted bytes */

  size_t byte_code_bytes; /**< allocated memory for byte code */
  size_t peak_byte_code_bytes; /**< peak allocated memory for byte code */

  size_t string_bytes; /**< allocated memory for strings */
  size_t peak_string_bytes; /**< peak allocated memory for strings */

  size_t object_bytes; /**< allocated memory for objects */
  size_t peak_object_bytes; /**< peak allocated memory for objects */

  size_t property_bytes; /**< allocated memory for properties */
  size_t peak_property_bytes; /**< peak allocated memory for properties */

  size_t skip_count; /**< Number of skip-aheads during insertion of free block */
  size_t nonskip_count; /**< Number of times we could not skip ahead during
                         *   free block insertion */

  size_t alloc_count; /**< number of memory allocations */
  size_t free_count; /**< number of memory frees */
  size_t alloc_iter_count; /**< Number of iterations required for allocations */
  size_t free_iter_count; /**< Number of iterations required for inserting free blocks */
} jmem_heap_stats_t;

void jmem_stats_print (void);
void jmem_stats_allocate_byte_code_bytes (size_t property_size);
void jmem_stats_free_byte_code_bytes (size_t property_size);
void jmem_stats_allocate_string_bytes (size_t string_size);
void jmem_stats_free_string_bytes (size_t string_size);
void jmem_stats_allocate_object_bytes (size_t object_size);
void jmem_stats_free_object_bytes (size_t string_size);
void jmem_stats_allocate_property_bytes (size_t property_size);
void jmem_stats_free_property_bytes (size_t property_size);

void jmem_heap_get_stats (jmem_heap_stats_t *);
#endif /* JMEM_STATS */

jmem_cpointer_t jmem_compress_pointer (const void *pointer_p) __attr_pure___;
void *jmem_decompress_pointer (uintptr_t compressed_pointer) __attr_pure___;

/**
 * A free memory callback routine type.
 */
typedef void (*jmem_free_unused_memory_callback_t) (jmem_free_unused_memory_severity_t);

void jmem_register_free_unused_memory_callback (jmem_free_unused_memory_callback_t callback);
void jmem_unregister_free_unused_memory_callback (jmem_free_unused_memory_callback_t callback);

void jmem_run_free_unused_memory_callbacks (jmem_free_unused_memory_severity_t severity);

/**
 * Define a local array variable and allocate memory for the array on the heap.
 *
 * If requested number of elements is zero, assign NULL to the variable.
 *
 * Warning:
 *         if there is not enough memory on the heap, shutdown engine with ERR_OUT_OF_MEMORY.
 */
#define JMEM_DEFINE_LOCAL_ARRAY(var_name, number, type) \
{ \
  size_t var_name ## ___size = (size_t) (number) * sizeof (type); \
  type *var_name = (type *) (jmem_heap_alloc_block (var_name ## ___size));

/**
 * Free the previously defined local array variable, freeing corresponding block on the heap,
 * if it was allocated (i.e. if the array's size was non-zero).
 */
#define JMEM_FINALIZE_LOCAL_ARRAY(var_name) \
  if (var_name != NULL) \
  { \
    JERRY_ASSERT (var_name ## ___size != 0); \
    \
    jmem_heap_free_block (var_name, var_name ## ___size); \
  } \
  else \
  { \
    JERRY_ASSERT (var_name ## ___size == 0); \
  } \
}

/**
 * Get value of pointer from specified non-null compressed pointer value
 */
#define JMEM_CP_GET_NON_NULL_POINTER(type, cp_value) \
  ((type *) (jmem_decompress_pointer (cp_value)))

/**
 * Get value of pointer from specified compressed pointer value
 */
#define JMEM_CP_GET_POINTER(type, cp_value) \
  (((unlikely ((cp_value) == JMEM_CP_NULL)) ? NULL : JMEM_CP_GET_NON_NULL_POINTER (type, cp_value)))

/**
 * Set value of non-null compressed pointer so that it will correspond
 * to specified non_compressed_pointer
 */
#ifdef PROF_COUNT__COMPRESSION_CALLERS
#define JMEM_CP_SET_NON_NULL_POINTER(cp_value, non_compressed_pointer) \
  profile_inc_count_compression_callers(2); /* compression callers */ \
  (cp_value) = jmem_compress_pointer (non_compressed_pointer)
#else
#define JMEM_CP_SET_NON_NULL_POINTER(cp_value, non_compressed_pointer) \
  (cp_value) = jmem_compress_pointer (non_compressed_pointer)
#endif

/**
 * Set value of compressed pointer so that it will correspond
 * to specified non_compressed_pointer
 */
#ifdef PROF_COUNT__COMPRESSION_CALLERS
#define JMEM_CP_SET_POINTER(cp_value, non_compressed_pointer) \
  do \
  { \
    void *ptr_value = (void *) non_compressed_pointer; \
    \
    if (unlikely ((ptr_value) == NULL)) \
    { \
      (cp_value) = JMEM_CP_NULL; \
    } \
    else \
    { \
      profile_inc_count_compression_callers(5); /* compression callers */ \
      JMEM_CP_SET_NON_NULL_POINTER (cp_value, ptr_value); \
    } \
  } while (false);
#else
#define JMEM_CP_SET_POINTER(cp_value, non_compressed_pointer) \
  do \
  { \
    void *ptr_value = (void *) non_compressed_pointer; \
    \
    if (unlikely ((ptr_value) == NULL)) \
    { \
      (cp_value) = JMEM_CP_NULL; \
    } \
    else \
    { \
      JMEM_CP_SET_NON_NULL_POINTER (cp_value, ptr_value); \
    } \
  } while (false);
#endif
/**
 * @}
 * \addtogroup poolman Memory pool manager
 * @{
 */

void *jmem_pools_alloc (size_t size);
void jmem_pools_free (void *chunk_p, size_t size);

void add_full_bitwidth_size(size_t full_bw_size);
void sub_full_bitwidth_size(size_t full_bw_size);

/**
 * @}
 * @}
 */

#endif /* !JMEM_H */
