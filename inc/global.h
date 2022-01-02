#ifndef GLOBAL_H
#define GLOBAL_H

#define _POSIX_C_SOURCE 200809L

#include <SDL2/SDL.h>

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

#define true 1
#define false 0
typedef char bool;

typedef struct  __attribute__((__packed__)) __pixel_ {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha; // not used but present for performance
} pixel_t;

typedef struct __attribute__((__packed__)) __enemy_ {
    unsigned char type;
    unsigned char life;
    unsigned char strategic_state;
    double moving_dir_x;
    double moving_dir_y;
    // Animation state
    unsigned char state;
    unsigned char animation_step;
    double x;
    double y;
    double angle;
} enemy_t;

typedef struct __attribute__((__packed__)) __object_ {
    unsigned char type;
    unsigned char texture;
    unsigned char special_effect;
    bool revealed;
    double x;
    double y;
} object_t;

typedef struct __level_ {
    pixel_t ceil_color;
    pixel_t floor_color;
    unsigned int width;
    unsigned int height;
    unsigned char door_open_texture;

    double observer_x;
    double observer_y;
    double observer_angle;
    double observer_angle2;
    unsigned char * content_type;
    unsigned char * texture;
    unsigned char * special_effects;
    bool * map_revealed;
    unsigned int objects_count;
    object_t * object;
    unsigned int enemies_count;
    enemy_t * enemy;

    unsigned int score;
    unsigned int level_start_score;
    unsigned char life;
    unsigned char lives;
    bool key_1;
    bool key_2;
    unsigned char weapon;
    unsigned char weapons_available;
    unsigned int ammo;
    unsigned int level_nr;
} level_t;

typedef struct __sprite_pack_ {
    char name[16];
    unsigned char width;
    unsigned char height;
    unsigned int sprite_size;
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
#define SPECIAL_EFFECT_MSG 11
#define SPECIAL_EFFECT_MINIGUN 12
#define SPECIAL_EFFECT_SMALL_HEALTH 13
#define SPECIAL_EFFECT_MEDIUM_HEALTH 14
#define SPECIAL_EFFECT_LARGE_HEALTH 15

#define ENEMY_STATE_STILL 0
#define ENEMY_STATE_MOVING 1
#define ENEMY_STATE_SHOT 2
#define ENEMY_STATE_ALERT 3
#define ENEMY_STATE_SHOOTING 4
#define ENEMY_STATE_DYING 5
#define ENEMY_STATE_DEAD 6

#define ENEMY_STRATEGIC_STATE_WAITING 0
#define ENEMY_STRATEGIC_STATE_ALERTED 1
#define ENEMY_STRATEGIC_STATE_ENGAGED 2

#define WEAPON_SPRITE_MULTIPLIER 4

#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

#define RADIAN_CONSTANT 57.2957795131 // equals 180 / Pi

#define KEYS_PRESSED_BUFFER_SIZE 16

#define VIEWPORT_WIDTH 800
#define VIEWPORT_HEIGHT 400
#define UI_PADDING 10
#define MAX_FPS 120
#define GAME_LOGIC_CYCLE_STEP 8 // ms, 8ms = 125 updates / sec
#define FIELD_OF_VIEW 72 // degrees
#define MAP_BLOCK_SIZE 8

#define MIN_LEVEL_SIZE 5
#define MAX_LEVEL_SIZE 128

#define MAX_FILE_NAME_SIZ (4 * 1024) // 4KiB
#define MAX_FILE_SIZE (8 * 1024 * 1024) // 8MiB

#define CLOSED_DOOR_TEXTURE (2 + 13 * 6)
#define OPEN_DOOR_TEXTURE (4 + 13 * 6)
#define LOCKED_DOOR_1_TEXTURE (2 + 14 * 6)
#define LOCKED_DOOR_2_TEXTURE (3 + 14 * 6)
#define SAFETY_WALL_TEXTURE (0 + 0 * 6)

#define DOOR_KEY_1_TEXTURE (2 + 4 * 5)
#define DOOR_KEY_2_TEXTURE (3 + 4 * 5)
#define TREASURE_1_TEXTURE (1 + 6 * 5)
#define TREASURE_2_TEXTURE (2 + 6 * 5)
#define TREASURE_3_TEXTURE (3 + 6 * 5)
#define TREASURE_4_TEXTURE (4 + 6 * 5)
#define SMALL_HEALTH_BONUS_TEXTURE (0 + 5 * 5)
#define MEDIUM_HEALTH_BONUS_TEXTURE (1 + 5 * 5)
#define LARGE_HEALTH_BONUS_TEXTURE (2 + 5 * 5)
#define SMALL_AMMO_TEXTURE (3 + 5 * 5)
#define SMG_TEXTURE (4 + 5 * 5)
#define MINIGUN_TEXTURE (0 + 6 * 5)

#define ENEMY_SHOT_TEXTURE (7 + 5 * 8)
#define ENEMY_ALERT_TEXTURE (0 + 6 * 8)
#define ENEMY_MOVING_TEXTURE (0 + 1 * 8)
#define ENEMY_SHOOTING_TEXTURE (1 + 6 * 8)
#define ENEMY_DYING_TEXTURE (0 + 5 * 8)
#define ENEMY_DEAD_TEXTURE (4 + 5 * 8)

#define DOOR_OPEN_SPEED 120
#define WEAPON_SWITCH_SPEED 80
#define TREASURE_PICKUP_FLASH_DURATION 8
#define PLAYER_SHOT_FLASH_DURATION 8
#define GAME_OVER_ANIMATION_SPEED 1.5
#define GAME_ENTER_EXIT_ANIMATION_SPEED 1.0
#define ENEMY_ALERT_PROXIMITY 10

// Voodoo:
#define MOVEMENT_CONSTANT 0.07
#define ROTATION_CONSTANT 4.0

// In rendering blocks we need very fine ray steps to catch the edges.
// To render simple objects, usually even round and with transparent sides,
// a much coarser step can be used for performance.
#define FINE_RAY_STEP_CONSTANT 0.0078125
#define ROUGH_RAY_STEP_CONSTANT 0.05

#define SHOOTING_ANIMATION_SPEED 50
#define SHOOTING_MINIGUN_ANIMATION_SPEED 35
#define SHOOTING_ANIMATION_PARTS 5
#define SHOOTING_ACTIVATION_PART 3
#define ENEMY_SHOT_ANIMATION_SPEED 25
#define ENEMY_DYING_ANIMATION_PARTS 4
#define ENEMY_DYING_ANIMATION_SPEED 48
#define ENEMY_SHOOTING_ANIMATION_PARTS 2
#define ENEMY_SHOOTING_ANIMATION_SPEED 70
#define ENEMY_SHOOTING_MSG_ANIMATION_SPEED 40
#define ENEMY_SHOOTING_ACTIVATION_PART 1
#define ENEMY_SHOOTING_ALERT_DELAY 60
#define ENEMY_MOVING_ANIMATION_PARTS 4
#define ENEMY_MOVING_ANIMATION_SPEED 100

#define ENEMY_SHOOTING_MAX_DISTANCE 8
#define ENEMY_VIEWING_DISTANCE 10
#define ENEMY_FIELD_OF_VIEW 70

#define SAVE_FILES_COUNT 8
#define SAVE_FILE_NAME_SIZ 30

#define MAX_AMMO 99

// raycaster.c
extern sprite_pack_t * wall_textures;
extern sprite_pack_t * objects_sprites;
extern sprite_pack_t * font_sprites;
extern pixel_t * fg_buffer;
void load_textures();

// color.c
extern pixel_t color_white;
extern pixel_t color_gray;
extern pixel_t color_black;
extern pixel_t color_red;
extern pixel_t color_dark_red;
extern pixel_t color_blue;
extern pixel_t color_dark_blue;
extern pixel_t color_cyan;
extern pixel_t color_gold;
extern pixel_t color_ui_bg;
extern pixel_t color_ui_bg_dark;
extern pixel_t color_ui_bg_light;

// sprites.c
void read_sprite_pack(sprite_pack_t * pack, const char * pack_name);

// map_format.c
level_t * read_level_info(unsigned int level_nr);

// utils.c
bool random_one_in(unsigned int n);
void error(const char * s);
void error_w_line(const char * s, unsigned int line);

// keyboard.c
void add_key_pressed(SDL_Keycode code);
void remove_key_pressed(SDL_Keycode code);
bool key_is_pressed(SDL_Keycode code);
bool player_moving();
void clear_keys_pressed();
bool is_mouse_left_key_pressed();
void set_mouse_left_key_pressed(bool value);

// file_io.c
unsigned int file_read(char * dst, unsigned int max_size, const char * filename);

// math.c
double fit_angle(double d);
int fit_angle_int(int d);
double distance(double a_x, double a_y, double b_x, double b_y);

// string.c
bool start_with(const char * s, const char * prefix);

// window.c
void set_cursor_visible(bool visible);
void window_start();
void window_close();
void window_update_pixels(const pixel_t * pixels);

// raycaster.c
void init_fish_eye_table();
void paint_scene(level_t * level, bool trigger_shot);
void init_raycaster(const level_t * level);
void invalidate_objects_cache();

// actions.c
void transition_step(level_t * level, bool * trigger_shot);
void start_weapon_transition(unsigned char weapon_nr);
bool weapon_transition(double * percentage);
bool opening_door_transition(double * percentage_open, unsigned int * door_x, unsigned int * door_y);
void open_door_in_front(level_t * level);
bool short_flash_effect(double * percentage, pixel_t * color);
void start_flash_effect(unsigned int duration, pixel_t * color);
void apply_special_effect(level_t * level, bool * exit_found);
bool shooting_state(unsigned int * step);
void shooting_start_action(const level_t * level);
void move_player(level_t * level, double x_change, double y_change);
void init_animations();

// ui.c
void paint_map(const level_t * level);

// color.c
pixel_t shading(pixel_t min_color, pixel_t max_color, double factor);
void scene_shading(pixel_t color, double factor);
pixel_t darken_shading(pixel_t color, double factor);
void init_base_colors();

// game.c
void make_weapon_available(level_t * level, unsigned char weapon_nr);
void switch_weapon(level_t * level, unsigned char new_weapon_nr);
void alert_enemies_in_proximity(const level_t * level, unsigned int distance);
void hit_enemy(level_t * level, enemy_t * enemy, double distance);
void update_enemies_state(level_t * level);
void init_game_buffers(const level_t * level);

// font.c
void screen_write_scaled(const char * str, unsigned int screen_x, unsigned int screen_y, unsigned int scale, double factor);
void screen_write(const char * s, unsigned int x, unsigned int y);

// save_state.c
void save_game_state(unsigned char idx, const level_t * level);
bool read_game_state_name(unsigned char idx, char * s);
level_t * load_game_state(unsigned char idx);

// options.c
void load_user_options();
bool is_fullscreen();
void toggle_fullscreen();
bool is_look_up_down();
void toggle_look_up_down();
bool is_show_fps();
void toggle_show_fps();
bool is_invert_mouse();
void toggle_invert_mouse();
unsigned char get_mouse_sensibility();
void increase_mouse_sensibility();

// timing.c
bool limit_fps(long unsigned int us_time);
unsigned int game_logic_cycles(long unsigned int ms_time);

#endif
