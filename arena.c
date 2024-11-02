#include "arena.h"

#include "utils.h"

#include <assert.h>
#include <memory.h>
#include <stdio.h>

#include "log/log.h"

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
    flog(LOG_ERROR, "arena allocation out of bounds\n");
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
    flog(LOG_ERROR, "resized arena item out of bounds\n");
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
