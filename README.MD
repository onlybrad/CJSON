# CJSON

**JSON parser in C (can be compiled and/or used with C++ too).**

## Table of contents
- [Table of contents](#table-of-contents)
- [Requirements](#requirements)
- [Compilation](#compilation)
- [Usage](#usage)
- [Parse From a File](#parse-from-a-file)
- [Parse From a String](#parse-from-a-string)
- [CJSON](#struct-cjson)
    - [Check Type](#check-the-json-type)
    - [Query Specific Value](#query-a-specific-value)
    - [Query Specific Value of a Specific Type](#query-a-specific-value-of-a-specific-type)
    - [Set Specific Value of a Specific Type](#set-a-specific-value-of-a-specific-type)
    - [Get Parsing Error as a string](#get-parsing-error-as-a-string)
- [CJSON_Object](#struct-cjson_object)
    - [Get Value with a Key](#get-a-value-with-a-key)
    - [Get Value of a Specific Type with a Key](#get-a-value-of-a-specific-type-with-a-key)
    - [Delete a key](#delete-a-key)
    - [Set Value to a Key](#set-a-value-to-a-key)
    - [Set Value of a Specific Type to a Key](#set-a-value-of-a-specific-type-to-a-key)
- [CJSON_Array](#struct-cjson_array)
    - [Get Value With an Index](#get-a-value-with-an-index)
    - [Get Value of a Specific Type With an Index](#get-a-value-with-an-index)
    - [Loop](#loop)
    - [Set Value at an Index](#set-value-at-index)
    - [Set Value of a Specific Type at an Index](#set-value-of-specific-type-at-index)
    - [Push Value](#push-value)

## Requirements
- Makefile
- MinGW (for Windows)

## Compilation
Static library:

```bash
make static
```

This will create a release build of the static library as cjson.a, link your program to this static library when compiling.

## Usage.

### Parse From a File.

```c
#include <cjson.h>
#include <stdlib.h>

int main(void) {
    struct CJSON_Parser parser;
    //call CJSON_Parser_init to initialize the CJSON_Parser struct in a valid
    CJSON_Parser_init(&parser);
    const struct CJSON *const json = CJSON_parse_file(&parser, "path/to/file");
    if(json == NULL) {
        return EXIT_FAILURE;
    }

    //...

    //call CJSON_Parser_free to deallocate all the resources used by the library
    CJSON_Parser_free(&parser);

    return EXIT_SUCCESS;
}
```

### Parse From a String.

```c
#include <cjson.h>
#include <stdlib.h>

int main(void) {
    const char json_example[] = "{"
        "\"key1\": {\"innerKey\": \"innerValue\"},"
        "\"key2\": \"value\""
    "}";

    struct CJSON_Parser parser;
    //call CJSON_Parser_init to initialize the CJSON_Parser struct in a valid
    CJSON_Parser_init(&parser);
    const struct CJSON *const json = CJSON_parse(&parser, json_example, (unsigned)(sizeof(json_example)) - 1U);
    if(json == NULL) {
        return EXIT_FAILURE;
    }

    //...

    
    //call CJSON_Parser_free to deallocate all the resources used by the library
    CJSON_Parser_free(&parser);

    return EXIT_SUCCESS;
}
```

### Struct CJSON.

### Check The Json Type.

```c
#include <cjson.h>
#include <stdlib.h>

int main(void) {
    //...

    switch(json->type) {
    case CJSON_OBJECT:
        const struct CJSON_Object *const object = &json->data.object;
        break;
    case CJSON_ARRAY:
        const struct CJSON_Array *const array = &json->data.array;
        break;
    case CJSON_STRING:
        const struct CJSON_String *const string = &json->data.string;
        const char *const c_string              = string->chars;
        const unsigned length                   = string->length;
        break;
    case CJSON_FLOAT64:
        const double float64 = json->data.float64;
        break;
    case CJSON_INT64:
        const int64_t int64 = json->data.int64;
        break;
    case CJSON_UINT64:
        const int64_t uint64 = json->data.uint64;
        break;
    case CJSON_BOOL:
        const bool boolean = json->data.boolean;
        break;
    case CJSON_NULL:
        const void *const null = json->data.null;
        break;
    case CJSON_ERROR:
        const enum CJSON_Error error = json->data.error;
        break;
    }

    CJSON_Parser_free(&parser);

    return EXIT_SUCCESS;
}
```

### Query a Specific Value.

The query is a c-string that ressembles the way you would access a value inside an object or an array in javascript.

Object keys must start with a dot (if it's the first character, it can be omitted) and can have any valid json character. Example: .some-key is allowed even though it's not allowed in javascript outside of brackets.

Array indices must be in brackets [], and the maximum index supported
is UINT_MAX.

example:

```c
".some-key.some-array[10].someOtherKey"
"some-key.some-array[10].someOtherKey" //equivalent
```

 Or you can use the struct CJSON_QueryBuilder and associated functions.

```c
#include <cjson.h>
#include <stdlib.h>

int main(void) {
    //...

    //first way
    const struct CJSON *const my_value = CJSON_get(json, ".key1.innerKey[5]");
    if(my_value != NULL) {
        //if my_value is not NULL then the CJSON struct at the parser has this form: {"key1": {"innerKey": [...,...,...,...,...,my_value]}}
        
        //...
    }

    //second way
    struct CJSON_QueryBuilder query_builder = CJSON_get_query_builder(json);
    CJSON_QueryBuilder_key(&query_builder, "key1");
    CJSON_QueryBuilder_key(&query_builder, "innerKey");
    CJSON_QueryBuilder_index(&query_builder, 5U);
    if(query_builder.json != NULL) {
        //if my_value is not NULL then the CJSON struct at the parser has this form: {"key1": {"innerKey": [...,...,...,...,...,my_value]}}
        
        //...
    }

    //third way. the format accepts 2 characters: 'k' for keys and 'i' for index. The types are const char* and unsigned respectively. Any other character is ignored.
    query_builder = CJSON_get_query_builder(json);
    CJSON_QueryBuilder_format(&query_builder, "kki", "key1", "key2", 5U);
    if(query_builder.json != NULL) {
        //if my_value is not NULL then the CJSON struct at the parser has this form: {"key1": {"innerKey": [...,...,...,...,...,my_value]}}
        
        //...
    }

    CJSON_Parser_free(&parser);

    return EXIT_SUCCESS;
}
```

### Query a Specific Value of a Specific Type.

Available functions:

- CJSON_get_array   => CJSON_Array*
- CJSON_get_object  => CJSON_Object*
- CJSON_get_string  => const char*
- CJSON_get_float64 => double
- CJSON_get_int64   => int64_t
- CJSON_get_uint64  => uint64_t
- CJSON_get_bool    => bool
- CJSON_get_null    => void* (NULL) 

```c
#include <cjson.h>
#include <stdlib.h>

int main(void) {
    //...

    bool success;
    const struct CJSON_Array *const my_array = CJSON_get_array(json, ".someKey.innerKey", &success);
    if(success) {
        //if success is true
        //   the CJSON struct at the parser has this form: {"key1": {"innerKey": my_value}}
        //   the type of the underlying CJSON struct is CJSON_ARRAY. my_array is not NULL.

        //...
    } else {
        //my_array is NULL
        //numerical types will be equal to 0, bool will be equal to false, null is always NULL
    }

    CJSON_Parser_free(&parser);

    return EXIT_SUCCESS;
}
```

### Set a Specific Value of a Specific Type.

Available functions:

- CJSON_set_array
- CJSON_set_object
- CJSON_set_string
- CJSON_set_float64
- CJSON_set_int64
- CJSON_set_uint64
- CJSON_set_bool
- CJSON_set_null 

```c
#include <cjson.h>
#include <stdlib.h>

int main(void) {
    //...

    struct CJSON_Array array;
    CJSON_Array_init(&array);

    //The CJSON_Array struct will be shallow copied by the CJSON_set_array function
    //The parser is now "[]"
    CJSON_set_array(json, &array);

    //The parser is now "true"
    CJSON_set_bool(json, true);
    
    //..

    CJSON_Parser_free(&parser);

    return EXIT_SUCCESS;
}
```

### Get Parsing Error as a String.

use the CJSON_get_error function to get the error as a statically allocated c-string.

```c
#include <cjson.h>
#include <stdlib.h>
#include <stdio.h>

int main(void) {
    struct CJSON_Parser parser = ...;

    if(!CJSON_parse_file(&parser, "/some/path/to/file.json")) {
        fputs(CJSON_get_error(&parser), stderr);
        return EXIT_FAILURE;
    }

    //...

    return EXIT_SUCCESS;
}
```
### Struct CJSON_Object.

### Get a Value With a Key.

```c
#include <cjson.h>
#include <stdlib.h>

int main(void) {
    //...

    struct CJSON_Object *object = ...;

    //...

    const struct CJSON *const some_value = CJSON_Object_get(object, "some-key");
    if(some_value != NULL) {
        //if some_value is not NULL then the CJSON_Object struct contains a key "some-key"
    }

    CJSON_Parser_free(&parser);

    return EXIT_SUCCESS;
}
```

### Get a Value of a Specific Type With a Key.

Available functions:

- CJSON_Object_get_string  => const char*
- CJSON_Object_get_float64 => double
- CJSON_Object_get_int64   => int64_t
- CJSON_Object_get_uint64  => uint64_t
- CJSON_Object_get_object  => struct CJSON_Object*
- CJSON_Object_get_array   => struct CJSON_Array*
- CJSON_Object_get_bool    => bool
- CJSON_Object_get_null    => void*

```c
#include <cjson.h>
#include <stdlib.h>

int main(void) {
    //...

    struct CJSON_Object *object = ...;

    //...

    bool success;
    const struct CJSON_Array *const some_array = CJSON_Object_get_array(object, "some-key", &success);
    if(success) {
        //if sucess is true then the CJSON_Object struct contains a key "some-key" and the type of the underlying CJSON struct value is CJSON_ARRAY. some_array is not NULL
    } else {
        //some_array is NULL
        //numerical types will be equal to 0, bool will be equal to false, null is always NULL
    }

    CJSON_Parser_free(&parser);

    return EXIT_SUCCESS;
}
```

### Delete a Key.
```c
#include <cjson.h>
#include <stdlib.h>

int main(void) {
    //...

    struct CJSON_Object *object = ...;

    //...

    CJSON_Object_delete(object, "some-key");

    CJSON_Parser_free(&parser);

    return EXIT_SUCCESS;
}
```

### Set a Value to a Key.

```c
#include <cjson.h>
#include <stdlib.h>

int main(void) {
    //...

    struct CJSON        *some_value = ...;
    struct CJSON_Object *object     = ...;

    //...

    if(CJSON_Object_set(object, &parser, "some-key", some_value)) {
        //The CJSON struct value has been shallow copied into the internal data array
    } else {
        //this only happen if memory allocation fails when resizing the internal data array or copying the key
    }

    CJSON_Parser_free(&parser);

    return EXIT_SUCCESS;
}
```

### Set a Value of a Specific Type to a Key

Available functions:

- CJSON_Object_set_string  => bool
- CJSON_Object_set_float64 => bool
- CJSON_Object_set_int64   => bool
- CJSON_Object_set_uint64  => bool
- CJSON_Object_set_object  => bool
- CJSON_Object_set_array   => bool
- CJSON_Object_set_bool    => bool
- CJSON_Object_set_null    => bool

```c
#include <cjson.h>
#include <stdlib.h>

int main(void) {
    //...

    struct CJSON_Object *object = ...;

    //...

    struct CJSON_Array array;
    CJSON_Array_init(&array);

    if(CJSON_Object_set_array(object, &parser, "some-key", &array)) {
        //The CJSON_Array struct has been shallow copied into the internal data array
    } else {
        //this only happen if memory allocation fails when resizing the internal data array or copying the key
    }

    CJSON_Parser_free(&parser);

    return EXIT_SUCCESS;
}
```

### Struct CJSON_Array.

### Get a Value With an Index.

```c
#include <cjson.h>
#include <stdlib.h>

int main(void) {
    //...

    struct CJSON_Array *array = ...;

    //...

    const struct CJSON *const some_value = CJSON_Array_get(array, 10U);
    if(some_value != NULL) {
        //if some_value is not NULL then the CJSON_Array struct has a value at index 10.
    }

    CJSON_Parser_free(&parser);

    return EXIT_SUCCESS;
}
```

### Get a Value of a Specific Type With an Index.

Available functions:

- CJSON_Array_get_string  => const char*
- CJSON_Array_get_float64 => double
- CJSON_Array_get_int64   => int64_t
- CJSON_Array_get_uint64  => uint64_t
- CJSON_Array_get_object  => struct CJSON_Object*
- CJSON_Array_get_array   => struct CJSON_Array*
- CJSON_Array_get_bool    => bool
- CJSON_Array_get_null    => void*

```c
#include <cjson.h>
#include <stdlib.h>

int main(void) {
    //...

    struct CJSON_Array *array = ...;

    //...

    bool success;
    const struct CJSON_Array *const some_array = CJSON_Array_get_array(array, 10U, &success);
    if(success) {
        //if sucess is true then the CJSON_Array struct contains a value at index 10 and the type of the underlying CJSON struct value is CJSON_ARRAY. some_array is not NULL
    } else {
        //some_array is NULL
        //numerical types will be equal to 0, bool will be equal to false, null is always NULL
    }

    CJSON_Parser_free(&parser);

    return EXIT_SUCCESS;
}
```

### Loop.

use the "count" member to access the length of the CJSON_Array struct, and "values" to access the values of the CJSON_Array struct.

```c
#include <cjson.h>
#include <stdlib.h>

int main(void) {
    //...

    struct CJSON_Array *array = ...;

    //...

    for(unsigned i = 0U; i < array->count; i++) {
        struct CJSON *const value = array->values[i];
        
        //...
    }

    CJSON_Parser_free(&parser);

    return EXIT_SUCCESS;
}
```

### Set Value at Index.

If the index is greater than the length of the array, all CJSON struct values between last element and the index will be zeroed (CJSON_Type == CJSON_NULL)

```c
#include <cjson.h>
#include <stdlib.h>

int main(void) {
    //...

    struct CJSON       *some_value = ...;
    struct CJSON_Array *array      = ...;

    //...

    if(CJSON_Array_set(array, &parser, 10U, some_value)) {
        //The CJSON struct value has been shallow copied into the internal data array at index 10
    } else {
        //this only happen if memory allocation fails when resizing the internal data array
    }

    CJSON_Parser_free(&parser);

    return EXIT_SUCCESS;
}
```

### Set Value of Specific Type at Index.

If the index is greater than the length of the array, all CJSON struct values between last element and the index will be zeroed (CJSON_Type == CJSON_NULL)

Available functions:

- CJSON_Array_set_string  => bool
- CJSON_Array_set_float64 => bool
- CJSON_Array_set_int64   => bool
- CJSON_Array_set_uint64  => bool
- CJSON_Array_set_object  => bool
- CJSON_Array_set_array   => bool
- CJSON_Array_set_bool    => bool
- CJSON_Array_set_null    => bool

```c
#include <cjson.h>
#include <stdlib.h>

int main(void) {
    //...

    struct CJSON_Array *array;

    //...

    struct CJSON_Object object;
    CJSON_Object_init(&object);

    if(CJSON_Array_set_object(array, &parser, 10U, &object)) {
        //The CJSON_Object struct has been shallow copied into the internal data array at index 10
    } else {
        //this only happen if memory allocation fails when resizing the internal data array
    }

    CJSON_Parser_free(&parser);

    return EXIT_SUCCESS;
}
```

### Push Value.

```c
#include <cjson.h>
#include <stdlib.h>

int main(void) {
    //...

    struct CJSON_Array *array = ...;

    //...

    struct CJSON number;
    CJSON_set_uint64(&number, 1024U);

    if(CJSON_Array_push(array, &parser, &number)) {
        //The CJSON_Object struct has been shallow copied into the internal data array and is the last element of the CJSON_Array struct
    } else {
        //this only happen if memory allocation fails when resizing the internal data array
    }

    CJSON_Parser_free(&parser);

    return EXIT_SUCCESS;
}
```