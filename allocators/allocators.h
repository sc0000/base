#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef DEFAULT_ALIGN
#define DEFAULT_ALIGN (2 * sizeof(void*))
#endif

uintptr_t align_ptr(uintptr_t ptr, uintptr_t align);
uintptr_t align_size(size_t size, uintptr_t align);

bool is_pow2(uintptr_t p);

bool within_bounds(void* ptr, unsigned char* buf, size_t buf_size);

typedef struct da_hdr_t {
  size_t capacity;
  size_t occupied;
  size_t element_size;
} da_hdr_t;

static inline da_hdr_t* darray_get_hdr(void* darray)
{
  uintptr_t hdr_size = align_size(sizeof(da_hdr_t), DEFAULT_ALIGN);
  return (da_hdr_t*)((uintptr_t)darray - hdr_size);
}

void* darray_init(size_t element_size, size_t num);
void* darray_alloc(void* darray, size_t element_size, size_t num);

#define darray_push(darray, element)\
do {\
  if (sizeof(*(darray)) != sizeof(element)) {\
    flog(LOG_ERROR, "darray_push: element to push has not the right type");\
    exit(EXIT_FAILURE);\
  }\
  \
  (darray) = darray_alloc(darray, sizeof(element), 1);\
  (darray)[darray_size(darray) - 1] = (element);\
}	while (0);

#define darray_insert(darray, pos, element)\
do {\
  if (sizeof(*(darray)) != sizeof(element)) {\
    flog(LOG_ERROR, "darray_push: element to push has not the right type");\
    exit(EXIT_FAILURE);\
  }\
  \
  (darray) = darray_make_space(darray, pos, sizeof(element), 1);\
  (darray)[pos] = (element);\
} while (0);

void* darray_make_space(void* darray, size_t pos, size_t element_size, size_t num);
void  darray_pop_last(void* darray);
void  darray_pop_first(void* darray);
void  darray_clear(void* darray);

size_t darray_size(void* darray);
size_t darray_capacity(void* darray);

void darray_reset_size(void* darray, size_t size);

#define darray_reserve(darray, new_cap)\
do {\
  darray_reserve_base((void**)&(darray), new_cap);\
} while (0);

void darray_reserve_base(void** darray, size_t new_cap);

#define darray_shrink_to_fit(darray)\
do {\
  darray_shrink_to_fit_base((void**)&(darray));\
} while (0);

void darray_shrink_to_fit_base(void** darray);

typedef struct arena_t {
  unsigned char* buf;
  size_t size;
  size_t curr_offset;
  size_t prev_offset;
} arena_t;

void  arena_init(arena_t* a, void* buf, size_t size);
void* arena_alloc(arena_t* a, size_t size);
void* arena_alloc_align(arena_t* a, size_t size, uintptr_t align);

void  arena_resize_element(arena_t* a, void* element, size_t old_size, size_t new_size);
void  arena_resize_element_align(arena_t* a, void* element, size_t old_size, size_t new_size, uintptr_t align);

void  arena_zero(arena_t* a);
void  arena_pop(arena_t* a);
void  arena_free(arena_t* a);

typedef struct hdr_t {
  struct hdr_t* linked_hdr;
} hdr_t;

typedef struct stack_t {
  unsigned char* buf;
  hdr_t* curr_hdr;
  size_t size;
  size_t curr_offset;
} stack_t;

uintptr_t align_ptr_hdr(uintptr_t ptr, uintptr_t align, size_t hdr_size);

void  stack_init(stack_t* s, void* buf, size_t size);
void* stack_alloc(stack_t* s, size_t size);
void* stack_alloc_align(stack_t* s, size_t size, uintptr_t align);

void  stack_resize_element(stack_t* s, void* element, size_t old_size, size_t new_size);
void  stack_resize_element_align(stack_t* s, void* element, size_t old_size, size_t new_size, uintptr_t align);

void  stack_pop(stack_t* s);
void  stack_free_all(stack_t* s);

typedef struct pool_t {
  unsigned char* buf;
  hdr_t* curr_hdr;
  size_t size;
  size_t slot_size;
} pool_t;

void  pool_init(pool_t* p, void* buf, size_t size, size_t slot_size);
void  pool_init_align(pool_t* p, void* buf, size_t size, size_t slot_size, uintptr_t align);
void* pool_alloc(pool_t* p);

void  pool_free(pool_t* p, void* slot);
void  pool_free_all(pool_t* p);

typedef struct fl_hdr_t {
  struct fl_hdr_t* linked_hdr;
  size_t block_size;
} fl_hdr_t;

typedef struct free_list_t {
  unsigned char* buf;
  size_t size;
} free_list_t;

typedef enum {
  FIRST_SLOT,
  BEST_SLOT,
  NUM_POLICIES
} fl_policy;

static inline fl_hdr_t* fl_first_hdr(free_list_t* fl)
{
  return (fl_hdr_t*)fl->buf;
}

void  free_list_init(free_list_t* fl, void* buf, size_t size);
void  free_list_init_align(free_list_t* fl, void* buf, size_t size, uintptr_t align);
void* free_list_alloc(free_list_t* fl, size_t size, fl_policy policy);
void* free_list_alloc_align(free_list_t* fl, size_t size, fl_policy policy, uintptr_t align); // ?

void  free_list_free(free_list_t* fl, void* element, size_t element_size);
void  free_list_free_align(free_list_t* fl, void* element, size_t element_size, uintptr_t align);
void  free_list_free_all(free_list_t* fl);
void  free_list_free_all_align(free_list_t* fl, uintptr_t align);

void  free_list_find_first(free_list_t* fl, size_t size, fl_hdr_t** found_hdr, fl_hdr_t** prev_hdr);
void  free_list_find_best(free_list_t* fl, size_t size, fl_hdr_t** found_hdr, fl_hdr_t** prev_hdr);
