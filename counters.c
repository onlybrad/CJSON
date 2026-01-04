#include <assert.h>
#include <stdlib.h>

#include "counters.h"
#include "util.h"

EXTERN_C void CJSON_Counters_init(struct CJSON_Counters *const counters) {
    assert(counters != NULL);

    counters->object          = 0U;
    counters->array           = 0U;
    counters->number          = 0U;
    counters->string          = 0U;
    counters->keyword         = 0U;
    counters->chars           = 0U;
    counters->comma           = 0U;
    counters->object_elements = 0U;
    counters->array_elements  = 0U;
}
