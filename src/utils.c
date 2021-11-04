#include "global.h"

void error(const char * s) {
    fprintf(stderr, "ERROR: %s\n", s);
    exit(EXIT_FAILURE);
}

void error_w_line(const char * s, unsigned int line) {
    fprintf(stderr, "ERROR line %u: %s\n", line, s);
    exit(EXIT_FAILURE);
}
