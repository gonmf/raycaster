#include "global.h"

sprite_pack_t * wall_textures;
sprite_pack_t * objects_sprites;
sprite_pack_t * enemy_sprites[5];
sprite_pack_t * weapons_sprites;
pixel_t fg_buffer[VIEWPORT_WIDTH * VIEWPORT_HEIGHT];

static char * enemy_angles;
static bool trigger_shot;
static double fish_eye_table[VIEWPORT_WIDTH];

static unsigned int horizon_offset(const level_t * level) {
    double vertical_multiplier = level->observer_angle2 / 90.0;
    return (unsigned int)((VIEWPORT_HEIGHT / 2) * vertical_multiplier);
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

static double cast_simple_ray(const level_t * level, double angle, double * block_x, unsigned int * block_hit_x, unsigned int * block_hit_y, unsigned int * enemy, unsigned int * step_count) {
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
                    double dist = distance(vertice_x[i], vertice_y[i], level->observer_x, level->observer_y);
                    if ((closest_vertice == -1) || (min_distance > dist)) {
                        closest_vertice = i;
                        min_distance = dist;
                    }
                }

                int left_vertice = (closest_vertice + 4 - 1) % 4;
                int right_vertice = (closest_vertice + 1) % 4;

                double hangle = distance(vertice_x[closest_vertice], vertice_y[closest_vertice], curr_x, curr_y);
                double distance_left = distance(vertice_x[left_vertice], vertice_y[left_vertice], curr_x, curr_y);
                double distance_right = distance(vertice_x[right_vertice], vertice_y[right_vertice], curr_x, curr_y);

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
            for (unsigned int i = 0; i < level->enemies_count; ++i) {
                if (rounded_x == (unsigned int)(level->enemy[i].x + 0.5) && rounded_y == (unsigned int)(level->enemy[i].y + 0.5)) {
                    if (*enemy == i) {
                        continue;
                    }

                    double enemy_x = level->enemy[i].x;
                    double enemy_y = level->enemy[i].y;
                    double dist = distance(level->observer_x, level->observer_y, enemy_x, enemy_y);
                    double angle_to_center = atan((level->observer_x - enemy_x) / (level->observer_y - enemy_y)) * RADIAN_CONSTANT;
                    double angle_diff = angle_to_center - angle;
                    double opposite = tan(angle_diff / RADIAN_CONSTANT) * dist;
                    if (fabs(opposite) > 0.5) {
                        continue;
                    }
                    *block_hit_x = INT_MAX;
                    *enemy = i;
                    *block_x = 1.0 - MIN(MAX((opposite + 0.5) / 1.0, 0.0), 1.0);
                    *step_count = steps + 1;
                    return dist;
                }
            }
            continue;
        }

        *block_hit_x = rounded_x;
        *block_hit_y = rounded_y;
        *enemy = INT_MAX;

        double dist = distance(level->observer_x, level->observer_y, rounded_x, rounded_y);
        double angle_to_center = atan((level->observer_x - rounded_x) / (level->observer_y - rounded_y)) * RADIAN_CONSTANT;
        double angle_diff = angle_to_center - angle;
        double opposite = tan(angle_diff / RADIAN_CONSTANT) * dist;
        if (fabs(opposite) > 0.5) {
            continue;
        }
        *block_x = 1.0 - MIN(MAX((opposite + 0.5) / 1.0, 0.0), 1.0);
        *step_count = steps + 1;
        return dist;
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
                double dist = distance(vertice_x[i], vertice_y[i], level->observer_x, level->observer_y);
                if ((closest_vertice == -1) || (min_distance > dist)) {
                    closest_vertice = i;
                    min_distance = dist;
                }
            }

            int left_vertice = (closest_vertice + 4 - 1) % 4;
            int right_vertice = (closest_vertice + 1) % 4;

            double hangle = distance(vertice_x[closest_vertice], vertice_y[closest_vertice], curr_x, curr_y);

            double distance_left = distance(vertice_x[left_vertice], vertice_y[left_vertice], curr_x, curr_y);
            double distance_right = distance(vertice_x[right_vertice], vertice_y[right_vertice], curr_x, curr_y);

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

static void enemy_color(pixel_t * dst, double sprite_x, double sprite_y, double intensity, unsigned char sprite_type, unsigned char enemy_type) {
    unsigned int x = (unsigned int)(SPRITE_WIDTH * sprite_x);
    unsigned int y = (unsigned int)(SPRITE_HEIGHT * sprite_y);

    pixel_t src = enemy_sprites[enemy_type]->sprites[sprite_type][x + y * SPRITE_WIDTH];
    // alpha == 1 is used as indicator another object's pixel was already painted
    if (src.alpha == 0 && dst->alpha == 0) {
        *dst = darken_shading(src, intensity);
        dst->alpha = 1;
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

static void fill_in_objects(const level_t * level) {
    unsigned int block_hit_x;
    unsigned int block_hit_y = INT_MAX;
    unsigned int enemy = INT_MAX;
    unsigned int viewport_mid = horizon_offset(level);
    unsigned char enemy_type;
    double block_x;

    // We remember enemy angles to make sure in the same frame always the same angle/sprite
    // is used.
    memset(enemy_angles, -1, level->enemies_count);

    for (unsigned int x = 0; x < VIEWPORT_WIDTH; ++x) {
        double x_angle = fit_angle(fish_eye_table[x] + level->observer_angle);
        block_hit_x = INT_MAX;
        enemy = INT_MAX;

        unsigned int min_step_count = 0;
        while(1) {
            double distance = cast_simple_ray(level, x_angle, &block_x, &block_hit_x, &block_hit_y, &enemy, &min_step_count);
            if (distance <= 0.0) {
                break;
            }

            bool render_enemy = block_hit_x == INT_MAX;
            distance *= cos((x_angle - level->observer_angle) / RADIAN_CONSTANT);
            distance = MAX(distance, 0.0);

            double block_size = (1.5 / distance);

            unsigned char sprite_id;

            if (render_enemy) {
                int angles_sum;
                unsigned int animation_step;

                switch (level->enemy[enemy].state) {
                    case ENEMY_STATE_STILL:
                        if (enemy_angles[enemy] == -1) {
                            angles_sum = (unsigned int)(x_angle + level->enemy[enemy].angle + 180 + 23);
                            enemy_angles[enemy] = (8 - (((unsigned int)angles_sum) / 45)) % 8;
                        }

                        sprite_id = enemy_angles[enemy];
                        break;
                    case ENEMY_STATE_MOVING:
                        // TODO:
                        if (enemy_angles[enemy] == -1) {
                            angles_sum = (unsigned int)(x_angle + level->enemy[enemy].angle + 180 + 23);
                            enemy_angles[enemy] = (8 - (((unsigned int)angles_sum) / 45)) % 8;
                        }

                        sprite_id = enemy_angles[enemy] + level->enemy[enemy].state_step * 8;
                        break;
                    case ENEMY_STATE_SHOT:
                        sprite_id = 7 + 5 * 8;
                        break;
                    case ENEMY_STATE_SHOOTING:
                        // TODO:
                        sprite_id = level->enemy[enemy].state_step + 5 * 8;
                        break;
                    case ENEMY_STATE_DYING:
                        animation_step = ((ENEMY_DYING_ANIMATION_SPEED - level->enemy[enemy].state_step) * ENEMY_DYING_ANIMATION_PARTS) / ENEMY_DYING_ANIMATION_SPEED;
                        sprite_id = animation_step + 5 * 8;
                        break;
                    default: // dead
                        sprite_id = 4 + 5 * 8;
                        break;
                }

                enemy_type = level->enemy[enemy].type;

                if (x == VIEWPORT_WIDTH / 2 && trigger_shot) {
                    hit_enemy(&level->enemy[enemy], distance);
                    trigger_shot = false;
                }
            } else {
                sprite_id = level->texture[block_hit_x + block_hit_y * level->width];
                level->map_revealed[block_hit_x + block_hit_y * level->width] = true;
            }

            unsigned int end_y = (unsigned int)(block_size * VIEWPORT_HEIGHT / 2.0);
            double block_y;
            pixel_t * pixel;
            unsigned int y;

            double intensity = MIN(MAX(10.0 / distance, 0.0), 1.0);

            for (unsigned int mid_y = 0; mid_y < end_y; ++mid_y) {
                // top part
                if (viewport_mid >= mid_y) {
                    y = viewport_mid - mid_y;
                    pixel = &(fg_buffer[x + y * VIEWPORT_WIDTH]);
                    block_y = (end_y - mid_y) / (VIEWPORT_HEIGHT * block_size);
                    if (render_enemy) {
                        enemy_color(pixel, block_x, block_y, intensity, sprite_id, enemy_type);
                    } else {
                        object_color(pixel, block_x, block_y, intensity, sprite_id);
                    }
                }

                // bottom part
                if (viewport_mid + mid_y < VIEWPORT_HEIGHT) {
                    y = viewport_mid + mid_y;
                    pixel = &(fg_buffer[x + y * VIEWPORT_WIDTH]);
                    block_y = (mid_y + VIEWPORT_HEIGHT * block_size * 0.5) / (VIEWPORT_HEIGHT * block_size);
                    if (render_enemy) {
                        enemy_color(pixel, block_x, block_y, intensity, sprite_id, enemy_type);
                    } else {
                        object_color(pixel, block_x, block_y, intensity, sprite_id);
                    }
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

        level->map_revealed[block_hit_x + block_hit_y * level->width] = true;

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

static void fill_in_weapon(level_t * level) {
    unsigned int weapon_id = 1;
    unsigned int animation_step;
    unsigned int step;

    if (shooting_state(level, &step, &trigger_shot)) {
        unsigned int animation_step_size = SHOOTING_ANIMATION_SPEED / SHOOTING_ANIMATION_PARTS;
/*
        if (animation_step == animation_step_size) {
            if (level->ammo == 0) {

            } else {
                level->ammo = level->ammo - 1;

            }
        }
*/
        animation_step = (SHOOTING_ANIMATION_SPEED - step) / animation_step_size;

    } else {
        trigger_shot = false;
        animation_step = 0;
    }

    pixel_t * sprite = weapons_sprites->sprites[animation_step + weapon_id * weapons_sprites->width];

    for (unsigned int y = 0; y < SPRITE_HEIGHT * UI_MULTIPLIER; ++y) {
        for (unsigned int x = 0; x < SPRITE_WIDTH * UI_MULTIPLIER; ++x) {
            if (sprite[x / UI_MULTIPLIER + (y / UI_MULTIPLIER) * SPRITE_WIDTH].alpha == 0) {
                unsigned y2 = y + VIEWPORT_HEIGHT - SPRITE_HEIGHT * UI_MULTIPLIER;
                unsigned x2 = x + VIEWPORT_WIDTH / 2 - SPRITE_WIDTH * UI_MULTIPLIER / 2;

                fg_buffer[x2 + y2 * VIEWPORT_WIDTH] = sprite[x / UI_MULTIPLIER + (y / UI_MULTIPLIER) * SPRITE_WIDTH];
            }
        }
    }
}

void paint_scene(level_t * level) {
    fill_in_background(level);

    fill_in_walls(level);

    fill_in_objects(level);

    fill_in_weapon(level);
}

void init_raycaster(const level_t * level) {
    if (enemy_angles) {
        free(enemy_angles);
    }

    enemy_angles = malloc(level->enemies_count);
}
