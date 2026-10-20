CC = gcc
AR = ar
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic
CHECK_CFLAGS = $(shell pkg-config --cflags check 2>/dev/null)
CHECK_LIBS = $(shell pkg-config --libs check 2>/dev/null)
GCOV ?= gcov
LCOV_IGNORE_ERRORS = $(shell lcov --version 2>/dev/null | awk -F'[ .]' '$$3 == "version" && $$4 >= 2 {print "--ignore-errors mismatch"}')
ifeq ($(strip $(CHECK_LIBS)),)
CHECK_LIBS = -lcheck -lsubunit -lm -lpthread -lrt
endif
ifeq ($(shell uname -s),Darwin)
TEST_WARNINGS = -Wno-gnu-zero-variadic-macro-arguments -Wno-deprecated-declarations
endif

SOURCES = s21_string.c s21_special.c s21_strerror.c s21_format.c s21_scan.c
OBJECTS = $(SOURCES:.c=.o)
TEST_FILES = tests/test_main.c tests/test_string_edges.c \
	tests/test_format_edges.c tests/test_scan_edges.c

.PHONY: all clean test gcov_report

all: s21_string.a

s21_string.a: $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)

%.o: %.c s21_string.h
	$(CC) $(CFLAGS) -c $< -o $@

test: s21_string.a
	$(CC) $(CFLAGS) $(TEST_WARNINGS) $(CHECK_CFLAGS) -I. $(TEST_FILES) \
		s21_string.a -o s21_tests $(CHECK_LIBS) -lm
	./s21_tests

gcov_report:
	rm -f s21_coverage-*.gcda s21_coverage-*.gcno coverage.raw.info coverage.info
	$(CC) $(CFLAGS) $(TEST_WARNINGS) --coverage $(CHECK_CFLAGS) -I. \
		$(TEST_FILES) $(SOURCES) -o s21_coverage $(CHECK_LIBS) -lm
	CK_FORK=no ./s21_coverage
	lcov --capture --directory . --gcov-tool $(GCOV) --output-file coverage.raw.info \
		$(LCOV_IGNORE_ERRORS)
	lcov --extract coverage.raw.info '*/s21_*.c' --output-file coverage.info
	genhtml coverage.info --output-directory report $(LCOV_IGNORE_ERRORS)

clean:
	rm -rf $(OBJECTS) s21_string.a s21_tests s21_coverage \
		s21_coverage-*.gcda s21_coverage-*.gcno coverage.raw.info \
		coverage.info report
