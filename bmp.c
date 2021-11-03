#include "global.h"

void read_sprite_pack(
    pixel_t * dst, const char * filename,
    unsigned int pack_width, unsigned int pack_height
) {
    char * buffer = malloc(MAX_SPRITES_FILE_SIZE);
    sprintf(buffer, "./sprites/%s.bmp", filename);

    FILE * file = fopen(buffer, "rb");
    if (file == NULL) {
        sprintf(buffer, "Could not open file \"./sprites/%s.bmp\"", filename);
        error(buffer);
    }
    printf( "Opening \"./sprites/%s.bmp\"\n", filename);

    size_t read = fread(buffer, 1, MAX_SPRITES_FILE_SIZE, file);
    fclose(file);

    buffer[read] = 0;

    if (buffer[0] != 0x42 || buffer[1] != 0x4D) {
        fprintf(stderr, "Not a Windows NT Bitmap (BM) file\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }


    unsigned int data_offset = (((unsigned int)buffer[13]) << 24) + (((unsigned int)buffer[12]) << 16) + (((unsigned int)buffer[11]) << 8) + buffer[10];
    char * data = buffer + data_offset;

    for (unsigned int y = 0; y < pack_height * SPRITE_HEIGHT; ++y) {
        for (unsigned int x = 0; x < pack_width * SPRITE_WIDTH; ++x) {
            unsigned int dst_idx = x + y * pack_width * SPRITE_WIDTH;
            unsigned int src_idx = x + (pack_height * SPRITE_HEIGHT - y - 1) * pack_width * SPRITE_WIDTH;

            dst[dst_idx].red = data[src_idx * 3 + 2];
            dst[dst_idx].green = data[src_idx * 3 + 1];
            dst[dst_idx].blue = data[src_idx * 3];
        }
    }

    free(buffer);
}

void read_subsprite(
    pixel_t * restrict dst, const pixel_t * restrict src,
    unsigned int pack_width, unsigned int pack_height,
    unsigned int pack_x, unsigned int pack_y
) {

    for (unsigned int y = 0; y < SPRITE_HEIGHT; ++y) {
        for (unsigned int x = 0; x < SPRITE_WIDTH; ++x) {
            unsigned int sx = x + pack_x * SPRITE_WIDTH;
            unsigned int sy = y + pack_y * SPRITE_HEIGHT;
            dst[x + y * SPRITE_WIDTH] = src[sx + sy * SPRITE_WIDTH * pack_height];
            dst[x + y * SPRITE_WIDTH] = src[x + pack_x * SPRITE_WIDTH + y * pack_width * SPRITE_WIDTH + pack_y * pack_width * SPRITE_WIDTH * SPRITE_HEIGHT];
        }
    }
}
