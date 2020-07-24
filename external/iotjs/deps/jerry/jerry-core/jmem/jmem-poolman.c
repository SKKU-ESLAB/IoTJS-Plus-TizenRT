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
 * Memory pool manager implementation
 */

#include "jcontext.h"
#include "jmem.h"
#include "jrt-libc-includes.h"

#define JMEM_ALLOCATOR_INTERNAL
#include "jmem.h"
#include "jmem-allocator-internal.h"
#include "jmem-config.h"
#include "jmem-heap-dynamic-emul-slab.h"

/** \addtogroup mem Memory allocation
 * @{
 *
 * \addtogroup poolman Memory pool manager
 * @{
 */

/*
 * Valgrind-related options and headers
 */
#ifdef JERRY_VALGRIND
#include "memcheck.h"

#define VALGRIND_NOACCESS_SPACE(p, s) VALGRIND_MAKE_MEM_NOACCESS((p), (s))
#define VALGRIND_UNDEFINED_SPACE(p, s) VALGRIND_MAKE_MEM_UNDEFINED((p), (s))
#define VALGRIND_DEFINED_SPACE(p, s) VALGRIND_MAKE_MEM_DEFINED((p), (s))
#else /* !JERRY_VALGRIND */
#define VALGRIND_NOACCESS_SPACE(p, s)
#define VALGRIND_UNDEFINED_SPACE(p, s)
#define VALGRIND_DEFINED_SPACE(p, s)
#endif /* JERRY_VALGRIND */

#ifdef JERRY_VALGRIND_FREYA
#include "memcheck.h"

#define VALGRIND_FREYA_MALLOCLIKE_SPACE(p, s) \
  VALGRIND_MALLOCLIKE_BLOCK((p), (s), 0, 0)
#define VALGRIND_FREYA_FREELIKE_SPACE(p) VALGRIND_FREELIKE_BLOCK((p), 0)
#else /* !JERRY_VALGRIND_FREYA */
#define VALGRIND_FREYA_MALLOCLIKE_SPACE(p, s)
#define VALGRIND_FREYA_FREELIKE_SPACE(p)
#endif /* JERRY_VALGRIND_FREYA */

/**
 * Finalize pool manager
 */
void jmem_pools_finalize(void) {
  jmem_pools_collect_empty();

  JERRY_ASSERT(JERRY_CONTEXT(jmem_free_8_byte_chunk_p) == NULL);
#if defined(JERRY_CPOINTER_32_BIT) || defined(SEG_FULLBIT_ADDRESS_ALLOC)
  JERRY_ASSERT(JERRY_CONTEXT(jmem_free_16_byte_chunk_p) == NULL);
#endif /* defined(JERRY_CPOINTER_32_BIT) || defined(SEG_FULLBIT_ADDRESS_ALLOC) */
} /* jmem_pools_finalize */

static inline void *__attr_hot___ __attr_always_inline___
jmem_heap_alloc_block_for_pool(size_t size) {
  #if defined(JMEM_DYNAMIC_HEAP_EMUL) && defined(DE_SLAB)
    // Dynamic heap with slab
    JERRY_UNUSED(size);
    return alloc_a_block_from_slab(size);
  #else /* defined(JMEM_DYNAMIC_HEAP_EMUL) && defined(DE_SLAB) */
    // Others 
    return jmem_heap_alloc_block_small_object(size);
  #endif /* !(defined(JMEM_DYNAMIC_HEAP_EMUL) && defined(DE_SLAB)) */
}

static inline void __attr_hot___ __attr_always_inline___
jmem_heap_free_block_for_pool(void* ptr, size_t size) {
  #if defined(JMEM_DYNAMIC_HEAP_EMUL) && defined(DE_SLAB)
    // Dynamic heap with slab
    free_a_block_from_slab(ptr, size);
  #else /* defined(JMEM_DYNAMIC_HEAP_EMUL) && defined(DE_SLAB) */
    // Others
    jmem_heap_free_block_small_object(ptr, size);
  #endif /* !(defined(JMEM_DYNAMIC_HEAP_EMUL) && defined(DE_SLAB)) */
}

/**
 * Allocate a chunk of specified size
 *
 * @return pointer to allocated chunk, if allocation was successful,
 *         or NULL - if not enough memory.
 */
inline void *__attr_hot___ __attr_always_inline___
jmem_pools_alloc(size_t size) /**< size of the chunk */
{
#ifdef JMEM_GC_BEFORE_EACH_ALLOC
  jmem_run_free_unused_memory_callbacks(JMEM_FREE_UNUSED_MEMORY_SEVERITY_HIGH);
#endif /* JMEM_GC_BEFORE_EACH_ALLOC */

  if (size <= 8) {
    if (JERRY_CONTEXT(jmem_free_8_byte_chunk_p) != NULL) {
      const jmem_pools_chunk_t *const chunk_p =
          JERRY_CONTEXT(jmem_free_8_byte_chunk_p);

      VALGRIND_DEFINED_SPACE(chunk_p, sizeof(jmem_pools_chunk_t));

      JERRY_CONTEXT(jmem_free_8_byte_chunk_p) = chunk_p->next_p;

      VALGRIND_UNDEFINED_SPACE(chunk_p, sizeof(jmem_pools_chunk_t));

      return (void *)chunk_p;
    } else {
      return (void *)jmem_heap_alloc_block_for_pool(8);
    }
  }

#if defined(JERRY_CPOINTER_32_BIT) || defined(SEG_FULLBIT_ADDRESS_ALLOC)
  JERRY_ASSERT(size <= 16);

  if (JERRY_CONTEXT(jmem_free_16_byte_chunk_p) != NULL) {
    const jmem_pools_chunk_t *const chunk_p =
        JERRY_CONTEXT(jmem_free_16_byte_chunk_p);

    VALGRIND_DEFINED_SPACE(chunk_p, sizeof(jmem_pools_chunk_t));

    JERRY_CONTEXT(jmem_free_16_byte_chunk_p) = chunk_p->next_p;

    VALGRIND_UNDEFINED_SPACE(chunk_p, sizeof(jmem_pools_chunk_t));

    return (void *)chunk_p;
  } else {
    return (void *)jmem_heap_alloc_block_for_pool(16);
  }
#else /* !(defined(JERRY_CPOINTER_32_BIT) || defined(SEG_FULLBIT_ADDRESS_ALLOC)) */
  JERRY_UNREACHABLE();
  return NULL;
#endif
} /* jmem_pools_alloc */

/**
 * Free the chunk
 */
inline void __attr_hot___ __attr_always_inline___
jmem_pools_free(void *chunk_p, /**< pointer to the chunk */
                size_t size)   /**< size of the chunk */
{
  JERRY_ASSERT(chunk_p != NULL);

  jmem_pools_chunk_t *const chunk_to_free_p = (jmem_pools_chunk_t *)chunk_p;

  VALGRIND_DEFINED_SPACE(chunk_to_free_p, size);

  if (size <= 8) {
    chunk_to_free_p->next_p = JERRY_CONTEXT(jmem_free_8_byte_chunk_p);
    JERRY_CONTEXT(jmem_free_8_byte_chunk_p) = chunk_to_free_p;
  } else {
#if defined(JERRY_CPOINTER_32_BIT) || defined(SEG_FULLBIT_ADDRESS_ALLOC)
    JERRY_ASSERT(size <= 16);

    chunk_to_free_p->next_p = JERRY_CONTEXT(jmem_free_16_byte_chunk_p);
    JERRY_CONTEXT(jmem_free_16_byte_chunk_p) = chunk_to_free_p;
#else  /* !(defined(JERRY_CPOINTER_32_BIT) || defined(SEG_FULLBIT_ADDRESS_ALLOC)) */
    JERRY_UNREACHABLE();
#endif /* (defined(JERRY_CPOINTER_32_BIT) || defined(SEG_FULLBIT_ADDRESS_ALLOC)) */
  }

  VALGRIND_NOACCESS_SPACE(chunk_to_free_p, size);
} /* jmem_pools_free */

/**
 *  Collect empty pool chunks
 */
void jmem_pools_collect_empty(void) {
  jmem_pools_chunk_t *chunk_p = JERRY_CONTEXT(jmem_free_8_byte_chunk_p);
  JERRY_CONTEXT(jmem_free_8_byte_chunk_p) = NULL;

  while (chunk_p) {
    VALGRIND_DEFINED_SPACE(chunk_p, sizeof(jmem_pools_chunk_t));
    jmem_pools_chunk_t *const next_p = chunk_p->next_p;
    VALGRIND_NOACCESS_SPACE(chunk_p, sizeof(jmem_pools_chunk_t));

    jmem_heap_free_block_for_pool(chunk_p, 8);
    chunk_p = next_p;
  }

#if defined(JERRY_CPOINTER_32_BIT) || defined(SEG_FULLBIT_ADDRESS_ALLOC)
  chunk_p = JERRY_CONTEXT(jmem_free_16_byte_chunk_p);
  JERRY_CONTEXT(jmem_free_16_byte_chunk_p) = NULL;

  while (chunk_p) {
    VALGRIND_DEFINED_SPACE(chunk_p, sizeof(jmem_pools_chunk_t));
    jmem_pools_chunk_t *const next_p = chunk_p->next_p;
    VALGRIND_NOACCESS_SPACE(chunk_p, sizeof(jmem_pools_chunk_t));

    jmem_heap_free_block_for_pool(chunk_p, 16);
    chunk_p = next_p;
  }
#endif /* defined(JERRY_CPOINTER_32_BIT) || defined(SEG_FULLBIT_ADDRESS_ALLOC) */
} /* jmem_pools_collect_empty */

inline void __attr_always_inline___ add_full_bitwidth_size(size_t full_bw_size) {
  // Apply the number of cpointers to the actually allocated heap size
#if defined(JMEM_DYNAMIC_HEAP_EMUL) || defined(SEG_FULLBIT_ADDRESS_ALLOC)
  // Update additional heap blocks size
  JERRY_CONTEXT(jmem_full_bitwidth_pointer_overhead) += full_bw_size;
#if defined(JMEM_DYNAMIC_HEAP_EMUL) && !defined(DE_SLAB)
  JERRY_CONTEXT(jmem_allocated_heap_size) += full_bw_size;
#endif
#else
  JERRY_UNUSED(full_bw_size);
#endif
}

inline void __attr_always_inline___ sub_full_bitwidth_size(size_t full_bw_size) {
  // Apply the number of cpointers to the actually allocated heap size
#if defined(JMEM_DYNAMIC_HEAP_EMUL) || defined(SEG_FULLBIT_ADDRESS_ALLOC)
  // Update additional heap blocks size
  JERRY_CONTEXT(jmem_full_bitwidth_pointer_overhead) -= full_bw_size;
#if defined(JMEM_DYNAMIC_HEAP_EMUL) && !defined(DE_SLAB)
  JERRY_CONTEXT(jmem_allocated_heap_size) -= full_bw_size;
#endif
#else
  JERRY_UNUSED(full_bw_size);
#endif
}

#undef VALGRIND_NOACCESS_SPACE
#undef VALGRIND_UNDEFINED_SPACE
#undef VALGRIND_DEFINED_SPACE
#undef VALGRIND_FREYA_MALLOCLIKE_SPACE
#undef VALGRIND_FREYA_FREELIKE_SPACE

/**
 * @}
 * @}
 */
