#include "global.h"

void hit_enemy(enemy_t * enemy, double distance) {
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
        }
        printf("Enemy shot! Life: %u/100 (Lost %u)\n", enemy->life, shot_power);
    }
}

