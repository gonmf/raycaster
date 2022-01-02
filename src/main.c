#include "global.h"

static level_t * level;
static unsigned int last_fps;
static bool action_btn_pressed = false;
static bool map_btn_pressed = false;
static bool key_1_pressed = false;
static bool key_2_pressed = false;
static bool key_3_pressed = false;
static bool key_4_pressed = false;
static bool key_up_pressed = false;
static bool key_down_pressed = false;
static bool key_enter_pressed = false;
static bool show_map = false;
static char * save_games[SAVE_FILES_COUNT];
static pixel_t * backup_buffer;
static bool show_warning_changes_will_only_apply_after_restart = false;

static void update_observer_state() {
    bool keyW = key_is_pressed(SDLK_w);
    bool keyS = key_is_pressed(SDLK_s);
    bool keyA = key_is_pressed(SDLK_a);
    bool keyD = key_is_pressed(SDLK_d);

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

        move_player(level, x_change, y_change);
    }
    if (keyS) {
        double angle_radians = level->observer_angle / RADIAN_CONSTANT;
        double x_change = sin(angle_radians) * mov_value;
        double y_change = cos(angle_radians) * mov_value;

        move_player(level, -x_change, -y_change);
    }
    if (keyA) {
        double angle_radians = fit_angle(level->observer_angle - 90.0) / RADIAN_CONSTANT;
        double x_change = sin(angle_radians) * (mov_value / 1.0);
        double y_change = cos(angle_radians) * (mov_value / 1.0);

        move_player(level, x_change, y_change);
    }
    if (keyD) {
        double angle_radians = fit_angle(level->observer_angle + 90.0) / RADIAN_CONSTANT;
        double x_change = sin(angle_radians) * (mov_value / 1.0);
        double y_change = cos(angle_radians) * (mov_value / 1.0);

        move_player(level, x_change, y_change);
    }

    if (key_is_pressed(SDLK_e)) {
        action_btn_pressed = true;
    } else {
        if (action_btn_pressed) {
            open_door_in_front(level);
            action_btn_pressed = false;
        }
    }
    if (key_is_pressed(SDLK_m)) {
        map_btn_pressed = true;
    } else {
        if (map_btn_pressed) {
            show_map = !show_map;
            map_btn_pressed = false;
        }
    }
    if (key_is_pressed(SDLK_1)) {
        key_1_pressed = true;
    } else {
        if (key_1_pressed) {
            switch_weapon(level, 0);
            key_1_pressed = false;
        }
    }
    if (key_is_pressed(SDLK_2)) {
        key_2_pressed = true;
    } else {
        if (key_2_pressed) {
            switch_weapon(level, 1);
            key_2_pressed = false;
        }
    }
    if (key_is_pressed(SDLK_3)) {
        key_3_pressed = true;
    } else {
        if (key_3_pressed) {
            switch_weapon(level, 2);
            key_3_pressed = false;
        }
    }
    if (key_is_pressed(SDLK_4)) {
        key_4_pressed = true;
    } else {
        if (key_4_pressed) {
            switch_weapon(level, 3);
            key_4_pressed = false;
        }
    }
}

static void start_color_to_screen_animation(pixel_t color, unsigned int duration) {
    double initial_duration = (double)duration;
    memcpy(backup_buffer, fg_buffer, VIEWPORT_WIDTH * VIEWPORT_HEIGHT * sizeof(pixel_t));

    while (true) {
        if (!window_is_open()) {
            exit(EXIT_SUCCESS);
        }

        if (duration-- == 0) {
            return;
        }

        double factor = duration / initial_duration;
        memcpy(fg_buffer, backup_buffer, VIEWPORT_WIDTH * VIEWPORT_HEIGHT * sizeof(pixel_t));
        scene_shading(color, factor);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                exit(EXIT_SUCCESS);
            }
        }

        window_update_pixels(fg_buffer);
    }
}

static void start_screen_to_color_animation(pixel_t color, unsigned int duration) {
    double initial_duration = (double)duration;
    memcpy(backup_buffer, fg_buffer, VIEWPORT_WIDTH * VIEWPORT_HEIGHT * sizeof(pixel_t));

    while (true) {
        if (!window_is_open()) {
            exit(EXIT_SUCCESS);
        }

        if (duration-- == 0) {
            return;
        }

        double factor = duration / initial_duration;
        memcpy(fg_buffer, backup_buffer, VIEWPORT_WIDTH * VIEWPORT_HEIGHT * sizeof(pixel_t));
        scene_shading(color, 1.0 - factor);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                exit(EXIT_SUCCESS);
            }
        }

        window_update_pixels(fg_buffer);
    }
}

static void write_text_ui() {
    char buffer[20];

    // Top left corner: Score
    screen_write("Score", UI_PADDING, UI_PADDING);
    snprintf(buffer, 20, "%05u", level->score);
    screen_write(buffer, UI_PADDING, UI_PADDING + font_sprites->sprite_size);

    // Top right corner: Level number and FPS
    snprintf(buffer, 20, "L%u", level->level_nr + 1);
    screen_write(buffer, VIEWPORT_WIDTH - UI_PADDING - strlen(buffer) * font_sprites->sprite_size, UI_PADDING);

    if (is_show_fps()) {
        snprintf(buffer, 20, "%u", last_fps);
        screen_write(buffer, VIEWPORT_WIDTH - UI_PADDING - strlen(buffer) * font_sprites->sprite_size, UI_PADDING + font_sprites->sprite_size);
    }

    // Bottom left corner: Health and lives
    snprintf(buffer, 20, "%3u%% ~ %u", level->life, level->lives);
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

static void render_menus_loop(unsigned int start_menu);
static bool game_logic_iteration(bool * trigger_shot) {
    if (level->life == 0) {
        start_screen_to_color_animation(color_dark_red, GAME_OVER_ANIMATION_SPEED);
        return true;
    }

    transition_step(level, trigger_shot);

    update_observer_state();

    update_enemies_state(level);

    bool exit_found;
    apply_special_effect(level, &exit_found);
    if (exit_found) {
        start_screen_to_color_animation(color_black, GAME_ENTER_EXIT_ANIMATION_SPEED);
        return true;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            exit(EXIT_SUCCESS);
        } else if (event.type == SDL_KEYDOWN) {
            SDL_Keycode code = ((SDL_KeyboardEvent *)&event)->keysym.sym;

            if (code == SDLK_ESCAPE) {
                render_menus_loop(3);
            } else {
                add_key_pressed(code);
            }
        } else if (event.type == SDL_KEYUP) {
            SDL_Keycode code = ((SDL_KeyboardEvent *)&event)->keysym.sym;

            remove_key_pressed(code);
        } else if (event.type == SDL_MOUSEMOTION) {
            int move_x = ((SDL_MouseMotionEvent *)&event)->xrel;
            int move_y = ((SDL_MouseMotionEvent *)&event)->yrel;

            if (move_x) {
                double angle_change = ((double)move_x) * ROTATION_CONSTANT * get_mouse_sensibility() / VIEWPORT_WIDTH;
                level->observer_angle += angle_change;
            }

            if (move_y && is_look_up_down()) {
                double angle_change = ((double)move_y) * ROTATION_CONSTANT * (get_mouse_sensibility() * 2) / VIEWPORT_HEIGHT;
                if (is_invert_mouse()) {
                    angle_change = -angle_change;
                }
                level->observer_angle2 = MIN(MAX(level->observer_angle2 - angle_change, 1.0), 179.0);
            }
        } else if (event.type == SDL_MOUSEBUTTONUP) {
            set_mouse_left_key_pressed(false);
        } else if (event.type == SDL_MOUSEBUTTONDOWN) {
            set_mouse_left_key_pressed(true);
        }
    }

    if (is_mouse_left_key_pressed()) {
        shooting_start_action(level);
    }

    return false;
}

static void main_render_loop() {
    unsigned int frames_second = 1;
    long unsigned int last_ms = 0;
    long unsigned int last_ctime = 0;
    struct timespec spec;

    if (window_is_open()) {
        start_color_to_screen_animation(color_black, GAME_ENTER_EXIT_ANIMATION_SPEED);
    }

    while (true) {
        if (!window_is_open()) {
            exit(EXIT_SUCCESS);
        }

        clock_gettime(CLOCK_REALTIME, &spec);
        long unsigned int ms = spec.tv_nsec / 1000000;

        if (last_ms > ms) {
            last_fps = frames_second;
            frames_second = 1;
        }

        frames_second += 1;
        last_ms = ms;

        long unsigned int ctime = spec.tv_sec * 1000 + ms;
        if (last_ctime == 0) {
            last_ctime = ctime;
        }

        long unsigned int elapsed = ctime - last_ctime;
        long unsigned int logic_cycles = elapsed / GAME_LOGIC_CYCLE_STEP;
        last_ctime += logic_cycles * GAME_LOGIC_CYCLE_STEP;

        // Advance game state one or more steps
        bool trigger_shot = false;
        for (long unsigned int cy = 0; cy < logic_cycles; ++cy) {
            if (game_logic_iteration(&trigger_shot)) {
                return;
            }
        }

        // Paint graphics and UI elements
        paint_scene(level, trigger_shot);
        write_text_ui();

        double flash_percentage;
        pixel_t flash_color;
        if (short_flash_effect(&flash_percentage, &flash_color)) {
            scene_shading(flash_color, flash_percentage);
        }

        if (show_map) {
            paint_map(level);
        }

        window_update_pixels(fg_buffer);
    }
}

static void open_level() {
    init_animations();
    init_raycaster(level);
    init_game_buffers(level);
    paint_scene(level, false);

    main_render_loop();
}

static void unload_assets() {
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
        free(level->object);
        free(level->enemy);
        free(level);
        level = NULL;
    }

    clear_keys_pressed();
}

static void advance_level() {
    level_t * new_level = read_level_info(level->level_nr + 1);

    new_level->score = level->score;
    new_level->level_start_score = level->score;
    new_level->life = level->life;
    new_level->lives = level->lives;
    new_level->key_1 = false;
    new_level->key_2 = false;
    new_level->weapon = level->weapon;
    new_level->weapons_available = level->weapons_available;
    new_level->ammo = level->ammo;
    new_level->level_nr = level->level_nr + 1;

    unload_assets();

    level = new_level;
}

static void game_level_loop() {
    char buffer[64];
    file_read(buffer, 64, "levels/all.info");
    int levels = atoi(buffer);
    if (levels <= 0 || levels > 99) {
        error("Invalid number of levels");
    }

    while (level->level_nr < (unsigned int)levels) {
        open_level();

        if (level->life == 0) {
            if (level->lives == 0) {
                break;
            } else {
                unsigned int level_nr = level->level_nr;
                unsigned int lives = level->lives - 1;
                unsigned int level_start_score = level->level_start_score;

                unload_assets();

                level = read_level_info(level_nr);

                level->score = level_start_score;
                level->level_start_score = level_start_score;
                level->life = 100;
                level->lives = lives;
                level->key_1 = false;
                level->key_2 = false;
                level->weapon = 1;
                level->weapons_available = 3;
                level->ammo = 10;
                level->level_nr = level_nr;
            }
        } else {
            if (level->level_nr == (unsigned int)(levels - 1)) {
                break;
            }
            advance_level();
        }
    }
}

static void start_new_game() {
    unload_assets();

    level = read_level_info(0);
    level->score = 0;
    level->level_start_score = 0;
    level->life = 100;
    level->lives = 3;
    level->key_1 = false;
    level->key_2 = false;
    level->weapon = 1;
    level->weapons_available = 3;
    level->ammo = 10;
    level->level_nr = 0;

    start_screen_to_color_animation(color_black, GAME_ENTER_EXIT_ANIMATION_SPEED);
    game_level_loop();
}

static void continue_game(unsigned char slot_nr) {
    level_t * new_level = load_game_state(slot_nr);

    if (new_level) {
        unload_assets();
        level = new_level;

        start_screen_to_color_animation(color_black, GAME_ENTER_EXIT_ANIMATION_SPEED);
        game_level_loop();
    }
}

static void render_main_menu(unsigned int current_option, double shading_factor) {
    for (unsigned int y = 0; y < VIEWPORT_HEIGHT; ++y) {
        for (unsigned int x = 0; x < VIEWPORT_WIDTH; ++x) {
            fg_buffer[x + y * VIEWPORT_WIDTH] = color_dark_blue;
        }
    }

    unsigned int scale = 8;
    unsigned int chr_siz = font_sprites->sprite_size * scale;
    screen_write_scaled("RAYCASTER", VIEWPORT_WIDTH / 2 - chr_siz * strlen("RAYCASTER") / 2, VIEWPORT_HEIGHT / 2 - chr_siz * 2, scale, 0.0);
    scale = 4;
    chr_siz = font_sprites->sprite_size * scale;
    unsigned int y_offset = VIEWPORT_HEIGHT / 2 - chr_siz;
    screen_write_scaled("Load Game", VIEWPORT_WIDTH / 2 - chr_siz * strlen("Load Game") / 2, y_offset, scale, current_option == 0 ? shading_factor : 0.0);
    y_offset += chr_siz + 8;
    screen_write_scaled("New Game", VIEWPORT_WIDTH / 2 - chr_siz * strlen("New Game") / 2, y_offset, scale, current_option == 1 ? shading_factor : 0.0);
    y_offset += chr_siz + 8;
    screen_write_scaled("Options", VIEWPORT_WIDTH / 2 - chr_siz * strlen("Options") / 2, y_offset, scale, current_option == 2 ? shading_factor : 0.0);
    y_offset += chr_siz + 8;
    screen_write_scaled("Quit", VIEWPORT_WIDTH / 2 - chr_siz * strlen("Quit") / 2, y_offset, scale, current_option == 3 ? shading_factor : 0.0);

    window_update_pixels(fg_buffer);
}

static void render_options_menu(unsigned int current_option, double shading_factor) {
    for (unsigned int y = 0; y < VIEWPORT_HEIGHT; ++y) {
        for (unsigned int x = 0; x < VIEWPORT_WIDTH; ++x) {
            fg_buffer[x + y * VIEWPORT_WIDTH] = color_dark_blue;
        }
    }

    char buf[24];
    unsigned int y_offset = VIEWPORT_HEIGHT / 5;
    unsigned int scale = 4;
    unsigned int chr_siz = font_sprites->sprite_size * scale;
    screen_write_scaled("OPTIONS", VIEWPORT_WIDTH / 2 - chr_siz * strlen("OPTIONS") / 2, y_offset, scale, 0.0);
    y_offset += chr_siz + chr_siz / 2;
    scale = 3;
    chr_siz = font_sprites->sprite_size * scale;
    char * opt_str = is_fullscreen() ? "Display: fullscreen" : "Display: windowed";
    screen_write_scaled(opt_str, VIEWPORT_WIDTH / 2 - chr_siz * strlen(opt_str) / 2, y_offset, scale, current_option == 0 ? shading_factor : 0.0);
    y_offset += chr_siz + chr_siz / 2;
    opt_str = is_show_fps() ? "Display FPS: yes" : "Display FPS: no";
    screen_write_scaled(opt_str, VIEWPORT_WIDTH / 2 - chr_siz * strlen(opt_str) / 2, y_offset, scale, current_option == 1 ? shading_factor : 0.0);
    y_offset += chr_siz + chr_siz / 2;
    opt_str = is_look_up_down() ? "Look up/down: yes" : "Look up/down: no";
    screen_write_scaled(opt_str, VIEWPORT_WIDTH / 2 - chr_siz * strlen(opt_str) / 2, y_offset, scale, current_option == 2 ? shading_factor : 0.0);
    y_offset += chr_siz + chr_siz / 2;
    opt_str = is_invert_mouse() ? "Invert mouse: yes" : "Invert mouse: no";
    screen_write_scaled(opt_str, VIEWPORT_WIDTH / 2 - chr_siz * strlen(opt_str) / 2, y_offset, scale, current_option == 3 ? shading_factor : 0.0);
    y_offset += chr_siz + chr_siz / 2;
    snprintf(buf, 24, "Mouse sensibility: %u", get_mouse_sensibility());
    screen_write_scaled(buf, VIEWPORT_WIDTH / 2 - chr_siz * strlen(buf) / 2, y_offset, scale, current_option == 4 ? shading_factor : 0.0);
    y_offset += chr_siz + chr_siz / 2;
    screen_write_scaled("Back", VIEWPORT_WIDTH / 2 - chr_siz * strlen("Back") / 2, y_offset, scale, current_option == 5 ? shading_factor : 0.0);

    if (show_warning_changes_will_only_apply_after_restart) {
        scale = 2;
        chr_siz = font_sprites->sprite_size * scale;
        screen_write_scaled("Option requires restarting the game", VIEWPORT_WIDTH / 2 - chr_siz * strlen("Option requires restarting the game") / 2, VIEWPORT_HEIGHT - chr_siz - 8, scale, 0.0);
    }

    window_update_pixels(fg_buffer);
}

static void render_pause_menu(unsigned int current_option, double shading_factor) {
    for (unsigned int y = 0; y < VIEWPORT_HEIGHT; ++y) {
        for (unsigned int x = 0; x < VIEWPORT_WIDTH; ++x) {
            fg_buffer[x + y * VIEWPORT_WIDTH] = color_dark_blue;
        }
    }

    unsigned int y_offset = VIEWPORT_HEIGHT / 4;
    unsigned int scale = 4;
    unsigned int chr_siz = font_sprites->sprite_size * scale;
    screen_write_scaled("PAUSED", VIEWPORT_WIDTH / 2 - chr_siz * strlen("PAUSED") / 2, y_offset, scale, 0.0);
    y_offset += chr_siz + chr_siz / 2;
    scale = 2;
    chr_siz = font_sprites->sprite_size * scale;
    screen_write_scaled("Resume", VIEWPORT_WIDTH / 2 - chr_siz * strlen("Resume") / 2, y_offset, scale, current_option == 0 ? shading_factor : 0.0);
    y_offset += chr_siz + chr_siz / 2;
    screen_write_scaled("Save Game", VIEWPORT_WIDTH / 2 - chr_siz * strlen("Save Game") / 2, y_offset, scale, current_option == 1 ? shading_factor : 0.0);
    y_offset += chr_siz + chr_siz / 2;
    screen_write_scaled("Quit", VIEWPORT_WIDTH / 2 - chr_siz * strlen("Quit") / 2, y_offset, scale, current_option == 2 ? shading_factor : 0.0);

    window_update_pixels(fg_buffer);
}

static void render_load_save_menu(unsigned int current_option, double shading_factor, bool is_load) {
    char buffer[SAVE_FILE_NAME_SIZ];

    for (unsigned int y = 0; y < VIEWPORT_HEIGHT; ++y) {
        for (unsigned int x = 0; x < VIEWPORT_WIDTH; ++x) {
            fg_buffer[x + y * VIEWPORT_WIDTH] = color_dark_blue;
        }
    }

    unsigned int y_offset = 64;
    unsigned int scale = 4;
    unsigned int chr_siz = font_sprites->sprite_size * scale;
    screen_write_scaled(is_load ? "LOAD GAME" : "SAVE GAME", VIEWPORT_WIDTH / 2 - chr_siz * strlen(is_load ? "LOAD GAME" : "SAVE GAME") / 2, y_offset, scale, 0.0);
    y_offset += chr_siz + chr_siz / 2;
    scale = 2;
    chr_siz = font_sprites->sprite_size * scale;
    for (unsigned int i = 0; i < SAVE_FILES_COUNT; ++i) {
        if (save_games[i]) {
            strcpy(buffer, save_games[i]);
        } else {
            snprintf(buffer, SAVE_FILE_NAME_SIZ, "empty slot %u", i + 1);
        }
        screen_write_scaled(buffer, VIEWPORT_WIDTH / 2 - chr_siz * strlen(buffer) / 2, y_offset, scale, current_option == i ? shading_factor : 0.0);
        y_offset += chr_siz + chr_siz / 2;
    }

    screen_write_scaled("Quit", VIEWPORT_WIDTH / 2 - chr_siz * strlen("Quit") / 2, y_offset, scale, current_option == SAVE_FILES_COUNT ? shading_factor : 0.0);

    window_update_pixels(fg_buffer);
}

static void reload_save_games() {
    char buffer[SAVE_FILE_NAME_SIZ];

    for (unsigned int i = 0; i < SAVE_FILES_COUNT; ++i) {
        if (save_games[i]) {
            free(save_games[i]);
            save_games[i] = NULL;
        }

        if (read_game_state_name(i, buffer)) {
            save_games[i] = calloc(SAVE_FILE_NAME_SIZ, sizeof(char));
            strcpy(save_games[i], buffer);
        }
    }
}

static void render_menus_loop(unsigned int start_menu) {
    clear_keys_pressed();
    unsigned int animation_step = 0;
    unsigned int current_option = 0;
    unsigned int current_menu = start_menu;

    while (true) {
        animation_step = (animation_step + 1) % 100;
        double shading_factor = (animation_step < 50 ? animation_step : 100 - animation_step) / 80.0;

        switch (current_menu) {
            case 0: // main menu
                render_main_menu(current_option, shading_factor);
                break;
            case 1: // load game
                render_load_save_menu(current_option, shading_factor, true);
                break;
            case 2: // options
                render_options_menu(current_option, shading_factor);
                break;
            case 3: // game paused
                render_pause_menu(current_option, shading_factor);
                break;
            case 4: // game paused - save menu
                render_load_save_menu(current_option, shading_factor, false);
        }

        if (key_is_pressed(SDLK_UP)) {
            key_up_pressed = true;
        } else {
            if (key_up_pressed) {
                animation_step = 35;
                if (current_menu == 1) {
                    if (current_option > 0) {
                        do {
                            current_option -= 1;
                        } while (current_option > 0 && !save_games[current_option]);
                    }
                } else {
                    if (current_option > 0) {
                        current_option -= 1;
                    }
                }
                key_up_pressed = false;
            }
        }
        if (key_is_pressed(SDLK_DOWN)) {
            key_down_pressed = true;
        } else {
            if (key_down_pressed) {
                animation_step = 35;
                switch (current_menu) {
                    case 0:
                        if (current_option < 3) {
                            current_option += 1;
                        }
                        break;
                    case 1:
                        if (current_option < SAVE_FILES_COUNT) {
                            do {
                                current_option += 1;
                            } while (current_option < SAVE_FILES_COUNT && !save_games[current_option]);
                        }
                        break;
                    case 2:
                        if (current_option < 5) {
                            current_option += 1;
                        }
                        break;
                    case 3:
                        if (current_option < 2) {
                            current_option += 1;
                        }
                        break;
                    case 4:
                        if (current_option < SAVE_FILES_COUNT) {
                            current_option += 1;
                        }
                }
                key_down_pressed = false;
            }
        }
        if (key_is_pressed(SDLK_RETURN)) {
            key_enter_pressed = true;
        } else {
            if (key_enter_pressed) {
                key_enter_pressed = false;
                switch (current_menu) {
                    case 0:
                        switch (current_option) {
                            case 0:
                                current_menu = 1;
                                current_option = 0;
                                break;
                            case 1:
                                start_new_game();
                                current_menu = 0;
                                current_option = 0;
                                break;
                            case 2:
                                current_menu = 2;
                                current_option = 0;
                                break;
                            case 3:
                                exit(EXIT_SUCCESS);
                        }
                        break;
                    case 1:
                        if (current_option < SAVE_FILES_COUNT) {
                            continue_game(current_option);
                        }
                        current_menu = 0;
                        current_option = 0;
                        break;
                    case 2:
                        switch (current_option) {
                            case 0:
                                show_warning_changes_will_only_apply_after_restart = true;
                                toggle_fullscreen();
                                break;
                            case 1:
                                toggle_show_fps();
                                break;
                            case 2:
                                toggle_look_up_down();
                                break;
                            case 3:
                                toggle_invert_mouse();
                                break;
                            case 4:
                                increase_mouse_sensibility();
                                break;
                            case 5:
                                show_warning_changes_will_only_apply_after_restart = false;
                                current_menu = 0;
                                current_option = 0;
                        }
                        break;
                    case 3:
                        switch (current_option) {
                            case 0:
                                return;
                            case 1:
                                current_menu = 4;
                                current_option = 0;
                                break;
                            case 2:
                                current_menu = 0;
                                current_option = 0;
                        }
                        break;
                    case 4:
                        if (current_option < SAVE_FILES_COUNT) {
                            save_game_state(current_option, level);
                            reload_save_games();
                        }
                        current_menu = 3;
                        current_option = 0;
                }
            }
        }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                exit(EXIT_SUCCESS);
            } else if (event.type == SDL_KEYDOWN) {
                SDL_Keycode code = ((SDL_KeyboardEvent *)&event)->keysym.sym;

                add_key_pressed(code);
            } else if (event.type == SDL_KEYUP) {
                SDL_Keycode code = ((SDL_KeyboardEvent *)&event)->keysym.sym;

                remove_key_pressed(code);
            }
        }
    }
}

static void mouse_unavailable_warning() {
    fprintf(stderr, "Warning: unable to test mouse position control\n");
    fprintf(stderr, "On some operating systems, like macOS, you may need to grant this program the permission to control the mouse position.\n");
}

int main() {
    init_base_colors();
    init_fish_eye_table();
    load_textures();
    load_user_options();
    reload_save_games();

    window_start();

    fg_buffer = calloc(window_width * window_height, sizeof(pixel_t));
    backup_buffer = malloc(window_width * window_height * sizeof(pixel_t));

    if (!test_mouse_control()) {
        mouse_unavailable_warning();
    }

    render_menus_loop(0);
    window_close();

    return EXIT_SUCCESS;
}
