#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "../json.h"

static void test_empty_object(void) {
    const char empty_object[] = "{}";
    JSON *const root = JSON_parse(empty_object, sizeof(empty_object) - 1);
    assert(root->type == JSON_OBJECT);
    JSON_free(root);
}

static void test_empty_array(void) {
    const char empty_array[] = "[]";
    JSON *const root = JSON_parse(empty_array, sizeof(empty_array) - 1);
    assert(root->type == JSON_ARRAY);
    JSON_free(root);
}

static void test_primitive_values(void) {
    const char string[] = "\"\"";
    JSON *root = JSON_parse(string, sizeof(string) - 1);
    assert(root->type == JSON_STRING);
    assert(strcmp(root->value.string, "") == 0);
    JSON_free(root);

    const char int64[] = "-125";
    root = JSON_parse(int64, sizeof(int64) - 1);
    assert(root->type == JSON_INT64);
    assert(root->value.int64 == -125);
    JSON_free(root);

    const char uint64[] = "2500";
    root = JSON_parse(uint64, sizeof(uint64) - 1);
    assert(root->type == JSON_UINT64);
    assert(root->value.uint64 == 2500);
    JSON_free(root);

    const char true_value[] = "true";
    root = JSON_parse(true_value, sizeof(true_value) - 1);
    assert(root->type == JSON_BOOL);
    assert(root->value.boolean);
    JSON_free(root);

    const char false_value[] = "false";
    root = JSON_parse(false_value, sizeof(false_value) - 1);
    assert(root->type == JSON_BOOL);
    assert(!root->value.boolean);
    JSON_free(root); 

    const char null_value[] = "null";
    root = JSON_parse(null_value, sizeof(null_value) - 1);
    assert(root->type == JSON_NULL);
    assert(root->value.null == NULL);
    JSON_free(root);   
}

static void test_key_value(void) {
    const char key_value[] = "{\"key\": \"value\"}";
    JSON *const root = JSON_parse(key_value, sizeof(key_value) - 1);
    assert(root->type == JSON_OBJECT);

    bool success;
    const char *value;
    const JSON *json;

    value = JSON_get_string(root, "key", &success);
    assert(success);
    assert(strcmp(value, "value") == 0);

    value = JSON_get_string(root, ".key", &success);
    assert(success);
    assert(strcmp(value, "value") == 0);

    json = JSON_get(root, ".key");
    assert(json != NULL);
    assert(json->type == JSON_STRING);
    assert(strcmp(json->value.string, "value") == 0);

    value = JSON_Object_get_string(&root->value.object, "key", &success);
    assert(success);
    assert(strcmp(value, "value") == 0);
    
    json = JSON_Object_get(&root->value.object, "key");
    assert(json != NULL);
    assert(json->type == JSON_STRING);
    assert(strcmp(json->value.string, "value") == 0);   

    JSON_free(root);
}

static void test_nested_objects(void) {
    const char nested_objects[] = "{"
        "\"key1\": {\"innerKey\": \"innerValue\"},"
        "\"key2\": \"value\""
    "}";

    JSON *const root = JSON_parse(nested_objects, sizeof(nested_objects) - 1);
    assert(root->type == JSON_OBJECT);

    bool success;
    const char *value;
    
    const JSON_Object *const inner_object = JSON_get_object(root, ".key1", &success);
    assert(success);
    value = JSON_Object_get_string(inner_object, "innerKey", &success);
    assert(success);
    assert(strcmp(value, "innerValue") == 0);

    value = JSON_get_string(root, ".key1.innerKey", &success);
    assert(success);
    assert(strcmp(value, "innerValue") == 0);

    value = JSON_get_string(root, ".key2", &success);
    assert(success);
    assert(strcmp(value, "value") == 0);

    JSON_free(root);
}

static void test_struct_array(void) {
    const char struct_array[] = "["
        "{\"key1\": \"value1\"},"
        "{\"key2\": \"value2\"}"
    "]";

    JSON *const root = JSON_parse(struct_array, sizeof(struct_array) - 1);
    assert(root->type == JSON_ARRAY);
    assert(root->value.array.length == 2U);

    bool success;
    const JSON *json;
    const JSON_Object *object;
    const char *value;

    object = JSON_get_object(root, "[0]", &success);
    assert(success);
    value = JSON_Object_get_string(object, "key1", &success);
    assert(success);
    assert(strcmp(value, "value1") == 0);

    object = JSON_get_object(root, "[1]", &success);    
    value = JSON_Object_get_string(object, "key2", &success);
    assert(success);
    assert(strcmp(value, "value2") == 0);

    object = JSON_Array_get_object(&root->value.array, 0U, &success);       
    assert(success);
    value = JSON_Object_get_string(object, "key1", &success);
    assert(success);
    assert(strcmp(value, "value1") == 0);
    object = JSON_Array_get_object(&root->value.array, 1U, &success);  
    value = JSON_Object_get_string(object, "key2", &success);
    assert(success);
    assert(strcmp(value, "value2") == 0);

    json = JSON_Array_get(&root->value.array, 0U);
    assert(json != NULL);
    assert(json->type == JSON_OBJECT);
    value = JSON_Object_get_string(&json->value.object, "key1", &success);
    assert(success);
    assert(strcmp(value, "value1") == 0);
    json = JSON_Array_get(&root->value.array, 1U);
    assert(json != NULL);
    assert(json->type == JSON_OBJECT);
    value = JSON_Object_get_string(&json->value.object, "key2", &success);
    assert(success);
    assert(strcmp(value, "value2") == 0);

    JSON_free(root);  
}

static void test_escaped_characters(void) {
    const char escaped_characters[] = "{\"key\": \"Line 1\\nLine 2\\\\\"}";

    JSON *const root = JSON_parse(escaped_characters, sizeof(escaped_characters) - 1);
    assert(root->type == JSON_OBJECT);
    bool success;

    const char *const value = JSON_get_string(root, "key", &success);
    assert(success);
    assert(value[6] == '\n');   

    JSON_free(root);
}

static void test_escaped_unicode(void) {
    const char escaped_characters[] = "{\"key\": \"Unicode test: \\u00A9\\u03A9\\uD840\\uDC00\"}";

    JSON *const root = JSON_parse(escaped_characters, sizeof(escaped_characters) - 1);
    assert(root->type == JSON_OBJECT);

    bool success;
    const char *const value = JSON_get_string(root, "key", &success);
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

    JSON_free(root);
}

static void test_bools(void) {
    const char escaped_characters[] = "{\"isTrue\": true, \"isFalse\": false}";

    JSON *const root = JSON_parse(escaped_characters, sizeof(escaped_characters) - 1);
    assert(root->type == JSON_OBJECT);

    bool success, value;
    JSON *json;

    json = JSON_get(root, "isTrue");
    assert(json != NULL);
    assert(json->type == JSON_BOOL);
    assert(json->value.boolean);

    value = JSON_get_bool(root, "isTrue", &success);
    assert(success);
    assert(value);

    json = JSON_get(root, "isFalse");
    assert(json != NULL);
    assert(json->type == JSON_BOOL);
    assert(!json->value.boolean);

    value = JSON_get_bool(root, "isFalse", &success);
    assert(success);
    assert(!value);

    JSON_free(root);
}

static void test_exponent(void) {
    const char exponent[] = "{\"largeNumber\": 1e15, \"negativeLarge\": -1e15}";

    JSON *const root = JSON_parse(exponent, sizeof(exponent) - 1);
    assert(root->type == JSON_OBJECT);

    bool success;

    const uint64_t positive_number = JSON_get_uint64(root, "largeNumber", &success);
    assert(success);
    assert(positive_number == (uint64_t)1e15);

    const int64_t negative_number = JSON_get_int64(root, "negativeLarge", &success);
    assert(success);
    assert(negative_number == (int64_t)-1e15);

    JSON_free(root);
}

static void test_null(void) {
    const char null_value[] = "{\"key\": null}";

    JSON *const root = JSON_parse(null_value, sizeof(null_value) - 1);
    assert(root->type == JSON_OBJECT);

    const JSON *const null_json = JSON_get(root, "key");
    assert(null_json != NULL);
    assert(null_json->type == JSON_NULL);
    assert(null_json->value.null == NULL);

    bool success;
    void *null = JSON_get_null(root, "key", &success);
    assert(success);
    assert(null == NULL);

    JSON_free(root);
}

static void test_missing_value(void) {
    const char missing_key[] = "{\"key1\": \"value1\", \"key2\": }";

    JSON *const root = JSON_parse(missing_key, sizeof(missing_key) - 1);
    assert(root->type == JSON_ERROR);
    assert(root->value.error == JSON_OBJECT_FAILED_TO_PARSE);

    JSON_free(root);
}

static void test_comments(void) {
    const char comments[] = "{"
        "// This is a comment"
        "\"key\": \"value\""
    "}";

    JSON *const root = JSON_parse(comments, sizeof(comments) - 1);
    assert(root->type == JSON_ERROR);
    assert(root->value.error == JSON_TOKEN_ERROR);

    JSON_free(root);
}

static void test_deep_nesting(void) {
    const char deep_nesting[] = "{\"key1\": {\"key2\": {\"key3\": {\"key4\": {\"key5\": \"value\"}}}}}";

    JSON *const root = JSON_parse(deep_nesting, sizeof(deep_nesting) - 1);
    assert(root->type == JSON_OBJECT);

    bool success;
    const char *const value = JSON_get_string(root, "key1.key2.key3.key4.key5", &success);
    assert(success);
    assert(strcmp(value, "value") == 0);

    JSON_free(root);
}

static void test_no_quotes_key(void) {
    const char no_quotes_key[] = "{ key: 1 }";

    JSON *const root = JSON_parse(no_quotes_key, sizeof(no_quotes_key) - 1);
    assert(root->type == JSON_ERROR);
    assert(root->value.error == JSON_TOKEN_ERROR);

    JSON_free(root);
}

static void test_nested_arrays(void) {
    const char nested_arrays[] = "[[1, 2, [3, 4]], [5, 6]]";

    JSON *const root = JSON_parse(nested_arrays, sizeof(nested_arrays) - 1);
    assert(root->type == JSON_ARRAY);
    assert(root->value.array.length == 2U);

    JSON *level1_json, *level2_json, *level3_json;

    //[1, 2, [3, 4]]
    level1_json = JSON_get(root, "[0]");
    assert(level1_json != NULL);
    assert(level1_json->type == JSON_ARRAY);
    assert(level1_json->value.array.length == 3U);

    //1
    level2_json = JSON_get(level1_json, "[0]");
    assert(level2_json != NULL);
    assert(level2_json->type == JSON_UINT64);
    assert(level2_json->value.uint64 == 1U);
    //2
    level2_json = JSON_get(level1_json, "[1]");
    assert(level2_json != NULL);
    assert(level2_json->type == JSON_UINT64);
    assert(level2_json->value.uint64 == 2U);
    //[3, 4]
    level2_json = JSON_get(level1_json, "[2]");
    assert(level2_json != NULL);
    assert(level2_json->type == JSON_ARRAY);
    assert(level2_json->value.array.length == 2U);
    //3
    level3_json = JSON_get(level2_json, "[0]");
    assert(level3_json != NULL);
    assert(level3_json->type == JSON_UINT64);
    assert(level3_json->value.uint64 == 3U);
    //4
    level3_json = JSON_get(level2_json, "[1]");
    assert(level3_json != NULL);
    assert(level3_json->type == JSON_UINT64);
    assert(level3_json->value.uint64 == 4U);

    //[5, 6]
    level1_json = JSON_get(root, "[1]");
    assert(level1_json != NULL);
    assert(level1_json->type == JSON_ARRAY);
    assert(level1_json->value.array.length == 2U);

    //5
    level2_json = JSON_get(level1_json, "[0]");
    assert(level2_json != NULL);
    assert(level2_json->type == JSON_UINT64);
    assert(level2_json->value.uint64 == 5U);
    //6
    level2_json = JSON_get(level1_json, "[1]");
    assert(level2_json != NULL);
    assert(level2_json->type == JSON_UINT64);
    assert(level2_json->value.uint64 == 6U);

    JSON_free(root);
}

static void test_duplicate_keys(void) {
    const char duplicate_keys[] = "{\"key\": \"value1\", \"key\": \"value2\"}";

    JSON *const root = JSON_parse(duplicate_keys, sizeof(duplicate_keys) - 1);
    assert(root->type == JSON_OBJECT);

    bool success;
    const char *const value = JSON_get_string(root, "key", &success);
    assert(success);
    assert(strcmp(value, "value2") == 0);

    JSON_free(root);
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

    return 0;
}