#include "utils.h"

#include <assert.h>
#include <stdio.h>

bool is_pow2(uintptr_t p) {
  uintptr_t mod = p & (p - 1);
  return mod == 0; 
}

uintptr_t align_fwd(uintptr_t ptr, uintptr_t align) {
  assert(is_pow2(align));

  uintptr_t mod = ptr & (align - 1);

  if (mod != 0)
    ptr += align - mod;

  return ptr;
}
