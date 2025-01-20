#ifdef _WIN32
#include <windows.h>
#endif
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>
#include "util.h"

inline bool is_whitespace(const char c) {
    switch(c) {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
        return true;
    default:
        return false;
    }
}

inline bool is_delimiter(const char c) {
    switch(c) {
    case '[':
    case ']':
    case '}':
    case '{':
    case ',':
    case ':':
        return true;
    default:
        return false;
    }
}

inline bool is_digit(const char c) {
    switch(c) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return true;
    default:
        return false;
    }
}

uint16_t parse_codepoint(const char *const codepoint, bool *const success) {
    assert(codepoint != NULL);
    assert(success != NULL);

    char hex[7] = "0x";
    memcpy(hex + 2, codepoint, 4);
    char *end_ptr;
    errno = 0;

    const uint16_t ret = (uint16_t)strtoul(hex, &end_ptr, 16);
    if(end_ptr == hex || *end_ptr != '\0' || errno == ERANGE) {
        *success = false;
        return (uint16_t)0;
    }

    *success = true;
    return ret;
}

unsigned int codepoint_utf16_to_utf8(char *const destination, const uint16_t codepoint) {
    assert(destination != NULL);

    if(codepoint <= 0x7F) {
        destination[0] = (char)(codepoint & 0x7f);
        return 1U;
    } else if(codepoint <= 0x7FF) {
        destination[0] = (char)((codepoint  >> 6)         | 0xC0);
        destination[1] = (char)(((codepoint >> 0) & 0x3F) | 0x80);
        return 2U;
    } else {
        destination[0] = (char)((codepoint  >> 12)        | 0xE0);
        destination[1] = (char)(((codepoint >> 6) & 0x3F) | 0x80);
        destination[2] = (char)(((codepoint >> 0) & 0x3F) | 0x80);
        return 3U;
    }
}

void surrogate_utf16_to_utf8(char *const destination, const uint16_t high, const uint16_t low) {
    assert(destination != NULL);

    const uint32_t codepoint = (uint32_t)(((high - 0xD800) << 10) | (low - 0xDC00)) + 0x10000;

    destination[0] = (char)((codepoint  >> 18)         | 0xF0);
    destination[1] = (char)(((codepoint >> 12) & 0x3F) | 0x80);
    destination[2] = (char)(((codepoint >> 6)  & 0x3F) | 0x80);
    destination[3] = (char)(((codepoint >> 0)  & 0x3F) | 0x80);
}

double parse_float64(const char *const str, bool *const success) {
    assert(str != NULL);
    assert(success != NULL);

    char *end_ptr;
    errno = 0;
    
    const double ret = strtod(str, &end_ptr);
    if(end_ptr == str || *end_ptr != '\0' || errno == ERANGE) {
        *success = false;
        return 0.0;
    }

    *success = true;
    return ret;
}

long double parse_long_double(const char *const str, bool *const success) {
    assert(str != NULL);
    assert(success != NULL);

    char *end_ptr;
    errno = 0;
    
    const long double ret = strtold(str, &end_ptr);
    if(end_ptr == str || *end_ptr != '\0' || errno == ERANGE) {
        *success = false;
        return 0.0L;
    }

    *success = true;
    return ret;
}

uint64_t parse_uint64(const char *const str, bool *const success) {
    assert(str != NULL);
    assert(success != NULL);

    char *end_ptr;
    errno = 0;
    
    const uint64_t ret = strtoull(str, &end_ptr, 10);
    if(end_ptr == str || *end_ptr != '\0' || errno == ERANGE) {
        *success = false;
        return 0ULL;
    }

    *success = true;
    return ret;
}

int64_t parse_int64(const char *const str, bool *const success) {
    assert(str != NULL);
    assert(success != NULL);

    char *end_ptr;
    errno = 0;
    
    const int64_t ret = strtoll(str, &end_ptr, 10);
    if(end_ptr == str || *end_ptr != '\0' || errno == ERANGE) {
        *success = false;
        return 0LL;
    }

    *success = true;
    return ret;
}

void print_bytes(const void *const buffer, const size_t size) {
    assert(buffer != NULL);
    assert(size > 0);

    putchar('[');
    for(size_t i = 0; i < size - 1; i++) {
        printf("0x%02hhx, ", ((const unsigned char*)buffer)[i]);
    }
    printf("0x%02hhx]\n", ((const unsigned char*)buffer)[size - 1]);
}

long usec_timestamp(void) {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);

    return current_time.tv_sec * 1000000L + current_time.tv_usec;
}

char *file_get_contents(const char *const path, size_t *const filesize) {
    assert(path != NULL);
    assert(strlen(path) > 0);
    assert(filesize != NULL);

#ifdef _WIN32
    const int wide_length = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
    if(wide_length == 0) {
        *filesize = 0;
        return NULL;
    }

    wchar_t *wpath = malloc((size_t)wide_length * sizeof(wchar_t));
    assert(wpath != NULL);

    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, wide_length);

    FILE *const file = _wfopen(wpath, L"rb");

    free(wpath);
#else
    FILE *const file = fopen(path, "rb");
#endif
    if(file == NULL) {
        *filesize = 0;
        return NULL;
    }

    fseeko(file, 0, SEEK_END);
    const off_t length = ftello(file);
    fseeko(file, 0, SEEK_SET);
    //the buffer returned has 1 extra byte allocated in case a null terminated string is required
    char *data = malloc(((size_t)length + 1) * sizeof(char));
    assert(data);
    fread(data, sizeof(char), (size_t)length, file);
    fclose(file);

    *filesize = (size_t)length;
    return data;
}
