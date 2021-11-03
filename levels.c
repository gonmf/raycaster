#include "global.h"

static int start_with(const char * s, const char * prefix) {
    return strlen(prefix) < strlen(s) && strncmp(prefix, s, strlen(prefix)) == 0;
}

static int valid_command(const char * s) {
    if (strcmp(s, "ceil_color") == 0) return 1;
    if (strcmp(s, "floor_color") == 0) return 1;
    if (start_with(s, "wall_type_")) return 1;
    if (strcmp(s, "map_layout") == 0) return 1;
    if (strcmp(s, "map_layout_end") == 0) return 1;
    if (strcmp(s, "player_start_angle") == 0) return 1;
    return 0;
}

static void validate_scalar(int val, int min, int max, unsigned int line) {
    if (val < min || val > max) {
        error_w_line("Invalid scalar value", line);
    }
}

level_t * read_level_info(const char * filename) {
    char * buffer = malloc(MAX_LEVEL_FILE_SIZE);
    char * orig_buffer = buffer;
    snprintf(buffer, MAX_LEVEL_FILE_SIZE, "./levels/%s", filename);

    FILE * file = fopen(buffer, "rb");
    if (file == NULL) {
        sprintf(buffer, "Could not open file \"./levels/%s\"", filename);
        error(buffer);
    }
    printf("Opening \"./levels/%s\"\n", filename);

    fread(buffer, 1, MAX_LEVEL_FILE_SIZE, file);
    fclose(file);

    int map_size_w = -1;
    int map_size_h = 0;
    int ceil_color_r = -1;
    int ceil_color_g = -1;
    int ceil_color_b = -1;
    int floor_color_r = -1;
    int floor_color_g = -1;
    int floor_color_b = -1;
    int wall_types_x[MAX_LEVEL_WALL_TYPES];
    int wall_types_y[MAX_LEVEL_WALL_TYPES];
    for (unsigned int i = 0; i < MAX_LEVEL_WALL_TYPES; ++i) {
        wall_types_x[i] = -1;
        wall_types_y[i] = -1;
    }
    unsigned char * map_layout = calloc(MAX_FILE_NAME_SIZ, 1);
    unsigned int map_layout_idx = 0;
    int player_start_x = -1;
    int player_start_y = -1;
    int player_start_angle = -1;

    int wall_type_nr = 0;
    char last_command[80];
    int last_command_set = 0;
    int last_command_uses = 0;
    char line_buffer[80];
    line_buffer[0] = 0;
    unsigned int line_pos = 0;
    char c;
    unsigned int line = 1;
    while ((c = buffer[0])) {
        if (c == '\n' || c == ',') {
            if (line_buffer[0]) {
                if (valid_command(line_buffer)) {
                    if (last_command_set) {
                        if (strcmp(last_command, "map_layout") != 0) {
                            error_w_line("unexpected command; expected argument", line);
                        }
                    }
                    strncpy(last_command, line_buffer, 80);
                    last_command_set = 1;
                    last_command_uses = 0;
                } else if (!last_command_set) {
                    error_w_line("unexpected argument; expected command", line);
                } else {
                    if (strcmp(last_command, "ceil_color") == 0) {
                        if (last_command_uses == 0) {
                            ceil_color_r = atoi(line_buffer);
                            validate_scalar(ceil_color_r, 0, 255, line);
                            last_command_uses++;
                        } else if (last_command_uses == 1) {
                            ceil_color_g = atoi(line_buffer);
                            validate_scalar(ceil_color_g, 0, 255, line);
                            last_command_uses++;
                        } else {
                            ceil_color_b = atoi(line_buffer);
                            validate_scalar(ceil_color_b, 0, 255, line);
                            last_command_set = 0;
                        }
                    } else if (strcmp(last_command, "floor_color") == 0) {
                        if (last_command_uses == 0) {
                            floor_color_r = atoi(line_buffer);
                            validate_scalar(floor_color_r, 0, 255, line);
                            last_command_uses++;
                        } else if (last_command_uses == 1) {
                            floor_color_g = atoi(line_buffer);
                            validate_scalar(floor_color_g, 0, 255, line);
                            last_command_uses++;
                        } else {
                            floor_color_b = atoi(line_buffer);
                            validate_scalar(floor_color_b, 0, 255, line);
                            last_command_set = 0;
                        }
                    } else if (start_with(last_command, "wall_type_")) {
                        if (last_command_uses == 0) {
                            wall_type_nr = atoi(last_command + strlen("wall_type_"));
                            --wall_type_nr;
                            validate_scalar(wall_type_nr, 0, MAX_LEVEL_WALL_TYPES - 1, line);
                            if (wall_types_x[wall_type_nr] != -1) {
                                error_w_line("duplicated wall type definition", line - 1);
                            }
                            wall_types_x[wall_type_nr] = atoi(line_buffer);
                            validate_scalar(wall_types_x[wall_type_nr], 0, TEXTURE_PACK_WIDTH - 1, line);
                            last_command_uses++;
                        } else {
                            wall_types_y[wall_type_nr] = atoi(line_buffer);
                            validate_scalar(wall_types_y[wall_type_nr], 0, TEXTURE_PACK_HEIGHT - 1, line);
                            last_command_set = 0;
                        }
                    } else if (strcmp(last_command, "map_layout") == 0) {
                        int size_w = strlen(line_buffer);
                        if (map_size_w == -1) {
                            map_size_w = size_w;
                        } else if (map_size_w != size_w) {
                            error_w_line("inconstant map size", line);
                        }

                        for (int i = 0; i < map_size_w; ++i) {
                            char d = line_buffer[i];
                            if (d == 'S') {
                                player_start_x = map_layout_idx % map_size_w;
                                player_start_y = map_layout_idx / map_size_w;
                                map_layout[map_layout_idx++] = 0;
                            } else if (d == '.') {
                                map_layout[map_layout_idx++] = 0;
                            } else if (/*d == 'E' || */((d - '1') >= 0 && (d - '1') < MAX_LEVEL_WALL_TYPES)) {
                                unsigned char block_id = d - '1';
                                if (wall_types_x[block_id] == -1) {
                                    error_w_line("wall type must be define prior", line);
                                }

                                map_layout[map_layout_idx++] = wall_types_x[block_id] + wall_types_y[block_id] * TEXTURE_PACK_WIDTH + 1;
                            } else {
                                error_w_line("invalid map layout", line);
                            }
                        }

                        map_size_h++;
                    } else if (strcmp(last_command, "player_start_angle") == 0) {
                        if (last_command_uses == 0) {
                            player_start_angle = atoi(line_buffer);
                            validate_scalar(player_start_angle, 0, 359, line);
                            last_command_set = 0;
                        }
                    } else {
                        error_w_line("unknown command", line);
                    }
                }

                line_pos = 0;
                line_buffer[line_pos] = 0;
            }
            if (c == '\n') {
                line++;
            }
        } else if (c != '\r') {
            line_buffer[line_pos] = c;
            line_pos++;
            line_buffer[line_pos] = 0;
        }

        buffer++;
    }


    if (map_size_w < 0) {
        error_w_line("end of file with map_size_w not defined", line);
    }
    if (map_size_w < 3 || map_size_h < 3) {
        error_w_line("map too small", line);
    }
    if (ceil_color_r < 0) {
        error_w_line("end of file with ceil_color not defined", line);
    }
    if (floor_color_r < 0) {
        error_w_line("end of file with floor_color not defined", line);
    }
    if (player_start_angle < 0) {
        error_w_line("end of file with player_start_angle not defined", line);
    }
    if (player_start_x < 0) {
        error_w_line("end of file with player start position not defined", line);
    }
    free(orig_buffer);

    unsigned int total_size = sizeof(pixel_t) * 2 + sizeof(int) * 2 + sizeof(double) * 3 + map_size_w * map_size_h;
    level_t * ret = (level_t *)calloc(total_size, 1);

    ret->width = map_size_w;
    ret->height = map_size_h;
    ret->observer_x = player_start_x;
    ret->observer_y = player_start_y;
    ret->observer_angle = (double)player_start_angle;
    ret->ceil_color.red = ceil_color_r;
    ret->ceil_color.green = ceil_color_g;
    ret->ceil_color.blue = ceil_color_b;
    ret->ceil_color.alpha = 0;
    ret->floor_color.red = floor_color_r;
    ret->floor_color.green = floor_color_g;
    ret->floor_color.blue = floor_color_b;
    ret->floor_color.alpha = 0;

    for (int i = 0; i < map_size_w * map_size_h; ++i) {
        ret->contents[i] = map_layout[i];
    }

    free(map_layout);

    // surround level rectangle with blocks for performance
    for (int i = 0; i < map_size_w; ++i) {
        if (!ret->contents[i]) {
            ret->contents[i] = SAFETY_BARRIER_BLOCK + 1;
        }
    if (!ret->contents[i + (map_size_h - 1) * map_size_w]) {
            ret->contents[i + (map_size_h - 1) * map_size_w] = SAFETY_BARRIER_BLOCK + 1;
        }
    }
    for (int i = 0; i < map_size_h; ++i) {
        if (!ret->contents[i * map_size_w]) {
            ret->contents[i * map_size_w] = SAFETY_BARRIER_BLOCK + 1;
        }
        if (!ret->contents[map_size_w - 1 + i * map_size_w]) {
            ret->contents[map_size_w - 1 + i * map_size_w] = SAFETY_BARRIER_BLOCK + 1;
        }
    }

    return ret;
}
