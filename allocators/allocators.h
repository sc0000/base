#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef DEFAULT_ALIGN
  #define DEFAULT_ALIGN (2 * sizeof(void*))
#endif

uintptr_t align_fwd(uintptr_t ptr, size_t align);
bool is_pow2(uintptr_t p);

typedef struct arena_t {
  size_t size;
  size_t curr_offset;
  size_t prev_offset;
  unsigned char* buf;
} arena_t;

void arena_init(arena_t* a, void* buf, size_t size);
void* arena_alloc(arena_t* a, size_t size);
void* arena_alloc_align(arena_t* a, size_t size, uintptr_t align);

void arena_resize_item(arena_t* a, void* item, size_t old_size, size_t new_size);
void arena_resize_item_align(arena_t* a, void* item, size_t old_size, size_t new_size, uintptr_t align);

void arena_zero(arena_t* a);

void arena_free(arena_t* a);

typedef struct header_t header_t;
struct header_t {
  header_t* prev_header;
};

typedef struct stack_t {
  size_t size;
  size_t curr_offset;
  header_t* curr_header;
  unsigned char* buf;
} stack_t;

uintptr_t align_fwd_header(uintptr_t ptr, uintptr_t align);

void stack_init(stack_t* s, void* buf, size_t size);
void* stack_alloc(stack_t* s, size_t size);
void* stack_alloc_align(stack_t* s, size_t size, uintptr_t align);

void stack_resize_item(stack_t* s, void* item, size_t old_size, size_t new_size);
void stack_resize_item_align(stack_t* s, void* item, size_t old_size, size_t new_size, uintptr_t align);

void stack_pop(stack_t* s);
void stack_free_all(stack_t* s);
