#include "global.h"

static unsigned int opening_door_x;
static unsigned int opening_door_y;
static unsigned int door_open_ticks;

static unsigned int flash_effect_ticks;
static unsigned int flash_effect_duration;

bool transition_step() {
    bool ret = false;

    if (door_open_ticks > 0) {
        door_open_ticks--;
        ret = true;
    }
    if (flash_effect_ticks > 0) {
        flash_effect_ticks--;
        ret = true;
    }

    return ret;
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

bool open_door_in_front(level_t * level) {
    // do not allow multiple doors opening at once
    if (door_open_ticks > 0) {
        return false;
    }

    double target_x = level->observer_x + sin(level->observer_angle / RADIAN_CONSTANT);
    double target_y = level->observer_y + cos(level->observer_angle / RADIAN_CONSTANT);
    unsigned int door_x = (unsigned int)(target_x + 0.5);
    unsigned int door_y = (unsigned int)(target_y + 0.5);

    if (level->content_type[door_x + door_y * level->width] != CONTENT_TYPE_DOOR) {
        return false;
    }

    opening_door_x = door_x;
    opening_door_y = door_y;
    door_open_ticks = DOOR_OPEN_SPEED;
    level->content_type[door_x + door_y * level->width] = CONTENT_TYPE_DOOR_OPEN;
    return true;
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

bool apply_special_effect(level_t * level, bool * exit_found) {
    double curr_x = level->observer_x;
    double curr_y = level->observer_y;
    unsigned int rounded_x = (unsigned int)(curr_x + 0.5);
    unsigned int rounded_y = (unsigned int)(curr_y + 0.5);

    *exit_found = false;

    unsigned int i = rounded_x + rounded_y * level->width;
    unsigned char effect = level->special_effects[i];

    switch(effect) {
        case SPECIAL_EFFECT_LEVEL_END:
            *exit_found = true;
            return true;
        case SPECIAL_EFFECT_SCORE_1:
            level->content_type[i] = CONTENT_TYPE_EMPTY;
            level->special_effects[i] = SPECIAL_EFFECT_NONE;
            start_flash_effect(TREASURE_PICKUP_FLASH_DURATION);
            level->score += 1;
            return true;
        case SPECIAL_EFFECT_SCORE_2:
            level->content_type[i] = CONTENT_TYPE_EMPTY;
            level->special_effects[i] = SPECIAL_EFFECT_NONE;
            start_flash_effect(TREASURE_PICKUP_FLASH_DURATION);
            level->score += 2;
            return true;
        case SPECIAL_EFFECT_SCORE_3:
            level->content_type[i] = CONTENT_TYPE_EMPTY;
            level->special_effects[i] = SPECIAL_EFFECT_NONE;
            start_flash_effect(TREASURE_PICKUP_FLASH_DURATION);
            level->score += 4;
            return true;
        case SPECIAL_EFFECT_SCORE_4:
            level->content_type[i] = CONTENT_TYPE_EMPTY;
            level->special_effects[i] = SPECIAL_EFFECT_NONE;
            start_flash_effect(TREASURE_PICKUP_FLASH_DURATION);
            level->score += 10;
            return true;
        default:
            return false;
    }
}
