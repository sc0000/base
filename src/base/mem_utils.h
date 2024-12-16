#pragma once

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "base/log.h"

#ifndef DEFAULT_ALIGN
  #if __STDC_VERSION__ >= 201112L
    #define DEFAULT_ALIGN alignof(max_align_t)
  #else
    #define DEFAULT_ALIGN (sizeof(void*) * 2)
  #endif
#endif

#define VALUE(value, ...) value
#define SELECT_VALIDATION(_1, _2, validation, ...) validation

/// @brief Takes a pointer and optionally a value.
/// Returns the function if the pointer is null, returns the value if it was given, void if not.
#define VALIDATE_PTR(...) SELECT_VALIDATION(__VA_ARGS__, VALIDATE_PTR_VAL, VALIDATE_PTR_VOID)(__VA_ARGS__)

#define VALIDATE_PTR_VAL(ptr, ...)\
do {\
  if (!ptr) {\
    flog(LOG_WARNING, "%s(): %s invalid", __func__, #ptr);\
    return VALUE(__VA_ARGS__);\
  }\
} while(0);

#define VALIDATE_PTR_VOID(ptr, ...)\
do {\
  if (!ptr) {\
    flog(LOG_WARNING, "%s(): %s invalid", __func__, #ptr);\
    return;\
  }\
} while (0);

/// @brief ---INTERNAL FUNCTION---
/// Aligns a given pointer according to the given alignment size.
/// Will crash if the alignment is not a power of 2.
/// @return The aligned pointer.
uintptr_t align_ptr(uintptr_t ptr, uintptr_t align);

/// @brief ---INTERNAL FUNCTION---
/// Aligns a given size (usually a buffer size) according to 
/// the given alignment.
/// @return The aligned size as uintptr_t.
uintptr_t align_size(size_t size, uintptr_t align);

/// @brief ---INTERNAL FUNCTION---
/// Checks if a given memory address is a power of 2.
static inline bool is_pow2(uintptr_t p)
{
  uintptr_t mod = p & (p - 1);
  return mod == 0; 
}

/// @brief ---INTERNAL FUNCTION--- 
/// Checks if a given memory address is within a given buffer.
bool within_bounds(void* ptr, unsigned char* buf, size_t buf_size);
