#include "global.h"

static double fish_eye_table[VIEWPORT_WIDTH];

static pixel_t bg_buffer[VIEWPORT_WIDTH * VIEWPORT_HEIGHT];
static pixel_t fg_buffer[VIEWPORT_WIDTH * VIEWPORT_HEIGHT];
static pixel_t wall_textures[TEXTURE_PACK_WIDTH][TEXTURE_PACK_HEIGHT][SPRITE_WIDTH * SPRITE_HEIGHT];

const pixel_t * foreground_buffer() {
    return fg_buffer;
}

void color_filter(double factor) {
    for (unsigned int i = 0; i < VIEWPORT_WIDTH * VIEWPORT_HEIGHT; ++i) {
        fg_buffer[i].red = (unsigned int)(MIN(MAX(fg_buffer[i].red * factor, 0.0), 255.0));
        fg_buffer[i].green = (unsigned int)(MIN(MAX(fg_buffer[i].green * factor, 0.0), 255.0));
        fg_buffer[i].blue = (unsigned int)(MIN(MAX(fg_buffer[i].blue * factor, 0.0), 255.0));
    }
}

static double horizon_distance(unsigned int x, unsigned int y) {
    if (x < VIEWPORT_WIDTH / 2) {
        x = VIEWPORT_WIDTH / 2 - x;
    } else {
        x = x - VIEWPORT_WIDTH / 2;
    }

    double w2 = (double)(VIEWPORT_WIDTH / 2);
    double h2 = (double)(VIEWPORT_HEIGHT / 2);
    double max = sqrt(w2 * w2 + h2 * h2);
    double dst_to_max = 1.0 - sqrt(((double)x) * x + ((double)y) * y) / max;
    return (dst_to_max + 2.0) / 3.0;
}

void init_fish_eye_table() {
    double half_fov = FIELD_OF_VIEW / 2.0;

    unsigned int center_x = VIEWPORT_WIDTH / 2;

    double plane_min_x = tan((-(90.0 - half_fov)) / RADIAN_CONSTANT) * ((double)center_x) * -1.0;
    double plane_max_x = tan(((90.0 - half_fov)) / RADIAN_CONSTANT) * ((double)center_x);

    for (unsigned int x = 0; x < center_x; ++x) {
        double arg = plane_min_x / ((double)(center_x - x));
        double angle = atan(arg) * RADIAN_CONSTANT - 90.0;
        fish_eye_table[x] = angle;
    }
    for (unsigned int x = center_x; x < VIEWPORT_WIDTH; ++x) {
        double arg = plane_max_x / ((double)(x - center_x));
        double angle = 90.0 - atan(arg) * RADIAN_CONSTANT;
        fish_eye_table[x] = angle;
    }
}

static void fill_in_background(const level_t * level) {
    for (unsigned int y = 0; y < VIEWPORT_HEIGHT; ++y) {
        for (unsigned int x = 0; x < VIEWPORT_WIDTH; ++x) {
            double r, g, b, factor;

            if (y < VIEWPORT_HEIGHT / 2) { // sky
                factor = horizon_distance(x, y);

                r = level->ceil_color.red * factor;
                g = level->ceil_color.green * factor;
                b = level->ceil_color.blue * factor;
            } else { // floor
                factor = horizon_distance(x, VIEWPORT_HEIGHT - y - 1);
                r = level->floor_color.red * factor;
                g = level->floor_color.green * factor;
                b = level->floor_color.blue * factor;
            }

            pixel_t * pixel = &(bg_buffer[x + y * VIEWPORT_WIDTH]);
            pixel->red = (unsigned char)r;
            pixel->green = (unsigned char)g;
            pixel->blue = (unsigned char)b;
        }
    }
}

static double cast_ray(const level_t * level, double angle, double * hit_angle, unsigned int * block_hit_x, unsigned int * block_hit_y) {
    double angle_radians = angle / RADIAN_CONSTANT;
    double x_change = sin(angle_radians) * RAY_STEP_CONSTANT;
    double y_change = cos(angle_radians) * RAY_STEP_CONSTANT;
    double curr_x = level->observer_x;
    double curr_y = level->observer_y;

    unsigned int steps = 0;
    while(1) {
        steps += 1;
        curr_x += x_change;
        curr_y += y_change;

        unsigned int rounded_x = (unsigned int)(curr_x + 0.5);
        unsigned int rounded_y = (unsigned int)(curr_y + 0.5);

        if (level->contents[rounded_x + rounded_y * level->width]) {
            *block_hit_x = rounded_x;
            *block_hit_y = rounded_y;

            curr_x -= x_change / 2.0;
            curr_y -= y_change / 2.0;
            double hangle;

            double vertice_x[4];
            double vertice_y[4];
            vertice_x[0] = ((double)rounded_x) + 0.5;
            vertice_y[0] = ((double)rounded_y) + 0.5;
            vertice_x[1] = ((double)rounded_x) + 0.5;
            vertice_y[1] = ((double)rounded_y) - 0.5;
            vertice_x[2] = ((double)rounded_x) - 0.5;
            vertice_y[2] = ((double)rounded_y) - 0.5;
            vertice_x[3] = ((double)rounded_x) - 0.5;
            vertice_y[3] = ((double)rounded_y) + 0.5;
            int closest_vertice = -1;
            double min_distance = 0.0;
            for (int i = 0; i < 4; ++i) {
                double distance = sqrt((vertice_x[i] - level->observer_x) * (vertice_x[i] - level->observer_x) + (vertice_y[i] - level->observer_y) * (vertice_y[i] - level->observer_y));
                if ((closest_vertice == -1) || (min_distance > distance)) {
                    closest_vertice = i;
                    min_distance = distance;
                }
            }

            int left_vertice = (closest_vertice + 4 - 1) % 4;
            int right_vertice = (closest_vertice + 1) % 4;

            hangle = sqrt((vertice_x[closest_vertice] - curr_x) * (vertice_x[closest_vertice] - curr_x) + (vertice_y[closest_vertice] - curr_y) * (vertice_y[closest_vertice] - curr_y));

            double distance_left = sqrt((vertice_x[left_vertice] - curr_x) * (vertice_x[left_vertice] - curr_x) + (vertice_y[left_vertice] - curr_y) * (vertice_y[left_vertice] - curr_y));
            double distance_right = sqrt((vertice_x[right_vertice] - curr_x) * (vertice_x[right_vertice] - curr_x) + (vertice_y[right_vertice] - curr_y) * (vertice_y[right_vertice] - curr_y));

            if (distance_left > distance_right) {
                hangle = 1.0 - hangle;
            }

            *hit_angle = MIN(MAX(hangle, 0.0), 1.0);
            return steps * RAY_STEP_CONSTANT;
        }
    }
}

static void block_color(pixel_t * dst, double block_x, double block_y, double factor, unsigned char block_type) {
    factor = (2.0 + factor) / 3.0;

    unsigned int x = (unsigned int)(SPRITE_WIDTH * block_x);
    unsigned int y = (unsigned int)(SPRITE_HEIGHT * block_y);

    unsigned char texture_x = block_type % TEXTURE_PACK_WIDTH;
    unsigned char texture_y = block_type / TEXTURE_PACK_WIDTH;
    pixel_t * src = &wall_textures[texture_x][texture_y][x + y * SPRITE_WIDTH];
    dst->red = (unsigned int)MIN(MAX(0, src->red * factor), src->red);
    dst->green = (unsigned int)MIN(MAX(0, src->green * factor), src->green);
    dst->blue = (unsigned int)MIN(MAX(0, src->blue * factor), src->blue);
}

void load_textures() {
    pixel_t * sprite_pack = calloc(SPRITE_WIDTH * TEXTURE_PACK_WIDTH * SPRITE_HEIGHT * TEXTURE_PACK_HEIGHT, sizeof(pixel_t));
    read_sprite_pack(sprite_pack, TEXTURE_PACK_NAME, TEXTURE_PACK_WIDTH, TEXTURE_PACK_HEIGHT);

    for (unsigned int y = 0; y < TEXTURE_PACK_HEIGHT; ++y) {
        for (unsigned int x = 0; x < TEXTURE_PACK_WIDTH; ++x) {
            read_subsprite(wall_textures[x][y], sprite_pack, TEXTURE_PACK_WIDTH, TEXTURE_PACK_HEIGHT, x, y);
        }
    }

    free(sprite_pack);
}

static void copy_bg_to_fg() {
    memcpy(fg_buffer, bg_buffer, sizeof(pixel_t) * VIEWPORT_WIDTH * VIEWPORT_HEIGHT);
}

static void fill_in_walls(const level_t * level) {
    for (unsigned int x = 0 ; x < VIEWPORT_WIDTH; ++x) {
        unsigned int block_hit_x;
        unsigned int block_hit_y;
        double block_x;

        double x_angle = fit_angle(fish_eye_table[x] + level->observer_angle);
        double distance = cast_ray(level, x_angle, &block_x, &block_hit_x, &block_hit_y);
        distance *= cos((x_angle - level->observer_angle) / RADIAN_CONSTANT);
        distance = MAX(distance, 0.0);
        double block_size = (1.8 / distance);

        unsigned char block_type = level->contents[block_hit_x + block_hit_y * level->width] - 1;

        double block_y;
        pixel_t * pixel;
        unsigned int y;
        unsigned int viewport_mid = VIEWPORT_HEIGHT / 2;
        unsigned int end_y = (unsigned int)(block_size * viewport_mid);
        unsigned int orig_end_y = end_y;
        if (end_y > viewport_mid) {
            end_y = viewport_mid;
        }
        for (unsigned int mid_y = 0; mid_y < end_y; ++mid_y) {
            // top part
            y = viewport_mid - mid_y;
            pixel = &(fg_buffer[x + y * VIEWPORT_WIDTH]);
            block_y = ((orig_end_y - mid_y) * 1) / (VIEWPORT_HEIGHT * block_size);
            block_color(pixel, block_x, block_y, block_size, block_type);

            // bottom part
            y = viewport_mid + mid_y;
            pixel = &(fg_buffer[x + y * VIEWPORT_WIDTH]);
            block_y = (mid_y + VIEWPORT_HEIGHT * block_size * 0.5) / (VIEWPORT_HEIGHT * block_size);
            block_color(pixel, block_x, block_y, block_size, block_type);
        }
    }
}

void paint_scene_first_time(const level_t * level) {
    fill_in_background(level);

    fill_in_walls(level);
}

void paint_scene(const level_t * level) {
    copy_bg_to_fg();

    fill_in_walls(level);
}
