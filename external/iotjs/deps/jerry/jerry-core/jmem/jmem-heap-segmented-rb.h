/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
 * Copyright 2016-2019 Gyeonghwan Hong, Eunsoo Park, Sungkyunkwan University
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

#ifndef JMEM_HEAP_SEGMENTED_RB_H
#define JMEM_HEAP_SEGMENTED_RB_H

#include "jmem-config.h"
#include "jrt.h"

#ifdef JMEM_SEGMENTED_HEAP
#ifdef JMEM_SEGMENT_RB_LOOKUP

#define container_of(ptr, type, member)                                        \
  ((type *)(((char *)(ptr)) - ((char *)(&((type *)0)->member))))

typedef struct _rb_node {
  unsigned long rb_parent_color;
#define RB_RED 0
#define RB_BLACK 1
  struct _rb_node *rb_right;
  struct _rb_node *rb_left;
} rb_node;

typedef struct {
  rb_node *rb_node;
} rb_root;

typedef struct _seg_node {
  rb_node node;
  uint8_t *base_addr;
  uint32_t seg_idx;
} seg_node_t;

extern seg_node_t *segment_node_lookup(rb_root *root, uint8_t *addr);
extern int segment_node_insert(rb_root *root, seg_node_t *node);
extern void segment_node_remove(rb_root *root, uint8_t *base_addr);

extern void rb_init_node(rb_node *rb);
extern void rb_set_parent(rb_node *rb, rb_node *p);
extern void rb_set_color(rb_node *rb, unsigned int color);
extern void rb_link_node(rb_node *node, rb_node *parent, rb_node **rb_link);

extern void rb_insert_color(rb_node *node, rb_root *root);
extern void __rb_erase_color(rb_node *node, rb_node *parent, rb_root *root);
extern void rb_erase(rb_node *node, rb_root *root);
extern void __rb_rotate_left(rb_node *node, rb_root *root);
extern void __rb_rotate_right(rb_node *node, rb_root *root);

extern rb_node *rb_first(const rb_root *root);
extern rb_node *rb_last(const rb_root *root);
extern rb_node *rb_prev(const rb_node *node);
extern rb_node *rb_next(const rb_node *node);

#define RB_ROOT                                                                \
  (rb_root) { NULL, }
#define rb_entry(ptr, type, member) container_of(ptr, type, member)

#define RB_EMPTY_ROOT(root) ((root)->rb_node == NULL)
#define RB_EMPTY_NODE(node) (rb_parent(node) == node)
#define RB_CLEAR_NODE(node) (rb_set_parent(node, node))

#define rb_parent(r) ((rb_node *)((r)->rb_parent_color & (unsigned long)~3))
#define rb_color(r) ((r)->rb_parent_color & (unsigned long)1)
#define rb_is_red(r) (!rb_color(r))
#define rb_is_black(r) rb_color(r)
#define rb_set_red(r)                                                          \
  do {                                                                         \
    (r)->rb_parent_color &= (unsigned long)~1;                                 \
  } while (0)
#define rb_set_black(r)                                                        \
  do {                                                                         \
    (r)->rb_parent_color |= (unsigned long)1;                                  \
  } while (0)

#endif /* JMEM_SEGMENT_RB_LOOKUP */
#endif /* JMEM_SEGMENTED_HEAP */

#endif /* !defined(JMEM_HEAP_SEGMENTED_RB_H) */