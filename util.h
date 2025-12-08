#ifndef CJSON_UTIL_H
#define CJSON_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

enum CJSON_UtilError {
    CJSON_UTIL_ERROR_NONE,
    CJSON_UTIL_ERROR_WIN32API,
    CJSON_UTIL_ERROR_TOO_LARGE,
    CJSON_UTIL_ERROR_MALLOC,
    CJSON_UTIL_ERROR_FOPEN,
    CJSON_UTIL_ERROR_FREAD,
    CJSON_UTIL_ERROR_FSEEK,
    CJSON_UTIL_ERROR_FTELL
};

struct CJSON_Buffer {
    unsigned char *data;
    unsigned       size;
};

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define MIN(A,B) ((A)>(B)?(B):(A))
#define static_strlen(STR) (sizeof(STR) - 1)

#define VALID_2_BYTES_UTF16(CODEPOINT) (CODEPOINT < 0xD7FF || CODEPOINT >= 0xE000)
#define VALID_4_BYTES_UTF16(HIGH, LOW) (HIGH >= 0xD800 && HIGH <= 0xDBFF && LOW >= 0xDC00 && LOW <= 0xDFFF)

#define UNSIGNED_MAX_LENGTH 10U

bool         is_whitespace          (char c);
bool         is_delimiter           (char c);
bool         is_digit               (char c);
uint16_t     parse_codepoint        (const char *unicode, bool *success);
unsigned     codepoint_utf16_to_utf8(char *destination, uint16_t codepoint);
void         surrogate_utf16_to_utf8 (char *destination, uint16_t high, uint16_t low);
double       parse_float64          (const char *str, bool *success);
long double  parse_long_double      (const char *str, bool *success);
uint64_t     parse_uint64           (const char *str, bool *success);
int64_t      parse_int64            (const char *str, bool *success);
void         print_bytes            (const void *buffer, const size_t size);
long         usec_timestamp         (void);

void CJSON_Buffer_free(struct CJSON_Buffer*);

enum CJSON_UtilError file_get_contents(const char *path, struct CJSON_Buffer*);

#ifdef __cplusplus
}
#endif

#endif