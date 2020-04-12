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

#ifndef JMEM_HEAP_DYNAMIC_HEAP_EMUL_SLAB_H
#define JMEM_HEAP_DYNAMIC_HEAP_EMUL_SLAB_H

#include "jmem-config.h"
#include "jcontext.h"

#if defined(JMEM_DYNAMIC_HEAP_EMUL) && defined(DE_SLAB)
extern void *alloc_a_block_from_slab(size_t size);
extern void free_a_block_from_slab(void *block_address, size_t size);
extern unsigned char alloc_a_slab_segment(void);
extern void free_a_slab_segment(unsigned char slab_index);

#endif /* defined(JMEM_DYNAMIC_HEAP_EMUL) && \
          defined(DE_SLAB) */

#endif /* !defined(JMEM_HEAP_DYNAMIC_HEAP_EMUL_SLAB_H) */