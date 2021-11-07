#include "global.h"

sprite_pack_t * wall_textures;
sprite_pack_t * objects_sprites;
pixel_t fg_buffer[VIEWPORT_WIDTH * VIEWPORT_HEIGHT];

static double fish_eye_table[VIEWPORT_WIDTH];

static unsigned int horizon_offset(const level_t * level) {
    double vertical_multiplier = level->observer_angle2 / 90.0;
    return (unsigned int)((VIEWPORT_HEIGHT / 2) * vertical_multiplier);
}

static pixel_t shading(pixel_t min_color, pixel_t max_color, double factor) {
    pixel_t ret;

    ret.red = (unsigned char)(min_color.red * (1.0 - factor) + max_color.red * factor);
    ret.green = (unsigned char)(min_color.green * (1.0 - factor) + max_color.green * factor);
    ret.blue = (unsigned char)(min_color.blue * (1.0 - factor) + max_color.blue * factor);

    return ret;
}

static pixel_t brighten_shading(pixel_t color, double factor) {
    pixel_t white;
    white.red = 255;
    white.green = 255;
    white.blue = 255;
    white.alpha = 0;

    return shading(color, white, factor);
}

static pixel_t darken_shading(pixel_t color, double factor) {
    pixel_t black;
    black.red = 0;
    black.green = 0;
    black.blue = 0;
    black.alpha = 0;

    return shading(black, color, factor);
}

void brighten_scene(double factor) {
    for (unsigned int i = 0; i < VIEWPORT_WIDTH * VIEWPORT_HEIGHT; ++i) {
        fg_buffer[i] = brighten_shading(fg_buffer[i], factor);
    }
}

void darken_scene(double factor) {
    for (unsigned int i = 0; i < VIEWPORT_WIDTH * VIEWPORT_HEIGHT; ++i) {
        fg_buffer[i] = darken_shading(fg_buffer[i], factor);
    }
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
    unsigned int viewport_mid = horizon_offset(level);

    for (unsigned int y = 0; y < VIEWPORT_HEIGHT; ++y) {
        for (unsigned int x = 0; x < VIEWPORT_WIDTH; ++x) {
            pixel_t * pixel = &(fg_buffer[x + y * VIEWPORT_WIDTH]);

            if (y < viewport_mid) { // sky
                *pixel = level->ceil_color;
            } else { // floor
                *pixel = level->floor_color;
            }
        }
    }
}

static double cast_simple_ray(const level_t * level, double angle, double * block_x, unsigned int * block_hit_x, unsigned int * block_hit_y, unsigned int * step_count) {
    double angle_radians = angle / RADIAN_CONSTANT;
    double x_change = sin(angle_radians) * ROUGH_RAY_STEP_CONSTANT;
    double y_change = cos(angle_radians) * ROUGH_RAY_STEP_CONSTANT;
    unsigned int initial_x = (unsigned int)(level->observer_x + 0.5);
    unsigned int initial_y = (unsigned int)(level->observer_y + 0.5);
    double curr_x = level->observer_x;
    double curr_y = level->observer_y;

    unsigned int opening_door_x, opening_door_y;
    double percentage_open;
    bool opening_door = opening_door_transition(&percentage_open, &opening_door_x, &opening_door_y);
    bool skipped_door = false;

    unsigned int steps = 0;
    unsigned int minimum_steps = *step_count;

    steps += minimum_steps;
    curr_x += minimum_steps * x_change;
    curr_y += minimum_steps * y_change;

    while(1) {
        steps += 1;
        curr_x += x_change;
        curr_y += y_change;

        unsigned int rounded_x = (unsigned int)(curr_x + 0.5);
        unsigned int rounded_y = (unsigned int)(curr_y + 0.5);

        if (rounded_x == initial_x && rounded_y == initial_y) {
            continue;
        }
        if (rounded_x == *block_hit_x && rounded_y == *block_hit_y) {
            continue;
        }

        bool opening_this_door = opening_door && opening_door_x == rounded_x && opening_door_y == rounded_y;

        if (opening_this_door && skipped_door) {
            continue;
        }

        unsigned char content_type = level->content_type[rounded_x + rounded_y * level->width];

        if (content_type == CONTENT_TYPE_WALL || content_type == CONTENT_TYPE_DOOR) {
            return -1;
        } else if (content_type == CONTENT_TYPE_DOOR_OPEN) {
            if (opening_this_door) {
                curr_x -= x_change / 2.0;
                curr_y -= y_change / 2.0;
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

                double hangle = sqrt((vertice_x[closest_vertice] - curr_x) * (vertice_x[closest_vertice] - curr_x) + (vertice_y[closest_vertice] - curr_y) * (vertice_y[closest_vertice] - curr_y));

                double distance_left = sqrt((vertice_x[left_vertice] - curr_x) * (vertice_x[left_vertice] - curr_x) + (vertice_y[left_vertice] - curr_y) * (vertice_y[left_vertice] - curr_y));
                double distance_right = sqrt((vertice_x[right_vertice] - curr_x) * (vertice_x[right_vertice] - curr_x) + (vertice_y[right_vertice] - curr_y) * (vertice_y[right_vertice] - curr_y));

                if (distance_left > distance_right) {
                    hangle = 1.0 - hangle;
                }

                hangle = MIN(MAX(hangle, 0.0), 1.0);
                if (hangle < percentage_open) {
                    skipped_door = true;
                    curr_x += x_change / 2.0;
                    curr_y += y_change / 2.0;
                    continue;
                } else {
                    return -1;
                }
            } else {
                continue;
            }
        } else if (content_type == CONTENT_TYPE_EMPTY) {
            continue;
        }

        *block_hit_x = rounded_x;
        *block_hit_y = rounded_y;

        double distance = sqrt((level->observer_x - rounded_x) * (level->observer_x - rounded_x) + (level->observer_y - rounded_y) * (level->observer_y - rounded_y));


        double angle_to_center = atan((level->observer_x - rounded_x) / (level->observer_y - rounded_y)) * RADIAN_CONSTANT;
        double angle_diff = angle_to_center - angle;
        double opposite = tan(angle_diff / RADIAN_CONSTANT) * distance;
        if (fabs(opposite) > 0.5) {
            continue;
        }
        *block_x = MIN(MAX(opposite / distance, 0.0), 1.0);
        *block_x = MIN(MAX((opposite + 0.5) / 1.0, 0.0), 1.0);
        *step_count = steps + 1;
        return distance;
    }
}

static double cast_ray(const level_t * level, double angle, double * hit_angle, unsigned int * block_hit_x, unsigned int * block_hit_y, unsigned char * last_seen_block_type) {
    double angle_radians = angle / RADIAN_CONSTANT;
    double x_change = sin(angle_radians) * FINE_RAY_STEP_CONSTANT;
    double y_change = cos(angle_radians) * FINE_RAY_STEP_CONSTANT;
    unsigned int initial_x = (unsigned int)(level->observer_x + 0.5);
    unsigned int initial_y = (unsigned int)(level->observer_y + 0.5);
    double curr_x = level->observer_x;
    double curr_y = level->observer_y;

    unsigned int opening_door_x, opening_door_y;
    double percentage_open;
    bool opening_door = opening_door_transition(&percentage_open, &opening_door_x, &opening_door_y);
    bool skipped_door = false;

    unsigned int rounded_x = (unsigned int)(curr_x + 0.5);
    unsigned int rounded_y = (unsigned int)(curr_y + 0.5);
    unsigned char last_seen_block_content_type = level->content_type[rounded_x + rounded_y * level->width];

    unsigned int steps = 0;
    while(1) {
        steps += 1;
        curr_x += x_change;
        curr_y += y_change;

        unsigned int rounded_x = (unsigned int)(curr_x + 0.5);
        unsigned int rounded_y = (unsigned int)(curr_y + 0.5);

        if (rounded_x == initial_x && rounded_y == initial_y) {
            continue;
        }
        if (rounded_x == *block_hit_x && rounded_y == *block_hit_y) {
            continue;
        }

        bool opening_this_door = opening_door && opening_door_x == rounded_x && opening_door_y == rounded_y;

        if (opening_this_door && skipped_door) {
            continue;
        }

        unsigned char content_type = level->content_type[rounded_x + rounded_y * level->width];

        if (content_type == CONTENT_TYPE_WALL || content_type == CONTENT_TYPE_DOOR || opening_this_door) {
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

            hangle = MIN(MAX(hangle, 0.0), 1.0);
            if (opening_this_door) {
                if (hangle < percentage_open) {
                    skipped_door = true;
                    curr_x += x_change / 2.0;
                    curr_y += y_change / 2.0;

                    if (last_seen_block_content_type != content_type) {
                        last_seen_block_content_type = content_type;
                    }
                    continue;
                } else {
                    hangle = MAX(hangle - percentage_open, 0.0);
                }
            }

            *hit_angle = hangle;
            *last_seen_block_type = last_seen_block_content_type;
            return steps * FINE_RAY_STEP_CONSTANT;
        }

        if (last_seen_block_content_type != content_type) {
            last_seen_block_content_type = content_type;
        }
    }
}

static void object_color(pixel_t * dst, double sprite_x, double sprite_y, double intensity, unsigned char sprite_type) {
    unsigned int x = (unsigned int)(SPRITE_WIDTH * sprite_x);
    unsigned int y = (unsigned int)(SPRITE_HEIGHT * sprite_y);

    pixel_t src = objects_sprites->sprites[sprite_type][x + y * SPRITE_WIDTH];
    // alpha == 1 is used as indicator another object's pixel was already painted
    if (src.alpha == 0 && dst->alpha == 0) {
        *dst = darken_shading(src, intensity);
        dst->alpha = 1;
    }
}

static void block_color(pixel_t * dst, double block_x, double block_y, double intensity, unsigned char block_type) {
    unsigned int x = (unsigned int)(SPRITE_WIDTH * block_x);
    unsigned int y = (unsigned int)(SPRITE_HEIGHT * block_y);

    pixel_t src = wall_textures->sprites[block_type][x + y * SPRITE_WIDTH];
    *dst = darken_shading(src, intensity);
}

static void color_to_alpha(sprite_pack_t * pack) {
    for (unsigned int pack_i = 0; pack_i < pack->width * pack->height; ++pack_i) {
        for (unsigned int sprite_i = 0; sprite_i < SPRITE_WIDTH * SPRITE_HEIGHT; ++sprite_i) {

            if (pack->sprites[pack_i][sprite_i].red == 0x98 &&
                pack->sprites[pack_i][sprite_i].green == 0 &&
                pack->sprites[pack_i][sprite_i].blue == 0x88) {
                pack->sprites[pack_i][sprite_i].alpha = 255;
            }
        }
    }
}

void load_textures() {
    wall_textures = calloc(1, sizeof(sprite_pack_t));
    read_sprite_pack(wall_textures, TEXTURE_PACK_NAME);

    objects_sprites = calloc(1, sizeof(sprite_pack_t));
    read_sprite_pack(objects_sprites, OBJECTS_PACK_NAME);
    color_to_alpha(objects_sprites);
}

static void fill_in_objects(const level_t * level) {
    unsigned int block_hit_x;
    unsigned int block_hit_y = INT_MAX;
    unsigned int viewport_mid = horizon_offset(level);
    double block_x;

    for (unsigned int x = 0; x < VIEWPORT_WIDTH; ++x) {
        double x_angle = fit_angle(fish_eye_table[x] + level->observer_angle);
        block_hit_x = INT_MAX;

        unsigned int min_step_count = 0;
        while(1) {
            double distance = cast_simple_ray(level, x_angle, &block_x, &block_hit_x, &block_hit_y, &min_step_count);
            if (distance <= 0.0) {
                break;
            }

            distance *= cos((x_angle - level->observer_angle) / RADIAN_CONSTANT);
            distance = MAX(distance, 0.0);

            double block_size = (1.5 / distance);

            unsigned char sprite_id = level->texture[block_hit_x + block_hit_y * level->width];

            unsigned int end_y = (unsigned int)(block_size * VIEWPORT_HEIGHT / 2.0);
            double block_y;
            pixel_t * pixel;
            unsigned int y;

            double intensity = MIN(MAX(10.0 / distance, 0.0), 1.0);;

            for (unsigned int mid_y = 0; mid_y < end_y; ++mid_y) {
                // top part
                if (viewport_mid >= mid_y) {
                    y = viewport_mid - mid_y;
                    pixel = &(fg_buffer[x + y * VIEWPORT_WIDTH]);
                    block_y = (end_y - mid_y) / (VIEWPORT_HEIGHT * block_size);
                    object_color(pixel, block_x, block_y, intensity, sprite_id);
                }

                // bottom part
                if (viewport_mid + mid_y < VIEWPORT_HEIGHT) {
                    y = viewport_mid + mid_y;
                    pixel = &(fg_buffer[x + y * VIEWPORT_WIDTH]);
                    block_y = (mid_y + VIEWPORT_HEIGHT * block_size * 0.5) / (VIEWPORT_HEIGHT * block_size);
                    object_color(pixel, block_x, block_y, intensity, sprite_id);
                }
            }
        }
    }
}

static void fill_in_walls(const level_t * level) {
    unsigned int block_hit_x;
    unsigned int block_hit_y = INT_MAX;
    double block_x;
    unsigned char last_seen_block_type;
    unsigned char sprite_id;
    unsigned int viewport_mid = horizon_offset(level);

    for (unsigned int x = 0; x < VIEWPORT_WIDTH; ++x) {
        block_hit_x = INT_MAX;
        double x_angle = fit_angle(fish_eye_table[x] + level->observer_angle);
        double distance = cast_ray(level, x_angle, &block_x, &block_hit_x, &block_hit_y, &last_seen_block_type);
        distance *= cos((x_angle - level->observer_angle) / RADIAN_CONSTANT);
        distance = MAX(distance, 0.0);
        double block_size = (1.5 / distance);

        if (last_seen_block_type == CONTENT_TYPE_DOOR_OPEN && level->content_type[block_hit_x + block_hit_y * level->width] == CONTENT_TYPE_WALL) {
            sprite_id = level->door_open_texture;
        } else {
            sprite_id = level->texture[block_hit_x + block_hit_y * level->width];
        }

        double block_y;
        pixel_t * pixel;
        unsigned int y;

        unsigned int end_y = (unsigned int)(block_size * VIEWPORT_HEIGHT / 2.0);
        double intensity = MIN(MAX(10.0 / distance, 0.0), 1.0);

        for (unsigned int mid_y = 0; mid_y < end_y; ++mid_y) {
            // top part
            if (viewport_mid >= mid_y) {
                y = viewport_mid - mid_y;
                pixel = &(fg_buffer[x + y * VIEWPORT_WIDTH]);
                block_y = (end_y - mid_y) / (VIEWPORT_HEIGHT * block_size);
                block_color(pixel, block_x, block_y, intensity, sprite_id);
            }

            // bottom part
            if (viewport_mid + mid_y < VIEWPORT_HEIGHT) {
                y = viewport_mid + mid_y;
                pixel = &(fg_buffer[x + y * VIEWPORT_WIDTH]);
                block_y = (mid_y + VIEWPORT_HEIGHT * block_size * 0.5) / (VIEWPORT_HEIGHT * block_size);
                block_color(pixel, block_x, block_y, intensity, sprite_id);
            }
        }
    }
}

void paint_scene(const level_t * level) {
    fill_in_background(level);

    fill_in_walls(level);

    fill_in_objects(level);
}
