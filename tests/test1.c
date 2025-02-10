#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "../parser.h"

static void test_empty_object(void) {
    const char empty_object[] = "{}";
    CJSON_JSON *const root = CJSON_parse(empty_object, sizeof(empty_object) - 1);
    assert(root->type == CJSON_OBJECT);
    CJSON_free(root);
}

static void test_empty_array(void) {
    const char empty_array[] = "[]";
    CJSON_JSON *const root = CJSON_parse(empty_array, sizeof(empty_array) - 1);
    assert(root->type == CJSON_ARRAY);
    CJSON_free(root);
}

static void test_primitive_values(void) {
    const char string[] = "\"\"";
    CJSON_JSON *root = CJSON_parse(string, sizeof(string) - 1);
    assert(root->type == CJSON_STRING);
    assert(strcmp(root->value.string, "") == 0);
    CJSON_free(root);

    const char int64[] = "-125";
    root = CJSON_parse(int64, sizeof(int64) - 1);
    assert(root->type == CJSON_INT64);
    assert(root->value.int64 == -125);
    CJSON_free(root);

    const char uint64[] = "2500";
    root = CJSON_parse(uint64, sizeof(uint64) - 1);
    assert(root->type == CJSON_UINT64);
    assert(root->value.uint64 == 2500);
    CJSON_free(root);

    const char true_value[] = "true";
    root = CJSON_parse(true_value, sizeof(true_value) - 1);
    assert(root->type == CJSON_BOOL);
    assert(root->value.boolean);
    CJSON_free(root);

    const char false_value[] = "false";
    root = CJSON_parse(false_value, sizeof(false_value) - 1);
    assert(root->type == CJSON_BOOL);
    assert(!root->value.boolean);
    CJSON_free(root); 

    const char null_value[] = "null";
    root = CJSON_parse(null_value, sizeof(null_value) - 1);
    assert(root->type == CJSON_NULL);
    assert(root->value.null == NULL);
    CJSON_free(root);   
}

static void test_key_value(void) {
    const char key_value[] = "{\"key\": \"value\"}";
    CJSON_JSON *const root = CJSON_parse(key_value, sizeof(key_value) - 1);
    assert(root->type == CJSON_OBJECT);

    bool success;
    const char *value;
    const CJSON_JSON *json;

    value = CJSON_get_string(root, "key", &success);
    assert(success);
    assert(strcmp(value, "value") == 0);

    value = CJSON_get_string(root, ".key", &success);
    assert(success);
    assert(strcmp(value, "value") == 0);

    json = CJSON_get(root, ".key");
    assert(json != NULL);
    assert(json->type == CJSON_STRING);
    assert(strcmp(json->value.string, "value") == 0);

    value = CJSON_Object_get_string(&root->value.object, "key", &success);
    assert(success);
    assert(strcmp(value, "value") == 0);
    
    json = CJSON_Object_get(&root->value.object, "key");
    assert(json != NULL);
    assert(json->type == CJSON_STRING);
    assert(strcmp(json->value.string, "value") == 0);   

    CJSON_free(root);
}

static void test_nested_objects(void) {
    const char nested_objects[] = "{"
        "\"key1\": {\"innerKey\": \"innerValue\"},"
        "\"key2\": \"value\""
    "}";

    CJSON_JSON *const root = CJSON_parse(nested_objects, sizeof(nested_objects) - 1);
    assert(root->type == CJSON_OBJECT);

    bool success;
    const char *value;
    
    const CJSON_Object *const inner_object = CJSON_get_object(root, ".key1", &success);
    assert(success);
    value = CJSON_Object_get_string(inner_object, "innerKey", &success);
    assert(success);
    assert(strcmp(value, "innerValue") == 0);

    value = CJSON_get_string(root, ".key1.innerKey", &success);
    assert(success);
    assert(strcmp(value, "innerValue") == 0);

    value = CJSON_get_string(root, ".key2", &success);
    assert(success);
    assert(strcmp(value, "value") == 0);

    CJSON_free(root);
}

static void test_struct_array(void) {
    const char struct_array[] = "["
        "{\"key1\": \"value1\"},"
        "{\"key2\": \"value2\"}"
    "]";

    CJSON_JSON *const root = CJSON_parse(struct_array, sizeof(struct_array) - 1);
    assert(root->type == CJSON_ARRAY);
    assert(root->value.array.length == 2U);

    bool success;
    const CJSON_JSON *json;
    const CJSON_Object *object;
    const char *value;

    object = CJSON_get_object(root, "[0]", &success);
    assert(success);
    value = CJSON_Object_get_string(object, "key1", &success);
    assert(success);
    assert(strcmp(value, "value1") == 0);

    object = CJSON_get_object(root, "[1]", &success);    
    value = CJSON_Object_get_string(object, "key2", &success);
    assert(success);
    assert(strcmp(value, "value2") == 0);

    object = CJSON_Array_get_object(&root->value.array, 0U, &success);       
    assert(success);
    value = CJSON_Object_get_string(object, "key1", &success);
    assert(success);
    assert(strcmp(value, "value1") == 0);
    object = CJSON_Array_get_object(&root->value.array, 1U, &success);  
    value = CJSON_Object_get_string(object, "key2", &success);
    assert(success);
    assert(strcmp(value, "value2") == 0);

    json = CJSON_Array_get(&root->value.array, 0U);
    assert(json != NULL);
    assert(json->type == CJSON_OBJECT);
    value = CJSON_Object_get_string(&json->value.object, "key1", &success);
    assert(success);
    assert(strcmp(value, "value1") == 0);
    json = CJSON_Array_get(&root->value.array, 1U);
    assert(json != NULL);
    assert(json->type == CJSON_OBJECT);
    value = CJSON_Object_get_string(&json->value.object, "key2", &success);
    assert(success);
    assert(strcmp(value, "value2") == 0);

    CJSON_free(root);  
}

static void test_escaped_characters(void) {
    const char escaped_characters[] = "{\"key\": \"Line 1\\nLine 2\\\\\"}";

    CJSON_JSON *const root = CJSON_parse(escaped_characters, sizeof(escaped_characters) - 1);
    assert(root->type == CJSON_OBJECT);
    bool success;

    const char *const value = CJSON_get_string(root, "key", &success);
    assert(success);
    assert(value[6] == '\n');   

    CJSON_free(root);
}

static void test_escaped_unicode(void) {
    const char escaped_characters[] = "{\"key\": \"Unicode test: \\u00A9\\u03A9\\uD840\\uDC00\"}";

    CJSON_JSON *const root = CJSON_parse(escaped_characters, sizeof(escaped_characters) - 1);
    assert(root->type == CJSON_OBJECT);

    bool success;
    const char *const value = CJSON_get_string(root, "key", &success);
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

    CJSON_free(root);
}

static void test_bools(void) {
    const char escaped_characters[] = "{\"isTrue\": true, \"isFalse\": false}";

    CJSON_JSON *const root = CJSON_parse(escaped_characters, sizeof(escaped_characters) - 1);
    assert(root->type == CJSON_OBJECT);

    bool success, value;
    CJSON_JSON *json;

    json = CJSON_get(root, "isTrue");
    assert(json != NULL);
    assert(json->type == CJSON_BOOL);
    assert(json->value.boolean);

    value = CJSON_get_bool(root, "isTrue", &success);
    assert(success);
    assert(value);

    json = CJSON_get(root, "isFalse");
    assert(json != NULL);
    assert(json->type == CJSON_BOOL);
    assert(!json->value.boolean);

    value = CJSON_get_bool(root, "isFalse", &success);
    assert(success);
    assert(!value);

    CJSON_free(root);
}

static void test_exponent(void) {
    const char exponent[] = "{\"largeNumber\": 1e15, \"negativeLarge\": -1e15}";

    CJSON_JSON *const root = CJSON_parse(exponent, sizeof(exponent) - 1);
    assert(root->type == CJSON_OBJECT);

    bool success;

    const uint64_t positive_number = CJSON_get_uint64(root, "largeNumber", &success);
    assert(success);
    assert(positive_number == (uint64_t)1e15);

    const int64_t negative_number = CJSON_get_int64(root, "negativeLarge", &success);
    assert(success);
    assert(negative_number == (int64_t)-1e15);

    CJSON_free(root);
}

static void test_null(void) {
    const char null_value[] = "{\"key\": null}";

    CJSON_JSON *const root = CJSON_parse(null_value, sizeof(null_value) - 1);
    assert(root->type == CJSON_OBJECT);

    const CJSON_JSON *const null_json = CJSON_get(root, "key");
    assert(null_json != NULL);
    assert(null_json->type == CJSON_NULL);
    assert(null_json->value.null == NULL);

    bool success;
    void *null = CJSON_get_null(root, "key", &success);
    assert(success);
    assert(null == NULL);

    CJSON_free(root);
}

static void test_missing_value(void) {
    const char missing_key[] = "{\"key1\": \"value1\", \"key2\": }";

    CJSON_JSON *const root = CJSON_parse(missing_key, sizeof(missing_key) - 1);
    assert(root->type == CJSON_ERROR);
    assert(root->value.error == CJSON_OBJECT_FAILED_TO_PARSE);

    CJSON_free(root);
}

static void test_comments(void) {
    const char comments[] = "{"
        "// This is a comment"
        "\"key\": \"value\""
    "}";

    CJSON_JSON *const root = CJSON_parse(comments, sizeof(comments) - 1);
    assert(root->type == CJSON_ERROR);
    assert(root->value.error == CJSON_TOKEN_ERROR);

    CJSON_free(root);
}

static void test_deep_nesting(void) {
    const char deep_nesting[] = "{\"key1\": {\"key2\": {\"key3\": {\"key4\": {\"key5\": \"value\"}}}}}";

    CJSON_JSON *const root = CJSON_parse(deep_nesting, sizeof(deep_nesting) - 1);
    assert(root->type == CJSON_OBJECT);

    bool success;
    const char *const value = CJSON_get_string(root, "key1.key2.key3.key4.key5", &success);
    assert(success);
    assert(strcmp(value, "value") == 0);

    CJSON_free(root);
}

static void test_no_quotes_key(void) {
    const char no_quotes_key[] = "{ key: 1 }";

    CJSON_JSON *const root = CJSON_parse(no_quotes_key, sizeof(no_quotes_key) - 1);
    assert(root->type == CJSON_ERROR);
    assert(root->value.error == CJSON_TOKEN_ERROR);

    CJSON_free(root);
}

static void test_nested_arrays(void) {
    const char nested_arrays[] = "[[1, 2, [3, 4]], [5, 6]]";

    CJSON_JSON *const root = CJSON_parse(nested_arrays, sizeof(nested_arrays) - 1);
    assert(root->type == CJSON_ARRAY);
    assert(root->value.array.length == 2U);

    CJSON_JSON *level1_json, *level2_json, *level3_json;

    //[1, 2, [3, 4]]
    level1_json = CJSON_get(root, "[0]");
    assert(level1_json != NULL);
    assert(level1_json->type == CJSON_ARRAY);
    assert(level1_json->value.array.length == 3U);

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
    assert(level2_json->value.array.length == 2U);
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
    level1_json = CJSON_get(root, "[1]");
    assert(level1_json != NULL);
    assert(level1_json->type == CJSON_ARRAY);
    assert(level1_json->value.array.length == 2U);

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

    CJSON_free(root);
}

static void test_duplicate_keys(void) {
    const char duplicate_keys[] = "{\"key\": \"value1\", \"key\": \"value2\"}";

    CJSON_JSON *const root = CJSON_parse(duplicate_keys, sizeof(duplicate_keys) - 1);
    assert(root->type == CJSON_OBJECT);

    bool success;
    const char *const value = CJSON_get_string(root, "key", &success);
    assert(success);
    assert(strcmp(value, "value2") == 0);

    CJSON_free(root);
}

static void test_create_string(void) {
    const char *const value = "test";
    CJSON_JSON *const root = CJSON_init();
    CJSON_set_string(root, value);
    assert(root->type == CJSON_STRING);
    assert(strcmp(root->value.string, value) == 0);
    assert(root->value.string != value);
    CJSON_free(root);
}

static void test_create_primitives(void) {
    const int64_t value1 = -25000000000LL;
    CJSON_JSON *root = CJSON_init();
    CJSON_set_int64(root, value1);
    assert(root->type == CJSON_INT64);
    assert(root->value.int64 == value1);
    CJSON_free(root);

    const uint64_t value2 = 25000000000ULL;
    root = CJSON_init();
    CJSON_set_uint64(root, value2);
    assert(root->type == CJSON_UINT64);
    assert(root->value.uint64 == value2);
    CJSON_free(root);

    const double value3 = 25000000000.50;
    root = CJSON_init();
    CJSON_set_float64(root, value3);
    assert(root->type == CJSON_FLOAT64);
    assert(root->value.float64 == value3);
    CJSON_free(root);

    const bool value4 = true;
    root = CJSON_init();
    CJSON_set_bool(root, value4);
    assert(root->type == CJSON_BOOL);
    assert(root->value.boolean);
    CJSON_free(root);

    root = CJSON_init();
    CJSON_set_null(root);
    assert(root->type == CJSON_NULL);
    assert(root->value.null == NULL);
    CJSON_free(root);
}

static void test_create_array(void) {
    CJSON_JSON *const root = CJSON_init();
    CJSON_Array *const array1 = CJSON_make_array(root);
    assert(root->type == CJSON_ARRAY);
    assert(&root->value.array == array1);

    const uint64_t value1 = 5ULL;
    const bool value2 = true;
    const int64_t value3 = -25000000000LL;

    CJSON_Array array2;
    CJSON_Array_init(&array2);
    CJSON_Array_set_uint64(&array2, 0, value1);
    
    CJSON_Array_set_array(array1, 0, &array2);
    CJSON_Array_set_bool(array1, 1, value2);
    CJSON_Array_set_int64(array1, 2, value3);

    assert(array2.nodes[0].type == CJSON_UINT64);
    assert(array2.nodes[0].value.uint64 == value1);
    assert(array1->nodes[0].type == CJSON_ARRAY);
    assert(&array1->nodes[0].value.array != &array2);
    assert(array1->nodes[1].type == CJSON_BOOL);
    assert(array1->nodes[1].value.boolean == value2);
    assert(array1->nodes[2].type == CJSON_INT64);
    assert(array1->nodes[2].value.int64 == value3);

    CJSON_free(root);
}

static void test_create_object(void) {
    CJSON_JSON *const root = CJSON_init();
    CJSON_Object *const object1 = CJSON_make_object(root);
    assert(root->type == CJSON_OBJECT);
    assert(&root->value.object == object1);

    const uint64_t value1 = 5ULL;
    const bool value2 = true;
    const int64_t value3 = -25000000000LL;

    CJSON_Object object2;
    CJSON_Object_init(&object2);
    CJSON_Object_set_uint64(&object2, "key1", value1);
    
    CJSON_Object_set_object(object1, "key1", &object2);
    CJSON_Object_set_bool(object1, "key2", value2);
    CJSON_Object_set_int64(object1, "key3", value3);

    bool success;
    assert(CJSON_Object_get_uint64(&object2, "key1", &success) == value1);
    assert(success);
    assert(CJSON_Object_get_object(object1, "key1", &success) != NULL);
    assert(success);
    assert(CJSON_Object_get_bool(object1, "key2", &success) == value2);
    assert(success);
    assert(CJSON_Object_get_int64(object1, "key3", &success) == value3);
    assert(success);

    CJSON_free(root);
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

    return 0;
}