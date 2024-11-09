#include "allocators.h"

#include <memory.h>
#include <stdio.h>

#include "../log/log.h"

bool is_pow2(uintptr_t p) {
  uintptr_t mod = p & (p - 1);
  return mod == 0; 
}

uintptr_t align_fwd(uintptr_t ptr, uintptr_t align) {
  if (!is_pow2(align)) {
    flog(LOG_ERROR, "Given alignment no power of 2\n");
    exit(EXIT_FAILURE);
  }

  uintptr_t mod = ptr & (align - 1);

  if (mod != 0)
    ptr += align - mod;

  return ptr;
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
  uintptr_t offset_ptr = align_fwd(curr_ptr, align);
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

void arena_resize_item(arena_t* a, void* item, size_t old_size, size_t new_size) {
  arena_resize_item_align(a, item, old_size, new_size, DEFAULT_ALIGN);  
}

void arena_resize_item_align(arena_t* a, void* item, size_t old_size, size_t new_size, uintptr_t align) {
  if (!item) {
    item = arena_alloc_align(a, new_size, align);
    return;
  }

  unsigned char* i = (unsigned char*)item;

  if (!(a->buf <= i && i <= a->buf + a->size)) {
    flog(LOG_ERROR, "Resized arena item out of bounds\n");
    exit(EXIT_FAILURE);
  } 

  if (i == a->buf + a->prev_offset) {
    a->curr_offset = a->prev_offset + new_size;
    
    if (new_size > old_size)
      memset(i + old_size, 0, new_size - old_size);
  }

  else {
    void* resized_item = arena_alloc_align(a, new_size, align);
    size_t size = new_size < old_size ? new_size : old_size;
    memcpy(resized_item, item, size);
    item = resized_item;
  }
}

void arena_zero(arena_t* a) {
  memset(a->buf, 0, a->size);
}

void arena_free(arena_t* a) {
  a->curr_offset = 0;
  a->prev_offset = 0;
}

uintptr_t align_fwd_header(uintptr_t ptr, uintptr_t align) {
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
  uintptr_t offset_ptr = align_fwd_header(curr_ptr, align);
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
    header->prev_header = s->curr_header;
  
  s->curr_header = header;

  memset(ptr, 0, size);

  return (void*)ptr;
}

void stack_resize_item(stack_t* s, void* item, size_t old_size, size_t new_size) {
  stack_resize_item_align(s, item, old_size, new_size, DEFAULT_ALIGN);
}

void stack_resize_item_align(stack_t* s, void* item, size_t old_size, size_t new_size, uintptr_t align) {
  if (!item) {
    stack_alloc_align(s, new_size, align);
    return;
  }
  
  unsigned char* i = (unsigned char*)item;
  
  if (!(s->buf <= i && i <= s->buf + s->size)) {
    flog(LOG_ERROR, "Resized stack item out of bounds\n");
    exit(EXIT_FAILURE);
  } 

  unsigned char* last_item = (unsigned char*)s->curr_header + sizeof(header_t);

  if (i == last_item) {
    s->curr_offset = (uintptr_t)s->curr_header + sizeof(header_t) + new_size;
    
    if (new_size > old_size)
      memset(i + old_size, 0, new_size - old_size);
  }

  else {
    void* resized_item = stack_alloc_align(s, new_size, align);
    size_t size = new_size < old_size ? new_size : old_size;
    memcpy(resized_item, item, size);
    item = resized_item;
  }
}

void stack_pop(stack_t* s) {
  if (!s->curr_header) return;
  s->curr_header = s->curr_header->prev_header;
  s->curr_offset = (uintptr_t)s->curr_header + sizeof(header_t); 
}

void stack_free_all(stack_t* s) {
  s->curr_header = NULL;
  s->curr_offset = 0;
}
