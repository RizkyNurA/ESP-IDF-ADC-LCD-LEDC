#include <stdio.h>
#include "utils.h"

void make_nvs_key(char *buf, size_t len, const char *base, int index)
{
    snprintf(buf, len, "%s_%d", base, index);
}