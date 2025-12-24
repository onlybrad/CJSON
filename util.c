#include "util.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/time.h>
#endif
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#include "allocator.h"

static int CJSON_fseek(FILE *const file, const int64_t offset, const int origin) {
#ifdef _WIN32
    return _fseeki64(file, (__int64)offset, origin);
#elif LONG_MAX < LLONG_MAX
    return fseeko(file, (off_t)offset, origin);
#else
    return fseek(file, (long)offset, origin);
#endif
}

static int64_t CJSON_ftell(FILE *const file) {
#ifdef _WIN32
    return (int64_t)_ftelli64(file);
#elif LONG_MAX < LLONG_MAX
    return (int64_t)ftello(file);
#else
    return (int64_t)ftell(file);
#endif    
}


EXTERN_C bool is_whitespace(const char c) {
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

EXTERN_C bool is_delimiter(const char c) {
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

EXTERN_C bool is_digit(const char c) {
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

EXTERN_C uint16_t hex_to_utf16(const char *const codepoint, bool *const success) {
    assert(codepoint != NULL);
    assert(success != NULL);

    char hex[7] = "0x";
    memcpy(hex + 2, codepoint, 4U);
    char *end_ptr;
    errno = 0;

    const uint16_t ret = (uint16_t)strtoul(hex, &end_ptr, 16);
    if(end_ptr == hex || *end_ptr != '\0' || errno == ERANGE) {
        *success = false;

        return (uint16_t)0U;
    }

    *success = true;

    return ret;
}

EXTERN_C unsigned utf16_to_utf8_2bytes(char *const destination, const uint16_t high) {
    assert(destination != NULL);

    if(high <= 0x7F) {
        destination[0] = (char)(high & 0x7f);
        return 1U;
    } else if(high <= 0x7FF) {
        destination[0] = (char)((high  >> 6)         | 0xC0);
        destination[1] = (char)(((high >> 0) & 0x3F) | 0x80);
        return 2U;
    } else {
        destination[0] = (char)((high  >> 12)        | 0xE0);
        destination[1] = (char)(((high >> 6) & 0x3F) | 0x80);
        destination[2] = (char)(((high >> 0) & 0x3F) | 0x80);
        return 3U;
    }
}

EXTERN_C void utf16_to_utf8_4bytes(char *const destination, const uint16_t high, const uint16_t low) {
    assert(destination != NULL);

    const uint32_t codepoint = (uint32_t)(((high - 0xD800) << 10) | (low - 0xDC00)) + 0x10000;

    destination[0] = (char)((codepoint  >> 18)         | 0xF0);
    destination[1] = (char)(((codepoint >> 12) & 0x3F) | 0x80);
    destination[2] = (char)(((codepoint >> 6)  & 0x3F) | 0x80);
    destination[3] = (char)(((codepoint >> 0)  & 0x3F) | 0x80);
}

EXTERN_C double parse_float64(const char *const str, bool *const success) {
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

EXTERN_C long double parse_long_double(const char *const str, bool *const success) {
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

EXTERN_C uint64_t parse_uint64(const char *const str, bool *const success) {
    assert(str != NULL);
    assert(success != NULL);
    
    char *end_ptr;
    errno = 0;

    const uint64_t ret = strtoull(str, &end_ptr, 10);
    if(end_ptr == str || *end_ptr != '\0' || errno == ERANGE) {
        *success = false;

        return (uint64_t)0;
    }

    *success = true;

    return ret;
}

EXTERN_C int64_t parse_int64(const char *const str, bool *const success) {
    assert(str != NULL);
    assert(success != NULL);

    char *end_ptr;
    errno = 0;
    
    const int64_t ret = strtoll(str, &end_ptr, 10);
    if(end_ptr == str || *end_ptr != '\0' || errno == ERANGE) {
        *success = false;

        return (int64_t)0;
    }

    *success = true;
    
    return ret;
}

EXTERN_C void print_bytes(const void *const buffer, const size_t size) {
    assert(buffer != NULL);
    assert(size > 0);

    putchar('[');
    for(size_t i = 0; i < size - 1; i++) {
        printf("0x%02hhx, ", ((const unsigned char*)buffer)[i]);
    }
    printf("0x%02hhx]\n", ((const unsigned char*)buffer)[size - 1]);
}

EXTERN_C void CJSON_Buffer_free(struct CJSON_Buffer *const buffer) {
    assert(buffer != NULL);

    CJSON_FREE(buffer->data);
    buffer->data = NULL;
    buffer->size = 0U;
}

EXTERN_C enum CJSON_UtilError file_get_contents(const char *const path, struct CJSON_Buffer *const buffer) {
    assert(path != NULL);
    assert(path[0] != '\0');

    buffer->data = NULL;
    buffer->size = 0U;

#ifdef _WIN32
    const int wide_length = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
    if(wide_length == 0) {
        return CJSON_UTIL_ERROR_WIN32API;
    }

    wchar_t *const wpath = (wchar_t*)CJSON_MALLOC((size_t)wide_length * sizeof(wchar_t));
    if(wpath == NULL) {
        return CJSON_UTIL_ERROR_MALLOC;
    }

    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, wide_length);

    FILE *const file = _wfopen(wpath, L"rb");

    CJSON_FREE(wpath);
#else
    FILE *const file = fopen(path, "rb");
#endif
    enum CJSON_UtilError error;
    int64_t length;

    do {
        if(file == NULL) {
            error = CJSON_UTIL_ERROR_FOPEN;
            break;
        }

        if(CJSON_fseek(file, 0, SEEK_END) != 0) {
            error = CJSON_UTIL_ERROR_FSEEK;
            break;
        }

        length = CJSON_ftell(file);
        if(length == -1) {
            error = CJSON_UTIL_ERROR_FTELL;
            break;
        }

        if((uintmax_t)length >= (uintmax_t)UINT_MAX) {
            error = CJSON_UTIL_ERROR_TOO_LARGE;
            break;
        }

        if(CJSON_fseek(file, 0, SEEK_SET) != 0) {
            error = CJSON_UTIL_ERROR_FSEEK;
            break;
        }

        //the buffer returned has 1 extra byte allocated in case a null terminated string is required
        buffer->data = (unsigned char*)CJSON_MALLOC(((size_t)length + 1) * sizeof(unsigned char));
        if(buffer->data == NULL) {
            error = CJSON_UTIL_ERROR_MALLOC;
            break;
        }
        buffer->data[length] = '\0';
        
        if(fread(buffer->data, sizeof(unsigned char), (size_t)length, file) != (size_t)length) {
            error = CJSON_UTIL_ERROR_FREAD;
            break;
        }

        error = CJSON_UTIL_ERROR_NONE;
        buffer->size = (unsigned)length;
    } while(0);

    if(error != CJSON_UTIL_ERROR_NONE) {
        CJSON_Buffer_free(buffer);
    }
    
    fclose(file);
    return error;
}

EXTERN_C unsigned long long usec_timestamp(void) {
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    unsigned long long tt = ft.dwHighDateTime;
    tt <<= 32ULL;
    tt |= ft.dwLowDateTime;
    tt /= 10ULL;
    tt -= 11644473600000000ULL;
    return tt;
#else
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    return (unsigned long long)(current_time.tv_sec * 1000000L + current_time.tv_usec);
#endif
}
