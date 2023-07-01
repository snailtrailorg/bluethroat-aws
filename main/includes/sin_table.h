#pragma once

#include <stdint.h>

#ifdef __CPLUSPLUS__
extern "C" {
#endif

#define SIN_TABLE_DATA_COUNT            (30030)

typedef int16_t sin_value_t;

extern const sin_value_t sin_table[];

#ifdef __CPLUSPLUS__
}
#endif
