#include "global.h"

static bool valid_command(const char * s) {
    if (strcmp(s, "CEIL_COLOR") == 0) return true;
    if (strcmp(s, "FLOOR_COLOR") == 0) return true;
    if (strcmp(s, "LAYOUT") == 0) return true;
    if (strcmp(s, "OBJECTS") == 0) return true;
    if (strcmp(s, "PLAYER_START_ANGLE") == 0) return true;
    if (start_with(s, "WALL_TYPE_")) return true;
    if (start_with(s, "FURNITURE_OBJECT_TYPE_")) return true;

    return false;
}

static void validate_scalar(int val, int min, int max, unsigned int line) {
    if (val < min || val > max) {
        error_w_line("invalid scalar value", line);
    }
}

static void validate_door_placement(const level_t * level) {
    for (unsigned int y = 1; y < level->height - 1; ++y) {
        for (unsigned int x = 1; x < level->width - 1; ++x) {
            if (level->content_type[x + y * level->width] == CONTENT_TYPE_DOOR) {
                bool wallW = level->content_type[x + 1 + y * level->width] == CONTENT_TYPE_WALL;
                bool wallS = level->content_type[x - 1 + y * level->width] == CONTENT_TYPE_WALL;
                bool wallA = level->content_type[x + (y + 1) * level->width] == CONTENT_TYPE_WALL;
                bool wallD = level->content_type[x + (y - 1) * level->width] == CONTENT_TYPE_WALL;

                bool valid = (wallW && wallS && !wallA && !wallD) || (!wallW && !wallS && wallA && wallD);
                if (!valid) {
                    char * s = calloc(MAX_FILE_NAME_SIZ, 1);
                    sprintf(s, "invalid door placement (%u,%u)", x, y);
                    error(s);
                }
            }
        }
    }
}

static void validate_object_placement(const level_t * level) {
    unsigned int rounded_x = (unsigned int)level->observer_x;
    unsigned int rounded_y = (unsigned int)level->observer_y;

    if (rounded_x == 0 || rounded_x == level->width - 1 ||
        rounded_y == 0 || rounded_y == level->height - 1) {
        error("player start position not empty");
    }
}

static void surround_w_safety_walls(level_t * level) {
    unsigned int x;
    unsigned int y;
    for (x = 0; x < level->width; ++x) {
        y = 0;
        if (level->content_type[x + y * level->width] == CONTENT_TYPE_DOOR) {
            fprintf(stderr, "WARNING: door on the edge converted to wall (%u,%u)\n", x, y);
            level->content_type[x + y * level->width] = CONTENT_TYPE_WALL;
            level->texture[x + y * level->width] = CLOSED_DOOR_TEXTURE;
        }
        if (level->content_type[x + y * level->width] != CONTENT_TYPE_WALL) {
            fprintf(stderr, "WARNING: empty space on the edge converted to wall (%u,%u)\n", x, y);
            level->content_type[x + y * level->width] = CONTENT_TYPE_WALL;
            level->texture[x + y * level->width] = SAFETY_WALL_TEXTURE;
        }
        y = (level->height - 1);
        if (level->content_type[x + y * level->width] == CONTENT_TYPE_DOOR) {
            fprintf(stderr, "WARNING: door on the edge converted to wall (%u,%u)\n", x, y);
            level->content_type[x + y * level->width] = CONTENT_TYPE_WALL;
            level->texture[x + y * level->width] = CLOSED_DOOR_TEXTURE;
        }
        if (level->content_type[x + y * level->width] != CONTENT_TYPE_WALL) {
            fprintf(stderr, "WARNING: empty space on the edge converted to wall (%u,%u)\n", x, y);
            level->content_type[x + y * level->width] = CONTENT_TYPE_WALL;
            level->texture[x + y * level->width] = SAFETY_WALL_TEXTURE;
        }
    }
    for (y = 0; y < level->height; ++y) {
        x = 0;
        if (level->content_type[x + y * level->width] == CONTENT_TYPE_DOOR) {
            fprintf(stderr, "WARNING: door on the edge converted to wall (%u,%u)\n", x, y);
            level->content_type[x + y * level->width] = CONTENT_TYPE_WALL;
            level->texture[x + y * level->width] = CLOSED_DOOR_TEXTURE;
        }
        if (level->content_type[x + y * level->width] != CONTENT_TYPE_WALL) {
            fprintf(stderr, "WARNING: empty space on the edge converted to wall (%u,%u)\n", x, y);
            level->content_type[x + y * level->width] = CONTENT_TYPE_WALL;
            level->texture[x + y * level->width] = SAFETY_WALL_TEXTURE;
        }
        x = level->width - 1;
        if (level->content_type[x + y * level->width] == CONTENT_TYPE_DOOR) {
            fprintf(stderr, "WARNING: door on the edge converted to wall (%u,%u)\n", x, y);
            level->content_type[x + y * level->width] = CONTENT_TYPE_WALL;
            level->texture[x + y * level->width] = CLOSED_DOOR_TEXTURE;
        }
        if (level->content_type[x + y * level->width] != CONTENT_TYPE_WALL) {
            fprintf(stderr, "WARNING: empty space on the edge converted to wall (%u,%u)\n", x, y);
            level->content_type[x + y * level->width] = CONTENT_TYPE_WALL;
            level->texture[x + y * level->width] = SAFETY_WALL_TEXTURE;
        }
    }
}

level_t * read_level_info(const char * filename) {
    char * str_buf = calloc(MAX_FILE_NAME_SIZ, 1);
    snprintf(str_buf, MAX_FILE_NAME_SIZ, "./levels/%s", filename);
    char * buffer = calloc(MAX_FILE_SIZE, 1);
    file_read(buffer, MAX_FILE_SIZE, str_buf);
    free(str_buf);

    int map_size_w = -1;
    int map_size_h = 0;
    int ceil_color_r = -1;
    int ceil_color_g = -1;
    int ceil_color_b = -1;
    int floor_color_r = -1;
    int floor_color_g = -1;
    int floor_color_b = -1;

    // "wall" texture IDs:
    int wall_types_x[10];
    int wall_types_y[10];
    for (unsigned int i = 0; i < 10; ++i) {
        wall_types_x[i] = -1;
        wall_types_y[i] = -1;
    }
    // "object" texture IDs:
    int furniture_obj_types_x[10];
    int furniture_obj_types_y[10];
    for (unsigned int i = 0; i < 10; ++i) {
        furniture_obj_types_x[i] = -1;
        furniture_obj_types_y[i] = -1;
    }

    unsigned int enemy_count = 0;
    enemy_t * map_enemies = calloc(MAX_LEVEL_SIZE * MAX_LEVEL_SIZE, sizeof(enemy_t));
    unsigned char * map_content_type = calloc(MAX_LEVEL_SIZE * MAX_LEVEL_SIZE, 1);
    memset(map_content_type, CONTENT_TYPE_EMPTY, MAX_LEVEL_SIZE * MAX_LEVEL_SIZE);
    unsigned char * map_special_effects = calloc(MAX_LEVEL_SIZE * MAX_LEVEL_SIZE, 1);
    memset(map_special_effects, SPECIAL_EFFECT_NONE, MAX_LEVEL_SIZE * MAX_LEVEL_SIZE);
    unsigned char * map_texture = calloc(MAX_LEVEL_SIZE * MAX_LEVEL_SIZE, 1);
    object_t * map_objects = calloc(MAX_LEVEL_SIZE * MAX_LEVEL_SIZE, sizeof(object_t));
    unsigned int map_objects_count = 0;
    unsigned int map_layout_idx = 0;
    int player_start_x = -1;
    int player_start_y = -1;
    int player_start_angle = 0;
    int type_nr = 0;
    bool exit_placed = false;
    char last_command[MAX_LEVEL_SIZE + 1];
    int last_command_set = 0;
    int last_command_uses = 0;
    char line_buffer[MAX_LEVEL_SIZE + 1];
    line_buffer[0] = 0;
    unsigned int line_pos = 0;
    char c;
    unsigned int line = 1;
    for (unsigned int ci = 0; buffer[ci]; ++ci) {
        c = buffer[ci];

        if (c == '\n') {
            if (line_pos > MAX_LEVEL_SIZE) {
                error("Invalid sprite pack format");
            }

            if (line_buffer[0]) {
                if (line_buffer[0] == '#') {
                    // skip line
                } else if (valid_command(line_buffer)) {
                    strncpy(last_command, line_buffer, MAX_LEVEL_SIZE + 1);
                    last_command_set = 1;
                    last_command_uses = 0;
                    map_layout_idx = 0;
                } else if (!last_command_set) {
                    error_w_line("unexpected argument; expected command", line);
                } else {
                    if (strcmp(last_command, "CEIL_COLOR") == 0) {
                        int val = (int)strtol(line_buffer, NULL, 16);
                        validate_scalar(val, 0, 0xffffff, line);

                        ceil_color_r = (val >> 16) & 0xff;
                        ceil_color_g = (val >> 8) & 0xff;
                        ceil_color_b = val & 0xff;
                        last_command_set = 0;
                    } else if (strcmp(last_command, "FLOOR_COLOR") == 0) {
                        int val = (int)strtol(line_buffer, NULL, 16);
                        validate_scalar(val, 0, 0xffffff, line);

                        floor_color_r = (val >> 16) & 0xff;
                        floor_color_g = (val >> 8) & 0xff;
                        floor_color_b = val & 0xff;
                        last_command_set = 0;
                    } else if (start_with(last_command, "WALL_TYPE_")) {
                        if (last_command_uses == 0) {
                            type_nr = (last_command + strlen("WALL_TYPE_"))[0] - 'A';
                            validate_scalar(type_nr, 0, 10, line);
                            if (wall_types_x[type_nr] != -1) {
                                error_w_line("duplicated command", line - 1);
                            }
                            wall_types_x[type_nr] = atoi(line_buffer);
                            validate_scalar(wall_types_x[type_nr], 0, wall_textures->width - 1, line);
                            last_command_uses++;
                        } else {
                            wall_types_y[type_nr] = atoi(line_buffer);
                            validate_scalar(wall_types_y[type_nr], 0, wall_textures->height - 1, line);
                            last_command_set = 0;
                        }
                    } else if (start_with(last_command, "FURNITURE_OBJECT_TYPE_")) {
                        if (last_command_uses == 0) {
                            type_nr = atoi(last_command + strlen("FURNITURE_OBJECT_TYPE_"));
                            validate_scalar(type_nr, 0, 10, line);
                            if (furniture_obj_types_x[type_nr] != -1) {
                                error_w_line("duplicated command", line - 1);
                            }
                            furniture_obj_types_x[type_nr] = atoi(line_buffer);
                            validate_scalar(furniture_obj_types_x[type_nr], 0, objects_sprites->width - 1, line);
                            last_command_uses++;
                        } else {
                            furniture_obj_types_y[type_nr] = atoi(line_buffer);
                            validate_scalar(furniture_obj_types_y[type_nr], 0, objects_sprites->height - 1, line);
                            last_command_set = 0;
                        }
                    } else if (strcmp(last_command, "LAYOUT") == 0) {
                        int size_w = strlen(line_buffer);
                        if (size_w < MIN_LEVEL_SIZE || size_w > MAX_LEVEL_SIZE) {
                            error_w_line("invalid map width", line);
                        } else if (map_size_h >= MAX_LEVEL_SIZE) {
                            error_w_line("invalid map height", line);
                        } else if (map_size_w == -1) {
                            map_size_w = size_w;
                        } else if (map_size_w != size_w) {
                            error_w_line("unstable map layout width", line);
                        }

                        for (int i = 0; i < map_size_w; ++i) {
                            char d = line_buffer[i];

                            // Doors
                            if (d == 'K') {
                                map_content_type[map_layout_idx] = CONTENT_TYPE_DOOR;
                                map_texture[map_layout_idx] = CLOSED_DOOR_TEXTURE;
                            } else if (d == 'L') {
                                map_content_type[map_layout_idx] = CONTENT_TYPE_DOOR;
                                map_texture[map_layout_idx] = LOCKED_DOOR_1_TEXTURE;
                                map_special_effects[map_layout_idx] = SPECIAL_EFFECT_REQUIRES_KEY_1;
                            } else if (d == 'M') {
                                map_content_type[map_layout_idx] = CONTENT_TYPE_DOOR;
                                map_texture[map_layout_idx] = LOCKED_DOOR_2_TEXTURE;
                                map_special_effects[map_layout_idx] = SPECIAL_EFFECT_REQUIRES_KEY_2;
                            // Walls
                            } else if ((d - 'A') >= 0 && (d - 'A') < 10) {
                                unsigned char block_id = d - 'A';
                                if (wall_types_x[block_id] == -1) {
                                    error_w_line("wall type must be defined prior", line);
                                }
                                map_content_type[map_layout_idx] = CONTENT_TYPE_WALL;
                                map_texture[map_layout_idx] = wall_types_x[block_id] + wall_types_y[block_id] * wall_textures->width;
                            // Start and end positions
                            } else if (d == 's') {
                                if (player_start_x != -1) {
                                    error_w_line("multiple player start definitions", line);
                                }
                                player_start_x = i;
                                player_start_y = map_size_h;
                            } else if (d == 'e') {
                                map_special_effects[map_layout_idx] = SPECIAL_EFFECT_LEVEL_END;
                                exit_placed = true;
                            // Door keys
                            } else if (d == 'k') {
                                map_objects[map_objects_count].type = OBJECT_TYPE_NON_BLOCK;
                                map_objects[map_objects_count].texture = DOOR_KEY_1_TEXTURE;
                                map_objects[map_objects_count].special_effect = SPECIAL_EFFECT_KEY_1;
                                map_objects[map_objects_count].x = (double)i;
                                map_objects[map_objects_count].y = (double)map_size_h;
                                map_objects_count++;
                            } else if (d == 'l') {
                                map_objects[map_objects_count].type = OBJECT_TYPE_NON_BLOCK;
                                map_objects[map_objects_count].texture = DOOR_KEY_2_TEXTURE;
                                map_objects[map_objects_count].special_effect = SPECIAL_EFFECT_KEY_2;
                                map_objects[map_objects_count].x = (double)i;
                                map_objects[map_objects_count].y = (double)map_size_h;
                                map_objects_count++;
                            // Treasures
                            } else if (d == 't') {
                                map_objects[map_objects_count].type = OBJECT_TYPE_NON_BLOCK;
                                map_objects[map_objects_count].texture = TREASURE_1_TEXTURE;
                                map_objects[map_objects_count].special_effect = SPECIAL_EFFECT_SCORE_1;
                                map_objects[map_objects_count].x = (double)i;
                                map_objects[map_objects_count].y = (double)map_size_h;
                                map_objects_count++;
                            } else if (d == 'y') {
                                map_objects[map_objects_count].type = OBJECT_TYPE_NON_BLOCK;
                                map_objects[map_objects_count].texture = TREASURE_2_TEXTURE;
                                map_objects[map_objects_count].special_effect = SPECIAL_EFFECT_SCORE_2;
                                map_objects[map_objects_count].x = (double)i;
                                map_objects[map_objects_count].y = (double)map_size_h;
                                map_objects_count++;
                            } else if (d == 'u') {
                                map_objects[map_objects_count].type = OBJECT_TYPE_NON_BLOCK;
                                map_objects[map_objects_count].texture = TREASURE_3_TEXTURE;
                                map_objects[map_objects_count].special_effect = SPECIAL_EFFECT_SCORE_3;
                                map_objects[map_objects_count].x = (double)i;
                                map_objects[map_objects_count].y = (double)map_size_h;
                                map_objects_count++;
                            } else if (d == 'i') {
                                map_objects[map_objects_count].type = OBJECT_TYPE_NON_BLOCK;
                                map_objects[map_objects_count].texture = TREASURE_4_TEXTURE;
                                map_objects[map_objects_count].special_effect = SPECIAL_EFFECT_SCORE_4;
                                map_objects[map_objects_count].x = (double)i;
                                map_objects[map_objects_count].y = (double)map_size_h;
                                map_objects_count++;
                            // Enemies
                            } else if (d == 'a' || d == 'z' || d == 'x' || d == 'c' || d == 'v') {
                                switch (d) {
                                    case 'a':
                                        map_enemies[enemy_count].type = 0;
                                        break;
                                    case 'z':
                                        map_enemies[enemy_count].type = 1;
                                        break;
                                    case 'x':
                                        map_enemies[enemy_count].type = 2;
                                        break;
                                    case 'c':
                                        map_enemies[enemy_count].type = 3;
                                        break;
                                    case 'v':
                                        map_enemies[enemy_count].type = 4;
                                        break;
                                }
                                map_enemies[enemy_count].life = 100;
                                map_enemies[enemy_count].x = (double)i;
                                map_enemies[enemy_count].y = map_size_h;
                                map_enemies[enemy_count].state = ENEMY_STATE_STILL;
                                map_enemies[enemy_count].strategic_state = ENEMY_STRATEGIC_STATE_WAITING;
                                map_enemies[enemy_count].animation_step = 0;
                                enemy_count++;
                            // Furniture
                            } else if ((d - '0') >= 0 && (d - '0') < 10) {
                                unsigned char block_id = d - '0';
                                if (furniture_obj_types_x[block_id] == -1) {
                                    error_w_line("object type must be defined prior", line);
                                }
                                map_objects[map_objects_count].type = OBJECT_TYPE_BLOCKING;
                                map_objects[map_objects_count].texture = furniture_obj_types_x[block_id] + furniture_obj_types_y[block_id] * objects_sprites->width;
                                map_objects[map_objects_count].special_effect = SPECIAL_EFFECT_NONE;
                                map_objects[map_objects_count].x = (double)i;
                                map_objects[map_objects_count].y = (double)map_size_h;
                                map_objects_count++;
                            } else if (d != '.') {
                                error_w_line("invalid character", line);
                            }
                            map_layout_idx++;
                        }
                        map_size_h++;
                    } else if (strcmp(last_command, "PLAYER_START_ANGLE") == 0) {
                        if (last_command_uses == 0) {
                            player_start_angle = atoi(line_buffer);
                            validate_scalar(player_start_angle, -359, 359, line);
                            last_command_set = 0;
                        }
                    } else {
                        error_w_line("unknown command", line);
                    }
                }

                line_pos = 0;
                line_buffer[line_pos] = 0;
            }

            line++;
        } else if (c != '\r') {
            line_buffer[line_pos] = c;
            line_pos++;
            line_buffer[line_pos] = 0;
        }
    }

    if (map_size_w < 0) {
        error_w_line("end of file with map not defined", line);
    }
    if (map_size_h < MIN_LEVEL_SIZE) {
        error_w_line("invalid map height", line);
    }
    if (ceil_color_r < 0) {
        error_w_line("end of file with ceil_color not defined", line);
    }
    if (floor_color_r < 0) {
        error_w_line("end of file with floor_color not defined", line);
    }
    if (player_start_angle < -359) {
        error_w_line("end of file with player_start_angle not defined", line);
    }
    if (player_start_x < 0) {
        error_w_line("end of file with player start position not defined", line);
    }
    if (!exit_placed) {
        error("invalid map design - no exits");
    }

    free(buffer);

    level_t * ret = (level_t *)calloc(sizeof(level_t), 1);

    ret->width = map_size_w;
    ret->height = map_size_h;
    ret->observer_x = (double)player_start_x;
    player_start_y = map_size_h - player_start_y - 1;
    ret->observer_y = (double)player_start_y;
    ret->observer_angle = fit_angle((double)player_start_angle);
    ret->observer_angle2 = 90.0;
    ret->ceil_color.red = ceil_color_r;
    ret->ceil_color.green = ceil_color_g;
    ret->ceil_color.blue = ceil_color_b;
    ret->ceil_color.alpha = 0;
    ret->floor_color.red = floor_color_r;
    ret->floor_color.green = floor_color_g;
    ret->floor_color.blue = floor_color_b;
    ret->floor_color.alpha = 0;
    ret->content_type = calloc(ret->width * ret->height, 1);
    ret->texture = calloc(ret->width * ret->height, 1);
    ret->special_effects = calloc(ret->width * ret->height, 1);
    ret->map_revealed = calloc(ret->width * ret->height, 1);
    ret->door_open_texture = OPEN_DOOR_TEXTURE;
    ret->enemy = NULL;
    ret->key_1 = false;
    ret->key_2 = false;
    ret->objects_count = map_objects_count;
    ret->object = calloc(MAX(map_objects_count + enemy_count, 1), sizeof(object_t));
    ret->enemies_count = enemy_count;
    ret->enemy = calloc(MAX(enemy_count, 1), sizeof(enemy_t));

    for (unsigned int y = 0; y < ret->height; ++y) {
        for (unsigned int x = 0; x < ret->width; ++x) {
            unsigned y2 = ret->height - y - 1;

            ret->content_type[x + y2 * ret->width] = map_content_type[x + y * ret->width];
            ret->texture[x + y2 * ret->width] = map_texture[x + y * ret->width];
            ret->special_effects[x + y2 * ret->width] = map_special_effects[x + y * ret->width];
            ret->map_revealed[x + y2 * ret->width] = false;
        }
    }

    for (unsigned int i = 0; i < map_objects_count; ++i) {
        ret->object[i] = map_objects[i];
        ret->object[i].y = map_size_h - map_objects[i].y - 1;
    }
    for (unsigned int i = 0; i < enemy_count; ++i) {
        ret->enemy[i] = map_enemies[i];
        ret->enemy[i].y = map_size_h - map_enemies[i].y - 1;
    }

    free(map_content_type);
    free(map_texture);
    free(map_special_effects);
    free(map_objects);
    free(map_enemies);

    surround_w_safety_walls(ret);
    validate_door_placement(ret);
    validate_object_placement(ret);

    ret->map_revealed[player_start_x + player_start_y * ret->width] = true;

    return ret;
}
