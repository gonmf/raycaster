#include "global.h"

pixel_t color_white;
pixel_t color_gray;
pixel_t color_black;
pixel_t color_red;
pixel_t color_dark_red;
pixel_t color_blue;
pixel_t color_dark_blue;
pixel_t color_cyan;
pixel_t color_gold;
pixel_t color_ui_bg;
pixel_t color_ui_bg_dark;
pixel_t color_ui_bg_light;

pixel_t shading(pixel_t min_color, pixel_t max_color, double factor) {
    pixel_t ret;

    ret.red = (unsigned char)(min_color.red * (1.0 - factor) + max_color.red * factor);
    ret.green = (unsigned char)(min_color.green * (1.0 - factor) + max_color.green * factor);
    ret.blue = (unsigned char)(min_color.blue * (1.0 - factor) + max_color.blue * factor);
    ret.alpha = 0;

    return ret;
}

void scene_shading(pixel_t color, double factor) {
    for (unsigned int i = 0; i < VIEWPORT_WIDTH * VIEWPORT_HEIGHT; ++i) {
        fg_buffer[i] = shading(fg_buffer[i], color, factor);
    }
}

pixel_t darken_shading(pixel_t color, double factor) {
    return shading(color_black, color, factor);
}

void init_base_colors() {
    color_white.red = 255;
    color_white.green = 255;
    color_white.blue = 255;

    color_gray.red = 128;
    color_gray.green = 128;
    color_gray.blue = 128;

    color_red.red = 255;

    color_dark_red.red = 139;

    color_dark_blue.blue = 99;

    color_blue.blue = 255;

    color_cyan.green = 255;
    color_cyan.blue = 255;

    color_gold.red = 255;
    color_gold.green = 215;

    color_ui_bg.green = 64;
    color_ui_bg.blue = 64;

    color_ui_bg_dark.green = 32;
    color_ui_bg_dark.blue = 32;

    color_ui_bg_light.green = 128;
    color_ui_bg_light.blue = 128;
}
