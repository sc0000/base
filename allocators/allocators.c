#include "allocators.h"

#include <memory.h>
#include <stdio.h>

#include "../log/log.h"


// TODO: Implement generic warning system for null returns (macro?)
// static int call_count = 0;
// char* msg = NULL;
// sprintf(msg, "free_list_find_first call #%d returned null\n", call_count++);

// if (!hdr)
//   flog(LOG_WARNING, msg);

bool is_pow2(uintptr_t p)
{
  uintptr_t mod = p & (p - 1);
  return mod == 0; 
}

bool within_bounds(void* ptr, unsigned char* buf, size_t buf_size)
{
  uintptr_t uptr = (uintptr_t)ptr;
  uintptr_t ubuf = (uintptr_t)buf;
  uintptr_t uend = (ubuf + (uintptr_t)buf_size);
  
  return (uptr > ubuf && uptr < uend);
}

uintptr_t align_ptr(uintptr_t ptr, uintptr_t align)
{
  static int call_count = 0;

  char* msg = NULL;
  sprintf(msg, "Given pointer alignment (#%d) no power of 2", call_count++);

  if (!is_pow2(align)) {
    flog(LOG_ERROR, msg);
    exit(EXIT_FAILURE);
  }

  uintptr_t mod = ptr & (align - 1);

  if (mod != 0)
    ptr += align - mod;

  return ptr;
}

uintptr_t align_size(size_t size, uintptr_t align)
{
  size_t align_s = (size_t)align;

  static int call_count = 0;

  char* msg = NULL;
  sprintf(msg, "Given size alignment (#%d) no power of 2", call_count++);

  if (!is_pow2(align_s)) {
    flog(LOG_ERROR, "Given alignment no power of 2");
    exit(EXIT_FAILURE);
  }

  size_t mod = size & (align_s - 1);

  if (mod != 0)
    size += align_s - mod;

  return size;
}

void arena_init(arena_t* a, void* buf, size_t size)
{
  a->size = size;
  a->curr_offset = 0;
  a->prev_offset = 0;
  a->buf = (unsigned char*)buf;
}

void* arena_alloc(arena_t* a, size_t size) {
  return arena_alloc_align(a, size, DEFAULT_ALIGN);
}

void* arena_alloc_align(arena_t* a, size_t size, uintptr_t align)
{
  uintptr_t curr_ptr = (uintptr_t)(a->buf + a->curr_offset);
  uintptr_t offset_ptr = align_ptr(curr_ptr, align);
  offset_ptr -= (uintptr_t)a->buf;

  if ((size_t)offset_ptr + size > a->size) {
    flog(LOG_ERROR, "Arena allocation out of bounds");
    exit(EXIT_FAILURE);
  } 

  a->prev_offset = offset_ptr;
  a->curr_offset = offset_ptr + size;

  void* ptr = a->buf + offset_ptr;

  return memset(ptr, 0, size);
}

void arena_resize_element(arena_t* a, void* element, size_t old_size, size_t new_size)
{
  arena_resize_element_align(a, element, old_size, new_size, DEFAULT_ALIGN);  
}

void arena_resize_element_align(arena_t* a, void* element, size_t old_size, size_t new_size, uintptr_t align)
{
  if (!element) {
    element = arena_alloc_align(a, new_size, align);
    return;
  }

  unsigned char* i = (unsigned char*)element;

  if (a->buf > i || i > a->buf + a->size) {
    flog(LOG_ERROR, "Resized arena element out of bounds");
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

void arena_zero(arena_t* a)
{
  memset(a->buf, 0, a->size);
}

void  arena_pop(arena_t* a)
{
  a->curr_offset = a->prev_offset;
}

void arena_free(arena_t* a)
{
  a->curr_offset = 0;
  a->prev_offset = 0;
}

uintptr_t align_ptr_hdr(uintptr_t ptr, uintptr_t align, size_t hdr_size)
{
  if (!is_pow2(align)) {
    flog(LOG_ERROR, "Given alignment no power of 2");
    exit(EXIT_FAILURE);
  }

  uintptr_t needed_size = (uintptr_t)hdr_size; 
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

void stack_init(stack_t* s, void* buf, size_t size)
{
  s->size = size;
  s->curr_offset = 0;
  s->curr_hdr = NULL;
  s->buf = (unsigned char*)buf;
}

void* stack_alloc(stack_t* s, size_t size)
{
  return stack_alloc_align(s, size, DEFAULT_ALIGN);
}

void* stack_alloc_align(stack_t* s, size_t size, uintptr_t align)
{
  uintptr_t curr_ptr = (uintptr_t)(s->buf + s->curr_offset);
  uintptr_t offset_ptr = align_ptr_hdr(curr_ptr, align, sizeof(hdr_t));
  offset_ptr -= (uintptr_t)s->buf;

  if ((size_t)offset_ptr + size > s->size) {
    flog(LOG_ERROR, "Stack allocation out of bounds");
    exit(EXIT_FAILURE);
  }

  s->curr_offset = offset_ptr + size;
  unsigned char* ptr = s->buf + offset_ptr;

  hdr_t* header = (hdr_t*)(ptr - sizeof(hdr_t));

  if (s->curr_hdr)
    header->linked_hdr = s->curr_hdr;
  
  s->curr_hdr = header;

  return memset(ptr, 0, size);
}

void stack_resize_element(stack_t* s, void* element, size_t old_size, size_t new_size)
{
  stack_resize_element_align(s, element, old_size, new_size, DEFAULT_ALIGN);
}

void stack_resize_element_align(stack_t* s, void* element, size_t old_size, size_t new_size, uintptr_t align)
{
  if (!element) {
    stack_alloc_align(s, new_size, align);
    return;
  }
  
  unsigned char* i = (unsigned char*)element;
  
  if (s->buf > i || i > s->buf + s->size) {
    flog(LOG_ERROR, "Resized stack element out of bounds");
    exit(EXIT_FAILURE);
  } 

  unsigned char* last_element = (unsigned char*)s->curr_hdr + sizeof(hdr_t);

  if (i == last_element) {
    s->curr_offset = (uintptr_t)s->curr_hdr + sizeof(hdr_t) + new_size;
    
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

void stack_pop(stack_t* s)
{
  if (!s->curr_hdr) return;
  s->curr_hdr = s->curr_hdr->linked_hdr;
  s->curr_offset = (uintptr_t)s->curr_hdr + sizeof(hdr_t); 
}

void stack_free_all(stack_t* s)
{
  s->curr_hdr = NULL;
  s->curr_offset = 0;
}

void pool_init(pool_t* p, void* buf, size_t size, size_t slot_size)
{
  pool_init_align(p, buf, size, slot_size, DEFAULT_ALIGN);
}

void pool_init_align(pool_t* p, void* buf, size_t size, size_t slot_size, uintptr_t align)
{
  uintptr_t buf_zero = (uintptr_t)buf;
  uintptr_t buf_zero_aligned = align_ptr(buf_zero, align);
  size -= buf_zero_aligned - buf_zero;
  slot_size = align_size(slot_size, align);
  
  p->size = size;
  p->slot_size = slot_size;
  p->curr_hdr = NULL;
  p->buf = (unsigned char*)buf;

  pool_free_all(p);
}

void* pool_alloc(pool_t* p)
{
  hdr_t* hdr = p->curr_hdr;

  if (!hdr) {
    flog(LOG_WARNING, "Pool allocation failed: No free slot");
    return NULL;
  }

  p->curr_hdr = p->curr_hdr->linked_hdr;
  return memset(hdr, 0, p->slot_size);
}

void pool_free(pool_t* p, void* slot)
{
  uintptr_t buf_int = (uintptr_t)p->buf;
  uintptr_t end_int = (uintptr_t)(p->buf + p->size);
  uintptr_t seg_int = (uintptr_t)slot;

  if (!seg_int || seg_int < buf_int || seg_int >= end_int) {
    flog(LOG_WARNING, "Failed to free pool slot: Given address null or out of bounds");
    return;
  }

  hdr_t* hdr = (hdr_t*)slot;
  hdr->linked_hdr = p->curr_hdr;
  p->curr_hdr = hdr;
}

void pool_free_all(pool_t* p)
{
  size_t slot_count = p->size / p->slot_size;

  for (size_t i = 0; i < slot_count; ++i) {
    hdr_t* hdr = (hdr_t*)(p->buf + i * p->slot_size);
    hdr->linked_hdr = p->curr_hdr;
    p->curr_hdr = hdr;
  }
}

void free_list_init(free_list_t* fl, void* buf, size_t size)
{
  free_list_init_align(fl, buf, size, DEFAULT_ALIGN);
}

void free_list_init_align(free_list_t* fl, void* buf, size_t size, uintptr_t align)
{
  uintptr_t buf_zero = (uintptr_t)buf;
  uintptr_t buf_zero_aligned = align_ptr(buf_zero, align);
  uintptr_t diff = buf_zero_aligned - buf_zero; 
  
  fl->buf = (unsigned char*)buf_zero_aligned;
  fl->size = size - diff;

  fl_hdr_t* first_hdr = fl_first_hdr(fl);
  size_t hdr_size = align_size(sizeof(fl_hdr_t), align);
  first_hdr->linked_hdr = NULL;
  first_hdr->block_size = size - hdr_size;

  char msg[256];
  sprintf(msg, "Free list init. Size aligned to %llu (diff %llu)", size, diff);
  flog(LOG_INFO, msg);
}

void* free_list_alloc(free_list_t* fl, size_t size, fl_policy policy)
{
  return free_list_alloc_align(fl, size, policy, DEFAULT_ALIGN);
}

void* free_list_alloc_align(free_list_t* fl, size_t size, fl_policy policy, uintptr_t align)
{
  if (!fl) return NULL;
  
  fl_hdr_t* hdr = NULL;
  fl_hdr_t* prev_hdr = NULL;

  size_t aligned_size = align_size(size, 4);
  
  switch (policy) {
    case FIRST_SLOT:
    free_list_find_first(fl, aligned_size, &hdr, &prev_hdr);
    break;
    case BEST_SLOT:
    free_list_find_best(fl, aligned_size, &hdr, &prev_hdr);
    break;
    default:
    break;
  }

  if (!hdr) {
    flog(LOG_WARNING, "Free list allocation failed: not enough space");
    return NULL;
  }

  hdr->block_size = 0;

  size_t hdr_size = align_size(sizeof(fl_hdr_t), align);
  uintptr_t ptr_pos = (uintptr_t)hdr + (uintptr_t)hdr_size;

  // printf("fl_alloc ptr pos %llu\n", ptr_pos - (uintptr_t)fl->buf);

  void* ptr = (void*)align_ptr(ptr_pos, align);

  uintptr_t alloc_block_end = ptr_pos + (uintptr_t)aligned_size;
  uintptr_t aligned_block_end = align_ptr(alloc_block_end, align);

  uintptr_t space_left = (hdr->linked_hdr) ? 
    (uintptr_t)hdr->linked_hdr - aligned_block_end :
    (uintptr_t)(fl->buf + fl->size) - aligned_block_end;

  // Only create a new header if there's at least a number of bytes equal
  // to the given alignment left.
  if (space_left >= ((uintptr_t)hdr_size + align)) {
    fl_hdr_t* new_hdr = (fl_hdr_t*)(aligned_block_end);
    new_hdr->block_size = space_left - hdr_size;
    new_hdr->linked_hdr = hdr->linked_hdr;
    hdr->linked_hdr = new_hdr; 
  }

  return memset(ptr, 0, size);
}

void free_list_free(free_list_t* fl, void* element, size_t element_size)
{
  free_list_free_align(fl, element, element_size, DEFAULT_ALIGN);
}

void  free_list_free_align(free_list_t* fl, void* element, size_t element_size, uintptr_t align) {
  uintptr_t hdr_size = align_size(sizeof(fl_hdr_t), align);

  static int call = 1;

  if (!within_bounds(element, fl->buf, fl->size)) {
    flog(LOG_WARNING, "free_list_free: the element to be freed is not in the given buffer");
    return;
  }
  
  fl_hdr_t* hdr = (fl_hdr_t*)((uintptr_t)element - hdr_size);
  
  hdr->block_size += align_size(element_size, align);

  fl_hdr_t* linked_hdr = hdr->linked_hdr;

  // Try to splice the following block.
  if (linked_hdr && linked_hdr->block_size > 0) {
    hdr->block_size += hdr_size + linked_hdr->block_size;
    hdr->linked_hdr = linked_hdr->linked_hdr;
  }

  else printf("free_list_free #%d: foll hdr is occupied\n", call);

  // Try to splice into the previous block.
  fl_hdr_t* prev_hdr = NULL;
  fl_hdr_t* nhdr = fl_first_hdr(fl);

  while (nhdr) {
    if (nhdr >= hdr) break;

    prev_hdr = nhdr;
    nhdr = nhdr->linked_hdr;
  }
  
  if (prev_hdr) {
    printf("free_list_free #%d: prev hdr offset %llu size %llu\n", 
      call, (uintptr_t)prev_hdr - (uintptr_t)fl->buf, prev_hdr->block_size);

    if (prev_hdr->block_size > 0) {
      prev_hdr->block_size += hdr_size + hdr->block_size;
      prev_hdr->linked_hdr = hdr->linked_hdr;
    }

    else printf("free_list_free #%d: prev hdr is occupied\n", call);
  }

  ++call;
}

void free_list_free_all(free_list_t* fl)
{
  free_list_free_all_align(fl, DEFAULT_ALIGN);
}

void free_list_free_all_align(free_list_t* fl, uintptr_t align) { 
  fl_hdr_t* first_hdr = fl_first_hdr(fl);
  size_t hdr_size = (size_t)align_size(sizeof(fl_hdr_t), align);

  first_hdr->block_size = fl->size - hdr_size;
  first_hdr->linked_hdr = NULL;
}

void free_list_find_first(free_list_t* fl, size_t size, fl_hdr_t** found_hdr, fl_hdr_t** prev_hdr)
{
  fl_hdr_t* hdr = fl_first_hdr(fl);
  fl_hdr_t* phdr = NULL;

  int counter = 1;

  while (hdr) {
    size_t curr_size = fl_hdr_get_block_size(hdr);

    if (fl_hdr_get_block_size(hdr) >= size) {
      printf("find_first: header %d block size %llu\n", counter, curr_size);
      break;
    }

    phdr = hdr;
    hdr = hdr->linked_hdr;
    ++counter;
  }

  *found_hdr = hdr;
  *prev_hdr = phdr;
}

void free_list_find_best(free_list_t* fl, size_t size, fl_hdr_t** found_hdr, fl_hdr_t** prev_hdr)
{
  fl_hdr_t* first_hdr = fl_first_hdr(fl); // TODO: Remove (see below).
  fl_hdr_t* hdr = fl_first_hdr(fl);
  fl_hdr_t* phdr = NULL;
  size_t best_size = SIZE_MAX;

  int counter = 0;

  while (hdr) {
    size_t curr_size = fl_hdr_get_block_size(hdr);
    printf("find_best: header %d block size %llu\n", counter++, curr_size);

    if (curr_size >= size && curr_size < best_size) {
      *found_hdr = hdr;
      *prev_hdr = phdr;
      best_size = curr_size;
    }

    phdr = hdr;
    hdr = hdr->linked_hdr;
  }

  // TODO: Remove.
  if (*prev_hdr)
    printf("Free list found best at offset %lld with prev_hdr at offset %lld\n", (uintptr_t)*found_hdr - (uintptr_t)first_hdr, (uintptr_t)*prev_hdr - (uintptr_t)first_hdr);

  else 
    printf("Free list found best at offset %lld\n", (uintptr_t)*found_hdr - (uintptr_t)first_hdr);
}
