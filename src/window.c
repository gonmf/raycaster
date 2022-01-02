#include "global.h"

static SDL_Renderer * renderer;
static SDL_Window * window;

unsigned int window_width;
unsigned int window_height;
unsigned int fullscreen_x_offset;
unsigned int fullscreen_y_offset;
double screen_ratio;

void set_cursor_visible(bool visible) {
    SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

static void window_start_windowed() {
    window_width = VIEWPORT_WIDTH;
    window_height = VIEWPORT_HEIGHT;
    screen_ratio = 1.0;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        error("Could not initialize SDL");
    }

    unsigned int desired_witdh = (unsigned int)(VIEWPORT_WIDTH * screen_ratio);
    unsigned desired_height = (unsigned int)(VIEWPORT_HEIGHT * screen_ratio);

    window = SDL_CreateWindow("Raycaster", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, desired_witdh, desired_height, 0);
    renderer = SDL_CreateRenderer(window, -1, 0);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    SDL_ShowCursor(SDL_DISABLE);

    SDL_SetRelativeMouseMode(SDL_TRUE);
}

static void window_start_fullscreen() {
    error("Unsupported");
}

void window_start() {
    if (is_fullscreen()) {
        window_start_fullscreen();
    } else {
        window_start_windowed();
    }
}

void window_close() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    window = NULL;
    renderer = NULL;
}

bool window_is_open() {
    // TODO: remove method
    return true;
}

void window_update_pixels(const pixel_t * pixels) {
    // TODO: take into consideration padding in fullscreen

    unsigned int i = 0;
    for (unsigned int y = 0; y < VIEWPORT_HEIGHT; ++y) {
        for (unsigned int x = 0; x < VIEWPORT_WIDTH; ++x) {
            pixel_t pixel = pixels[i++];

            SDL_SetRenderDrawColor(renderer, pixel.red, pixel.green, pixel.blue, SDL_ALPHA_OPAQUE);
            SDL_RenderDrawPoint(renderer, x, y);
        }
    }

    SDL_RenderPresent(renderer);
}

bool test_mouse_control() {
    // TODO: remove method
    return true;
}
