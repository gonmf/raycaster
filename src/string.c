#include "global.h"

int start_with(const char * s, const char * prefix) {
    return strlen(prefix) < strlen(s) && strncmp(prefix, s, strlen(prefix)) == 0;
}
