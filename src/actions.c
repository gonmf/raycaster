#include "global.h"

static unsigned int opening_door_x;
static unsigned int opening_door_y;
static unsigned int door_open_ticks;

static unsigned int flash_effect_ticks;
static unsigned int flash_effect_duration;

static unsigned int shooting_ticks;

void transition_step() {
    if (door_open_ticks > 0) {
        door_open_ticks--;
    }
    if (flash_effect_ticks > 0) {
        flash_effect_ticks--;
    }
    if (shooting_ticks > 0) {
        shooting_ticks--;
    }
}

bool opening_door_transition(double * percentage_open, unsigned int * door_x, unsigned int * door_y) {
    if (door_open_ticks > 0) {
        *door_x = opening_door_x;
        *door_y = opening_door_y;
        *percentage_open = 1.0 - door_open_ticks / ((double)DOOR_OPEN_SPEED);
        return true;
    } else {
        return false;
    }
}

void open_door_in_front(level_t * level) {
    // do not allow multiple doors opening at once
    if (door_open_ticks > 0) {
        return;
    }

    double target_x = level->observer_x + sin(level->observer_angle / RADIAN_CONSTANT);
    double target_y = level->observer_y + cos(level->observer_angle / RADIAN_CONSTANT);
    unsigned int door_x = (unsigned int)(target_x + 0.5);
    unsigned int door_y = (unsigned int)(target_y + 0.5);

    if (level->content_type[door_x + door_y * level->width] != CONTENT_TYPE_DOOR) {
        return;
    }

    unsigned char door_effect = level->special_effects[door_x + door_y * level->width];
    if (door_effect == SPECIAL_EFFECT_REQUIRES_KEY_1) {
        if(level->key_1) {
            printf("Key used\n");
            level->special_effects[door_x + door_y * level->width] = SPECIAL_EFFECT_NONE;
        } else {
            printf("Door requires key\n");
            return;
        }
    } else if (door_effect == SPECIAL_EFFECT_REQUIRES_KEY_2) {
        if(level->key_2) {
            printf("Key used\n");
            level->special_effects[door_x + door_y * level->width] = SPECIAL_EFFECT_NONE;
        } else {
            printf("Door requires key\n");
            return;
        }
    }

    opening_door_x = door_x;
    opening_door_y = door_y;
    door_open_ticks = DOOR_OPEN_SPEED;
    level->content_type[door_x + door_y * level->width] = CONTENT_TYPE_DOOR_OPEN;
}

bool short_flash_effect(double * percentage) {
    if (flash_effect_ticks > 0) {
        if (flash_effect_ticks >= flash_effect_duration / 2) {
            *percentage = 1.0 - flash_effect_ticks / ((double)flash_effect_duration);
        } else {
            *percentage = flash_effect_ticks / ((double)flash_effect_duration);
        }
        return true;
    }
    return false;
}

void start_flash_effect(unsigned int duration) {
    flash_effect_ticks = duration;
    flash_effect_duration = duration;
}

static int closest_object_w_special_effect(const level_t * level) {
    double curr_x = level->observer_x;
    double curr_y = level->observer_y;
    int min_i = -1;
    double min_distance = 0.0;

    for (unsigned int i = 0; i < level->objects_count; ++i) {
        if (level->object[i].special_effect != SPECIAL_EFFECT_NONE) {
            double dist = distance(level->object[i].x, level->object[i].y, curr_x, curr_y);
            if (min_i == -1 || dist < min_distance) {
                min_i = i;
                min_distance = dist;
            }
        }
    }

    return min_distance < 0.5 ? min_i : -1;
}

bool apply_special_effect(level_t * level, bool * exit_found) {
    double curr_x = level->observer_x;
    double curr_y = level->observer_y;
    unsigned int rounded_x = (unsigned int)(curr_x + 0.5);
    unsigned int rounded_y = (unsigned int)(curr_y + 0.5);

    *exit_found = false;

    unsigned int i = rounded_x + rounded_y * level->width;
    unsigned char effect = level->special_effects[i];

    switch (effect) {
        case SPECIAL_EFFECT_NONE:
            break;
        case SPECIAL_EFFECT_LEVEL_END:
            *exit_found = true;
            return true;
        default:
            error("Unknown special effect applicable to content type");
    }

    int obj_i = closest_object_w_special_effect(level);
    if (obj_i != -1) {
        effect = level->object[obj_i].special_effect;

        switch (effect) {
            case SPECIAL_EFFECT_NONE:
                break;
            case SPECIAL_EFFECT_SCORE_1:
                level->objects_count--;
                level->object[obj_i] = level->object[level->objects_count];
                start_flash_effect(TREASURE_PICKUP_FLASH_DURATION);
                level->score += 1;
                printf("Score: %u\n", level->score);
                return true;
            case SPECIAL_EFFECT_SCORE_2:
                level->objects_count--;
                level->object[obj_i] = level->object[level->objects_count];
                start_flash_effect(TREASURE_PICKUP_FLASH_DURATION);
                level->score += 2;
                printf("Score: %u\n", level->score);
                return true;
            case SPECIAL_EFFECT_SCORE_3:
                level->objects_count--;
                level->object[obj_i] = level->object[level->objects_count];
                start_flash_effect(TREASURE_PICKUP_FLASH_DURATION);
                level->score += 4;
                printf("Score: %u\n", level->score);
                return true;
            case SPECIAL_EFFECT_SCORE_4:
                level->objects_count--;
                level->object[obj_i] = level->object[level->objects_count];
                start_flash_effect(TREASURE_PICKUP_FLASH_DURATION);
                level->score += 10;
                printf("Score: %u\n", level->score);
                return true;
            case SPECIAL_EFFECT_KEY_1:
                level->objects_count--;
                level->object[obj_i] = level->object[level->objects_count];
                start_flash_effect(TREASURE_PICKUP_FLASH_DURATION);
                level->key_1 = true;
                printf("Keys: %s\n", level->key_1 && level->key_2 ? "1 & 2" : (level->key_1 ? "1" : "2"));
                return true;
            case SPECIAL_EFFECT_KEY_2:
                level->objects_count--;
                level->object[obj_i] = level->object[level->objects_count];
                start_flash_effect(TREASURE_PICKUP_FLASH_DURATION);
                level->key_2 = true;
                printf("Keys: %s\n", level->key_1 && level->key_2 ? "1 & 2" : (level->key_1 ? "1" : "2"));
                return true;
            case SPECIAL_EFFECT_AMMO:
                level->objects_count--;
                level->object[obj_i] = level->object[level->objects_count];
                start_flash_effect(TREASURE_PICKUP_FLASH_DURATION);
                level->ammo += 8;
                return true;
            default:
                error("Unknown special effect applicable to object");
        }
    }

    return false;
}

bool shooting_state(level_t * level, unsigned int * step, bool * trigger_shot) {
    if (shooting_ticks) {
        unsigned int animation_step_size = SHOOTING_ANIMATION_SPEED / SHOOTING_ANIMATION_PARTS;
        if (shooting_ticks == animation_step_size * 3) {
            if (level->ammo == 0) {
                shooting_ticks -= 2 * animation_step_size;
                *trigger_shot = false;
            } else {
                level->ammo = level->ammo - 1;
                *trigger_shot = true;
            }
        }
        *step = shooting_ticks;
        return true;
    } else {
        return false;
    }
}

void shooting_start_action() {
    if (shooting_ticks == 0) {
        shooting_ticks = SHOOTING_ANIMATION_SPEED;
    }
}
