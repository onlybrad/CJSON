#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include "../cjson.h"

static void test_empty_object(void) {
    const char empty_object[] = "{}";
    struct CJSON_Root root;
    const bool success = CJSON_parse(&root, empty_object, sizeof(empty_object) - 1);
    assert(success);
    assert(root.json.type == CJSON_OBJECT);
    CJSON_free(&root);
}

static void test_empty_array(void) {
    const char empty_array[] = "[]";
    struct CJSON_Root root;
    const bool success = CJSON_parse(&root, empty_array, sizeof(empty_array) - 1);
    assert(success);
    assert(root.json.type == CJSON_ARRAY);
    CJSON_free(&root);
}

static void test_primitive_values(void) {
    const char string[] = "\"\"";
    struct CJSON_Root root;
    bool success = CJSON_parse(&root, string, sizeof(string) - 1);
    assert(success);
    assert(root.json.type == CJSON_STRING);
    assert(strcmp(root.json.data.string.chars, "") == 0);
    CJSON_free(&root);

    const char int64[] = "-125";
    success = CJSON_parse(&root, int64, sizeof(int64) - 1);
    assert(root.json.type == CJSON_INT64);
    assert(root.json.data.int64 == -125);
    CJSON_free(&root);

    const char uint64[] = "2500";
    success = CJSON_parse(&root, uint64, sizeof(uint64) - 1);
    assert(root.json.type == CJSON_UINT64);
    assert(root.json.data.uint64 == 2500);
    CJSON_free(&root);

    const char true_value[] = "true";
    success = CJSON_parse(&root, true_value, sizeof(true_value) - 1);
    assert(root.json.type == CJSON_BOOL);
    assert(root.json.data.boolean);
    CJSON_free(&root);

    const char false_value[] = "false";
    success = CJSON_parse(&root, false_value, sizeof(false_value) - 1);
    assert(root.json.type == CJSON_BOOL);
    assert(!root.json.data.boolean);
    CJSON_free(&root); 

    const char null_value[] = "null";
    success = CJSON_parse(&root, null_value, sizeof(null_value) - 1);
    assert(root.json.type == CJSON_NULL);
    assert(root.json.data.null == NULL);
    CJSON_free(&root);   
}

static void test_key_value(void) {
    const char key_value[] = "{\"key\": \"value\"}";
    struct CJSON_Root root;
    bool success = CJSON_parse(&root, key_value, sizeof(key_value) - 1);
    assert(success);
    assert(root.json.type == CJSON_OBJECT);

    const char *value;
    const struct CJSON *json;

    value = CJSON_get_string(&root.json, "key", &success);
    assert(success);
    assert(strcmp(value, "value") == 0);

    value = CJSON_get_string(&root.json, ".key", &success);
    assert(success);
    assert(strcmp(value, "value") == 0);

    json = CJSON_get(&root.json, ".key");
    assert(json != NULL);
    assert(json->type == CJSON_STRING);
    assert(strcmp(json->data.string.chars, "value") == 0);

    value = CJSON_Object_get_string(&root.json.data.object, "key", &success);
    assert(success);
    assert(strcmp(value, "value") == 0);
    
    json = CJSON_Object_get(&root.json.data.object, "key");
    assert(json != NULL);
    assert(json->type == CJSON_STRING);
    assert(strcmp(json->data.string.chars, "value") == 0);   

    CJSON_free(&root);
}

static void test_nested_objects(void) {
    const char nested_objects[] = "{"
        "\"key1\": {\"innerKey\": \"innerValue\"},"
        "\"key2\": \"value\""
    "}";

    struct CJSON_Root root;
    bool success = CJSON_parse(&root, nested_objects, sizeof(nested_objects) - 1);
    assert(success);
    assert(root.json.type == CJSON_OBJECT);

    const char *value;
    
    const struct CJSON_Object *const inner_object = CJSON_get_object(&root.json, ".key1", &success);
    assert(success);
    value = CJSON_Object_get_string(inner_object, "innerKey", &success);
    assert(success);
    assert(strcmp(value, "innerValue") == 0);

    value = CJSON_get_string(&root.json, ".key1.innerKey", &success);
    assert(success);
    assert(strcmp(value, "innerValue") == 0);

    value = CJSON_get_string(&root.json, ".key2", &success);
    assert(success);
    assert(strcmp(value, "value") == 0);

    CJSON_free(&root);
}

static void test_struct_array(void) {
    const char struct_array[] = "["
        "{\"key1\": \"value1\"},"
        "{\"key2\": \"value2\"}"
    "]";

    struct CJSON_Root root;
    bool success = CJSON_parse(&root, struct_array, sizeof(struct_array) - 1);
    assert(success);
    assert(root.json.type == CJSON_ARRAY);
    assert(root.json.data.array.count == 2U);

    const struct CJSON *json;
    const struct CJSON_Object *object;
    const char *value;

    object = CJSON_get_object(&root.json, "[0]", &success);
    assert(success);
    value = CJSON_Object_get_string(object, "key1", &success);
    assert(success);
    assert(strcmp(value, "value1") == 0);

    object = CJSON_get_object(&root.json, "[1]", &success);    
    value = CJSON_Object_get_string(object, "key2", &success);
    assert(success);
    assert(strcmp(value, "value2") == 0);

    object = CJSON_Array_get_object(&root.json.data.array, 0U, &success);       
    assert(success);
    value = CJSON_Object_get_string(object, "key1", &success);
    assert(success);
    assert(strcmp(value, "value1") == 0);
    object = CJSON_Array_get_object(&root.json.data.array, 1U, &success);  
    value = CJSON_Object_get_string(object, "key2", &success);
    assert(success);
    assert(strcmp(value, "value2") == 0);

    json = CJSON_Array_get(&root.json.data.array, 0U);
    assert(json != NULL);
    assert(json->type == CJSON_OBJECT);
    value = CJSON_Object_get_string(&json->data.object, "key1", &success);
    assert(success);
    assert(strcmp(value, "value1") == 0);
    json = CJSON_Array_get(&root.json.data.array, 1U);
    assert(json != NULL);
    assert(json->type == CJSON_OBJECT);
    value = CJSON_Object_get_string(&json->data.object, "key2", &success);
    assert(success);
    assert(strcmp(value, "value2") == 0);

    CJSON_free(&root);  
}

static void test_escaped_characters(void) {
    const char escaped_characters[] = "{\"key\": \"Line 1\\nLine 2\\\\\"}";

    struct CJSON_Root root;
    bool success = CJSON_parse(&root, escaped_characters, sizeof(escaped_characters) - 1);
    assert(success);
    assert(root.json.type == CJSON_OBJECT);

    const char *const value = CJSON_get_string(&root.json, "key", &success);
    assert(success);
    assert(value[6] == '\n');   

    CJSON_free(&root);
}

static void test_escaped_unicode(void) {
    const char escaped_characters[] = "{\"key\": \"Unicode test: \\u00A9\\u03A9\\uD840\\uDC00\"}";

    struct CJSON_Root root;
    bool success = CJSON_parse(&root, escaped_characters, sizeof(escaped_characters) - 1);
    assert(success);
    assert(root.json.type == CJSON_OBJECT);

    const char *const value = CJSON_get_string(&root.json, "key", &success);
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

    CJSON_free(&root);
}

static void test_bools(void) {
    const char escaped_characters[] = "{\"isTrue\": true, \"isFalse\": false}";

    struct CJSON_Root root;
    bool success = CJSON_parse(&root, escaped_characters, sizeof(escaped_characters) - 1);
    assert(success);
    assert(root.json.type == CJSON_OBJECT);

    bool value;
    struct CJSON *json;

    json = CJSON_get(&root.json, "isTrue");
    assert(json != NULL);
    assert(json->type == CJSON_BOOL);
    assert(json->data.boolean);

    value = CJSON_get_bool(&root.json, "isTrue", &success);
    assert(success);
    assert(value);

    json = CJSON_get(&root.json, "isFalse");
    assert(json != NULL);
    assert(json->type == CJSON_BOOL);
    assert(!json->data.boolean);

    value = CJSON_get_bool(&root.json, "isFalse", &success);
    assert(success);
    assert(!value);

    CJSON_free(&root);
}

static void test_exponent(void) {
    const char exponent[] = "{\"largeNumber\": 1e15, \"negativeLarge\": -1e15}";

    struct CJSON_Root root;
    bool success = CJSON_parse(&root, exponent, sizeof(exponent) - 1);
    assert(success);
    assert(root.json.type == CJSON_OBJECT);

    const uint64_t positive_number = CJSON_get_uint64(&root.json, "largeNumber", &success);
    assert(success);
    assert(positive_number == (uint64_t)1e15);

    const int64_t negative_number = CJSON_get_int64(&root.json, "negativeLarge", &success);
    assert(success);
    assert(negative_number == (int64_t)-1e15);

    CJSON_free(&root);
}

static void test_null(void) {
    const char null_value[] = "{\"key\": null}";

    struct CJSON_Root root;
    bool success = CJSON_parse(&root, null_value, sizeof(null_value) - 1);
    assert(success);
    assert(root.json.type == CJSON_OBJECT);

    const struct CJSON *const null_json = CJSON_get(&root.json, "key");
    assert(null_json != NULL);
    assert(null_json->type == CJSON_NULL);
    assert(null_json->data.null == NULL);

    void *null = CJSON_get_null(&root.json, "key", &success);
    assert(success);
    assert(null == NULL);

    CJSON_free(&root);
}

static void test_missing_value(void) {
    const char missing_key[] = "{\"key1\": \"value1\", \"key2\": }";

    struct CJSON_Root root;
    const bool success = CJSON_parse(&root, missing_key, sizeof(missing_key) - 1);
    assert(!success);
    assert(root.json.type == CJSON_ERROR);
    assert(root.json.data.error == CJSON_ERROR_OBJECT);

    CJSON_free(&root);
}

static void test_comments(void) {
    const char comments[] = "{"
        "// This is a comment"
        "\"key\": \"value\""
    "}";

    struct CJSON_Root root;
    const bool success = CJSON_parse(&root, comments, sizeof(comments) - 1);
    assert(!success);
    assert(root.json.type == CJSON_ERROR);
    assert(root.json.data.error == CJSON_ERROR_TOKEN);

    CJSON_free(&root);
}

static void test_deep_nesting(void) {
    const char deep_nesting[] = "{\"key1\": {\"key2\": {\"key3\": {\"key4\": {\"key5\": \"value\"}}}}}";

    struct CJSON_Root root;
    bool success = CJSON_parse(&root, deep_nesting, sizeof(deep_nesting) - 1);
    assert(success);
    assert(root.json.type == CJSON_OBJECT);

    const char *const value = CJSON_get_string(&root.json, "key1.key2.key3.key4.key5", &success);
    assert(success);
    assert(strcmp(value, "value") == 0);

    CJSON_free(&root);
}

static void test_no_quotes_key(void) {
    const char no_quotes_key[] = "{ key: 1 }";

    struct CJSON_Root root;
    const bool success = CJSON_parse(&root, no_quotes_key, sizeof(no_quotes_key) - 1);
    assert(!success);
    assert(root.json.type == CJSON_ERROR);
    assert(root.json.data.error == CJSON_ERROR_TOKEN);

    CJSON_free(&root);
}

static void test_nested_arrays(void) {
    const char nested_arrays[] = "[[1, 2, [3, 4]], [5, 6]]";

    struct CJSON_Root root;
    const bool success = CJSON_parse(&root, nested_arrays, sizeof(nested_arrays) - 1);
    assert(success);
    assert(root.json.type == CJSON_ARRAY);
    assert(root.json.data.array.count == 2U);

    struct CJSON *level1_json, *level2_json, *level3_json;

    //[1, 2, [3, 4]]
    level1_json = CJSON_get(&root.json, "[0]");
    assert(level1_json != NULL);
    assert(level1_json->type == CJSON_ARRAY);
    assert(level1_json->data.array.count == 3U);

    //1
    level2_json = CJSON_get(level1_json, "[0]");
    assert(level2_json != NULL);
    assert(level2_json->type == CJSON_UINT64);
    assert(level2_json->data.uint64 == 1U);
    //2
    level2_json = CJSON_get(level1_json, "[1]");
    assert(level2_json != NULL);
    assert(level2_json->type == CJSON_UINT64);
    assert(level2_json->data.uint64 == 2U);
    //[3, 4]
    level2_json = CJSON_get(level1_json, "[2]");
    assert(level2_json != NULL);
    assert(level2_json->type == CJSON_ARRAY);
    assert(level2_json->data.array.count == 2U);
    //3
    level3_json = CJSON_get(level2_json, "[0]");
    assert(level3_json != NULL);
    assert(level3_json->type == CJSON_UINT64);
    assert(level3_json->data.uint64 == 3U);
    //4
    level3_json = CJSON_get(level2_json, "[1]");
    assert(level3_json != NULL);
    assert(level3_json->type == CJSON_UINT64);
    assert(level3_json->data.uint64 == 4U);

    //[5, 6]
    level1_json = CJSON_get(&root.json, "[1]");
    assert(level1_json != NULL);
    assert(level1_json->type == CJSON_ARRAY);
    assert(level1_json->data.array.count == 2U);

    //5
    level2_json = CJSON_get(level1_json, "[0]");
    assert(level2_json != NULL);
    assert(level2_json->type == CJSON_UINT64);
    assert(level2_json->data.uint64 == 5U);
    //6
    level2_json = CJSON_get(level1_json, "[1]");
    assert(level2_json != NULL);
    assert(level2_json->type == CJSON_UINT64);
    assert(level2_json->data.uint64 == 6U);

    CJSON_free(&root);
}

static void test_duplicate_keys(void) {
    const char duplicate_keys[] = "{\"key\": \"value1\", \"key\": \"value2\"}";

    struct CJSON_Root root;
    bool success = CJSON_parse(&root, duplicate_keys, sizeof(duplicate_keys) - 1);
    assert(success);
    assert(root.json.type == CJSON_OBJECT);

    const char *const value = CJSON_get_string(&root.json, "key", &success);
    assert(success);
    assert(strcmp(value, "value2") == 0);

    CJSON_free(&root);
}

static void test_create_string(void) {
    const char *const value = "test";
    struct CJSON_Root root;
    CJSON_init(&root);
    CJSON_set_string(&root.json, &root, value);
    assert(root.json.type == CJSON_STRING);
    assert(strcmp(root.json.data.string.chars, value) == 0);
    assert(root.json.data.string.chars != value);
    CJSON_free(&root);
}

static void test_create_primitives(void) {
    const int64_t value1 = -25000000000LL;
    struct CJSON_Root root;
    CJSON_init(&root);
    CJSON_set_int64(&root.json, value1);
    assert(root.json.type == CJSON_INT64);
    assert(root.json.data.int64 == value1);
    CJSON_free(&root);

    CJSON_init(&root);
    const uint64_t value2 = 25000000000ULL;
    CJSON_set_uint64(&root.json, value2);
    assert(root.json.type == CJSON_UINT64);
    assert(root.json.data.uint64 == value2);
    CJSON_free(&root);

    CJSON_init(&root);
    const double value3 = 25000000000.50;
    CJSON_set_float64(&root.json, value3);
    assert(root.json.type == CJSON_FLOAT64);
    assert(root.json.data.float64 == value3);
    CJSON_free(&root);

    CJSON_init(&root);
    const bool value4 = true;
    CJSON_set_bool(&root.json, value4);
    assert(root.json.type == CJSON_BOOL);
    assert(root.json.data.boolean);
    CJSON_free(&root);

    CJSON_init(&root);
    CJSON_set_null(&root.json);
    assert(root.json.type == CJSON_NULL);
    assert(root.json.data.null == NULL);
    CJSON_free(&root);
}

static void test_create_array(void) {
    struct CJSON_Root root;
    CJSON_init(&root);
    struct CJSON_Array *const array1 = CJSON_make_array(&root.json, &root);
    assert(root.json.type == CJSON_ARRAY);
    assert(&root.json.data.array == array1);

    const uint64_t value1 = 5ULL;
    const bool value2 = true;
    const int64_t value3 = -25000000000LL;

    struct CJSON_Array array2;
    CJSON_Array_init(&array2, &root, 0U);
    CJSON_Array_set_uint64(&array2, &root, 0, value1);
    
    CJSON_Array_set_array(array1, &root, 0, &array2);
    CJSON_Array_set_bool(array1, &root, 1, value2);
    CJSON_Array_set_int64(array1, &root, 2, value3);

    assert(array2.values[0].type == CJSON_UINT64);
    assert(array2.values[0].data.uint64 == value1);
    assert(array1->values[0].type == CJSON_ARRAY);
    assert(&array1->values[0].data.array != &array2);
    assert(array1->values[1].type == CJSON_BOOL);
    assert(array1->values[1].data.boolean == value2);
    assert(array1->values[2].type == CJSON_INT64);
    assert(array1->values[2].data.int64 == value3);

    CJSON_free(&root);
}

static void test_create_object(void) {
    struct CJSON_Root root;
    CJSON_init(&root);
    struct CJSON_Object *const object1 = CJSON_make_object(&root.json, &root);
    assert(root.json.type == CJSON_OBJECT);
    assert(&root.json.data.object == object1);

    const uint64_t value1 = 5ULL;
    const bool value2 = true;
    const int64_t value3 = -25000000000LL;

    struct CJSON_Object object2;
    CJSON_Object_init(&object2, &root, 0U);
    CJSON_Object_set_uint64(&object2, &root, "key1", value1);
    
    CJSON_Object_set_object(object1, &root, "key1", &object2);
    CJSON_Object_set_bool(object1, &root, "key2", value2);
    CJSON_Object_set_int64(object1, &root, "key3", value3);

    bool success;
    assert(CJSON_Object_get_uint64(&object2, "key1", &success) == value1);
    assert(success);
    assert(CJSON_Object_get_object(object1, "key1", &success) != NULL);
    assert(success);
    assert(CJSON_Object_get_bool(object1, "key2", &success) == value2);
    assert(success);
    assert(CJSON_Object_get_int64(object1, "key3", &success) == value3);
    assert(success);

    CJSON_free(&root);
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