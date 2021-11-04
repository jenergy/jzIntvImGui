#ifndef JZINTVIMGUI_INI_H
#define JZINTVIMGUI_INI_H

#define GENERAL_SECTION "General"
#define GAMES_SECTION "Games"
#define CONTROLS_SECTION "Controls"

#define ROMS_FOLDER_OPTION "roms_folder"
#define EXEC_BIN_CRC_32_OPTION "exec_bin_crc32"
#define GROM_BIN_CRC_32_OPTION "grom_bin_crc32"
#define TUTORVISION_EXEC_BIN_CRC_32_OPTION "tutorvision_exec_bin_crc32"
#define TUTORVISION_GROM_BIN_CRC_32_OPTION "tutorvision_grom_bin_crc32"
#define ECS_BIN_CRC_32_OPTION "ecs_bin_crc32"
#define CUSTOM_COMMAND_OPTION "custom_command"
#define KEYBOARD_HACK_FILE_OPTION "keyboard_hack_file"
#define PALETTE_FILE_OPTION "palette_file"
#define HIDE_UNAVAILABLE_ROMS_OPTION "hide_unavailable_roms"
#define LAST_CRC_32_OPTION "last_crc32"
#define FONT_FILENAME_OPTION "font_filename"
#define FONT_SIZE_OPTION "font_size"
#define STYLE_INDEX_OPTION "style_index"
#define JZINTV_RESOLUTION_INDEX_OPTION "jzintv_resolution_index"

#define NUM_ROMS_JUMP_OPTION "num_roms_jump"

#define WINDOW_WIDTH_OPTION "window_width"
#define WINDOW_HEIGHT_OPTION "window_height"

#define BUTTONS_SIZE_OPTION "buttons_size"
#define SCROLLBAR_SIZE_OPTION "scrollbar_size"
#define ROMS_LIST_WIDTH_PERCENTAGE_OPTION "roms_list_width_percentage"
#define IMAGE_HEIGHT_PERCENTAGE_OPTION "image_height_percentage"

#define GAME_NAME_OPTION "game_name"
#define DESCRIPTION_OPTION "description"
#define IMAGE_FILE_NAME_OPTION "image_file_name"
#define BOX_FILE_NAME_OPTION "box_file_name"

#define USE_TUTORVISION_EXEC_OPTION "use_tutorvision_exec"
#define USE_TUTORVISION_GROM_OPTION "use_tutorvision_grom"
#define USE_TUTORVISION_GRAM_OPTION "use_tutorvision_gram"

#define ECS_TAPE_NAME_AUTO_OPTION "ecs_tape_name_auto"
#define MOBILE_ECS_PORTRAIT_ALPHA_OPTION "mobile_ecs_portrait_alpha"
#define MOBILE_ECS_LANDSCAPE_ALPHA_OPTION "mobile_ecs_landscape_alpha"
#define JLP_SAVE_FILE_AUTO_OPTION "jlp_save_file_auto"

#define WINDOW_MAXIMIZED_OPTION "window_maximized"

// Mobile Screen
#define MOBILE_SCREEN_PORTRAIT_X_PERC_OPTION  "mobile_screen_portrait_x_perc"
#define MOBILE_SCREEN_PORTRAIT_Y_PERC_OPTION  "mobile_screen_portrait_y_perc"
#define MOBILE_SCREEN_PORTRAIT_W_PERC_OPTION  "mobile_screen_portrait_w_perc"
#define MOBILE_SCREEN_PORTRAIT_H_PERC_OPTION  "mobile_screen_portrait_h_perc"
#define MOBILE_SCREEN_LANDSCAPE_X_PERC_OPTION "mobile_screen_landscape_x_perc"
#define MOBILE_SCREEN_LANDSCAPE_Y_PERC_OPTION "mobile_screen_landscape_y_perc"
#define MOBILE_SCREEN_LANDSCAPE_W_PERC_OPTION "mobile_screen_landscape_w_perc"
#define MOBILE_SCREEN_LANDSCAPE_H_PERC_OPTION "mobile_screen_landscape_h_perc"

// Mobile Options
#define MOBILE_PORTRAIT_TOP_GAP_PERCENTAGE_OPTION "mobile_portrait_top_gap_perc"
#define MOBILE_PORTRAIT_BOTTOM_GAP_PERCENTAGE_OPTION "mobile_portrait_bottom_gap_perc"
#define MOBILE_LANDSCAPE_LEFT_GAP_PERCENTAGE_OPTION "mobile_landscape_left_gap_perc"
#define MOBILE_LANDSCAPE_RIGHT_GAP_PERCENTAGE_OPTION "mobile_landscape_right_gap_perc"
#define MOBILE_SHOW_CONTROLS_OPTION "mobile_show_controls"
#define MOBILE_SHOW_CONFIGURATION_CONTROLS_OPTION "mobile_show_configuration_controls"
#define MOBILE_USE_INVERTED_CONTROLS_OPTION "mobile_use_inverted_controls"
#define MOBILE_SCREEN_PORTRAIT_S_PERC_OPTION "mobile_screen_portrait_s_perc"
#define MOBILE_SCREEN_LANDSCAPE_S_PERC_OPTION "mobile_screen_landscape_s_perc"
#define MOBILE_DEFAULT_PORTRAIT_CONTROLS_SIZE_OPTION "mobile_default_portrait_controls_size"
#define MOBILE_DEFAULT_LANDSCAPE_CONTROLS_SIZE_OPTION "mobile_default_landscape_controls_size"

// Controls
#define CONTROL_PORTRAIT_X_PERC_OPTION "control_portrait_x_perc"
#define CONTROL_PORTRAIT_Y_PERC_OPTION "control_portrait_y_perc"
#define CONTROL_PORTRAIT_W_PERC_OPTION "control_portrait_w_perc"
#define CONTROL_PORTRAIT_H_PERC_OPTION "control_portrait_h_perc"
#define CONTROL_LANDSCAPE_X_PERC_OPTION "control_landscape_x_perc"
#define CONTROL_LANDSCAPE_Y_PERC_OPTION "control_landscape_y_perc"
#define CONTROL_LANDSCAPE_W_PERC_OPTION "control_landscape_w_perc"
#define CONTROL_LANDSCAPE_H_PERC_OPTION "control_landscape_h_perc"
#define CONTROL_VISIBLE_OPTION "control_is_visible"
#define CONTROL_ALPHA_PORTRAIT_OPTION "control_alpha_portrait"
#define CONTROL_ALPHA_LANDSCAPE_OPTION "control_alpha_landscape"
#define CONTROL_FILE_NAME_RELEASED_OPTION "control_file_name_released"
#define CONTROL_FILE_NAME_PRESSED_OPTION "control_file_name_pressed"
#define CONTROL_OVERRIDE_EVENT_OPTION "control_override_event"

#define JZINTV_FULLSCREEN_OPTION "jzintv_fullscreen"
#define CONTROL_PORTRAIT_S_PERC_OPTION "control_portrait_s_perc"
#define CONTROL_LANDSCAPE_S_PERC_OPTION "control_landscape_s_perc"

#define GAME_CONTROLS_OPTION "game_controls_option"

enum ini_types {
    CHAR_POINTER_T,
    UINT_64_T,
    INT_64_T,
    DOUBLE_T,
    BOOL_T,
    SDL_FRECT_T,
    STRING_T,
    VECTOR_POINTER_T
};

typedef struct ini_option_t {
    const char *key;
    int type;
    bool (*condition_func)(void**);
    string (*print_func)(const char*, void**);
} ini_option_t;

#endif //JZINTVIMGUI_INI_H
