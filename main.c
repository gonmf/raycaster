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

#define RADIAN_CONSTANT 57.2957795131 // equals 180 / Pi

#define MAZE_EMPTY 0 
#define MAZE_BLOCK 1
#define KEYS_PRESSED_BUFFER_SIZE 16

#define VIEWPORT_WIDTH 800
#define VIEWPORT_HEIGHT 600
#define MAX_FPS 60
#define FIELD_OF_VIEW 72.0 // degrees

// Voodoo:
#define MOVEMENT_CONSTANT 0.095
#define RAY_STEP_CONSTANT 0.0078125

static pixel_t bg_buffer[VIEWPORT_WIDTH * VIEWPORT_HEIGHT];
static pixel_t fg_buffer[VIEWPORT_WIDTH * VIEWPORT_HEIGHT];

static sfKeyCode keys_pressed[KEYS_PRESSED_BUFFER_SIZE];
static unsigned char keys_pressed_count;
static maze_t * maze;

static maze_t * create_maze(unsigned int width, unsigned int height) {
    unsigned int total_size = sizeof(int) * 2 + sizeof(double) * 3 + width * height;

    maze_t * ret = (maze_t *)calloc(total_size, 1);

    ret->width = width;
    ret->height = height;
    ret->observer_x = width / 2.0;
    ret->observer_y = height / 2.0;
    ret->observer_angle = 30.0;

    // Surround maze with blocks for performance
    for (unsigned int i = 0; i < width; ++i) {
        ret->contents[i] = MAZE_BLOCK;
        ret->contents[i + (height - 1) * width] = MAZE_BLOCK;
    }
    for (unsigned int i = 0; i < height; ++i) {
        ret->contents[i * width] = MAZE_BLOCK;
        ret->contents[width - 1 + i * width] = MAZE_BLOCK;
    }

    return ret;
}

static double horizon_distance(unsigned int x, unsigned int y) {
    if (x < VIEWPORT_WIDTH / 2) {
        x = VIEWPORT_WIDTH / 2 - x;
    } else {
        x = x - VIEWPORT_WIDTH / 2;
    }

    double w2 = (double)(VIEWPORT_WIDTH / 2);
    double h2 = (double)(VIEWPORT_HEIGHT / 2);
    double max = sqrt(w2 * w2 + h2 * h2);
    double dst_to_max = 1.0 - sqrt(((double)x) * x + ((double)y) * y) / max;
    return (dst_to_max + 1.0) / 2.0;
}

static void fill_in_background() {
    for (unsigned int y = 0; y < VIEWPORT_HEIGHT; ++y) {
        for (unsigned int x = 0; x < VIEWPORT_WIDTH; ++x) {
            double r, g, b, factor;

            if (y < VIEWPORT_HEIGHT / 2) { // sky
                factor = horizon_distance(x, y);

                r = 113.0 * factor;
                g = 113.0 * factor;
                b = 210.0 * factor;
            } else { // floor
                factor = horizon_distance(x, VIEWPORT_HEIGHT - y - 1);
                r = 113.0 * factor;
                g = 112.0 * factor;
                b = 107.0 * factor;
            }

            pixel_t * pixel = &(bg_buffer[x + y * VIEWPORT_WIDTH]);
            pixel->red = (unsigned char)r;
            pixel->green = (unsigned char)g;
            pixel->blue = (unsigned char)b;
        }
    }
}

static void randomize_maze() {
    for (unsigned int y = 0; y < maze->height; ++y) {
        for (unsigned int x = 0; x < maze->width; ++x) {
            double r = ((double)rand()) / RAND_MAX;

            if (r < 0.1) {
                maze->contents[x + y * maze->width] = MAZE_BLOCK;
            }
        }
    }

    // make sure starting observer position is not blocked randomly
    unsigned int x = (unsigned int)(maze->observer_x + 0.5);
    unsigned int y = (unsigned int)(maze->observer_y + 0.5);
    maze->contents[x + y * maze->width] = MAZE_EMPTY;
}

static double fit_angle(double d) {
    if (d < 0.0) {
        d += 360.0;
    } else if (d >= 360.0) {
        d -= 360.0;
    }
    return d;
}

static unsigned int cast_ray(const maze_t * maze, double angle) {
    double angle_radians = angle / RADIAN_CONSTANT;
    double x_change = sin(angle_radians) * RAY_STEP_CONSTANT;
    double y_change = cos(angle_radians) * RAY_STEP_CONSTANT;
    double curr_x = maze->observer_x;
    double curr_y = maze->observer_y;

    unsigned int steps = 0;
    while(1) {
        steps += 1;
        curr_x += x_change;
        curr_y += y_change;

        unsigned int rounded_x = (unsigned int)(curr_x + 0.5);
        unsigned int rounded_y = (unsigned int)(curr_y + 0.5);

        if (maze->contents[rounded_x + rounded_y * maze->width] == MAZE_BLOCK) {
            return steps;
        }
    }

    return 0;
}

static unsigned char block_color(unsigned char val, double factor) {
    return (unsigned char)(val * factor);
}

static void paint_maze(const maze_t * maze) {
    double min_angle = fit_angle(maze->observer_angle - FIELD_OF_VIEW / 2.0);
    unsigned int min_distance = 0;

    for (unsigned int x = 0 ; x < VIEWPORT_WIDTH; ++x) {
        double x_angle = (((double)x) / VIEWPORT_WIDTH);
        x_angle = fit_angle(min_angle + FIELD_OF_VIEW * x_angle);


        unsigned int distance = cast_ray(maze, x_angle);
        if (min_distance == 0 || min_distance > distance) {
            min_distance = distance;
        }

        double block_size;
        block_size = 3.0 / (distance / 100.0);
        if (block_size > 1.0) {
            block_size = 1.0;
        }

        unsigned int mid = VIEWPORT_HEIGHT / 2;
        for (unsigned int y = 0; y < (unsigned int)(mid * block_size); ++y) {
            pixel_t * pixel;

            pixel = &(fg_buffer[x + (y + mid) * VIEWPORT_WIDTH]);
            pixel->red = block_color(61, block_size);
            pixel->green = block_color(40, block_size);
            pixel->blue = block_color(23, block_size);
            pixel = &(fg_buffer[x + (mid - y) * VIEWPORT_WIDTH]);
            pixel->red = block_color(61, block_size);
            pixel->green = block_color(40, block_size);
            pixel->blue = block_color(23, block_size);
        }
    }
}

static void copy_bg_to_fg() {
    memcpy(fg_buffer, bg_buffer, sizeof(pixel_t) * VIEWPORT_WIDTH * VIEWPORT_HEIGHT);
}

static void add_key_pressed(sfKeyCode code) {
    if (keys_pressed_count == KEYS_PRESSED_BUFFER_SIZE) {
        return;
    }

    for (int i = 0; i < keys_pressed_count; ++i) {
        if (keys_pressed[i] == code) {
            return;
        }
    }

    keys_pressed[keys_pressed_count] = code;
    keys_pressed_count += 1;
}

static void remove_key_pressed(sfKeyCode code) {
    for (int i = 0; i < keys_pressed_count; ++i) {
        if (keys_pressed[i] == code) {
            keys_pressed[i] = keys_pressed[keys_pressed_count - 1];
            keys_pressed_count -= 1;
            return;
        }
    }
}

static int key_is_pressed(sfKeyCode code) {
    for (int i = 0; i < keys_pressed_count; ++i) {
        if (keys_pressed[i] == code) {
            return 1;
        }
    }

    return 0;
}

static void move_observer(maze_t * maze, double x_change, double y_change) {
    double new_x = maze->observer_x + x_change;
    double new_y = maze->observer_y + y_change;

    unsigned int rounded_x = (unsigned int)(new_x + 0.5);
    unsigned int rounded_y = (unsigned int)(new_y + 0.5);

    if (rounded_x >= maze->width || rounded_y >= maze->height) {
        return;
    }

    if (maze->contents[rounded_x + rounded_y * maze->width] == MAZE_BLOCK) {
        return;
    }

    maze->observer_x = new_x;
    maze->observer_y = new_y;
}

static void rotate_observer(maze_t * maze, double degrees) {
    maze->observer_angle = fit_angle(maze->observer_angle + degrees);
}

static void update_observer_state() {
    if (key_is_pressed(sfKeyW)) {
        double angle_radians = maze->observer_angle / RADIAN_CONSTANT;
        double x_change = sin(angle_radians) * MOVEMENT_CONSTANT;
        double y_change = cos(angle_radians) * MOVEMENT_CONSTANT;

        move_observer(maze, x_change, y_change);
    }
    if (key_is_pressed(sfKeyS)) {
        double angle_radians = maze->observer_angle / RADIAN_CONSTANT;
        double x_change = sin(angle_radians) * MOVEMENT_CONSTANT;
        double y_change = cos(angle_radians) * MOVEMENT_CONSTANT;

        move_observer(maze, -x_change, -y_change);
    }
    if (key_is_pressed(sfKeyQ)) {
        double angle_radians = fit_angle(maze->observer_angle + 270.0) / RADIAN_CONSTANT;
        double x_change = sin(angle_radians) * MOVEMENT_CONSTANT;
        double y_change = cos(angle_radians) * MOVEMENT_CONSTANT;

        move_observer(maze, x_change, y_change);
    }
    if (key_is_pressed(sfKeyE)) {
        double angle_radians = fit_angle(maze->observer_angle + 90.0) / RADIAN_CONSTANT;
        double x_change = sin(angle_radians) * MOVEMENT_CONSTANT;
        double y_change = cos(angle_radians) * MOVEMENT_CONSTANT;

        move_observer(maze, x_change, y_change);
    }
    if (key_is_pressed(sfKeyA)) {
        rotate_observer(maze, -1.8);
    }
    if (key_is_pressed(sfKeyD)) {
        rotate_observer(maze, 1.8);
    }
}

static void main_render_loop() {
    fill_in_background();

    sfVideoMode videoMode;
    videoMode.width = VIEWPORT_WIDTH;
    videoMode.height = VIEWPORT_HEIGHT;
    videoMode.bitsPerPixel = 32;

    sfRenderWindow * window = sfRenderWindow_create(videoMode, "Raycaster", sfResize | sfClose, NULL);
    sfRenderWindow_setFramerateLimit(window, MAX_FPS);

    sfTexture * texture = sfTexture_create(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    sfSprite * sprite = sfSprite_create();
    sfSprite_setTexture(sprite, texture, sfFalse);

    sfColor bgColor;
    memset(&bgColor, 0, sizeof(bgColor));

    sfEvent event;
    sfRenderStates renderStates;
    renderStates.blendMode = sfBlendNone;
    renderStates.transform = sfTransform_Identity;
    renderStates.texture = texture;
    renderStates.shader = NULL;

    while (sfRenderWindow_isOpen(window)) {
        update_observer_state();

        copy_bg_to_fg();
        paint_maze(maze);

        while (sfRenderWindow_pollEvent(window, &event) == sfTrue) {
            if (event.type == sfEvtClosed) {
                sfRenderWindow_close(window);
            } else if (event.type == sfEvtKeyPressed) {
                sfKeyCode code = ((sfKeyEvent *)&event)->code;

                if (code == sfKeyEscape) {
                    sfRenderWindow_close(window);
                }

                add_key_pressed(code);
            } else if (event.type == sfEvtKeyReleased) {
                sfKeyCode code = ((sfKeyEvent *)&event)->code;

                remove_key_pressed(code);
            }
        }

        sfTexture_updateFromPixels(texture, (sfUint8 *)fg_buffer, VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 0, 0);
        sfRenderWindow_drawSprite(window, sprite, &renderStates);
        sfRenderWindow_display(window);
    }
}

int main() {
    srand((unsigned int)getpid());

    maze = create_maze(20, 20);
    randomize_maze();

    main_render_loop();
    return EXIT_SUCCESS;
}
