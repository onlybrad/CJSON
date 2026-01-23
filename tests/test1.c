#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>

#include "../cjson.h"

static bool success;

static void test_empty_object(void) {
    const char empty_object[] = "{}";
    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    struct CJSON *const json = CJSON_parse(&parser, empty_object, sizeof(empty_object) - 1);
    assert(json != NULL);
    assert(json->type == CJSON_OBJECT);
    assert(parser.error == CJSON_ERROR_NONE);
    CJSON_Parser_free(&parser);
}

static void test_empty_array(void) {
    const char empty_array[] = "[]";
    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    struct CJSON *const json = CJSON_parse(&parser, empty_array, sizeof(empty_array) - 1);
    assert(json != NULL);
    assert(json->type == CJSON_ARRAY);
    assert(parser.error == CJSON_ERROR_NONE);
    CJSON_Parser_free(&parser);
}

static void test_primitive_values(void) {
    const char string[] = "\"\"";
    
    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    const struct CJSON *json = CJSON_parse(&parser, string, sizeof(string) - 1);
    assert(json != NULL);
    assert(json->type == CJSON_STRING);
    assert(parser.error == CJSON_ERROR_NONE);
    assert(strcmp(json->value.string.chars, "") == 0);

    const char int64[] = "-125";
    json = CJSON_parse(&parser, int64, sizeof(int64) - 1);
    assert(json != NULL);
    assert(json->type == CJSON_INT64);
    assert(json->value.int64 == -125);
    assert(parser.error == CJSON_ERROR_NONE);

    const char uint64[] = "2500";
    json = CJSON_parse(&parser, uint64, sizeof(uint64) - 1);
    assert(json != NULL);
    assert(json->type == CJSON_UINT64);
    assert(json->value.uint64 == 2500);
    assert(parser.error == CJSON_ERROR_NONE);

    const char true_value[] = "true";
    json = CJSON_parse(&parser, true_value, sizeof(true_value) - 1);
    assert(json != NULL);
    assert(json->type == CJSON_BOOL);
    assert(json->value.boolean);
    assert(parser.error == CJSON_ERROR_NONE);

    const char false_value[] = "false";
    json = CJSON_parse(&parser, false_value, sizeof(false_value) - 1);
    assert(json != NULL);
    assert(json->type == CJSON_BOOL);
    assert(!json->value.boolean);
    assert(parser.error == CJSON_ERROR_NONE);

    const char null_value[] = "null";
    json = CJSON_parse(&parser, null_value, sizeof(null_value) - 1);
    assert(json != NULL);
    assert(json->type == CJSON_NULL);
    assert(json->value.null == NULL);
    assert(parser.error == CJSON_ERROR_NONE);

    CJSON_Parser_free(&parser);   
}

static void test_key_value(void) {
    const char key_value[] = "{\"key\": \"value\"}";
    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    struct CJSON *const json = CJSON_parse(&parser, key_value, sizeof(key_value) - 1);
    assert(json != NULL);
    assert(json->type == CJSON_OBJECT);
    assert(parser.error == CJSON_ERROR_NONE);

    const char         *value;
    const struct CJSON *json2;

    value = CJSON_get_string(json, "key", &success);
    assert(value != NULL);
    assert(success);
    assert(strcmp(value, "value") == 0);

    value = CJSON_get_string(json, ".key", &success);
    assert(value != NULL);
    assert(success);
    assert(strcmp(value, "value") == 0);

    json2 = CJSON_get(json, ".key");
    assert(json2 != NULL);
    assert(json2->type == CJSON_STRING);
    assert(strcmp(json2->value.string.chars, "value") == 0);

    value = CJSON_Object_get_string(&json->value.object, "key", &success);
    assert(value != NULL);
    assert(success);
    assert(strcmp(value, "value") == 0);
    
    json2 = CJSON_Object_get(&json->value.object, "key");
    assert(json2 != NULL);
    assert(json2->type == CJSON_STRING);
    assert(strcmp(json2->value.string.chars, "value") == 0);   

    CJSON_Parser_free(&parser);
}

static void test_nested_objects(void) {
    const char nested_objects[] = "{"
        "\"key1\": {\"innerKey\": \"innerValue\"},"
        "\"key2\": \"value\""
    "}";

    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    struct CJSON *const json = CJSON_parse(&parser, nested_objects, sizeof(nested_objects) - 1);
    assert(json != NULL);
    assert(json->type == CJSON_OBJECT);
    assert(parser.error == CJSON_ERROR_NONE);

    const char *value;
    
    const struct CJSON_Object *const inner_object = CJSON_get_object(json, ".key1", &success);
    assert(inner_object != NULL);
    assert(success);
    value = CJSON_Object_get_string(inner_object, "innerKey", &success);
    assert(value != NULL);
    assert(success);
    assert(strcmp(value, "innerValue") == 0);

    value = CJSON_get_string(json, ".key1.innerKey", &success);
    assert(value != NULL);
    assert(success);
    assert(strcmp(value, "innerValue") == 0);

    value = CJSON_get_string(json, ".key2", &success);
    assert(value != NULL);
    assert(success);
    assert(strcmp(value, "value") == 0);

    CJSON_Parser_free(&parser);
}

static void test_struct_array(void) {
    const char struct_array[] = "["
        "{\"key1\": \"value1\"},"
        "{\"key2\": \"value2\"}"
    "]";

    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    struct CJSON *const json = CJSON_parse(&parser, struct_array, sizeof(struct_array) - 1);
    assert(json != NULL);
    assert(json->type == CJSON_ARRAY);
    assert(json->value.array.count == 2U);
    assert(parser.error == CJSON_ERROR_NONE);

    const struct CJSON        *json2;
    const struct CJSON_Object *object;
    const char                *value;

    object = CJSON_get_object(json, "[0]", &success);
    assert(object != NULL);
    assert(success);
    value = CJSON_Object_get_string(object, "key1", &success);
    assert(value != NULL);
    assert(success);
    assert(strcmp(value, "value1") == 0);

    object = CJSON_get_object(json, "[1]", &success);    
    assert(object != NULL);
    assert(success);
    value = CJSON_Object_get_string(object, "key2", &success);
    assert(value != NULL);
    assert(success);
    assert(strcmp(value, "value2") == 0);

    object = CJSON_Array_get_object(&json->value.array, 0U, &success);  
    assert(object != NULL);     
    assert(success);
    value = CJSON_Object_get_string(object, "key1", &success);
    assert(value != NULL);
    assert(success);
    assert(strcmp(value, "value1") == 0);
    object = CJSON_Array_get_object(&json->value.array, 1U, &success); 
    assert(object != NULL);
    assert(success);
    value = CJSON_Object_get_string(object, "key2", &success);
    assert(value != NULL);
    assert(success);
    assert(strcmp(value, "value2") == 0);

    json2 = CJSON_Array_get(&json->value.array, 0U);
    assert(json2 != NULL);
    assert(json2->type == CJSON_OBJECT);
    value = CJSON_Object_get_string(&json2->value.object, "key1", &success);
    assert(value != NULL);
    assert(success);
    assert(strcmp(value, "value1") == 0);
    json2 = CJSON_Array_get(&json->value.array, 1U);
    assert(json2 != NULL);
    assert(json2->type == CJSON_OBJECT);
    value = CJSON_Object_get_string(&json2->value.object, "key2", &success);
    assert(value != NULL);
    assert(success);
    assert(strcmp(value, "value2") == 0);

    CJSON_Parser_free(&parser);  
}

static void test_escaped_characters(void) {
    const char escaped_characters[] = "{\"key\": \"Line 1\\nLine 2\\\\\"}";
    
    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    struct CJSON *const json = CJSON_parse(&parser, escaped_characters, sizeof(escaped_characters) - 1);
    assert(json != NULL);
    assert(json->type == CJSON_OBJECT);
    assert(parser.error == CJSON_ERROR_NONE);

    const char *const value = CJSON_get_string(json, "key", &success);
    assert(value != NULL);
    assert(success);
    assert(value[6] == '\n');   

    CJSON_Parser_free(&parser);
}

static void test_escaped_unicode(void) {
    const char escaped_characters[] = "{\"key\": \"Unicode test: \\u00A9\\u03A9\\uD840\\uDC00\"}";

    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    struct CJSON *const json = CJSON_parse(&parser, escaped_characters, sizeof(escaped_characters) - 1);
    assert(json != NULL);
    assert(json->type == CJSON_OBJECT);
    assert(parser.error == CJSON_ERROR_NONE);

    const char *const value = CJSON_get_string(json, "key", &success);
    assert(value != NULL);
    assert(success);

    //\u00A9\u03A9 == ©Ω == (unsigned utf8) {194, 169, 206, 169} == (signed utf8) {-62, -87, -50, -87 }
    assert(value[14] == -62);
    assert(value[15] == -87);
    assert(value[16] == -50);
    assert(value[17] == -87);
    //\uD840\\uDC00 == 𠀀 == (unsigned utf8) {240, 160, 128, 128} == (signed utf8) {-16, -96, -128, -128}
    assert(value[18] == -16);
    assert(value[19] == -96);
    assert(value[20] == -128);
    assert(value[21] == -128);

    CJSON_Parser_free(&parser);
}

static void test_bools(void) {
    const char escaped_characters[] = "{\"isTrue\": true, \"isFalse\": false}";

    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    struct CJSON *const json = CJSON_parse(&parser, escaped_characters, sizeof(escaped_characters) - 1);
    assert(json != NULL);
    assert(json->type == CJSON_OBJECT);
    assert(parser.error == CJSON_ERROR_NONE);

    bool          value;
    struct CJSON *json2;

    json2 = CJSON_get(json, "isTrue");
    assert(json2 != NULL);
    assert(json2->type == CJSON_BOOL);
    assert(json2->value.boolean);

    value = CJSON_get_bool(json, "isTrue", &success);
    assert(success);
    assert(value);

    json2 = CJSON_get(json, "isFalse");
    assert(json2 != NULL);
    assert(json2->type == CJSON_BOOL);
    assert(!json2->value.boolean);

    value = CJSON_get_bool(json, "isFalse", &success);
    assert(success);
    assert(!value);

    CJSON_Parser_free(&parser);
}

static void test_exponent(void) {
    const char exponent[] = "{\"largeNumber\": 1e15, \"negativeLarge\": -1e15}";
    
    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    struct CJSON *const json = CJSON_parse(&parser, exponent, sizeof(exponent) - 1);
    assert(json != NULL);
    assert(success);
    assert(json->type == CJSON_OBJECT);
    assert(parser.error == CJSON_ERROR_NONE);

    const uint64_t positive_number = CJSON_get_uint64(json, "largeNumber", &success);
    assert(success);
    assert(positive_number == (uint64_t)1e15);

    const int64_t negative_number = CJSON_get_int64(json, "negativeLarge", &success);
    assert(success);
    assert(negative_number == (int64_t)-1e15);

    CJSON_Parser_free(&parser);
}

static void test_null(void) {
    const char null_value[] = "{\"key\": null}";
    
    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    struct CJSON *const json = CJSON_parse(&parser, null_value, sizeof(null_value) - 1);
    assert(json != NULL);
    assert(json->type == CJSON_OBJECT);
    assert(parser.error == CJSON_ERROR_NONE);

    const struct CJSON *const null_json = CJSON_get(json, "key");
    assert(null_json != NULL);
    assert(null_json->type == CJSON_NULL);
    assert(null_json->value.null == NULL);

    void *null = CJSON_get_null(json, "key", &success);
    assert(success);
    assert(null == NULL);

    CJSON_Parser_free(&parser);
}

static void test_missing_value(void) {
    const char missing_key[] = "{\"key1\": \"value1\", \"key2\": }";

    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    struct CJSON *const json = CJSON_parse(&parser, missing_key, sizeof(missing_key) - 1);
    assert(json == NULL);
    assert(parser.error == CJSON_ERROR_OBJECT);

    CJSON_Parser_free(&parser);
}

static void test_comments(void) {
    const char comments[] = "{"
        "// This is a comment"
        "\"key\": \"value\""
    "}";

    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    struct CJSON *const json = CJSON_parse(&parser, comments, sizeof(comments) - 1);
    assert(json == NULL);
    assert(parser.error == CJSON_ERROR_TOKEN);

    CJSON_Parser_free(&parser);
}

static void test_deep_nesting(void) {
    const char deep_nesting[] = "{\"key1\": {\"key2\": {\"key3\": {\"key4\": {\"key5\": [0, 1, 2, 3, 4, \"value\"]}}}}}";
    
    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    struct CJSON *const json = CJSON_parse(&parser, deep_nesting, sizeof(deep_nesting) - 1);
    assert(json != NULL);
    assert(json->type == CJSON_OBJECT);
    assert(parser.error == CJSON_ERROR_NONE);

    const char *value = CJSON_get_string(json, "key1.key2.key3.key4.key5[5]", &success);
    assert(value != NULL);
    assert(success);
    assert(strcmp(value, "value") == 0);

    struct CJSON_QueryBuilder query_builder = CJSON_get_query_builder(json);
    CJSON_QueryBuilder_key(&query_builder, "key1");
    CJSON_QueryBuilder_key(&query_builder, "key2");
    CJSON_QueryBuilder_key(&query_builder, "key3");
    CJSON_QueryBuilder_key(&query_builder, "key4");
    CJSON_QueryBuilder_key(&query_builder, "key5");
    CJSON_QueryBuilder_index(&query_builder, 5U);
    assert(query_builder.json != NULL);
    assert(query_builder.json->type == CJSON_STRING);
    value = query_builder.json->value.string.chars;
    assert(strcmp(value, "value") == 0);

    query_builder = CJSON_get_query_builder(json);
    CJSON_QueryBuilder_format(&query_builder, "kkkkki", "key1", "key2", "key3", "key4", "key5", 5U);
    assert(query_builder.json != NULL);
    assert(query_builder.json->type == CJSON_STRING);
    value = query_builder.json->value.string.chars;
    assert(strcmp(value, "value") == 0);
    
    CJSON_Parser_free(&parser);
}

static void test_no_quotes_key(void) {
    const char no_quotes_key[] = "{ key: 1 }";

    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    struct CJSON *const json = CJSON_parse(&parser, no_quotes_key, sizeof(no_quotes_key) - 1);
    assert(json == NULL);
    assert(parser.error == CJSON_ERROR_TOKEN);

    CJSON_Parser_free(&parser);
}

static void test_nested_arrays(void) {
    const char nested_arrays[] = "[[1, 2, [3, 4]], [5, 6]]";

    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    struct CJSON *const json = CJSON_parse(&parser, nested_arrays, sizeof(nested_arrays) - 1);
    assert(json != NULL);
    assert(json->type == CJSON_ARRAY);
    assert(json->value.array.count == 2U);
    assert(parser.error == CJSON_ERROR_NONE);

    struct CJSON *level1_json, *level2_json, *level3_json;

    //[1, 2, [3, 4]]
    level1_json = CJSON_get(json, "[0]");
    assert(level1_json != NULL);
    assert(level1_json->type == CJSON_ARRAY);
    assert(level1_json->value.array.count == 3U);

    //1
    level2_json = CJSON_get(level1_json, "[0]");
    assert(level2_json != NULL);
    assert(level2_json->type == CJSON_UINT64);
    assert(level2_json->value.uint64 == 1U);
    //2
    level2_json = CJSON_get(level1_json, "[1]");
    assert(level2_json != NULL);
    assert(level2_json->type == CJSON_UINT64);
    assert(level2_json->value.uint64 == 2U);
    //[3, 4]
    level2_json = CJSON_get(level1_json, "[2]");
    assert(level2_json != NULL);
    assert(level2_json->type == CJSON_ARRAY);
    assert(level2_json->value.array.count == 2U);
    //3
    level3_json = CJSON_get(level2_json, "[0]");
    assert(level3_json != NULL);
    assert(level3_json->type == CJSON_UINT64);
    assert(level3_json->value.uint64 == 3U);
    //4
    level3_json = CJSON_get(level2_json, "[1]");
    assert(level3_json != NULL);
    assert(level3_json->type == CJSON_UINT64);
    assert(level3_json->value.uint64 == 4U);

    //[5, 6]
    level1_json = CJSON_get(json, "[1]");
    assert(level1_json != NULL);
    assert(level1_json->type == CJSON_ARRAY);
    assert(level1_json->value.array.count == 2U);

    //5
    level2_json = CJSON_get(level1_json, "[0]");
    assert(level2_json != NULL);
    assert(level2_json->type == CJSON_UINT64);
    assert(level2_json->value.uint64 == 5U);
    //6
    level2_json = CJSON_get(level1_json, "[1]");
    assert(level2_json != NULL);
    assert(level2_json->type == CJSON_UINT64);
    assert(level2_json->value.uint64 == 6U);

    CJSON_Parser_free(&parser);
}

static void test_duplicate_keys(void) {
    const char duplicate_keys[] = "{\"key\": \"value1\", \"key\": \"value2\"}";
    
    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    struct CJSON *const json = CJSON_parse(&parser, duplicate_keys, sizeof(duplicate_keys) - 1);
    assert(json != NULL);
    assert(json->type == CJSON_OBJECT);
    assert(parser.error == CJSON_ERROR_NONE);

    const char *const value = CJSON_get_string(json, "key", &success);
    assert(value != NULL);
    assert(success);
    assert(strcmp(value, "value2") == 0);

    CJSON_Parser_free(&parser);
}

static void test_create_string(void) {
    const char *const value = "test";

    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    struct CJSON *const json = CJSON_new(&parser);
    assert(json != NULL);
    assert(json->type == CJSON_NULL);
    assert(parser.error == CJSON_ERROR_NONE);

    CJSON_set_string(json, &parser, value);
    assert(json->type == CJSON_STRING);
    assert(strcmp(json->value.string.chars, value) == 0);
    assert(json->value.string.chars != value);

    CJSON_Parser_free(&parser);
}

static void test_create_primitives(void) {
    const int64_t value1 = -25000000000LL;

    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    struct CJSON *const json = CJSON_new(&parser);
    assert(json != NULL);
    assert(json->type == CJSON_NULL);
    assert(parser.error == CJSON_ERROR_NONE);

    CJSON_set_int64(json, value1);
    assert(json->type == CJSON_INT64);
    assert(json->value.int64 == value1);

    const uint64_t value2 = 25000000000ULL;
    CJSON_set_uint64(json, value2);
    assert(json->type == CJSON_UINT64);
    assert(json->value.uint64 == value2);

    const double value3 = 25000000000.50;
    CJSON_set_float64(json, value3);
    assert(json->type == CJSON_FLOAT64);
    assert(json->value.float64 == value3);

    const bool value4 = true;
    CJSON_set_bool(json, value4);
    assert(json->type == CJSON_BOOL);
    assert(json->value.boolean);

    CJSON_set_null(json);
    assert(json->type == CJSON_NULL);
    assert(json->value.null == NULL);

    CJSON_Parser_free(&parser);
}

static void test_create_array(void) {
    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    struct CJSON *const json = CJSON_new(&parser);
    assert(json != NULL);
    assert(json->type == CJSON_NULL);
    assert(parser.error == CJSON_ERROR_NONE);

    struct CJSON_Array *const array1 = CJSON_make_array(json, &parser);
    assert(array1 != NULL);
    assert(json->type == CJSON_ARRAY);
    assert(&json->value.array == array1);

    const uint64_t value1 = 5ULL;
    const bool value2 = true;
    const int64_t value3 = -25000000000LL;

    struct CJSON_Array array2;
    CJSON_Array_init(&array2);

    CJSON_Array_set_uint64(&array2, &parser, 0, value1);
    CJSON_Array_set_array(array1, &parser, 0, &array2);
    CJSON_Array_set_bool(array1, &parser, 1, value2);
    CJSON_Array_set_int64(array1, &parser, 2, value3);

    assert(array2.values[0].type == CJSON_UINT64);
    assert(array2.values[0].value.uint64 == value1);
    assert(array1->values[0].type == CJSON_ARRAY);
    assert(&array1->values[0].value.array != &array2);
    assert(array1->values[1].type == CJSON_BOOL);
    assert(array1->values[1].value.boolean == value2);
    assert(array1->values[2].type == CJSON_INT64);
    assert(array1->values[2].value.int64 == value3);

    CJSON_Parser_free(&parser);
}

static void test_create_object(void) {
    struct CJSON_Parser parser;
    CJSON_Parser_init(&parser);
    struct CJSON *const json = CJSON_new(&parser);
    assert(json != NULL);
    assert(json->type == CJSON_NULL);
    assert(parser.error == CJSON_ERROR_NONE);
    
    struct CJSON_Object *const object1 = CJSON_make_object(json, &parser);
    assert(json->type == CJSON_OBJECT);
    assert(&json->value.object == object1);

    const uint64_t value1 = 5ULL;
    const bool value2 = true;
    const int64_t value3 = -25000000000LL;

    struct CJSON_Object object2;
    CJSON_Object_init(&object2);

    CJSON_Object_set_uint64(&object2, &parser, "key1", value1);
    CJSON_Object_set_object(object1, &parser, "key1", &object2);
    CJSON_Object_set_bool(object1, &parser, "key2", value2);
    CJSON_Object_set_int64(object1, &parser, "key3", value3);

    assert(CJSON_Object_get_uint64(&object2, "key1", &success) == value1);
    assert(success);
    assert(CJSON_Object_get_object(object1, "key1", &success) != NULL);
    assert(success);
    assert(CJSON_Object_get_bool(object1, "key2", &success) == value2);
    assert(success);
    assert(CJSON_Object_get_int64(object1, "key3", &success) == value3);
    assert(success);

    CJSON_Parser_free(&parser);
}

int main(void) {
    test_empty_object();
    test_empty_array();
    test_primitive_values();
    test_key_value();
    test_nested_objects();
    test_struct_array();
    test_escaped_characters();
    test_escaped_unicode();
    test_bools();
    test_exponent();
    test_null();
    test_missing_value();
    test_comments();
    test_deep_nesting();
    test_no_quotes_key();
    test_nested_arrays();
    test_duplicate_keys();
    test_create_string();
    test_create_primitives();
    test_create_array();
    test_create_object();

    puts("All tests successful");

    return EXIT_SUCCESS;
}