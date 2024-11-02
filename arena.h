#pragma once

#include "defs.h"

typedef struct arena_t {
  size_t size;
  size_t curr_offset;
  size_t prev_offset;
  uchar* buf;
} arena_t;

#ifndef DEFAULT_ALIGN
#define DEFAULT_ALIGN (2 * sizeof(void*))
#endif

void arena_init(arena_t* a, void* buf, size_t size);
void* arena_alloc(arena_t* a, size_t size);
void* arena_alloc_align(arena_t* a, size_t size, uintptr_t align);

void arena_resize_item(arena_t* a, void* item, size_t old_size, size_t new_size);
void arena_resize_item_align(arena_t* a, void* item, size_t old_size, size_t new_size, uintptr_t align);

void arena_zero(arena_t* a);

void arena_free(arena_t* a);
