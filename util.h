#ifdef __cplusplus
extern "C" {
#endif

#ifndef CJSON_UTIL_H
#define CJSON_UTIL_H

#ifndef _WIN32
    #define _FILE_OFFSET_BITS 64
    #include <unistd.h>
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifndef EXTERN_C
#ifdef __cplusplus
    #define EXTERN_C extern "C"
#else
    #define EXTERN_C
#endif
#endif

#ifndef MIN
    #define MIN(A,B) ((A)>(B)?(B):(A))
#endif
#ifndef MAX
    #define MAX(A,B) ((A)>(B)?(A):(B))
#endif

#ifndef static_strlen
    #define static_strlen(STR) (sizeof(STR) - 1)
#endif

#ifndef UNSIGNED_TO_VOID_PTR
    #define UNSIGNED_TO_VOID_PTR(UNSIGNED)((void*)(uintptr_t)(UNSIGNED))
#endif
#ifndef VOID_PTR_TO_UNSIGNED
    #define VOID_PTR_TO_UNSIGNED(VOID_PTR)((unsigned)(uintptr_t)(VOID_PTR))
#endif

#ifndef DBL_PRECISION
    #ifdef DBL_DECIMAL_DIG
        #define DBL_PRECISION DBL_DECIMAL_DIG
    #else
        #define DBL_PRECISION DBL_DIG
    #endif
#endif

#define IS_VALID_2_BYTES_UTF16(CODEPOINT) (CODEPOINT < 0xD7FF || CODEPOINT >= 0xE000)
#define IS_VALID_4_BYTES_UTF16(HIGH, LOW) (HIGH >= 0xD800 && HIGH <= 0xDBFF && LOW >= 0xDC00 && LOW <= 0xDFFF)

bool        CJSON_is_whitespace       (char c);
bool        CJSON_is_delimiter        (char c);
bool        CJSON_is_digit            (char c);
bool        CJSON_is_control_char     (char c);
uint16_t    CJSON_hex_to_utf16        (const char *unicode, bool *success);
unsigned    CJSON_utf16_to_utf8_2bytes(char *destination, uint16_t codepoint);
void        CJSON_utf16_to_utf8_4bytes(char *destination, uint16_t high, uint16_t low);
double      CJSON_parse_float64       (const char *str, bool *success);
long double CJSON_parse_long_double   (const char *str, bool *success);
uint64_t    CJSON_parse_uint64        (const char *str, bool *success);
int64_t     CJSON_parse_int64         (const char *str, bool *success);
void        CJSON_print_bytes         (const void *buffer, const size_t size);
uint64_t    CJSON_usec_timestamp      (void);

unsigned CJSON_safe_unsigned_mult          (unsigned a, unsigned b, bool *success);
bool     CJSON_check_unsigned_mult_overflow(unsigned a, unsigned b);

#endif

#ifdef __cplusplus
}
#endif