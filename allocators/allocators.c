#include "allocators.h"

#include <memory.h>
#include <stdio.h>

#include "../log/log.h"

bool is_pow2(uintptr_t p) {
  uintptr_t mod = p & (p - 1);
  return mod == 0; 
}

uintptr_t align_ptr(uintptr_t ptr, uintptr_t align) {
  static int index = 0;

  char* msg = NULL;
  sprintf(msg, "Given pointer alignment (#%d) no power of 2\n", index++);

  if (!is_pow2(align)) {
    flog(LOG_ERROR, msg);
    exit(EXIT_FAILURE);
  }

  uintptr_t mod = ptr & (align - 1);

  if (mod != 0)
    ptr += align - mod;

  return ptr;
}

uintptr_t align_size(size_t size, uintptr_t align) {
  size_t align_s = (size_t)align;

  static int index = 0;

  char* msg = NULL;
  sprintf(msg, "Given size alignment (#%d) no power of 2\n", index++);

  if (!is_pow2(align_s)) {
    flog(LOG_ERROR, "Given alignment no power of 2\n");
    exit(EXIT_FAILURE);
  }

  size_t mod = size & (align_s - 1);

  if (mod != 0)
    size += align_s - mod;

  return size;

}

void arena_init(arena_t* a, void* buf, size_t size) {
  a->size = size;
  a->curr_offset = 0;
  a->prev_offset = 0;
  a->buf = buf;
}

void* arena_alloc(arena_t* a, size_t size) {
  return arena_alloc_align(a, size, DEFAULT_ALIGN);
}

void* arena_alloc_align(arena_t* a, size_t size, uintptr_t align) {
  uintptr_t curr_ptr = (uintptr_t)(a->buf + a->curr_offset);
  uintptr_t offset_ptr = align_ptr(curr_ptr, align);
  offset_ptr -= (uintptr_t)a->buf;

  if ((size_t)offset_ptr + size > a->size) {
    flog(LOG_ERROR, "Arena allocation out of bounds\n");
    exit(EXIT_FAILURE);
  } 

  a->prev_offset = offset_ptr;
  a->curr_offset = offset_ptr + size;

  void* ptr = a->buf + offset_ptr;

  memset(ptr, 0, size);

  return ptr;
}

void arena_resize_element(arena_t* a, void* element, size_t old_size, size_t new_size) {
  arena_resize_element_align(a, element, old_size, new_size, DEFAULT_ALIGN);  
}

void arena_resize_element_align(arena_t* a, void* element, size_t old_size, size_t new_size, uintptr_t align) {
  if (!element) {
    element = arena_alloc_align(a, new_size, align);
    return;
  }

  unsigned char* i = (unsigned char*)element;

  if (!(a->buf <= i && i <= a->buf + a->size)) {
    flog(LOG_ERROR, "Resized arena element out of bounds\n");
    exit(EXIT_FAILURE);
  } 

  if (i == a->buf + a->prev_offset) {
    a->curr_offset = a->prev_offset + new_size;
    
    if (new_size > old_size)
      memset(i + old_size, 0, new_size - old_size);
  }

  else {
    void* resized_element = arena_alloc_align(a, new_size, align);
    size_t size = new_size < old_size ? new_size : old_size;
    memcpy(resized_element, element, size);
    element = resized_element;
  }
}

void arena_zero(arena_t* a) {
  memset(a->buf, 0, a->size);
}

void arena_free(arena_t* a) {
  a->curr_offset = 0;
  a->prev_offset = 0;
}

uintptr_t align_ptr_header(uintptr_t ptr, uintptr_t align) {
  uintptr_t needed_size = (uintptr_t)sizeof(header_t); 
  
  if (!is_pow2(needed_size)) {
    flog(LOG_ERROR, "Header size of no power of 2\n");
    exit(EXIT_FAILURE);
  }
  
  if (!is_pow2(align)) {
    flog(LOG_ERROR, "Given alignment no power of 2\n");
    exit(EXIT_FAILURE);
  }

  uintptr_t align_mask = align - 1;
  uintptr_t mod = ptr & align_mask;
  uintptr_t padding = 0;

  padding = align - mod;

  if (padding < needed_size) {
    needed_size -= padding;

    if ((needed_size & align_mask) != 0) 
      padding += align * (1 + needed_size / align);

    else
      padding += needed_size;
  }

  // printf("ptr %d mod %d padding %d header size %d\n", ptr, mod, padding, needed_size);
  
  return ptr + padding;
}

void stack_init(stack_t* s, void* buf, size_t size) {
  s->size = size;
  s->curr_offset = 0;
  s->curr_header = NULL;
  s->buf = buf;
}

void* stack_alloc(stack_t* s, size_t size) {
  return stack_alloc_align(s, size, DEFAULT_ALIGN);
}

void* stack_alloc_align(stack_t* s, size_t size, uintptr_t align) {
  uintptr_t curr_ptr = (uintptr_t)(s->buf + s->curr_offset);
  uintptr_t offset_ptr = align_ptr_header(curr_ptr, align);
  printf("offset after alignment %d\n", offset_ptr - curr_ptr);
  offset_ptr -= (uintptr_t)s->buf;

  if ((size_t)offset_ptr + size > s->size) {
    flog(LOG_ERROR, "Stack allocation out of bounds\n");
    exit(EXIT_FAILURE);
  }

  s->curr_offset = offset_ptr + size;
  unsigned char* ptr = s->buf + offset_ptr;

  header_t* header = (header_t*)(ptr - sizeof(header_t));

  if (s->curr_header)
    header->linked_header = s->curr_header;
  
  s->curr_header = header;

  return memset(ptr, 0, size);
}

void stack_resize_element(stack_t* s, void* element, size_t old_size, size_t new_size) {
  stack_resize_element_align(s, element, old_size, new_size, DEFAULT_ALIGN);
}

void stack_resize_element_align(stack_t* s, void* element, size_t old_size, size_t new_size, uintptr_t align) {
  if (!element) {
    stack_alloc_align(s, new_size, align);
    return;
  }
  
  unsigned char* i = (unsigned char*)element;
  
  if (!(s->buf <= i && i <= s->buf + s->size)) {
    flog(LOG_ERROR, "Resized stack element out of bounds\n");
    exit(EXIT_FAILURE);
  } 

  unsigned char* last_element = (unsigned char*)s->curr_header + sizeof(header_t);

  if (i == last_element) {
    s->curr_offset = (uintptr_t)s->curr_header + sizeof(header_t) + new_size;
    
    if (new_size > old_size)
      memset(i + old_size, 0, new_size - old_size);
  }

  else {
    void* resized_element = stack_alloc_align(s, new_size, align);
    size_t size = new_size < old_size ? new_size : old_size;
    memcpy(resized_element, element, size);
    element = resized_element;
  }
}

void stack_pop(stack_t* s) {
  if (!s->curr_header) return;
  s->curr_header = s->curr_header->linked_header;
  s->curr_offset = (uintptr_t)s->curr_header + sizeof(header_t); 
}

void stack_free_all(stack_t* s) {
  s->curr_header = NULL;
  s->curr_offset = 0;
}

void pool_init(pool_t* p, void* buf, size_t size, size_t slot_size) {
  pool_init_align(p, buf, size, slot_size, DEFAULT_ALIGN);
}

void pool_init_align(pool_t* p, void* buf, size_t size, size_t slot_size, uintptr_t align) {
  uintptr_t buf_zero = (uintptr_t)buf;
  uintptr_t buf_zero_aligned = align_ptr(buf_zero, align);
  size -= buf_zero_aligned - buf_zero;
  slot_size = align_size(slot_size, align);

  printf("aligned buffer size %d\naligned slot size %d\n", size, slot_size);

  if (size % slot_size != 0) {
    printf("slots don't perfectly align\n");
  }
  
  p->size = size;
  p->slot_size = slot_size;
  p->current_header = NULL;
  p->buf = (unsigned char*)buf;

  pool_free_all(p);
}

void* pool_alloc(pool_t* p) {
  header_t* hdr = p->current_header;

  if (!hdr) {
    flog(LOG_WARNING, "Pool allocation failed: No free slot\n");
    return NULL;
  }

  p->current_header = p->current_header->linked_header;
  return memset(hdr, 0, p->slot_size);
}

void pool_free(pool_t* p, void* slot) {
  uintptr_t buf_int = (uintptr_t)p->buf;
  uintptr_t end_int = (uintptr_t)(p->buf + p->size);
  uintptr_t seg_int = (uintptr_t)slot;

  if (!seg_int || seg_int < buf_int || seg_int >= end_int) {
    flog(LOG_WARNING, "Failed to free pool slot: Given address null or out of bounds\n");
    return;
  }

  header_t* hdr = (header_t*)slot;
  hdr->linked_header = p->current_header;
  p->current_header = hdr;
}

void pool_free_all(pool_t* p) {
  size_t slot_count = p->size / p->slot_size;

  for (size_t i = 0; i < slot_count; ++i) {
    header_t* hdr = (header_t*)(p->buf + i * p->slot_size);
    hdr->linked_header = p->current_header;
    p->current_header = hdr;
  }
}
