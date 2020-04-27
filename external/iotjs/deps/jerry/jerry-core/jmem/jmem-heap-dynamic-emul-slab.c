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

#include "jmem-heap-dynamic-emul-slab.h"
#include "jcontext.h"
#include "jmem-profiler.h"

#if defined(JMEM_DYNAMIC_HEAP_EMUL) && defined(DE_SLAB)

void *alloc_a_block_from_slab(size_t size) {
  // Search a slab
  unsigned char slab_index;
  bool is_slab_found = false;
  for (slab_index = 0; slab_index < DE_MAX_NUM_SLABS; slab_index++) {
    bool is_allocated = JERRY_CONTEXT(is_slab_allocated[slab_index]);
    int num_allocated_slots = JERRY_CONTEXT(num_allocated_slots[slab_index]);
    if (!is_allocated) {
      continue;
    }
    if (num_allocated_slots + 1 < DE_NUM_SLOTS_PER_SLAB) {
      is_slab_found = true;
      break;
    }
  }
  // Allocate a slab if proper slab is not found
  if (!is_slab_found) {
    slab_index = alloc_a_slab_segment();
  }

  // Allocate a real block from static heap (because it is just an emulation)
  void *block_address = jmem_heap_alloc_block_small_object(size);

  // Allocate slots to the slab segment: update slab metadata
  jmem_cpointer_t block_cp = jmem_compress_pointer(block_address);
  JERRY_CONTEXT(cp2si_rmap[block_cp]) = slab_index;    // Update rmap
  JERRY_CONTEXT(num_allocated_slots[slab_index]) += 1; // Update size

  return block_address;
}

void free_a_block_from_slab(void *block_address, size_t size) {
  // Free a real block from static heap (because it is just an emulation)
  jmem_heap_free_block_small_object(block_address, size);

  // Free slots from the slab segment: update slab metadata
  jmem_cpointer_t block_cp = jmem_compress_pointer(block_address);
  unsigned char slab_index = JERRY_CONTEXT(cp2si_rmap[block_cp]);

  JERRY_CONTEXT(num_allocated_slots[slab_index]) -= 1; // Update size
  // rmap needs not to be updated

  // If the slab segment becomes empty, free the slab.
  if (JERRY_CONTEXT(num_allocated_slots[slab_index]) == 0) {
    free_a_slab_segment(slab_index);
  }
  // printf("Free: slab #%d = %d\n", slab_index,
  // JERRY_CONTEXT(num_allocated_slots[slab_index]));
}

unsigned char alloc_a_slab_segment(void) {
  // Get a slab index for the new slab
  unsigned char slab_index;
  bool is_slab_index_found = false;
  for (slab_index = 0; slab_index < DE_MAX_NUM_SLABS; slab_index++) {
    bool is_allocated = JERRY_CONTEXT(is_slab_allocated[slab_index]);
    if (!is_allocated) {
      is_slab_index_found = true;
      break;
    }
  }
  JERRY_ASSERT(is_slab_index_found);

  // Call GC if there is no space for a new slab segment
  size_t allocated_size =
      JERRY_CONTEXT(jmem_allocated_heap_size) + DE_SLAB_SEGMENT_SIZE;
  if (allocated_size > JMEM_HEAP_SIZE) {
    printf("GC on slab segment alloc\n");
    print_segment_utiliaztion_profile_before_gc(
        DE_SLAB_SEGMENT_SIZE); /* Segment utilization profiling */
    jmem_run_free_unused_memory_callbacks(JMEM_FREE_UNUSED_MEMORY_SEVERITY_LOW);
    print_segment_utiliaztion_profile_after_gc(
        DE_SLAB_SEGMENT_SIZE); /* Segment utilization profiling */
  }

  // Update slab metadata
  JERRY_CONTEXT(is_slab_allocated[slab_index]) = true;
  JERRY_CONTEXT(num_allocated_slabs)++;

  // Update allocated heap size, system allocator metadata size
  JERRY_CONTEXT(jmem_allocated_heap_size) += DE_SLAB_SEGMENT_SIZE;
  JERRY_CONTEXT(jmem_system_allocator_metadata_size) +=
      SYSTEM_ALLOCATOR_METADATA_SIZE;

  // Return the slab index of the allocated slab
  return slab_index;
}

void free_a_slab_segment(unsigned char slab_index) {
  // Update slab metadata
  JERRY_CONTEXT(is_slab_allocated[slab_index]) = false;
  JERRY_CONTEXT(num_allocated_slabs)--;

  // Update allocated heap size, system allocator metadata size
  JERRY_CONTEXT(jmem_allocated_heap_size) -= DE_SLAB_SEGMENT_SIZE;
  JERRY_CONTEXT(jmem_system_allocator_metadata_size) -=
      SYSTEM_ALLOCATOR_METADATA_SIZE;
}
#endif /* defined(JMEM_DYNAMIC_HEAP_EMUL) && \
          defined(DE_SLAB) */