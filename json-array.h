#ifndef CJSON_ARRAY_H
#define CJSON_ARRAY_H

#define CJSON_ARRAY_MINIMUM_CAPACITY  8U

struct CJSON;

struct CJSON_Array {
    struct CJSON *values;
    unsigned      count,
                  capacity;
};

#endif