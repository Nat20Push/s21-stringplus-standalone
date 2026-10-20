#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#include "s21_string.h"

#define S21_DECIMAL_LIMIT 20000

typedef struct {
  char* destination;
  int written;
} S21Output;

typedef struct {
  int left;
  int plus;
  int blank;
  int alternate;
  int zero;
  int width;
  int precision;
  int precision_set;
  char length;
  char conversion;
} S21PrintSpec;

typedef struct {
  unsigned char digits[S21_DECIMAL_LIMIT];
  int count;
  int scale;
} S21ExactDecimal;

static void s21_emit(S21Output* output, char symbol) {
  output->destination[output->written++] = symbol;
}

static void s21_emit_many(S21Output* output, char symbol, int count) {
  for (int index = 0; index < count; index++) s21_emit(output, symbol);
}

static void s21_emit_text(S21Output* output, const char* text, int count) {
  for (int index = 0; index < count; index++) s21_emit(output, text[index]);
}

static int s21_parse_number(const char** cursor) {
  int number = 0;
  while (**cursor >= '0' && **cursor <= '9') {
    int digit = *(*cursor)++ - '0';
    number = number > (INT_MAX - digit) / 10 ? INT_MAX : number * 10 + digit;
  }
  return number;
}

static S21PrintSpec s21_read_print_spec(const char** format, va_list* values) {
  S21PrintSpec spec = {0};
  int scanning = 1;
  while (scanning) {
    switch (**format) {
      case '-':
        spec.left = 1;
        break;
      case '+':
        spec.plus = 1;
        break;
      case ' ':
        spec.blank = 1;
        break;
      case '#':
        spec.alternate = 1;
        break;
      case '0':
        spec.zero = 1;
        break;
      default:
        scanning = 0;
        break;
    }
    if (scanning) (*format)++;
  }
  if (**format == '*') {
    int width = va_arg(*values, int);
    if (width < 0) {
      spec.left = 1;
      spec.width = width == INT_MIN ? INT_MAX : -width;
    } else {
      spec.width = width;
    }
    (*format)++;
  } else {
    spec.width = s21_parse_number(format);
  }
  if (**format == '.') {
    (*format)++;
    spec.precision_set = 1;
    if (**format == '*') {
      spec.precision = va_arg(*values, int);
      if (spec.precision < 0) spec.precision_set = 0;
      (*format)++;
    } else {
      spec.precision = s21_parse_number(format);
    }
  }
  if (**format == 'h' || **format == 'l') {
    char first = *(*format)++;
    spec.length = first;
    if (**format == first) {
      spec.length = first == 'h' ? 'H' : 'q';
      (*format)++;
    }
  } else if (**format == 'L') {
    spec.length = *(*format)++;
  }
  spec.conversion = **format;
  if (**format != '\0') (*format)++;
  return spec;
}

static void s21_write_field(S21Output* output, const S21PrintSpec* spec,
                            const char* prefix, int prefix_size,
                            const char* body, int body_size,
                            int precision_zeros, int pad_with_zero) {
  int size = prefix_size + body_size + precision_zeros;
  int padding = spec->width > size ? spec->width - size : 0;
  if (!spec->left && !pad_with_zero) s21_emit_many(output, ' ', padding);
  s21_emit_text(output, prefix, prefix_size);
  if (!spec->left && pad_with_zero) s21_emit_many(output, '0', padding);
  s21_emit_many(output, '0', precision_zeros);
  s21_emit_text(output, body, body_size);
  if (spec->left) s21_emit_many(output, ' ', padding);
}

static intmax_t s21_get_signed(va_list* values, char length) {
  intmax_t result = 0;
  if (length == 'l') {
    result = va_arg(*values, long);
  } else if (length == 'q') {
    result = va_arg(*values, long long);
  } else if (length == 'h') {
    result = (short)va_arg(*values, int);
  } else if (length == 'H') {
    result = (signed char)va_arg(*values, int);
  } else {
    result = va_arg(*values, int);
  }
  return result;
}

static uintmax_t s21_get_unsigned(va_list* values, char length) {
  uintmax_t result = 0;
  if (length == 'l') {
    result = va_arg(*values, unsigned long);
  } else if (length == 'q') {
    result = va_arg(*values, unsigned long long);
  } else if (length == 'h') {
    result = (unsigned short)va_arg(*values, unsigned int);
  } else if (length == 'H') {
    result = (unsigned char)va_arg(*values, unsigned int);
  } else {
    result = va_arg(*values, unsigned int);
  }
  return result;
}

static int s21_make_digits(uintmax_t value, unsigned int base, int upper,
                           char* result) {
  char backwards[sizeof(uintmax_t) * CHAR_BIT + 1];
  int count = 0;
  do {
    int digit = (int)(value % base);
    backwards[count++] =
        (char)(digit < 10 ? '0' + digit : (upper ? 'A' : 'a') + digit - 10);
    value /= base;
  } while (value != 0);
  for (int index = 0; index < count; index++) {
    result[index] = backwards[count - index - 1];
  }
  return count;
}

static void s21_print_integer(S21Output* output, S21PrintSpec spec,
                              va_list* values) {
  int is_signed = spec.conversion == 'd' || spec.conversion == 'i';
  int upper = spec.conversion == 'X';
  unsigned int base = spec.conversion == 'o' ? 8 : 10;
  if (spec.conversion == 'x' || upper) base = 16;
  char prefix[4] = {0};
  int prefix_size = 0;
  uintmax_t magnitude = 0;
  if (is_signed) {
    intmax_t value = s21_get_signed(values, spec.length);
    magnitude = value < 0 ? (uintmax_t)(-(value + 1)) + 1 : (uintmax_t)value;
    if (value < 0) {
      prefix[prefix_size++] = '-';
    } else if (spec.plus) {
      prefix[prefix_size++] = '+';
    } else if (spec.blank) {
      prefix[prefix_size++] = ' ';
    }
  } else {
    magnitude = s21_get_unsigned(values, spec.length);
  }
  char digits[sizeof(uintmax_t) * CHAR_BIT + 1];
  int digit_count = s21_make_digits(magnitude, base, upper, digits);
  if (spec.precision_set && spec.precision == 0 && magnitude == 0) {
    digit_count = 0;
  }
  int precision_zeros = spec.precision_set && spec.precision > digit_count
                            ? spec.precision - digit_count
                            : 0;
  if (base == 8 && spec.alternate && digit_count == 0) {
    prefix[prefix_size++] = '0';
  } else if (base == 8 && spec.alternate && digits[0] != '0' &&
             precision_zeros == 0) {
    prefix[prefix_size++] = '0';
  }
  if (base == 16 && spec.alternate && magnitude != 0) {
    prefix[prefix_size++] = '0';
    prefix[prefix_size++] = upper ? 'X' : 'x';
  }
  s21_write_field(output, &spec, prefix, prefix_size, digits, digit_count,
                  precision_zeros,
                  spec.zero && !spec.left && !spec.precision_set);
}

static void s21_decimal_times(S21ExactDecimal* number, int factor) {
  int carry = 0;
  for (int index = 0; index < number->count; index++) {
    int product = number->digits[index] * factor + carry;
    number->digits[index] = (unsigned char)(product % 10);
    carry = product / 10;
  }
  while (carry != 0 && number->count < S21_DECIMAL_LIMIT) {
    number->digits[number->count++] = (unsigned char)(carry % 10);
    carry /= 10;
  }
}

static void s21_decimal_add_bit(S21ExactDecimal* number, int bit) {
  for (int index = 0; bit != 0 && index < S21_DECIMAL_LIMIT; index++) {
    if (index == number->count) number->digits[number->count++] = 0;
    int sum = number->digits[index] + bit;
    number->digits[index] = (unsigned char)(sum % 10);
    bit = sum / 10;
  }
}

static void s21_decimal_from_float(S21ExactDecimal* number, long double value,
                                   int significand_bits) {
  number->count = 1;
  number->scale = 0;
  number->digits[0] = 0;
  if (value == 0.0L) return;
  int exponent = 0;
  long double fraction = frexpl(value, &exponent);
  for (int bit = 0; bit < significand_bits; bit++) {
    fraction *= 2.0L;
    int one = fraction >= 1.0L;
    fraction -= one;
    s21_decimal_times(number, 2);
    s21_decimal_add_bit(number, one);
  }
  int shift = exponent - significand_bits;
  if (shift >= 0) {
    for (int index = 0; index < shift; index++) s21_decimal_times(number, 2);
  } else {
    number->scale = -shift;
    for (int index = 0; index < -shift; index++) {
      s21_decimal_times(number, 5);
    }
  }
}

static int s21_decimal_at(const S21ExactDecimal* number, int power) {
  long long position = (long long)power + number->scale;
  return position >= 0 && position < number->count ? number->digits[position]
                                                   : 0;
}

static int s21_round_away(const S21ExactDecimal* number, int next_power,
                          int previous_digit) {
  int next_digit = s21_decimal_at(number, next_power);
  long long position = (long long)next_power + number->scale;
  int remainder = 0;
  for (int index = 0; index < position && index < number->count; index++) {
    if (number->digits[index] != 0) remainder = 1;
  }
  return next_digit > 5 ||
         (next_digit == 5 && (remainder || previous_digit % 2 != 0));
}

static int s21_increment_digits(char* text, int count) {
  int cursor = count - 1;
  while (cursor >= 0) {
    if (text[cursor] == '.') {
      cursor--;
    } else if (text[cursor] == '9') {
      text[cursor--] = '0';
    } else {
      text[cursor]++;
      return count;
    }
  }
  for (int index = count; index > 0; index--) text[index] = text[index - 1];
  text[0] = '1';
  return count + 1;
}

static int s21_format_fixed(const S21ExactDecimal* number, int precision,
                            int alternate, char* text) {
  int highest = number->count - number->scale - 1;
  if (highest < 0) highest = 0;
  int count = 0;
  for (int power = highest; power >= 0; power--) {
    text[count++] = (char)('0' + s21_decimal_at(number, power));
  }
  if (precision > 0 || alternate) text[count++] = '.';
  for (int place = 1; place <= precision; place++) {
    text[count++] = (char)('0' + s21_decimal_at(number, -place));
  }
  if (s21_round_away(number, -precision - 1,
                     s21_decimal_at(number, -precision))) {
    count = s21_increment_digits(text, count);
  }
  text[count] = '\0';
  return count;
}

static int s21_format_scientific(const S21ExactDecimal* number, int precision,
                                 int alternate, int upper, char* text) {
  int exponent = number->count == 1 && number->digits[0] == 0
                     ? 0
                     : number->count - number->scale - 1;
  int count = 0;
  for (int place = 0; place <= precision; place++) {
    if (place == 1) text[count++] = '.';
    text[count++] = (char)('0' + s21_decimal_at(number, exponent - place));
  }
  if (precision == 0 && alternate) text[count++] = '.';
  if (s21_round_away(number, exponent - precision - 1,
                     s21_decimal_at(number, exponent - precision))) {
    count = s21_increment_digits(text, count);
    if (text[0] == '1' && text[1] == '0') {
      exponent++;
      count = 0;
      text[count++] = '1';
      if (precision > 0 || alternate) text[count++] = '.';
      for (int index = 0; index < precision; index++) text[count++] = '0';
    }
  }
  text[count++] = upper ? 'E' : 'e';
  text[count++] = exponent < 0 ? '-' : '+';
  int magnitude = exponent < 0 ? -exponent : exponent;
  char backwards[16];
  int digits = 0;
  do {
    backwards[digits++] = (char)('0' + magnitude % 10);
    magnitude /= 10;
  } while (magnitude != 0);
  if (digits < 2) backwards[digits++] = '0';
  while (digits != 0) text[count++] = backwards[--digits];
  text[count] = '\0';
  return count;
}

static int s21_format_general(const S21ExactDecimal* number, int precision,
                              int alternate, int upper, char* text) {
  if (precision == 0) precision = 1;
  int count =
      s21_format_scientific(number, precision - 1, alternate, upper, text);
  int marker = 0;
  while (marker < count && text[marker] != 'e' && text[marker] != 'E') marker++;
  int exponent = 0;
  for (int index = marker + 2; index < count; index++) {
    exponent = exponent * 10 + text[index] - '0';
  }
  if (text[marker + 1] == '-') exponent = -exponent;
  if (exponent >= -4 && exponent < precision) {
    count = s21_format_fixed(number, precision - exponent - 1, alternate, text);
    marker = count;
  }
  if (!alternate) {
    int point = 0;
    while (point < marker && text[point] != '.') point++;
    if (point < marker) {
      int last = marker - 1;
      while (last > point && text[last] == '0') last--;
      if (last == point) last--;
      int removed = marker - last - 1;
      for (int index = marker; index <= count; index++) {
        text[index - removed] = text[index];
      }
      count -= removed;
    }
  }
  return count;
}

static int s21_print_float(S21Output* output, S21PrintSpec spec,
                           va_list* values) {
  long double number = spec.length == 'L' ? va_arg(*values, long double)
                                          : va_arg(*values, double);
  char prefix[2] = {0};
  int prefix_size = 0;
  if (signbit(number)) {
    prefix[prefix_size++] = '-';
  } else if (spec.plus) {
    prefix[prefix_size++] = '+';
  } else if (spec.blank) {
    prefix[prefix_size++] = ' ';
  }
  int precision = spec.precision_set ? spec.precision : 6;
  if (precision > S21_DECIMAL_LIMIT / 2) return -1;
  char* body = malloc(S21_DECIMAL_LIMIT + (s21_size_t)precision + 32);
  if (body == S21_NULL) return -1;
  int body_size = 0;
  int special = isnan(number) || isinf(number);
  if (special) {
    const char* label = isnan(number) ? "nan" : "inf";
    if (spec.conversion == 'E' || spec.conversion == 'F' ||
        spec.conversion == 'G') {
      label = isnan(number) ? "NAN" : "INF";
    }
    s21_memcpy(body, label, 3);
    body_size = 3;
  } else {
    S21ExactDecimal decimal;
    s21_decimal_from_float(&decimal, fabsl(number),
                           spec.length == 'L' ? LDBL_MANT_DIG : DBL_MANT_DIG);
    if (spec.conversion == 'e' || spec.conversion == 'E') {
      body_size = s21_format_scientific(&decimal, precision, spec.alternate,
                                        spec.conversion == 'E', body);
    } else if (spec.conversion == 'g' || spec.conversion == 'G') {
      body_size = s21_format_general(&decimal, precision, spec.alternate,
                                     spec.conversion == 'G', body);
    } else {
      body_size = s21_format_fixed(&decimal, precision, spec.alternate, body);
    }
  }
  s21_write_field(output, &spec, prefix, prefix_size, body, body_size, 0,
                  spec.zero && !spec.left && !special);
  free(body);
  return 0;
}

static void s21_print_string(S21Output* output, S21PrintSpec spec,
                             va_list* values) {
  const char* text = va_arg(*values, const char*);
  if (text == S21_NULL) {
#if defined(__GLIBC__)
    text = (!spec.precision_set || spec.precision >= 6) ? "(null)" : "";
#else
    text = "(null)";
#endif
  }
  int length = (int)s21_strlen(text);
  if (spec.precision_set && spec.precision < length) length = spec.precision;
  s21_write_field(output, &spec, "", 0, text, length, 0, 0);
}

static void s21_print_character(S21Output* output, S21PrintSpec spec,
                                va_list* values) {
  char symbol = (char)va_arg(*values, int);
  s21_write_field(output, &spec, "", 0, &symbol, 1, 0, 0);
}

static void s21_print_pointer(S21Output* output, S21PrintSpec spec,
                              va_list* values) {
  void* address = va_arg(*values, void*);
  if (address == S21_NULL) {
#if defined(__APPLE__)
    const char* empty = "0x0";
#elif defined(__linux__) && !defined(__GLIBC__)
    const char* empty = "0";
#else
    const char* empty = "(nil)";
#endif
    s21_write_field(output, &spec, "", 0, empty, (int)s21_strlen(empty), 0, 0);
  } else {
    char digits[sizeof(uintptr_t) * CHAR_BIT + 1];
    int count = s21_make_digits((uintptr_t)address, 16, 0, digits);
    int zeros = spec.precision_set && spec.precision > count
                    ? spec.precision - count
                    : 0;
    char prefix[3] = {0};
    int prefix_size = 0;
    if (spec.plus) {
      prefix[prefix_size++] = '+';
    } else if (spec.blank) {
      prefix[prefix_size++] = ' ';
    }
    prefix[prefix_size++] = '0';
    prefix[prefix_size++] = 'x';
    s21_write_field(output, &spec, prefix, prefix_size, digits, count, zeros,
                    spec.zero && !spec.left && !spec.precision_set);
  }
}

static void s21_print_count(S21Output* output, char length, va_list* values) {
  if (length == 'H') {
    *va_arg(*values, signed char*) = (signed char)output->written;
  } else if (length == 'h') {
    *va_arg(*values, short*) = (short)output->written;
  } else if (length == 'l') {
    *va_arg(*values, long*) = (long)output->written;
  } else if (length == 'q') {
    *va_arg(*values, long long*) = (long long)output->written;
  } else {
    *va_arg(*values, int*) = output->written;
  }
}

int s21_sprintf(char* destination, const char* format, ...) {
  if (destination == S21_NULL || format == S21_NULL) return -1;
  S21Output output = {destination, 0};
  va_list values;
  va_start(values, format);
  int error = 0;
  while (*format != '\0' && !error) {
    if (*format != '%') {
      s21_emit(&output, *format++);
      continue;
    }
    const char* conversion_start = format;
    format++;
    S21PrintSpec spec = s21_read_print_spec(&format, &values);
    switch (spec.conversion) {
      case 'd':
      case 'i':
      case 'u':
      case 'o':
      case 'x':
      case 'X':
        s21_print_integer(&output, spec, &values);
        break;
      case 'f':
      case 'F':
      case 'e':
      case 'E':
      case 'g':
      case 'G':
        error = s21_print_float(&output, spec, &values);
        break;
      case 's':
        s21_print_string(&output, spec, &values);
        break;
      case 'c':
        s21_print_character(&output, spec, &values);
        break;
      case 'p':
        s21_print_pointer(&output, spec, &values);
        break;
      case 'n':
        s21_print_count(&output, spec.length, &values);
        break;
      case '%':
        s21_emit(&output, '%');
        break;
      case '\0':
        error = -1;
        break;
      default:
        while (conversion_start < format) {
          s21_emit(&output, *conversion_start++);
        }
        break;
    }
  }
  va_end(values);
  output.destination[output.written] = '\0';
  return error ? -1 : output.written;
}
