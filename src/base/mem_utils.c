#include "base/mem_utils.h"

bool within_bounds(void* ptr, unsigned char* buf, size_t buf_size)
{
  VALIDATE_PTR(ptr, false);
  VALIDATE_PTR(buf, false);

  uintptr_t uptr = (uintptr_t)ptr;
  uintptr_t ubuf = (uintptr_t)buf;
  uintptr_t uend = (ubuf + (uintptr_t)buf_size);
  
  return (uptr > ubuf && uptr < uend);
}

uintptr_t align_ptr(uintptr_t ptr, uintptr_t align)
{
  if (!is_pow2(align)) {
    flog(LOG_ERROR, "align_ptr(): Given pointer alignment no power of 2");
    exit(EXIT_FAILURE);
  }

  uintptr_t mod = ptr & (align - 1);

  if (mod != 0)
    ptr += align - mod;

  return ptr;
}

uintptr_t align_size(size_t size, uintptr_t align)
{
  if (!is_pow2(align)) {
    flog(LOG_ERROR, "align_size(): Given alignment no power of 2");
    exit(EXIT_FAILURE);
  }

  uintptr_t size_ptr = (uintptr_t)size;
  uintptr_t mod = size_ptr & (align - 1);

  if (mod != 0)
    size_ptr += align - mod;

  return size_ptr;
}
