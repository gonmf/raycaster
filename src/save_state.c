#include "global.h"

static FILE * file;
static char * line_buffer;
static size_t line_buffer_size;

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

static void read_string(char * str) {
    getline(&line_buffer, &line_buffer_size, file);

    snprintf(str, 80, "%s", line_buffer);
}

static double read_double() {
    getline(&line_buffer, &line_buffer_size, file);

    return atof(line_buffer);
}

static unsigned int read_int() {
    getline(&line_buffer, &line_buffer_size, file);

    return (unsigned int)atoi(line_buffer);
}

static unsigned char read_char() {
    getline(&line_buffer, &line_buffer_size, file);

    return (unsigned char)atoi(line_buffer);
}

static bool read_bool() {
    getline(&line_buffer, &line_buffer_size, file);

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
    char * orig_line_buffer = calloc(SAVE_FILE_NAME_SIZ, sizeof(char));
    line_buffer = orig_line_buffer;
    line_buffer_size = SAVE_FILE_NAME_SIZ;

    char buffer[SAVE_FILE_NAME_SIZ];
    snprintf(buffer, SAVE_FILE_NAME_SIZ, "saves/%u.save", idx);

    file = fopen(buffer, "r");
    if (file == NULL) {
        free(orig_line_buffer);
        return false;
    }

    read_string(s);

    fclose(file);
    free(orig_line_buffer);

    return true;
}

level_t * load_game_state(unsigned char idx) {
    char * orig_line_buffer = calloc(SAVE_FILE_NAME_SIZ, sizeof(char));
    line_buffer = orig_line_buffer;
    line_buffer_size = SAVE_FILE_NAME_SIZ;

    char buffer[SAVE_FILE_NAME_SIZ];
    snprintf(buffer, SAVE_FILE_NAME_SIZ, "saves/%u.save", idx);

    file = fopen(buffer, "r");

    char save_name[SAVE_FILE_NAME_SIZ];
    read_string(save_name);

    unsigned int level_nr = read_int();

    level_t * level = read_level_info(level_nr);
    free(level->content_type);
    free(level->texture);
    free(level->special_effects);
    free(level->map_revealed);
    free(level->object);
    free(level->enemy);

    level->level_nr = level_nr;

    level->observer_x = read_double();
    level->observer_y = read_double();
    level->observer_angle = read_double();
    level->observer_angle2 = read_double();
    level->content_type = read_char_array(level->width * level->height);
    level->texture = read_char_array(level->width * level->height);
    level->special_effects = read_char_array(level->width * level->height);
    level->map_revealed = read_bool_array(level->width * level->height);
    level->objects_count = read_int();
    level->object = read_object_array(level->objects_count);
    level->enemies_count = read_int();
    level->enemy = read_enemy_array(level->enemies_count);

    level->score = read_int();
    level->level_start_score = read_int();
    level->life = read_char();
    level->lives = read_char();
    level->key_1 = read_bool();
    level->key_2 = read_bool();
    level->weapon = read_char();
    level->weapons_available = read_char();
    level->ammo = read_int();

    fclose(file);
    free(orig_line_buffer);

    return level;
}
