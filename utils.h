#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "defs.h"

uintptr_t align_fwd(uintptr_t ptr, size_t align);
bool is_pow2(uintptr_t p);
