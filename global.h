#ifndef GLOBAL_H
#define GLOBAL_H

#include <SFML/Graphics.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>

typedef struct  __attribute__((__packed__)) __pixel_ {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha; // not used but present for performance
} pixel_t;

typedef struct  __attribute__((__packed__)) __maze_ {
    unsigned int width;
    unsigned int height;
    double observer_x;
    double observer_y;
    double observer_angle;
    unsigned char contents[];
} maze_t;

#define MAX(X, Y) (X > Y ? X : Y)
#define MIN(X, Y) (X < Y ? X : Y)

#define DEBUG 0

#define RADIAN_CONSTANT 57.2957795131 // equals 180 / Pi

#define MAZE_EMPTY 0
#define MAZE_BLOCK 1
#define KEYS_PRESSED_BUFFER_SIZE 16

#define VIEWPORT_WIDTH 800
#define VIEWPORT_HEIGHT 600
#define MAX_FPS 60
#define FIELD_OF_VIEW 72.0 // degrees

#define SPRITE_WIDTH 64
#define SPRITE_HEIGHT 64

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

#endif
