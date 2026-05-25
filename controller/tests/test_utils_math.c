/**
 * utils/math suite — clamp_i32, math_fabs, atan2_rad (reference vs libm).
 */

#include "test_runner.h"
#include "utils/math.h"
#include <math.h>
#include <stdint.h>

#define UTILS_MATH_SUITE "utils_math"
#define ATAN2_TOL_RAD (0.02)

static int assert_near(double a, double b, double eps) {
  double diff = a - b;
  if (diff < 0.0) {
    diff = -diff;
  }
  return diff <= eps;
}

#define ASSERT_NEAR(a, b, eps) ASSERT(assert_near((a), (b), (eps)))

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

TEST(clamp_i32_symmetric_range) {
  ASSERT_EQ_I32(clamp_i32(500, -32768, 32767), 500);
  ASSERT_EQ_I32(clamp_i32(40000, -32768, 32767), 32767);
  ASSERT_EQ_I32(clamp_i32(-40000, -32768, 32767), -32768);
  return 0;
}

TEST(math_fabs_positive_and_zero) {
  ASSERT_NEAR(math_fabs(3.5), 3.5, 1e-12);
  ASSERT_NEAR(math_fabs(0.0), 0.0, 1e-12);
  return 0;
}

TEST(math_fabs_negative) {
  ASSERT_NEAR(math_fabs(-3.5), 3.5, 1e-12);
  ASSERT_NEAR(math_fabs(-0.0), 0.0, 1e-12);
  return 0;
}

TEST(atan2_rad_cardinal_directions) {
  ASSERT_NEAR(atan2_rad(0.0, 1.0), 0.0, ATAN2_TOL_RAD);
  ASSERT_NEAR(atan2_rad(1.0, 0.0), MATH_PI / 2.0, ATAN2_TOL_RAD);
  ASSERT_NEAR(atan2_rad(0.0, -1.0), MATH_PI, ATAN2_TOL_RAD);
  ASSERT_NEAR(atan2_rad(-1.0, 0.0), -MATH_PI / 2.0, ATAN2_TOL_RAD);
  return 0;
}

TEST(atan2_rad_matches_libm_samples) {
  const double cases[][2] = {
      {1.0, 1.0},   {1.0, -1.0},  {-1.0, 1.0},  {-1.0, -1.0},
      {3.0, 4.0},   {-3.0, 4.0},  {100.0, 1.0}, {1.0, 100.0},
      {-50.0, 2.0},
  };
  size_t i;

  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    const double y = cases[i][0];
    const double x = cases[i][1];
    const double expected = atan2(y, x);
    const double actual = atan2_rad(y, x);
    ASSERT_NEAR(actual, expected, ATAN2_TOL_RAD);
  }
  return 0;
}

TEST(atan2_rad_heading_quadrants) {
  /* Magnetometer-style headings: atan2_rad(my, mx), degrees ~ atan2 * 180/pi */
  ASSERT_NEAR(atan2_rad(0.0, 100.0) * (180.0 / MATH_PI), 0.0, 1.0);
  ASSERT_NEAR(atan2_rad(100.0, 0.0) * (180.0 / MATH_PI), 90.0, 1.0);
  ASSERT_NEAR(atan2_rad(0.0, -100.0) * (180.0 / MATH_PI), 180.0, 1.0);
  ASSERT_NEAR(atan2_rad(-100.0, 0.0) * (180.0 / MATH_PI), -90.0, 1.0);
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
  test_registry_add(reg, UTILS_MATH_SUITE, "clamp_i32_symmetric_range",
                  test_clamp_i32_symmetric_range);
  test_registry_add(reg, UTILS_MATH_SUITE, "math_fabs_positive_and_zero",
                  test_math_fabs_positive_and_zero);
  test_registry_add(reg, UTILS_MATH_SUITE, "math_fabs_negative",
                  test_math_fabs_negative);
  test_registry_add(reg, UTILS_MATH_SUITE, "atan2_rad_cardinal_directions",
                  test_atan2_rad_cardinal_directions);
  test_registry_add(reg, UTILS_MATH_SUITE, "atan2_rad_matches_libm_samples",
                  test_atan2_rad_matches_libm_samples);
  test_registry_add(reg, UTILS_MATH_SUITE, "atan2_rad_heading_quadrants",
                  test_atan2_rad_heading_quadrants);
}
