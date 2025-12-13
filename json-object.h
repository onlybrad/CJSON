#ifndef CJSON_OBJECT_H
#define CJSON_OBJECT_H

#define CJSON_OBJECT_MINIMUM_CAPACITY 8U

struct CJSON_KV;

struct CJSON_Object {
    struct CJSON_KV *entries;
    unsigned         capacity;
};

#endif