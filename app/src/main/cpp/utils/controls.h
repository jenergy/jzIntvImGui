#ifndef JZINTVIMGUI_CONTROLS_H
#define JZINTVIMGUI_CONTROLS_H

#include "SDL.h"
#include <map>

#define PD0L_KP1 "PD0L_KP1"
#define PD0L_KP2 "PD0L_KP2"
#define PD0L_KP3 "PD0L_KP3"
#define PD0L_KP4 "PD0L_KP4"
#define PD0L_KP5 "PD0L_KP5"
#define PD0L_KP6 "PD0L_KP6"
#define PD0L_KP7 "PD0L_KP7"
#define PD0L_KP8 "PD0L_KP8"
#define PD0L_KP9 "PD0L_KP9"
#define PD0L_KPC "PD0L_KPC"
#define PD0L_KP0 "PD0L_KP0"
#define PD0L_KPE "PD0L_KPE"
#define PD0L_A_T "PD0L_A_T"
#define PD0L_A_L "PD0L_A_L"
#define PD0L_A_R "PD0L_A_R"
#define PD0R_KP1 "PD0R_KP1"
#define PD0R_KP2 "PD0R_KP2"
#define PD0R_KP3 "PD0R_KP3"
#define PD0R_KP4 "PD0R_KP4"
#define PD0R_KP5 "PD0R_KP5"
#define PD0R_KP6 "PD0R_KP6"
#define PD0R_KP7 "PD0R_KP7"
#define PD0R_KP8 "PD0R_KP8"
#define PD0R_KP9 "PD0R_KP9"
#define PD0R_KPC "PD0R_KPC"
#define PD0R_KP0 "PD0R_KP0"
#define PD0R_KPE "PD0R_KPE"
#define PD0R_A_T "PD0R_A_T"
#define PD0R_A_L "PD0R_A_L"
#define PD0R_A_R "PD0R_A_R"
#define PAUSE "PAUSE"
#define QUIT "QUIT"
#define RESET "RESET"
#define DISC_KEY "DISC"
#define CHANGE_PLAYER "CHANGE_PLAYER"
#define KEYBOARD "KEYBOARD"
#define SHOT "SHOT"
#define PLAYER_SELECTED "PLAYER_SELECTED"
#define PLAYER_NOT_SELECTED "PLAYER_NOT_SELECTED"
#define DISC_DIRECTION_KEY "DISC_DIRECTION"
#define CONFIGURATION_SAVED_GLOBAL_TOASTER "CONFIGURATION_SAVED_GLOBAL_TOASTER"
#define CONFIGURATION_SAVED_FOR_GAME_TOASTER "CONFIGURATION_SAVED_FOR_GAME_TOASTER"
#define CONFIGURATION_COVER "CONFIGURATION_COVER"
#define CONFIGURATION_SWITCH_MODE "CONFIGURATION_SWITCH_MODE"
#define CONFIGURATION_SELECTION "CONFIGURATION_SELECTION"
#define CONFIGURATION_PREV "CONFIGURATION_PREV"
#define CONFIGURATION_NEXT "CONFIGURATION_NEXT"
#define CONFIGURATION_VISIBLE "CONFIGURATION_VISIBLE"
#define CONFIGURATION_DEFAULT "CONFIGURATION_DEFAULT"
#define CONFIGURATION_BRIGHTNESS_LESS "CONFIGURATION_BRIGHTNESS_LESS"
#define CONFIGURATION_BRIGHTNESS_MORE "CONFIGURATION_BRIGHTNESS_MORE"
#define CONFIGURATION_SAVE_GLOBAL "CONFIGURATION_SAVE_GLOBAL"
#define CONFIGURATION_SAVE_GAME "CONFIGURATION_SAVE_GAME"
#define CONFIGURATION_DISC_KEY "CONFIGURATION_DISC"
#define CONFIGURATION_DISC_DIRECTION_KEY "CONFIGURATION_DISC_DIRECTION"
#define CONFIGURATION_X_FLIP "CONFIGURATION_X_FLIP"

// P1 Disc
#define PD0L_J_E    "PD0L_J_E"
#define PD0L_J_ENE  "PD0L_J_ENE"
#define PD0L_J_NE   "PD0L_J_NE"
#define PD0L_J_NNE  "PD0L_J_NNE"
#define PD0L_J_N    "PD0L_J_N"
#define PD0L_J_NNW  "PD0L_J_NNW"
#define PD0L_J_NW   "PD0L_J_NW"
#define PD0L_J_WNW  "PD0L_J_WNW"
#define PD0L_J_W    "PD0L_J_W"
#define PD0L_J_WSW  "PD0L_J_WSW"
#define PD0L_J_SW   "PD0L_J_SW"
#define PD0L_J_SSW  "PD0L_J_SSW"
#define PD0L_J_S    "PD0L_J_S"
#define PD0L_J_SSE  "PD0L_J_SSE"
#define PD0L_J_ESE  "PD0L_J_ESE"
#define PD0L_J_SE   "PD0L_J_SE"

// P2 Disc
#define PD0R_J_E    "PD0R_J_E"
#define PD0R_J_ENE  "PD0R_J_ENE"
#define PD0R_J_NE   "PD0R_J_NE"
#define PD0R_J_NNE  "PD0R_J_NNE"
#define PD0R_J_N    "PD0R_J_N"
#define PD0R_J_NNW  "PD0R_J_NNW"
#define PD0R_J_NW   "PD0R_J_NW"
#define PD0R_J_WNW  "PD0R_J_WNW"
#define PD0R_J_W    "PD0R_J_W"
#define PD0R_J_WSW  "PD0R_J_WSW"
#define PD0R_J_SW   "PD0R_J_SW"
#define PD0R_J_SSW  "PD0R_J_SSW"
#define PD0R_J_S    "PD0R_J_S"
#define PD0R_J_SSE  "PD0R_J_SSE"
#define PD0R_J_ESE  "PD0R_J_ESE"
#define PD0R_J_SE   "PD0R_J_SE"

// Ecs
#define KEYB_LEFT "KEYB_LEFT"
#define KEYB_PERIOD "KEYB_PERIOD"
#define KEYB_SEMI "KEYB_SEMI"
#define KEYB_P "KEYB_P"
#define KEYB_ESC "KEYB_ESC"
#define KEYB_0 "KEYB_0"
#define KEYB_ENTER "KEYB_ENTER"
#define KEYB_COMMA "KEYB_COMMA"
#define KEYB_M "KEYB_M"
#define KEYB_K "KEYB_K"
#define KEYB_I "KEYB_I"
#define KEYB_9 "KEYB_9"
#define KEYB_8 "KEYB_8"
#define KEYB_O "KEYB_O"
#define KEYB_L "KEYB_L"
#define KEYB_N "KEYB_N"
#define KEYB_B "KEYB_B"
#define KEYB_H "KEYB_H"
#define KEYB_Y "KEYB_Y"
#define KEYB_7 "KEYB_7"
#define KEYB_6 "KEYB_6"
#define KEYB_U "KEYB_U"
#define KEYB_J "KEYB_J"
#define KEYB_V "KEYB_V"
#define KEYB_C "KEYB_C"
#define KEYB_F "KEYB_F"
#define KEYB_R "KEYB_R"
#define KEYB_5 "KEYB_5"
#define KEYB_4 "KEYB_4"
#define KEYB_T "KEYB_T"
#define KEYB_G "KEYB_G"
#define KEYB_X "KEYB_X"
#define KEYB_Z "KEYB_Z"
#define KEYB_S "KEYB_S"
#define KEYB_W "KEYB_W"
#define KEYB_3 "KEYB_3"
#define KEYB_2 "KEYB_2"
#define KEYB_E "KEYB_E"
#define KEYB_D "KEYB_D"
#define KEYB_SPACE "KEYB_SPACE"
#define KEYB_DOWN "KEYB_DOWN"
#define KEYB_UP "KEYB_UP"
#define KEYB_Q "KEYB_Q"
#define KEYB_1 "KEYB_1"
#define KEYB_RIGHT "KEYB_RIGHT"
#define KEYB_CTRL "KEYB_CTRL"
#define KEYB_A "KEYB_A"
#define KEYB_SHIFT "KEYB_SHIFT"
#define KEYB_SHIFT_2 "KEYB_SHIFT_2"

#define LEFT_HAND_INDEX 0
#define RIGHT_HAND_INDEX 1

#define CONTROLS_ALPHA_ENABLED 160;
#define CONTROLS_ALPHA_DISABLED 90;

#define DEFAULT_ALPHA_CONFIG 205

typedef struct controls_configuration_t {
    int type;
    void (*config_func)(void**);
} controls_configuration_t;

class Control {
public:
    Control();

    ~Control();

    bool is_in_control(float checked_x, float checked_y, SDL_FRect *real_pos);
    void set_default_position_and_size(int hand_index);
    string get_effective_event();
    SDL_FRect *get_control_frect();
    void update_textures_alpha();
    bool check_and_fix_size();
    bool normalize_to_delta(Control* parent_control, bool keep_univisible);
    bool normalize_from_delta(Control* parent_control);
    void print();

    // Ini members: don't change order or ini won't save correctly!!
    SDL_FRect portrait_frect;
    SDL_FRect landscape_frect;
    int64_t is_visible;
    int64_t alpha_portrait;
    int64_t alpha_landscape;
    string file_name_released;
    string file_name_pressed;
    string override_event;

    // Technical members
    string original_event;
    int last_direction;
    SDL_Texture *tx_released;
    SDL_Texture *tx_pressed;
    int jzintv_event_index;
    bool last_pressed;
    bool is_pressed;
    bool is_disc;
    bool is_drawn;
    bool is_pressable;
    void (*config_func)(void*);
    int config_parameter;
    bool continuous_click;
    float x_direction = 0;
    float y_direction = 0;
    SDL_FRect real_pos;
    bool is_configurable;
    int num_positions;
    bool act_on_release;
    bool was_pressable;
    long millis_for_release;
    vector<Control*> children;
};

#endif