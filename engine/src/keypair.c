#include "keypair.h"
#include "../../thirdparty/cjson/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int load_keypair(Keypair *kp, const char *path) {

    FILE *fptr = fopen(path, "r");
    if (!fptr)
        return -1;
    fseek(fptr, 0, SEEK_END);
    long sz = ftell(fptr);
    rewind(fptr);

    char *buf = malloc(sz + 1);
    fread(buf, 1, sz, fptr);
    buf[sz] = '\0';
    fclose(fptr);

    cJSON *arr = cJSON_Parse(buf);
    free(buf);
    if (!arr || !cJSON_IsArray(arr) || cJSON_GetArraySize(arr) != 64) {
        cJSON_Delete(arr);
        return -1;
    }

    for (int i = 0; i < 32; i++)
        kp->secret[i] = (uint8_t)cJSON_GetArrayItem(arr, i)->valueint;
    for (int i = 0; i < 32; i++)
        kp->pubkey[i] = (uint8_t)cJSON_GetArrayItem(arr, 32 + i)->valueint;

    cJSON_Delete(arr);

    return 0;
}
