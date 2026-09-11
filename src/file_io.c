#include "global.h"

unsigned int file_read(char * dst, unsigned int max_size, const char * filename) {
    FILE * file = fopen(filename, "rb");

    if (file == NULL) {
        char * sb = calloc(MAX_FILE_NAME_SIZ, 1);
        sprintf(sb, "Could not open file \"%s\"", filename);
        error(sb);
    }

    unsigned int total_read = 0;

    while (total_read < max_size) {
        size_t read = fread(dst + total_read, 1, max_size - total_read, file);

        if (ferror(file)) {
            char * sb = calloc(MAX_FILE_NAME_SIZ, 1);
            sprintf(sb, "Error reading file \"%s\"", filename);
            error(sb);
        }

        total_read += (unsigned int)read;

        if (read == 0) {
            break;
        }
    }

    fclose(file);

    return total_read;
}
