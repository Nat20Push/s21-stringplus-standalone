#include "s21_string.h"

void* s21_memchr(const void* buffer, int byte, s21_size_t count) {
  const unsigned char* data = buffer;
  void* found = S21_NULL;
  for (s21_size_t index = 0; index < count && found == S21_NULL; index++) {
    if (data[index] == (unsigned char)byte) found = (void*)(data + index);
  }
  return found;
}

int s21_memcmp(const void* left, const void* right, s21_size_t count) {
  const unsigned char* first = left;
  const unsigned char* second = right;
  int difference = 0;
  for (s21_size_t index = 0; index < count && difference == 0; index++) {
    difference = (int)first[index] - (int)second[index];
  }
  return difference;
}

void* s21_memcpy(void* destination, const void* source, s21_size_t count) {
  unsigned char* output = destination;
  const unsigned char* input = source;
  for (s21_size_t index = 0; index < count; index++)
    output[index] = input[index];
  return destination;
}

void* s21_memset(void* buffer, int byte, s21_size_t count) {
  unsigned char* output = buffer;
  for (s21_size_t index = 0; index < count; index++) {
    output[index] = (unsigned char)byte;
  }
  return buffer;
}

s21_size_t s21_strlen(const char* text) {
  s21_size_t length = 0;
  while (text[length] != '\0') length++;
  return length;
}

char* s21_strncat(char* destination, const char* source, s21_size_t count) {
  s21_size_t offset = s21_strlen(destination);
  s21_size_t index = 0;
  while (index < count && source[index] != '\0') {
    destination[offset + index] = source[index];
    index++;
  }
  destination[offset + index] = '\0';
  return destination;
}

char* s21_strchr(const char* text, int symbol) {
  unsigned char expected = (unsigned char)symbol;
  while ((unsigned char)*text != expected && *text != '\0') text++;
  return (unsigned char)*text == expected ? (char*)text : S21_NULL;
}

int s21_strncmp(const char* left, const char* right, s21_size_t count) {
  int difference = 0;
  for (s21_size_t index = 0; index < count && difference == 0; index++) {
    difference = (unsigned char)left[index] - (unsigned char)right[index];
    if (left[index] == '\0') break;
  }
  return difference;
}

char* s21_strncpy(char* destination, const char* source, s21_size_t count) {
  s21_size_t index = 0;
  while (index < count && source[index] != '\0') {
    destination[index] = source[index];
    index++;
  }
  while (index < count) destination[index++] = '\0';
  return destination;
}

s21_size_t s21_strcspn(const char* text, const char* reject) {
  s21_size_t length = 0;
  while (text[length] != '\0' && s21_strchr(reject, text[length]) == S21_NULL) {
    length++;
  }
  return length;
}

char* s21_strpbrk(const char* text, const char* accept) {
  char* found = S21_NULL;
  while (*text != '\0' && found == S21_NULL) {
    if (s21_strchr(accept, *text) != S21_NULL) found = (char*)text;
    text++;
  }
  return found;
}

char* s21_strrchr(const char* text, int symbol) {
  const char* last = S21_NULL;
  unsigned char expected = (unsigned char)symbol;
  do {
    if ((unsigned char)*text == expected) last = text;
  } while (*text++ != '\0');
  return (char*)last;
}

char* s21_strstr(const char* text, const char* pattern) {
  char* found = S21_NULL;
  s21_size_t length = s21_strlen(pattern);
  if (length == 0) found = (char*)text;
  while (*text != '\0' && found == S21_NULL) {
    if (s21_strncmp(text, pattern, length) == 0) found = (char*)text;
    text++;
  }
  return found;
}

char* s21_strtok(char* text, const char* delimiters) {
  static char* cursor = S21_NULL;
  if (text != S21_NULL) cursor = text;
  if (cursor == S21_NULL) return S21_NULL;
  while (*cursor != '\0' && s21_strchr(delimiters, *cursor) != S21_NULL) {
    cursor++;
  }
  if (*cursor == '\0') {
    cursor = S21_NULL;
    return S21_NULL;
  }
  char* token = cursor;
  while (*cursor != '\0' && s21_strchr(delimiters, *cursor) == S21_NULL) {
    cursor++;
  }
  if (*cursor == '\0') {
    cursor = S21_NULL;
  } else {
    *cursor++ = '\0';
  }
  return token;
}
