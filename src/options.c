#include "global.h"

static bool fullscreen;
static bool look_up_down;
static bool show_fps;

static void save_user_options() {
    FILE * file = fopen("local.options", "w");
    if (file == NULL) {
        error("Could not open file local.options for writing");
    }

    fprintf(file, "fullscreen: %u\n", fullscreen ? 1 : 0);
    fprintf(file, "look_up_down: %u\n", look_up_down ? 1 : 0);
    fprintf(file, "show_fps: %u\n", show_fps ? 1 : 0);

    fclose(file);
}

void load_user_options() {
    FILE * file = fopen("local.options", "r");
    if (file == NULL) {
        fullscreen = true;
        look_up_down = true;
        show_fps = false;

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
