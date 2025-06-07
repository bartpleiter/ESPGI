#pragma once
#include <Arduino.h>
#include "config.h"

// Fixed-point multiplication
static inline int32_t fixed_mul(int32_t a, int32_t b)
{
  int64_t temp = (int64_t)a * b;
  return (int32_t)(temp >> FP_SHIFT);
}

// Fixed-point division with overflow protection
static inline int32_t fixed_div(int32_t a, int32_t b)
{
  if (b == 0)
    return 0; // Avoid division by zero
  int64_t temp_a = (int64_t)a << FP_SHIFT;
  return (int32_t)(temp_a / b);
}
