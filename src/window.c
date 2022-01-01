#include "global.h"

static sfRenderWindow * window;
static sfSprite * sprite;
static sfTexture * texture;
static sfRenderStates renderStates;

unsigned int window_width;
unsigned int window_height;
unsigned int mid_real_window_width;
unsigned int mid_real_window_height;
unsigned int fullscreen_x_offset;
unsigned int fullscreen_y_offset;
double screen_ratio;

void window_center_mouse() {
    sfVector2i position;
    position.x = (unsigned int)(window_width * screen_ratio / 2.0);
    position.y = (unsigned int)(window_height * screen_ratio / 2.0);
    sfMouse_setPositionRenderWindow(position, window);
}

void set_cursor_visible(bool visible) {
    sfRenderWindow_setMouseCursorVisible(window, visible);
}

static void window_start_windowed() {
    window_width = VIEWPORT_WIDTH;
    window_height = VIEWPORT_HEIGHT;
    screen_ratio = 4.0;

    sfVideoMode screen_video_mode;
    screen_video_mode = sfVideoMode_getDesktopMode();

    while (true) {
        if (screen_video_mode.width >= (unsigned int)round(VIEWPORT_WIDTH * screen_ratio) && screen_video_mode.height >= (unsigned int)round(VIEWPORT_HEIGHT * screen_ratio)) {
            break;
        }

        screen_ratio -= 0.5;

        if (screen_ratio < 1.2) {
            error("Screen resolution too small for viewport");
        }
    }

    sfVideoMode desired_video_mode;
    desired_video_mode.width = (unsigned int)(VIEWPORT_WIDTH * screen_ratio);
    desired_video_mode.height = (unsigned int)(VIEWPORT_HEIGHT * screen_ratio);
    desired_video_mode.bitsPerPixel = 32;

    window = sfRenderWindow_create(desired_video_mode, "Raycaster", sfResize, NULL);
    sfRenderWindow_setFramerateLimit(window, MAX_FPS);

    sfVector2i position;
    position.x = screen_video_mode.width / 2 - desired_video_mode.width / 2;
    position.y = screen_video_mode.height / 2 - desired_video_mode.height / 2;
    sfRenderWindow_setPosition(window, position);

    set_cursor_visible(false);
    window_center_mouse();

    texture = sfTexture_create((unsigned int)(VIEWPORT_WIDTH * screen_ratio), (unsigned int)(VIEWPORT_HEIGHT * screen_ratio));
    sprite = sfSprite_create();
    sfSprite_setTexture(sprite, texture, sfFalse);

    sfTransform transform = sfTransform_Identity;
    sfTransform_scale(&transform, screen_ratio, screen_ratio);

    renderStates.blendMode = sfBlendNone;
    renderStates.transform = transform;
    renderStates.texture = texture;
    renderStates.shader = NULL;
}

static void window_start_fullscreen() {
    sfVideoMode screen_video_mode;
    screen_video_mode = sfVideoMode_getDesktopMode();
    if (screen_video_mode.width < VIEWPORT_WIDTH || screen_video_mode.height < VIEWPORT_HEIGHT) {
        error("Screen resolution too small for viewport");
    }

    double width_ratio = screen_video_mode.width / ((double)VIEWPORT_WIDTH);
    double height_ratio = screen_video_mode.height / ((double)VIEWPORT_HEIGHT);
    screen_ratio = MIN(width_ratio, height_ratio);

    window_width = (unsigned int)round(screen_video_mode.width / screen_ratio);
    window_height = (unsigned int)round(screen_video_mode.height / screen_ratio);
    fullscreen_x_offset = (((unsigned int)window_width) - VIEWPORT_WIDTH) / 2;
    fullscreen_y_offset = (((unsigned int)window_height) - VIEWPORT_HEIGHT) / 2;

    sfVideoMode desired_video_mode;
    desired_video_mode.width = screen_video_mode.width;
    desired_video_mode.height = screen_video_mode.height;
    desired_video_mode.bitsPerPixel = 32;

    window = sfRenderWindow_create(desired_video_mode, "Raycaster", sfFullscreen, NULL);
    sfRenderWindow_setFramerateLimit(window, MAX_FPS);

    set_cursor_visible(false);
    window_center_mouse();

    texture = sfTexture_create(window_width, window_height);
    sprite = sfSprite_create();
    sfSprite_setTexture(sprite, texture, sfFalse);

    sfColor bgColor;
    memset(&bgColor, 0, sizeof(bgColor));

    sfTransform transform = sfTransform_Identity;
    sfTransform_scale(&transform, screen_ratio, screen_ratio);

    renderStates.blendMode = sfBlendNone;
    renderStates.transform = transform;
    renderStates.texture = texture;
    renderStates.shader = NULL;

    sfUint8 * tmp = calloc(window_width * window_height * (32 / 8), sizeof(sfUint8));
    sfTexture_updateFromPixels(texture, tmp, window_width, window_height, 0, 0);
    free(tmp);
    sfRenderWindow_drawSprite(window, sprite, &renderStates);
    sfRenderWindow_display(window);
}

void window_start() {
    if (is_fullscreen()) {
        window_start_fullscreen();
    } else {
        window_start_windowed();
    }

    mid_real_window_width = (unsigned int)(window_width * screen_ratio / 2.0);
    mid_real_window_height = (unsigned int)(window_height * screen_ratio / 2.0);
}

void window_close() {
    sfRenderWindow_close(window);
    window = NULL;
}

bool window_is_open() {
    return window && sfRenderWindow_isOpen(window);
}

void window_update_pixels(const pixel_t * pixels) {
    sfTexture_updateFromPixels(texture, (sfUint8 *)pixels, VIEWPORT_WIDTH, VIEWPORT_HEIGHT, fullscreen_x_offset, fullscreen_y_offset);
    sfRenderWindow_drawSprite(window, sprite, &renderStates);
    sfRenderWindow_display(window);
}

bool window_poll_event(sfEvent * event) {
    return sfRenderWindow_pollEvent(window, event);
}

bool test_mouse_control() {
    sfVector2i obtained = sfMouse_getPositionRenderWindow(window);

    sfVector2i position;
    position.x = obtained.x + 1;
    position.y = obtained.y + 1;
    sfMouse_setPositionRenderWindow(position, window);

    obtained = sfMouse_getPositionRenderWindow(window);

    return obtained.x == position.x && obtained.y == position.y;
}
