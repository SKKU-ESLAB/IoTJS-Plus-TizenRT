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

#include "jmem-config.h"
#include "jmem.h"
#include "jrt.h"
#include "jcontext.h"

#include <stdlib.h>
#define MALLOC(size) ((void *)malloc(size))
#define FREE(ptr) (free(ptr))

#include "jmem-heap-segmented-rmap-rb.h"

#ifdef JMEM_SEGMENTED_HEAP
#ifdef SEG_RMAP_BINSEARCH

seg_rmap_node_t *segment_rmap_lookup(rb_root *root, uint8_t *addr) {
  rb_node *node = root->rb_node;

  while (node) {
    INCREASE_LOOKUP_DEPTH();
    seg_rmap_node_t *curr_node = container_of(node, seg_rmap_node_t, node);
    uint8_t *curr_addr = curr_node->base_addr;
    intptr_t result = (intptr_t)addr - (intptr_t)curr_addr;

    // curr_addr     < ------ >      addr
    //                result
    if (result >= (intptr_t)SEG_SEGMENT_SIZE) {
      node = node->rb_right;
    } else if (result < 0) {
      node = node->rb_left;
    } else {
      JERRY_HEAP_CONTEXT(comp_i_offset) = (uint32_t)result;
      return (seg_rmap_node_t *)node;
    }
  }

  return (seg_rmap_node_t *)NULL;
}

int segment_rmap_insert(rb_root *root, uint8_t *segment_area, uint32_t sidx) {
  seg_rmap_node_t *node_to_insert =
      (seg_rmap_node_t *)MALLOC(sizeof(seg_rmap_node_t));
  node_to_insert->base_addr = segment_area;
  node_to_insert->sidx = sidx;

  rb_node **_new = &(root->rb_node), *parent = NULL;

  while (*_new) {
    seg_rmap_node_t *node = container_of(*_new, seg_rmap_node_t, node);
    intptr_t result =
        (intptr_t)node_to_insert->base_addr - (intptr_t)node->base_addr;

    parent = *_new;
    if (result < 0)
      _new = &((*_new)->rb_left);
    else if (result > 0)
      _new = &((*_new)->rb_right);
    // else {
    //   JERRY_ASSERT(true);
    // }
  }

  rb_link_node(&(node_to_insert->node), parent, _new);
  rb_insert_color(&(node_to_insert->node), root);
  return 1;
}

void segment_rmap_remove(rb_root *root, uint8_t *addr) {
  seg_rmap_node_t *node_to_free = segment_rmap_lookup(root, addr);
  // JERRY_ASSERT(addr == node_to_free->base_addr);

  if (node_to_free)
    rb_erase(&(node_to_free->node), root);

  FREE(node_to_free);
}

extern void rb_init_node(rb_node *rb) {
  rb->rb_parent_color = 0;
  rb->rb_right = NULL;
  rb->rb_left = NULL;
  RB_CLEAR_NODE(rb);
}

void rb_set_parent(rb_node *rb, rb_node *p) {
  rb->rb_parent_color =
      (rb->rb_parent_color & (unsigned long)3) | (unsigned long)p;
}

void rb_set_color(rb_node *rb, unsigned int color) {
  rb->rb_parent_color = (rb->rb_parent_color & (unsigned long)~1) | color;
}

void rb_link_node(rb_node *node, rb_node *parent, rb_node **rb_link) {
  node->rb_parent_color = (unsigned long)parent;
  node->rb_left = node->rb_right = NULL;

  *rb_link = node;
}

void rb_insert_color(rb_node *node, rb_root *root) {
  rb_node *parent, *gparent;

  while ((parent = rb_parent(node)) && rb_is_red(parent)) {
    gparent = rb_parent(parent);

    if (parent == gparent->rb_left) {
      {
        register rb_node *uncle = gparent->rb_right;
        if (uncle && rb_is_red(uncle)) {
          rb_set_black(uncle);
          rb_set_black(parent);
          rb_set_red(gparent);
          node = gparent;
          continue;
        }
      }

      if (parent->rb_right == node) {
        register rb_node *tmp;
        __rb_rotate_left(parent, root);
        tmp = parent;
        parent = node;
        node = tmp;
      }

      rb_set_black(parent);
      rb_set_red(gparent);
      __rb_rotate_right(gparent, root);
    } else {
      {
        register rb_node *uncle = gparent->rb_left;
        if (uncle && rb_is_red(uncle)) {
          rb_set_black(uncle);
          rb_set_black(parent);
          rb_set_red(gparent);
          node = gparent;
          continue;
        }
      }

      if (parent->rb_left == node) {
        register rb_node *tmp;
        __rb_rotate_right(parent, root);
        tmp = parent;
        parent = node;
        node = tmp;
      }

      rb_set_black(parent);
      rb_set_red(gparent);
      __rb_rotate_left(gparent, root);
    }
  }

  rb_set_black(root->rb_node);
}

void __rb_erase_color(rb_node *node, rb_node *parent, rb_root *root) {
  rb_node *other;

  while ((!node || rb_is_black(node)) && node != root->rb_node) {
    if (parent->rb_left == node) {
      other = parent->rb_right;
      if (rb_is_red(other)) {
        rb_set_black(other);
        rb_set_red(parent);
        __rb_rotate_left(parent, root);
        other = parent->rb_right;
      }
      if ((!other->rb_left || rb_is_black(other->rb_left)) &&
          (!other->rb_right || rb_is_black(other->rb_right))) {
        rb_set_red(other);
        node = parent;
        parent = rb_parent(node);
      } else {
        if (!other->rb_right || rb_is_black(other->rb_right)) {
          rb_set_black(other->rb_left);
          rb_set_red(other);
          __rb_rotate_right(other, root);
          other = parent->rb_right;
        }
        rb_set_color(other, rb_color(parent));
        rb_set_black(parent);
        rb_set_black(other->rb_right);
        __rb_rotate_left(parent, root);
        node = root->rb_node;
        break;
      }
    } else {
      other = parent->rb_left;
      if (rb_is_red(other)) {
        rb_set_black(other);
        rb_set_red(parent);
        __rb_rotate_right(parent, root);
        other = parent->rb_left;
      }
      if ((!other->rb_left || rb_is_black(other->rb_left)) &&
          (!other->rb_right || rb_is_black(other->rb_right))) {
        rb_set_red(other);
        node = parent;
        parent = rb_parent(node);
      } else {
        if (!other->rb_left || rb_is_black(other->rb_left)) {
          rb_set_black(other->rb_right);
          rb_set_red(other);
          __rb_rotate_left(other, root);
          other = parent->rb_left;
        }
        rb_set_color(other, rb_color(parent));
        rb_set_black(parent);
        rb_set_black(other->rb_left);
        __rb_rotate_right(parent, root);
        node = root->rb_node;
        break;
      }
    }
  }
  if (node)
    rb_set_black(node);
}

void rb_erase(rb_node *node, rb_root *root) {
  rb_node *child, *parent;
  unsigned int color;

  if (!node->rb_left)
    child = node->rb_right;
  else if (!node->rb_right)
    child = node->rb_left;
  else {
    rb_node *old = node, *left;

    node = node->rb_right;
    while ((left = node->rb_left) != NULL)
      node = left;

    if (rb_parent(old)) {
      if (rb_parent(old)->rb_left == old)
        rb_parent(old)->rb_left = node;
      else
        rb_parent(old)->rb_right = node;
    } else
      root->rb_node = node;

    child = node->rb_right;
    parent = rb_parent(node);
    color = rb_color(node);

    if (parent == old) {
      parent = node;
    } else {
      if (child)
        rb_set_parent(child, parent);
      parent->rb_left = child;

      node->rb_right = old->rb_right;
      rb_set_parent(old->rb_right, node);
    }

    node->rb_parent_color = old->rb_parent_color;
    node->rb_left = old->rb_left;
    rb_set_parent(old->rb_left, node);

    goto color;
  }

  parent = rb_parent(node);
  color = rb_color(node);

  if (child)
    rb_set_parent(child, parent);
  if (parent) {
    if (parent->rb_left == node)
      parent->rb_left = child;
    else
      parent->rb_right = child;
  } else
    root->rb_node = child;

color:
  if (color == RB_BLACK)
    __rb_erase_color(child, parent, root);
}

void __rb_rotate_left(rb_node *node, rb_root *root) {
  rb_node *right = node->rb_right;
  rb_node *parent = rb_parent(node);

  if ((node->rb_right = right->rb_left))
    rb_set_parent(right->rb_left, node);
  right->rb_left = node;

  rb_set_parent(right, parent);

  if (parent) {
    if (node == parent->rb_left)
      parent->rb_left = right;
    else
      parent->rb_right = right;
  } else
    root->rb_node = right;
  rb_set_parent(node, right);
}

void __rb_rotate_right(rb_node *node, rb_root *root) {
  rb_node *left = node->rb_left;
  rb_node *parent = rb_parent(node);

  if ((node->rb_left = left->rb_right))
    rb_set_parent(left->rb_right, node);
  left->rb_right = node;

  rb_set_parent(left, parent);

  if (parent) {
    if (node == parent->rb_right)
      parent->rb_right = left;
    else
      parent->rb_left = left;
  } else
    root->rb_node = left;
  rb_set_parent(node, left);
}

rb_node *rb_first(const rb_root *root) {
  rb_node *n;

  n = root->rb_node;
  if (!n)
    return NULL;
  while (n->rb_left)
    n = n->rb_left;
  return n;
}

rb_node *rb_last(const rb_root *root) {
  rb_node *n;

  n = root->rb_node;
  if (!n)
    return NULL;
  while (n->rb_right)
    n = n->rb_right;
  return n;
}

rb_node *rb_prev(const rb_node *node) {
  rb_node *parent;

  if (rb_parent(node) == node)
    return NULL;

  /* If we have a left-hand child, go down and then right as far
     as we can. */
  if (node->rb_left) {
    node = node->rb_left;
    while (node->rb_right)
      node = node->rb_right;
    return (rb_node *)node;
  }

  /* No left-hand children. Go up till we find an ancestor which
     is a right-hand child of its parent */
  while ((parent = rb_parent(node)) && node == parent->rb_left)
    node = parent;

  return parent;
}

rb_node *rb_next(const rb_node *node) {
  rb_node *parent;

  if (rb_parent(node) == node)
    return NULL;

  /* If we have a right-hand child, go down and then left as far
     as we can. */
  if (node->rb_right) {
    node = node->rb_right;
    while (node->rb_left)
      node = node->rb_left;
    return (rb_node *)node;
  }

  /* No right-hand children.	Everything down and left is
     smaller than us, so any 'next' node must be in the general
     direction of our parent. Go up the tree; any time the
     ancestor is a right-hand child of its parent, keep going
     up. First time it's a left-hand child of its parent, said
     parent is our 'next' node. */
  while ((parent = rb_parent(node)) && node == parent->rb_right)
    node = parent;

  return parent;
}
#endif /* SEG_RMAP_BINSEARCH */
#endif /* JMEM_SEGMENTED_HEAP */