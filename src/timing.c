#include "global.h"

static long unsigned int last_logic_ms_time;
static long unsigned int last_render_us_time;

#define US_PER_FRAME ((unsigned int long)(1000000 / MAX_FPS))

bool limit_fps(long unsigned int us_time) {
    if (last_render_us_time + US_PER_FRAME <= us_time) {
        last_render_us_time = us_time;
        return false;
    } else {
        SDL_Delay(1);
        return true;
    }
}

unsigned int game_logic_cycles(long unsigned int ms_time) {
    if (last_logic_ms_time == 0) {
        last_logic_ms_time = ms_time;
    }

    long unsigned int elapsed = ms_time - last_logic_ms_time;
    long unsigned int logic_cycles = elapsed / GAME_LOGIC_CYCLE_STEP;
    last_logic_ms_time += logic_cycles * GAME_LOGIC_CYCLE_STEP;
    return logic_cycles;
}
