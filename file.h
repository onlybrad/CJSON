#ifdef __cplusplus
extern "C" {
#endif

#ifndef CJSON_FILE_H
#define CJSON_FILE_H

#ifndef _WIN32
    #define _FILE_OFFSET_BITS 64
    #include <unistd.h>
#endif

enum CJSON_FileContents_Error {
    CJSON_FILECONTENTS_ERROR_NONE,
    CJSON_FILECONTENTS_ERROR_WIN32_API,
    CJSON_FILECONTENTS_ERROR_TOO_LARGE,
    CJSON_FILECONTENTS_ERROR_MEMORY,
    CJSON_FILECONTENTS_ERROR_FOPEN,
    CJSON_FILECONTENTS_ERROR_FREAD,
    CJSON_FILECONTENTS_ERROR_FWRITE,
    CJSON_FILECONTENTS_ERROR_FSEEK,
    CJSON_FILECONTENTS_ERROR_FTELL,
    CJSON_FILECONTENTS_ERROR_FCLOSE
};

struct CJSON_FileContents {
    unsigned char *data;
    unsigned       size;
};

void                          CJSON_FileContents_init(struct CJSON_FileContents*);
void                          CJSON_FileContents_free(struct CJSON_FileContents*);
enum CJSON_FileContents_Error CJSON_FileContents_get(struct CJSON_FileContents*, const char *path);
enum CJSON_FileContents_Error CJSON_FileContents_put(const struct CJSON_FileContents*, const char *path);

#endif

#ifdef __cplusplus
}
#endif
