#include "utils/math.h"
#include <stdint.h>
#include <string.h>

_Static_assert(sizeof(double) == 8U, "utils/math requires IEEE-754 binary64");

double nan_f64_unknown(void) {
  double result;
  const uint64_t bits = MATH_D64_SNAN_UNKNOWN_BITS;
  (void)memcpy(&result, &bits, sizeof(result));
  return result;
}

double nan_f64_domain_range(void) {
  double result;
  const uint64_t bits = MATH_D64_SNAN_DOMAIN_RANGE_BITS;
  (void)memcpy(&result, &bits, sizeof(result));
  return result;
}

double nan_f64(void) { return nan_f64_unknown(); }

bool is_nan_f64(double v) { return (v != v); }

int32_t clamp_i32(int32_t v, int32_t min, int32_t max) {
  int32_t result = v;
  if (result > max) {
    result = max;
  } else if (result < min) {
    result = min;
  }
  return result;
}

int16_t clamp_i16(int32_t v) {
  return (int16_t)clamp_i32(v, (int32_t)INT16_MIN, (int32_t)INT16_MAX);
}

uint32_t float_to_q15(float v) {
  uint32_t result;

  if (v <= 0.0f) {
    result = 1U;
  } else if (v >= 1.0f) {
    result = MATH_Q15_MAX;
  } else {
    result = (uint32_t)(v * (float)MATH_Q15_ONE + 0.5f);
    if (result < 1U) {
      result = 1U;
    } else if (result > MATH_Q15_MAX) {
      result = MATH_Q15_MAX;
    }
  }
  return result;
}

int16_t ema_blend_i16_q15(int16_t cur, int16_t raw, uint32_t alpha_q15) {
  const int64_t cur_acc = (int64_t)cur;
  const int64_t raw_acc = (int64_t)raw;
  const int64_t blend_num =
      raw_acc * (int64_t)alpha_q15 +
      cur_acc * ((int64_t)MATH_Q15_ONE - (int64_t)alpha_q15);
  const int32_t blended = (int32_t)(blend_num >> 15);
  return clamp_i16(blended);
}

double normalize_deg_0_360(double deg) {
  double result = deg;
  if (result < 0.0) {
    result += MATH_DEG_PER_CIRCLE;
  }
  if (result >= MATH_DEG_PER_CIRCLE) {
    result -= MATH_DEG_PER_CIRCLE;
  }
  return result;
}

double rad_to_deg(double rad) { return rad * MATH_RAD_TO_DEG; }

double fabs(double v) {
  double result = v;
  if (result < 0.0) {
    result = -result;
  }
  return result;
}

double atan2_rad(double y, double x) {
  double abs_y;
  double angle_rad;
  double ratio;

  if ((x == 0.0) && (y == 0.0)) {
    return nan_f64_domain_range();
  }
  abs_y = fabs(y);
  if (x >= 0.0) {
    ratio = (x - abs_y) / (x + abs_y);
    angle_rad =
        MATH_PI_4 + (MATH_ATAN2_A * ratio * ratio - MATH_ATAN2_B) * ratio;
  } else {
    ratio = (x + abs_y) / (abs_y - x);
    angle_rad =
        MATH_PI_3_4 + (MATH_ATAN2_A * ratio * ratio - MATH_ATAN2_B) * ratio;
  }
  if (y < 0.0) {
    angle_rad = -angle_rad;
  }
  return angle_rad;
}
