#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef DEFAULT_ALIGN
  #define DEFAULT_ALIGN (2 * sizeof(void*))
#endif

uintptr_t align_ptr(uintptr_t ptr, uintptr_t align);
uintptr_t align_size(size_t size, uintptr_t align);
bool is_pow2(uintptr_t p);

typedef struct arena_t {
  size_t size;
  size_t curr_offset;
  size_t prev_offset;
  unsigned char* buf;
} arena_t;

void  arena_init(arena_t* a, void* buf, size_t size);
void* arena_alloc(arena_t* a, size_t size);
void* arena_alloc_align(arena_t* a, size_t size, uintptr_t align);

void  arena_resize_element(arena_t* a, void* element, size_t old_size, size_t new_size);
void  arena_resize_element_align(arena_t* a, void* element, size_t old_size, size_t new_size, uintptr_t align);

void  arena_zero(arena_t* a);

void  arena_free(arena_t* a);

typedef struct header_t header_t;
struct header_t {
  header_t* linked_header;
};

typedef struct stack_t {
  size_t size;
  size_t curr_offset;
  header_t* curr_header;
  unsigned char* buf;
} stack_t;

uintptr_t align_fwd_header(uintptr_t ptr, uintptr_t align);

void  stack_init(stack_t* s, void* buf, size_t size);
void* stack_alloc(stack_t* s, size_t size);
void* stack_alloc_align(stack_t* s, size_t size, uintptr_t align);

void  stack_resize_element(stack_t* s, void* element, size_t old_size, size_t new_size);
void  stack_resize_element_align(stack_t* s, void* element, size_t old_size, size_t new_size, uintptr_t align);

void  stack_pop(stack_t* s);
void  stack_free_all(stack_t* s);

typedef struct pool_t {
  size_t size;
  size_t slot_size;
  header_t* current_header;
  unsigned char* buf;
} pool_t;

void  pool_init(pool_t* p, void* buf, size_t size, size_t slot_size);
void  pool_init_align(pool_t* p, void* buf, size_t size, size_t slot_size, uintptr_t align);
void* pool_alloc(pool_t* p);

void  pool_free(pool_t* p, void* slot);
void  pool_free_all(pool_t* p);
