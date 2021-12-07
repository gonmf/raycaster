#include "global.h"

#define CHAR_SLASH_X 15
#define CHAR_SLASH_Y 1
#define CHAR_PERCENT_X 5
#define CHAR_PERCENT_Y 1
#define CHAR_SQUIGLY_X 14
#define CHAR_SQUIGLY_Y 6
#define CHAR_NUMBERS_X 0
#define CHAR_NUMBERS_Y 2
#define CHAR_CAP_LET_X 1
#define CHAR_CAP_LET_Y 3
#define CHAR_LOW_LET_X 1
#define CHAR_LOW_LET_Y 5
#define CHAR_HIFEN_X 13
#define CHAR_HIFEN_Y 1
#define CHAR_TWO_DOTS_X 10
#define CHAR_TWO_DOTS_Y 2

static bool get_pos(char c, unsigned int * sprite_x, unsigned int * sprite_y) {
    if (c == ' ') {
        return false;
    }
    if (c == '/') {
        *sprite_x = CHAR_SLASH_X;
        *sprite_y = CHAR_SLASH_Y;
        return true;
    }
    if (c == '%') {
        *sprite_x = CHAR_PERCENT_X;
        *sprite_y = CHAR_PERCENT_Y;
        return true;
    }
    if (c == '~') {
        *sprite_x = CHAR_SQUIGLY_X;
        *sprite_y = CHAR_SQUIGLY_Y;
        return true;
    }
    if (c == '-') {
        *sprite_x = CHAR_HIFEN_X;
        *sprite_y = CHAR_HIFEN_Y;
        return true;
    }
    if (c == ':') {
        *sprite_x = CHAR_TWO_DOTS_X;
        *sprite_y = CHAR_TWO_DOTS_Y;
        return true;
    }
    if (c >= '0' && c <= '9') {
        *sprite_x = CHAR_NUMBERS_X + (c - '0');
        *sprite_y = CHAR_NUMBERS_Y;
        return true;
    }
    if (c >= 'A' && c <= 'Z') {
        *sprite_x = CHAR_CAP_LET_X + (c - 'A');
        *sprite_y = CHAR_CAP_LET_Y;
        return true;
    }
    if (c >= 'a' && c <= 'z') {
        *sprite_x = CHAR_LOW_LET_X + (c - 'a');
        *sprite_y = CHAR_LOW_LET_Y;
        return true;
    }
    if (c < 32) {
        return false;
    }
    error("Unsupported font character");
    return false;
}

static void screen_write_char(const pixel_t * sprite, unsigned int screen_x, unsigned int screen_y, unsigned int scale, double factor) {
    for (unsigned int src_x = 0; src_x < font_sprites->sprite_size; ++src_x) {
        for (unsigned int src_y = 0; src_y < font_sprites->sprite_size; ++src_y) {
            pixel_t pixel = sprite[src_x + src_y * font_sprites->sprite_size];

            if (pixel.alpha == 0) {
                if (factor > 0.0) {
                    pixel = shading(pixel, color_red, factor);
                }

                for (unsigned int dst_y = screen_y + src_y * scale; dst_y < screen_y + src_y * scale + scale; ++dst_y) {
                    for (unsigned int dst_x = screen_x + src_x * scale; dst_x < screen_x + src_x * scale + scale; ++dst_x) {
                        fg_buffer[dst_x + dst_y * VIEWPORT_WIDTH] = pixel;
                    }
                }
            }
        }
    }
}

void screen_write_scaled(const char * str, unsigned int screen_x, unsigned int screen_y, unsigned int scale, double factor) {
    char c;
    unsigned int sprite_x = 0;
    unsigned int sprite_y = 0;
    while ((c = *str)) {
        if (get_pos(c, &sprite_x, &sprite_y)) {
            pixel_t * sprite = font_sprites->sprites[sprite_x + sprite_y * font_sprites->width];

            screen_write_char(sprite, screen_x, screen_y, scale, factor);
        }

        screen_x += font_sprites->sprite_size * scale;

        str++;
    }
}

void screen_write(const char * str, unsigned int screen_x, unsigned int screen_y) {
    screen_write_scaled(str, screen_x, screen_y, 1, 0.0);
}
