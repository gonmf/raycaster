#include "global.h"

static level_t * level;
static pixel_t * background;

static bool pause_btn_pressed = false;
static bool paused = false;
static bool action_btn_pressed = false;
static bool map_btn_pressed = false;
static bool show_map = false;

static bool position_blocked(
    unsigned int rounded_x, unsigned int rounded_y,
    bool allow_enter_door, unsigned int door_x, unsigned int door_y
) {
    unsigned char content_type = level->content_type[rounded_x + rounded_y * level->width];
    unsigned char special_effect = level->special_effects[rounded_x + rounded_y * level->width];

    if (content_type == CONTENT_TYPE_WALL || (content_type == CONTENT_TYPE_OBJECT && special_effect == SPECIAL_EFFECT_NONE) || content_type == CONTENT_TYPE_DOOR || (content_type == CONTENT_TYPE_DOOR_OPEN && !allow_enter_door && door_x == rounded_x && door_y == rounded_y)) {
        return true;
    }

    for (unsigned int i = 0; i < level->enemies_count; ++i) {
        if (rounded_x == (unsigned int)(level->enemy[i].x + 0.5) && rounded_y == (unsigned int)(level->enemy[i].y + 0.5)) {
            return true;
        }
    }

    return false;
}

static void move_observer2(double x_change, double y_change) {
    double new_x = level->observer_x + x_change;
    double new_y = level->observer_y + y_change;
    unsigned int rounded_x = (unsigned int)(new_x + 0.5);
    unsigned int rounded_y = (unsigned int)(new_y + 0.5);
    unsigned int door_x;
    unsigned int door_y;
    double percentage_open;
    bool door_is_opening = opening_door_transition(&percentage_open, &door_x, &door_y);
    bool allow_enter_door = !door_is_opening || (door_is_opening && percentage_open > 0.75);

    rounded_x = (unsigned int)(new_x + 0.5 + 0.22);
    if (position_blocked(rounded_x, rounded_y, allow_enter_door, door_x, door_y)) {
        return;
    }

    rounded_x = (unsigned int)(new_x + 0.5 - 0.22);
    if (position_blocked(rounded_x, rounded_y, allow_enter_door, door_x, door_y)) {
        return;
    }

    rounded_x = (unsigned int)(new_x + 0.5);
    rounded_y = (unsigned int)(new_y + 0.5 + 0.22);
    if (position_blocked(rounded_x, rounded_y, allow_enter_door, door_x, door_y)) {
        return;
    }

    rounded_y = (unsigned int)(new_y + 0.5 - 0.22);
    if (position_blocked(rounded_x, rounded_y, allow_enter_door, door_x, door_y)) {
        return;
    }

    level->observer_x = new_x;
    level->observer_y = new_y;
}

static void move_observer(double x_change, double y_change) {
    move_observer2(x_change, 0.0);
    move_observer2(0.0, y_change);
}

static bool update_observer_state() {
    bool needs_refresh = false;

    if (!paused) {
        bool keyW = key_is_pressed(sfKeyW);
        bool keyS = key_is_pressed(sfKeyS);
        bool keyA = key_is_pressed(sfKeyA);
        bool keyD = key_is_pressed(sfKeyD);

        if (keyW && keyS) {
            keyW = keyS = false;
        }
        if (keyA && keyD) {
            keyA = keyD = false;
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
            needs_refresh = true;
        }
        if (keyS) {
            double angle_radians = level->observer_angle / RADIAN_CONSTANT;
            double x_change = sin(angle_radians) * mov_value;
            double y_change = cos(angle_radians) * mov_value;

            move_observer(-x_change, -y_change);
            needs_refresh = true;
        }
        if (keyA) {
            double angle_radians = fit_angle(level->observer_angle - 90.0) / RADIAN_CONSTANT;
            double x_change = sin(angle_radians) * (mov_value / 1.0);
            double y_change = cos(angle_radians) * (mov_value / 1.0);

            move_observer(x_change, y_change);
            needs_refresh = true;
        }
        if (keyD) {
            double angle_radians = fit_angle(level->observer_angle + 90.0) / RADIAN_CONSTANT;
            double x_change = sin(angle_radians) * (mov_value / 1.0);
            double y_change = cos(angle_radians) * (mov_value / 1.0);

            move_observer(x_change, y_change);
            needs_refresh = true;
        }
    }

    if (key_is_pressed(sfKeyP)) {
        pause_btn_pressed = true;
    } else {
        if (pause_btn_pressed) {
            paused = !paused;
            if (paused) {
                darken_scene(0.5);
                window_update_pixels(fg_buffer, VIEWPORT_WIDTH, VIEWPORT_HEIGHT, UI_BORDER, UI_BORDER);
                window_refresh();
                set_cursor_visible(true);
            } else {
                set_cursor_visible(false);
                needs_refresh = true;
            }
            pause_btn_pressed = false;
        }
    }
    if (key_is_pressed(sfKeyE)) {
        action_btn_pressed = true;
    } else {
        if (action_btn_pressed) {
            if (open_door_in_front(level)) {
                needs_refresh = true;
            }
            action_btn_pressed = false;
        }
    }
    if (key_is_pressed(sfKeyM)) {
        map_btn_pressed = true;
    } else {
        if (map_btn_pressed) {
            show_map = !show_map;
            map_btn_pressed = false;
            needs_refresh = true;
        }
    }

    return needs_refresh;
}

static void main_render_loop() {
    window_update_pixels(background, WINDOW_TOTAL_WIDTH, WINDOW_TOTAL_HEIGHT, 0, 0);

    unsigned int frames_second = 1;
    long unsigned int last_ms = 0;
    struct timespec spec;
    bool needs_refresh = true;

    while (window_is_open()) {
        if (!paused) {
            clock_gettime(CLOCK_REALTIME, &spec);
            long unsigned int ms = spec.tv_nsec / 1000000;

            if (last_ms > ms) {
                printf("FPS=%u   \r", frames_second);
                fflush(stdout);
                frames_second = 1;
            }

            frames_second += 1;
            last_ms = ms;

            if (transition_step(level)) {
                needs_refresh = true;
            }
        }

        if (update_observer_state()) {
            needs_refresh = true;
        }

        if (!paused) {
            paint_scene(level);

            double flash_percentage;
            if (short_flash_effect(&flash_percentage)) {
                brighten_scene(flash_percentage);
            }

            if (needs_refresh) {
                bool exit_found;
                apply_special_effect(level, &exit_found);

                if (exit_found) {
                    window_close();
                    printf("Level exit found\n");
                    return;
                }
            }

            if (show_map) {
                paint_map(level);
            }
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
                int move_y = ((sfMouseMoveEvent *)&event)->y - VIEWPORT_HEIGHT / 2;

                if (!paused) {
                    if (move_x) {
                        double angle_change = ((double)move_x) * HORIZONTAL_ROTATION_CONSTANT / VIEWPORT_WIDTH;
                        level->observer_angle += angle_change;
                        needs_refresh = true;
                    }

                    if (move_y) {
                        double angle_change = ((double)move_y) * VERTICAL_ROTATION_CONSTANT / VIEWPORT_HEIGHT;
                        level->observer_angle2 = MIN(MAX(level->observer_angle2 - angle_change, 1.0), 179.0);
                        needs_refresh = true;
                    }
                }
            }
        }

        if (!paused) {
            if (needs_refresh) {
                window_center_mouse();
                window_update_pixels(fg_buffer, VIEWPORT_WIDTH, VIEWPORT_HEIGHT, UI_BORDER, UI_BORDER);
                needs_refresh = false;
            }
            window_refresh();
        }
    }
}

static void paint_ui() {
    if (background == NULL) {
        background = calloc(WINDOW_TOTAL_WIDTH * WINDOW_TOTAL_HEIGHT, sizeof(pixel_t));

        for (unsigned int y = 0; y < WINDOW_TOTAL_HEIGHT; ++y) {
            for (unsigned int x = 0; x < WINDOW_TOTAL_WIDTH; ++x) {
                background[x + y * WINDOW_TOTAL_WIDTH] = color_ui_bg;
            }
        }
        for (unsigned int shade_size = 0; shade_size < 5; ++shade_size) {
            // left and right sides
            for (unsigned y = UI_BORDER - shade_size; y < UI_BORDER + VIEWPORT_HEIGHT + shade_size; ++y) {
                unsigned int x = UI_BORDER - shade_size;
                background[x + y * WINDOW_TOTAL_WIDTH] = color_ui_bg_dark;
                x = WINDOW_TOTAL_WIDTH - x - 1;
                background[x + y * WINDOW_TOTAL_WIDTH] = color_ui_bg_light;
            }
            // top and bottom sides
            for (unsigned x = UI_BORDER - shade_size; x < UI_BORDER + VIEWPORT_WIDTH + shade_size; ++x) {
                unsigned int y = UI_BORDER - shade_size;
                background[x + y * WINDOW_TOTAL_WIDTH] = color_ui_bg_dark;
                y = UI_BORDER + VIEWPORT_HEIGHT + shade_size - 1;
                background[x + y * WINDOW_TOTAL_WIDTH] = color_ui_bg_light;
            }
        }
    }
}

static void open_level() {
    printf("Level start - press P to pause and Esc to quit\n");

    paint_ui();
    paint_scene(level);

    window_start();

    main_render_loop();

    printf("Level closed\n");
}


static int valid_level_file_name(const char * s) {
    return !(s[0] == 0 || s[0] == '.' || s[0] == '/' || s[0] == '\\');
}

static bool select_level() {
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
            levels[levels_listed] = calloc(MAX_FILE_NAME_SIZ, 1);
            strncpy(levels[levels_listed], entry->d_name, MAX_FILE_NAME_SIZ);

            printf("%3u - %s\n", levels_listed + 1, levels[levels_listed]);

            levels_listed++;
        }
    }

    closedir(dir);

    if (levels_listed == 0) {
        error("No levels found");
    }

    printf("\nSelect level (Q to quit)\n");

    unsigned int selection = 0;
    bool quit = false;

    if (levels_listed > 1) {
        unsigned int buf_idx;
        char * buf = calloc(MAX_FILE_NAME_SIZ, 1);

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

            if (strcmp(buf, "q") == 0 || strcmp(buf, "Q") == 0) {
                quit = true;
                break;
            }

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

    if (!quit) {
        level = read_level_info(levels[selection]);
    }

    for (unsigned int i = 0; i < levels_listed; ++i) {
        free(levels[i]);
    }

    return !quit;
}

static void unload_assets() {
    pause_btn_pressed = false;
    paused = false;
    action_btn_pressed = false;
    map_btn_pressed = false;
    show_map = false;

    if (level) {
        free(level->content_type);
        free(level->texture);
        free(level->special_effects);
        free(level->map_revealed);
        free(level->enemy);
        free(level);
        level = NULL;
    }
    if (background) {
        free(background);
        background = NULL;
    }
    if (wall_textures) {
        free(wall_textures);
        wall_textures = NULL;
    }
    if (objects_sprites) {
        free(objects_sprites);
        objects_sprites = NULL;
    }
    for (int i = 0; i < 5; ++i) {
        if (enemy_sprites[i]) {
            free(enemy_sprites[i]);
            enemy_sprites[i] = NULL;
        }
    }
}

int main() {
    init_base_colors();
    init_fish_eye_table();

    while (select_level()) {
        open_level();
#if 0
        break;
#endif
        unload_assets();
    }

    printf("Goodbye\n");

    return EXIT_SUCCESS;
}
