#include "global.h"

static bool sdl_inited = false;
static SDL_Renderer * renderer;
static SDL_Window * window;
static SDL_Texture * texture;
static SDL_Rect dst_rect;

void set_cursor_visible(bool visible) {
    SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

static void window_start_windowed() {
    double screen_ratio = 1.0;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        error("Could not initialize SDL");
    }
    sdl_inited = true;

    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm)) {
        error("Could not read display information");
    }
    if (dm.w < VIEWPORT_WIDTH || dm.h < VIEWPORT_HEIGHT) {
        error("Screen resolution too small for viewport");
    }

    while (true) {
        double new_screen_ratio = screen_ratio + 0.5;
        double new_width = (unsigned int)(VIEWPORT_WIDTH * new_screen_ratio);
        double new_height = (unsigned int)(VIEWPORT_HEIGHT * new_screen_ratio);

        if (new_width > dm.w || new_height > dm.h) {
            break;
        }

        screen_ratio = new_screen_ratio;
    }

    unsigned int window_width = (unsigned int)(VIEWPORT_WIDTH * screen_ratio);
    unsigned int window_height = (unsigned int)(VIEWPORT_HEIGHT * screen_ratio);

    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.w = window_width;
    dst_rect.h = window_height;

    window = SDL_CreateWindow("Raycaster", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, 0);
    renderer = SDL_CreateRenderer(window, -1, 0);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32 | SDL_PACKEDORDER_RGBA, SDL_TEXTUREACCESS_STREAMING, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    SDL_ShowCursor(SDL_DISABLE);

    SDL_SetRelativeMouseMode(SDL_TRUE);
}

static void window_start_fullscreen() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        error("Could not initialize SDL");
    }
    sdl_inited = true;

    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm)) {
        error("Could not read display information");
    }
    if (dm.w < VIEWPORT_WIDTH || dm.h < VIEWPORT_HEIGHT) {
        error("Screen resolution too small for viewport");
    }

    double width_ratio = dm.w / ((double)VIEWPORT_WIDTH);
    double height_ratio = dm.h / ((double)VIEWPORT_HEIGHT);
    double screen_ratio = MIN(width_ratio, height_ratio);

    unsigned int window_width = dm.w;
    unsigned int window_height = dm.h;
    unsigned int fullscreen_x_offset = window_width / 2 - (unsigned int)(VIEWPORT_WIDTH * screen_ratio / 2.0);
    unsigned int fullscreen_y_offset = window_height / 2 - (unsigned int)(VIEWPORT_HEIGHT * screen_ratio / 2.0);

    dst_rect.x = fullscreen_x_offset;
    dst_rect.y = fullscreen_y_offset;
    dst_rect.w = window_width - fullscreen_x_offset * 2;
    dst_rect.h = window_height - fullscreen_y_offset * 2;

    window = SDL_CreateWindow("Raycaster", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, SDL_WINDOW_FULLSCREEN);
    renderer = SDL_CreateRenderer(window, -1, 0);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32 | SDL_PACKEDORDER_RGBA, SDL_TEXTUREACCESS_STREAMING, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    SDL_ShowCursor(SDL_DISABLE);

    SDL_SetRelativeMouseMode(SDL_TRUE);
}

void window_start() {
    if (sdl_inited) {
        error("SDL already initialized");
    }

    if (is_fullscreen()) {
        window_start_fullscreen();
    } else {
        window_start_windowed();
    }
}

void window_close() {
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    if (sdl_inited) {
        SDL_Quit();
        sdl_inited = false;
    }
}

void window_update_pixels(const pixel_t * pixels) {
    SDL_UpdateTexture(texture, NULL, pixels, VIEWPORT_WIDTH * 4);

    SDL_RenderCopy(renderer, texture, NULL, &dst_rect);

    SDL_RenderPresent(renderer);
}
