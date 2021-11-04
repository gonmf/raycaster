#include "global.h"

static level_t * level;

static int paused_pressed;
static int paused;

static void move_observer(double x_change, double y_change) {
    // 4.0 so we don't get too close to the block
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
                color_filter(0.5);
                window_update_pixels(foreground_buffer());
                window_refresh();
                set_cursor_visible(1);
            } else {
                set_cursor_visible(0);
            }
            paused_pressed = 0;
        }
    }

    return needs_refresh;
}

static void main_render_loop() {
    unsigned int frames_second = 1;
    long unsigned int last_ms = 0;
    struct timespec spec;
    int needs_refresh = 1;

    while (window_is_open()) {
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
            paint_scene(level);
        }

        sfEvent event;
        while (window_poll_event(&event)) {
            if (event.type == sfEvtClosed) {
                window_close();
                return;
            } else if (event.type == sfEvtKeyPressed) {
                sfKeyCode code = ((sfKeyEvent *)&event)->code;

                if (code == sfKeyEscape) {
                    window_close();
                    return;
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

        if (!paused) {
            if (needs_refresh) {
                window_center_mouse();
                window_update_pixels(foreground_buffer());
                needs_refresh = 0;
            }
            window_refresh();
        }
    }
}

static void open_level() {
    printf("Level start - press P to pause and Esc to quit\n");
    paint_scene_first_time(level);

    window_start();

    main_render_loop();

    printf("\r        \r"); // clear FPS
    printf("Level closed\n");
}


static int valid_level_file_name(const char * s) {
    return !(s[0] == 0 || s[0] == '.' || s[0] == '/' || s[0] == '\\');
}

static void select_level() {
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

    level = read_level_info(levels[selection]);

    for (unsigned int i = 0; i < levels_listed; ++i) {
        free(levels[i]);
    }
}

int main() {
    load_textures();
    init_fish_eye_table();

    select_level();
    open_level();

    return EXIT_SUCCESS;
}
