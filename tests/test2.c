#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../json.h"
#include "../util.h"

int main(void) {
    size_t filesize;
    char *const data = file_get_contents("E:\\code\\c\\json\\tests\\really-big-json-file.json", &filesize);

    const long start = usec_timestamp();
    JSON *const json = JSON_parse(data, (unsigned)filesize);
    const long end = usec_timestamp();

    free(data);
    JSON_free(json);

    printf("Parsing time: %li microseconds\n", end - start);

    return 0;
}