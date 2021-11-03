#include "global.h"

static pixel_t bg_buffer[VIEWPORT_WIDTH * VIEWPORT_HEIGHT];
static pixel_t fg_buffer[VIEWPORT_WIDTH * VIEWPORT_HEIGHT];

static sfKeyCode keys_pressed[KEYS_PRESSED_BUFFER_SIZE];
static unsigned char keys_pressed_count;

static level_t * level;

static pixel_t wall_textures[TEXTURE_PACK_WIDTH][TEXTURE_PACK_HEIGHT][SPRITE_WIDTH * SPRITE_HEIGHT];

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
    return (dst_to_max + 2.0) / 3.0;
}

static void fill_in_background() {
    for (unsigned int y = 0; y < VIEWPORT_HEIGHT; ++y) {
        for (unsigned int x = 0; x < VIEWPORT_WIDTH; ++x) {
            double r, g, b, factor;

            if (y < VIEWPORT_HEIGHT / 2) { // sky
                factor = horizon_distance(x, y);

                r = level->ceil_color.red * factor;
                g = level->ceil_color.green * factor;
                b = level->ceil_color.blue * factor;
            } else { // floor
                factor = horizon_distance(x, VIEWPORT_HEIGHT - y - 1);
                r = level->floor_color.red * factor;
                g = level->floor_color.green * factor;
                b = level->floor_color.blue * factor;
            }

            pixel_t * pixel = &(bg_buffer[x + y * VIEWPORT_WIDTH]);
            pixel->red = (unsigned char)r;
            pixel->green = (unsigned char)g;
            pixel->blue = (unsigned char)b;
        }
    }
}

static double fit_angle(double d) {
    if (d < 0.0) {
        d += 360.0;
    } else if (d >= 360.0) {
        d -= 360.0;
    }
    return d;
}

static unsigned int cast_ray(double angle, double * hit_angle, unsigned int * block_hit_x, unsigned int * block_hit_y) {
    double angle_radians = angle / RADIAN_CONSTANT;
    double x_change = sin(angle_radians) * RAY_STEP_CONSTANT;
    double y_change = cos(angle_radians) * RAY_STEP_CONSTANT;
    double curr_x = level->observer_x;
    double curr_y = level->observer_y;

    unsigned int steps = 0;
    while(1) {
        steps += 1;
        curr_x += x_change;
        curr_y += y_change;

        unsigned int rounded_x = (unsigned int)(curr_x + 0.5);
        unsigned int rounded_y = (unsigned int)(curr_y + 0.5);

        if (level->contents[rounded_x + rounded_y * level->width]) {
            *block_hit_x = rounded_x;
            *block_hit_y = rounded_y;

            curr_x -= x_change / 2.0;
            curr_y -= y_change / 2.0;
            double hangle;

            double vertice_x[4];
            double vertice_y[4];
            vertice_x[0] = ((double)rounded_x) + 0.5;
            vertice_y[0] = ((double)rounded_y) + 0.5;
            vertice_x[1] = ((double)rounded_x) + 0.5;
            vertice_y[1] = ((double)rounded_y) - 0.5;
            vertice_x[2] = ((double)rounded_x) - 0.5;
            vertice_y[2] = ((double)rounded_y) - 0.5;
            vertice_x[3] = ((double)rounded_x) - 0.5;
            vertice_y[3] = ((double)rounded_y) + 0.5;
            int closest_vertice = -1;
            double min_distance = 0.0;
            for (int i = 0; i < 4; ++i) {
                double distance = sqrt((vertice_x[i] - level->observer_x) * (vertice_x[i] - level->observer_x) + (vertice_y[i] - level->observer_y) * (vertice_y[i] - level->observer_y));
                if ((closest_vertice == -1) || (min_distance > distance)) {
                    closest_vertice = i;
                    min_distance = distance;
                }
            }

            int left_vertice = (closest_vertice + 4 - 1) % 4;
            int right_vertice = (closest_vertice + 1) % 4;

            hangle = sqrt((vertice_x[closest_vertice] - curr_x) * (vertice_x[closest_vertice] - curr_x) + (vertice_y[closest_vertice] - curr_y) * (vertice_y[closest_vertice] - curr_y));

            double distance_left = sqrt((vertice_x[left_vertice] - curr_x) * (vertice_x[left_vertice] - curr_x) + (vertice_y[left_vertice] - curr_y) * (vertice_y[left_vertice] - curr_y));
            double distance_right = sqrt((vertice_x[right_vertice] - curr_x) * (vertice_x[right_vertice] - curr_x) + (vertice_y[right_vertice] - curr_y) * (vertice_y[right_vertice] - curr_y));

            if (distance_left > distance_right) {
                hangle = 1.0 - hangle;
            }

            *hit_angle = MIN(MAX(hangle, 0.0), 1.0);
            return steps;
        }
    }

    return 0;
}

static void block_color(pixel_t * dst, double block_x, double block_y, double factor, unsigned char block_type) {
    factor = (2.0 + factor) / 3.0;

    unsigned int x = (unsigned int)(SPRITE_WIDTH * block_x);
    unsigned int y = (unsigned int)(SPRITE_HEIGHT * block_y);

    unsigned char texture_x = block_type % TEXTURE_PACK_WIDTH;
    unsigned char texture_y = block_type / TEXTURE_PACK_WIDTH;
    pixel_t * src = &wall_textures[texture_x][texture_y][x + y * SPRITE_WIDTH];
    dst->red = (unsigned int)MIN(MAX(0, src->red * factor), src->red);
    dst->green = (unsigned int)MIN(MAX(0, src->green * factor), src->green);
    dst->blue = (unsigned int)MIN(MAX(0, src->blue * factor), src->blue);
}

static void paint_level() {
    double min_angle = fit_angle(level->observer_angle - FIELD_OF_VIEW / 2.0);
    unsigned int min_distance = 0;

    for (unsigned int x = 0 ; x < VIEWPORT_WIDTH; ++x) {
        double x_angle = (((double)x) / VIEWPORT_WIDTH);
        x_angle = fit_angle(min_angle + FIELD_OF_VIEW * x_angle);

        unsigned int block_hit_x;
        unsigned int block_hit_y;
        double block_x;

        unsigned int distance = cast_ray(x_angle, &block_x, &block_hit_x, &block_hit_y);
        if (min_distance == 0 || min_distance > distance) {
            min_distance = distance;
        }

        double block_size;
        block_size = 3.0 / (distance / 100.0);
        unsigned char block_type = level->contents[block_hit_x + block_hit_y * level->width] - 1;

        double block_y;
        pixel_t * pixel;
        unsigned int y;
        unsigned int viewport_mid = VIEWPORT_HEIGHT / 2;
        unsigned int end_y = (unsigned int)(block_size * viewport_mid);
        unsigned int orig_end_y = end_y;
        if (end_y > viewport_mid) {
            end_y = viewport_mid;
        }
        for (unsigned int mid_y = 0; mid_y < end_y; ++mid_y) {
            // top part
            y = viewport_mid - mid_y;
            pixel = &(fg_buffer[x + y * VIEWPORT_WIDTH]);
            block_y = ((orig_end_y - mid_y) * 1) / (VIEWPORT_HEIGHT * block_size);
            block_color(pixel, block_x, block_y, block_size, block_type);

            // bottom part
            y = viewport_mid + mid_y;
            pixel = &(fg_buffer[x + y * VIEWPORT_WIDTH]);
            block_y = (mid_y + VIEWPORT_HEIGHT * block_size * 0.5) / (VIEWPORT_HEIGHT * block_size);
            block_color(pixel, block_x, block_y, block_size, block_type);
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

static void move_observer(double x_change, double y_change) {
    double new_x = level->observer_x + x_change;
    double new_y = level->observer_y + y_change;

    unsigned int rounded_x = (unsigned int)(new_x + 0.5);
    unsigned int rounded_y = (unsigned int)(new_y + 0.5);

    if (rounded_x >= level->width || rounded_y >= level->height) {
        return;
    }

    if (level->contents[rounded_x + rounded_y * level->width]) {
        return;
    }

    level->observer_x = new_x;
    level->observer_y = new_y;
}

static void rotate_observer(double degrees) {
    level->observer_angle = fit_angle(level->observer_angle + degrees);
}

static void update_observer_state() {
    if (key_is_pressed(sfKeyW)) {
        double angle_radians = level->observer_angle / RADIAN_CONSTANT;
        double x_change = sin(angle_radians) * MOVEMENT_CONSTANT;
        double y_change = cos(angle_radians) * MOVEMENT_CONSTANT;

        move_observer(x_change, y_change);
    }
    if (key_is_pressed(sfKeyS)) {
        double angle_radians = level->observer_angle / RADIAN_CONSTANT;
        double x_change = sin(angle_radians) * MOVEMENT_CONSTANT;
        double y_change = cos(angle_radians) * MOVEMENT_CONSTANT;

        move_observer(-x_change, -y_change);
    }
    if (key_is_pressed(sfKeyQ)) {
        double angle_radians = fit_angle(level->observer_angle - 90.0) / RADIAN_CONSTANT;
        double x_change = sin(angle_radians) * MOVEMENT_CONSTANT;
        double y_change = cos(angle_radians) * MOVEMENT_CONSTANT;

        move_observer(x_change, y_change);
    }
    if (key_is_pressed(sfKeyE)) {
        double angle_radians = fit_angle(level->observer_angle + 90.0) / RADIAN_CONSTANT;
        double x_change = sin(angle_radians) * MOVEMENT_CONSTANT;
        double y_change = cos(angle_radians) * MOVEMENT_CONSTANT;

        move_observer(x_change, y_change);
    }
    if (key_is_pressed(sfKeyA)) {
        rotate_observer(-1.8);
    }
    if (key_is_pressed(sfKeyD)) {
        rotate_observer(1.8);
    }
}

static void main_render_loop() {
    printf("Level start\n");
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
        paint_level();

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

    printf("Level end\n");
}

static void load_textures() {
    pixel_t * sprite_pack = calloc(SPRITE_WIDTH * TEXTURE_PACK_WIDTH * SPRITE_HEIGHT * TEXTURE_PACK_HEIGHT, sizeof(pixel_t));
    read_sprite_pack(sprite_pack, TEXTURE_PACK_NAME, TEXTURE_PACK_WIDTH, TEXTURE_PACK_HEIGHT);

    for (unsigned int y = 0; y < TEXTURE_PACK_HEIGHT; ++y) {
        for (unsigned int x = 0; x < TEXTURE_PACK_WIDTH; ++x) {
            read_subsprite(wall_textures[x][y], sprite_pack, TEXTURE_PACK_WIDTH, TEXTURE_PACK_HEIGHT, x, y);
        }
    }

    free(sprite_pack);
}

static level_t * select_level() {
    DIR * d;
    struct dirent * dir;
    d = opendir("./levels");

    if (!d) {
        error("Could not open directory \"./levels\"");
    }

    printf("Select level:\n");
    char * levels[128];
    unsigned int levels_listed = 0;

    while ((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] != '.') {
            levels[levels_listed] = malloc(MAX_FILE_NAME_SIZ);
            strncpy(levels[levels_listed], dir->d_name, MAX_FILE_NAME_SIZ);

            printf("%u - %s\n", levels_listed + 1, levels[levels_listed]);

            levels_listed++;
        }
    }

    closedir(d);

    // TODO: prompt user for level number

    level_t * ret = read_level_info(levels[0]);

    for (unsigned int i = 0; i < levels_listed; ++i) {
        free(levels[i]);
    }

    return ret;
}

int main() {
    load_textures();

    level = select_level();
    main_render_loop();
    free(level);
    return EXIT_SUCCESS;
}
