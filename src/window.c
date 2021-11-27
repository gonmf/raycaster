#include "global.h"

static sfRenderWindow * window;
static sfSprite * sprite;
static sfTexture * texture;
static sfRenderStates renderStates;

void window_center_mouse() {
    sfVector2i position;
    position.x = VIEWPORT_WIDTH / 2;
    position.y = VIEWPORT_HEIGHT / 2;
    sfMouse_setPositionRenderWindow(position, window);
}

void set_cursor_visible(bool visible) {
    sfRenderWindow_setMouseCursorVisible(window, visible);
}

void window_start() {
    sfVideoMode screen_video_mode;
    screen_video_mode = sfVideoMode_getDesktopMode();
    if (screen_video_mode.width < WINDOW_TOTAL_WIDTH || screen_video_mode.height < WINDOW_TOTAL_HEIGHT) {
        error("Screen resolution too small for viewport");
    }

    sfVideoMode desired_video_mode;
    desired_video_mode.width = (unsigned int)(WINDOW_TOTAL_WIDTH * WINDOW_UPSCALE);
    desired_video_mode.height = (unsigned int)(WINDOW_TOTAL_HEIGHT * WINDOW_UPSCALE);
    desired_video_mode.bitsPerPixel = 32;

    window = sfRenderWindow_create(desired_video_mode, "Raycaster", sfClose, NULL);
    sfRenderWindow_setFramerateLimit(window, MAX_FPS);

    sfVector2i position;
    position.x = screen_video_mode.width / 2 - desired_video_mode.width / 2;
    position.y = screen_video_mode.height / 2 - desired_video_mode.height / 2;
    sfRenderWindow_setPosition(window, position);

    set_cursor_visible(false);
    window_center_mouse();

    texture = sfTexture_create((unsigned int)(WINDOW_TOTAL_WIDTH * WINDOW_UPSCALE), (unsigned int)(WINDOW_TOTAL_HEIGHT * WINDOW_UPSCALE));
    sprite = sfSprite_create();
    sfSprite_setTexture(sprite, texture, sfFalse);

    sfColor bgColor;
    memset(&bgColor, 0, sizeof(bgColor));

    sfTransform transform = sfTransform_Identity;
    sfTransform_scale(&transform, WINDOW_UPSCALE, WINDOW_UPSCALE);

    renderStates.blendMode = sfBlendNone;
    renderStates.transform = transform;
    renderStates.texture = texture;
    renderStates.shader = NULL;
}

void window_close() {
    sfRenderWindow_close(window);
    window = NULL;
}

bool window_is_open() {
    return window && sfRenderWindow_isOpen(window);
}

void window_update_pixels(const pixel_t * pixels, unsigned int width, unsigned int height, unsigned int offset_x, unsigned int offset_y) {
    sfTexture_updateFromPixels(texture, (sfUint8 *)pixels, width, height, offset_x, offset_y);
    sfRenderWindow_drawSprite(window, sprite, &renderStates);
}

void window_refresh() {
    sfRenderWindow_display(window);
}

bool window_poll_event(sfEvent * event) {
    return sfRenderWindow_pollEvent(window, event);
}
