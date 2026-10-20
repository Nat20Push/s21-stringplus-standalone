#include <check.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "s21_string.h"

START_TEST(memory_search_with_embedded_zero) {
  unsigned char bytes[] = {'A', 0, 255, 'B'};
  ck_assert_ptr_eq(s21_memchr(bytes, 0, sizeof(bytes)),
                   memchr(bytes, 0, sizeof(bytes)));
  ck_assert_ptr_eq(s21_memchr(bytes, 255, sizeof(bytes)),
                   memchr(bytes, 255, sizeof(bytes)));
  ck_assert_ptr_null(s21_memchr(bytes, 'B', 3));
}
END_TEST

START_TEST(memory_compare_unsigned_bytes) {
  unsigned char high[] = {0, 255};
  unsigned char low[] = {0, 1};
  ck_assert_int_gt(s21_memcmp(high, low, sizeof(high)), 0);
  ck_assert_int_lt(s21_memcmp(low, high, sizeof(low)), 0);
  ck_assert_int_eq(s21_memcmp(high, low, 1), 0);
}
END_TEST

START_TEST(memory_copy_and_fill_boundaries) {
  unsigned char actual[] = {1, 2, 3, 4, 5};
  unsigned char expected[] = {1, 2, 3, 4, 5};
  s21_memset(actual + 1, 0x123, 2);
  memset(expected + 1, 0x123, 2);
  ck_assert_mem_eq(actual, expected, sizeof(actual));
  s21_memcpy(actual + 2, "XY", 2);
  memcpy(expected + 2, "XY", 2);
  ck_assert_mem_eq(actual, expected, sizeof(actual));
}
END_TEST

START_TEST(string_length_and_concatenation) {
  char actual[16] = "ab";
  char expected[16] = "ab";
  ck_assert_uint_eq(s21_strlen(actual), strlen(expected));
  s21_strncat(actual, "cdef", 2);
  strncat(expected, "cdef", 2);
  ck_assert_str_eq(actual, expected);
  ck_assert_uint_eq(s21_strlen(actual), strlen(expected));
}
END_TEST

START_TEST(string_search_positions) {
  const char* text = "abca";
  ck_assert_ptr_eq(s21_strchr(text, 'a'), strchr(text, 'a'));
  ck_assert_ptr_eq(s21_strrchr(text, 'a'), strrchr(text, 'a'));
  ck_assert_ptr_eq(s21_strchr(text, '\0'), text + strlen(text));
  ck_assert_ptr_null(s21_strrchr(text, 'z'));
}
END_TEST

START_TEST(string_compare_prefix) {
  ck_assert_int_eq(s21_strncmp("abc", "abd", 2), 0);
  ck_assert_int_lt(s21_strncmp("abc", "abd", 3), 0);
  ck_assert_int_gt(s21_strncmp("abd", "abc", 3), 0);
  ck_assert_int_eq(s21_strncmp("", "anything", 0), 0);
}
END_TEST

START_TEST(string_compare_high_bytes) {
  const char high[] = {(char)0xFF, '\0'};
  const char low[] = {(char)0x01, '\0'};
  ck_assert_int_gt(s21_strncmp(high, low, 1), 0);
  ck_assert_int_lt(s21_strncmp(low, high, 1), 0);
}
END_TEST

START_TEST(string_copy_padding) {
  char actual[8];
  char expected[8];
  memset(actual, 'x', sizeof(actual));
  memset(expected, 'x', sizeof(expected));
  s21_strncpy(actual, "a", 5);
  strncpy(expected, "a", 5);
  ck_assert_mem_eq(actual, expected, sizeof(actual));
}
END_TEST

START_TEST(string_sets_and_substrings) {
  const char* text = "alpha:beta";
  ck_assert_uint_eq(s21_strcspn(text, ":"), strcspn(text, ":"));
  ck_assert_ptr_eq(s21_strpbrk(text, "z:"), strpbrk(text, "z:"));
  ck_assert_ptr_eq(s21_strstr(text, "beta"), strstr(text, "beta"));
  ck_assert_ptr_null(s21_strstr(text, "gamma"));
}
END_TEST

START_TEST(string_token_delimiter_changes) {
  char actual[] = "a,b;c";
  char expected[] = "a,b;c";
  ck_assert_str_eq(s21_strtok(actual, ","), strtok(expected, ","));
  ck_assert_str_eq(s21_strtok(S21_NULL, ";"), strtok(NULL, ";"));
  ck_assert_str_eq(s21_strtok(S21_NULL, ";"), strtok(NULL, ";"));
  ck_assert_ptr_null(s21_strtok(S21_NULL, ";"));
}
END_TEST

START_TEST(string_case_conversion) {
  char* upper = s21_to_upper("Hello 123!");
  char* lower = s21_to_lower("Hello 123!");
  ck_assert_str_eq(upper, "HELLO 123!");
  ck_assert_str_eq(lower, "hello 123!");
  free(upper);
  free(lower);
}
END_TEST

START_TEST(string_insert_positions) {
  char* beginning = s21_insert("world", "hello ", 0);
  char* ending = s21_insert("hello", " world", 5);
  ck_assert_str_eq(beginning, "hello world");
  ck_assert_str_eq(ending, "hello world");
  free(beginning);
  free(ending);
}
END_TEST

START_TEST(string_trim_all_or_none) {
  char* all = s21_trim("xxxx", "x");
  char* none = s21_trim("hello", "x");
  ck_assert_str_eq(all, "");
  ck_assert_str_eq(none, "hello");
  free(all);
  free(none);
}
END_TEST

START_TEST(string_error_number_extremes) {
  ck_assert_str_eq(s21_strerror(INT_MIN), strerror(INT_MIN));
  ck_assert_str_eq(s21_strerror(INT_MAX), strerror(INT_MAX));
  ck_assert_str_eq(s21_strerror(1000), strerror(1000));
}
END_TEST

void add_string_edge_tests(TCase* case_group) {
  tcase_add_test(case_group, memory_search_with_embedded_zero);
  tcase_add_test(case_group, memory_compare_unsigned_bytes);
  tcase_add_test(case_group, memory_copy_and_fill_boundaries);
  tcase_add_test(case_group, string_length_and_concatenation);
  tcase_add_test(case_group, string_search_positions);
  tcase_add_test(case_group, string_compare_prefix);
  tcase_add_test(case_group, string_compare_high_bytes);
  tcase_add_test(case_group, string_copy_padding);
  tcase_add_test(case_group, string_sets_and_substrings);
  tcase_add_test(case_group, string_token_delimiter_changes);
  tcase_add_test(case_group, string_case_conversion);
  tcase_add_test(case_group, string_insert_positions);
  tcase_add_test(case_group, string_trim_all_or_none);
  tcase_add_test(case_group, string_error_number_extremes);
}
