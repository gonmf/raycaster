#include "global.h"

static unsigned int opening_door_x;
static unsigned int opening_door_y;
static unsigned int door_open_ticks;

static unsigned int flash_effect_ticks;
static unsigned int flash_effect_duration;
static pixel_t * flash_effect_color;

static unsigned int shooting_ticks;

static unsigned int weapon_transition_ticks;
static unsigned char weapon_transition_weapon_nr;

void transition_step(level_t * level, bool * trigger_shot) {
    if (door_open_ticks > 0) {
        door_open_ticks--;
    }
    if (flash_effect_ticks > 0) {
        flash_effect_ticks--;
    }
    if (shooting_ticks > 0) {
        shooting_ticks--;

        if (level->weapon == 2) {
            unsigned int part_ticks = SHOOTING_ANIMATION_SPEED / SHOOTING_ANIMATION_PARTS;
            if (shooting_ticks == part_ticks + part_ticks / 2 && sfMouse_isButtonPressed(sfMouseLeft)) {
                shooting_ticks += part_ticks * 2;
            }
        }
        if (level->weapon == 3) {
            unsigned int part_ticks = SHOOTING_MINIGUN_ANIMATION_SPEED / SHOOTING_ANIMATION_PARTS;
            if (shooting_ticks == part_ticks && sfMouse_isButtonPressed(sfMouseLeft)) {
                shooting_ticks += part_ticks * 2;
            }
        }

        unsigned int animation_step_size;
        if (level->weapon == 3) {
            animation_step_size = SHOOTING_MINIGUN_ANIMATION_SPEED / SHOOTING_ANIMATION_PARTS;
        } else {
            animation_step_size = SHOOTING_ANIMATION_SPEED / SHOOTING_ANIMATION_PARTS;
        }
        if (shooting_ticks == animation_step_size * SHOOTING_ACTIVATION_PART) {
            if (level->ammo > 0 || level->weapon == 0) {
                if (level->weapon > 0) {
                    alert_enemies_in_proximity(level, ENEMY_ALERT_PROXIMITY);
                    level->ammo -= 1;
                }
                *trigger_shot = true;
            } else {
                shooting_ticks -= 2 * animation_step_size;
            }
        }
    }
    if (weapon_transition_ticks > 0) {
        weapon_transition_ticks--;
        if (weapon_transition_ticks == WEAPON_SWITCH_SPEED / 2) {
            level->weapon = weapon_transition_weapon_nr;
        }
    }
}

void start_weapon_transition(unsigned char weapon_nr) {
    if (weapon_transition_ticks == 0) {
        weapon_transition_ticks = WEAPON_SWITCH_SPEED;
        weapon_transition_weapon_nr = weapon_nr;
    }
}

bool weapon_transition(double * percentage) {
    if (weapon_transition_ticks > 0) {
        *percentage = ((double)(WEAPON_SWITCH_SPEED - weapon_transition_ticks)) / WEAPON_SWITCH_SPEED;
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
            level->special_effects[door_x + door_y * level->width] = SPECIAL_EFFECT_NONE;
        } else {
            return;
        }
    } else if (door_effect == SPECIAL_EFFECT_REQUIRES_KEY_2) {
        if(level->key_2) {
            level->special_effects[door_x + door_y * level->width] = SPECIAL_EFFECT_NONE;
        } else {
            return;
        }
    }

    opening_door_x = door_x;
    opening_door_y = door_y;
    door_open_ticks = DOOR_OPEN_SPEED;
    level->content_type[door_x + door_y * level->width] = CONTENT_TYPE_DOOR_OPEN;
}

bool short_flash_effect(double * percentage, pixel_t * color) {
    if (flash_effect_ticks > 0) {
        if (flash_effect_ticks >= flash_effect_duration / 2) {
            *percentage = 1.0 - flash_effect_ticks / ((double)flash_effect_duration);
        } else {
            *percentage = flash_effect_ticks / ((double)flash_effect_duration);
        }
        *color = *flash_effect_color;
        return true;
    }
    return false;
}

void start_flash_effect(unsigned int duration, pixel_t * color) {
    flash_effect_ticks = duration;
    flash_effect_duration = duration;
    flash_effect_color = color;
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

void apply_special_effect(level_t * level, bool * exit_found) {
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
            return;
        default:
            error("Unknown special effect applicable to content type");
    }

    int obj_i = closest_object_w_special_effect(level);
    if (obj_i == -1) {
        return;
    }

    effect = level->object[obj_i].special_effect;

    switch (effect) {
        case SPECIAL_EFFECT_NONE:
            break;
        case SPECIAL_EFFECT_SCORE_1:
            level->objects_count--;
            level->object[obj_i] = level->object[level->objects_count];
            start_flash_effect(TREASURE_PICKUP_FLASH_DURATION, &color_white);
            level->score += 1;
            break;
        case SPECIAL_EFFECT_SCORE_2:
            level->objects_count--;
            level->object[obj_i] = level->object[level->objects_count];
            start_flash_effect(TREASURE_PICKUP_FLASH_DURATION, &color_white);
            level->score += 2;
            break;
        case SPECIAL_EFFECT_SCORE_3:
            level->objects_count--;
            level->object[obj_i] = level->object[level->objects_count];
            start_flash_effect(TREASURE_PICKUP_FLASH_DURATION, &color_white);
            level->score += 4;
            break;
        case SPECIAL_EFFECT_SCORE_4:
            level->objects_count--;
            level->object[obj_i] = level->object[level->objects_count];
            start_flash_effect(TREASURE_PICKUP_FLASH_DURATION, &color_white);
            level->score += 10;
            break;
        case SPECIAL_EFFECT_KEY_1:
            level->objects_count--;
            level->object[obj_i] = level->object[level->objects_count];
            start_flash_effect(TREASURE_PICKUP_FLASH_DURATION, &color_white);
            level->key_1 = true;
            break;
        case SPECIAL_EFFECT_KEY_2:
            level->objects_count--;
            level->object[obj_i] = level->object[level->objects_count];
            start_flash_effect(TREASURE_PICKUP_FLASH_DURATION, &color_white);
            level->key_2 = true;
            break;
        case SPECIAL_EFFECT_SMALL_HEALTH:
            if (level->life < 100) {
                level->objects_count--;
                level->object[obj_i] = level->object[level->objects_count];
                start_flash_effect(TREASURE_PICKUP_FLASH_DURATION, &color_white);
                level->life = MIN(level->life + 4, 100);
            }
            break;
        case SPECIAL_EFFECT_MEDIUM_HEALTH:
            if (level->life < 100) {
                level->objects_count--;
                level->object[obj_i] = level->object[level->objects_count];
                start_flash_effect(TREASURE_PICKUP_FLASH_DURATION, &color_white);
                level->life = MIN(level->life + 10, 100);
            }
            break;
        case SPECIAL_EFFECT_LARGE_HEALTH:
            if (level->life < 100) {
                level->objects_count--;
                level->object[obj_i] = level->object[level->objects_count];
                start_flash_effect(TREASURE_PICKUP_FLASH_DURATION, &color_white);
                level->life = MIN(level->life + 30, 100);
            }
            break;
        case SPECIAL_EFFECT_AMMO:
            if (level->ammo < MAX_AMMO) {
                level->objects_count--;
                level->object[obj_i] = level->object[level->objects_count];
                start_flash_effect(TREASURE_PICKUP_FLASH_DURATION, &color_white);
                level->ammo = MIN(level->ammo + 8, MAX_AMMO);
            }
            break;
        case SPECIAL_EFFECT_MSG:
            level->objects_count--;
            level->object[obj_i] = level->object[level->objects_count];
            start_flash_effect(TREASURE_PICKUP_FLASH_DURATION, &color_white);
            level->ammo = MIN(level->ammo + 8, MAX_AMMO);
            make_weapon_available(level, 2);
            break;
        case SPECIAL_EFFECT_MINIGUN:
            level->objects_count--;
            level->object[obj_i] = level->object[level->objects_count];
            start_flash_effect(TREASURE_PICKUP_FLASH_DURATION, &color_white);
            level->ammo = MIN(level->ammo + 8, MAX_AMMO);
            make_weapon_available(level, 3);
            break;
        default:
            error("Unknown special effect applicable to object");
    }
}

bool shooting_state(unsigned int * step) {
    if (shooting_ticks) {
        *step = shooting_ticks;
        return true;
    } else {
        return false;
    }
}

void shooting_start_action(const level_t * level) {
    if (shooting_ticks == 0) {
        double weapon_switch_percentage;
        bool weapon_switching = weapon_transition(&weapon_switch_percentage);

        if (!weapon_switching) {
            if (level->weapon == 3) {
                shooting_ticks = SHOOTING_MINIGUN_ANIMATION_SPEED;
            } else {
                shooting_ticks = SHOOTING_ANIMATION_SPEED;
            }
        }
    }
}

static bool position_blocked(
    const level_t * level,
    unsigned int rounded_x, unsigned int rounded_y,
    bool allow_enter_door, unsigned int door_x, unsigned int door_y
) {
    unsigned char content_type = level->content_type[rounded_x + rounded_y * level->width];
    if (content_type == CONTENT_TYPE_WALL || content_type == CONTENT_TYPE_DOOR || (content_type == CONTENT_TYPE_DOOR_OPEN && !allow_enter_door && door_x == rounded_x && door_y == rounded_y)) {
        return true;
    }

    for (unsigned int i = 0; i < level->objects_count; ++i) {
        if (rounded_x == (unsigned int)(level->object[i].x + 0.5) && rounded_y == (unsigned int)(level->object[i].y + 0.5)) {
            if (level->object[i].type == OBJECT_TYPE_BLOCKING) {
                return true;
            }
        }
    }

    for (unsigned int i = 0; i < level->enemies_count; ++i) {
        if (rounded_x == (unsigned int)(level->enemy[i].x + 0.5) && rounded_y == (unsigned int)(level->enemy[i].y + 0.5)) {
            if (level->enemy[i].state != ENEMY_STATE_DEAD) {
                return true;
            }
        }
    }

    return false;
}

static void move_player2(level_t * level, double x_change, double y_change) {
    double new_x = level->observer_x + x_change;
    double new_y = level->observer_y + y_change;
    unsigned int rounded_x = (unsigned int)(new_x + 0.5);
    unsigned int rounded_y = (unsigned int)(new_y + 0.5);
    unsigned int door_x = 0;
    unsigned int door_y = 0;
    double percentage_open;
    bool door_is_opening = opening_door_transition(&percentage_open, &door_x, &door_y);
    bool allow_enter_door = !door_is_opening || (door_is_opening && percentage_open > 0.75);

    rounded_x = (unsigned int)(new_x + 0.5 + 0.22);
    if (position_blocked(level, rounded_x, rounded_y, allow_enter_door, door_x, door_y)) {
        return;
    }

    rounded_x = (unsigned int)(new_x + 0.5 - 0.22);
    if (position_blocked(level, rounded_x, rounded_y, allow_enter_door, door_x, door_y)) {
        return;
    }

    rounded_x = (unsigned int)(new_x + 0.5);
    rounded_y = (unsigned int)(new_y + 0.5 + 0.22);
    if (position_blocked(level, rounded_x, rounded_y, allow_enter_door, door_x, door_y)) {
        return;
    }

    rounded_y = (unsigned int)(new_y + 0.5 - 0.22);
    if (position_blocked(level, rounded_x, rounded_y, allow_enter_door, door_x, door_y)) {
        return;
    }

    level->observer_x = new_x;
    level->observer_y = new_y;
}

void move_player(level_t * level, double x_change, double y_change) {
    move_player2(level, x_change, 0.0);
    move_player2(level, 0.0, y_change);
}

void init_animations() {
    door_open_ticks = 0;
    flash_effect_ticks = 0;
    shooting_ticks = 0;
    weapon_transition_ticks = 0;
}
