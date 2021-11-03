#include "global.h"

static sfKeyCode keys_pressed[KEYS_PRESSED_BUFFER_SIZE];
static unsigned char keys_pressed_count;

void add_key_pressed(sfKeyCode code) {
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

void remove_key_pressed(sfKeyCode code) {
    for (int i = 0; i < keys_pressed_count; ++i) {
        if (keys_pressed[i] == code) {
            keys_pressed[i] = keys_pressed[keys_pressed_count - 1];
            keys_pressed_count -= 1;
            return;
        }
    }
}

int key_is_pressed(sfKeyCode code) {
    for (int i = 0; i < keys_pressed_count; ++i) {
        if (keys_pressed[i] == code) {
            return 1;
        }
    }

    return 0;
}
