#ifndef GLOBAL_H
#define GLOBAL_H

#define _POSIX_C_SOURCE 200809L

#include <SFML/Graphics.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "local.h"

#define true 1
#define false 0
typedef char bool;

typedef struct  __attribute__((__packed__)) __pixel_ {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha; // not used but present for performance
} pixel_t;

/*
typedef struct  __attribute__((__packed__)) __list_ {
    struct __list_ * next;
    double angle;
} list_t;
*/

typedef struct __attribute__((__packed__)) __enemy_ {
    unsigned char type;
    unsigned char life;
    unsigned char strategic_state;
    // double goal_x;
    // double goal_y;
    // list_t * moving_plan;
    // Animation state
    unsigned char state;
    unsigned char state_step;
    double x;
    double y;
    double angle;
} enemy_t;

typedef struct __attribute__((__packed__)) __object_ {
    unsigned char type;
    unsigned char texture;
    unsigned char special_effect;
    double x;
    double y;
} object_t;

typedef struct __level_ {
    pixel_t ceil_color;
    pixel_t floor_color;
    unsigned int width;
    unsigned int height;
    double observer_x;
    double observer_y;
    double observer_angle;
    double observer_angle2;
    unsigned int score;
    unsigned char * content_type;
    unsigned char * texture;
    unsigned char * special_effects;
    bool * map_revealed;
    unsigned char door_open_texture;
    unsigned int objects_count;
    object_t * object;
    unsigned int enemies_count;
    enemy_t * enemy;
    bool key_1;
    bool key_2;
    unsigned int ammo;
} level_t;

#define SPRITE_WIDTH 64
#define SPRITE_HEIGHT 64

typedef struct __sprite_pack_ {
    char name[16];
    unsigned char width;
    unsigned char height;
    pixel_t ** sprites; // [columns x rows][pixels_x x pixels_y]
} sprite_pack_t;

#define EQUAL_PIXEL(A,B) (A.red == B.red && A.green == B.green && A.blue == B.blue)

#define CONTENT_TYPE_EMPTY 0
#define CONTENT_TYPE_WALL 1
#define CONTENT_TYPE_DOOR 2
#define CONTENT_TYPE_DOOR_OPEN 3

#define OBJECT_TYPE_BLOCKING 0
#define OBJECT_TYPE_NON_BLOCK 1

#define SPECIAL_EFFECT_NONE 0
#define SPECIAL_EFFECT_LEVEL_END 1
#define SPECIAL_EFFECT_SCORE_1 2
#define SPECIAL_EFFECT_SCORE_2 3
#define SPECIAL_EFFECT_SCORE_3 4
#define SPECIAL_EFFECT_SCORE_4 5
#define SPECIAL_EFFECT_REQUIRES_KEY_1 6
#define SPECIAL_EFFECT_REQUIRES_KEY_2 7
#define SPECIAL_EFFECT_KEY_1 8
#define SPECIAL_EFFECT_KEY_2 9
#define SPECIAL_EFFECT_AMMO 10

#define ENEMY_STATE_STILL 0
#define ENEMY_STATE_MOVING 1
#define ENEMY_STATE_SHOT 2
#define ENEMY_STATE_SHOOTING 3
#define ENEMY_STATE_DYING 4
#define ENEMY_STATE_DEAD 5

#define ENEMY_STRATEGIC_STATE_WAITING 0
#define ENEMY_STRATEGIC_STATE_ALERTED 1
#define ENEMY_STRATEGIC_STATE_ENGAGED 2

#define UI_MULTIPLIER 4

#define MAX(X, Y) (X > Y ? X : Y)
#define MIN(X, Y) (X < Y ? X : Y)

#define RADIAN_CONSTANT 57.2957795131 // equals 180 / Pi

#define KEYS_PRESSED_BUFFER_SIZE 16

#define VIEWPORT_WIDTH 600
#define VIEWPORT_HEIGHT 300
#define UI_BORDER 8
#define UI_BOTTOM 128
#define WINDOW_TOTAL_WIDTH (VIEWPORT_WIDTH + UI_BORDER + UI_BORDER)
#define WINDOW_TOTAL_HEIGHT (VIEWPORT_HEIGHT + UI_BORDER + UI_BOTTOM)
#define UI_BG_COLOR "0,111,112"
#define MAX_FPS 120
#define FIELD_OF_VIEW 72 // degrees
#define MAP_BLOCK_SIZE 8

#define MIN_LEVEL_SIZE 5
#define MAX_LEVEL_SIZE 128

#define MAX_FILE_NAME_SIZ (4 * 1024) // 4KiB
#define MAX_FILE_SIZE (8 * 1024 * 1024) // 8MiB

#define TEXTURE_PACK_NAME "wall_textures"
#define OBJECTS_PACK_NAME "objects"
#define SAFETY_WALL_TEXTURE 7

#define DOOR_OPEN_SPEED 80
#define TREASURE_PICKUP_FLASH_DURATION 8

// Voodoo:
#define MOVEMENT_CONSTANT 0.07
#define HORIZONTAL_ROTATION_CONSTANT 16.0
#define VERTICAL_ROTATION_CONSTANT 32.0

// In rendering blocks we need very fine ray steps to catch the edges.
// To render simple objects, usually even round and with transparent sides,
// a much coarser step can be used for performance.
#define FINE_RAY_STEP_CONSTANT 0.0078125
#define ROUGH_RAY_STEP_CONSTANT 0.05

#define SHOOTING_ANIMATION_SPEED 50
#define SHOOTING_ANIMATION_PARTS 5
#define ENEMY_SHOT_ANIMATION_SPEED 20
#define ENEMY_DYING_ANIMATION_PARTS 4
#define ENEMY_DYING_ANIMATION_SPEED 48

// raycaster.c
extern sprite_pack_t * wall_textures;
extern sprite_pack_t * objects_sprites;
extern sprite_pack_t * enemy_sprites[5];
extern sprite_pack_t * weapons_sprites;
extern pixel_t fg_buffer[VIEWPORT_WIDTH * VIEWPORT_HEIGHT];

// color.c
extern pixel_t color_white;
extern pixel_t color_gray;
extern pixel_t color_black;
extern pixel_t color_red;
extern pixel_t color_blue;
extern pixel_t color_cyan;
extern pixel_t color_gold;
extern pixel_t color_ui_bg;
extern pixel_t color_ui_bg_dark;
extern pixel_t color_ui_bg_light;

// sprites.c
void read_sprite_pack(sprite_pack_t * pack, const char * pack_name);

// map_format.c
level_t * read_level_info(const char * filename);

// utils.c
void error(const char * s);
void error_w_line(const char * s, unsigned int line);

// keyboard.c
void add_key_pressed(sfKeyCode code);
void remove_key_pressed(sfKeyCode code);
bool key_is_pressed(sfKeyCode code);

// file_io.c
unsigned int file_read(char * dst, unsigned int max_size, const char * filename);

// math.c
double fit_angle(double d);
int fit_angle_int(int d);
double distance(double a_x, double a_y, double b_x, double b_y);

// string.c
bool start_with(const char * s, const char * prefix);

// window.c
void window_center_mouse();
void set_cursor_visible(bool visible);
void window_start();
void window_close();
bool window_is_open();
void window_update_pixels(const pixel_t * pixels, unsigned int width, unsigned int height,
    unsigned int offset_x, unsigned int offset_y);
void window_refresh();
bool window_poll_event(sfEvent * event);

// raycaster.c
void init_fish_eye_table();
void paint_scene(level_t * level);
void init_raycaster(const level_t * level);

// actions.c
void transition_step();
bool opening_door_transition(double * percentage_open, unsigned int * door_x, unsigned int * door_y);
void open_door_in_front(level_t * level);
bool short_flash_effect(double * percentage);
void start_flash_effect(unsigned int duration);
bool apply_special_effect(level_t * level, bool * exit_found);
bool shooting_state(level_t * level, unsigned int * step, bool * trigger_shot);
void shooting_start_action();

// ui.c
void paint_map(const level_t * level);

// color.c
pixel_t shading(pixel_t min_color, pixel_t max_color, double factor);
pixel_t brighten_shading(pixel_t color, double factor);
pixel_t darken_shading(pixel_t color, double factor);
void brighten_scene(double factor);
void darken_scene(double factor);
void init_base_colors();

// game.c
void hit_enemy(level_t * level, unsigned int enemy_i, double distance);

#endif
