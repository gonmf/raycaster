#include "global.h"

static unsigned int opening_door_x;
static unsigned int opening_door_y;
static unsigned int door_open_ticks;

bool transition_step() {
    if (door_open_ticks > 0) {
        door_open_ticks--;
        return true;
    }
    return false;
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

bool apply_special_effect(const level_t * level, bool * exit_found) {
    double curr_x = level->observer_x;
    double curr_y = level->observer_y;
    unsigned int rounded_x = (unsigned int)(curr_x + 0.5);
    unsigned int rounded_y = (unsigned int)(curr_y + 0.5);

    *exit_found = false;

    if (level->special_effects[rounded_x + rounded_y * level->width] == SPECIAL_EFFECT_LEVEL_END) {
        *exit_found = true;
        return true;
    }

    return false;
}
