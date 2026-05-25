#include "utils/math.h"
#include <stdint.h>

int32_t clamp_i32(int32_t v, int32_t min, int32_t max) {
  int32_t result = v;
  if (result > max) {
    result = max;
  } else if (result < min) {
    result = min;
  }
  return result;
}

double math_fabs(double v) {
  double out = v;
  if (out < 0.0) {
    out = -out;
  }
  return out;
}

double atan2_rad(double y, double x) {
  double abs_y;
  double angle_rad;
  double r;
  abs_y = math_fabs(y);
  if (x >= 0.0) {
    r = (x - abs_y) / (x + abs_y);
    angle_rad = MATH_PI_4 + (MATH_ATAN2_A * r * r - MATH_ATAN2_B) * r;
  } else {
    r = (x + abs_y) / (abs_y - x);
    angle_rad = MATH_PI_3_4 + (MATH_ATAN2_A * r * r - MATH_ATAN2_B) * r;
  }
  if (y < 0.0) {
    angle_rad = -angle_rad;
  }
  return angle_rad;
}
