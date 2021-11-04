#include "global.h"

char * file_read(const char * filename) {
    char * buffer = malloc(MAX_FILE_SIZE);
    FILE * file = fopen(filename, "rb");

    if (file == NULL) {
        sprintf(buffer, "Could not open file \"%s\"", filename);
        error(buffer);
    }

    printf("Read \"%s\"\n", filename);

    fread(buffer, 1, MAX_FILE_SIZE, file);

    fclose(file);
    
    return buffer;
}
