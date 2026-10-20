#include <check.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "s21_string.h"

#define COMPARE_FORMAT(pattern, ...)                               \
  do {                                                             \
    char actual[512] = {0};                                        \
    char expected[512] = {0};                                      \
    int actual_size = s21_sprintf(actual, pattern, __VA_ARGS__);   \
    int expected_size = sprintf(expected, pattern, __VA_ARGS__);   \
    ck_assert_int_eq(actual_size, expected_size);                  \
    ck_assert_mem_eq(actual, expected, (size_t)expected_size + 1); \
  } while (0)

START_TEST(format_signed_boundaries) {
  COMPARE_FORMAT("%+d %+d % d", 0, INT_MAX, INT_MIN);
  COMPARE_FORMAT("%lld %lld", LLONG_MIN, LLONG_MAX);
  COMPARE_FORMAT("%.0d|%5.0d", 0, 0);
}
END_TEST

START_TEST(format_unsigned_bases) {
  COMPARE_FORMAT("%#x %#X %#o", 31u, 31u, 31u);
  COMPARE_FORMAT("%#08x|%-#8o", 0xABu, 9u);
  COMPARE_FORMAT("%u %lu %llu", UINT_MAX, ULONG_MAX, ULLONG_MAX);
}
END_TEST

START_TEST(format_dynamic_width_and_precision) {
  COMPARE_FORMAT("%*.*d", 10, 4, -12);
  COMPARE_FORMAT("%*.*s", -8, 3, "abcdef");
  COMPARE_FORMAT("%.*f", -1, 1.25);
}
END_TEST

START_TEST(format_character_and_percent) {
  char* missing = S21_NULL;
  COMPARE_FORMAT("%c|%5c|%%", 'A', 0);
  COMPARE_FORMAT("%-8s|%.3s", "hi", "abcdef");
  COMPARE_FORMAT("%s", missing);
  COMPARE_FORMAT("%.3s", missing);
}
END_TEST

START_TEST(format_pointer) {
  int value = 7;
  COMPARE_FORMAT("%p", (void*)&value);
  COMPARE_FORMAT("%20p", (void*)0);
}
END_TEST

START_TEST(format_count_conversion) {
  char actual[64] = {0};
  char expected[64] = {0};
  int actual_count = -1;
  int expected_count = -1;
  int actual_size = s21_sprintf(actual, "%05d%nX", 42, &actual_count);
  int expected_size = sprintf(expected, "%05d%nX", 42, &expected_count);
  ck_assert_int_eq(actual_size, expected_size);
  ck_assert_int_eq(actual_count, expected_count);
  ck_assert_str_eq(actual, expected);
}
END_TEST

START_TEST(format_fixed_rounding) {
  COMPARE_FORMAT("%.0f %.0f", 2.5, 3.5);
  COMPARE_FORMAT("%.2f %.2f", 1.125, -1.125);
  COMPARE_FORMAT("%#.0f|%+010.2f", 10.0, 1.5);
}
END_TEST

START_TEST(format_scientific_and_general) {
  COMPARE_FORMAT("%.3e %.3E", 0.0001234, 123400.0);
  COMPARE_FORMAT("%.4g %.4G", 0.00009999, 99999.0);
  COMPARE_FORMAT("%#g", 10.0);
}
END_TEST

START_TEST(format_float_extremes) {
  COMPARE_FORMAT("%.6g", DBL_MIN);
  COMPARE_FORMAT("%.6g", DBL_MAX);
  COMPARE_FORMAT("%+f", INFINITY);
  COMPARE_FORMAT("%E", NAN);
}
END_TEST

START_TEST(format_long_double) {
  COMPARE_FORMAT("%.8Lf", 1.23456789L);
  COMPARE_FORMAT("%.4Le", LDBL_MIN);
}
END_TEST

START_TEST(format_large_precision_and_width) {
  char actual[4096] = {0};
  char expected[4096] = {0};
  int actual_size = s21_sprintf(actual, "%.500f", 0.1);
  int expected_size = sprintf(expected, "%.500f", 0.1);
  ck_assert_int_eq(actual_size, expected_size);
  ck_assert_str_eq(actual, expected);
  actual_size = s21_sprintf(actual, "%1500.3f", 1e300);
  expected_size = sprintf(expected, "%1500.3f", 1e300);
  ck_assert_int_eq(actual_size, expected_size);
  ck_assert_str_eq(actual, expected);
}
END_TEST

START_TEST(format_rounding_sweep) {
  char actual[256];
  char expected[256];
  for (int index = 1; index <= 500; index++) {
    double value = index / 8.0 + index * 0.001;
    int actual_size =
        s21_sprintf(actual, "%.0f|%.2e|%.6g", value, value, value);
    int expected_size =
        sprintf(expected, "%.0f|%.2e|%.6g", value, value, value);
    ck_assert_int_eq(actual_size, expected_size);
    ck_assert_str_eq(actual, expected);
  }
}
END_TEST

START_TEST(format_mixed_fields) {
  COMPARE_FORMAT("[%+05d][%-8.2f][%.3s]", 17, 1.25, "hello");
  COMPARE_FORMAT("%d %f %s", -2, 0.5, "ok");
}
END_TEST

void add_format_edge_tests(TCase* case_group) {
  tcase_add_test(case_group, format_signed_boundaries);
  tcase_add_test(case_group, format_unsigned_bases);
  tcase_add_test(case_group, format_dynamic_width_and_precision);
  tcase_add_test(case_group, format_character_and_percent);
  tcase_add_test(case_group, format_pointer);
  tcase_add_test(case_group, format_count_conversion);
  tcase_add_test(case_group, format_fixed_rounding);
  tcase_add_test(case_group, format_scientific_and_general);
  tcase_add_test(case_group, format_float_extremes);
  tcase_add_test(case_group, format_long_double);
  tcase_add_test(case_group, format_large_precision_and_width);
  tcase_add_test(case_group, format_rounding_sweep);
  tcase_add_test(case_group, format_mixed_fields);
}
