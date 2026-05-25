#ifndef UTILS_MATH_H
#define UTILS_MATH_H

#include <stdint.h>

#define MATH_PI_4 (0.7853981633974483)
#define MATH_PI_3_4 (2.356194490192345)
#define MATH_ATAN2_A (0.1963)
#define MATH_ATAN2_B (0.9817)
#define MATH_PI (3.14159265358979323846)

/// @brief Clamp for int32_t
/// @param v Value to clamp
/// @param min Minimum value
/// @param max Maximum value
/// @return Clamped value
int32_t clamp_i32(int32_t v, int32_t min, int32_t max);

/// @brief Fast atan2 (radians) without libm; used for heading from smoothed
/// mag XY.
/// @param y Y coordinate
/// @param x X coordinate
/// @pre X and Y are not both zero
/// @return Angle in radians
double atan2_rad(double y, double x);

/// @brief Fabs (absolute value) without libm; used for heading from
/// smoothed mag XY.
/// @param v Value to get the absolute value of
/// @return Absolute value
double math_fabs(double v);

#endif