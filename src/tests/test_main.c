#include <check.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "s21_string.h"

void add_string_edge_tests(TCase* case_group);
void add_format_edge_tests(TCase* case_group);
void add_scan_edge_tests(TCase* case_group);

#define CHECK_PRINT(pattern, ...)                                  \
  do {                                                             \
    char result[16384] = {0};                                      \
    char reference[16384] = {0};                                   \
    int written = s21_sprintf(result, pattern, __VA_ARGS__);       \
    int expected = sprintf(reference, pattern, __VA_ARGS__);       \
    ck_assert_int_eq(written, expected);                           \
    ck_assert_mem_eq(result, reference, (s21_size_t)expected + 1); \
  } while (0)

START_TEST(check_memory) {
  unsigned char input[] = {0, 240, 45, 0, 255};
  unsigned char actual[10] = {0};
  unsigned char expected[10] = {0};
  ck_assert_ptr_eq(s21_memchr(input, 240, 5), memchr(input, 240, 5));
  ck_assert_ptr_eq(s21_memchr(input, 500, 5), memchr(input, 500, 5));
  ck_assert_ptr_null(s21_memchr(input, 1, 5));
  ck_assert_ptr_null(s21_memchr(input, 0, 0));
  ck_assert_int_eq(s21_memcmp(input, input, 5), 0);
  ck_assert_int_gt(s21_memcmp("b", "a", 1), 0);
  ck_assert_int_lt(s21_memcmp("a", "b", 1), 0);
  ck_assert_int_eq(s21_memcmp(input, "", 0), 0);
  ck_assert_ptr_eq(s21_memcpy(actual, input, 5), actual);
  memcpy(expected, input, 5);
  ck_assert_mem_eq(actual, expected, 10);
  s21_memcpy(actual, input, 0);
  ck_assert_ptr_eq(s21_memset(actual, 255, 3), actual);
  memset(expected, 255, 3);
  ck_assert_mem_eq(actual, expected, 10);
  s21_memset(actual, 0, 0);
}
END_TEST

START_TEST(check_strings) {
  char actual[40] = "cat";
  char expected[40] = "cat";
  ck_assert_ptr_eq(s21_strncat(actual, "nap", 2), actual);
  strncat(expected, "nap", 2);
  ck_assert_str_eq(actual, expected);
  s21_strncat(actual, "!", 0);
  ck_assert_str_eq(actual, expected);
  ck_assert_uint_eq(s21_strlen(""), 0);
  ck_assert_uint_eq(s21_strlen(actual), strlen(expected));
  ck_assert_ptr_eq(s21_strchr(actual, 'a'), strchr(actual, 'a'));
  ck_assert_ptr_eq(s21_strchr(actual, '\0'), strchr(actual, '\0'));
  ck_assert_ptr_null(s21_strchr(actual, 'z'));
  ck_assert_ptr_eq(s21_strrchr("banana", 'a'), strrchr("banana", 'a'));
  ck_assert_ptr_eq(s21_strrchr("banana", '\0'), strrchr("banana", '\0'));
  ck_assert_ptr_null(s21_strrchr("banana", 'x'));
  ck_assert_int_eq(s21_strncmp("a", "a", 2), 0);
  ck_assert_int_lt(s21_strncmp("a", "b", 1), 0);
  ck_assert_int_gt(s21_strncmp("b", "a", 1), 0);
  ck_assert_int_eq(s21_strncmp("a", "b", 0), 0);
  char padded[8];
  memset(padded, 'Q', 8);
  ck_assert_ptr_eq(s21_strncpy(padded, "hi", 5), padded);
  ck_assert_mem_eq(padded, "hi\0\0\0QQQ", 8);
  s21_strncpy(padded, "longword", 2);
  ck_assert_mem_eq(padded, "lo", 2);
  ck_assert_uint_eq(s21_strcspn("abc:def", ":"), 3);
  ck_assert_uint_eq(s21_strcspn("abc", ""), 3);
  ck_assert_ptr_eq(s21_strpbrk("abc:def", "x:"), strchr("abc:def", ':'));
  ck_assert_ptr_null(s21_strpbrk("abc", "xyz"));
  ck_assert_ptr_eq(s21_strstr("banana", "nan"), strstr("banana", "nan"));
  ck_assert_ptr_eq(s21_strstr("abc", ""), strstr("abc", ""));
  ck_assert_ptr_null(s21_strstr("abc", "xyz"));
  char tokens[] = ",one;;two,three,";
  ck_assert_str_eq(s21_strtok(tokens, ",;"), "one");
  ck_assert_str_eq(s21_strtok(S21_NULL, ",;"), "two");
  ck_assert_str_eq(s21_strtok(S21_NULL, ",;"), "three");
  ck_assert_ptr_null(s21_strtok(S21_NULL, ",;"));
  char empty[] = ",,,";
  ck_assert_ptr_null(s21_strtok(empty, ","));
  for (int code = -2; code < 140; code++) {
    ck_assert_str_eq(s21_strerror(code), strerror(code));
  }
}
END_TEST

START_TEST(check_extra) {
  char* upper = s21_to_upper("aZ 3");
  char* lower = s21_to_lower("Az 3");
  ck_assert_str_eq(upper, "AZ 3");
  ck_assert_str_eq(lower, "az 3");
  free(upper);
  free(lower);
  ck_assert_ptr_null(s21_to_upper(S21_NULL));
  ck_assert_ptr_null(s21_to_lower(S21_NULL));
  char* inserted = s21_insert("abcd", "XX", 2);
  ck_assert_str_eq(inserted, "abXXcd");
  free(inserted);
  inserted = s21_insert("abc", "", 3);
  ck_assert_str_eq(inserted, "abc");
  free(inserted);
  ck_assert_ptr_null(s21_insert("a", "b", 2));
  ck_assert_ptr_null(s21_insert(S21_NULL, "b", 0));
  ck_assert_ptr_null(s21_insert("a", S21_NULL, 0));
  char* trimmed = s21_trim("--hi--", "-");
  ck_assert_str_eq(trimmed, "hi");
  free(trimmed);
  trimmed = s21_trim(" \tHello \n", S21_NULL);
  ck_assert_str_eq(trimmed, "Hello");
  free(trimmed);
  trimmed = s21_trim("xxxxx", "x");
  ck_assert_str_eq(trimmed, "");
  free(trimmed);
  ck_assert_ptr_null(s21_trim(S21_NULL, " "));
}
END_TEST

START_TEST(check_print_numbers) {
  CHECK_PRINT("%d", -24);
  CHECK_PRINT("%+09d", 42);
  CHECK_PRINT("% .0d", 0);
  CHECK_PRINT("%-10.6d", -500);
  CHECK_PRINT("%hd", (int)SHRT_MIN);
  CHECK_PRINT("%hhd", (int)SCHAR_MAX);
  CHECK_PRINT("%ld", LONG_MIN);
  CHECK_PRINT("%lld", LLONG_MIN);
  CHECK_PRINT("%u", UINT_MAX);
  CHECK_PRINT("%hu", (unsigned int)USHRT_MAX);
  CHECK_PRINT("%hhu", (unsigned int)UCHAR_MAX);
  CHECK_PRINT("%lu", ULONG_MAX);
  CHECK_PRINT("%llu", ULLONG_MAX);
  CHECK_PRINT("%#08x", 255u);
  CHECK_PRINT("%#X", 255u);
  CHECK_PRINT("%#.0o", 0u);
  CHECK_PRINT("%#.5o", 8u);
  CHECK_PRINT("%#o", 0u);
  CHECK_PRINT("%*.*d", 9, 5, 17);
  CHECK_PRINT("%*d", -8, 17);
  CHECK_PRINT("%lld", LLONG_MAX);
}
END_TEST

START_TEST(check_print_text) {
  CHECK_PRINT("%-10.3s", "abcdef");
  CHECK_PRINT("%10c", 'A');
  CHECK_PRINT("%-5s", "");
  CHECK_PRINT("%s", "hello");
  int pointer_value = 1234;
  CHECK_PRINT("%p", (void*)&pointer_value);
  CHECK_PRINT("%p", (void*)0);
  CHECK_PRINT("%c", 0);
  CHECK_PRINT("%d%%", 42);
  char actual[128] = {0};
  char expected[128] = {0};
  int actual_count = -1;
  int expected_count = -1;
  ck_assert_int_eq(s21_sprintf(actual, "ab%ncd", &actual_count),
                   sprintf(expected, "ab%ncd", &expected_count));
  ck_assert_int_eq(actual_count, expected_count);
  ck_assert_str_eq(actual, expected);
  signed char tiny = 0;
  short small = 0;
  long large = 0;
  long long largest = 0;
  s21_sprintf(actual, "X%hhn%hn%ln%lln", &tiny, &small, &large, &largest);
  ck_assert_int_eq(tiny, 1);
  ck_assert_int_eq(small, 1);
  ck_assert_int_eq(large, 1);
  ck_assert_int_eq(largest, 1);
  ck_assert_int_eq(s21_sprintf(actual, "%.3s", "hello"),
                   sprintf(expected, "%.3s", "hello"));
  ck_assert_str_eq(actual, expected);
}
END_TEST

START_TEST(check_print_floats) {
  double values[] = {0.0,  -0.0,    0.5,     -125.625, 1e-12,
                     1e25, DBL_MIN, DBL_MAX, INFINITY, -INFINITY};
  for (s21_size_t index = 0; index < sizeof(values) / sizeof(values[0]);
       index++) {
    CHECK_PRINT("%f", values[index]);
    CHECK_PRINT("%+.3e", values[index]);
    CHECK_PRINT("%#14.6g", values[index]);
    CHECK_PRINT("%-12.2E", values[index]);
  }
  CHECK_PRINT("%Lf", 123.456L);
  CHECK_PRINT("%.19Le", LDBL_MAX);
  CHECK_PRINT("%g", 9.999999);
  CHECK_PRINT("%#.0f", 10.0);
  CHECK_PRINT("%f", NAN);
  CHECK_PRINT("%E", NAN);
}
END_TEST

START_TEST(check_scan_integers) {
  int first = 0;
  unsigned int second = 0;
  unsigned int third = 0;
  int consumed = 0;
  ck_assert_int_eq(s21_sscanf("-25 037 0xff", "%d %o %x%n", &first, &second,
                              &third, &consumed),
                   3);
  ck_assert_int_eq(first, -25);
  ck_assert_uint_eq(second, 31);
  ck_assert_uint_eq(third, 255);
  ck_assert_int_eq(consumed, 12);
  int automatic_first = 0;
  int automatic_second = 0;
  int automatic_third = 0;
  ck_assert_int_eq(s21_sscanf("0x2a 077 42", "%i %i %i", &automatic_first,
                              &automatic_second, &automatic_third),
                   3);
  ck_assert_int_eq(automatic_first, 42);
  ck_assert_int_eq(automatic_second, 63);
  ck_assert_int_eq(automatic_third, 42);
  ck_assert_int_eq(s21_sscanf("10 20", "%*d %u", &second), 1);
  ck_assert_uint_eq(second, 20);
  ck_assert_int_eq(s21_sscanf("123", "%2d%n", &first, &consumed), 1);
  ck_assert_int_eq(first, 12);
  ck_assert_int_eq(consumed, 2);
  long long large = 0;
  ck_assert_int_eq(s21_sscanf("9223372036854775807", "%lld", &large), 1);
  ck_assert_int_eq(large, LLONG_MAX);
  unsigned long long unsigned_large = 0;
  ck_assert_int_eq(s21_sscanf("18446744073709551615", "%llu", &unsigned_large),
                   1);
  ck_assert_uint_eq(unsigned_large, ULLONG_MAX);
  signed char tiny = 0;
  short small = 0;
  ck_assert_int_eq(s21_sscanf("-4 32000", "%hhd %hd", &tiny, &small), 2);
  ck_assert_int_eq(tiny, -4);
  ck_assert_int_eq(small, 32000);
}
END_TEST

START_TEST(check_scan_floats) {
  double number = 0;
  int consumed = 0;
  ck_assert_int_eq(s21_sscanf("  -3.14159e2!", "%lf%n", &number, &consumed), 1);
  ck_assert_double_eq(number, -314.159);
  ck_assert_int_eq(consumed, 12);
  ck_assert_int_eq(s21_sscanf("0x1.8p+2", "%lf", &number), 1);
  ck_assert_double_eq(number, 6.0);
  ck_assert_int_eq(s21_sscanf("inf", "%lf", &number), 1);
  ck_assert(isinf(number));
  ck_assert_int_eq(s21_sscanf("nan", "%lf", &number), 1);
  ck_assert(isnan(number));
  float small = 0;
  long double extended = 0;
  ck_assert_int_eq(s21_sscanf("1.25 3.5", "%f %Lf", &small, &extended), 2);
  ck_assert_float_eq(small, 1.25f);
  ck_assert(extended == 3.5L);
  ck_assert_int_eq(s21_sscanf("1e-320", "%lf", &number), 1);
  ck_assert(number > 0.0);
  ck_assert_int_eq(s21_sscanf("1.25e50", "%lf", &number), 1);
  ck_assert(number > 1e49);
}
END_TEST

START_TEST(check_scan_text) {
  char word[32] = {0};
  char letters[32] = {0};
  char symbol = 0;
  ck_assert_int_eq(
      s21_sscanf(" abc-17!", "%3s%[a-z-]%c", word, letters, &symbol), 3);
  ck_assert_str_eq(word, "abc");
  ck_assert_str_eq(letters, "-");
  ck_assert_int_eq(symbol, '1');
  ck_assert_int_eq(s21_sscanf("token rest", "%[^ ]", word), 1);
  ck_assert_str_eq(word, "token");
  char bytes[3] = {0};
  ck_assert_int_eq(s21_sscanf("AB", "%2c", bytes), 1);
  ck_assert_mem_eq(bytes, "AB", 2);
  ck_assert_int_eq(s21_sscanf("abc", "%*[ab]%c", &symbol), 1);
  ck_assert_int_eq(symbol, 'c');
  ck_assert_int_eq(s21_sscanf("]abc", "%[]a-c]", word), 1);
  ck_assert_str_eq(word, "]abc");
  signed char tiny_count = 0;
  short short_count = 0;
  long long long_count = 0;
  long wide_count = 0;
  ck_assert_int_eq(s21_sscanf("AB", "%c%hhn%hn%ln%lln", &symbol, &tiny_count,
                              &short_count, &wide_count, &long_count),
                   1);
  ck_assert_int_eq(tiny_count, 1);
  ck_assert_int_eq(short_count, 1);
  ck_assert_int_eq(wide_count, 1);
  ck_assert_int_eq(long_count, 1);
  void* pointer = S21_NULL;
  ck_assert_int_eq(s21_sscanf("0x1234", "%p", &pointer), 1);
  ck_assert_ptr_eq(pointer, (void*)0x1234);
  ck_assert_int_eq(s21_sscanf("value%", "value%%"), 0);
  int number = 0;
  ck_assert_int_eq(s21_sscanf("", "%d", &number), -1);
  ck_assert_int_eq(s21_sscanf("x", "%d", &number), 0);
}
END_TEST

int main(void) {
  Suite* suite = suite_create("s21_string");
  TCase* string_cases = tcase_create("strings");
  TCase* format_cases = tcase_create("sprintf");
  TCase* scan_cases = tcase_create("sscanf");
  tcase_add_test(string_cases, check_memory);
  tcase_add_test(string_cases, check_strings);
  tcase_add_test(string_cases, check_extra);
  add_string_edge_tests(string_cases);
  tcase_add_test(format_cases, check_print_numbers);
  tcase_add_test(format_cases, check_print_text);
  tcase_add_test(format_cases, check_print_floats);
  add_format_edge_tests(format_cases);
  tcase_add_test(scan_cases, check_scan_integers);
  tcase_add_test(scan_cases, check_scan_floats);
  tcase_add_test(scan_cases, check_scan_text);
  add_scan_edge_tests(scan_cases);
  suite_add_tcase(suite, string_cases);
  suite_add_tcase(suite, format_cases);
  suite_add_tcase(suite, scan_cases);
  SRunner* runner = srunner_create(suite);
  srunner_run_all(runner, CK_NORMAL);
  int failures = srunner_ntests_failed(runner);
  srunner_free(runner);
  return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
