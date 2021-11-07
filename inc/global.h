#ifndef GLOBAL_H
#define GLOBAL_H

#define _POSIX_C_SOURCE 200809L

#include <SFML/Graphics.h>
#include <inttypes.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

typedef struct  __attribute__((__packed__)) __pixel_ {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha; // not used but present for performance
} pixel_t;

typedef struct __attribute__((__packed__)) __enemy_ {
    unsigned char type;
    double x;
    double y;
    double angle;
    double life;
} enemy_t;

typedef struct  __level_ {
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
    unsigned char door_open_texture;
    unsigned int enemies_count; // not used yet
    enemy_t * enemy; // not used yet
} level_t;

#define SPRITE_WIDTH 64
#define SPRITE_HEIGHT 64

typedef struct __sprite_pack_ {
    char name[16];
    unsigned char width;
    unsigned char height;
    pixel_t ** sprites; // [columns x rows][pixels_x x pixels_y]
} sprite_pack_t;

#define true sfTrue
#define false sfFalse
typedef sfBool bool;

#define EQUAL_PIXEL(A,B) (A.red == B.red && A.green == B.green && A.blue == B.blue && A.alpha == B.alpha)

#define CONTENT_TYPE_EMPTY 0
#define CONTENT_TYPE_WALL 1
#define CONTENT_TYPE_DOOR 2
#define CONTENT_TYPE_DOOR_OPEN 3
#define CONTENT_TYPE_OBJECT 4

#define SPECIAL_EFFECT_NONE 0
#define SPECIAL_EFFECT_LEVEL_END 1
#define SPECIAL_EFFECT_SCORE_1 2
#define SPECIAL_EFFECT_SCORE_2 3
#define SPECIAL_EFFECT_SCORE_3 4
#define SPECIAL_EFFECT_SCORE_4 5

#define MAX(X, Y) (X > Y ? X : Y)
#define MIN(X, Y) (X < Y ? X : Y)

#define RADIAN_CONSTANT 57.2957795131 // equals 180 / Pi

#define KEYS_PRESSED_BUFFER_SIZE 16

#define VIEWPORT_WIDTH 1200
#define VIEWPORT_HEIGHT 600
#define UI_BORDER 8
#define UI_BOTTOM 128
#define WINDOW_TOTAL_WIDTH (VIEWPORT_WIDTH + UI_BORDER + UI_BORDER)
#define WINDOW_TOTAL_HEIGHT (VIEWPORT_HEIGHT + UI_BORDER + UI_BOTTOM)
#define UI_BG_COLOR "0,111,112"
#define MAX_FPS 120
#define FIELD_OF_VIEW 72 // degrees

#define MAX_LEVEL_SIZE 256

#define MAX_FILE_NAME_SIZ (4 * 1024) // 4KiB
#define MAX_FILE_SIZE (8 * 1024 * 1024) // 8MiB

#define TEXTURE_PACK_NAME "wall_textures"
#define OBJECTS_PACK_NAME "objects"
#define SAFETY_WALL_TEXTURE 7

#define DOOR_OPEN_SPEED 80
#define TREASURE_PICKUP_FLASH_DURATION 10
// Voodoo:
#define MOVEMENT_CONSTANT 0.07
#define HORIZONTAL_ROTATION_CONSTANT 16.0
#define VERTICAL_ROTATION_CONSTANT 32.0

// In rendering blocks we need very fine ray steps to catch the edges.
// To render simple objects, usually even round and with transparent sides,
// a much coarser step can be used for performance.
#define FINE_RAY_STEP_CONSTANT 0.0078125
#define ROUGH_RAY_STEP_CONSTANT 0.1

extern sprite_pack_t * wall_textures;
extern sprite_pack_t * objects_sprites;
extern pixel_t fg_buffer[VIEWPORT_WIDTH * VIEWPORT_HEIGHT];

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
void brighten_scene(double factor);
void darken_scene(double factor);
void init_fish_eye_table();
void load_textures();
void paint_scene(const level_t * level);

// actions.c
bool transition_step();
bool opening_door_transition(double * percentage_open, unsigned int * door_x, unsigned int * door_y);
bool open_door_in_front(level_t * level);
bool short_flash_effect(double * percentage);
void start_flash_effect(unsigned int duration);
bool apply_special_effect(level_t * level, bool * exit_found);

#endif
