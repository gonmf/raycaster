#include "global.h"

static bool fullscreen = true;
static bool look_up_down = true;
static bool show_fps = false;
static bool invert_mouse = false;
static unsigned char mouse_sensibility = 4;

static void save_user_options() {
    FILE * file = fopen("local.options", "w");
    if (file == NULL) {
        error("Could not open file local.options for writing");
    }

    fprintf(file, "fullscreen: %u\n", fullscreen ? 1 : 0);
    fprintf(file, "look_up_down: %u\n", look_up_down ? 1 : 0);
    fprintf(file, "show_fps: %u\n", show_fps ? 1 : 0);
    fprintf(file, "invert_mouse: %u\n", invert_mouse ? 1 : 0);
    fprintf(file, "mouse_sensibility: %u\n", mouse_sensibility);

    fclose(file);
}

void load_user_options() {
    FILE * file = fopen("local.options", "r");
    if (file == NULL) {
        save_user_options();
        return;
    }

    size_t buffer_siz = 20;
    char * buffer = malloc(buffer_siz);

    while (getline(&buffer, &buffer_siz, file) != -1) {
        bool val = (strstr(buffer, ": 1") != NULL);
        if (start_with(buffer, "fullscreen:")) {
            fullscreen = val;
        } else if (start_with(buffer, "look_up_down:")) {
            look_up_down = val;
        } else if (start_with(buffer, "show_fps:")) {
            show_fps = val;
        } else if (start_with(buffer, "invert_mouse:")) {
            invert_mouse = val;
        } else if (start_with(buffer, "mouse_sensibility:")) {
            mouse_sensibility = MIN(MAX(atoi(buffer + strlen("mouse_sensibility: ")), 1), 8);
        } else {
            error("Unexpected local.options file format");
        }
    }

    free(buffer);
    fclose(file);
}

bool is_fullscreen() {
    return fullscreen;
}

void toggle_fullscreen() {
    fullscreen = !fullscreen;

    save_user_options();

    window_close();
    window_start();
}

bool is_look_up_down() {
    return look_up_down;
}

void toggle_look_up_down() {
    look_up_down = !look_up_down;

    save_user_options();
}

bool is_show_fps() {
    return show_fps;
}

void toggle_show_fps() {
    show_fps = !show_fps;

    save_user_options();
}

bool is_invert_mouse() {
    return invert_mouse;
}

void toggle_invert_mouse() {
    invert_mouse = !invert_mouse;

    save_user_options();
}

unsigned char get_mouse_sensibility() {
    return mouse_sensibility;
}

void increase_mouse_sensibility() {
    mouse_sensibility++;
    if (mouse_sensibility > 8) {
        mouse_sensibility = 1;
    }

    save_user_options();
}
