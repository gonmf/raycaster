#include "global.h"

void read_sprite_pack(
    pixel_t * dst, const char * filename,
    unsigned int pack_width, unsigned int pack_height
) {
    char * str_buf = malloc(MAX_FILE_NAME_SIZ);
    snprintf(str_buf, MAX_FILE_NAME_SIZ, "./sprites/%s.bmp", filename);
    char * buffer = malloc(MAX_FILE_SIZE);
    unsigned int read = file_read(buffer, MAX_FILE_SIZE, str_buf);
    free(str_buf);

    if (buffer[0] != 0x42 || buffer[1] != 0x4D) {
        error("Not a Windows Bitmap (BM) file");
    }

    unsigned int data_offset = (((unsigned int)buffer[13]) << 24) + (((unsigned int)buffer[12]) << 16) + (((unsigned int)buffer[11]) << 8) + buffer[10];
    if (read < data_offset) {
        error("Not a valid Windows Bitmap file with 24pp");
    }

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
