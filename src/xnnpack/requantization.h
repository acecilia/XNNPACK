// Copyright (c) Facebook, Inc. and its affiliates.
// All rights reserved.
//
// Copyright 2019 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <math.h>

#include <fp16.h>

#include <xnnpack/common.h>
#include <xnnpack/math.h>
#include <xnnpack/params.h>


union xnn_qu8_requantization_params {
  struct {
    int32_t multiplier;
    int32_t remainder_mask;
    int32_t remainder_threshold;
    uint32_t shift;
    int32_t min_less_zero_point;
    int32_t max_less_zero_point;
    int32_t zero_point;
  } gemmlowp;
};

static inline void xnn_init_qu8_requantization_gemmlowp_params(
  union xnn_qu8_requantization_params params[XNN_MIN_ELEMENTS(1)],
  float scale,
  uint8_t zero_point,
  uint8_t min,
  uint8_t max)
{
  // Compute requantization parameters.
  assert(scale < 1.0f);
  assert(scale >= 0x1.0p-32f);
  const uint32_t scale_bits = fp32_to_bits(scale);

  // Multiplier is in [0x40000000, 0x7FFFFF80] range.
  const int32_t multiplier = (int32_t)(((scale_bits & UINT32_C(0x007FFFFF)) | UINT32_C(0x00800000)) << 7);
  assert(multiplier >= INT32_C(0x40000000));
  assert(multiplier <= INT32_C(0x7FFFFF80));

  // Shift is in [0, 31] range.
  const int32_t shift = 127 + 31 - 32 - (fp32_to_bits(scale) >> 23);
  assert(shift >= 0);
  assert(shift < 32);

  const uint32_t remainder_mask = (UINT32_C(1) << shift) - UINT32_C(1);
  const uint32_t remainder_threshold = remainder_mask >> 1;
  params->gemmlowp.multiplier = multiplier;
  params->gemmlowp.remainder_mask = (int32_t) remainder_mask;
  params->gemmlowp.remainder_threshold = (int32_t) remainder_threshold;
  params->gemmlowp.shift = (uint32_t) shift;
  params->gemmlowp.min_less_zero_point = (int32_t) (uint32_t) min - (int32_t) (uint32_t) zero_point;
  params->gemmlowp.max_less_zero_point = (int32_t) (uint32_t) max - (int32_t) (uint32_t) zero_point;
  params->gemmlowp.zero_point = (int32_t) (uint32_t) zero_point;
}

static inline uint8_t xnn_qu8_requantize_gemmlowp(
  int32_t n,
  union xnn_qu8_requantization_params params[XNN_MIN_ELEMENTS(1)])
{
  const int64_t product = (int64_t) n * (int64_t) params->gemmlowp.multiplier;
  const int32_t q31product = (int32_t) (uint32_t) ((uint64_t) (product + INT64_C(0x40000000)) >> 31);
  const int32_t remainder = (q31product & params->gemmlowp.remainder_mask) - (int32_t) (n < 0);
  n = asr_s32(q31product, params->gemmlowp.shift) + (int32_t) (remainder > params->gemmlowp.remainder_threshold);
  n = math_max_s32(n, params->gemmlowp.min_less_zero_point);
  n = math_min_s32(n, params->gemmlowp.max_less_zero_point);
  return (uint8_t) (n + params->gemmlowp.zero_point);
}


union xnn_qs8_requantization_params {
  struct {
    int32_t multiplier;
    int32_t remainder_mask;
    int32_t remainder_threshold;
    uint32_t shift;
    int32_t min_less_zero_point;
    int32_t max_less_zero_point;
    int32_t zero_point;
  } gemmlowp;
  struct {
    float scale;
    long min_less_zero_point;
    long max_less_zero_point;
    int32_t zero_point;
  } fp32;
};

typedef void (*xnn_init_qs8_requantization_params_fn)(
  union xnn_qs8_requantization_params params[XNN_MIN_ELEMENTS(1)],
  float scale,
  int8_t output_zero_point,
  int8_t output_min,
  int8_t output_max);

typedef int8_t (*xnn_qs8_requantize_fn)(
  int32_t n,
  union xnn_qs8_requantization_params params[XNN_MIN_ELEMENTS(1)]);

static inline void xnn_init_qs8_requantization_gemmlowp_params(
  union xnn_qs8_requantization_params params[XNN_MIN_ELEMENTS(1)],
  float scale,
  int8_t zero_point,
  int8_t min,
  int8_t max)
{
  // Compute requantization parameters.
  assert(scale < 1.0f);
  assert(scale >= 0x1.0p-32f);
  const uint32_t scale_bits = fp32_to_bits(scale);

  // Multiplier is in [0x40000000, 0x7FFFFF80] range.
  const int32_t multiplier = (int32_t)(((scale_bits & UINT32_C(0x007FFFFF)) | UINT32_C(0x00800000)) << 7);
  assert(multiplier >= INT32_C(0x40000000));
  assert(multiplier <= INT32_C(0x7FFFFF80));

  // Shift is in [0, 31] range.
  const int32_t shift = 127 + 31 - 32 - (fp32_to_bits(scale) >> 23);
  assert(shift >= 0);
  assert(shift < 32);

  const uint32_t remainder_mask = (UINT32_C(1) << shift) - UINT32_C(1);
  const uint32_t remainder_threshold = remainder_mask >> 1;
  params->gemmlowp.multiplier = multiplier;
  params->gemmlowp.remainder_mask = (int32_t) remainder_mask;
  params->gemmlowp.remainder_threshold = (int32_t) remainder_threshold;
  params->gemmlowp.shift = (uint32_t) shift;
  params->gemmlowp.min_less_zero_point = (int32_t) min - (int32_t) zero_point;
  params->gemmlowp.max_less_zero_point = (int32_t) max - (int32_t) zero_point;
  params->gemmlowp.zero_point = (int32_t) zero_point;
}

static inline int8_t xnn_qs8_requantize_gemmlowp(
  int32_t n,
  union xnn_qs8_requantization_params params[XNN_MIN_ELEMENTS(1)])
{
  const int64_t product = (int64_t) n * (int64_t) params->gemmlowp.multiplier;
  const int32_t q31product = (int32_t) (uint32_t) ((uint64_t) (product + INT64_C(0x40000000)) >> 31);
  const int32_t remainder = (q31product & params->gemmlowp.remainder_mask) - (int32_t) (n < 0);
  n = asr_s32(q31product, params->gemmlowp.shift) + (int32_t) (remainder > params->gemmlowp.remainder_threshold);
  n = math_max_s32(n, params->gemmlowp.min_less_zero_point);
  n = math_min_s32(n, params->gemmlowp.max_less_zero_point);
  return (int8_t) (n + params->gemmlowp.zero_point);
}

static inline void xnn_init_qs8_requantization_fp32_params(
  union xnn_qs8_requantization_params params[XNN_MIN_ELEMENTS(1)],
  float scale,
  int8_t zero_point,
  int8_t min,
  int8_t max)
{
  // Validate requantization parameters.
  assert(scale < 1.0f);
  assert(scale >= 0x1.0p-32f);

  params->fp32.scale = scale;
  params->fp32.min_less_zero_point = (long) ((int32_t) min - (int32_t) zero_point);
  params->fp32.max_less_zero_point = (long) ((int32_t) max - (int32_t) zero_point);
  params->fp32.zero_point = (int32_t) zero_point;
}

static inline int8_t xnn_qs8_requantize_fp32(
  int32_t n,
  union xnn_qs8_requantization_params params[XNN_MIN_ELEMENTS(1)])
{
  const float scaled_n = (float) n * params->fp32.scale;
  long rounded_n = lrintf(scaled_n);
  rounded_n =
    XNN_UNPREDICTABLE(rounded_n < params->fp32.min_less_zero_point) ? params->fp32.min_less_zero_point : rounded_n;
  rounded_n =
    XNN_UNPREDICTABLE(rounded_n > params->fp32.max_less_zero_point) ? params->fp32.max_less_zero_point : rounded_n;
  return (int8_t) ((int32_t) rounded_n + params->fp32.zero_point);
}

inline static uint8_t xnn_qu8_requantize_rndna(
  int32_t value,
  float scale,
  uint8_t zero_point,
  uint8_t qmin,
  uint8_t qmax)
{
  assert(scale < 1.0f);
  assert(scale >= 0x1.0p-32f);

  const uint32_t scale_bits = fp32_to_bits(scale);
  const uint32_t multiplier = (scale_bits & UINT32_C(0x007FFFFF)) | UINT32_C(0x00800000);
  const uint32_t shift = 127 + 23 - (scale_bits >> 23);
  assert(shift >= 24);
  assert(shift < 56);

  // Compute absolute value of input as unsigned 32-bit int.
  // All further computations will work with unsigned values to avoid undefined behaviour on signed operations.
  const uint32_t abs_value = (value >= 0) ? (uint32_t) value : -(uint32_t) value;

  // Compute full 64-bit product of 32-bit factors
  const uint64_t product = (uint64_t) abs_value * (uint64_t) multiplier;

  // Shift the full 64-bit product right with rounding.
  // Rounding is performed towards closest integer, with midpoints rounded up (same as away from zero).
  const uint64_t rounding = UINT64_C(1) << (shift - 1);
  const uint32_t abs_scaled_value = (uint32_t) ((product + rounding) >> shift);

  // Copy the sign of input to scaled absolute input value.
  const int32_t scaled_value = (int32_t) (value >= 0 ? abs_scaled_value : -abs_scaled_value);

  // Clamp scaled value with zero point between smin and smax.
  int32_t clamped_value = scaled_value;
  const int32_t smin = (int32_t) (uint32_t) qmin - (int32_t) (uint32_t) zero_point;
  if (clamped_value < smin) {
    clamped_value = smin;
  }
  const int32_t smax = (int32_t) (uint32_t) qmax - (int32_t) (uint32_t) zero_point;
  if (clamped_value > smax) {
    clamped_value = smax;
  }

  // Add zero point to clamped value.
  const int32_t biased_value = clamped_value + (int32_t) (uint32_t) zero_point;

  return biased_value;
}

inline static int8_t xnn_qs8_requantize_rndna(
  int32_t value,
  float scale,
  int8_t zero_point,
  int8_t qmin,
  int8_t qmax)
{
  assert(scale < 1.0f);
  assert(scale >= 0x1.0p-32f);

  const uint32_t scale_bits = fp32_to_bits(scale);
  const uint32_t multiplier = (scale_bits & UINT32_C(0x007FFFFF)) | UINT32_C(0x00800000);
  const uint32_t shift = 127 + 23 - (scale_bits >> 23);
  assert(shift >= 24);
  assert(shift < 56);

  // Compute absolute value of input as unsigned 32-bit int.
  // All further computations will work with unsigned values to avoid undefined behaviour on signed operations.
  const uint32_t abs_value = (value >= 0) ? (uint32_t) value : -(uint32_t) value;

  // Compute full 64-bit product of 32-bit factors
  const uint64_t product = (uint64_t) abs_value * (uint64_t) multiplier;

  // Shift the full 64-bit product right with rounding.
  // Rounding is performed towards closest integer, with midpoints rounded up (same as away from zero).
  const uint64_t rounding = UINT64_C(1) << (shift - 1);
  const uint32_t abs_scaled_value = (uint32_t) ((product + rounding) >> shift);

  // Copy the sign of input to scaled absolute input value.
  const int32_t scaled_value = (int32_t) (value >= 0 ? abs_scaled_value : -abs_scaled_value);

  // Clamp scaled value with zero point between smin and smax.
  int32_t clamped_value = scaled_value;
  const int32_t smin = (int32_t) qmin - (int32_t) zero_point;
  if (clamped_value < smin) {
    clamped_value = smin;
  }
  const int32_t smax = (int32_t) qmax - (int32_t) zero_point;
  if (clamped_value > smax) {
    clamped_value = smax;
  }

  // Add zero point to clamped value.
  const int32_t biased_value = clamped_value + (int32_t) zero_point;

  return biased_value;
}

static inline uint8_t xnn_qu8_quantize_avgpool(
  int32_t n,
  union xnn_qu8_avgpool_params params)
{
  const int64_t product = (int64_t) n * (int64_t) params.scalar.multiplier;
  const int64_t adjusted_product = product - (int64_t) (n < 0);

  n = (int32_t) asr_s64(adjusted_product + params.scalar.rounding, params.scalar.right_shift);
  if (n < params.scalar.output_min_less_zero_point) {
    n = params.scalar.output_min_less_zero_point;
  }
  if (n > params.scalar.output_max_less_zero_point) {
    n = params.scalar.output_max_less_zero_point;
  }

  return (uint8_t) (n + params.scalar.output_zero_point);
}

static inline int8_t xnn_qs8_quantize_avgpool(
  int32_t n,
  union xnn_qs8_avgpool_params params)
{
  const int64_t product = (int64_t) n * (int64_t) params.scalar.multiplier;
  const int64_t adjusted_product = product - (int64_t) (n < 0);

  n = (int32_t) asr_s64(adjusted_product + params.scalar.rounding, params.scalar.shift);
  if (n < params.scalar.output_min_less_zero_point) {
    n = params.scalar.output_min_less_zero_point;
  }
  if (n > params.scalar.output_max_less_zero_point) {
    n = params.scalar.output_max_less_zero_point;
  }

  return (int8_t) (n + params.scalar.output_zero_point);
}

static inline uint8_t xnn_qu8_quantize_add(
  uint8_t a, uint8_t b,
  union xnn_qu8_add_params params)
{
  // Multiply by factors and accumulate products.
  int32_t acc = params.scalar.zero_point_product +
    (int32_t) ((uint32_t) a * params.scalar.a_multiplier) +
    (int32_t) ((uint32_t) b * params.scalar.b_multiplier);

  // Shift right and round.
  const int32_t rem = (acc & params.scalar.remainder_mask) - (int32_t) (acc < 0);
  acc = asr_s32(acc, params.scalar.shift) + (int32_t) (rem > params.scalar.remainder_threshold);

  // Clamp and add output zero point.
  int32_t y = acc + params.scalar.y_zero_point;
  if (y >= params.scalar.y_max) {
    y = params.scalar.y_max;
  }
  if (y <= params.scalar.y_min) {
    y = params.scalar.y_min;
  }
  return (uint8_t) y;
}

static inline int8_t xnn_qs8_quantize_add(
  int8_t x, int8_t y,
  union xnn_qs8_add_params params)
{
  // Multiply by factors and accumulate products.
  int32_t acc = params.scalar.zero_point_product +
    (int32_t) ((int32_t) x * params.scalar.x_multiplier) +
    (int32_t) ((int32_t) y * params.scalar.y_multiplier);

  // Shift right and round.
  const int32_t rem = (acc & params.scalar.remainder_mask) - (int32_t) (acc < 0);
  acc = asr_s32(acc, params.scalar.shift) + (int32_t) (rem > params.scalar.remainder_threshold);

  // Clamp and add output zero point.
  int32_t out = acc + params.scalar.output_zero_point;
  if (out >= params.scalar.output_max) {
    out = params.scalar.output_max;
  }
  if (out <= params.scalar.output_min) {
    out = params.scalar.output_min;
  }
  return (int8_t) out;
}
