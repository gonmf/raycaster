#include "global.h"

static bool valid_command(const char * s) {
    if (strcmp(s, "CEIL_COLOR") == 0) return true;
    if (strcmp(s, "FLOOR_COLOR") == 0) return true;
    if (strcmp(s, "OPEN_DOOR") == 0) return true;
    if (strcmp(s, "CLOSED_DOOR") == 0) return true;
    if (start_with(s, "WALL_TYPE_")) return true;
    if (start_with(s, "OBJECT_TYPE_")) return true;
    if (strcmp(s, "LAYOUT") == 0) return true;
    if (strcmp(s, "OBJECTS") == 0) return true;
    if (strcmp(s, "PLAYER_START_ANGLE") == 0) return true;
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
    if (level->observer_x == 0 || level->observer_x == level->width - 1 ||
        level->observer_y == 0 || level->observer_y == level->height - 1) {
        error("player start position not empty");
    }
}

static void surround_w_safety_walls(level_t * level, unsigned char door_closed_texture) {
    unsigned int x;
    unsigned int y;
    for (x = 0; x < level->width; ++x) {
        y = 0;
        if (level->content_type[x + y * level->width] == CONTENT_TYPE_EMPTY || level->content_type[x + y * level->width] == CONTENT_TYPE_OBJECT) {
            fprintf(stderr, "WARNING: empty space on the edge converted to wall (%u,%u)\n", x, y);
            level->content_type[x + y * level->width] = CONTENT_TYPE_WALL;
            level->texture[x + y * level->width] = SAFETY_WALL_TEXTURE;
        }
        if (level->content_type[x + y * level->width] == CONTENT_TYPE_DOOR) {
            fprintf(stderr, "WARNING: door on the edge converted to wall (%u,%u)\n", x, y);
            level->content_type[x + y * level->width] = CONTENT_TYPE_WALL;
            level->texture[x + y * level->width] = door_closed_texture;
        }
        y = (level->height - 1) * level->width;
        if (level->content_type[x + y * level->width] == CONTENT_TYPE_EMPTY || level->content_type[x + y * level->width] == CONTENT_TYPE_OBJECT) {
            fprintf(stderr, "WARNING: empty space on the edge converted to wall (%u,%u)\n", x, y);
            level->content_type[x + y * level->width] = CONTENT_TYPE_WALL;
            level->texture[x + y * level->width] = SAFETY_WALL_TEXTURE;
        }
        if (level->content_type[x + y * level->width] == CONTENT_TYPE_DOOR) {
            fprintf(stderr, "WARNING: door on the edge converted to wall (%u,%u)\n", x, y);
            level->content_type[x + y * level->width] = CONTENT_TYPE_WALL;
            level->texture[x + y * level->width] = door_closed_texture;
        }
    }
    for (y = 0; y < level->height; ++y) {
        x = 0;
        if (level->content_type[x + y * level->width] == CONTENT_TYPE_EMPTY || level->content_type[x + y * level->width] == CONTENT_TYPE_OBJECT) {
            fprintf(stderr, "WARNING: empty space on the edge converted to wall (%u,%u)\n", x, y);
            level->content_type[x + y * level->width] = CONTENT_TYPE_WALL;
            level->texture[x + y * level->width] = SAFETY_WALL_TEXTURE;
        }
        if (level->content_type[x + y * level->width] == CONTENT_TYPE_DOOR) {
            fprintf(stderr, "WARNING: door on the edge converted to wall (%u,%u)\n", x, y);
            level->content_type[x + y * level->width] = CONTENT_TYPE_WALL;
            level->texture[x + y * level->width] = door_closed_texture;
        }
        x = level->width - 1;
        if (level->content_type[x + y * level->width] == CONTENT_TYPE_EMPTY || level->content_type[x + y * level->width] == CONTENT_TYPE_OBJECT) {
            fprintf(stderr, "WARNING: empty space on the edge converted to wall (%u,%u)\n", x, y);
            level->content_type[x + y * level->width] = CONTENT_TYPE_WALL;
            level->texture[x + y * level->width] = SAFETY_WALL_TEXTURE;
        }
        if (level->content_type[x + y * level->width] == CONTENT_TYPE_DOOR) {
            fprintf(stderr, "WARNING: door on the edge converted to wall (%u,%u)\n", x, y);
            level->content_type[x + y * level->width] = CONTENT_TYPE_WALL;
            level->texture[x + y * level->width] = door_closed_texture;
        }
    }
}

level_t * read_level_info(const char * filename) {
    char * str_buf = calloc(MAX_FILE_NAME_SIZ, 1);
    snprintf(str_buf, MAX_FILE_NAME_SIZ, "./levels/%s", filename);
    char * buffer = calloc(MAX_FILE_SIZE, 1);
    file_read(buffer, MAX_FILE_SIZE, str_buf);
    free(str_buf);
    char * orig_buffer = buffer;

    int map_size_w = -1;
    int map_size_h1 = 0;
    int map_size_h2 = 0;
    int ceil_color_r = -1;
    int ceil_color_g = -1;
    int ceil_color_b = -1;
    int floor_color_r = -1;
    int floor_color_g = -1;
    int floor_color_b = -1;
    int wall_types_x[10];
    int wall_types_y[10];
    int object_types_x[10];
    int object_types_y[10];
    for (unsigned int i = 0; i < 10; ++i) {
        wall_types_x[i] = -1;
        wall_types_y[i] = -1;
        object_types_x[i] = -1;
        object_types_y[i] = -1;
    }
    unsigned char * map_content_type = calloc(MAX_LEVEL_SIZE * MAX_LEVEL_SIZE, 1);
    memset(map_content_type, CONTENT_TYPE_EMPTY, MAX_LEVEL_SIZE * MAX_LEVEL_SIZE);
    unsigned char * map_texture = calloc(MAX_LEVEL_SIZE * MAX_LEVEL_SIZE, 1);
    unsigned int map_layout_idx = 0;
    int player_start_x = -1;
    int player_start_y = -1;
    int player_start_angle = -36;
    int open_door_x = -1;
    int open_door_y = -1;
    int closed_door_x = -1;
    int closed_door_y = -1;
    int type_nr = 0;
    char last_command[MAX_LEVEL_SIZE];
    int last_command_set = 0;
    int last_command_uses = 0;
    char line_buffer[MAX_LEVEL_SIZE];
    line_buffer[0] = 0;
    unsigned int line_pos = 0;
    char c;
    unsigned int line = 1;
    while ((c = *buffer)) {
        if (c == '\n') {
            if (line_pos == MAX_LEVEL_SIZE) {
                error("Invalid sprite pack format");
            }

            if (line_buffer[0]) {
                if (line_buffer[0] == '#') {
                    // skip line
                } else if (valid_command(line_buffer)) {
                    strncpy(last_command, line_buffer, MAX_LEVEL_SIZE);
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
                    } else if (strcmp(last_command, "OPEN_DOOR") == 0) {
                        if (last_command_uses == 0) {
                            open_door_x = atoi(line_buffer);
                            validate_scalar(open_door_x, 0, wall_textures->width - 1, line);
                            last_command_uses++;
                        } else {
                            open_door_y = atoi(line_buffer);
                            validate_scalar(open_door_y, 0, wall_textures->height - 1, line);
                            last_command_set = 0;
                        }
                    } else if (strcmp(last_command, "CLOSED_DOOR") == 0) {
                        if (last_command_uses == 0) {
                            closed_door_x = atoi(line_buffer);
                            validate_scalar(closed_door_x, 0, wall_textures->width - 1, line);
                            last_command_uses++;
                        } else {
                            closed_door_y = atoi(line_buffer);
                            validate_scalar(closed_door_y, 0, wall_textures->height - 1, line);
                            last_command_set = 0;
                        }
                    } else if (start_with(last_command, "WALL_TYPE_")) {
                        if (last_command_uses == 0) {
                            type_nr = atoi(last_command + strlen("WALL_TYPE_"));
                            validate_scalar(type_nr, 0, 10, line);
                            if (wall_types_x[type_nr] != -1) {
                                error_w_line("duplicated wall type definition", line - 1);
                            }
                            wall_types_x[type_nr] = atoi(line_buffer);
                            validate_scalar(wall_types_x[type_nr], 0, wall_textures->width - 1, line);
                            last_command_uses++;
                        } else {
                            wall_types_y[type_nr] = atoi(line_buffer);
                            validate_scalar(wall_types_y[type_nr], 0, wall_textures->height - 1, line);
                            last_command_set = 0;
                        }
                    } else if (start_with(last_command, "OBJECT_TYPE_")) {
                        if (last_command_uses == 0) {
                            type_nr = atoi(last_command + strlen("OBJECT_TYPE_"));
                            validate_scalar(type_nr, 0, 10, line);
                            if (object_types_x[type_nr] != -1) {
                                error_w_line("duplicated object type definition", line - 1);
                            }
                            object_types_x[type_nr] = atoi(line_buffer);
                            validate_scalar(object_types_x[type_nr], 0, wall_textures->width - 1, line);
                            last_command_uses++;
                        } else {
                            object_types_y[type_nr] = atoi(line_buffer);
                            validate_scalar(object_types_y[type_nr], 0, wall_textures->height - 1, line);
                            last_command_set = 0;
                        }
                    } else if (strcmp(last_command, "LAYOUT") == 0) {
                        int size_w = strlen(line_buffer);
                        if (size_w >= MAX_LEVEL_SIZE) {
                            error_w_line("maximum map layout width exceeded", line);
                        } else if (map_size_w == -1) {
                            map_size_w = size_w;
                        } else if (map_size_w != size_w) {
                            error_w_line("unstable map layout width", line);
                        }

                        for (int i = 0; i < map_size_w; ++i) {
                            char d = line_buffer[i];
                            if (d == 'd') {
                                if (map_content_type[map_layout_idx] != CONTENT_TYPE_EMPTY) {
                                    error_w_line("wall position must be empty", line);
                                }
                                if (closed_door_y == -1) {
                                    error_w_line("closed door texture must be defined prior", line);
                                }
                                map_content_type[map_layout_idx] = CONTENT_TYPE_DOOR;
                                map_texture[map_layout_idx] = closed_door_x + closed_door_y * wall_textures->width;
                            } else if ((d - '0') >= 0 && (d - '0') < 10) {
                                if (map_content_type[map_layout_idx] != CONTENT_TYPE_EMPTY) {

                                }
                                unsigned char block_id = d - '0';
                                if (wall_types_x[block_id] == -1) {
                                    error_w_line("wall type must be defined prior", line);
                                }

                                map_content_type[map_layout_idx] = CONTENT_TYPE_WALL;
                                map_texture[map_layout_idx] = wall_types_x[block_id] + wall_types_y[block_id] * wall_textures->width;
                            } else if (d != '.') {
                                error_w_line("invalid map layout", line);
                            }
                            map_layout_idx++;
                        }
                        map_size_h1++;
                    } else if (strcmp(last_command, "OBJECTS") == 0) {
                        int size_w = strlen(line_buffer);
                        if (size_w >= MAX_LEVEL_SIZE) {
                            error_w_line("maximum map layout width exceeded", line);
                        } else if (map_size_w == -1) {
                            map_size_w = size_w;
                        } else if (map_size_w != size_w) {
                            error_w_line("unstable map layout width", line);
                        }

                        for (int i = 0; i < map_size_w; ++i) {
                            char d = line_buffer[i];
                            if (d == 's') {
                                if (map_content_type[map_layout_idx] != CONTENT_TYPE_EMPTY) {
                                    error_w_line("player start position must be empty", line);
                                }
                                if (player_start_x != -1) {
                                    error_w_line("multiple player start (s) definitions", line);
                                }
                                player_start_x = i;
                                player_start_y = map_size_h2;
                            } else if ((d - '0') >= 0 && (d - '0') < 10) {
                                unsigned char block_id = d - '0';
                                if (object_types_x[block_id] == -1) {
                                    error_w_line("object type must be defined prior", line);
                                }
                                if (map_content_type[map_layout_idx] != CONTENT_TYPE_EMPTY) {
                                    error_w_line("object position must be empty", line);
                                }

                                map_content_type[map_layout_idx] = CONTENT_TYPE_OBJECT;
                                map_texture[map_layout_idx] = object_types_x[block_id] + object_types_y[block_id] * wall_textures->width;
                            } else if (d != '.') {
                                error_w_line("invalid objects layout", line);
                            }
                            map_layout_idx++;
                        }
                        map_size_h2++;
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

        buffer++;
    }

    if (map_size_w < 0) {
        error_w_line("end of file with map not defined", line);
    }
    if (map_size_w < 3 || map_size_h1 < 3) {
        error_w_line("map too small", line);
    }
    if (map_size_h1 != map_size_h2) {
        error_w_line("map layout size does not match objects layout", line);
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
    if (open_door_y < 0) {
        error_w_line("end of file without open door texture defined", line);
    }
    if (closed_door_y < 0) {
        error_w_line("end of file without closed door texture defined", line);
    }
    free(orig_buffer);

    level_t * ret = (level_t *)calloc(sizeof(level_t), 1);

    ret->width = map_size_w;
    ret->height = map_size_h1;
    ret->observer_x = player_start_x;
    ret->observer_y = map_size_h1 - player_start_y - 1;
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
    ret->door_open_texture = open_door_x + open_door_y * wall_textures->width;
    ret->enemies_count = 0;
    ret->enemy = calloc(ret->enemies_count, sizeof(enemy_t));

    for (unsigned int y = 0; y < ret->height; ++y) {
        for (unsigned int x = 0; x < ret->width; ++x) {
            unsigned y2 = ret->height - y - 1;

            ret->content_type[x + y2 * ret->width] = map_content_type[x + y * ret->width];
            ret->texture[x + y2 * ret->width] = map_texture[x + y * ret->width];
        }
    }

    free(map_content_type);
    free(map_texture);

    unsigned char door_closed_texture = closed_door_x + closed_door_y * wall_textures->width;
    surround_w_safety_walls(ret, door_closed_texture);
    validate_door_placement(ret);
    validate_object_placement(ret);

    return ret;
}
