/**
 * utils/math suite — no libm; golden expected values for atan2_rad.
 */

#include "test_runner.h"
#include "utils/math.h"
#include <stdint.h>
#include <string.h>

#define UTILS_MATH_SUITE "utils_math"
#define ATAN2_TOL_RAD (0.02)

typedef struct {
  double y;
  double x;
  double expected_rad;
} atan2_golden_t;

static int assert_near(double a, double b, double eps) {
  double diff = fabs(a - b);
  return diff <= eps;
}

#define ASSERT_NEAR(a, b, eps) ASSERT(assert_near((a), (b), (eps)))

static int assert_f64_has_bits(double v, uint64_t expected_bits) {
  uint64_t actual_bits;
  (void)memcpy(&actual_bits, &v, sizeof(actual_bits));
  return actual_bits == expected_bits;
}

#define ASSERT_F64_BITS(v, bits) ASSERT(assert_f64_has_bits((v), (bits)))

TEST(clamp_i32_in_range_unchanged) {
  ASSERT_EQ_I32(clamp_i32(5, 0, 10), 5);
  ASSERT_EQ_I32(clamp_i32(-3, -10, 10), -3);
  ASSERT_EQ_I32(clamp_i32(0, -1, 1), 0);
  return 0;
}

TEST(clamp_i32_at_bounds) {
  ASSERT_EQ_I32(clamp_i32(10, 0, 10), 10);
  ASSERT_EQ_I32(clamp_i32(0, 0, 10), 0);
  ASSERT_EQ_I32(clamp_i32(-10, -10, 10), -10);
  return 0;
}

TEST(clamp_i32_above_max) {
  ASSERT_EQ_I32(clamp_i32(11, 0, 10), 10);
  ASSERT_EQ_I32(clamp_i32(1000, -100, 100), 100);
  return 0;
}

TEST(clamp_i32_below_min) {
  ASSERT_EQ_I32(clamp_i32(-1, 0, 10), 0);
  ASSERT_EQ_I32(clamp_i32(-1000, -100, 100), -100);
  return 0;
}

TEST(clamp_i16_limits) {
  ASSERT_EQ_I32(clamp_i16(40000), 32767);
  ASSERT_EQ_I32(clamp_i16(-40000), -32768);
  ASSERT_EQ_I32(clamp_i16(100), 100);
  return 0;
}

TEST(fabs_positive_and_zero) {
  ASSERT_NEAR(fabs(3.5), 3.5, 1e-12);
  ASSERT_NEAR(fabs(0.0), 0.0, 1e-12);
  return 0;
}

TEST(fabs_negative) {
  ASSERT_NEAR(fabs(-3.5), 3.5, 1e-12);
  ASSERT_NEAR(fabs(-0.0), 0.0, 1e-12);
  return 0;
}

TEST(float_to_q15) {
  ASSERT_EQ_I32(float_to_q15(0.0f), 1);
  ASSERT_EQ_I32(float_to_q15(-1.0f), 1);
  ASSERT_EQ_I32(float_to_q15(1.0f), (int32_t)MATH_Q15_MAX);
  ASSERT_EQ_I32(float_to_q15(0.5f), 16384);
  return 0;
}

TEST(ema_blend_i16_q15_extremes) {
  /* Q15 max alpha ≈ raw; min alpha ≈ cur (off-by-one in Q15_ONE). */
  ASSERT_EQ_I32(ema_blend_i16_q15(0, 1000, MATH_Q15_MAX), 999);
  ASSERT_EQ_I32(ema_blend_i16_q15(1000, 0, 1), 999);
  return 0;
}

TEST(normalize_deg_0_360) {
  ASSERT_NEAR(normalize_deg_0_360(0.0), 0.0, 1e-9);
  ASSERT_NEAR(normalize_deg_0_360(-90.0), 270.0, 1e-9);
  ASSERT_NEAR(normalize_deg_0_360(400.0), 40.0, 1e-9);
  ASSERT_NEAR(normalize_deg_0_360(360.0), 0.0, 1e-9);
  return 0;
}

TEST(rad_to_deg_quarter_turn) {
  ASSERT_NEAR(rad_to_deg(MATH_PI / 2.0), 90.0, 1e-6);
  return 0;
}

TEST(nan_f64_is_detected) {
  ASSERT(is_nan_f64(nan_f64_unknown()));
  ASSERT(is_nan_f64(nan_f64_domain_range()));
  ASSERT(is_nan_f64(nan_f64()));
  ASSERT(!is_nan_f64(0.0));
  ASSERT(!is_nan_f64(1.5));
  return 0;
}

TEST(nan_f64_matches_bit_defines) {
  ASSERT_F64_BITS(nan_f64_unknown(), MATH_D64_SNAN_UNKNOWN_BITS);
  ASSERT_F64_BITS(nan_f64_domain_range(), MATH_D64_SNAN_DOMAIN_RANGE_BITS);
  ASSERT_F64_BITS(nan_f64(), MATH_D64_SNAN_UNKNOWN_BITS);
  return 0;
}

TEST(atan2_rad_zero_vector_is_domain_nan) {
  const double angle = atan2_rad(0.0, 0.0);
  ASSERT(is_nan_f64(angle));
  ASSERT_F64_BITS(angle, MATH_D64_SNAN_DOMAIN_RANGE_BITS);
  return 0;
}

TEST(atan2_rad_cardinal_directions) {
  ASSERT_NEAR(atan2_rad(0.0, 1.0), 0.0, ATAN2_TOL_RAD);
  ASSERT_NEAR(atan2_rad(1.0, 0.0), MATH_PI / 2.0, ATAN2_TOL_RAD);
  ASSERT_NEAR(atan2_rad(0.0, -1.0), MATH_PI, ATAN2_TOL_RAD);
  ASSERT_NEAR(atan2_rad(-1.0, 0.0), -MATH_PI / 2.0, ATAN2_TOL_RAD);
  return 0;
}

TEST(atan2_rad_golden_samples) {
  static const atan2_golden_t goldens[] = {
      {1.0, 1.0, 0.78539816339744828},
      {1.0, -1.0, 2.35619449019234484},
      {-1.0, 1.0, -0.78539816339744828},
      {-1.0, -1.0, -2.35619449019234484},
      {3.0, 4.0, 0.64350110879328437},
      {-3.0, 4.0, -0.64350110879328437},
      {100.0, 1.0, 1.56079666010823148},
      {1.0, 100.0, 0.00999966668666524},
      {-50.0, 2.0, -1.53081763967160667},
  };
  size_t i;

  for (i = 0; i < sizeof(goldens) / sizeof(goldens[0]); i++) {
    const double actual =
        atan2_rad(goldens[i].y, goldens[i].x);
    ASSERT_NEAR(actual, goldens[i].expected_rad, ATAN2_TOL_RAD);
  }
  return 0;
}

TEST(atan2_rad_heading_quadrants) {
  ASSERT_NEAR(rad_to_deg(atan2_rad(0.0, 100.0)), 0.0, 1.0);
  ASSERT_NEAR(rad_to_deg(atan2_rad(100.0, 0.0)), 90.0, 1.0);
  ASSERT_NEAR(rad_to_deg(atan2_rad(0.0, -100.0)), 180.0, 1.0);
  ASSERT_NEAR(rad_to_deg(atan2_rad(-100.0, 0.0)), -90.0, 1.0);
  return 0;
}

void utils_math_register_tests(test_registry_t *reg) {
  test_registry_add(reg, UTILS_MATH_SUITE, "clamp_i32_in_range_unchanged",
                  test_clamp_i32_in_range_unchanged);
  test_registry_add(reg, UTILS_MATH_SUITE, "clamp_i32_at_bounds",
                  test_clamp_i32_at_bounds);
  test_registry_add(reg, UTILS_MATH_SUITE, "clamp_i32_above_max",
                  test_clamp_i32_above_max);
  test_registry_add(reg, UTILS_MATH_SUITE, "clamp_i32_below_min",
                  test_clamp_i32_below_min);
  test_registry_add(reg, UTILS_MATH_SUITE, "clamp_i16_limits",
                  test_clamp_i16_limits);
  test_registry_add(reg, UTILS_MATH_SUITE, "fabs_positive_and_zero",
                  test_fabs_positive_and_zero);
  test_registry_add(reg, UTILS_MATH_SUITE, "fabs_negative", test_fabs_negative);
  test_registry_add(reg, UTILS_MATH_SUITE, "float_to_q15", test_float_to_q15);
  test_registry_add(reg, UTILS_MATH_SUITE, "ema_blend_i16_q15_extremes",
                  test_ema_blend_i16_q15_extremes);
  test_registry_add(reg, UTILS_MATH_SUITE, "normalize_deg_0_360",
                  test_normalize_deg_0_360);
  test_registry_add(reg, UTILS_MATH_SUITE, "rad_to_deg_quarter_turn",
                  test_rad_to_deg_quarter_turn);
  test_registry_add(reg, UTILS_MATH_SUITE, "nan_f64_is_detected",
                  test_nan_f64_is_detected);
  test_registry_add(reg, UTILS_MATH_SUITE, "nan_f64_matches_bit_defines",
                  test_nan_f64_matches_bit_defines);
  test_registry_add(reg, UTILS_MATH_SUITE, "atan2_rad_zero_vector_is_domain_nan",
                  test_atan2_rad_zero_vector_is_domain_nan);
  test_registry_add(reg, UTILS_MATH_SUITE, "atan2_rad_cardinal_directions",
                  test_atan2_rad_cardinal_directions);
  test_registry_add(reg, UTILS_MATH_SUITE, "atan2_rad_golden_samples",
                  test_atan2_rad_golden_samples);
  test_registry_add(reg, UTILS_MATH_SUITE, "atan2_rad_heading_quadrants",
                  test_atan2_rad_heading_quadrants);
}
