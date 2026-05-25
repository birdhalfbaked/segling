#ifndef UTILS_MATH_H
#define UTILS_MATH_H

#include <stdbool.h>
#include <stdint.h>

/* Requires IEC 60559 / IEEE-754 binary64 (little-endian for MATH_D64_*_BITS). */

/* Signaling NaN payloads (fraction MSB = 0); internal only — never publish on wire. */
#define MATH_D64_SNAN_UNKNOWN_BITS (0x7FF0000000000001ULL)
#define MATH_D64_SNAN_DOMAIN_RANGE_BITS (0x7FF0000000000002ULL)

#define MATH_PI_4 (0.7853981633974483)
#define MATH_PI_3_4 (2.356194490192345)
#define MATH_ATAN2_A (0.1963)
#define MATH_ATAN2_B (0.9817)
#define MATH_PI (3.14159265358979323846)
#define MATH_RAD_TO_DEG (180.0 / MATH_PI)
#define MATH_DEG_PER_CIRCLE (360.0)

#define MATH_Q15_ONE (32768U)
#define MATH_Q15_MAX (32767U)

/// @brief Clamp for int32_t
/// @param v Value to clamp
/// @param min Minimum value
/// @param max Maximum value
/// @return Clamped value
int32_t clamp_i32(int32_t v, int32_t min, int32_t max);

/// @brief Clamp int32 to int16 range and truncate.
int16_t clamp_i16(int32_t v);

/// @brief Map float in (0,1) to Q15; 0 maps to 1, 1 maps to MATH_Q15_MAX.
uint32_t float_to_q15(float v);

/// @brief Q15 EMA: cur + alpha * (raw - cur) with alpha in [1, MATH_Q15_MAX].
int16_t ema_blend_i16_q15(int16_t cur, int16_t raw, uint32_t alpha_q15);

/// @brief Wrap degrees to [0, 360).
double normalize_deg_0_360(double deg);

/// @brief Radians to degrees without libm.
double rad_to_deg(double rad);

/// @brief Fast atan2 (radians) without libm.
/// @param y Y coordinate
/// @param x X coordinate
/// @return Angle in radians, or MATH_D64_SNAN_DOMAIN_RANGE_BITS NaN when x and y are both zero.
double atan2_rad(double y, double x);

/// @brief Absolute value of double without libm (avoid including math.h in the same TU).
double fabs(double v);

/// @brief Signaling NaN from MATH_D64_SNAN_UNKNOWN_BITS.
double nan_f64_unknown(void);

/// @brief Signaling NaN from MATH_D64_SNAN_DOMAIN_RANGE_BITS.
double nan_f64_domain_range(void);

/// @brief Same as nan_f64_unknown().
double nan_f64(void);

/// @brief True when v is NaN (IEEE-754: v != v).
bool is_nan_f64(double v);

#endif
