#ifdef __cplusplus
extern "C" {
#endif


#ifndef CJSON_COUNTERS_H
#define CJSON_COUNTERS_H

struct CJSON_Counters {
    unsigned string,
             number,
             array,
             object,
             keyword,
             comma,
             chars,
             array_elements,
             object_elements;
};

void CJSON_Counters_init(struct CJSON_Counters *const counters);

#endif

#ifdef __cplusplus
}
#endif
