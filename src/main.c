#include <stdio.h>
#include "../include/hashtable.h"

int main() {
    struct hashMap map;
    initHashMap(&map);

    set(&map, "janis", "17");
    set(&map, "marta", "19");
    set(&map, "martins", "20");
    set(&map, "aigats", "50");

    char* janisAge = get(&map, "janis");
    printf("janis age = %s\n", janisAge);
    printf("martins age = %s\n", get(&map, "martins"));

    printf("exists marta?: %i\n", exists(&map, "marta"));
    del(&map, "marta");
    printf("exists marta?: %i\n", exists(&map, "marta"));
    printf("exists martina?: %i\n", exists(&map, "martina"));

    return 0;
}   