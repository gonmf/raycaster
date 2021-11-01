#include <SFML/Graphics.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>

typedef struct  __attribute__((__packed__)) __pixel_ {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
} pixel_t;

typedef struct  __attribute__((__packed__)) __image_ {
    unsigned int width;
    unsigned int height;
    pixel_t data[];
} image_t;

typedef struct  __attribute__((__packed__)) __maze_ {
    unsigned int width;
    unsigned int height;
    double observer_x;
    double observer_y;
    double observer_angle;
    unsigned char contents[];
} maze_t;

#define MAZE_EMPTY 0 
#define MAZE_BLOCK 1
#define RAY_STEP 128.0

static image_t * create_image(unsigned int width, unsigned int height) {
    unsigned int total_size = sizeof(int) * 2 + sizeof(pixel_t) * width * height;

    image_t * ret = (image_t *)calloc(total_size, 1);

    ret->width = width;
    ret->height = height;

    return ret;
}

static maze_t * create_maze(unsigned int width, unsigned int height) {
    unsigned int total_size = sizeof(int) * 2 + sizeof(double) * 3 + width * height;

    maze_t * ret = (maze_t *)calloc(total_size, 1);

    ret->width = width;
    ret->height = height;
    ret->observer_x = width / 2.0;
    ret->observer_y = height / 2.0;
    ret->observer_angle = 30.0;

    // Surround maze with blocks
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

static void dump_maze(const maze_t * maze) {
    printf("size=%u,%u\n", maze->width, maze->height);

    for (unsigned int y = 0; y < maze->height; ++y) {
        for (unsigned int x = 0; x < maze->width; ++x) {
            char chr = maze->contents[x + y * maze->width] == MAZE_EMPTY ? '_' : '#';
            if (floor(maze->observer_x + 0.5) == x && floor(maze->observer_y + 0.5) == y) {
                chr = '@';
            }
            printf(" %c", chr);
        }
        printf("\n");
    }
}

static double horizon_distance(const image_t * image, unsigned int x, unsigned int y) {
    if (x < image->width / 2) {
        x = image->width / 2 - x;
    } else {
        x = x - image->width / 2;
    }

    double w2 = (double)(image->width / 2);
    double h2 = (double)(image->height / 2);
    double max = sqrt(w2 * w2 + h2 * h2);
    double dst_to_max = 1.0 - sqrt(((double)x) * x + ((double)y) * y) / max;
    return (dst_to_max + 1.0) / 2.0;
}

static void paint_horizon(image_t * image) {
    for (unsigned int y = 0; y < image->height; ++y) {
        for (unsigned int x = 0; x < image->width; ++x) {
            double r, g, b, factor;

            if (y < image->height / 2) { // sky
                factor = horizon_distance(image, x, y);

                r = 113.0 * factor;
                g = 113.0 * factor;
                b = 210.0 * factor;
            } else { // floor
                factor = horizon_distance(image, x, image->height - y - 1);
                r = 113.0 * factor;
                g = 112.0 * factor;
                b = 107.0 * factor;
            }

            pixel_t * pixel = &(image->data[x + y * image->width]);
            pixel->red = (unsigned char)r;
            pixel->green = (unsigned char)g;
            pixel->blue = (unsigned char)b;
        }
    }
}

static void randomize_maze(maze_t * maze) {
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
    while (d < 0.0) d += 360.0;
    while (d >= 360.0) d -= 360.0;
    return d;
}

static int cast_ray(const image_t * image, const maze_t * maze, double angle) {
    double angle_radians = angle / 57.2957795131; // 180 / Pi
    double hypotenuse = 1.0 / RAY_STEP;
    double x_change = sin(angle_radians) * hypotenuse;
    double y_change = cos(angle_radians) * hypotenuse;
    double curr_x = maze->observer_x;
    double curr_y = maze->observer_y;

    int steps = 0;
    while(1) {
        steps += 1;
        curr_x += x_change;
        curr_y += y_change;

        unsigned int rounded_x = (unsigned int)(curr_x + 0.5);
        unsigned int rounded_y = (unsigned int)(curr_y + 0.5);

        if (maze->contents[rounded_x + rounded_y * maze->width] == MAZE_BLOCK) {
            // printf("\thit block %u,%u\n", rounded_x, rounded_y);
            return steps;
        }

        if (steps > 100000) {
            dump_maze(maze);
            printf("%f,%f   %f,%f\n", curr_x, curr_y, x_change, y_change);
            printf("ERROR\n");
            exit(1);
        }
    }

    return -1;
}

static unsigned char block_color(unsigned char val, double factor) {
    return (unsigned char)(val * factor);
}

static void paint_maze(image_t * image, const maze_t * maze) {
    double fov = 72.0;
    double min_angle = fit_angle(maze->observer_angle - fov / 2.0);
    int min_distance = -1;

    for (unsigned int x = 0 ; x < image->width; ++x) {
        double x_angle = (((double)x) / image->width);
        x_angle = fit_angle(min_angle + fov * x_angle);


        int distance = cast_ray(image, maze, x_angle);
        // printf("x=%u d=%f distance=%d\n", x, x_angle, distance);

        if (distance == -1) {
            continue;
        }

        if (min_distance == -1 || min_distance > distance) {
            min_distance = distance;
        }

        double block_size;
        block_size = 3.0 / (distance / 100.0);
        if (block_size > 1.0) {
            block_size = 1.0;
        }

        // printf("\tblock_size=%f\n", block_size);
        unsigned int mid = image->height / 2;
        for (unsigned int y = 0; y < (unsigned int)(mid * block_size); ++y) {
            pixel_t * pixel;

            pixel = &(image->data[x + (y + mid) * image->width]);
            pixel->red = block_color(61, block_size);
            pixel->green = block_color(40, block_size);
            pixel->blue = block_color(23, block_size);
            pixel = &(image->data[x + (mid - y) * image->width]);
            pixel->red = block_color(61, block_size);
            pixel->green = block_color(40, block_size);
            pixel->blue = block_color(23, block_size);
        }
    }

    // printf("min_distance=%f\n", min_distance / RAY_STEP);
    // dump_maze(maze);
}

static void clone_painting(image_t * restrict dst, const image_t * restrict src) {
    unsigned int total_size = sizeof(int) * 2 + sizeof(pixel_t) * src->width * src->height;
    memcpy(dst, src, total_size);
}

static unsigned char add_key_pressed(sfKeyCode * keys_pressed, unsigned char keys_pressed_count, sfKeyCode code) {
    for (int i = 0; i < keys_pressed_count; ++i) {
        if (keys_pressed[i] == code) {
            return keys_pressed_count;
        }
    }

    keys_pressed[keys_pressed_count] = code;
    return keys_pressed_count + 1;
}

static unsigned char remove_key_pressed(sfKeyCode * keys_pressed, unsigned char keys_pressed_count, sfKeyCode code) {
    for (int i = 0; i < keys_pressed_count; ++i) {
        if (keys_pressed[i] == code) {
            keys_pressed[i] = keys_pressed[keys_pressed_count - 1];
            return keys_pressed_count - 1;
        }
    }

    return keys_pressed_count;
}

static int key_is_pressed(const sfKeyCode * keys_pressed, unsigned char keys_pressed_count, sfKeyCode code) {
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

static void maze_render_cycle(maze_t * maze, unsigned int window_width, unsigned int window_height, unsigned int max_fps) {
    image_t * image = create_image(window_width, window_height);
    image_t * horizon = create_image(window_width, window_height);
    paint_horizon(horizon);

    sfVideoMode videoMode;
    videoMode.width = window_width;
    videoMode.height = window_height;
    videoMode.bitsPerPixel = 32;

    sfRenderWindow * window = sfRenderWindow_create(videoMode, "Raycaster", sfResize | sfClose, NULL);
    sfRenderWindow_setFramerateLimit(window, max_fps);

    sfTexture * texture = sfTexture_create(window_width, window_height);
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

    sfKeyCode keys_pressed[8];
    unsigned char keys_pressed_count = 0;

    while (sfRenderWindow_isOpen(window)) {
        if (key_is_pressed(keys_pressed, keys_pressed_count, sfKeyW)) {
            double angle_radians = maze->observer_angle / 57.2957795131; // 180 / Pi
            double hypotenuse = 0.095;
            double x_change = sin(angle_radians) * hypotenuse;
            double y_change = cos(angle_radians) * hypotenuse;

            move_observer(maze, x_change, y_change);
        }
        if (key_is_pressed(keys_pressed, keys_pressed_count, sfKeyS)) {
            double angle_radians = maze->observer_angle / 57.2957795131; // 180 / Pi
            double hypotenuse = 0.095;
            double x_change = sin(angle_radians) * hypotenuse;
            double y_change = cos(angle_radians) * hypotenuse;

            move_observer(maze, -x_change, -y_change);
        }
        if (key_is_pressed(keys_pressed, keys_pressed_count, sfKeyA)) {
            maze->observer_angle = fit_angle(maze->observer_angle - 1.8);
        }
        if (key_is_pressed(keys_pressed, keys_pressed_count, sfKeyD)) {
            maze->observer_angle = fit_angle(maze->observer_angle + 1.8);
        }

        clone_painting(image, horizon);
        paint_maze(image, maze);

        while (sfRenderWindow_pollEvent(window, &event) == sfTrue) {
            if (event.type == sfEvtClosed) {
                sfRenderWindow_close(window);
            } else if (event.type == sfEvtKeyPressed) {
                sfKeyCode code = ((sfKeyEvent *)&event)->code;

                if (code == sfKeyQ) {
                    sfRenderWindow_close(window);
                }

                keys_pressed_count = add_key_pressed(keys_pressed, keys_pressed_count, code);
            } else if (event.type == sfEvtKeyReleased) {
                sfKeyCode code = ((sfKeyEvent *)&event)->code;

                keys_pressed_count = remove_key_pressed(keys_pressed, keys_pressed_count, code);
            }
        }

        sfTexture_updateFromPixels(texture, (sfUint8 *)image->data, window_width, window_height, 0, 0);
        sfRenderWindow_drawSprite(window, sprite, &renderStates);
        sfRenderWindow_display(window);
    }
}

int main(int argc, char * argv[]) {
    srand(time(NULL));

    maze_t * maze = create_maze(20, 20);
    randomize_maze(maze);

    maze_render_cycle(maze, 800, 600, 60);
    return 0;
}
