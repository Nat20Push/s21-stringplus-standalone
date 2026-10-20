#include <check.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "s21_string.h"

START_TEST(scan_decimal_with_width) {
  int actual = 0;
  int expected = 0;
  int actual_count = -1;
  int expected_count = -1;
  ck_assert_int_eq(s21_sscanf("  -1234x", "%3d%n", &actual, &actual_count),
                   sscanf("  -1234x", "%3d%n", &expected, &expected_count));
  ck_assert_int_eq(actual, expected);
  ck_assert_int_eq(actual_count, expected_count);
}
END_TEST

START_TEST(scan_automatic_integer_bases) {
  int actual[3] = {0};
  int expected[3] = {0};
  ck_assert_int_eq(
      s21_sscanf("010 0x10 10", "%i %i %i", &actual[0], &actual[1], &actual[2]),
      sscanf("010 0x10 10", "%i %i %i", &expected[0], &expected[1],
             &expected[2]));
  ck_assert_mem_eq(actual, expected, sizeof(actual));
}
END_TEST

START_TEST(scan_unsigned_bases) {
  unsigned int actual[3] = {0};
  unsigned int expected[3] = {0};
  ck_assert_int_eq(
      s21_sscanf("77 ff 42", "%o %x %u", &actual[0], &actual[1], &actual[2]),
      sscanf("77 ff 42", "%o %x %u", &expected[0], &expected[1], &expected[2]));
  ck_assert_mem_eq(actual, expected, sizeof(actual));
  ck_assert_int_eq(s21_sscanf("AB", "%x", &actual[0]),
                   sscanf("AB", "%x", &expected[0]));
  ck_assert_uint_eq(actual[0], expected[0]);
}
END_TEST

START_TEST(scan_suppressed_conversion) {
  int actual = 0;
  int expected = 0;
  ck_assert_int_eq(s21_sscanf("ignore:25", "%*[^:] :%d", &actual),
                   sscanf("ignore:25", "%*[^:] :%d", &expected));
  ck_assert_int_eq(actual, expected);
}
END_TEST

START_TEST(scan_length_modifiers) {
  signed char actual_small = 0;
  signed char expected_small = 0;
  long long actual_large = 0;
  long long expected_large = 0;
  ck_assert_int_eq(
      s21_sscanf("-7 123456", "%hhd %lld", &actual_small, &actual_large),
      sscanf("-7 123456", "%hhd %lld", &expected_small, &expected_large));
  ck_assert_int_eq(actual_small, expected_small);
  ck_assert(actual_large == expected_large);
}
END_TEST

START_TEST(scan_all_integer_length_modifiers) {
  const char* input =
      "-12 -1234 123456 -9223372036854775808 255 65535 "
      "123456 18446744073709551615";
  const char* format = "%hhd %hd %ld %lld %hhu %hu %lu %llu";
  signed char actual_byte = 0;
  signed char expected_byte = 0;
  short actual_short = 0;
  short expected_short = 0;
  long actual_long = 0;
  long expected_long = 0;
  long long actual_long_long = 0;
  long long expected_long_long = 0;
  unsigned char actual_unsigned_byte = 0;
  unsigned char expected_unsigned_byte = 0;
  unsigned short actual_unsigned_short = 0;
  unsigned short expected_unsigned_short = 0;
  unsigned long actual_unsigned_long = 0;
  unsigned long expected_unsigned_long = 0;
  unsigned long long actual_unsigned_long_long = 0;
  unsigned long long expected_unsigned_long_long = 0;
  int actual_count = s21_sscanf(
      input, format, &actual_byte, &actual_short, &actual_long,
      &actual_long_long, &actual_unsigned_byte, &actual_unsigned_short,
      &actual_unsigned_long, &actual_unsigned_long_long);
  int expected_count = sscanf(
      input, format, &expected_byte, &expected_short, &expected_long,
      &expected_long_long, &expected_unsigned_byte, &expected_unsigned_short,
      &expected_unsigned_long, &expected_unsigned_long_long);
  ck_assert_int_eq(actual_count, expected_count);
  ck_assert_int_eq(actual_byte, expected_byte);
  ck_assert_int_eq(actual_short, expected_short);
  ck_assert(actual_long == expected_long);
  ck_assert(actual_long_long == expected_long_long);
  ck_assert_uint_eq(actual_unsigned_byte, expected_unsigned_byte);
  ck_assert_uint_eq(actual_unsigned_short, expected_unsigned_short);
  ck_assert(actual_unsigned_long == expected_unsigned_long);
  ck_assert(actual_unsigned_long_long == expected_unsigned_long_long);
}
END_TEST

START_TEST(scan_decimal_floats) {
  float actual_small = 0;
  float expected_small = 0;
  double actual_large = 0;
  double expected_large = 0;
  ck_assert_int_eq(
      s21_sscanf("-1.25 6.02e3", "%f %lf", &actual_small, &actual_large),
      sscanf("-1.25 6.02e3", "%f %lf", &expected_small, &expected_large));
  ck_assert(actual_small == expected_small);
  ck_assert(actual_large == expected_large);
}
END_TEST

START_TEST(scan_hexadecimal_float) {
  double actual = 0;
  double expected = 0;
  ck_assert_int_eq(s21_sscanf("0x1.8p+2", "%lf", &actual),
                   sscanf("0x1.8p+2", "%lf", &expected));
  ck_assert(actual == expected);
}
END_TEST

START_TEST(scan_special_floats) {
  double infinity = 0;
  double not_a_number = 0;
  ck_assert_int_eq(s21_sscanf("-inf nan", "%lf %lf", &infinity, &not_a_number),
                   2);
  ck_assert(isinf(infinity) && signbit(infinity));
  ck_assert(isnan(not_a_number));
}
END_TEST

START_TEST(scan_word_and_characters) {
  char actual_word[8] = {0};
  char expected_word[8] = {0};
  char actual_chars[3] = {0};
  char expected_chars[3] = {0};
  ck_assert_int_eq(
      s21_sscanf(" hello XY", "%5s %2c", actual_word, actual_chars),
      sscanf(" hello XY", "%5s %2c", expected_word, expected_chars));
  ck_assert_str_eq(actual_word, expected_word);
  ck_assert_mem_eq(actual_chars, expected_chars, sizeof(actual_chars));
}
END_TEST

START_TEST(scan_scanset) {
  char actual[16] = {0};
  char expected[16] = {0};
  ck_assert_int_eq(s21_sscanf("abc123", "%[a-z]", actual),
                   sscanf("abc123", "%[a-z]", expected));
  ck_assert_str_eq(actual, expected);
  ck_assert_int_eq(s21_sscanf("123abc", "%[^a-z]", actual),
                   sscanf("123abc", "%[^a-z]", expected));
  ck_assert_str_eq(actual, expected);
}
END_TEST

START_TEST(scan_pointer_and_count) {
  void* actual_pointer = S21_NULL;
  void* expected_pointer = NULL;
  int actual_count = -1;
  int expected_count = -1;
  ck_assert_int_eq(
      s21_sscanf("0x1234!", "%p%n", &actual_pointer, &actual_count),
      sscanf("0x1234!", "%p%n", &expected_pointer, &expected_count));
  ck_assert_ptr_eq(actual_pointer, expected_pointer);
  ck_assert_int_eq(actual_count, expected_count);
}
END_TEST

START_TEST(scan_no_match_or_input) {
  int value = 0;
  ck_assert_int_eq(s21_sscanf("", "%d", &value), sscanf("", "%d", &value));
  ck_assert_int_eq(s21_sscanf("abc", "%d", &value),
                   sscanf("abc", "%d", &value));
}
END_TEST

START_TEST(scan_floating_point_limits) {
  const char* inputs[] = {"4.9406564584124654e-324", "2.2250738585072014e-308",
                          "1.7976931348623157e308", "1e-5000", "1e5000"};
  for (size_t index = 0; index < sizeof(inputs) / sizeof(inputs[0]); index++) {
    double actual = 0;
    double expected = 0;
    ck_assert_int_eq(s21_sscanf(inputs[index], "%lf", &actual),
                     sscanf(inputs[index], "%lf", &expected));
    ck_assert(actual == expected);
  }
}
END_TEST

START_TEST(scan_decimal_rounding_sweep) {
  char input[80];
  for (int index = -1000; index <= 1000; index++) {
    double source = index / 37.0;
    sprintf(input, "%.17g", source);
    double actual = 0;
    double expected = 0;
    ck_assert_int_eq(s21_sscanf(input, "%lf", &actual),
                     sscanf(input, "%lf", &expected));
    ck_assert(actual == expected);
  }
}
END_TEST

START_TEST(scan_long_digit_string) {
  char input[1102];
  for (int index = 0; index < 1100; index++) {
    input[index] = (char)('1' + index % 9);
  }
  input[1100] = '\0';
  double actual = 0;
  double expected = 0;
  ck_assert_int_eq(s21_sscanf(input, "%lf", &actual),
                   sscanf(input, "%lf", &expected));
  ck_assert(actual == expected);
  input[4] = '.';
  ck_assert_int_eq(s21_sscanf(input, "%lf", &actual),
                   sscanf(input, "%lf", &expected));
  ck_assert(actual == expected);
}
END_TEST

START_TEST(scan_extended_long_double) {
  const char* inputs[] = {"0.1", "1e-4000", "1e4000",
                          "3.3621031431120935063e-4932",
                          "1.1897314953572317650e4932"};
  for (size_t index = 0; index < sizeof(inputs) / sizeof(inputs[0]); index++) {
    long double actual = 0;
    long double expected = 0;
    ck_assert_int_eq(s21_sscanf(inputs[index], "%Lf", &actual),
                     sscanf(inputs[index], "%Lf", &expected));
    ck_assert(actual == expected);
  }
}
END_TEST

void add_scan_edge_tests(TCase* case_group) {
  tcase_add_test(case_group, scan_decimal_with_width);
  tcase_add_test(case_group, scan_automatic_integer_bases);
  tcase_add_test(case_group, scan_unsigned_bases);
  tcase_add_test(case_group, scan_suppressed_conversion);
  tcase_add_test(case_group, scan_length_modifiers);
  tcase_add_test(case_group, scan_all_integer_length_modifiers);
  tcase_add_test(case_group, scan_decimal_floats);
  tcase_add_test(case_group, scan_hexadecimal_float);
  tcase_add_test(case_group, scan_special_floats);
  tcase_add_test(case_group, scan_word_and_characters);
  tcase_add_test(case_group, scan_scanset);
  tcase_add_test(case_group, scan_pointer_and_count);
  tcase_add_test(case_group, scan_no_match_or_input);
  tcase_add_test(case_group, scan_floating_point_limits);
  tcase_add_test(case_group, scan_decimal_rounding_sweep);
  tcase_add_test(case_group, scan_long_digit_string);
  tcase_add_test(case_group, scan_extended_long_double);
}
