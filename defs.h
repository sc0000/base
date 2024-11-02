#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef int8_t        i8;
typedef int16_t       i16;
typedef int32_t       i32;
typedef int64_t       i64;
typedef uint8_t       u8;
typedef uint16_t      u16;
typedef uint32_t      u32;
typedef uint64_t      u64;
typedef size_t        usize;

typedef uintptr_t     uptr;

typedef float         f32;
typedef double        f64;

typedef unsigned char uchar;


// #define min(a, b) ({ __typeof__ a _a = (a), _b = (b); _a < _b ? _a : _b; })
// #define max(a, b) ({ __typeof__ a _a = (a), _b = (b); _a > _b ? _a : _b; })
