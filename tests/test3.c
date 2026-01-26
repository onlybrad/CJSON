#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../cjson.h"
#include "../util.h"

static struct CJSON_Parser parser;

static void test_string_to_string(void) {
    const char string[] = "\"value\"";

    const struct CJSON *const json = CJSON_parse(&parser, string, (unsigned)static_strlen(string));
    assert(json != NULL);
    const unsigned size = CJSON_to_string_size(json, 2U);
    assert(size == static_strlen(string));
    char *const string2 = CJSON_to_string(json, 2U);
    assert(strcmp(string, string2) == 0);
    CJSON_FREE(string2);
}

static void test_numbers_to_string(void) {
    const char int64[]   = "-102345";
    const char uint64[]  = "1234567";
    const char float64[] = "-112340.00123456789123456000000";

    const struct CJSON *json = CJSON_parse(&parser, int64, (unsigned)(sizeof(int64)) - 1U);
    assert(json != NULL);
    unsigned size = CJSON_to_string_size(json, 2U);
    assert(size == static_strlen(int64));
    char *string = CJSON_to_string(json, 2U);
    assert(strcmp(string, int64) == 0);
    CJSON_FREE(string);

    json = CJSON_parse(&parser, uint64, (unsigned)(sizeof(uint64)) - 1U);
    assert(json != NULL);
    size = CJSON_to_string_size(json, 2U);
    assert(size == static_strlen(uint64));
    string = CJSON_to_string(json, 2U);
    assert(strcmp(string, uint64) == 0);
    CJSON_FREE(string);

    json = CJSON_parse(&parser, float64, (unsigned)(sizeof(float64)) - 1U);
    assert(json != NULL);
    char *const float64String = CJSON_to_string(json, 2U);
#ifdef DBL_DECIMAL_DIG
    assert(strcmp(float64String, "-112340.00123456788") == 0);
#else
    assert(strcmp(float64String, "-112340.001234568") == 0);
#endif
    CJSON_FREE(float64String);
}

static void test_object_to_string(void) {
    const char object[] = "{\"key1\": \"value1\"}";
    const char expected[] = "{\n  \"key1\": \"value1\"\n}";

    const struct CJSON *const json = CJSON_parse(&parser, object, (unsigned)(sizeof(object)) - 1U);
    assert(json != NULL);
    const unsigned size = CJSON_to_string_size(json, 2U);
    assert(size == static_strlen(expected));
    char *const string = CJSON_to_string(json, 2U);
    assert(strcmp(string, expected) == 0);
    CJSON_FREE(string);
}

static void test_array_of_object_to_string(void) {
    const char array[] = "["
        "{\"key1\": \"value1\"},"
        "{\"key2\": \"value2\"}"
    "]";
    const char expected[] = "[\n  {\n    \"key1\": \"value1\"\n  },\n  {\n    \"key2\": \"value2\"\n  }\n]";

    const struct CJSON *const json = CJSON_parse(&parser, array, (unsigned)(sizeof(array)) - 1U);
    assert(json != NULL);
    const unsigned size = CJSON_to_string_size(json, 2U);
    assert(size == static_strlen(expected));
    char *const string = CJSON_to_string(json, 2U);
    assert(strcmp(string, expected) == 0);
    CJSON_FREE(string);
}

static void test_deeply_nested_array(void) {
    const char array[] = "[\n  1,\n  2,\n  3,\n  [\n    4,\n    5,\n    [\n      4,\n      5\n    ],\n    6,\n    [\n      7,\n      8\n    ]\n  ]\n]";

    const struct CJSON *const json = CJSON_parse(&parser, array, (unsigned)(sizeof(array)) - 1U);
    assert(json != NULL);
    const unsigned size = CJSON_to_string_size(json, 2U);
    assert(size == 106U);
}

static void test_to_string(void) {
    const char array[] = "["
        "{\"key1\": \"value1\"},"
        "{\"key2\": {\"key3\": [true, {\"key4\": false}, null]}},"
        "{\"key5\": {}},"
        "{\"key6\": []},"
        "{\"key7\": \"\"},"
        "{\"key8\": 1e5}"
    "]";

    const char expected_indentation0[] = "[{\"key1\":\"value1\"},{\"key2\":{\"key3\":[true,{\"key4\":false},null]}},{\"key5\":{}},{\"key6\":[]},{\"key7\":\"\"},{\"key8\":100000}]";
    const char expected_indentation2[] = "[\n  {\n    \"key1\": \"value1\"\n  },\n  {\n    \"key2\": {\n      \"key3\": [\n        true,\n        {\n          \"key4\": false\n        },\n        null\n      ]\n    }\n  },\n  {\n    \"key5\": {}\n  },\n  {\n    \"key6\": []\n  },\n  {\n    \"key7\": \"\"\n  },\n  {\n    \"key8\": 100000\n  }\n]";
    const char expected_indentation4[] = "[\n    {\n        \"key1\": \"value1\"\n    },\n    {\n        \"key2\": {\n            \"key3\": [\n                true,\n                {\n                    \"key4\": false\n                },\n                null\n            ]\n        }\n    },\n    {\n        \"key5\": {}\n    },\n    {\n        \"key6\": []\n    },\n    {\n        \"key7\": \"\"\n    },\n    {\n        \"key8\": 100000\n    }\n]";

    const struct CJSON *json = CJSON_parse(&parser, array, (unsigned)static_strlen(array));
    assert(json != NULL);

    char *string = CJSON_to_string(json, 0U);
    assert(strcmp(string, expected_indentation0) == 0);
    CJSON_FREE(string);

    string = CJSON_to_string(json, 2U);
    assert(strcmp(string, expected_indentation2) == 0);
    CJSON_FREE(string);

    string = CJSON_to_string(json, 4U);
    assert(strcmp(string, expected_indentation4) == 0);
    CJSON_FREE(string);

    string = CJSON_format(array, (unsigned)static_strlen(array), 0U);
    assert(strcmp(string, expected_indentation0) == 0);
    CJSON_FREE(string);

    string = CJSON_format(array, (unsigned)static_strlen(array), 2U);
    assert(strcmp(string, expected_indentation2) == 0);
    CJSON_FREE(string);

    string = CJSON_format(array, (unsigned)static_strlen(array), 4U);
    assert(strcmp(string, expected_indentation4) == 0);
    CJSON_FREE(string);

    json = CJSON_parse(&parser, array, (unsigned)static_strlen(array));
    assert(json != NULL);    

    assert(CJSON_to_file(json, "tests/test3-0-identation.json", 0U));
    remove("tests/test3-0-identation.json");
    assert(CJSON_to_file(json, "tests/test3-2-identation.json", 2U));
    remove("tests/test3-2-identation.json");
    assert(CJSON_to_file(json, "tests/test3-4-identation.json", 4U));
    remove("tests/test3-4-identation.json");
}

int main(void) {
    CJSON_Parser_init(&parser);

    test_string_to_string();
    test_numbers_to_string();
    test_object_to_string();
    test_array_of_object_to_string();
    test_deeply_nested_array();
    test_to_string();

    puts("All tests successful");

    CJSON_Parser_free(&parser);

    return EXIT_SUCCESS;
}