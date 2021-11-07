#include "global.h"

static bool valid_command(const char * s) {
    if (strcmp(s, "CEIL_COLOR") == 0) return true;
    if (strcmp(s, "FLOOR_COLOR") == 0) return true;
    if (strcmp(s, "OPEN_DOOR") == 0) return true;
    if (strcmp(s, "CLOSED_DOOR") == 0) return true;
    if (start_with(s, "WALL_TYPE_")) return true;
    if (start_with(s, "FURNITURE_OBJECT_TYPE_")) return true;
    if (start_with(s, "TREASURE_OBJECT_TYPE_")) return true;
    if (start_with(s, "LOCKED_DOOR_TYPE_")) return true;
    if (start_with(s, "KEY_OBJECT_TYPE_")) return true;
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
        if (level->content_type[x + y * level->width] == CONTENT_TYPE_DOOR) {
            fprintf(stderr, "WARNING: door on the edge converted to wall (%u,%u)\n", x, y);
            level->content_type[x + y * level->width] = CONTENT_TYPE_WALL;
            level->texture[x + y * level->width] = door_closed_texture;
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
            level->texture[x + y * level->width] = door_closed_texture;
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
            level->texture[x + y * level->width] = door_closed_texture;
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
            level->texture[x + y * level->width] = door_closed_texture;
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
    int map_size_h1 = 0;
    int map_size_h2 = 0;
    int ceil_color_r = -1;
    int ceil_color_g = -1;
    int ceil_color_b = -1;
    int floor_color_r = -1;
    int floor_color_g = -1;
    int floor_color_b = -1;
    bool locked_door_1_set = false;
    bool key_1_set = false;
    bool locked_door_2_set = false;
    bool key_2_set = false;

    // "wall" texture IDs:
    int open_door_x = -1;
    int open_door_y = -1;
    int closed_door_x = -1;
    int closed_door_y = -1;
    int wall_types_x[10];
    int wall_types_y[10];
    for (unsigned int i = 0; i < 10; ++i) {
        wall_types_x[i] = -1;
        wall_types_y[i] = -1;
    }
    int locked_door_x[2];
    int locked_door_y[2];
    for (unsigned int i = 0; i < 2; ++i) {
        locked_door_x[i] = -1;
        locked_door_y[i] = -1;
    }
    // "object" texture IDs:
    int furniture_obj_types_x[10];
    int furniture_obj_types_y[10];
    for (unsigned int i = 0; i < 10; ++i) {
        furniture_obj_types_x[i] = -1;
        furniture_obj_types_y[i] = -1;
    }
    int treasure_obj_types_x[4];
    int treasure_obj_types_y[4];
    for (unsigned int i = 0; i < 4; ++i) {
        treasure_obj_types_x[i] = -1;
        treasure_obj_types_y[i] = -1;
    }
    int door_key_x[2];
    int door_key_y[2];
    for (unsigned int i = 0; i < 2; ++i) {
        door_key_x[i] = -1;
        door_key_y[i] = -1;
    }

    unsigned char * map_content_type = calloc(MAX_LEVEL_SIZE * MAX_LEVEL_SIZE, 1);
    memset(map_content_type, CONTENT_TYPE_EMPTY, MAX_LEVEL_SIZE * MAX_LEVEL_SIZE);
    unsigned char * map_special_effects = calloc(MAX_LEVEL_SIZE * MAX_LEVEL_SIZE, 1);
    memset(map_special_effects, CONTENT_TYPE_EMPTY, MAX_LEVEL_SIZE * MAX_LEVEL_SIZE);
    unsigned char * map_texture = calloc(MAX_LEVEL_SIZE * MAX_LEVEL_SIZE, 1);
    unsigned int map_layout_idx = 0;
    int player_start_x = -1;
    int player_start_y = -1;
    int player_start_angle = -36;
    int type_nr = 0;
    bool exit_placed = false;
    char last_command[MAX_LEVEL_SIZE];
    int last_command_set = 0;
    int last_command_uses = 0;
    char line_buffer[MAX_LEVEL_SIZE];
    line_buffer[0] = 0;
    unsigned int line_pos = 0;
    char c;
    unsigned int line = 1;
    for (unsigned int ci = 0; buffer[ci]; ++ci) {
        c = buffer[ci];

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
                    } else if (start_with(last_command, "FURNITURE_OBJECT_TYPE_")) {
                        if (last_command_uses == 0) {
                            type_nr = atoi(last_command + strlen("FURNITURE_OBJECT_TYPE_"));
                            validate_scalar(type_nr, 0, 10, line);
                            if (furniture_obj_types_x[type_nr] != -1) {
                                error_w_line("duplicated object type definition", line - 1);
                            }
                            furniture_obj_types_x[type_nr] = atoi(line_buffer);
                            validate_scalar(furniture_obj_types_x[type_nr], 0, objects_sprites->width - 1, line);
                            last_command_uses++;
                        } else {
                            furniture_obj_types_y[type_nr] = atoi(line_buffer);
                            validate_scalar(furniture_obj_types_y[type_nr], 0, objects_sprites->height - 1, line);
                            last_command_set = 0;
                        }
                    } else if (start_with(last_command, "TREASURE_OBJECT_TYPE_")) {
                        if (last_command_uses == 0) {
                            type_nr = atoi(last_command + strlen("TREASURE_OBJECT_TYPE_"));
                            validate_scalar(type_nr, 0, 4, line);
                            if (treasure_obj_types_x[type_nr] != -1) {
                                error_w_line("duplicated object type definition", line - 1);
                            }
                            treasure_obj_types_x[type_nr] = atoi(line_buffer);
                            validate_scalar(treasure_obj_types_x[type_nr], 0, objects_sprites->width - 1, line);
                            last_command_uses++;
                        } else {
                            treasure_obj_types_y[type_nr] = atoi(line_buffer);
                            validate_scalar(treasure_obj_types_y[type_nr], 0, objects_sprites->height - 1, line);
                            last_command_set = 0;
                        }
                    } else if (start_with(last_command, "KEY_OBJECT_TYPE_")) {
                        if (last_command_uses == 0) {
                            type_nr = atoi(last_command + strlen("KEY_OBJECT_TYPE_"));
                            validate_scalar(type_nr, 0, 4, line);
                            if (door_key_x[type_nr] != -1) {
                                error_w_line("duplicated object type definition", line - 1);
                            }
                            door_key_x[type_nr] = atoi(line_buffer);
                            validate_scalar(door_key_x[type_nr], 0, objects_sprites->width - 1, line);
                            last_command_uses++;
                        } else {
                            door_key_y[type_nr] = atoi(line_buffer);
                            validate_scalar(door_key_y[type_nr], 0, objects_sprites->height - 1, line);
                            last_command_set = 0;
                        }
                    } else if (start_with(last_command, "LOCKED_DOOR_TYPE_")) {
                        if (last_command_uses == 0) {
                            type_nr = atoi(last_command + strlen("LOCKED_DOOR_TYPE_"));
                            validate_scalar(type_nr, 0, 4, line);
                            if (locked_door_x[type_nr] != -1) {
                                error_w_line("duplicated door type definition", line - 1);
                            }
                            locked_door_x[type_nr] = atoi(line_buffer);
                            validate_scalar(locked_door_x[type_nr], 0, wall_textures->width - 1, line);
                            last_command_uses++;
                        } else {
                            locked_door_y[type_nr] = atoi(line_buffer);
                            validate_scalar(locked_door_y[type_nr], 0, wall_textures->height - 1, line);
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
                            if (d != '.' && map_content_type[map_layout_idx] != CONTENT_TYPE_EMPTY) {
                                error_w_line("wall layout and object layout collision", line);
                            }

                            // Doors
                            if (d == 'd') {
                                if (closed_door_y == -1) {
                                    error_w_line("closed door texture must be defined prior", line);
                                }
                                map_content_type[map_layout_idx] = CONTENT_TYPE_DOOR;
                                map_texture[map_layout_idx] = closed_door_x + closed_door_y * wall_textures->width;
                            } else if (d == 'f') {
                                if (locked_door_y[0] == -1) {
                                    error_w_line("closed door texture must be defined prior", line);
                                }
                                map_content_type[map_layout_idx] = CONTENT_TYPE_DOOR;
                                map_texture[map_layout_idx] = locked_door_x[0] + locked_door_y[0] * wall_textures->width;
                                map_special_effects[map_layout_idx] = SPECIAL_EFFECT_REQUIRES_KEY_1;
                                locked_door_1_set = true;
                            } else if (d == 'g') {
                                if (locked_door_y[1] == -1) {
                                    error_w_line("closed door texture must be defined prior", line);
                                }
                                map_content_type[map_layout_idx] = CONTENT_TYPE_DOOR;
                                map_texture[map_layout_idx] = locked_door_x[1] + locked_door_y[1] * wall_textures->width;
                                map_special_effects[map_layout_idx] = SPECIAL_EFFECT_REQUIRES_KEY_2;
                                locked_door_2_set = true;
                            // Walls
                            } else if ((d - '0') >= 0 && (d - '0') < 10) {
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
                            if (d != '.' && map_content_type[map_layout_idx] != CONTENT_TYPE_EMPTY) {
                                error_w_line("wall layout and object layout collision", line);
                            }

                            // Start and end positions
                            if (d == 's') {
                                player_start_x = i;
                                player_start_y = map_size_h2;
                            } else if (d == 'e') {
                                map_special_effects[map_layout_idx] = SPECIAL_EFFECT_LEVEL_END;
                                exit_placed = true;
                            // Door keys
                            } else if (d == 'k') {
                                if (door_key_x[0] == -1) {
                                    error_w_line("key 1 texture must be defined prior", line);
                                }
                                if (key_1_set) {
                                    error_w_line("duplicate key found", line);
                                }
                                map_content_type[map_layout_idx] = CONTENT_TYPE_OBJECT;
                                map_texture[map_layout_idx] = door_key_x[0] + door_key_y[0] * objects_sprites->width;
                                map_special_effects[map_layout_idx] = SPECIAL_EFFECT_KEY_1;
                                key_1_set = true;
                            } else if (d == 'l') {
                                if (door_key_x[1] == -1) {
                                    error_w_line("key 2 texture must be defined prior", line);
                                }
                                if (key_2_set) {
                                    error_w_line("duplicate key found", line);
                                }
                                map_content_type[map_layout_idx] = CONTENT_TYPE_OBJECT;
                                map_texture[map_layout_idx] = door_key_x[1] + door_key_y[1] * objects_sprites->width;
                                map_special_effects[map_layout_idx] = SPECIAL_EFFECT_KEY_2;
                                key_2_set = true;
                            // Treasures
                            } else if (d == 't') {
                                if (treasure_obj_types_x[0] == -1) {
                                    error_w_line("treasure type 1 texture must be defined prior", line);
                                }
                                map_content_type[map_layout_idx] = CONTENT_TYPE_OBJECT;
                                map_texture[map_layout_idx] = treasure_obj_types_x[0] + treasure_obj_types_y[0] * objects_sprites->width;
                                map_special_effects[map_layout_idx] = SPECIAL_EFFECT_SCORE_1;
                            } else if (d == 'y') {
                                if (treasure_obj_types_x[1] == -1) {
                                    error_w_line("treasure type 2 texture must be defined prior", line);
                                }
                                map_content_type[map_layout_idx] = CONTENT_TYPE_OBJECT;
                                map_texture[map_layout_idx] = treasure_obj_types_x[1] + treasure_obj_types_y[1] * objects_sprites->width;
                                map_special_effects[map_layout_idx] = SPECIAL_EFFECT_SCORE_2;
                            } else if (d == 'u') {
                                if (treasure_obj_types_x[2] == -1) {
                                    error_w_line("treasure type 3 texture must be defined prior", line);
                                }
                                map_content_type[map_layout_idx] = CONTENT_TYPE_OBJECT;
                                map_texture[map_layout_idx] = treasure_obj_types_x[2] + treasure_obj_types_y[2] * objects_sprites->width;
                                map_special_effects[map_layout_idx] = SPECIAL_EFFECT_SCORE_3;
                            } else if (d == 'i') {
                                if (treasure_obj_types_x[3] == -1) {
                                    error_w_line("treasure type 4 texture must be defined prior", line);
                                }
                                map_content_type[map_layout_idx] = CONTENT_TYPE_OBJECT;
                                map_texture[map_layout_idx] = treasure_obj_types_x[3] + treasure_obj_types_y[3] * objects_sprites->width;
                                map_special_effects[map_layout_idx] = SPECIAL_EFFECT_SCORE_4;
                            // Furniture
                            } else if ((d - '0') >= 0 && (d - '0') < 10) {
                                unsigned char block_id = d - '0';
                                if (furniture_obj_types_x[block_id] == -1) {
                                    error_w_line("object type must be defined prior", line);
                                }

                                map_content_type[map_layout_idx] = CONTENT_TYPE_OBJECT;
                                map_texture[map_layout_idx] = furniture_obj_types_x[block_id] + furniture_obj_types_y[block_id] * objects_sprites->width;
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
    if (!exit_placed) {
        error("invalid map design - no exits");
    }
    if (locked_door_1_set != key_1_set || locked_door_2_set != key_2_set) {
        error("invalid map design - mismatch between locked doors and keys");
    }

    // TODO: validate all doors and keys are defined in match
    free(buffer);

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
    ret->special_effects = calloc(ret->width * ret->height, 1);
    ret->door_open_texture = open_door_x + open_door_y * wall_textures->width;
    ret->enemies_count = 0;
    ret->enemy = calloc(ret->enemies_count, sizeof(enemy_t));
    ret->score = 0;
    ret->key_1 = false;
    ret->key_2 = false;

    for (unsigned int y = 0; y < ret->height; ++y) {
        for (unsigned int x = 0; x < ret->width; ++x) {
            unsigned y2 = ret->height - y - 1;

            ret->content_type[x + y2 * ret->width] = map_content_type[x + y * ret->width];
            ret->texture[x + y2 * ret->width] = map_texture[x + y * ret->width];
            ret->special_effects[x + y2 * ret->width] = map_special_effects[x + y * ret->width];
        }
    }

    free(map_content_type);
    free(map_texture);
    free(map_special_effects);

    unsigned char door_closed_texture = closed_door_x + closed_door_y * wall_textures->width;
    surround_w_safety_walls(ret, door_closed_texture);
    validate_door_placement(ret);
    validate_object_placement(ret);

    return ret;
}
