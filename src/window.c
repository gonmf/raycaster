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
    renderStates.blendMode = sfBlendNone;
    renderStates.transform = sfTransform_Identity;
    renderStates.texture = texture;
    renderStates.shader = NULL;

    sfVideoMode videoMode;
    videoMode.width = VIEWPORT_WIDTH;
    videoMode.height = VIEWPORT_HEIGHT;
    videoMode.bitsPerPixel = 32;

    window = sfRenderWindow_create(videoMode, "Raycaster", sfResize | sfClose, NULL);
    sfRenderWindow_setFramerateLimit(window, MAX_FPS);

    videoMode = sfVideoMode_getDesktopMode();
    sfVector2i position;
    position.x = videoMode.width / 2 - VIEWPORT_WIDTH / 2;
    position.y = videoMode.height / 2 - VIEWPORT_HEIGHT / 2;

    sfRenderWindow_setPosition(window, position);
    set_cursor_visible(false);
    window_center_mouse();

    texture = sfTexture_create(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    sprite = sfSprite_create();
    sfSprite_setTexture(sprite, texture, sfFalse);

    sfColor bgColor;
    memset(&bgColor, 0, sizeof(bgColor));

    renderStates.blendMode = sfBlendNone;
    renderStates.transform = sfTransform_Identity;
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

void window_update_pixels(const pixel_t * pixels) {
    sfTexture_updateFromPixels(texture, (sfUint8 *)pixels, VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 0, 0);
    sfRenderWindow_drawSprite(window, sprite, &renderStates);
}

void window_refresh() {
    sfRenderWindow_display(window);
}

bool window_poll_event(sfEvent * event) {
    return sfRenderWindow_pollEvent(window, event);
}
