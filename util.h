#ifndef JSON_UTIL_H
#define JSON_UTIL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define MIN(A,B) ((A)>(B)?(B):(A))
#define static_strlen(STR) (sizeof(STR) - 1)

#define VALID_2_BYTES_UTF16(CODEPOINT) (CODEPOINT < 0xD7FF || CODEPOINT >= 0xE000)
#define VALID_4_BYTES_UTF16(HIGH, LOW) (HIGH >= 0xD800 && HIGH <= 0xDBFF && LOW >= 0xDC00 && LOW <= 0xDFFF)

#define UNSIGNED_MAX_LENGTH 10U

bool        is_whitespace          (const char c);
bool        is_delimiter           (const char c);
bool        is_digit               (const char c);
uint16_t    parse_codepoint        (const char *const unicode, bool *const success);
unsigned int    codepoint_utf16_to_utf8(char *const destination, const uint16_t codepoint);
void        surrogate_utf16_to_utf8(char *const destination, const uint16_t high, const uint16_t low);
double      parse_float64          (const char *const str, bool *const success);
long double parse_long_double      (const char *const str, bool *const success);
uint64_t    parse_uint64           (const char *const str, bool *const success);
int64_t     parse_int64            (const char *const str, bool *const success);
void        print_bytes            (void *const buffer, size_t size);
char       *file_get_contents      (const char *const path, size_t *const filesize);
long        usec_timestamp         (void);     

#endif