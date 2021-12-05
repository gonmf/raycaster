#include "global.h"

static level_t * level;
static unsigned int last_fps;
static bool pause_btn_pressed = false;
static bool paused = false;
static bool action_btn_pressed = false;
static bool map_btn_pressed = false;
static bool key_1_pressed = false;
static bool key_2_pressed = false;
static bool key_3_pressed = false;
static bool key_4_pressed = false;
static bool show_map = false;

static bool position_blocked(
    unsigned int rounded_x, unsigned int rounded_y,
    bool allow_enter_door, unsigned int door_x, unsigned int door_y
) {
    unsigned char content_type = level->content_type[rounded_x + rounded_y * level->width];
    if (content_type == CONTENT_TYPE_WALL || content_type == CONTENT_TYPE_DOOR || (content_type == CONTENT_TYPE_DOOR_OPEN && !allow_enter_door && door_x == rounded_x && door_y == rounded_y)) {
        return true;
    }

    for (unsigned int i = 0; i < level->objects_count; ++i) {
        if (rounded_x == (unsigned int)(level->object[i].x + 0.5) && rounded_y == (unsigned int)(level->object[i].y + 0.5)) {
            if (level->object[i].type == OBJECT_TYPE_BLOCKING) {
                return true;
            }
        }
    }

    for (unsigned int i = 0; i < level->enemies_count; ++i) {
        if (rounded_x == (unsigned int)(level->enemy[i].x + 0.5) && rounded_y == (unsigned int)(level->enemy[i].y + 0.5)) {
            if (level->enemy[i].state != ENEMY_STATE_DEAD) {
                return true;
            }
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

static void update_observer_state() {
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
        }
        if (keyS) {
            double angle_radians = level->observer_angle / RADIAN_CONSTANT;
            double x_change = sin(angle_radians) * mov_value;
            double y_change = cos(angle_radians) * mov_value;

            move_observer(-x_change, -y_change);
        }
        if (keyA) {
            double angle_radians = fit_angle(level->observer_angle - 90.0) / RADIAN_CONSTANT;
            double x_change = sin(angle_radians) * (mov_value / 1.0);
            double y_change = cos(angle_radians) * (mov_value / 1.0);

            move_observer(x_change, y_change);
        }
        if (keyD) {
            double angle_radians = fit_angle(level->observer_angle + 90.0) / RADIAN_CONSTANT;
            double x_change = sin(angle_radians) * (mov_value / 1.0);
            double y_change = cos(angle_radians) * (mov_value / 1.0);

            move_observer(x_change, y_change);
        }
    }

    if (key_is_pressed(sfKeyP)) {
        pause_btn_pressed = true;
    } else {
        if (pause_btn_pressed) {
            paused = !paused;
            if (paused) {
                scene_shading(color_black, 0.5);
                window_update_pixels(fg_buffer);
                window_refresh();
                set_cursor_visible(true);
            } else {
                set_cursor_visible(false);
            }
            pause_btn_pressed = false;
        }
    }
    if (key_is_pressed(sfKeyE)) {
        action_btn_pressed = true;
    } else {
        if (action_btn_pressed) {
            open_door_in_front(level);
            action_btn_pressed = false;
        }
    }
    if (key_is_pressed(sfKeyM)) {
        map_btn_pressed = true;
    } else {
        if (map_btn_pressed) {
            show_map = !show_map;
            map_btn_pressed = false;
        }
    }
    if (key_is_pressed(sfKeyNum1)) {
        key_1_pressed = true;
    } else {
        if (key_1_pressed) {
            switch_weapon(level, 0);
            key_1_pressed = false;
        }
    }
    if (key_is_pressed(sfKeyNum2)) {
        key_2_pressed = true;
    } else {
        if (key_2_pressed) {
            switch_weapon(level, 1);
            key_2_pressed = false;
        }
    }
    if (key_is_pressed(sfKeyNum3)) {
        key_3_pressed = true;
    } else {
        if (key_3_pressed) {
            switch_weapon(level, 2);
            key_3_pressed = false;
        }
    }
    if (key_is_pressed(sfKeyNum4)) {
        key_4_pressed = true;
    } else {
        if (key_4_pressed) {
            switch_weapon(level, 3);
            key_4_pressed = false;
        }
    }
}

static void game_over_animation() {
    unsigned int duration = GAME_OVER_ANIMATION_SPEED;

    while (window_is_open()) {
        if (duration-- == 0) {
            window_close();
            return;
        }

        double factor = ((GAME_OVER_ANIMATION_SPEED - duration) / 4.0) / GAME_OVER_ANIMATION_SPEED;
        scene_shading(color_dark_red, factor);

        sfEvent event;
        while (window_poll_event(&event)) {
            if (event.type == sfEvtClosed) {
                window_close();
                return;
            }
        }

        window_center_mouse();
        window_update_pixels(fg_buffer);
        window_refresh();
    }
}

static void write_text_ui() {
    char buffer[20];

    // Top left corner: Score
    screen_write("Score", UI_PADDING, UI_PADDING);
    snprintf(buffer, 20, "%05u", level->score);
    screen_write(buffer, UI_PADDING, UI_PADDING + font_sprites->sprite_size);

    // Top right corner: FPS
    snprintf(buffer, 20, "%u", last_fps);
    screen_write(buffer, VIEWPORT_WIDTH - UI_PADDING - strlen(buffer) * font_sprites->sprite_size, UI_PADDING);

    // Bottom left corner: Health and lives
    snprintf(buffer, 20, "%3u%% ~ %u", level->life, 3);
    screen_write(buffer, UI_PADDING, VIEWPORT_HEIGHT - UI_PADDING - font_sprites->sprite_size);

    // Bottom right corner: Weapon and ammo
    char * weapon_name;
    switch (level->weapon) {
        case 0:
            weapon_name = "Knife";
            break;
        case 1:
            weapon_name = "Pistol";
            break;
        case 2:
            weapon_name = "SMG";
            break;
        default:
            weapon_name = "Minigun";
    }

    if (level->weapon == 0) {
        screen_write(weapon_name, VIEWPORT_WIDTH - UI_PADDING - strlen(weapon_name) * font_sprites->sprite_size, VIEWPORT_HEIGHT - UI_PADDING - font_sprites->sprite_size);
    } else {
        screen_write(weapon_name, VIEWPORT_WIDTH - UI_PADDING - strlen(weapon_name) * font_sprites->sprite_size, VIEWPORT_HEIGHT - UI_PADDING - 2 * font_sprites->sprite_size);
        snprintf(buffer, 20, "%u / 99", level->ammo);
        screen_write(buffer, VIEWPORT_WIDTH - UI_PADDING - strlen(buffer) * font_sprites->sprite_size, VIEWPORT_HEIGHT - UI_PADDING - font_sprites->sprite_size);
    }
}

static void main_render_loop() {
    unsigned int frames_second = 1;
    long unsigned int last_ms = 0;
    struct timespec spec;

    while (window_is_open()) {
        if (level->life == 0) {
            game_over_animation();
            return;
        }

        if (!paused) {
            clock_gettime(CLOCK_REALTIME, &spec);
            long unsigned int ms = spec.tv_nsec / 1000000;

            if (last_ms > ms) {
                last_fps = frames_second;
                frames_second = 1;
            }

            frames_second += 1;
            last_ms = ms;
            transition_step(level);
        }

        update_observer_state();

        if (!paused) {
            update_enemies_state(level);

            paint_scene(level);
            write_text_ui();

            double flash_percentage;
            pixel_t flash_color;
            if (short_flash_effect(&flash_percentage, &flash_color)) {
                scene_shading(flash_color, flash_percentage);
            }

            bool exit_found;
            apply_special_effect(level, &exit_found);

            if (exit_found) {
                printf("Level exit found\n");
                window_close();
                return;
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
            } else if (paused) {
                // do nothing
            } else if (event.type == sfEvtMouseMoved) {
                int move_x = ((sfMouseMoveEvent *)&event)->x - VIEWPORT_WIDTH / 2;
                int move_y = ((sfMouseMoveEvent *)&event)->y - VIEWPORT_HEIGHT / 2;

                if (move_x) {
                    double angle_change = ((double)move_x) * HORIZONTAL_ROTATION_CONSTANT / VIEWPORT_WIDTH;
                    level->observer_angle += angle_change;
                }

                if (move_y) {
                    double angle_change = ((double)move_y) * VERTICAL_ROTATION_CONSTANT / VIEWPORT_HEIGHT;
                    level->observer_angle2 = MIN(MAX(level->observer_angle2 - angle_change, 1.0), 179.0);
                }
            }
        }

        if (!paused) {
            if (sfMouse_isButtonPressed(sfMouseLeft)) {
                shooting_start_action();
            }

            window_center_mouse();
            window_update_pixels(fg_buffer);
            window_refresh();
        }
    }
}

static void open_level() {
    printf("Level start - press P to pause and Esc to quit\n");

    init_raycaster(level);
    init_game_buffers(level);
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

    if (!SKIP_LEVEL_SELECT && levels_listed > 1) {
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
    key_1_pressed = false;
    key_2_pressed = false;
    key_3_pressed = false;
    key_4_pressed = false;
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

    clear_keys_pressed();
}

int main() {
#if DEBUG
    printf("Running in debug mode\n");
#endif
    init_base_colors();
    init_fish_eye_table();
    load_textures();

    while (select_level()) {
        open_level();
#if DEBUG || SKIP_LEVEL_SELECT
        break;
#endif
        unload_assets();
    }

    printf("Goodbye\n");

    return EXIT_SUCCESS;
}
