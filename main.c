#include "global.h"

static pixel_t bg_buffer[VIEWPORT_WIDTH * VIEWPORT_HEIGHT];
static pixel_t fg_buffer[VIEWPORT_WIDTH * VIEWPORT_HEIGHT];

static sfRenderWindow * window;
static sfSprite * sprite;
static sfTexture * texture;
static sfRenderStates renderStates;

static level_t * level;

static pixel_t wall_textures[TEXTURE_PACK_WIDTH][TEXTURE_PACK_HEIGHT][SPRITE_WIDTH * SPRITE_HEIGHT];

static int paused_pressed;
static int paused;

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

static double fish_eye_table[VIEWPORT_WIDTH];

static void init_fish_eye_table() {
    double half_fov = FIELD_OF_VIEW / 2.0;

    int center_x = VIEWPORT_WIDTH / 2;

    double plane_min_x = tan((-(90.0 - half_fov)) / RADIAN_CONSTANT) * ((double)(-center_x));
    double plane_max_x = tan(((90.0 - half_fov)) / RADIAN_CONSTANT) * ((double)(center_x));

    for (int x = 0; x < center_x; ++x) {
        double arg = plane_min_x / ((double)(center_x - x));
        double angle = atan(arg) * RADIAN_CONSTANT - 90.0;
        fish_eye_table[x] = angle;
    }
    for (int x = center_x; x < VIEWPORT_WIDTH; ++x) {
        double arg = plane_max_x / ((double)(x - center_x));
        double angle = 90.0 - atan(arg) * RADIAN_CONSTANT;
        fish_eye_table[x] = angle;
    }
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

static double cast_ray(double angle, double * hit_angle, unsigned int * block_hit_x, unsigned int * block_hit_y) {
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
            return steps * RAY_STEP_CONSTANT;
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

static void paint_scene() {
    for (unsigned int x = 0 ; x < VIEWPORT_WIDTH; ++x) {
        unsigned int block_hit_x;
        unsigned int block_hit_y;
        double block_x;

        double x_angle = fit_angle(fish_eye_table[x] + level->observer_angle);
        double distance = cast_ray(x_angle, &block_x, &block_hit_x, &block_hit_y);
        distance *= cos((x_angle - level->observer_angle) / RADIAN_CONSTANT);
        distance = MAX(distance, 0.0);
        double block_size = (1.8 / distance);

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

static void move_observer(double x_change, double y_change) {
    double test_x = level->observer_x + x_change * 4.0;
    double test_y = level->observer_y + y_change * 4.0;
    unsigned int rounded_x = (unsigned int)(test_x + 0.5);
    unsigned int rounded_y = (unsigned int)(test_y + 0.5);

    if (rounded_x >= level->width || rounded_y >= level->height) {
        return;
    }
    if (level->contents[rounded_x + rounded_y * level->width]) {
        return;
    }

    double new_x = level->observer_x + x_change;
    double new_y = level->observer_y + y_change;
    level->observer_x = new_x;
    level->observer_y = new_y;
}

static int update_observer_state() {
    int needs_refresh = 0;

    if (!paused) {
        int keyW = key_is_pressed(sfKeyW);
        int keyS = key_is_pressed(sfKeyS);
        int keyA = key_is_pressed(sfKeyA);
        int keyD = key_is_pressed(sfKeyD);

        if (keyW && keyS) {
            keyW = keyS = 0;
        }
        if (keyA && keyD) {
            keyA = keyD = 0;
        }

        double mov_value = MOVEMENT_CONSTANT;

        if ((keyW || keyS) && (keyA || keyD)) {
            mov_value *=  0.70710678118;
        }

        if (keyW) {
            double angle_radians = level->observer_angle / RADIAN_CONSTANT;
            double x_change = sin(angle_radians) * mov_value;
            double y_change = cos(angle_radians) * mov_value;

            move_observer(x_change, y_change);
            needs_refresh = 1;
        }
        if (keyS) {
            double angle_radians = level->observer_angle / RADIAN_CONSTANT;
            double x_change = sin(angle_radians) * mov_value;
            double y_change = cos(angle_radians) * mov_value;

            move_observer(-x_change, -y_change);
            needs_refresh = 1;
        }
        if (keyA) {
            double angle_radians = fit_angle(level->observer_angle - 90.0) / RADIAN_CONSTANT;
            double x_change = sin(angle_radians) * (mov_value / 1.0);
            double y_change = cos(angle_radians) * (mov_value / 1.0);

            move_observer(x_change, y_change);
            needs_refresh = 1;
        }
        if (keyD) {
            double angle_radians = fit_angle(level->observer_angle + 90.0) / RADIAN_CONSTANT;
            double x_change = sin(angle_radians) * (mov_value / 1.0);
            double y_change = cos(angle_radians) * (mov_value / 1.0);

            move_observer(x_change, y_change);
            needs_refresh = 1;
        }
    }
    if (key_is_pressed(sfKeyP)) {
        paused_pressed = 1;
    } else {
        if (paused_pressed) {
            paused = !paused;
            if (paused) {
                for (int i = 0; i < VIEWPORT_WIDTH * VIEWPORT_HEIGHT; ++i) {
                    fg_buffer[i].red = (unsigned int)(fg_buffer[i].red / 2.0);
                    fg_buffer[i].green = (unsigned int)(fg_buffer[i].green / 2.0);
                    fg_buffer[i].blue = (unsigned int)(fg_buffer[i].blue / 2.0);
                }

                sfTexture_updateFromPixels(texture, (sfUint8 *)fg_buffer, VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 0, 0);
                sfRenderWindow_drawSprite(window, sprite, &renderStates);
                sfRenderWindow_display(window);
                sfRenderWindow_setMouseCursorVisible(window, sfTrue);
            } else {
                sfRenderWindow_setMouseCursorVisible(window, sfFalse);
            }
            paused_pressed = 0;
        }
    }

    return needs_refresh;
}

static void center_mouse() {
    sfVector2i position;
    position.x = VIEWPORT_WIDTH / 2;
    position.y = VIEWPORT_HEIGHT / 2;
    sfMouse_setPositionRenderWindow(position, window);
}

static void main_render_loop() {
    printf("Level start - press P to pause and Esc to quit\n");
    fill_in_background();

    sfVideoMode videoMode;
    videoMode.width = VIEWPORT_WIDTH;
    videoMode.height = VIEWPORT_HEIGHT;
    videoMode.bitsPerPixel = 32;

    window = sfRenderWindow_create(videoMode, "Raycaster", sfResize | sfClose, NULL);
    sfRenderWindow_setFramerateLimit(window, MAX_FPS);

    videoMode = sfVideoMode_getDesktopMode();
    sfVector2i position;
    position.x = videoMode.width / 2 - VIEWPORT_WIDTH / 2;
    position.y = videoMode.height / 2 - VIEWPORT_HEIGHT / 2;

    sfRenderWindow_setPosition(window, position);
    sfRenderWindow_setMouseCursorVisible(window, sfFalse);
    center_mouse();

    texture = sfTexture_create(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    sprite = sfSprite_create();
    sfSprite_setTexture(sprite, texture, sfFalse);

    sfColor bgColor;
    memset(&bgColor, 0, sizeof(bgColor));

    sfEvent event;
    renderStates.blendMode = sfBlendNone;
    renderStates.transform = sfTransform_Identity;
    renderStates.texture = texture;
    renderStates.shader = NULL;

    unsigned int frames_second = 1;
    long unsigned int last_ms = 0;
    struct timespec spec;
    int needs_refresh = 1;

    while (sfRenderWindow_isOpen(window)) {
        if (!paused) {
            clock_gettime(CLOCK_REALTIME, &spec);
            long unsigned int ms = spec.tv_nsec / 1000000;

            if (last_ms > ms) {
                printf("   \rFPS=%d", frames_second);
                fflush(stdout);
                frames_second = 1;
            }

            frames_second += 1;
            last_ms = ms;
        }

        if (update_observer_state()) {
            needs_refresh = 1;
        }

        if (!paused) {
            copy_bg_to_fg();
            paint_scene();
        }

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
            } else if (event.type == sfEvtMouseMoved) {
                int move_x = ((sfMouseMoveEvent *)&event)->x - VIEWPORT_WIDTH / 2;

                if (move_x) {
                    if (!paused) {
                        double angle_change = ((double)move_x) * ROTATION_CONSTANT / (VIEWPORT_WIDTH / 16);
                        level->observer_angle += angle_change;
                        needs_refresh = 1;
                    }
                }
            }
        }

        if (paused) {

        } else {
            if (needs_refresh) {
                center_mouse();
                sfTexture_updateFromPixels(texture, (sfUint8 *)fg_buffer, VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 0, 0);
                sfRenderWindow_drawSprite(window, sprite, &renderStates);
                needs_refresh = 0;
            }
            sfRenderWindow_display(window);
        }
    }

    printf("\r        \r"); // clear FPS
    printf("Level closed\n");
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

static int valid_level_file_name(const char * s) {
    return !(s[0] == 0 || s[0] == '.' || s[0] == '/' || s[0] == '\\');
}

static level_t * select_level() {
    DIR * dir = opendir("./levels");
    struct dirent * entry;

    if (!dir) {
        error("Could not open directory \"./levels\"");
    }

    printf("\nLevels found:\n");
    char * levels[99];
    unsigned int levels_listed = 0;

    while (levels_listed < 99 && (entry = readdir(dir)) != NULL) {
        if (valid_level_file_name(entry->d_name)) {
            levels[levels_listed] = malloc(MAX_FILE_NAME_SIZ);
            strncpy(levels[levels_listed], entry->d_name, MAX_FILE_NAME_SIZ);

            printf("%3u - %s\n", levels_listed + 1, levels[levels_listed]);

            levels_listed++;
        }
    }

    closedir(dir);

    if (levels_listed == 0) {
        error("No levels found");
    }

    printf("\nSelect level\n");

    unsigned int selection = 0;

    if (levels_listed > 1) {
        unsigned int buf_idx;
        char * buf = malloc(MAX_FILE_NAME_SIZ);

        while(1) {
            printf("> ");
            fflush(stdout);

            buf_idx = 0;
            while (buf_idx < MAX_FILE_NAME_SIZ - 1) {
                char c = getchar();

                if (c == 0 || c == '\n') {
                    break;
                }
                buf[buf_idx++] = c;
            }
            buf[buf_idx] = 0;

            selection = (unsigned int)atoi(buf);
            if (selection >= 1 && selection <= levels_listed) {
                selection--;
                break;
            } else {
                printf("Invalid input\n");
            }
        }
        free(buf);
    } else {
        printf("> 1\n");
    }

    level_t * ret = read_level_info(levels[selection]);

    for (unsigned int i = 0; i < levels_listed; ++i) {
        free(levels[i]);
    }

    return ret;
}

int main() {
    init_fish_eye_table();
    load_textures();

    level = select_level();
    main_render_loop();
    free(level);
    return EXIT_SUCCESS;
}
