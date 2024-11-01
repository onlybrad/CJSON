#ifndef JSON_TOKEN_LIST_H
#define JSON_TOKEN_LIST_H

#define INITIAL_TOKEN_LIST_CAPACITY (1 << 10)

#include <stdbool.h>
#include "token.h"
#include "allocator.h"

typedef struct JSON_Token_List {
    JSON_Token *tokens;
    unsigned index;
    unsigned length;
    unsigned capacity;
} JSON_Token_List;

void JSON_Token_List_init(JSON_Token_List *const list);
void JSON_Token_list_free(JSON_Token_List *const list);
JSON_Token *JSON_Token_List_next(JSON_Token_List *const list);

#endif
