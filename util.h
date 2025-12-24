#ifdef __cplusplus
extern "C" {
#endif

#ifndef CJSON_UTIL_H
#define CJSON_UTIL_H

#ifndef _WIN32
    #define _FILE_OFFSET_BITS 64
    #include <unistd.h>
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

#define IS_VALID_2_BYTES_UTF16(CODEPOINT) (CODEPOINT < 0xD7FF || CODEPOINT >= 0xE000)
#define IS_VALID_4_BYTES_UTF16(HIGH, LOW) (HIGH >= 0xD800 && HIGH <= 0xDBFF && LOW >= 0xDC00 && LOW <= 0xDFFF)

#define UNSIGNED_MAX_LENGTH 10U

bool               is_whitespace       (char c);
bool               is_delimiter        (char c);
bool               is_digit            (char c);
uint16_t           hex_to_utf16        (const char *unicode, bool *success);
unsigned           utf16_to_utf8_2bytes(char *destination, uint16_t codepoint);
void               utf16_to_utf8_4bytes(char *destination, uint16_t high, uint16_t low);
double             parse_float64       (const char *str, bool *success);
long double        parse_long_double   (const char *str, bool *success);
uint64_t           parse_uint64        (const char *str, bool *success);
int64_t            parse_int64         (const char *str, bool *success);
void               print_bytes         (const void *buffer, const size_t size);
unsigned long long usec_timestamp      (void);

void CJSON_Buffer_free(struct CJSON_Buffer*);

enum CJSON_UtilError file_get_contents(const char *path, struct CJSON_Buffer*);

#endif

#ifdef __cplusplus
}
#endif