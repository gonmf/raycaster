#include "global.h"

static int * player_dist_map;
static unsigned int player_dist_map_x;
static unsigned int player_dist_map_y;

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
            printf("Enemy #%u health: %u\n", enemy_i, enemy->life);
        } else {
            enemy->life = 0;
            enemy->state = ENEMY_STATE_DYING;
            enemy->animation_step = ENEMY_DYING_ANIMATION_SPEED;

            level->object[level->objects_count].type = OBJECT_TYPE_NON_BLOCK;
            level->object[level->objects_count].texture = 3 + 5 * 5;
            level->object[level->objects_count].special_effect = SPECIAL_EFFECT_AMMO;
            level->object[level->objects_count].x = enemy->x;
            level->object[level->objects_count].y = enemy->y;
            level->objects_count++;
            printf("Enemy #%u killed\n", enemy_i);
        }
    }
}

static void decrement_animation_step(enemy_t * enemy) {
    if (enemy->animation_step > 0) {
        enemy->animation_step--;
    }
}

static bool enemy_in_range(const level_t * level, enemy_t * enemy) {
    double dist = distance(enemy->x, enemy->y, level->observer_x, level->observer_y);

    // TODO: don't just test the distance
    return dist < ENEMY_SHOOTING_MAX_DISTANCE;
}

static void fill_in_player_dist_map(const level_t * level, unsigned int x, unsigned int y, int steps) {
    unsigned int width = level->width;
    unsigned int height = level->height;

    if (x >= width || y >= height) {
        return;
    }
    if (player_dist_map[x + y * width] == -2) {
        return;
    }
    if (level->content_type[x + y * width] == CONTENT_TYPE_WALL || level->content_type[x + y * width] == CONTENT_TYPE_DOOR) {
        return;
    }
    if (player_dist_map[x + y * width] != -1 && player_dist_map[x + y * width] <= steps) {
        return;
    }
    if (player_dist_map[x + y * width] == -3) {
        steps += 5;
    }

    player_dist_map[x + y * width] = steps;

    fill_in_player_dist_map(level, x + 1, y, steps + 10);
    fill_in_player_dist_map(level, x - 1, y, steps + 10);
    fill_in_player_dist_map(level, x, y + 1, steps + 10);
    fill_in_player_dist_map(level, x, y - 1, steps + 10);

    fill_in_player_dist_map(level, x + 1, y + 1, steps + 14);
    fill_in_player_dist_map(level, x + 1, y - 1, steps + 14);
    fill_in_player_dist_map(level, x - 1, y + 1, steps + 14);
    fill_in_player_dist_map(level, x - 1, y - 1, steps + 14);
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

static void calc_enemy_distances_to_player(const level_t * level) {
    unsigned int width = level->width;
    unsigned int height = level->height;
    unsigned int rounded_x = (unsigned int)(level->observer_x + 0.5);
    unsigned int rounded_y = (unsigned int)(level->observer_y + 0.5);

    if (player_dist_map_x == rounded_x && player_dist_map_y == rounded_y) {
        return;
    }
    // -1 for not explored, -2 for blocked by object, -3 for difficult to traverse
    for (unsigned int i = 0; i < width * height; ++i) {
        player_dist_map[i] = -1;
    }
    for (unsigned int i = 0; i < level->enemies_count; ++i) {
        unsigned int x = (unsigned int)(level->enemy[i].x + 0.5);
        unsigned int y = (unsigned int)(level->enemy[i].y + 0.5);

        player_dist_map[x + y * level->width] = -3;
    }
    for (unsigned int i = 0; i < level->objects_count; ++i) {
        unsigned int x = (unsigned int)(level->object[i].x + 0.5);
        unsigned int y = (unsigned int)(level->object[i].y + 0.5);

        if (level->object[i].type == OBJECT_TYPE_BLOCKING) {
            player_dist_map[x + y * level->width] = -2;
        } else {
            player_dist_map[x + y * level->width] = -3;
        }
    }

    fill_in_player_dist_map(level, rounded_x, rounded_y, 0);
}

static bool make_movement_plan(const level_t * level, enemy_t * enemy) {
    calc_enemy_distances_to_player(level);

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

    pick_target_coord(&target_x, &target_y, &angle, &min_distance, level, rounded_enemy_x, rounded_enemy_y, 1, 0, 270.0);
    pick_target_coord(&target_x, &target_y, &angle, &min_distance, level, rounded_enemy_x, rounded_enemy_y, 0, 1, 0.0);
    pick_target_coord(&target_x, &target_y, &angle, &min_distance, level, rounded_enemy_x, rounded_enemy_y, -1, 0, 90.0);
    pick_target_coord(&target_x, &target_y, &angle, &min_distance, level, rounded_enemy_x, rounded_enemy_y, 0, -1, 180.0);
    pick_target_coord(&target_x, &target_y, &angle, &min_distance, level, rounded_enemy_x, rounded_enemy_y, 1, 1, 315.0);
    pick_target_coord(&target_x, &target_y, &angle, &min_distance, level, rounded_enemy_x, rounded_enemy_y, 1, -1, 225.0);
    pick_target_coord(&target_x, &target_y, &angle, &min_distance, level, rounded_enemy_x, rounded_enemy_y, -1, 1, 45.0);
    pick_target_coord(&target_x, &target_y, &angle, &min_distance, level, rounded_enemy_x, rounded_enemy_y, -1, -1, 135.0);

    if (min_distance == -1) {
        error("unexpected error in path finding");
    }

    enemy->moving_dir_x = (((double)target_x) - enemy->x) / ENEMY_MOVING_ANIMATION_SPEED;
    enemy->moving_dir_y = (((double)target_y) - enemy->y) / ENEMY_MOVING_ANIMATION_SPEED;
    enemy->angle = angle;

    return true;
}

void update_enemies_state(level_t * level) {
    player_dist_map_x = UINT_MAX; // force regen of map if needed
    bool in_range_calcd = false;
    bool in_range = false;

    for (unsigned int enemy_i = 0; enemy_i < level->enemies_count; ++enemy_i) {
        in_range_calcd = false;
        enemy_t * enemy = &level->enemy[enemy_i];

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
                in_range = enemy_in_range(level, enemy);

                if (in_range) {
                    enemy->animation_step = ENEMY_SHOOTING_ANIMATION_SPEED;
                    enemy->state = ENEMY_STATE_SHOOTING;
                } else {
                    enemy->state = ENEMY_STATE_ALERT;
                }
            }
            break;
        case ENEMY_STATE_SHOT:
            decrement_animation_step(enemy);
            if (enemy->animation_step == 0) {
                in_range = enemy_in_range(level, enemy);

                if (in_range) {
                    enemy->animation_step = ENEMY_SHOOTING_ANIMATION_SPEED;
                    enemy->state = ENEMY_STATE_SHOOTING;
                } else {
                    enemy->state = ENEMY_STATE_ALERT;
                }
            }
            break;
        case ENEMY_STATE_SHOOTING:
            decrement_animation_step(enemy);
            if (enemy->animation_step == 0) {
                in_range = enemy_in_range(level, enemy);

                if (in_range) {
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

        if ((enemy->state != ENEMY_STATE_STILL && enemy->state != ENEMY_STATE_ALERT) || enemy->strategic_state == ENEMY_STRATEGIC_STATE_WAITING) {
            continue;
        }

        in_range = in_range_calcd ? in_range : enemy_in_range(level, enemy);

        if (enemy->strategic_state == ENEMY_STRATEGIC_STATE_ALERTED) {
            if (in_range) {
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

        if (enemy->strategic_state == ENEMY_STRATEGIC_STATE_ENGAGED) {
            if (in_range) {
                enemy->state = ENEMY_STATE_SHOOTING;
                enemy->animation_step = ENEMY_SHOOTING_ANIMATION_SPEED;
            } else {
                enemy->strategic_state = ENEMY_STRATEGIC_STATE_ALERTED;
            }
        }
    }
}

void init_game_buffers(const level_t * level) {
    if (player_dist_map) {
        free(player_dist_map);
    }

    unsigned int width = level->width;
    unsigned int height = level->height;

    player_dist_map = malloc(width * height * sizeof(int));
    player_dist_map_x = UINT_MAX;
}
