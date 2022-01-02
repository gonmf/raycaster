#include "global.h"

static SDL_Keycode keys_pressed[KEYS_PRESSED_BUFFER_SIZE];
static unsigned int keys_pressed_count;
static bool mouse_left_key_pressed = false;

void add_key_pressed(SDL_Keycode code) {
    if (keys_pressed_count == KEYS_PRESSED_BUFFER_SIZE) {
        return;
    }

    for (unsigned int i = 0; i < keys_pressed_count; ++i) {
        if (keys_pressed[i] == code) {
            return;
        }
    }

    keys_pressed[keys_pressed_count] = code;
    keys_pressed_count += 1;
}

void remove_key_pressed(SDL_Keycode code) {
    for (unsigned int i = 0; i < keys_pressed_count; ++i) {
        if (keys_pressed[i] == code) {
            keys_pressed[i] = keys_pressed[keys_pressed_count - 1];
            keys_pressed_count -= 1;
            return;
        }
    }
}

bool key_is_pressed(SDL_Keycode code) {
    for (unsigned int i = 0; i < keys_pressed_count; ++i) {
        if (keys_pressed[i] == code) {
            return true;
        }
    }

    return false;
}

bool player_moving() {
    for (unsigned int i = 0; i < keys_pressed_count; ++i) {
        SDL_Keycode code = keys_pressed[i];

        if (code == SDLK_w || code == SDLK_s || code == SDLK_a || code == SDLK_d) {
            return true;
        }
    }

    return false;
}

void clear_keys_pressed() {
    keys_pressed_count = 0;
    mouse_left_key_pressed = false;
}

bool is_mouse_left_key_pressed() {
    return mouse_left_key_pressed;
}

void set_mouse_left_key_pressed(bool value) {
    mouse_left_key_pressed = value;
}
