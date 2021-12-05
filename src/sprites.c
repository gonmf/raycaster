#include "global.h"

static void read_subsprite(
    pixel_t * restrict dst, const pixel_t * restrict src,
    const sprite_pack_t * pack,
    unsigned int pack_x, unsigned int pack_y
) {

    for (unsigned int y = 0; y < pack->sprite_size; ++y) {
        for (unsigned int x = 0; x < pack->sprite_size; ++x) {
            dst[x + y * pack->sprite_size] = src[x + pack_x * pack->sprite_size + y * pack->width * pack->sprite_size + pack_y * pack->width * pack->sprite_size * pack->sprite_size];
        }
    }
}

static unsigned int validate_sprite_pack_val(int val, int min, int max) {
    if (val < min || val > max) {
        error("Invalid sprite pack format");
    }

    return (unsigned int)val;
}
static void parse_sprite_pack_metadata(
    sprite_pack_t * pack, const char * pack_name,
    bool * has_transparent_color, unsigned int * transparent_color
) {
    if (strlen(pack_name) >= 16) {
        error("Invalid sprite pack name");
    }
    snprintf(pack->name, 16, "%s", pack_name);

    char * str_buf = calloc(MAX_FILE_NAME_SIZ, 1);
    snprintf(str_buf, MAX_FILE_NAME_SIZ, "./sprites/%s.txt", pack->name);
    char * file_buf = calloc(MAX_FILE_SIZE, 1);
    file_read(file_buf, MAX_FILE_SIZE, str_buf);
    free(str_buf);

    char * buffer = file_buf;
    unsigned int line = 0;
    char line_buffer[8];
    line_buffer[0] = 0;
    unsigned int line_pos = 0;
    char c;
    while ((c = *buffer)) {
        if (c == '\n') {
            if (line_pos == 3) {
                error("Invalid sprite pack format");
            }

            int val = atoi(line_buffer);
            if (line == 0) {
                pack->width = validate_sprite_pack_val(val, 1, 1024);
            } else if (line == 1) {
                pack->height = validate_sprite_pack_val(val, 1, 1024);
            } else if (line == 2) {
                pack->sprite_size = validate_sprite_pack_val(val, 8, 256);
            } else if (line == 3) {
                *has_transparent_color = (validate_sprite_pack_val(val, 0, 1) == 1);
            } else if (line == 4 && *has_transparent_color) {
                *transparent_color = validate_sprite_pack_val((int)strtol(line_buffer, NULL, 16), 0, 0xffffff);
            }

            line++;
            line_pos = 0;
            line_buffer[line_pos] = 0;
        } else if (c != '\r') {
            line_buffer[line_pos++] = c;
            if (line_pos == 8) {
                error("Invalid sprite pack format");
            }
            line_buffer[line_pos] = 0;
        }
        buffer++;
    }

    if (line != 5) {
        error("Invalid sprite pack format");
    }

    if ((pack->sprite_size % 2) == 1) {
        error("Invalid sprite pack individual sprite size (odd)");
    }

    free(file_buf);

    pack->sprites = calloc(pack->sprite_size * pack->sprite_size, sizeof(pixel_t));
}

static void read_sprite_pack_pixels(
    pixel_t * dst, const sprite_pack_t * pack,
    bool has_transparent_color, unsigned int transparent_color
) {
    char * str_buf = calloc(MAX_FILE_NAME_SIZ, 1);
    snprintf(str_buf, MAX_FILE_NAME_SIZ, "./sprites/%s.bmp", pack->name);
    char * file_buf = calloc(MAX_FILE_SIZE, 1);
    unsigned int read = file_read(file_buf, MAX_FILE_SIZE, str_buf);
    free(str_buf);

    if (file_buf[0] != 0x42 || file_buf[1] != 0x4D) {
        error("Not a Windows Bitmap (BM) file");
    }

    unsigned int data_offset = (((unsigned int)file_buf[13]) << 24) + (((unsigned int)file_buf[12]) << 16) + (((unsigned int)file_buf[11]) << 8) + file_buf[10];
    if (read < data_offset) {
        error("Not a valid Windows Bitmap file with 24pp");
    }

    pixel_t transparent_pixel;
    transparent_pixel.red = (transparent_color >> 16) & 0xff;
    transparent_pixel.green = (transparent_color >> 8) & 0xff;
    transparent_pixel.blue = transparent_color & 0xff;
    transparent_pixel.alpha = 0;

    char * data = file_buf + data_offset;
    unsigned int total_width = pack->width * pack->sprite_size;
    unsigned int total_height = pack->height * pack->sprite_size;

    for (unsigned int y = 0, yt = 0; yt < total_height; ++y, ++yt) {
        for (unsigned int x = 0, xt = 0; xt < total_width; ++x, ++xt) {
            unsigned int dst_idx = x + y * pack->width * pack->sprite_size;
            unsigned int inverted_yt = total_height - yt - 1;
            unsigned int src_idx = xt + inverted_yt * total_width;

            dst[dst_idx].red = data[src_idx * 3 + 2];
            dst[dst_idx].green = data[src_idx * 3 + 1];
            dst[dst_idx].blue = data[src_idx * 3];

            if (has_transparent_color && EQUAL_PIXEL(dst[dst_idx], transparent_pixel)) {
                dst[dst_idx].alpha = 255;
            }
        }
    }

    free(file_buf);
}

#if DUMP_SUB_SPRITES
static void dump_sprite(
    const pixel_t * image, const char * pack_name, unsigned int sprite_size, unsigned int x, unsigned int y
) {
    char filename[100];
    sprintf(filename, "sprites/dump/dump_sprite_%s_%ux%u.ppm", pack_name, x, y);

    FILE * file = fopen(filename, "w");

    fprintf(file, "P3\n");
    fprintf(file, "%u %u\n", sprite_size, sprite_size);
    fprintf(file, "255\n");

    for(unsigned int y = 0; y < sprite_size; ++y) {
        for(unsigned int x = 0; x < sprite_size; ++x) {
            pixel_t pixel = image[x + y * sprite_size];

            fprintf(file, "%u %u %u ", pixel.red, pixel.green, pixel.blue);
        }

        fprintf(file, "\n");
    }

    fclose(file);
}
#endif

void read_sprite_pack(sprite_pack_t * pack, const char * pack_name) {
#if DUMP_SUB_SPRITES
    struct stat st;
    if (stat("sprites/dump", &st) == -1) {
        mkdir("sprites/dump", 0700);
    }
#endif

    bool has_transparent_color = false;
    unsigned int transparent_color = 0;

    parse_sprite_pack_metadata(pack, pack_name, &has_transparent_color, &transparent_color);

    unsigned int total_width = pack->width * pack->sprite_size;
    unsigned int total_height = pack->height * pack->sprite_size;
    pixel_t * pixel_buf = calloc(total_width * total_height, sizeof(pixel_t));

    read_sprite_pack_pixels(pixel_buf, pack, has_transparent_color, transparent_color);

    for (unsigned int pack_y = 0; pack_y < pack->height; ++pack_y) {
        for (unsigned int pack_x = 0; pack_x < pack->width; ++pack_x) {
            pixel_t * dst = calloc(pack->sprite_size * pack->sprite_size, sizeof(pixel_t));
            pack->sprites[pack_x + pack_y * pack->width] = dst;

            read_subsprite(dst, pixel_buf, pack, pack_x, pack_y);

#if DUMP_SUB_SPRITES
            dump_sprite(dst, pack_name, pack->sprite_size, pack_x, pack_y);
#endif
        }
    }

    free(pixel_buf);
}
