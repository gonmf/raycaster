#include "global.h"


typedef struct  __obj_distance_ {
    bool is_object;
    unsigned int i;
    double val;
} obj_distance_t;

sprite_pack_t * wall_textures;
sprite_pack_t * objects_sprites;
static sprite_pack_t * enemy_sprites[5];
static sprite_pack_t * weapons_sprites;
sprite_pack_t * font_sprites;

pixel_t fg_buffer[VIEWPORT_WIDTH * VIEWPORT_HEIGHT];

static char * enemy_angles;
static bool trigger_shot;
static obj_distance_t * sorted_distances;
static double fish_eye_table[VIEWPORT_WIDTH];
static bool * object_locations;
static bool object_locations_refresh_required;

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

static int obj_distance_compare(const void * a, const void * b) {
  return ((const obj_distance_t *)a)->val > ((const obj_distance_t *)b)->val;
}

static double cast_simple_ray(const level_t * level, double angle, double * block_x, unsigned int * object, unsigned int * enemy, unsigned int * step_count) {
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

        bool opening_this_door = opening_door && opening_door_x == rounded_x && opening_door_y == rounded_y;
        if (opening_this_door && skipped_door) {
            continue;
        }

        unsigned char content_type = level->content_type[rounded_x + rounded_y * level->width];

        if (content_type == CONTENT_TYPE_WALL || content_type == CONTENT_TYPE_DOOR) {
            return -1;
        } else if (content_type == CONTENT_TYPE_DOOR_OPEN && opening_this_door) {
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
        } else { // CONTENT_TYPE_EMPTY
            if (!object_locations[rounded_x + rounded_y * level->width]) continue;

            unsigned int sorted_count = 0;

            for (unsigned int i = 0; i < level->objects_count; ++i) {
                if (*object == i) {
                        continue;
                    }
                double target_x = level->object[i].x;
                double target_y = level->object[i].y;
                double dist = distance(target_x, target_y, curr_x, curr_y);
                if (dist > 0.5) { continue; }

                sorted_distances[sorted_count].is_object = true;
                sorted_distances[sorted_count].val = dist;
                sorted_distances[sorted_count].i = i;
                sorted_count++;
            }
            for (unsigned int i = 0; i < level->enemies_count; ++i) {
                if (*enemy == i) {
                        continue;
                    }
                double target_x = level->enemy[i].x;
                double target_y = level->enemy[i].y;
                double dist = distance(target_x, target_y, curr_x, curr_y);
                if (dist > 0.5) { continue; }

                sorted_distances[sorted_count].is_object = false;
                sorted_distances[sorted_count].val = dist;
                sorted_distances[sorted_count].i = i;
                sorted_count++;
            }

            if (sorted_count > 1) {
                qsort(sorted_distances, sorted_count, sizeof(obj_distance_t), obj_distance_compare);
            }

            for (unsigned int i = 0; i < sorted_count; ++i) {
                double target_x;
                double target_y;
                if (sorted_distances[i].is_object) {
                    target_x = level->object[sorted_distances[i].i].x;
                    target_y = level->object[sorted_distances[i].i].y;
                } else {
                    target_x = level->enemy[sorted_distances[i].i].x;
                    target_y = level->enemy[sorted_distances[i].i].y;
                }
                double obs_dist = distance(level->observer_x, level->observer_y, target_x, target_y);
                double angle_to_center = atan((level->observer_x - target_x) / (level->observer_y - target_y)) * RADIAN_CONSTANT;
                double angle_diff = angle_to_center - angle;
                double opposite = tan(angle_diff / RADIAN_CONSTANT) * obs_dist;
                if (fabs(opposite) > 0.5) {
                    continue;
                }

                if (sorted_distances[i].is_object) {
                    *object = sorted_distances[i].i;
                    *enemy = INT_MAX;
                } else {
                    *object = INT_MAX;
                    *enemy = sorted_distances[i].i;
                }
                *block_x = 1.0 - MIN(MAX((opposite + 0.5) / 1.0, 0.0), 1.0);
                *step_count = steps + 1;
                return obs_dist;
            }
            continue;
        }
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
    unsigned int x = (unsigned int)(enemy_sprites[enemy_type]->sprite_size * sprite_x);
    unsigned int y = (unsigned int)(enemy_sprites[enemy_type]->sprite_size * sprite_y);

    pixel_t src = enemy_sprites[enemy_type]->sprites[sprite_type][x + y * enemy_sprites[enemy_type]->sprite_size];
    // alpha == 1 is used as indicator another object's pixel was already painted
    if (src.alpha == 0 && dst->alpha == 0) {
        *dst = darken_shading(src, intensity);
        dst->alpha = 1;
    }
}

static void object_color(pixel_t * dst, double sprite_x, double sprite_y, double intensity, unsigned char sprite_type) {
    unsigned int x = (unsigned int)(objects_sprites->sprite_size * sprite_x);
    unsigned int y = (unsigned int)(objects_sprites->sprite_size * sprite_y);

    pixel_t src = objects_sprites->sprites[sprite_type][x + y * objects_sprites->sprite_size];
    // alpha == 1 is used as indicator another object's pixel was already painted
    if (src.alpha == 0 && dst->alpha == 0) {
        *dst = darken_shading(src, intensity);
        dst->alpha = 1;
    }
}

static void block_color(pixel_t * dst, double block_x, double block_y, double intensity, unsigned char block_type) {
    unsigned int x = (unsigned int)(wall_textures->sprite_size * block_x);
    unsigned int y = (unsigned int)(wall_textures->sprite_size * block_y);

    pixel_t src = wall_textures->sprites[block_type][x + y * wall_textures->sprite_size];
    *dst = darken_shading(src, intensity);
}

void invalidate_objects_cache() {
    object_locations_refresh_required = true;
}

static void refresh_object_cache(const level_t * level) {
    if (!object_locations_refresh_required) {
        return;
    }

    object_locations_refresh_required = false;
    memset(object_locations, false, level->width * level->height * sizeof(bool));
    for (unsigned int i = 0; i < level->enemies_count; ++i) {
        unsigned int x = (unsigned int)(level->enemy[i].x + 0.5);
        unsigned int y = (unsigned int)(level->enemy[i].y + 0.5);

        object_locations[x + y * level->width] = true;
        if (x > 0) {
            object_locations[x - 1 + y * level->width] = true;
        }
        if (x < level->width - 1) {
            object_locations[x + 1 + y * level->width] = true;
        }
        if (y > 0) {
            object_locations[x + (y - 1) * level->width] = true;
        }
        if (y < level->height - 1) {
            object_locations[x + (y + 1) * level->width] = true;
        }
    }
    for (unsigned int i = 0; i < level->objects_count; ++i) {
        unsigned int x = (unsigned int)(level->object[i].x + 0.5);
        unsigned int y = (unsigned int)(level->object[i].y + 0.5);

        object_locations[x + y * level->width] = true;
    }
}

static void fill_in_objects(level_t * level) {
    refresh_object_cache(level);

    unsigned int object = INT_MAX;
    unsigned int enemy = INT_MAX;
    unsigned int viewport_mid = horizon_offset(level);
    unsigned char enemy_type = 0;
    double block_x;

    // We remember enemy angles to make sure in the same frame always the same angle/sprite
    // is used.
    memset(enemy_angles, -1, level->enemies_count);

    for (unsigned int x = 0; x < VIEWPORT_WIDTH; ++x) {
        double x_angle = fit_angle(fish_eye_table[x] + level->observer_angle);
        object = INT_MAX;
        enemy = INT_MAX;

        unsigned int min_step_count = 0;
        while(1) {
            double distance = cast_simple_ray(level, x_angle, &block_x, &object, &enemy, &min_step_count);
            if (distance <= 0.0) {
                break;
            }

            bool render_enemy = enemy != INT_MAX;
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
                        if (enemy_angles[enemy] == -1) {
                            angles_sum = (unsigned int)(x_angle + level->enemy[enemy].angle + 180 + 23);
                            enemy_angles[enemy] = (8 - (((unsigned int)angles_sum) / 45)) % 8;
                        }

                        animation_step = ((ENEMY_MOVING_ANIMATION_SPEED - level->enemy[enemy].animation_step) * ENEMY_MOVING_ANIMATION_PARTS) / ENEMY_MOVING_ANIMATION_SPEED;
                        sprite_id = enemy_angles[enemy] + animation_step * 8 + ENEMY_MOVING_TEXTURE;
                        break;
                    case ENEMY_STATE_SHOT:
                        sprite_id = ENEMY_SHOT_TEXTURE;
                        break;
                    case ENEMY_STATE_ALERT:
                        sprite_id = ENEMY_ALERT_TEXTURE;
                        break;
                    case ENEMY_STATE_SHOOTING:
                        animation_step = ((ENEMY_SHOOTING_ANIMATION_SPEED - level->enemy[enemy].animation_step) * ENEMY_SHOOTING_ANIMATION_PARTS) / ENEMY_SHOOTING_ANIMATION_SPEED;
                        sprite_id = animation_step + ENEMY_SHOOTING_TEXTURE;
                        break;
                    case ENEMY_STATE_DYING:
                        animation_step = ((ENEMY_DYING_ANIMATION_SPEED - level->enemy[enemy].animation_step) * ENEMY_DYING_ANIMATION_PARTS) / ENEMY_DYING_ANIMATION_SPEED;
                        sprite_id = animation_step + ENEMY_DYING_TEXTURE;
                        break;
                    default: // dead
                        sprite_id = ENEMY_DEAD_TEXTURE;
                        break;
                }

                enemy_type = level->enemy[enemy].type;

                if (x == VIEWPORT_WIDTH / 2 && trigger_shot) {
                    if (level->enemy[enemy].state != ENEMY_STATE_DYING && level->enemy[enemy].state != ENEMY_STATE_DEAD) {
                        if (level->weapon > 0 || distance < 1.2) { // not a knife or in close quarters
                            hit_enemy(level, enemy, distance);
                        }
                        trigger_shot = false;
                    }
                }
            } else { // render object
                sprite_id = level->object[object].texture;
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
    unsigned int weapon_id = level->weapon;
    unsigned int animation_step;
    unsigned int step;
    double weapon_switch_percentage;
    bool weapon_switching = weapon_transition(&weapon_switch_percentage);

    if (shooting_state(level, &step, &trigger_shot)) {
        unsigned int animation_step_size = SHOOTING_ANIMATION_SPEED / SHOOTING_ANIMATION_PARTS;
        animation_step = (SHOOTING_ANIMATION_SPEED - step) / animation_step_size;
    } else {
        trigger_shot = false;
        animation_step = 0;
    }

    pixel_t * sprite = weapons_sprites->sprites[animation_step + weapon_id * weapons_sprites->width];
    double shift = weapon_switching ? (weapon_switch_percentage <= 0.5 ? weapon_switch_percentage * 2 : 2.0 - weapon_switch_percentage * 2) : 0;

    for (unsigned int y = 0; y < weapons_sprites->sprite_size * WEAPON_SPRITE_MULTIPLIER; ++y) {
        for (unsigned int x = 0; x < weapons_sprites->sprite_size * WEAPON_SPRITE_MULTIPLIER; ++x) {
            if (sprite[x / WEAPON_SPRITE_MULTIPLIER + (y / WEAPON_SPRITE_MULTIPLIER) * weapons_sprites->sprite_size].alpha == 0) {
                unsigned int y2 = y + VIEWPORT_HEIGHT - (weapons_sprites->sprite_size - ((unsigned int)(shift * weapons_sprites->sprite_size))) * WEAPON_SPRITE_MULTIPLIER;
                if (y2 >= VIEWPORT_HEIGHT) {
                    continue;
                }
                unsigned int x2 = x + VIEWPORT_WIDTH / 2 - weapons_sprites->sprite_size * WEAPON_SPRITE_MULTIPLIER / 2;

                fg_buffer[x2 + y2 * VIEWPORT_WIDTH] = sprite[x / WEAPON_SPRITE_MULTIPLIER + (y / WEAPON_SPRITE_MULTIPLIER) * weapons_sprites->sprite_size];
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

void load_textures() {
    wall_textures = calloc(1, sizeof(sprite_pack_t));
    read_sprite_pack(wall_textures, "walls");
    objects_sprites = calloc(1, sizeof(sprite_pack_t));
    read_sprite_pack(objects_sprites, "objects");
    enemy_sprites[0] = calloc(1, sizeof(sprite_pack_t));
    read_sprite_pack(enemy_sprites[0], "soldier1");
    enemy_sprites[1] = calloc(1, sizeof(sprite_pack_t));
    read_sprite_pack(enemy_sprites[1], "soldier2");
    // enemy_sprites[2] = calloc(1, sizeof(sprite_pack_t));
    // read_sprite_pack(enemy_sprites[2], "soldier3");
    // enemy_sprites[3] = calloc(1, sizeof(sprite_pack_t));
    // read_sprite_pack(enemy_sprites[3], "soldier4");
    // enemy_sprites[4] = calloc(1, sizeof(sprite_pack_t));
    // read_sprite_pack(enemy_sprites[4], "soldier5");
    weapons_sprites = calloc(1, sizeof(sprite_pack_t));
    read_sprite_pack(weapons_sprites, "weapons");
    font_sprites = calloc(1, sizeof(sprite_pack_t));
    read_sprite_pack(font_sprites, "font_red");
}

void init_raycaster(const level_t * level) {
    if (enemy_angles) {
        free(enemy_angles);
    }
    if (sorted_distances) {
        free(sorted_distances);
    }
    if (object_locations) {
        free(object_locations);
    }

    enemy_angles = malloc(level->enemies_count);
    // level->enemies_count * 2 because dead enemies create an object
    sorted_distances = malloc((level->objects_count + level->enemies_count * 2) * sizeof(obj_distance_t));

    object_locations = malloc(level->width * level->height * sizeof(bool));
    object_locations_refresh_required = true;
}
