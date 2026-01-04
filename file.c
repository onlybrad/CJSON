#include "file.h"

#ifdef _WIN32
    #include <windows.h>
#endif
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#include "util.h"
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

EXTERN_C void CJSON_FileContents_init(struct CJSON_FileContents *const file_contents) {
    assert(file_contents != NULL);

    file_contents->data = NULL;
    file_contents->size = 0U;
}

void CJSON_FileContents_free(struct CJSON_FileContents *const file_contents) {
    assert(file_contents != NULL);

    CJSON_FREE(file_contents->data);
    CJSON_FileContents_init(file_contents);
}

EXTERN_C enum CJSON_FileContents_Error CJSON_FileContents_get(struct CJSON_FileContents *const file_contents, const char *const path) {
    assert(path != NULL);
    assert(path[0] != '\0');

#ifdef _WIN32
    const int wide_length = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
    if(wide_length == 0) {
        return CJSON_FILECONTENTS_ERROR_WIN32API;
    }

    wchar_t *const wpath = (wchar_t*)CJSON_MALLOC((size_t)wide_length * sizeof(wchar_t));
    if(wpath == NULL) {
        return CJSON_FILECONTENTS_ERROR_MEMORY;
    }

    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, wide_length);

    FILE *const file = _wfopen(wpath, L"rb");

    CJSON_FREE(wpath);
#else
    FILE *const file = fopen(path, "rb");
#endif
    enum CJSON_FileContents_Error error;
    int64_t length;

    do {
        if(file == NULL) {
            error = CJSON_FILECONTENTS_ERROR_FOPEN;
            break;
        }

        if(CJSON_fseek(file, 0, SEEK_END) != 0) {
            error = CJSON_FILECONTENTS_ERROR_FSEEK;
            break;
        }

        length = CJSON_ftell(file);
        if(length == -1) {
            error = CJSON_FILECONTENTS_ERROR_FTELL;
            break;
        }

        if((uintmax_t)length >= (uintmax_t)UINT_MAX) {
            error = CJSON_FILECONTENTS_ERROR_TOO_LARGE;
            break;
        }

        if(CJSON_fseek(file, 0, SEEK_SET) != 0) {
            error = CJSON_FILECONTENTS_ERROR_FSEEK;
            break;
        }

        //the buffer returned has 1 extra byte allocated in case a null terminated string is required
        file_contents->data = (unsigned char*)CJSON_MALLOC(((size_t)length + 1) * sizeof(unsigned char));
        if(file_contents->data == NULL) {
            error = CJSON_FILECONTENTS_ERROR_MEMORY;
            break;
        }
        file_contents->data[length] = '\0';
        
        if(fread(file_contents->data, sizeof(unsigned char), (size_t)length, file) != (size_t)length) {
            error = CJSON_FILECONTENTS_ERROR_FREAD;
            break;
        }

        error               = CJSON_FILECONTENTS_ERROR_NONE;
        file_contents->size = (unsigned)length;
    } while(0);

    if(error != CJSON_FILECONTENTS_ERROR_NONE) {
        CJSON_FileContents_free(file_contents);
    }
    
    fclose(file);
    
    return error;
}

