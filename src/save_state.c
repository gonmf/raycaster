#include "global.h"

static FILE * file;
static char * line_buffer;
static size_t line_buffer_size;
static bool read_error;

static void write_string(const char * v) {
    fprintf(file, "%s\n", v);
}

static void write_int(unsigned int v) {
    fprintf(file, "%u\n", v);
}

static void write_double(double v) {
    fprintf(file, "%f\n", v);
}

static void write_char(unsigned char v) {
    fprintf(file, "%u\n", v);
}

static void write_bool(bool v) {
    fprintf(file, "%u\n", v ? true : false);
}

static void write_char_array(unsigned char * arr, unsigned int len) {
    for (unsigned int i = 0; i < len; ++i) {
        write_char(arr[i]);
    }
}

static void write_bool_array(bool * arr, unsigned int len) {
    for (unsigned int i = 0; i < len; ++i) {
        write_bool(arr[i]);
    }
}

static void write_object(object_t * object) {
    write_char(object->type);
    write_char(object->texture);
    write_char(object->special_effect);
    write_bool(object->revealed);
    write_double(object->x);
    write_double(object->y);
}

static void write_enemy(enemy_t * enemy) {
    write_char(enemy->type);
    write_char(enemy->life);
    write_char(enemy->strategic_state);
    write_double(enemy->moving_dir_x);
    write_double(enemy->moving_dir_y);
    write_char(enemy->state);
    write_char(enemy->animation_step);
    write_double(enemy->x);
    write_double(enemy->y);
    write_double(enemy->angle);
}

static void write_object_array(object_t * arr, unsigned int len) {
    for (unsigned int i = 0; i < len; ++i) {
        write_object(arr + i);
    }
}

static void write_enemy_array(enemy_t * arr, unsigned int len) {
    for (unsigned int i = 0; i < len; ++i) {
        write_enemy(arr + i);
    }
}

static void read_string(char * str, size_t str_size) {
    if (getline(&line_buffer, &line_buffer_size, file) == -1) {
        read_error = true;
        str[0] = 0;
        return;
    }

    snprintf(str, str_size, "%s", line_buffer);

    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = 0;
        len--;
    }
}

static double read_double(void) {
    if (getline(&line_buffer, &line_buffer_size, file) == -1) {
        read_error = true;
        return 0.0;
    }

    return atof(line_buffer);
}

static unsigned int read_int(void) {
    if (getline(&line_buffer, &line_buffer_size, file) == -1) {
        read_error = true;
        return 0;
    }

    return (unsigned int)atoi(line_buffer);
}

static unsigned char read_char(void) {
    if (getline(&line_buffer, &line_buffer_size, file) == -1) {
        read_error = true;
        return 0;
    }

    return (unsigned char)atoi(line_buffer);
}

static bool read_bool(void) {
    if (getline(&line_buffer, &line_buffer_size, file) == -1) {
        read_error = true;
        return false;
    }

    return (bool)atoi(line_buffer);
}

static unsigned char * read_char_array(unsigned int len) {
    unsigned char * ret = calloc(len, sizeof(unsigned char));

    for (unsigned int i = 0; i < len; ++i) {
        ret[i] = read_char();
    }

    return ret;
}

static bool * read_bool_array(unsigned int len) {
    bool * ret = calloc(len, sizeof(bool));

    for (unsigned int i = 0; i < len; ++i) {
        ret[i] = read_bool();
    }

    return ret;
}

static void read_object(object_t * object) {
    object->type = read_char();
    object->texture = read_char();
    object->special_effect = read_char();
    object->revealed = read_bool();
    object->x = read_double();
    object->y = read_double();
}

static object_t * read_object_array(unsigned int len) {
    object_t * ret = calloc(len, sizeof(object_t));

    for (unsigned int i = 0; i < len; ++i) {
        read_object(&ret[i]);
    }

    return ret;
}

static void read_enemy(enemy_t * enemy) {
    enemy->type = read_char();
    enemy->life = read_char();
    enemy->strategic_state = read_char();
    enemy->moving_dir_x = read_double();
    enemy->moving_dir_y = read_double();
    enemy->state = read_char();
    enemy->animation_step = read_char();
    enemy->x = read_double();
    enemy->y = read_double();
    enemy->angle = read_double();
}

static enemy_t * read_enemy_array(unsigned int len) {
    enemy_t * ret = calloc(len, sizeof(enemy_t));

    for (unsigned int i = 0; i < len; ++i) {
        read_enemy(&ret[i]);
    }

    return ret;
}

void save_game_state(unsigned char idx, const level_t * level) {
    char buffer[200];
    snprintf(buffer, 200, "saves/%u.save", idx);

    file = fopen(buffer, "w");
    if (file == NULL) {
        fprintf(stderr, "WARNING: could not open file \"%s\" for writing\n", buffer);
        return;
    }

    time_t t = time(NULL);
    char buffer2[18];
    strftime(buffer2, 18, "%F %R", localtime(&t));
    snprintf(buffer, SAVE_FILE_NAME_SIZ, "L%u %s", level->level_nr + 1, buffer2);

    write_string(buffer);

    write_int(level->level_nr);
    write_double(level->observer_x);
    write_double(level->observer_y);
    write_double(level->observer_angle);
    write_double(level->observer_angle2);
    write_char_array(level->content_type, level->width * level->height);
    write_char_array(level->texture, level->width * level->height);
    write_char_array(level->special_effects, level->width * level->height);
    write_bool_array(level->map_revealed, level->width * level->height);
    write_int(level->objects_count);
    write_object_array(level->object, level->objects_count);
    write_int(level->enemies_count);
    write_enemy_array(level->enemy, level->enemies_count);

    write_int(level->score);
    write_int(level->level_start_score);
    write_char(level->life);
    write_char(level->lives);
    write_bool(level->key_1);
    write_bool(level->key_2);
    write_char(level->weapon);
    write_char(level->weapons_available);
    write_int(level->ammo);

    fclose(file);
}

bool read_game_state_name(unsigned char idx, char * s) {
    line_buffer = calloc(SAVE_FILE_NAME_SIZ, sizeof(char));
    line_buffer_size = SAVE_FILE_NAME_SIZ;
    read_error = false;

    char buffer[SAVE_FILE_NAME_SIZ];
    snprintf(buffer, SAVE_FILE_NAME_SIZ, "saves/%u.save", idx);

    file = fopen(buffer, "r");
    if (file == NULL) {
        free(line_buffer);
        line_buffer = NULL;
        return false;
    }

    read_string(s, SAVE_FILE_NAME_SIZ);

    fclose(file);
    free(line_buffer);
    line_buffer = NULL;

    return !read_error;
}

static bool valid_cell_effect(unsigned char effect) {
    return effect == SPECIAL_EFFECT_NONE || effect == SPECIAL_EFFECT_LEVEL_END ||
           effect == SPECIAL_EFFECT_REQUIRES_KEY_1 || effect == SPECIAL_EFFECT_REQUIRES_KEY_2;
}

static bool valid_object_effect(unsigned char effect) {
    switch (effect) {
        case SPECIAL_EFFECT_NONE:
        case SPECIAL_EFFECT_SCORE_1:
        case SPECIAL_EFFECT_SCORE_2:
        case SPECIAL_EFFECT_SCORE_3:
        case SPECIAL_EFFECT_SCORE_4:
        case SPECIAL_EFFECT_KEY_1:
        case SPECIAL_EFFECT_KEY_2:
        case SPECIAL_EFFECT_AMMO:
        case SPECIAL_EFFECT_MSG:
        case SPECIAL_EFFECT_MINIGUN:
        case SPECIAL_EFFECT_SMALL_HEALTH:
        case SPECIAL_EFFECT_MEDIUM_HEALTH:
        case SPECIAL_EFFECT_LARGE_HEALTH:
            return true;
        default:
            return false;
    }
}

static bool level_is_sane(const level_t * level) {
    unsigned int cells = level->width * level->height;
    unsigned int wall_texture_count = wall_textures->width * wall_textures->height;
    unsigned int object_texture_count = objects_sprites->width * objects_sprites->height;

    if (level->observer_x < 0.0 || level->observer_x > (double)(level->width - 1) ||
        level->observer_y < 0.0 || level->observer_y > (double)(level->height - 1)) {
        return false;
    }

    for (unsigned int i = 0; i < cells; ++i) {
        if (level->content_type[i] > CONTENT_TYPE_DOOR_OPEN) {
            return false;
        }
        if (level->texture[i] >= wall_texture_count) {
            return false;
        }
        if (!valid_cell_effect(level->special_effects[i])) {
            return false;
        }
    }

    for (unsigned int i = 0; i < level->objects_count; ++i) {
        const object_t * object = &level->object[i];

        if (object->type > OBJECT_TYPE_NON_BLOCK || object->texture >= object_texture_count ||
            !valid_object_effect(object->special_effect) ||
            object->x < 0.0 || object->x > (double)(level->width - 1) ||
            object->y < 0.0 || object->y > (double)(level->height - 1)) {
            return false;
        }
    }

    for (unsigned int i = 0; i < level->enemies_count; ++i) {
        const enemy_t * enemy = &level->enemy[i];

        if (enemy->type > 2 || enemy->state > ENEMY_STATE_DEAD ||
            enemy->strategic_state > ENEMY_STRATEGIC_STATE_ENGAGED ||
            enemy->x < 0.0 || enemy->x > (double)(level->width - 1) ||
            enemy->y < 0.0 || enemy->y > (double)(level->height - 1)) {
            return false;
        }
    }

    return level->weapon <= 3;
}

static level_t * abort_load(level_t * level, const char * filename) {
    fprintf(stderr, "WARNING: ignoring truncated or corrupt save file \"%s\"\n", filename);

    if (level) {
        free(level->content_type);
        free(level->texture);
        free(level->special_effects);
        free(level->map_revealed);
        free(level->object);
        free(level->enemy);
        free(level);
    }

    fclose(file);
    free(line_buffer);
    line_buffer = NULL;

    return NULL;
}

level_t * load_game_state(unsigned char idx) {
    line_buffer = calloc(SAVE_FILE_NAME_SIZ, sizeof(char));
    line_buffer_size = SAVE_FILE_NAME_SIZ;
    read_error = false;

    char buffer[SAVE_FILE_NAME_SIZ];
    snprintf(buffer, SAVE_FILE_NAME_SIZ, "saves/%u.save", idx);

    file = fopen(buffer, "r");
    if (file == NULL) {
        free(line_buffer);
        line_buffer = NULL;
        return NULL;
    }

    char save_name[SAVE_FILE_NAME_SIZ];
    read_string(save_name, SAVE_FILE_NAME_SIZ);

    unsigned int level_nr = read_int();

    if (read_error) {
        return abort_load(NULL, buffer);
    }

    level_t * level = read_level_info(level_nr);
    free(level->content_type);
    level->content_type = NULL;
    free(level->texture);
    level->texture = NULL;
    free(level->special_effects);
    level->special_effects = NULL;
    free(level->map_revealed);
    level->map_revealed = NULL;
    free(level->object);
    level->object = NULL;
    free(level->enemy);
    level->enemy = NULL;

    level->level_nr = level_nr;

    level->observer_x = read_double();
    level->observer_y = read_double();
    level->observer_angle = fit_angle(read_double());
    double observer_angle2 = read_double();
    level->observer_angle2 = MIN(MAX(observer_angle2, 1.0), 179.0);
    level->content_type = read_char_array(level->width * level->height);
    level->texture = read_char_array(level->width * level->height);
    level->special_effects = read_char_array(level->width * level->height);
    level->map_revealed = read_bool_array(level->width * level->height);

    // Both counts are bounded by the map: map objects and enemies sit on distinct
    // cells, and a dying enemy leaves at most one object behind.
    unsigned int max_count = level->width * level->height;

    level->objects_count = read_int();
    if (read_error || level->objects_count > max_count) {
        return abort_load(level, buffer);
    }
    object_t * objects = read_object_array(level->objects_count);

    level->enemies_count = read_int();
    if (read_error || level->enemies_count > max_count) {
        free(objects);
        return abort_load(level, buffer);
    }
    level->enemy = read_enemy_array(level->enemies_count);

    level->object = calloc(MAX(level->objects_count + level->enemies_count, 1), sizeof(object_t));
    if (level->objects_count > 0) {
        memcpy(level->object, objects, level->objects_count * sizeof(object_t));
    }
    free(objects);

    level->score = read_int();
    level->level_start_score = read_int();
    level->life = read_char();
    level->lives = read_char();
    level->key_1 = read_bool();
    level->key_2 = read_bool();
    level->weapon = read_char();
    level->weapons_available = read_char();
    level->ammo = read_int();

    if (read_error || !level_is_sane(level)) {
        return abort_load(level, buffer);
    }

    level->life = MIN(level->life, 100);
    level->ammo = MIN(level->ammo, (unsigned int)MAX_AMMO);

    fclose(file);
    free(line_buffer);
    line_buffer = NULL;

    return level;
}
