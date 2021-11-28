#include "global.h"

void hit_enemy(level_t * level, unsigned int enemy_i, double distance) {
    enemy_t * enemy = &level->enemy[enemy_i];

    if (enemy->state == ENEMY_STATE_STILL || enemy->state == ENEMY_STATE_MOVING) {
        enemy->state = ENEMY_STATE_SHOT;
        enemy->state_step = ENEMY_SHOT_ANIMATION_SPEED;
    }

    if (enemy->state != ENEMY_STATE_DYING && enemy->state != ENEMY_STATE_DEAD) {
        unsigned char shot_power = MAX(MIN((unsigned int)(18 / distance) + 18, 50), 18);
        if (enemy->life > shot_power) {
            enemy->life -= shot_power;
            enemy->state = ENEMY_STATE_SHOT;
            enemy->state_step = ENEMY_SHOT_ANIMATION_SPEED;
        } else {
            enemy->life = 0;
            enemy->state = ENEMY_STATE_DYING;
            enemy->state_step = ENEMY_DYING_ANIMATION_SPEED;

            level->object[level->objects_count].type = OBJECT_TYPE_NON_BLOCK;
            level->object[level->objects_count].texture = 3 + 5 * 5;
            level->object[level->objects_count].special_effect = SPECIAL_EFFECT_AMMO;
            level->object[level->objects_count].x = enemy->x;
            level->object[level->objects_count].y = enemy->y;
            level->objects_count++;
        }
    }
}

