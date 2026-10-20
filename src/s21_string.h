#ifndef S21_STRING_H_
#define S21_STRING_H_

#include <stddef.h>

#define S21_NULL ((void*)0)

typedef size_t s21_size_t;
typedef size_t s21_size;

void* s21_memchr(const void* buffer, int byte, s21_size_t count);
int s21_memcmp(const void* left, const void* right, s21_size_t count);
void* s21_memcpy(void* destination, const void* source, s21_size_t count);
void* s21_memset(void* buffer, int byte, s21_size_t count);
char* s21_strncat(char* destination, const char* source, s21_size_t count);
char* s21_strchr(const char* text, int symbol);
int s21_strncmp(const char* left, const char* right, s21_size_t count);
char* s21_strncpy(char* destination, const char* source, s21_size_t count);
s21_size_t s21_strcspn(const char* text, const char* reject);
char* s21_strerror(int number);
s21_size_t s21_strlen(const char* text);
char* s21_strpbrk(const char* text, const char* accept);
char* s21_strrchr(const char* text, int symbol);
char* s21_strstr(const char* text, const char* pattern);
char* s21_strtok(char* text, const char* delimiters);

int s21_sprintf(char* destination, const char* format, ...);
int s21_sscanf(const char* source, const char* format, ...);

void* s21_to_upper(const char* text);
void* s21_to_lower(const char* text);
void* s21_insert(const char* text, const char* addition, s21_size_t position);
void* s21_trim(const char* text, const char* characters);

#endif
