#ifndef GLOBAL_H
#define GLOBAL_H

#include <SFML/Graphics.h>
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

typedef struct  __attribute__((__packed__)) __level_ {
    pixel_t ceil_color;
    pixel_t floor_color;
    unsigned int width;
    unsigned int height;
    double observer_x;
    double observer_y;
    double observer_angle;
    unsigned char contents[];
} level_t;

#define MAX(X, Y) (X > Y ? X : Y)
#define MIN(X, Y) (X < Y ? X : Y)

#define RADIAN_CONSTANT 57.2957795131 // equals 180 / Pi

#define KEYS_PRESSED_BUFFER_SIZE 16

#define VIEWPORT_WIDTH 912
#define VIEWPORT_HEIGHT 456
#define MAX_FPS 60
#define FIELD_OF_VIEW 72.0 // degrees

#define SPRITE_WIDTH 64
#define SPRITE_HEIGHT 64

#define MAX_FILE_NAME_SIZ 1024
#define MAX_LEVEL_WALL_TYPES 8

#define MAX_LEVEL_FILE_SIZE (1024 * 1024)
#define MAX_SPRITES_FILE_SIZE (4 * 1024 * 1024)

#define TEXTURE_PACK_NAME "wall_textures"
#define TEXTURE_PACK_WIDTH 6
#define TEXTURE_PACK_HEIGHT 19
#define SAFETY_BARRIER_BLOCK (0 + 7 * TEXTURE_PACK_WIDTH)

// Voodoo:
#define MOVEMENT_CONSTANT 0.095
#define RAY_STEP_CONSTANT 0.0078125

void read_sprite_pack(
    pixel_t * dst, const char * filename,
    unsigned int pack_width, unsigned int pack_height
);

// One sprite can be subdivided into other sprites
void read_subsprite(
    pixel_t * restrict dst, const pixel_t * restrict src,
    unsigned int pack_width, unsigned int pack_height,
    unsigned int pack_x, unsigned int pack_y
);

level_t * read_level_info(const char * filename);

void error(const char * s);
void error_w_line(const char * s, unsigned int line);

#endif
