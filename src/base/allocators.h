#pragma once

#ifdef _MSC_VER
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include <memory.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "base/mem_utils.h"
#include "base/log.h"

/* 
  --- DYNAMIC ARRAY --- 
  
  Simple dynamic array, similar to std::vector. It unfortunately
  only checks for the size of a pushed element.
  Grows geometrically.

*/

typedef struct da_hdr {
  size_t capacity;
  size_t occupied;
  size_t element_size;
} da_hdr_t;

/// @brief ---INTERNAL FUNCTION---
/// Returns the header of the array, containing relevant meta data.
static inline da_hdr_t* darray_get_hdr(void* darray)
{
  uintptr_t hdr_size = align_size(sizeof(da_hdr_t), DEFAULT_ALIGN);
  return darray ? (da_hdr_t*)((uintptr_t)darray - hdr_size) : NULL;
}

/// @brief Initializes a dynamic array with the given size.
void* darray_init(size_t element_size, size_t num);

/// @brief ---INTERNAL FUNCTION---
/// Checks if the dynamic array has enough capacity to 
/// accommdate the given number additional elements. 
/// Re-allocates the array otherwise.
/// Sets the `occupied` parameter to include the new elements.
/// Will be updated to use `darray_reserve` internally.
void* darray_alloc(void* darray, size_t element_size, size_t num);

/// @brief Pushes an element to the given array.
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

/// @brief Inserts an element at the given position.
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

/// @brief Creates a given number of vacant slots at the given position. 
void* darray_make_space(void* darray, size_t pos, size_t element_size, size_t num);

/// @brief Removes the last element. Capacity remains unchanged.
void darray_pop_last(void* darray);

/// @brief Removes the first element. 
/// The others each move one position to the left.
void darray_pop_first(void* darray);

/// @brief Removes all elements. Capacity remains unchanged.
void darray_clear(void* darray);

/// @brief Returns the number of elements contained in the array.
size_t darray_size(void* darray);

/// @brief Returns the capacity of the array.
size_t darray_capacity(void* darray);

/// @brief Sets the `occupied` parameter of the array to the given number.
/// This function is probably rarely useful. Doesn't do anything if 
/// new given size is greater than the size of the array. It was originally
/// written as part of a software renderer, to reset the number of 
/// triangles in a rendered 3D object after the frustum clipping stage 
/// added a few temporary ones. Capacity remains unchanged.
void darray_reset_size(void* darray, size_t size);

/// @brief Sets the capacity of the array, to avoid unnecessary
/// reallocations when adding elements repeatedly.
/// This macro creates a more intuitive syntax when calling the function.
#define darray_reserve(darray, new_cap)\
do {\
  darray_reserve_base((void**)&(darray), new_cap);\
} while (0);


/// @brief Base function of the `darray_reserve` macro.
void darray_reserve_base(void** darray, size_t new_cap);

/// @brief Shrinks the capacity to the current number of contained elements.
/// This macro creates a more intuitive syntax when calling the function.
#define darray_shrink_to_fit(darray)\
do {\
  darray_shrink_to_fit_base((void**)&(darray));\
} while (0);

/// @brief Base function of the `darray_shrink_to_fit` macro.
void darray_shrink_to_fit_base(void** darray);

/* 
  --- ARENA ALLOCATOR ---

  Linear allocator without any header structure. As opposed to
  to dynamic array, the arena accommodates data of mixed types.
  Only allows to pop the last element *once*, otherwise all elements have
  to be freed at once (similar to a stack frame).
  As of yet, the arena's buffer can't be re-allocated either.

*/

typedef struct arena {
  unsigned char* buf;
  size_t size;
  size_t curr_offset;
  size_t prev_offset;
} arena_t;

/// @brief Initializes the arena. The buffer might live on either
/// stack or heap and is therefore needed to be given manually.
void arena_init(arena_t* a, void* buf, size_t size);

/// @brief Allocates the specified number of bytes in the arena, 
/// with manual alignment (probably rarely of use). For default
/// alignment, use `arena_alloc` instead.
/// @return A pointer to the allocated block of memory.
void* arena_alloc_align(arena_t* a, size_t size, uintptr_t align);


/// @brief Allocates the specified number of bytes in the arena, here
/// with manual alignment, with default alignment. For manual
/// alignment, use `arena_alloc_align()` instead.
/// @return A pointer to the allocated block of memory.
static inline void* arena_alloc(arena_t* a, size_t size)
{
  return arena_alloc_align(a, size, DEFAULT_ALIGN);
}

/// @brief Resizes the block of memory that the `element` pointer has access to,
/// with manual alignment. For default alignment, use `arena_resize_element()` instead.
/// If the new size is larger than the old size and it is not the last element, the
/// element will be moved to the end of the occupied block, leaving a gap that won't
/// reused until the arena is freed. If `element` is null, a new block of size `new_size`
/// will be allocated, with `element` then pointing to it.
void arena_resize_element_align(arena_t* a, void* element, size_t old_size, size_t new_size, uintptr_t align);

/// @brief Resizes the block of memory that the `element` pointer has access to,
/// with default alignment. For manual alignment, use `arena_resize_element_align()` instead.
/// If the new size is larger than the old size and it is not the last element, the
/// element will be moved to the end of the occupied block, leaving a gap that won't
/// reused until the arena is freed. If `element` is null, a new block of size `new_size`
/// will be allocated, with `element` then pointing to it.
static inline void  arena_resize_element(arena_t* a, void* element, size_t old_size, size_t new_size)
{
  arena_resize_element_align(a, element, old_size, new_size, DEFAULT_ALIGN);  
}

/// @brief Sets all bytes in the arena to 0.
static inline void arena_zero(arena_t* a)
{
  if (!a) {
    flog(LOG_WARNING, "arena_zero(): arena invalid");
    return;
  }

  memset(a->buf, 0, a->size);
}

/// @brief Removes the last element. This works only once before
/// a new element needs to be added. For more granular control,
/// use another allocator. 
static inline void arena_pop(arena_t* a)
{
  if (!a) {
    flog(LOG_WARNING, "arena_pop(): arena invalid");
    return;
  }

  a->curr_offset = a->prev_offset;
}

/// @brief Sets the internal offset to 0, allowing the buffer to
/// be overwritten in its entirety. This doesn't free the buffer
/// itself, however! 
static inline void arena_clear(arena_t* a)
{
  if (!a) {
    flog(LOG_WARNING, "arena_clear(): arena invalid");
    return;
  }

  a->curr_offset = 0;
  a->prev_offset = 0;
}

/* 
  --- STACK ALLOCATOR ---

  Linear allocator following the FAFO principle. As opposed to
  to the arena allocator, this one allows for removing the last element
  repeatedly, by replacing a (currently) 8-byte header between each element.
  As of yet, the stack's buffer can't be re-allocated.

*/

typedef struct hdr {
  struct hdr* linked_hdr;
} hdr_t;

typedef struct stack {
  unsigned char* buf;
  hdr_t* curr_hdr;
  size_t size;
  size_t curr_offset;
} stack_t;

/// @brief ---INTERNAL FUNCTION--- Adjust the current offset to accommodate a header
/// before a new block of memory is allocated.
uintptr_t align_ptr_hdr(uintptr_t ptr, uintptr_t align, size_t hdr_size);

/// @brief Initializes a stack allocator. The buffer might live on either
/// stack or heap and is therefore needed to be given manually.
void  stack_init(stack_t* s, void* buf, size_t size);

/// @brief Allocates the specified number of bytes in the stack buffer, 
/// with manual alignment (probably rarely of use). For default
/// alignment, use `stack_alloc()` instead.
/// @return A pointer to the allocated block of memory.
void* stack_alloc_align(stack_t* s, size_t size, uintptr_t align);

/// @brief Allocates the specified number of bytes in the stack buffer, 
/// with default alignment. For manual alignment, use `stack_alloc_align()`
/// instead.
/// @return A pointer to the allocated block of memory.
static inline void* stack_alloc(stack_t* s, size_t size)
{
  return stack_alloc_align(s, size, DEFAULT_ALIGN);
}

/// @brief Resizes the block of memory that the `element` pointer has access to,
/// with manual alignment. For default alignment, use `stack_resize_element()` instead.
/// If the new size is larger than the old size and it is not the last element, the
/// element will be moved to the end of the occupied block, leaving a gap that won't
/// reused until the arena is freed. If `element` is null, a new block of size `new_size`
/// will be allocated, with `element` then pointing to it.
void stack_resize_element_align(stack_t* s, void* element, size_t old_size, size_t new_size, uintptr_t align);

/// @brief Resizes the block of memory that the `element` pointer has access to,
/// with default alignment. For manual alignment, use `stack_resize_element_align()` instead.
/// If the new size is larger than the old size and it is not the last element, the
/// element will be moved to the end of the occupied block, leaving a gap that won't
/// reused until the arena is freed. If `element` is null, a new block of size `new_size`
/// will be allocated, with `element` then pointing to it.
static inline void stack_resize_element(stack_t* s, void* element, size_t old_size, size_t new_size)
{
  stack_resize_element_align(s, element, old_size, new_size, DEFAULT_ALIGN);
}

/// @brief Removes the last element from the stack. 
static inline void stack_pop(stack_t* s)
{
  if (!s->curr_hdr) {
    flog(LOG_WARNING, "stack_pop(): stack invalid");
    return;
  }

  s->curr_hdr = s->curr_hdr->linked_hdr;
  s->curr_offset = (uintptr_t)s->curr_hdr + sizeof(hdr_t); 
}

/// @brief Sets the internal offset to 0, allowing the buffer to
/// be overwritten in its entirety. This doesn't free the buffer
/// itself, however! 
static inline void stack_clear(stack_t* s)
{
  if (!s->curr_hdr) {
    flog(LOG_WARNING, "stack_clear(): stack invalid");
    return;
  }

  s->curr_hdr = NULL;
  s->curr_offset = 0;
}

/* 
  --- POOL ALLOCATOR ---

  This allocator allows for random access of all its elements. They're
  each occupying slots of uniform size, so there will be a significant
  amount of fragmentation if the elements are widely varying in size.
  Allocation is as fast as with the linear allocators.  

*/

typedef struct pool {
  unsigned char* buf;
  hdr_t* curr_hdr;
  size_t size;
  size_t slot_size;
} pool_t;

/// @brief Initializes a pool allocator. The buffer might live on either
/// stack or heap and is therefore needed to be given as an argument.
/// This function allows for manual alignment of the buffer and slot sizes
/// (probably rarely of use). For default alignment, use `pool_init()` instead.
void pool_init_align(pool_t* p, void* buf, size_t size, size_t slot_size, uintptr_t align);

/// @brief Initializes a pool allocator. The buffer might live on either
/// stack or heap and is therefore needed to be given as an argument.
/// This function applies default alignment of the buffer and slot sizes.
/// For manual alignment, use `pool_init_align()` instead.
static inline void pool_init(pool_t* p, void* buf, size_t size, size_t slot_size)
{
  pool_init_align(p, buf, size, slot_size, DEFAULT_ALIGN);
}

/// @brief Allocates a block of memory of fixed size, as specified in
/// `pool_init()` or `pool_init_align`.
/// @return A pointer to the allocated block, or null if the pool itself
/// is null, or not initialized.
void* pool_alloc(pool_t* p);

/// @brief Marks the given slot as free, nulls the pointer.
void  pool_free(pool_t* p, void* slot);

/// @brief Marks all slots in the pool as free. 
void  pool_free_all(pool_t* p);

/* 
  --- FREE LIST ALLOCATOR ---

  Similar to the pool allocator, the free list allocator allows for 
  random access of all its elements. Depending on the allocation policy,
  either the first or best fitting free stretch of memory is allocated.
  Neighboring free blocks are concatenated. Each free or occupied block
  is preceded by a (currently) 16-byte header.

*/

typedef struct fl_hdr {
  struct fl_hdr* linked_hdr;
  size_t block_size;
} fl_hdr_t;

typedef struct free_list {
  unsigned char* buf;
  size_t size;
} free_list_t;

typedef enum {
  FIRST_SLOT,
  BEST_SLOT,
  NUM_POLICIES
} fl_policy;

/// @brief ---INTERNAL FUNCTION---
/// Returns the first header in the buffer.
static inline fl_hdr_t* fl_first_hdr(free_list_t* fl)
{
  return fl ? (fl_hdr_t*)fl->buf : NULL;
}

/// @brief Initializes the free list with manual alignment (probably rarely of use).
/// For default alignment, use `free_list_init()` instead. The buffer might live on
/// either stack or heap, so it has to be given as an argument.
void free_list_init_align(free_list_t* fl, void* buf, size_t size, uintptr_t align);

/// @brief Initializes the free list with default alignment.
/// For manual alignment, use `free_list_init_align()` instead. The buffer might live on
/// either stack or heap, so it has to be given as an argument.
static inline void free_list_init(free_list_t* fl, void* buf, size_t size)
{
  free_list_init_align(fl, buf, size, DEFAULT_ALIGN);
}

/// @brief Allocates either the first (with the `FIRST_SLOT` policy) or best fitting
/// (with the `BEST_SLOT` policy) memory block. The given size is aligned according to the given
/// value. For default alignment, use `free_list_alloc()` instead.
/// @returns A pointer to the found block.
void* free_list_alloc_align(free_list_t* fl, size_t size, fl_policy policy, uintptr_t align); // ?

/// @brief Allocates either the first (with the `FIRST_SLOT` policy) or best fitting
/// (with the `BEST_SLOT` policy) free memory block. The given size is aligned automatically.
/// For manual alignment, use `free_list_alloc_aling()` instead.
/// @returns A pointer to the found block.
static inline void* free_list_alloc(free_list_t* fl, size_t size, fl_policy policy)
{
  return free_list_alloc_align(fl, size, policy, DEFAULT_ALIGN);
}

/// @brief Frees the given block. Tries to merge with neighboring blocks, if they are free
/// themselves. Calculates all sizes with given alignment value (which should probably match
/// the one used for allocation). For working with default alignment, use `free_list_free()` instead.
void free_list_free_align(free_list_t* fl, void* element, size_t element_size, uintptr_t align);

/// @brief Frees the given block. Tries to merge with neighboring blocks, if they are free
/// themselves. Calculates all sizes with the default alignment value.
/// For working with default alignment, use `free_list_free_align()` instead.
static inline void free_list_free(free_list_t* fl, void* element, size_t element_size)
{
  free_list_free_align(fl, element, element_size, DEFAULT_ALIGN);
}

/// @brief Clears the entire buffer, stores the then available size as 
/// reduced according to the given alignment value. For default alignment,
/// use `free_list_free()` instead.
void free_list_free_all_align(free_list_t* fl, uintptr_t align);

/// @brief Clears the entire buffer, stores the then available size as 
/// reduced according to the default alignment value. For manual alignment,
/// use `free_list_free_align()` instead.
static inline void free_list_free_all(free_list_t* fl)
{
  free_list_free_all_align(fl, DEFAULT_ALIGN);
}

/// @brief ---INTERNAL FUNCTION---
/// Finds the first available memory block of at least the given size.
void free_list_find_first(free_list_t* fl, size_t size, fl_hdr_t** found_hdr, fl_hdr_t** prev_hdr);

/// @brief ---INTERNAL FUNCTION---
/// Finds the smallest memory block that still accommodates the given size.
void free_list_find_best(free_list_t* fl, size_t size, fl_hdr_t** found_hdr, fl_hdr_t** prev_hdr);
