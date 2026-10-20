#include <stdlib.h>

#include "s21_string.h"

static int s21_is_trimmed(char symbol, const char* characters) {
  return s21_strchr(characters, symbol) != S21_NULL;
}

void* s21_to_upper(const char* text) {
  if (text == S21_NULL) return S21_NULL;
  s21_size_t length = s21_strlen(text);
  char* copy = malloc(length + 1);
  if (copy != S21_NULL) {
    for (s21_size_t index = 0; index < length; index++) {
      char current = text[index];
      copy[index] = current >= 'a' && current <= 'z'
                        ? (char)(current - 'a' + 'A')
                        : current;
    }
    copy[length] = '\0';
  }
  return copy;
}

void* s21_to_lower(const char* text) {
  if (text == S21_NULL) return S21_NULL;
  s21_size_t length = s21_strlen(text);
  char* copy = malloc(length + 1);
  if (copy != S21_NULL) {
    for (s21_size_t index = 0; index < length; index++) {
      char current = text[index];
      copy[index] = current >= 'A' && current <= 'Z'
                        ? (char)(current - 'A' + 'a')
                        : current;
    }
    copy[length] = '\0';
  }
  return copy;
}

void* s21_insert(const char* text, const char* addition, s21_size_t position) {
  if (text == S21_NULL || addition == S21_NULL) return S21_NULL;
  s21_size_t original_length = s21_strlen(text);
  s21_size_t added_length = s21_strlen(addition);
  if (position > original_length ||
      added_length > (s21_size_t)-1 - original_length - 1) {
    return S21_NULL;
  }
  char* copy = malloc(original_length + added_length + 1);
  if (copy != S21_NULL) {
    s21_memcpy(copy, text, position);
    s21_memcpy(copy + position, addition, added_length);
    s21_memcpy(copy + position + added_length, text + position,
               original_length - position + 1);
  }
  return copy;
}

void* s21_trim(const char* text, const char* characters) {
  if (text == S21_NULL) return S21_NULL;
  if (characters == S21_NULL || *characters == '\0') {
    characters = " \t\r\n\v\f";
  }
  s21_size_t first = 0;
  s21_size_t last = s21_strlen(text);
  while (first < last && s21_is_trimmed(text[first], characters)) first++;
  while (last > first && s21_is_trimmed(text[last - 1], characters)) last--;
  s21_size_t length = last - first;
  char* copy = malloc(length + 1);
  if (copy != S21_NULL) {
    s21_memcpy(copy, text + first, length);
    copy[length] = '\0';
  }
  return copy;
}
