#ifndef JZINTVIMGUI_MAIN_H
#define JZINTVIMGUI_MAIN_H

#include <cstring>
#include <string>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <imgui.h>

#define IMGUI_DEFINE_PLACEMENT_NEW
#define IMGUI_DEFINE_MATH_OPERATORS

#include <imgui_internal.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <dirent.h>
#include <set>
#include "imgui_scrollable.h"
#include "popup.h"
#include "utils/gui_events.h"
#include "utils/controls.h"
#include "utils/messages.h"
#include "utils/ini.h"
#include "utils/exceptions.h"
#include "SDL.h"
#include "logger.h"

#ifdef WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#define VERSION "2.3.2"
#define EMBEDDED_JZINTV_VERSION "jzintv-20200712"
#define JZINTV_URL "http://spatula-city.org/~im14u2c/intv/"
#define ZENDOCON_URL "https://atariage.com/forums/profile/31886-zendocon/"
#define DEAR_IMGUI_VERSION "1.84"
#define DEAR_IMGUI_URL "https://github.com/ocornut/imgui"
#define LIBSDL2_VERSION "2.0.12"
#define LIBSDL2_URL "https://www.libsdl.org/"


#define MY_FAVOURITE_BORDER 5

#define HIDE_UNAVAILABLE_GAMES_TEXT "Hide unavailable games"
#define SHOW_ON_SCREEN_CONTROLS_TEXT "Show on-screen controls"
#define SHOW_CONFIGURATION_CONTROLS_TEXT "Show configuration controls"
#define RESET_CUSTOM_CONTROLS "Reset controls to default"
#define FULLSCREEN_TEXT "Fullscreen"
#define SHOW_DEAR_IMGUI_DEMO_TEXT "Show Dear ImGui demo"
#define MIN_MALLOC_SIZE 100
#define ROM_AVAILABLE_STATUS_NOT_FOUND 0 // Red
#define ROM_AVAILABLE_STATUS_FOUND 1     // Green
#define ROM_AVAILABLE_STATUS_UNKNOWN 2   // Yellow
#define NO_GAME_IMAGE_X 300
#define NO_GAME_IMAGE_Y 300
#define STB_IMAGE_IMPLEMENTATION
#define WRONG_FONT_TITLE "Wrong font"
#define ADD_JZINTV_COMMAND(x, y) {std::stringstream command_variable; std::cout << y << std::endl; add_jzintv_command(x, command_variable << y);}

#define USE_CURRENT_ROM_INDEX -1
#define GO_EXACTLY_AT_THIS_POSITION -2
#define NOT_NEEDED -1

#define DESCRIPTION_MAX_LENGTH 2500

#define BEFORE_EMULATION 1
#define AFTER_EMULATION 2
#define EXITING 3

static int start_gui();

#define NUM_PIXELS_BEFORE_SECTION_NAME 20
#define NUM_PIXELS_SPACE_SECTION_NAME 5
#define INTERFACE_OPTIONS_SECTION_NAME "Interface options"
#define JZINTV_OPTIONS_SECTION_NAME "jzIntv options"
#define TUTORVISION_OPTIONS_SECTION_NAME "Tutorvision options"
#define INTERFACE_GAME_INFO_SECTION_NAME "Game info"
#define JZINTV_CUSTOM_COMMANDS_SECTION_NAME "jzIntv parameters"
#define ECS_OPTIONS_SECTION_NAME "Ecs keyboard"
#define CHOOSE_ITEM_TEXT "Choose..."
#define CLEAR_TEXT "Clear"
#define RESET_TEXT "Reset"
#define yellow_col ImVec4(1.0f, 1.0f, 0.0f, 1.0f)
#define green_col ImVec4(0.0f, 1.0f, 0.0f, 1.0f)
#define red_col ImVec4(1.0f, 0.0f, 0.0f, 1.0f)
#define blue_col ImVec4(0.0f, 0.0f, 1.0f, 1.0f)
#define darkcian_col ImVec4(0.0f, 0.7f, 0.8f, 1.0f)
#define lightblue_col ImVec4(0.1f, 0.78f, 1.0f, 1.0f)

#define RIGHT_KEY_INDEX 1
#define LEFT_KEY_INDEX 2
#define DOWN_KEY_INDEX 3
#define UP_KEY_INDEX 4
#define ENTER_KEY_INDEX 5
#define TAB_KEY_INDEX 6
#define PGDOWN_KEY_INDEX 7
#define PGUP_KEY_INDEX 8

#define SIMPLE_SCROLL_MODE 0
#define RECOMPILE_MODE 1
#define RELOAD_ROMS_AND_RECOMPILE_MODE 2

struct app_config_struct_t {
    // Ini members: don't change order or ini won't save correctly!!
    // If you add members, add it also in copy_config function
    uint64_t execBinCrc32;
    uint64_t gromBinCrc32;
    uint64_t tutorvisionExecBinCrc32;
    uint64_t tutorvisionGromBinCrc32;
    uint64_t ecsBinCrc32;
    uint64_t window_width;
    uint64_t window_height;
    uint64_t buttons_size;
    uint64_t scrollbar_size;
    uint64_t last_crc32;
    uint64_t num_roms_jump;
    uint64_t font_size;
    uint64_t style_index;
    uint64_t jzintv_resolution_index;
    uint64_t mobile_default_portrait_controls_size;
    uint64_t mobile_default_landscape_controls_size;
    uint64_t mobile_ecs_portrait_alpha;
    uint64_t mobile_ecs_landscape_alpha;
    bool hide_unavailable_roms;
    bool window_maximized;
    bool jzintv_fullscreen;
    bool mobile_show_controls;
    bool mobile_show_configuration_controls;
    bool mobile_use_inverted_controls;
    // To align memory pointer for saving ini (really sad)
    bool dummy1;
    bool dummy2;
    double roms_list_width_percentage;
    double image_height_percentage;
    double mobile_portrait_top_gap_percentage;
    double mobile_portrait_bottom_gap_percentage;
    double mobile_landscape_left_gap_percentage;
    double mobile_landscape_right_gap_percentage;

    vector<string> *custom_commands;
    char *roms_folder_ini;
    char *keyboard_hack_file;
    char *palette_file;
    char *font_filename;

    SDL_FRect mobile_portrait_rect;
    SDL_FRect mobile_landscape_rect;

    // Technical members
    char *root_folder_for_configuration; // Fixed
    char *resource_folder_absolute_path; // Fixed
    char *internal_sd_path; // Fixed
    char *external_sd_path; // Fixed
    char *roms_folder_absolute_path; // Variable
    int num_total_crc32s;
    int num_valid_crc32s;
    bool mobile_mode;
    bool consume_mouse_events_only_for_simulate_controls;
    char *dialog_reference = NULL;
    int act_player;
    bool starting_game = false;
    bool ending_game = false;
    bool custom_font_loaded = false;
};

#define DISABLE_GLOBAL_VALUE "DISABLE_GLOBAL_VALUE"
struct rom_config_struct_t {
    // Ini members: don't change order or ini won't save correctly!!
    // If you add members, add it also in copy_config function
    char *game_name;
    char *description;
    char *image_file_name;
    char *box_file_name;
    char *keyboard_hack_file;
    char *palette_file;
    bool use_tutorvision_exec = false;
    bool use_tutorvision_grom = false;
    bool use_tutorvision_gram = false;
    bool ecs_tape_name_auto;
    bool jlp_save_file_auto;
    // To align memory pointer for saving ini (really sad)
    bool dummy3;
    bool dummy4;
    bool dummy5;
    vector<string> *custom_commands;
    SDL_FRect mobile_portrait_rect;
    SDL_FRect mobile_landscape_rect;
    vector<Control *> controls_delta[2];

    // Technical members
    uint32_t crc32;
    bool double_row;
    int available_status;
    char *file_name;
    int original_texture_screenshot_width;
    int original_texture_screenshot_height;
    int original_texture_box_width;
    int original_texture_box_height;
    uint32_t texture_screenshot;
    uint32_t texture_box;
    vector<Control *> controls[2];
};

#define MAX_CUSTOM_COMMANDS 10
struct backup_config_struct_t {
    app_config_struct_t app_flags;
    rom_config_struct_t rom_flags;
    // Original custom commands fields are of type string, but utils method want char*
    char **custom_commands_array_data;
    char **custom_commands_array_data_backup;
    bool hack_file_use_global;
    bool hack_file_was_global;
    bool palette_file_use_global;
    bool palette_file_was_global;
    bool custom_command_use_global[MAX_CUSTOM_COMMANDS];
    bool custom_command_was_global[MAX_CUSTOM_COMMANDS];
    bool font_required = false;
    bool reset_custom_controls = false;
};

struct gui_util_struct_t {
    ImVec2 border = {-1, -1};
    int act_window_width = 0;
    int act_window_height = 0;
    int reference_window_width = -1;
    int reference_window_height = -1;
    int mobile_major_size = 0;
    int mobile_minor_size = 0;
    bool portrait = true;
    int rom_index_selected = 0;
    int last_rom_index_selected = 0; // For double click management in mobile mode
    bool force_unhover = false;
    bool request_for_update_list_view = true; // Reload all roms or simply scroll the list
    bool list_is_dirty = true; // Positions and graphic must be updated
    bool update_graphic_child = false; // Positions are ok, we need just to update graphics (upDown splitted moved)
    bool force_list_to_dirty = false;
    bool reload_roms_on_refresh = true;
    bool changed_size_while_cleaning = false; // Mobile extreme case
    ImguiScrollable roms_list_scrollable;
    ImguiScrollable description_scrollable;
    ImguiScrollable options_scrollable;
    ImguiScrollable options_sub_scrollable;
    ImguiScrollable options_desc_scrollable;

    // Portrait
//    vector<float> *mobile_portrait_positions = NULL;   // Optimization for mobile: if we change orientation and we have valid data, don't recalculate positions
//    uint32_t mobile_portrait_scrollbar_pos = -1;       // Optimization for mobile: used to simply scroll without recalculate graphics (for current orientation)
//    int mobile_portrait_scrollbar_index = -1;          // Optimization for mobile: used to simply scroll without recalculate graphics (for other orientation)
//    float mobile_portrait_scrollbar_index_offset = -1; // Optimization for mobile: used to simply scroll without recalculate graphics (for other orientation)
    vector<float> portrait_positions_vec;
    float portrait_max_scrollbar_pos = -1;

    // Landscape
//    vector<float> *mobile_landscape_positions = NULL;  // Optimization for mobile: if we change orientation and we have valid data, don't recalculate positions
//    uint32_t mobile_landscape_scrollbar_pos = -1;      // Optimization for mobile: used to simply scroll without recalculate graphics (for current orientation)
//    int mobile_landscape_scrollbar_index = -1;         // Optimization for mobile: used to simply scroll without recalculate graphics (for other orientation)
//    float mobile_landscape_scrollbar_index_offset = -1;// Optimization for mobile: used to simply scroll without recalculate graphics (for other orientation)
    vector<float> landscape_positions_vec;
    float landscape_max_scrollbar_pos = -1;

    vector<float> *positions = NULL;

    // Positions and sizes
    ImVec2 main_window_pos;
    ImVec2 main_window_size;

    ImVec2 left_child_pos;
    ImVec2 left_child_size;

    ImVec2 right_child_size;

    ImVec2 roms_child_size;

    ImVec2 right_up_child_pos;
    ImVec2 right_up_child_size;

    ImVec2 right_down_child_pos;
    ImVec2 right_down_child_size;

    // Splitters
    float splitter_size_left = 0;
    float splitter_size_right = 0;
    float splitter_size_up = 0;
    float splitter_size_down = 0;
    float last_splitter_size_left = -1;
    float last_splitter_size_up = -1;
    float splitter_tickness = 0;
    int splitter_up_down_percent_in_pixels = -1;
    int splitter_left_right_percent_in_pixels = -1;
    ImRect left_right_splitter_rect;
    ImRect up_down_splitter_rect;

    std::vector<std::string> roms_names;
    ImVec2 selectable_imvec;
    int last_key_released;
    int par_int;
    float par_float;
    bool change_tab_index = false;
    int act_tab_index = 0;
    bool configuration_change_tab_index = false;
    int configuration_act_tab_index = 0;
    int last_rom_index_checked;
    ImVec2 title_bar_size;

    // Millis
    long last_rom_index_millis = 0; // For double click management in mobile mode
    long last_key_released_millis = 0; // Avoid multiple key released
    long last_roms_checked_millis = 0; // Used to load image in mobile mode landscape
    bool show_config_window;

    backup_config_struct_t backup;
    backup_config_struct_t backup_game;
    bool reset_backup_data = true;
    bool show_demo_window;
    bool flip_demo_window;
    bool show_loading = true;
    int roms_per_page; // Estimation, we don't need exact count
    int num_configs_tabs;
    bool center_at_rom_index_if_needed = true;
    bool force_center_at_rom_index = false;
    int left_gap_pixels = 0;
};

struct roms_list_struct_t {
    char exec_bin_file_name[100];
    char grom_bin_file_name[100];
    char tutorvision_exec_bin_file_name[100];
    char tutorvision_grom_bin_file_name[100];
    char ecs_bin_file_name[100];
    uint32_t exec_bin_crc32;
    uint32_t grom_bin_crc32;
    uint32_t tutorvision_exec_bin_crc32;
    uint32_t tutorvision_grom_bin_crc32;
    uint32_t ecs_bin_crc32;
    char *folder;
    int total_roms_num;
    int execBinStatus;
    int gromBinStatus;
    int tutorvisionExecBinStatus;
    int tutorvisionGromBinStatus;
    int ecsBinStatus;
    vector<rom_config_struct_t> list;
};

struct Hash_compute_integer_t {
    size_t operator()(const unsigned int &key, const int &hashSize = 1000) const {
        return std::hash<unsigned int>()(key) % hashSize;
    }
};
extern "C" {
#include "jzintv/event/event_tbl.h"
#include "jzintv/event/event_plat.h"
extern int force_sound_atten;
extern unsigned int file_crc32(char *fname);
extern int jzintv_entry_point(int argc, char *argv[]);
extern void event_enqueue_custom(int press_status, const char *ev_name);
}

// Main
extern struct app_config_struct_t app_config_struct;
extern struct roms_list_struct_t roms_list_struct;
extern vector<rom_config_struct_t> roms_configuration;
extern int font_size;
extern rom_config_struct_t *selected_rom;
extern ImGuiWindowFlags get_window_flags(bool no_title_bar, bool no_scroll_bar, bool noScrollWithMouse, bool no_focus);
extern void Push_buttons_size(bool original = true);
extern void Pop_buttons_size();
extern void set_tab_index(int index);
extern int get_tab_index();
extern int find_roms_config_index(int index);
extern void apply_style(int val = -1);
extern void reset_rom_config(rom_config_struct_t *act_rom_config, uint32_t crc32);
extern void sort_config_by_crc_32(vector<rom_config_struct_t> *vec);
extern void free_rom_config_struct(rom_config_struct_t *rom_config);
extern void clear_sdl_frect(SDL_FRect *frect);
extern int get_key_pressed();
extern void free_popup();
extern void resume_gui();
extern void suspend_gui();
extern bool start_emulation(int index);
extern void update_roms_list();
extern int get_pixel_percentage(int whole, double perc);
extern void get_gap_pixels(int *l, int *r, int *t, int *b);

// Main window
extern void manage_key_pressed_main(bool any_popup_visible);
extern void draw_main_window();
extern void check_for_update_list();

// Configuration window
extern void reset_backup_data();
extern void show_configuration_window(bool *refresh_for_text);
extern bool check_back_config_window();
extern void manage_key_pressed_config();

// Platform dependent
extern int setup_window(int desired_w, int desired_h, bool maximized, char *title);
extern void new_frame();
extern void get_window_size(int *w, int *h);
extern void render();
extern void clean(int mode);
extern void check_for_special_event(bool *exit, app_config_struct_t *config);
extern bool is_ok_permission();
extern long get_act_millis();
extern bool get_mobile_mode();
extern bool get_default_mobile_show_controls();
extern bool get_default_mobile_show_configuration_controls();
extern bool get_force_fullscreen();
extern SDL_FRect get_default_jzintv_rendering_frect(bool is_portrait);
extern char *get_root_folder_for_configuration();
extern void init_platform(int argc, char **argv);
extern void emulation_start();
extern void emulation_end();
extern char *get_forced_resolution_argument();
extern void custom_show_message(string message);
extern void on_render();
extern void on_font_change();
extern "C" void set_window(SDL_Window * w);

#ifdef __ANDROID__
extern char *get_internal_sd_path();
extern char *get_external_sd_path();
#endif
extern void openUrl(string url);
extern int get_default_font_size();
extern int get_default_buttons_size();
extern int get_default_scrollbar_size();

// Strings
extern char *wrap_string_by_size(char *str, int size);
extern char *replaceWord(const char *s, const char *oldW, const char *newW);
extern char *findWord(const char *s, const char *oldW);
extern bool startsWith(const char *a, const char *b);
extern std::vector<std::string> split(const char *str, const char *delimiter, bool accept_empty);
extern void trim(std::string &s);
extern void ltrim(std::string &s);
extern int strlen_trim(char *str);
extern float string_to_float(char *val);
extern uint32_t string_to_int(char *val);
extern bool string_to_bool(char *val);
extern uint32_t hex_to_uint32(char *val);
extern vector<std::string> get_vector_string(string str, int size);

// Messages
extern void add_message_by_stream(std::basic_ostream<char, std::char_traits<char>> &ostream, int level);
extern void free_message(int which);
extern void show_messages(roms_list_struct_t *rom_list_st);

// Ini
extern char properties_file_name[FILENAME_MAX];
extern void save_config_file();
extern void LoadIniSettingsFromMemory(const char *ini_data, size_t ini_size);
extern rom_config_struct_t *add_new_game_by_crc32(uint32_t crc32);
extern bool set_control_value(char *key, char *value, Control *act_control);
extern int remove_duplicates(vector<rom_config_struct_t> *vec, bool is_for_config);

// Memory
extern bool is_memory_empty(const char *ptr);
extern bool is_memory_blank(const char *ptr);

// File system
extern char *normalize_path(const char *path, bool is_directory);
extern void create_folder(string folder);
extern char *get_absolute_path(const char *original_root_path, const char *append_path, bool is_path_free, int *result);
extern bool exist_file(const char *fileName);
extern bool exist_folder(string pathname);
extern char *get_file_name(string pathname);
extern bool copy_file(string source, string destination);
extern std::string browse_item(std::string saved_path, const char *restore, bool directory, char **filter_descriptions = NULL, char **filter_extensions = NULL, int numFilters = 0);

// Images
extern void load_images(struct app_config_struct_t *app_conf);
extern void clear_general_textures();
extern void clear_roms_textures(struct roms_list_struct_t *roms_list_struct);
extern void draw_background(ImVec2 vec);
extern void draw_loading(ImVec2 vec);
extern void manage_image_window(ImVec2 size,
                                struct roms_list_struct_t *roms_list_struct,
                                struct app_config_struct_t *app_config_struct,
                                int tab_selected,
                                int rom_index_selected,
                                uint32_t last_landscape_scrollbar_pos,
                                ImguiScrollable *roms_list_scrollable,
                                long *last_millis_checked,
                                int *last_rom_index_checked);

// Gui events
extern std::vector<GuiEvent *> gui_events;
extern void submit_gui_event(int event, int param_int, float param_float);


// Controls
extern void clear_all_default_game_controls();
extern void clear_effective_and_config_game_controls();
extern void clear_controls(vector<Control *> *container);
extern void release_all_controls();
extern Control *get_control_by_touch_point(float x_perc, float y_perc, SDL_FRect *real_pos);
extern void manage_button_press_or_release_by_event(SDL_Event *old_event, bool pressed);
extern void manage_button_motion(SDL_Event *old_event);
extern void init_controls();
extern void normalize_default_controls();
extern void change_control_press_status(Control *c, bool pressed);
extern Control *get_old_control_pressed();
extern Control *add_new_delta_default_control_by_undecoded_event(const char *event);
extern void init_effective_and_config_game_controls(int conf_index);
extern Control *get_control_by_event(string event, vector<Control *> *container);
extern bool check_switch_mode();
extern bool manage_override_control_for_game(char *key, char *value, struct rom_config_struct_t *act_rom_config);
extern void normalize_controls_to_delta(vector<Control *> *new_delta_default_controls, int hand_index, bool source_is_destination = false);
extern void duplicate_controls(vector<Control *> *source, vector<Control *> *destination);
extern void refresh_rect_on_screen_size_change();
extern void remove_custom_controls();
extern void manage_on_release_controls_queue();

// Screen
extern int window_x;
extern int window_y;
extern float window_ratio_portrait;
extern float window_ratio_landscape;
extern void init_jzintv_screen_references();
extern SDL_Rect **get_act_jzintv_rendering_rect_ref(bool is_portrait);
extern SDL_FRect **get_act_jzintv_rendering_frect_ref(bool is_portrait);
extern SDL_Rect transform_to_sdl_rect(SDL_FRect *act_rect);
extern SDL_FRect *get_delta_screen_frect(bool is_portrait);
extern SDL_FRect *get_rom_screen_frect(rom_config_struct_t *rom, bool is_portrait);
extern void init_jzintv_rendering_rect(bool is_for_custom_game);
extern void normalize_screen_to_delta();
extern "C" void check_screen_change();

// Exceptions
extern void throw_by_stream(std::basic_ostream<char, std::char_traits<char>> &ostream);

// Popup
extern void add_popup_by_stream(const char *title, std::basic_ostream<char, std::char_traits<char>> &ostream);

// Events
extern bool manage_pause;
extern bool custom_emulation_paused;
extern void init_events();
extern void normalize_events();
extern void clear_events();
extern void reset_mappings();
extern void custom_pause_event();
extern void add_event_to_monitor(const char *event);

#endif
