#define _CJSON_CONCAT(A, B)           A##B
#define CJSON_CONCAT(A, B)            _CJSON_CONCAT(A, B)
#define CJSON_GET_FUNCTION_NAME(TYPE) CJSON_CONCAT(CJSON_Object_get_, TYPE)

#ifndef CJSON_GET_TYPE
    #error "Missing CJSON_GET_TYPE macro."
#endif

#ifndef CJSON_GET_MEMBER
    #error "Missing CJSON_GET_MEMBER macro."
#endif

#if !defined CJSON_GET_SUFFIX && !defined CJSON_GET_SUFFIX_BOOL
    #error "CJSON_GET_SUFFIX or CJSON_GET_SUFFIX_BOOL are not defined."
#elif defined CJSON_GET_SUFFIX && defined CJSON_GET_SUFFIX_BOOL
    #error "CJSON_GET_SUFFIX and CJSON_GET_SUFFIX_BOOL cannot be defined at the same time."
#endif

#ifndef CJSON_GET_RETURN_TYPE
    #error "Missing CJSON_GET_RETURN_TYPE macro."
#endif

#ifdef CJSON_GET_SUFFIX_BOOL
    #define CJSON_GET_FUNCTION CJSON_Object_get_bool
#else
    #define CJSON_GET_FUNCTION CJSON_GET_FUNCTION_NAME(CJSON_GET_SUFFIX)
#endif

EXTERN_C CJSON_GET_RETURN_TYPE CJSON_GET_FUNCTION(const struct CJSON_Object *const object, const char *const key, bool *const success) {
    assert(object != NULL);
    assert(key != NULL);
    assert(success != NULL);
                            \
    struct CJSON *const ret = CJSON_Object_get(object, key);
    if(ret == NULL || ret->type != CJSON_GET_TYPE) {
        *success = false;
        return 0;
    }\
    *success = true;

#ifdef CJSON_GET_RETURN_PTR
    return &ret->data.CJSON_GET_MEMBER;
#undef CJSON_GET_RETURN_PTR
#else
    return ret->data.CJSON_GET_MEMBER;
#endif
}

#undef CJSON_CONCAT
#undef _CJSON_CONCAT
#undef CJSON_GET_FUNCTION_NAME
#undef CJSON_GET_TYPE
#undef CJSON_GET_MEMBER
#undef CJSON_GET_SUFFIX
#undef CJSON_GET_RETURN_TYPE
#undef CJSON_GET_FUNCTION
