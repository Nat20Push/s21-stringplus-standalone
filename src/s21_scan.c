#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>

#include "s21_string.h"

#define S21_SCAN_WORDS 4096

typedef struct {
  uint32_t words[S21_SCAN_WORDS];
  int used;
  int lost;
} S21LargeNumber;

typedef struct {
  const char* position;
  int limit;
} S21Field;

typedef struct {
  int suppressed;
  int width;
  char length;
  char conversion;
  unsigned char allowed[256];
  int inverted;
} S21ScanSpec;

static int s21_space(char symbol) {
  return symbol == ' ' || symbol == '\t' || symbol == '\n' || symbol == '\r' ||
         symbol == '\v' || symbol == '\f';
}

static char s21_ascii_lower(char symbol) {
  return symbol >= 'A' && symbol <= 'Z' ? (char)(symbol - 'A' + 'a') : symbol;
}

static int s21_scan_digit(char symbol) {
  int digit = -1;
  if (symbol >= '0' && symbol <= '9') {
    digit = symbol - '0';
  } else if (symbol >= 'a' && symbol <= 'f') {
    digit = symbol - 'a' + 10;
  } else if (symbol >= 'A' && symbol <= 'F') {
    digit = symbol - 'A' + 10;
  }
  return digit;
}

static int s21_field_peek(const S21Field* field) {
  return field->limit > 0 ? (unsigned char)*field->position : 0;
}

static void s21_field_advance(S21Field* field) {
  field->position++;
  field->limit--;
}

static int s21_word(S21Field* field, const char* word) {
  S21Field saved = *field;
  while (*word != '\0' &&
         s21_ascii_lower((char)s21_field_peek(field)) == *word) {
    s21_field_advance(field);
    word++;
  }
  if (*word != '\0') *field = saved;
  return *word == '\0';
}

static void s21_large_trim(S21LargeNumber* number) {
  while (number->used > 0 && number->words[number->used - 1] == 0) {
    number->used--;
  }
}

static void s21_large_one(S21LargeNumber* number) {
  number->used = 1;
  number->words[0] = 1;
  number->lost = 0;
}

static void s21_large_multiply(S21LargeNumber* number, uint32_t factor,
                               uint32_t extra) {
  uint64_t carry = extra;
  for (int index = 0; index < number->used; index++) {
    uint64_t product = (uint64_t)number->words[index] * factor + carry;
    number->words[index] = (uint32_t)product;
    carry = product >> 32;
  }
  if (carry != 0) {
    if (number->used < S21_SCAN_WORDS) {
      number->words[number->used++] = (uint32_t)carry;
    } else {
      number->lost = 1;
    }
  }
}

static int s21_large_bits(const S21LargeNumber* number) {
  int count = number->used > 0 ? (number->used - 1) * 32 : 0;
  if (number->used > 0) {
    uint32_t top = number->words[number->used - 1];
    while (top != 0) {
      count++;
      top >>= 1;
    }
  }
  return count;
}

static int s21_large_compare(const S21LargeNumber* left,
                             const S21LargeNumber* right) {
  int result = 0;
  if (left->used < right->used) {
    result = -1;
  } else if (left->used > right->used) {
    result = 1;
  } else {
    for (int index = left->used - 1; index >= 0 && result == 0; index--) {
      if (left->words[index] < right->words[index]) result = -1;
      if (left->words[index] > right->words[index]) result = 1;
    }
  }
  return result;
}

static void s21_large_shift_left(S21LargeNumber* number, int shift) {
  if (number->used == 0 || shift <= 0) return;
  int whole = shift / 32;
  int partial = shift % 32;
  int old_count = number->used;
  if (old_count + whole + 1 >= S21_SCAN_WORDS) {
    number->lost = 1;
    return;
  }
  for (int index = old_count - 1; index >= 0; index--) {
    number->words[index + whole] = number->words[index];
  }
  for (int index = 0; index < whole; index++) number->words[index] = 0;
  number->used = old_count + whole;
  if (partial != 0) {
    uint32_t carry = 0;
    for (int index = whole; index < number->used; index++) {
      uint64_t value = ((uint64_t)number->words[index] << partial) | carry;
      number->words[index] = (uint32_t)value;
      carry = (uint32_t)(value >> 32);
    }
    if (carry != 0) number->words[number->used++] = carry;
  }
}

static void s21_large_shift_right_one(S21LargeNumber* number) {
  uint32_t carry = 0;
  for (int index = number->used - 1; index >= 0; index--) {
    uint32_t next = number->words[index] & 1U;
    number->words[index] = (number->words[index] >> 1) | (carry << 31);
    carry = next;
  }
  s21_large_trim(number);
}

static void s21_large_subtract(S21LargeNumber* left,
                               const S21LargeNumber* right) {
  uint64_t borrow = 0;
  for (int index = 0; index < left->used; index++) {
    uint64_t part = index < right->used ? right->words[index] : 0;
    uint64_t difference = (uint64_t)left->words[index] - part - borrow;
    left->words[index] = (uint32_t)difference;
    borrow = (difference >> 63) & 1U;
  }
  s21_large_trim(left);
}

static long double s21_exact_binary(const S21LargeNumber* mantissa,
                                    int binary_power, int decimal_power,
                                    int precision_bits, int emin, int emax) {
  if (mantissa->used == 0) return 0.0L;
  S21LargeNumber numerator = *mantissa;
  S21LargeNumber denominator = {0};
  s21_large_one(&denominator);
  int power = binary_power;
  if (decimal_power > 0) {
    for (int index = 0; index < decimal_power; index++) {
      s21_large_multiply(&numerator, 5, 0);
    }
    power += decimal_power;
  } else if (decimal_power < 0) {
    for (int index = 0; index < -decimal_power; index++) {
      s21_large_multiply(&denominator, 5, 0);
    }
    power += decimal_power;
  }
  int delta = s21_large_bits(&numerator) - s21_large_bits(&denominator);
  S21LargeNumber comparison = {0};
  if (delta >= 0) {
    comparison = denominator;
    s21_large_shift_left(&comparison, delta);
    if (s21_large_compare(&numerator, &comparison) < 0) delta--;
  } else {
    comparison = numerator;
    s21_large_shift_left(&comparison, -delta);
    if (s21_large_compare(&comparison, &denominator) < 0) delta--;
  }
  int exponent = delta + power;
  if (exponent > emax) return INFINITY;
  if (exponent < emin - precision_bits - 3) return 0.0L;
  int output_exponent = exponent < emin ? emin : exponent;
  int shift = power + precision_bits - 1 - output_exponent;
  S21LargeNumber scaled_num = numerator;
  S21LargeNumber scaled_den = denominator;
  if (shift >= 0) {
    s21_large_shift_left(&scaled_num, shift);
  } else {
    s21_large_shift_left(&scaled_den, -shift);
  }
  int quotient_bits = s21_large_bits(&scaled_num) - s21_large_bits(&scaled_den);
  uint64_t quotient_low = 0;
  uint64_t quotient_high = 0;
  if (quotient_bits >= 0) {
    S21LargeNumber top_divisor = scaled_den;
    s21_large_shift_left(&top_divisor, quotient_bits);
    if (s21_large_compare(&scaled_num, &top_divisor) < 0) quotient_bits--;
  }
  if (quotient_bits >= 0 && quotient_bits < 128) {
    S21LargeNumber divisor = scaled_den;
    s21_large_shift_left(&divisor, quotient_bits);
    for (int bit = quotient_bits; bit >= 0; bit--) {
      if (s21_large_compare(&scaled_num, &divisor) >= 0) {
        s21_large_subtract(&scaled_num, &divisor);
        if (bit < 64) {
          quotient_low |= UINT64_C(1) << bit;
        } else {
          quotient_high |= UINT64_C(1) << (bit - 64);
        }
      }
      s21_large_shift_right_one(&divisor);
    }
  }
  s21_large_shift_left(&scaled_num, 1);
  int halfway = s21_large_compare(&scaled_num, &scaled_den);
  if (halfway > 0 || (halfway == 0 && (quotient_low & 1U))) {
    quotient_low++;
    if (quotient_low == 0) quotient_high++;
  }
  if ((precision_bits < 64 &&
       quotient_low == (UINT64_C(1) << precision_bits)) ||
      (precision_bits >= 64 &&
       quotient_high == (UINT64_C(1) << (precision_bits - 64)))) {
    quotient_low = (quotient_low >> 1) | (quotient_high << 63);
    quotient_high >>= 1;
    output_exponent++;
  }
  if (output_exponent > emax) return INFINITY;
  long double rounded =
      ldexpl((long double)quotient_high, 64) + (long double)quotient_low;
  return ldexpl(rounded, output_exponent - precision_bits + 1);
}

static void s21_parse_scanset(const char** format, S21ScanSpec* spec) {
  if (**format == '^') {
    spec->inverted = 1;
    (*format)++;
  }
  if (**format == ']') {
    spec->allowed[(unsigned char)']'] = 1;
    (*format)++;
  }
  while (**format != '\0' && **format != ']') {
    unsigned char first = (unsigned char)*(*format)++;
    if (**format == '-' && (*format)[1] != '\0' && (*format)[1] != ']') {
      (*format)++;
      unsigned char last = (unsigned char)*(*format)++;
      if (first <= last) {
        for (int character = first; character <= last; character++) {
          spec->allowed[character] = 1;
        }
      } else {
        spec->allowed[first] = 1;
        spec->allowed[(unsigned char)'-'] = 1;
        spec->allowed[last] = 1;
      }
    } else {
      spec->allowed[first] = 1;
    }
  }
  if (**format == ']') (*format)++;
}

static S21ScanSpec s21_read_scan_spec(const char** format) {
  S21ScanSpec spec = {0};
  spec.width = INT_MAX;
  if (**format == '*') {
    spec.suppressed = 1;
    (*format)++;
  }
  if (**format >= '0' && **format <= '9') {
    spec.width = 0;
    while (**format >= '0' && **format <= '9') {
      int digit = *(*format)++ - '0';
      spec.width = spec.width > (INT_MAX - digit) / 10
                       ? INT_MAX
                       : spec.width * 10 + digit;
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
  if (spec.conversion == '[') s21_parse_scanset(format, &spec);
  return spec;
}

static void s21_store_integer(va_list* values, char length, int is_signed,
                              unsigned long long magnitude, int negative) {
  if (is_signed) {
    long long limit = negative ? LLONG_MIN : LLONG_MAX;
    long long result = 0;
    if (magnitude > (unsigned long long)LLONG_MAX + (unsigned)negative) {
      result = limit;
    } else if (negative) {
      result = magnitude == (unsigned long long)LLONG_MAX + 1
                   ? LLONG_MIN
                   : -(long long)magnitude;
    } else {
      result = (long long)magnitude;
    }
    if (length == 'H') {
      *va_arg(*values, signed char*) = (signed char)result;
    } else if (length == 'h') {
      *va_arg(*values, short*) = (short)result;
    } else if (length == 'l') {
      *va_arg(*values, long*) = (long)result;
    } else if (length == 'q') {
      *va_arg(*values, long long*) = result;
    } else {
      *va_arg(*values, int*) = (int)result;
    }
  } else {
    unsigned long long result = negative ? 0ULL - magnitude : magnitude;
    if (length == 'H') {
      *va_arg(*values, unsigned char*) = (unsigned char)result;
    } else if (length == 'h') {
      *va_arg(*values, unsigned short*) = (unsigned short)result;
    } else if (length == 'l') {
      *va_arg(*values, unsigned long*) = (unsigned long)result;
    } else if (length == 'q') {
      *va_arg(*values, unsigned long long*) = result;
    } else {
      *va_arg(*values, unsigned int*) = (unsigned int)result;
    }
  }
}

static int s21_scan_integer(S21Field* field, const S21ScanSpec* spec,
                            va_list* values) {
  if (s21_field_peek(field) == 0) return 0;
  int negative = 0;
  if (s21_field_peek(field) == '+' || s21_field_peek(field) == '-') {
    negative = s21_field_peek(field) == '-';
    s21_field_advance(field);
  }
  int base = 10;
  if (spec->conversion == 'o') base = 8;
  if (spec->conversion == 'x' || spec->conversion == 'X' ||
      spec->conversion == 'p') {
    base = 16;
  }
  if (spec->conversion == 'i' && s21_field_peek(field) == '0') base = 8;
  int digit_count = 0;
  unsigned long long magnitude = 0;
  int overflow = 0;
  if ((base == 8 || base == 16) && s21_field_peek(field) == '0') {
    s21_field_advance(field);
    digit_count = 1;
    if (s21_ascii_lower((char)s21_field_peek(field)) == 'x' &&
        (base == 16 || spec->conversion == 'i')) {
      base = 16;
      s21_field_advance(field);
    }
  }
  while (field->limit > 0) {
    int digit = s21_scan_digit((char)s21_field_peek(field));
    if (digit < 0 || digit >= base) break;
    if (magnitude > (ULLONG_MAX - (unsigned)digit) / (unsigned)base) {
      overflow = 1;
    } else if (!overflow) {
      magnitude = magnitude * (unsigned)base + (unsigned)digit;
    }
    digit_count++;
    s21_field_advance(field);
  }
  if (digit_count == 0) return 0;
  if (overflow) magnitude = ULLONG_MAX;
  if (!spec->suppressed) {
    if (spec->conversion == 'p') {
      *va_arg(*values, void**) = (void*)(uintptr_t)magnitude;
    } else {
      int is_signed = spec->conversion == 'd' || spec->conversion == 'i';
      s21_store_integer(values, spec->length, is_signed, magnitude,
                        overflow && !is_signed ? 0 : negative);
    }
  }
  return 1;
}

static int s21_read_float(S21Field* field, const S21ScanSpec* spec,
                          va_list* values) {
  if (s21_field_peek(field) == 0) return 0;
  int negative = 0;
  if (s21_field_peek(field) == '+' || s21_field_peek(field) == '-') {
    negative = s21_field_peek(field) == '-';
    s21_field_advance(field);
  }
  long double number = 0.0L;
  S21Field word = *field;
  if (s21_word(&word, "infinity") || s21_word(&word, "inf")) {
    *field = word;
    number = INFINITY;
  } else if (s21_word(&word, "nan")) {
    *field = word;
    number = NAN;
    if (s21_field_peek(field) == '(') {
      S21Field payload = *field;
      s21_field_advance(&payload);
      while (payload.limit > 0 && s21_field_peek(&payload) != 0 &&
             s21_field_peek(&payload) != ')') {
        s21_field_advance(&payload);
      }
      if (s21_field_peek(&payload) == ')') {
        s21_field_advance(&payload);
        *field = payload;
      }
    }
  } else {
    int radix = 10;
    if (field->limit > 2 && field->position[0] == '0' &&
        s21_ascii_lower(field->position[1]) == 'x') {
      int next = s21_scan_digit(field->position[2]);
      int after_dot = field->limit > 3 && field->position[2] == '.'
                          ? s21_scan_digit(field->position[3])
                          : -1;
      if ((next >= 0 && next < 16) || (after_dot >= 0 && after_dot < 16)) {
        radix = 16;
        s21_field_advance(field);
        s21_field_advance(field);
      }
    }
    S21LargeNumber significand = {0};
    int fractional_digits = 0;
    int all_digits = 0;
    int seen_dot = 0;
    int significant_digits = 0;
    while (field->limit > 0) {
      int symbol = s21_field_peek(field);
      if (symbol == '.' && !seen_dot) {
        seen_dot = 1;
        s21_field_advance(field);
      } else {
        int digit = s21_scan_digit((char)symbol);
        if (digit < 0 || digit >= radix) break;
        s21_large_multiply(&significand, (uint32_t)radix, (uint32_t)digit);
        if (seen_dot) fractional_digits++;
        if (digit != 0 || significant_digits != 0) significant_digits++;
        all_digits++;
        s21_field_advance(field);
      }
    }
    if (all_digits == 0) return 0;
    int explicit_power = 0;
    char marker = radix == 16 ? 'p' : 'e';
    if (s21_ascii_lower((char)s21_field_peek(field)) == marker) {
      S21Field before_exponent = *field;
      s21_field_advance(field);
      int exponent_negative = 0;
      if (s21_field_peek(field) == '+' || s21_field_peek(field) == '-') {
        exponent_negative = s21_field_peek(field) == '-';
        s21_field_advance(field);
      }
      int exponent_digits = 0;
      while (s21_field_peek(field) >= '0' && s21_field_peek(field) <= '9') {
        int digit = s21_field_peek(field) - '0';
        explicit_power =
            explicit_power >= 10000 ? 10000 : explicit_power * 10 + digit;
        exponent_digits++;
        s21_field_advance(field);
      }
      if (exponent_digits == 0) {
        *field = before_exponent;
      } else if (exponent_negative) {
        explicit_power = -explicit_power;
      }
    }
    int mantissa_bits = FLT_MANT_DIG;
    int emin = FLT_MIN_EXP - 1;
    int emax = FLT_MAX_EXP - 1;
    int max_decimal = FLT_MAX_10_EXP;
    int min_decimal = FLT_MIN_10_EXP;
    if (spec->length == 'l') {
      mantissa_bits = DBL_MANT_DIG;
      emin = DBL_MIN_EXP - 1;
      emax = DBL_MAX_EXP - 1;
      max_decimal = DBL_MAX_10_EXP;
      min_decimal = DBL_MIN_10_EXP;
    } else if (spec->length == 'L') {
      mantissa_bits = LDBL_MANT_DIG;
      emin = LDBL_MIN_EXP - 1;
      emax = LDBL_MAX_EXP - 1;
      max_decimal = LDBL_MAX_10_EXP;
      min_decimal = LDBL_MIN_10_EXP;
    }
    if (significand.used == 0) {
      number = 0.0L;
    } else if (radix == 10) {
      int decimal_place =
          significant_digits + explicit_power - fractional_digits;
      if (decimal_place > max_decimal + 4) {
        number = INFINITY;
      } else if (decimal_place < min_decimal - mantissa_bits - 4) {
        number = 0.0L;
      } else {
        number = s21_exact_binary(&significand, 0,
                                  explicit_power - fractional_digits,
                                  mantissa_bits, emin, emax);
      }
    } else {
      number =
          s21_exact_binary(&significand, explicit_power - 4 * fractional_digits,
                           0, mantissa_bits, emin, emax);
    }
  }
  if (negative) number = -number;
  if (!spec->suppressed) {
    if (spec->length == 'L') {
      *va_arg(*values, long double*) = number;
    } else if (spec->length == 'l') {
      *va_arg(*values, double*) = (double)number;
    } else {
      *va_arg(*values, float*) = (float)number;
    }
  }
  return 1;
}

static int s21_scan_text(S21Field* field, const S21ScanSpec* spec,
                         va_list* values) {
  const char* begin = field->position;
  int copied = 0;
  int maximum =
      spec->conversion == 'c' && spec->width == INT_MAX ? 1 : spec->width;
  while (copied < maximum && s21_field_peek(field) != 0) {
    unsigned char symbol = (unsigned char)s21_field_peek(field);
    if (spec->conversion == 's' && s21_space((char)symbol)) break;
    if (spec->conversion == '[' && spec->allowed[symbol] == spec->inverted) {
      break;
    }
    s21_field_advance(field);
    copied++;
  }
  if (copied == 0) return 0;
  if (!spec->suppressed) {
    char* destination = va_arg(*values, char*);
    s21_memcpy(destination, begin, (s21_size_t)copied);
    if (spec->conversion != 'c') destination[copied] = '\0';
  }
  return 1;
}

static void s21_store_count(va_list* values, char length, s21_size_t count) {
  if (length == 'H') {
    *va_arg(*values, signed char*) = (signed char)count;
  } else if (length == 'h') {
    *va_arg(*values, short*) = (short)count;
  } else if (length == 'l') {
    *va_arg(*values, long*) = (long)count;
  } else if (length == 'q') {
    *va_arg(*values, long long*) = (long long)count;
  } else {
    *va_arg(*values, int*) = (int)count;
  }
}

int s21_sscanf(const char* source, const char* format, ...) {
  if (source == S21_NULL || format == S21_NULL) return -1;
  const char* position = source;
  va_list values;
  va_start(values, format);
  int assigned = 0;
  int failed = 0;
  int input_ended = 0;
  while (*format != '\0' && !failed) {
    if (s21_space(*format)) {
      while (s21_space(*format)) format++;
      while (s21_space(*position)) position++;
    } else if (*format != '%') {
      if (*position == *format) {
        position++;
        format++;
      } else {
        input_ended = *position == '\0';
        failed = 1;
      }
    } else {
      format++;
      S21ScanSpec spec = s21_read_scan_spec(&format);
      if (spec.conversion == '\0') {
        input_ended = *position == '\0';
        failed = 1;
        continue;
      }
      if (spec.conversion == '%') {
        if (*position == '%') {
          position++;
        } else {
          input_ended = *position == '\0';
          failed = 1;
        }
        continue;
      }
      if (spec.conversion != 'c' && spec.conversion != '[' &&
          spec.conversion != 'n') {
        while (s21_space(*position)) position++;
      }
      if (spec.conversion == 'n') {
        if (!spec.suppressed) {
          s21_store_count(&values, spec.length,
                          (s21_size_t)(position - source));
        }
        continue;
      }
      S21Field field = {position, spec.width};
      int success = 0;
      if (spec.conversion == 'd' || spec.conversion == 'i' ||
          spec.conversion == 'u' || spec.conversion == 'o' ||
          spec.conversion == 'x' || spec.conversion == 'X' ||
          spec.conversion == 'p') {
        if (spec.conversion == 'p' && s21_word(&field, "(nil)")) {
          if (!spec.suppressed) *va_arg(values, void**) = S21_NULL;
          success = 1;
        } else {
          success = s21_scan_integer(&field, &spec, &values);
        }
      } else if (spec.conversion == 'f' || spec.conversion == 'F' ||
                 spec.conversion == 'e' || spec.conversion == 'E' ||
                 spec.conversion == 'g' || spec.conversion == 'G' ||
                 spec.conversion == 'a' || spec.conversion == 'A') {
        success = s21_read_float(&field, &spec, &values);
      } else if (spec.conversion == 's' || spec.conversion == 'c' ||
                 spec.conversion == '[') {
        success = s21_scan_text(&field, &spec, &values);
      }
      if (success) {
        position = field.position;
        if (!spec.suppressed) assigned++;
      } else {
        input_ended = *position == '\0';
        failed = 1;
      }
    }
  }
  va_end(values);
  return assigned == 0 && failed && input_ended ? -1 : assigned;
}
