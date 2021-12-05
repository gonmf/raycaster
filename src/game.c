#include "global.h"

static int * player_dist_map;
static int * noise_dist_map;
static unsigned int player_dist_map_x;
static unsigned int player_dist_map_y;

void make_weapon_available(level_t * level, unsigned char weapon_nr) {
    level->weapons_available |= 1 << weapon_nr;

    switch_weapon(level, weapon_nr);
}

void switch_weapon(level_t * level, unsigned char new_weapon_nr) {
    if (level->weapon != new_weapon_nr && level->weapons_available & (1 << new_weapon_nr)) {
        start_weapon_transition(new_weapon_nr);
    }
}

void hit_enemy(level_t * level, unsigned int enemy_i, double distance) {
    enemy_t * enemy = &level->enemy[enemy_i];

    if (enemy->state == ENEMY_STATE_STILL || enemy->state == ENEMY_STATE_MOVING || enemy->state == ENEMY_STATE_ALERT) {
        enemy->state = ENEMY_STATE_SHOT;
        enemy->animation_step = ENEMY_SHOT_ANIMATION_SPEED;
    }

    if (enemy->state != ENEMY_STATE_DYING && enemy->state != ENEMY_STATE_DEAD) {
        unsigned char shot_power = MAX(MIN((unsigned int)(18 / distance) + 18, 50), 18);
        if (enemy->life > shot_power) {
            enemy->life -= shot_power;
            enemy->state = ENEMY_STATE_SHOT;
            enemy->animation_step = ENEMY_SHOT_ANIMATION_SPEED;

            if (enemy->strategic_state == ENEMY_STRATEGIC_STATE_WAITING) {
                enemy->strategic_state = ENEMY_STRATEGIC_STATE_ALERTED;
            }
        } else {
            enemy->life = 0;
            enemy->state = ENEMY_STATE_DYING;
            enemy->animation_step = ENEMY_DYING_ANIMATION_SPEED;

            level->object[level->objects_count].type = OBJECT_TYPE_NON_BLOCK;
            level->object[level->objects_count].texture = SMALL_AMMO_TEXTURE;
            level->object[level->objects_count].special_effect = SPECIAL_EFFECT_AMMO;
            level->object[level->objects_count].x = enemy->x;
            level->object[level->objects_count].y = enemy->y;
            level->objects_count++;
            invalidate_objects_cache();
        }
    }
}

static void hit_player(level_t * level, enemy_t * enemy) {
    double dist = distance(enemy->x, enemy->y, level->observer_x, level->observer_y);

    // 2: 100% accuracy
    // ENEMY_SHOOTING_MAX_DISTANCE: 33% accuracy
    double accuracy = (1.0 - (MAX(dist, 2.0) - 2.0) / (ENEMY_SHOOTING_MAX_DISTANCE - 2)) * 0.67 + 0.33;
    bool hit = rand() / ((double)RAND_MAX) < accuracy;

    if (!hit) {
        return;
    }

    if (player_moving() && random_one_in(2)) {
        return;
    }

    unsigned char shot_power = MAX(MIN((unsigned int)(18 / dist) + 18, 50), 18) / 2;
    if (level->life > shot_power) {
        level->life -= shot_power;
        start_flash_effect(PLAYER_SHOT_FLASH_DURATION, &color_dark_red);
    } else {
        level->life = 0;
    }
}

static void decrement_animation_step(enemy_t * enemy) {
    if (enemy->animation_step > 0) {
        enemy->animation_step--;
    }
}

static void fill_in_player_dist_map(const level_t * level, int * map, unsigned int x, unsigned int y, int steps) {
    unsigned int width = level->width;
    unsigned int height = level->height;

    if (x >= width || y >= height) {
        return;
    }
    if (map[x + y * width] == -2 || level->content_type[x + y * width] == CONTENT_TYPE_WALL || level->content_type[x + y * width] == CONTENT_TYPE_DOOR) {
        return;
    }
    if (map[x + y * width] == -1) {
        steps += 10;
    }

    if (map[x + y * width] > 0 && map[x + y * width] <= steps) {
        return;
    }

    map[x + y * width] = steps;

    fill_in_player_dist_map(level, map, x + 1, y, steps + 10);
    fill_in_player_dist_map(level, map, x - 1, y, steps + 10);
    fill_in_player_dist_map(level, map, x, y + 1, steps + 10);
    fill_in_player_dist_map(level, map, x, y - 1, steps + 10);

    fill_in_player_dist_map(level, map, x + 1, y + 1, steps + 14);
    fill_in_player_dist_map(level, map, x + 1, y - 1, steps + 14);
    fill_in_player_dist_map(level, map, x - 1, y + 1, steps + 14);
    fill_in_player_dist_map(level, map, x - 1, y - 1, steps + 14);
}

static void pick_target_coord(
    unsigned int * target_x, unsigned int * target_y, double * target_angle,
    int * min_distance,
    const level_t * level,
    unsigned int x, unsigned int y,
    int change_x, int change_y, double angle
) {
    unsigned int goal_x = (unsigned int)(((int)x) + change_x);
    unsigned int goal_y = (unsigned int)(((int)y) + change_y);

    if (goal_x >= level->width || goal_y >= level->height) {
        return;
    }

    int dist = player_dist_map[goal_x + goal_y * level->width];
    if (dist <= 0) {
        return;
    }
    dist /= 10;

    if (*min_distance == -1 || dist < *min_distance) {
        *min_distance = dist;
        *target_x = goal_x;
        *target_y = goal_y;
        *target_angle = angle;
    }
}

static void calc_distances_to_player(const level_t * level) {
    unsigned int width = level->width;
    unsigned int height = level->height;
    unsigned int rounded_x = (unsigned int)(level->observer_x + 0.5);
    unsigned int rounded_y = (unsigned int)(level->observer_y + 0.5);

    if (player_dist_map_x == rounded_x && player_dist_map_y == rounded_y) {
        return;
    }

    // map can be accessible (0), difficult (-1) or impassable (-2)
    memset(player_dist_map, 0, width * height * sizeof(int));
    memset(noise_dist_map, 0, width * height * sizeof(int));

    for (unsigned int i = 0; i < level->enemies_count; ++i) {
        unsigned int x = (unsigned int)(level->enemy[i].x + 0.5);
        unsigned int y = (unsigned int)(level->enemy[i].y + 0.5);

        player_dist_map[x + y * level->width] = -1;
    }
    for (unsigned int i = 0; i < level->objects_count; ++i) {
        unsigned int x = (unsigned int)(level->object[i].x + 0.5);
        unsigned int y = (unsigned int)(level->object[i].y + 0.5);

        if (level->object[i].type == OBJECT_TYPE_BLOCKING) {
            player_dist_map[x + y * level->width] = -2;
        } else {
            player_dist_map[x + y * level->width] = -1;
        }
    }

    fill_in_player_dist_map(level, player_dist_map, rounded_x, rounded_y, 0);
    fill_in_player_dist_map(level, noise_dist_map, rounded_x, rounded_y, 0);
}

void alert_enemies_in_proximity(const level_t * level, unsigned int distance) {
    calc_distances_to_player(level);

    for (unsigned int i = 0; i < level->enemies_count; ++i) {
        enemy_t * enemy = &level->enemy[i];

        if (enemy->strategic_state == ENEMY_STRATEGIC_STATE_WAITING) {
            unsigned int x = (unsigned int)(enemy->x + 0.5);
            unsigned int y = (unsigned int)(enemy->y + 0.5);

            int dist = noise_dist_map[x + y * level->width];

            if (dist > 0 && dist < (int)(distance * 10)) {
                enemy->strategic_state = ENEMY_STRATEGIC_STATE_ALERTED;
            }
        }
    }
}

static bool is_empty(const level_t * level, unsigned int x, unsigned int y) {
    if (x < level->width && y < level->height) {
        return level->content_type[x + y * level->width] == CONTENT_TYPE_EMPTY || level->content_type[x + y * level->width] == CONTENT_TYPE_DOOR_OPEN;
    }
    return true;
}

static bool make_movement_plan(const level_t * level, enemy_t * enemy) {
    calc_distances_to_player(level);

    unsigned int rounded_enemy_x = (unsigned int)(enemy->x + 0.5);
    unsigned int rounded_enemy_y = (unsigned int)(enemy->y + 0.5);

    int dist = player_dist_map[rounded_enemy_x + rounded_enemy_y * level->width];
    if (dist == -1) {
        return false; // no path exists
    }

    unsigned int target_x = 0;
    unsigned int target_y = 0;
    int min_distance = -1;
    double angle = 0.0;

    bool can_move_diagonally = is_empty(level, rounded_enemy_x + 1, rounded_enemy_y) &&
                               is_empty(level, rounded_enemy_x, rounded_enemy_y + 1) &&
                               is_empty(level, rounded_enemy_x - 1, rounded_enemy_y) &&
                               is_empty(level, rounded_enemy_x, rounded_enemy_y - 1);
    pick_target_coord(&target_x, &target_y, &angle, &min_distance, level, rounded_enemy_x, rounded_enemy_y, 1, 0, 270.0);
    pick_target_coord(&target_x, &target_y, &angle, &min_distance, level, rounded_enemy_x, rounded_enemy_y, 0, 1, 0.0);
    pick_target_coord(&target_x, &target_y, &angle, &min_distance, level, rounded_enemy_x, rounded_enemy_y, -1, 0, 90.0);
    pick_target_coord(&target_x, &target_y, &angle, &min_distance, level, rounded_enemy_x, rounded_enemy_y, 0, -1, 180.0);
    if (can_move_diagonally) {
        pick_target_coord(&target_x, &target_y, &angle, &min_distance, level, rounded_enemy_x, rounded_enemy_y, 1, 1, 315.0);
        pick_target_coord(&target_x, &target_y, &angle, &min_distance, level, rounded_enemy_x, rounded_enemy_y, 1, -1, 225.0);
        pick_target_coord(&target_x, &target_y, &angle, &min_distance, level, rounded_enemy_x, rounded_enemy_y, -1, 1, 45.0);
        pick_target_coord(&target_x, &target_y, &angle, &min_distance, level, rounded_enemy_x, rounded_enemy_y, -1, -1, 135.0);
    }

    if (min_distance == -1) {
        error("unexpected error in path finding");
    }

    double final_target_x = ((double)target_x) - 0.2 + 0.4 * (rand() / ((double)RAND_MAX));
    double final_target_y = ((double)target_y) - 0.2 + 0.4 * (rand() / ((double)RAND_MAX));

    double distance_to_target = distance(final_target_x, final_target_y, enemy->x, enemy->y);

    enemy->moving_dir_x = (final_target_x - enemy->x) / (ENEMY_MOVING_ANIMATION_SPEED * distance_to_target);
    enemy->moving_dir_y = (final_target_y - enemy->y) / (ENEMY_MOVING_ANIMATION_SPEED * distance_to_target);
    enemy->angle = angle;

    return true;
}

static bool in_enemy_view(level_t * level, enemy_t * enemy) {
    double x = level->observer_x - enemy->x;
    double y = level->observer_y - enemy->y;

    double angle = fit_angle(atan(y / x) * RADIAN_CONSTANT);
    if (x < 0.0) {
        angle -= 180.0;
    }

    angle += enemy->angle;

    return angle > 90.0 - (ENEMY_FIELD_OF_VIEW / 2.0) && angle < 90.0 + (ENEMY_FIELD_OF_VIEW / 2.0);
}

static bool in_shooting_distance(double distance) {
    if (distance < ENEMY_SHOOTING_MAX_DISTANCE - 2) {
        return true;
    }
    if (distance < ENEMY_SHOOTING_MAX_DISTANCE) {
        return random_one_in(2) || random_one_in(2); // probability 75%
    }
    return false;
}

static bool has_line_of_fire(const level_t * level, const enemy_t * enemy, double distance) {
    double x = level->observer_x - enemy->x;
    double y = level->observer_y - enemy->y;

    double angle_radians = atan(y / x);
    double x_change = cos(angle_radians) * ROUGH_RAY_STEP_CONSTANT;
    double y_change = sin(angle_radians) * ROUGH_RAY_STEP_CONSTANT;
    double curr_x = enemy->x + 0.5;
    double curr_y = enemy->y + 0.5;

    unsigned int steps = (unsigned int)(distance / ROUGH_RAY_STEP_CONSTANT);

    for (unsigned int i = 0; i <= steps; ++i) {
        curr_x += x_change;
        curr_y += y_change;

        unsigned int rounded_x = (unsigned int)curr_x;
        unsigned int rounded_y = (unsigned int)curr_y;

        unsigned int content_type = level->content_type[rounded_x + rounded_y * level->width];
        if (content_type == CONTENT_TYPE_WALL || content_type == CONTENT_TYPE_DOOR) {
            return false;
        }
    }

    return true;
}

void update_enemies_state(level_t * level) {
    for (unsigned int enemy_i = 0; enemy_i < level->enemies_count; ++enemy_i) {
        enemy_t * enemy = &level->enemy[enemy_i];
        double enemy_distance = distance(enemy->x, enemy->y, level->observer_x, level->observer_y);

        switch (enemy->state) {
        case ENEMY_STATE_STILL:
            break;
        case ENEMY_STATE_ALERT:
            break;
        case ENEMY_STATE_MOVING:
            decrement_animation_step(enemy);
            enemy->x += enemy->moving_dir_x;
            enemy->y += enemy->moving_dir_y;
            if (enemy->animation_step == 0) {
                if (in_shooting_distance(enemy_distance) && has_line_of_fire(level, enemy, enemy_distance)) {
                    enemy->animation_step = ENEMY_SHOOTING_ANIMATION_SPEED;
                    enemy->state = ENEMY_STATE_SHOOTING;
                } else {
                    enemy->state = ENEMY_STATE_ALERT;
                }
            }
            invalidate_objects_cache();
            break;
        case ENEMY_STATE_SHOT:
            decrement_animation_step(enemy);
            if (enemy->animation_step == 0) {
                if (in_shooting_distance(enemy_distance) && has_line_of_fire(level, enemy, enemy_distance)) {
                    enemy->animation_step = ENEMY_SHOOTING_ANIMATION_SPEED;
                    enemy->state = ENEMY_STATE_SHOOTING;
                } else {
                    enemy->state = ENEMY_STATE_ALERT;
                }
            }
            break;
        case ENEMY_STATE_SHOOTING:
            decrement_animation_step(enemy);
            if (enemy->animation_step == (ENEMY_SHOOTING_ANIMATION_SPEED * ENEMY_SHOOTING_ACTIVATION_PART) / ENEMY_SHOOTING_ANIMATION_PARTS) {
                hit_player(level, enemy);
            } else if (enemy->animation_step == 0) {
                if (in_shooting_distance(enemy_distance) && has_line_of_fire(level, enemy, enemy_distance)) {
                    enemy->animation_step = ENEMY_SHOOTING_ANIMATION_SPEED;
                    enemy->state = ENEMY_STATE_SHOOTING;
                } else {
                    enemy->state = ENEMY_STATE_ALERT;
                }
            }
            break;
        case ENEMY_STATE_DYING:
            decrement_animation_step(enemy);
            if (enemy->animation_step == 0) {
                enemy->state = ENEMY_STATE_DEAD;
            }
            break;
        default: // dead
            break;
        }

        if (enemy->state != ENEMY_STATE_STILL && enemy->state != ENEMY_STATE_ALERT) {
            continue;
        }
        if (enemy->strategic_state == ENEMY_STRATEGIC_STATE_WAITING) {
            if (enemy_distance < ENEMY_VIEWING_DISTANCE && in_enemy_view(level, enemy)) {
                enemy->strategic_state = ENEMY_STRATEGIC_STATE_ALERTED;
            }
        }

        if (enemy->strategic_state == ENEMY_STRATEGIC_STATE_ENGAGED) {
            if (in_shooting_distance(enemy_distance) && has_line_of_fire(level, enemy, enemy_distance)) {
                enemy->state = ENEMY_STATE_SHOOTING;
                enemy->animation_step = ENEMY_SHOOTING_ANIMATION_SPEED;
            } else {
                enemy->strategic_state = ENEMY_STRATEGIC_STATE_ALERTED;
            }
        }

        if (enemy->strategic_state == ENEMY_STRATEGIC_STATE_ALERTED) {
            if (in_shooting_distance(enemy_distance) && has_line_of_fire(level, enemy, enemy_distance)) {
                enemy->strategic_state = ENEMY_STRATEGIC_STATE_ENGAGED;
            } else {
                if (make_movement_plan(level, enemy)) {
                    enemy->state = ENEMY_STATE_MOVING;
                    enemy->animation_step = ENEMY_MOVING_ANIMATION_SPEED;
                } else {
                    // on alert but out of range and unable to find a valid path to player
                }
            }
        }
    }
}

void init_game_buffers(const level_t * level) {
    if (player_dist_map) {
        free(player_dist_map);
    }
    if (noise_dist_map) {
        free(noise_dist_map);
    }

    unsigned int width = level->width;
    unsigned int height = level->height;

    player_dist_map = malloc(width * height * sizeof(int));
    noise_dist_map = malloc(width * height * sizeof(int));
    player_dist_map_x = UINT_MAX;
}
