#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "token-list.h"

static void JSON_Token_list_resize(JSON_Token_List *const list, const double multiplier) {
    assert(list != NULL);
    assert(multiplier > 1.0); //multiplier must actually increase the size
    assert(multiplier <= UINT_MAX / list->capacity); //check overflow

    const unsigned capacity = (unsigned)((double)list->capacity * multiplier);
    JSON_Token *tokens = JSON_REALLOC(list->tokens, (size_t)capacity * sizeof(JSON_Token), list->capacity * sizeof(JSON_Token));
    assert(tokens != NULL);

    list->tokens = tokens;
    list->capacity = capacity;
}

void JSON_Token_List_init(JSON_Token_List *const list) {
    assert(list != NULL);

    JSON_Token *tokens = JSON_MALLOC(INITIAL_TOKEN_LIST_CAPACITY * sizeof(JSON_Token));
    assert(tokens != NULL);

    *list = (JSON_Token_List) {
        .tokens = tokens,
        .capacity = INITIAL_TOKEN_LIST_CAPACITY,
    };
}

inline void JSON_Token_list_free(JSON_Token_List *const list) {
    assert(list != NULL);

    JSON_FREE(list->tokens);
    *list = (JSON_Token_List){0};
}

JSON_Token *JSON_Token_List_next(JSON_Token_List *const list) {
    assert(list != NULL);

    if(list->length == list->capacity) {
        JSON_Token_list_resize(list, 2.0);
    }

    JSON_Token *const token = list->tokens + list->length;
    list->length++;

    return token;
}

