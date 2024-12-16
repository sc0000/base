#include "base/allocators.h"
#include "base/log.h"
#include "base/mem_utils.h"

void* darray_init(size_t element_size, size_t num)
{
  uintptr_t hdr_size = align_size(sizeof(da_hdr_t), DEFAULT_ALIGN);
  
  size_t raw_size = hdr_size + element_size * num;
  void* raw = malloc(raw_size);

  if (!raw) {
    flog(LOG_ERROR, "darray_init(): buffer allocation failed");
    exit(EXIT_FAILURE);
  }

  da_hdr_t* hdr = (da_hdr_t*)raw;
  hdr->capacity = num;
  hdr->occupied = 0;
  hdr->element_size = element_size;

  void* buf = (void*)((uintptr_t)hdr + hdr_size);

  return buf;
}

void* darray_alloc(void* darray, size_t element_size, size_t num)
{
  if (!darray)
    darray = darray_init(element_size, num);

  da_hdr_t* hdr = darray_get_hdr(darray);
  uintptr_t hdr_size = align_size(sizeof(da_hdr_t), DEFAULT_ALIGN);
  size_t needed_size = (size_t)hdr->occupied + num;

  if (needed_size > hdr->capacity) {
    size_t double_cap = hdr->capacity * 2;
    size_t new_cap = (needed_size > double_cap) ? needed_size : double_cap;
    size_t new_size = (size_t)hdr_size + new_cap * element_size;

    void* new_raw = realloc(hdr, new_size);
    
    if (!new_raw) {
        flog(LOG_ERROR, "darray_alloc(): re-allocation failed");
        exit(EXIT_FAILURE);
    }

    hdr = (da_hdr_t*)new_raw;
    hdr->capacity = new_cap; 
  }

  hdr->occupied += num;

  return (void*)((uintptr_t)hdr + hdr_size);
}

void* darray_make_space(void* darray, size_t pos, size_t element_size, size_t num)
{
  darray = darray_alloc(darray, element_size, num);

  // darray_alloc() guarantees a valid darray (or crash).

  uintptr_t ptr = (uintptr_t)darray;
  da_hdr_t* hdr = darray_get_hdr(darray);

  uintptr_t mem_occ = (uintptr_t)(hdr->occupied * element_size);
  uintptr_t mem_left = (uintptr_t)(pos * element_size);
  uintptr_t mem_right = mem_occ - mem_left;
  uintptr_t old_pos = ptr + mem_left;
  uintptr_t offset = (uintptr_t)(element_size * num);
  uintptr_t new_pos = old_pos + offset;

  memcpy((void*)new_pos, (void*)old_pos, mem_right);

  return darray;
}

void darray_pop_last(void* darray)
{
  VALIDATE_PTR(darray);

  da_hdr_t* hdr = darray_get_hdr(darray);
  hdr->occupied -= 1;
}

void darray_pop_first(void* darray)
{
  VALIDATE_PTR(darray);

  da_hdr_t* hdr = darray_get_hdr(darray);
  uintptr_t second_element_ptr = (uintptr_t)darray + (uintptr_t)hdr->element_size;
  hdr->occupied -= 1;
  size_t size = hdr->occupied * hdr->element_size;

  memcpy(darray, (void*)second_element_ptr, size);
}

void darray_clear(void* darray)
{
  VALIDATE_PTR(darray);

  da_hdr_t* hdr = darray_get_hdr(darray);
  hdr->occupied = 0;
}

void darray_reset_size(void* darray, size_t size)
{
  VALIDATE_PTR(darray);

  da_hdr_t* hdr = darray_get_hdr(darray);

  if (size > hdr->occupied) {
    flog(LOG_WARNING, "darray_reset: size to reset to is larger than current size");
    return;
  }

  hdr->occupied = size;
}

size_t darray_size(void* darray)
{
  VALIDATE_PTR(darray, 0);

  da_hdr_t* hdr = darray_get_hdr(darray);
  return hdr->occupied;
}

size_t darray_capacity(void* darray)
{
  VALIDATE_PTR(darray, 0);

  da_hdr_t* hdr = darray_get_hdr(darray);
  return hdr->capacity;
}

void darray_reserve_base(void** darray, size_t new_cap)
{
  VALIDATE_PTR(*darray);

  da_hdr_t* hdr = darray_get_hdr(*darray);

  if (new_cap < hdr->capacity) return;

  uintptr_t hdr_size = align_size(sizeof(da_hdr_t), DEFAULT_ALIGN);
  size_t new_size = (size_t)hdr_size + new_cap * hdr->element_size;

  void* new_raw = realloc(hdr, new_size);
    
  VALIDATE_PTR(new_raw);

  hdr = (da_hdr_t*)new_raw;
  hdr->capacity = new_cap; 

  *darray = (void*)((uintptr_t)hdr + hdr_size);
}

void darray_shrink_to_fit_base(void** darray)
{
  VALIDATE_PTR(*darray);

  da_hdr_t* hdr = darray_get_hdr(*darray);
  uintptr_t hdr_size = align_size(sizeof(da_hdr_t), DEFAULT_ALIGN);

  size_t new_cap = hdr->occupied;
  size_t new_size = (size_t)hdr_size + hdr->element_size * new_cap; 
  void* new_raw = realloc(hdr, new_size);
  
  if (!new_raw) {
    flog(LOG_ERROR, "darray_shrink_to_fit: re-allocation failed");
    return;
  }

  hdr = (da_hdr_t*)new_raw;
  hdr->capacity = new_cap;

  *darray = (void*)((uintptr_t)hdr + hdr_size);
}

void arena_init(arena_t* a, void* buf, size_t size)
{
  VALIDATE_PTR(buf);

  a->size = size;
  a->curr_offset = 0;
  a->prev_offset = 0;
  a->buf = (unsigned char*)buf;
}

void* arena_alloc_align(arena_t* a, size_t size, uintptr_t align)
{
  VALIDATE_PTR(a, NULL);

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

void arena_resize_element_align(arena_t* a, void* element, size_t old_size, size_t new_size, uintptr_t align)
{
  VALIDATE_PTR(a);

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
  
  return ptr + padding;
}

void stack_init(stack_t* s, void* buf, size_t size)
{
  VALIDATE_PTR(buf);

  s->size = size;
  s->curr_offset = 0;
  s->curr_hdr = NULL;
  s->buf = (unsigned char*)buf;
}

void* stack_alloc_align(stack_t* s, size_t size, uintptr_t align)
{
  VALIDATE_PTR(s, NULL);

  uintptr_t curr_ptr = (uintptr_t)(s->buf + s->curr_offset);
  uintptr_t offset_ptr = align_ptr_hdr(curr_ptr, align, sizeof(hdr_t));
  offset_ptr -= (uintptr_t)s->buf;

  // ???
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

void stack_resize_element_align(stack_t* s, void* element, size_t old_size, size_t new_size, uintptr_t align)
{
  if (!element) {
    element = stack_alloc_align(s, new_size, align);
    return;
  }
  
  unsigned char* i = (unsigned char*)element;
  
  if (!within_bounds(i, s->buf, s->size)) {
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

void pool_init_align(pool_t* p, void* buf, size_t size, size_t slot_size, uintptr_t align)
{
  VALIDATE_PTR(buf);

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
  VALIDATE_PTR(p, NULL);

  hdr_t* hdr = p->curr_hdr;

  VALIDATE_PTR(hdr, NULL);

  p->curr_hdr = p->curr_hdr->linked_hdr;
  return memset(hdr, 0, p->slot_size);
}

void pool_free(pool_t* p, void* slot)
{
  VALIDATE_PTR(p);
  VALIDATE_PTR(slot);

  uintptr_t buf_int = (uintptr_t)p->buf;
  uintptr_t end_int = (uintptr_t)(p->buf + p->size);
  uintptr_t seg_int = (uintptr_t)slot;

  // ???
  if (seg_int < buf_int || seg_int >= end_int) {
    flog(LOG_WARNING, "Failed to free pool slot: Given address out of bounds");
    return;
  }

  hdr_t* hdr = (hdr_t*)slot;
  hdr->linked_hdr = p->curr_hdr;
  p->curr_hdr = hdr;
  slot = NULL;
}

void pool_free_all(pool_t* p)
{
  VALIDATE_PTR(p);

  size_t slot_count = p->size / p->slot_size;

  for (size_t i = 0; i < slot_count; ++i) {
    hdr_t* hdr = (hdr_t*)(p->buf + i * p->slot_size);
    hdr->linked_hdr = p->curr_hdr;
    p->curr_hdr = hdr;
  }
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
}

void* free_list_alloc_align(free_list_t* fl, size_t size, fl_policy policy, uintptr_t align)
{
  VALIDATE_PTR(fl, NULL);
  
  fl_hdr_t* hdr = NULL;
  fl_hdr_t* prev_hdr = NULL;

  size_t aligned_size = align_size(size, align);
  
  switch (policy) {
    case FIRST_SLOT:
    free_list_find_first(fl, aligned_size, &hdr, &prev_hdr);
    break;
    case BEST_SLOT:
    free_list_find_best(fl, aligned_size, &hdr, &prev_hdr);
    break;
    case NUM_POLICIES:
    break;
    default:
    break;
  }

  VALIDATE_PTR(hdr, NULL);

  hdr->block_size = 0;

  size_t hdr_size = align_size(sizeof(fl_hdr_t), align);
  uintptr_t ptr_pos = (uintptr_t)hdr + (uintptr_t)hdr_size;

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

void free_list_free_align(free_list_t* fl, void* element, size_t element_size, uintptr_t align) {
  uintptr_t hdr_size = align_size(sizeof(fl_hdr_t), align);

  VALIDATE_PTR(fl);

  if (!within_bounds(element, fl->buf, fl->size)) {
    flog(LOG_WARNING, "free_list_free: the element to be freed is not in the given buffer");
    return;
  }
  
  fl_hdr_t* hdr = (fl_hdr_t*)((uintptr_t)element - hdr_size);
  
  hdr->block_size += align_size(element_size, align);

  fl_hdr_t* linked_hdr = hdr->linked_hdr;

  // Try to merge the following block.
  if (linked_hdr && linked_hdr->block_size > 0) {
    hdr->block_size += hdr_size + linked_hdr->block_size;
    hdr->linked_hdr = linked_hdr->linked_hdr;
  }

  // Try to merge into the previous block.
  fl_hdr_t* prev_hdr = NULL;
  fl_hdr_t* nhdr = fl_first_hdr(fl);

  while (nhdr) {
    if (nhdr >= hdr) break;

    prev_hdr = nhdr;
    nhdr = nhdr->linked_hdr;
  }
  
  if (prev_hdr && prev_hdr->block_size > 0) {
    prev_hdr->block_size += hdr_size + hdr->block_size;
    prev_hdr->linked_hdr = hdr->linked_hdr;
  }
}

void free_list_free_all_align(free_list_t* fl, uintptr_t align)
{
  VALIDATE_PTR(fl);

  fl_hdr_t* first_hdr = fl_first_hdr(fl);
  size_t hdr_size = (size_t)align_size(sizeof(fl_hdr_t), align);

  first_hdr->block_size = fl->size - hdr_size;
  first_hdr->linked_hdr = NULL;
}

void free_list_find_first(free_list_t* fl, size_t size, fl_hdr_t** found_hdr, fl_hdr_t** prev_hdr)
{
  VALIDATE_PTR(fl);

  fl_hdr_t* hdr = fl_first_hdr(fl);
  fl_hdr_t* phdr = NULL;

  while (hdr) {
    size_t curr_size = hdr->block_size;

    if (curr_size >= size)
      break;

    phdr = hdr;
    hdr = hdr->linked_hdr;
  }

  *found_hdr = hdr;
  *prev_hdr = phdr;
}

void free_list_find_best(free_list_t* fl, size_t size, fl_hdr_t** found_hdr, fl_hdr_t** prev_hdr)
{
  VALIDATE_PTR(fl);

  fl_hdr_t* hdr = fl_first_hdr(fl);
  fl_hdr_t* phdr = NULL;
  size_t best_size = SIZE_MAX;

  while (hdr) {
    size_t curr_size = hdr->block_size;

    if (curr_size >= size && curr_size < best_size) {
      *found_hdr = hdr;
      *prev_hdr = phdr;
      best_size = curr_size;
    }

    phdr = hdr;
    hdr = hdr->linked_hdr;
  }
}
