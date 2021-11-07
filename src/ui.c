#include "global.h"

static void paint_part(pixel_t color, unsigned int x, unsigned int y, unsigned int offset_x, unsigned int offset_y) {
    for (unsigned int by = 0; by < MAP_BLOCK_SIZE; ++by) {
        unsigned int y2 = y * MAP_BLOCK_SIZE + by + offset_y;

        if (y2 >= VIEWPORT_HEIGHT) {
            return;
        }

        for (unsigned int bx = 0; bx < MAP_BLOCK_SIZE; ++bx) {
            unsigned int x2 = x * MAP_BLOCK_SIZE + bx + offset_x;

            if (x2 >= VIEWPORT_WIDTH) {
                return;
            }

            fg_buffer[x2 + y2 * VIEWPORT_WIDTH] = color;
        }
    }
}

void paint_map(const level_t * level) {
    for (unsigned int i = 0; i < VIEWPORT_WIDTH * VIEWPORT_HEIGHT; ++i) {
        fg_buffer[i] = shading(fg_buffer[i], color_blue, 0.75);
    }

    unsigned int viewport_center_x = VIEWPORT_WIDTH / 2;
    unsigned int viewport_center_y = VIEWPORT_HEIGHT / 2;
    unsigned int observer_x = (unsigned int)(level->observer_x + 0.5);
    unsigned int observer_y = (unsigned int)(level->observer_y + 0.5);
    unsigned int offset_x = viewport_center_x - observer_x * MAP_BLOCK_SIZE;
    unsigned int offset_y = viewport_center_y - (level->height - observer_y - 1) * MAP_BLOCK_SIZE;

    for (unsigned int y = 0; y < level->height; ++y) {
        unsigned int iy = level->height - y - 1;
        for (unsigned int x = 0; x < level->width; ++x) {
            if (x == observer_x && iy == observer_y) {
                paint_part(color_red, x, y, offset_x, offset_y);
            } else {
                bool revealed = level->map_revealed[x + iy * level->width];
                unsigned char content_type = level->content_type[x + iy * level->width];

                if (!revealed) {
                    continue;
                }

                if (content_type == CONTENT_TYPE_WALL) {
                    paint_part(color_gray, x, y, offset_x, offset_y);
                } else if (content_type == CONTENT_TYPE_DOOR || content_type == CONTENT_TYPE_DOOR_OPEN) {
                    paint_part(color_cyan, x, y, offset_x, offset_y);
                } else if (content_type == CONTENT_TYPE_OBJECT) {
                    if (level->special_effects[x + iy * level->width] == SPECIAL_EFFECT_NONE) {
                        paint_part(color_gray, x, y, offset_x, offset_y);
                    } else {
                        paint_part(color_gold, x, y, offset_x, offset_y);
                    }
                }
            }
        }
    }
}
