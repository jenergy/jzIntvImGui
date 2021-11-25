/*
Mit license
Copyright 2021 Daniele Moglia

 Permission is hereby granted, free of charge, to
 any person obtaining a copy of this software and
 associated documentation files (the "Software"),
 to deal in the Software without restriction,
 including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit
 persons to whom the Software is furnished to do so,
 subject to the following conditions:

 The above copyright notice and this permission
 notice shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

// Thanks:
// jzIntv http://spatula-city.org/~im14u2c/intv/
// Dear ImGui https://github.com/ocornut/imgui
// CLion project thanks to https://github.com/joelcancela/ImguiDemoCLion
// Switch imgui glfw https://github.com/MstrVLT/switch_imgui_glfw
// Android imgui https://github.com/sfalexrog/Imgui_Android
// Lazyfoo native example https://lazyfoo.net/tutorials/SDL/52_hello_mobile/index.php
// LibSDl2_Image with cmake https://trenki2.github.io/blog/2017/07/04/using-sdl2-image-with-cmake/
// FileDialog c++ dll https://docs.microsoft.com/it-it/samples/microsoft/windows-classic-samples/open-dialog-box-sample/
// FileDialog c++ dll https://www.daniweb.com/programming/software-development/threads/446920/setting-a-hook-for-getopenfilename
// Nativefiledialog-extended https://github.com/btzy/nativefiledialog-extended
// Imgui File dialog https://github.com/aiekick/ImGuiFileDialog

#include "main.h"
#include "includes_specific.h"

#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

// Configurazione app da file di config app
struct app_config_struct_t app_config_struct;

// Lista roms da file ini
vector<rom_config_struct_t> roms_configuration;

// Lista roms trovate
struct roms_list_struct_t roms_list_struct;

static ImFont *custom_font;

int font_size;
rom_config_struct_t *selected_rom;
extern Popup *popup;
extern long loading_millis;
ImVec2 frame_padding;
gui_util_struct_t gui_util_str;

void free_rom_config_struct(rom_config_struct_t *rom_config) {
    if (rom_config->game_name != nullptr) {
        free(rom_config->game_name);
        rom_config->game_name = nullptr;
    }
    if (rom_config->description != nullptr) {
        free(rom_config->description);
        rom_config->description = nullptr;
    }
    if (rom_config->image_file_name != nullptr) {
        free(rom_config->image_file_name);
        rom_config->image_file_name = nullptr;
    }
    if (rom_config->box_file_name != nullptr) {
        free(rom_config->box_file_name);
        rom_config->box_file_name = nullptr;
    }
    if (rom_config->file_name != nullptr) {
        free(rom_config->file_name);
        rom_config->file_name = nullptr;
    }
    if (rom_config->keyboard_hack_file != nullptr) {
        free(rom_config->keyboard_hack_file);
        rom_config->keyboard_hack_file = nullptr;
    }
    if (rom_config->palette_file != nullptr) {
        free(rom_config->palette_file);
        rom_config->palette_file = nullptr;
    }
    if (rom_config->custom_commands != nullptr) {
        vector<string> &vec = *(rom_config->custom_commands);
        vec.clear();
        delete rom_config->custom_commands;
    }
    rom_config->custom_commands = nullptr;

    if (rom_config->texture_screenshot > 0) {
        glDeleteTextures(1, &(rom_config->texture_screenshot));
    }
    rom_config->texture_screenshot = 0;

    if (rom_config->texture_box > 0) {
        glDeleteTextures(1, &(rom_config->texture_box));
    }
    rom_config->texture_box = 0;
    clear_controls(&(rom_config->controls[0]));
    clear_controls(&(rom_config->controls[1]));
    clear_controls(&(rom_config->controls_delta[0]));
    clear_controls(&(rom_config->controls_delta[1]));
}

char *get_curr_folder() {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    return normalize_path(cwd, true);
}

char *get_default_roms_folder_relative() {
    return strdup("./resources/Roms/");
}

void clear_sdl_frect(SDL_FRect *frect) {
    frect->x = -1;
    frect->y = -1;
    frect->w = -1;
    frect->h = -1;
}

static bool compareByCrc32(const rom_config_struct_t &a, const rom_config_struct_t &b) {
    return a.crc32 < b.crc32;
}

void sort_config_by_crc_32(vector<rom_config_struct_t> *vec) {
    std::sort(vec->begin(), vec->end(), compareByCrc32);
}

static void apply_default_settings() {
    int result;
    app_config_struct_t *app_conf = &app_config_struct;

    app_conf->root_folder_for_configuration = get_root_folder_for_configuration();
#ifdef __ANDROID__
    app_conf->internal_sd_path = get_internal_sd_path();
    app_conf->external_sd_path = get_external_sd_path();
#endif

    app_conf->roms_folder_ini = get_default_roms_folder_relative();
    app_conf->roms_folder_absolute_path = get_absolute_path(app_conf->root_folder_for_configuration, app_conf->roms_folder_ini, true, &result);

    app_conf->resource_folder_absolute_path = get_absolute_path(app_conf->root_folder_for_configuration, "./resources", true, &result);
    if (!exist_folder(app_conf->resource_folder_absolute_path)) {
        ADD_POPUP("Resource folder not found", "Resource folder not found");
    }

    app_conf->keyboard_hack_file = strdup("");
    app_conf->palette_file = strdup("");

    app_conf->execBinCrc32 = hex_to_uint32((char *) "0xCBCE86F7");
    app_conf->gromBinCrc32 = hex_to_uint32((char *) "0x683A4158");
    app_conf->tutorvisionExecBinCrc32 = hex_to_uint32((char *) "0x7558A4CF");
    app_conf->tutorvisionGromBinCrc32 = hex_to_uint32((char *) "0x82736456");
    app_conf->ecsBinCrc32 = hex_to_uint32((char *) "0xEA790A06");

    app_conf->mobile_ecs_portrait_alpha = 0;
    app_conf->mobile_ecs_landscape_alpha = 128;

    app_conf->window_width = 1024;
    app_conf->window_height = 768;

    app_conf->roms_list_width_percentage = 50;
    app_conf->image_height_percentage = 45;

    app_conf->num_roms_jump = 20;
    app_conf->hide_unavailable_roms = true;
    app_conf->last_crc32 = 0;

    app_conf->mobile_mode = get_mobile_mode();

    app_conf->font_filename = strdup("");
    app_conf->font_size = get_default_font_size();
    app_conf->buttons_size = get_default_buttons_size();
    app_conf->scrollbar_size = get_default_scrollbar_size();
    font_size = app_conf->font_size;
    app_conf->style_index = 0;
    app_conf->jzintv_resolution_index = 1;

    app_conf->window_maximized = false;

    app_conf->mobile_portrait_top_gap_percentage = 0;
    app_conf->mobile_portrait_bottom_gap_percentage = 5;
    app_conf->mobile_landscape_left_gap_percentage = 0;
    app_conf->mobile_landscape_right_gap_percentage = 0;
    clear_sdl_frect(&app_conf->mobile_landscape_rect);
    clear_sdl_frect(&app_conf->mobile_portrait_rect);

    app_conf->consume_mouse_events_only_for_simulate_controls = false;
    app_conf->mobile_show_controls = get_default_mobile_show_controls();
    app_conf->mobile_show_configuration_controls = get_default_mobile_show_configuration_controls();
    app_conf->mobile_default_portrait_controls_size = 4;
    app_conf->mobile_default_landscape_controls_size = 2;
    app_conf->jzintv_fullscreen = false;
    app_conf->mobile_use_inverted_controls = false;
    app_conf->act_player = 0;

    app_conf->custom_commands = new vector<string>();

//    if (app_config_struct.mobile_mode) {
        gui_util_str.splitter_tickness = 12.0f;
//    } else {
//        gui_util_str.splitter_tickness = 3.0f;
//    }
}

static void load_configuration() {
    app_config_struct_t *app_conf = &app_config_struct;
    apply_default_settings();

    sprintf(properties_file_name, "%s%s", app_conf->root_folder_for_configuration, "jzIntvImGui.ini");
    if (exist_file(properties_file_name)) {
        size_t file_data_size;
        char *file_data = (char *) ImFileLoadToMemory(properties_file_name, "rb", &file_data_size);
        try {
            if (file_data != nullptr) {
                LoadIniSettingsFromMemory(file_data, file_data_size);
                IM_FREE(file_data);
            } else {
                ADD_CONFIG_WARNING("Unable to read from file " << properties_file_name);
            }
        } catch (const std::stringstream &ex) {
            ADD_CONFIG_WARNING(ex.str().c_str() << " - Skipping remaining configuration values");
        }
        catch (...) {
            ADD_CONFIG_WARNING("Error parsing file " << (char *) properties_file_name << " - Skipping remaining configuration values");
        }
    } else {
        ADD_CONFIG_WARNING("Cannot find file " << (char *) properties_file_name);
    }

    free(app_conf->roms_folder_absolute_path);
    int result;
    app_conf->roms_folder_absolute_path = get_absolute_path(app_conf->root_folder_for_configuration, app_conf->roms_folder_ini, true, &result);
    if (result == -1) {
        ADD_CONFIG_WARNING("roms_folder invalid: " << app_conf->roms_folder_ini);
    }
}

static void free_roms_list(struct roms_list_struct_t *str) {
    if (str->folder != nullptr) {
        free(str->folder);
    }

    clear_roms_textures(str);

    for (int i = 0; i < str->total_roms_num; i++) {
        free_rom_config_struct(&(str->list[i]));
    }
    if (str->list.size() > 0) {
        str->list.clear();
    }
    str->total_roms_num = 0;
}

static void copy_config(rom_config_struct_t *original_config, rom_config_struct_t *act_config) {
    act_config->game_name = nullptr;
    if (original_config->game_name != nullptr) {
        act_config->game_name = strdup(original_config->game_name);
    }

    act_config->description = nullptr;
    if (original_config->description != nullptr) {
        act_config->description = strdup(original_config->description);
    }

    act_config->file_name = nullptr;
    if (original_config->file_name != nullptr) {
        act_config->file_name = strdup(original_config->file_name);
    }

    act_config->image_file_name = nullptr;
    if (original_config->image_file_name != nullptr) {
        act_config->image_file_name = strdup(original_config->image_file_name);
    }

    act_config->box_file_name = nullptr;
    if (original_config->box_file_name != nullptr) {
        act_config->box_file_name = strdup(original_config->box_file_name);
    }

    if (original_config->keyboard_hack_file != nullptr) {
        act_config->keyboard_hack_file = strdup(original_config->keyboard_hack_file);
    } else {
        act_config->keyboard_hack_file = strdup("");
    }

    if (original_config->palette_file != nullptr) {
        act_config->palette_file = strdup(original_config->palette_file);
    } else {
        act_config->palette_file = strdup("");
    }

    // Tutorvision
    act_config->use_tutorvision_exec = original_config->use_tutorvision_exec;
    act_config->use_tutorvision_grom = original_config->use_tutorvision_grom;
    act_config->use_tutorvision_gram = original_config->use_tutorvision_gram;

    // Ecs
    act_config->ecs_tape_name_auto = original_config->ecs_tape_name_auto;

    // Jlp
    act_config->jlp_save_file_auto = original_config->jlp_save_file_auto;

    act_config->custom_commands = new vector<string>();
    if (original_config->custom_commands != nullptr) {
        vector<string> &source_vec = *(original_config->custom_commands);
        vector<string> &dest_vec = *(act_config->custom_commands);
        for (int i = 0; i < source_vec.size(); i++) {
            dest_vec.push_back(source_vec[i]);
        }
    }

    duplicate_controls(&original_config->controls[0], &act_config->controls[0]);
    duplicate_controls(&original_config->controls[1], &act_config->controls[1]);

    memcpy(&act_config->mobile_portrait_rect, &original_config->mobile_portrait_rect, sizeof(SDL_FRect));
    memcpy(&act_config->mobile_landscape_rect, &original_config->mobile_landscape_rect, sizeof(SDL_FRect));

    act_config->crc32 = original_config->crc32;
    act_config->double_row = original_config->double_row;
    act_config->available_status = original_config->available_status;
    act_config->original_texture_screenshot_width = original_config->original_texture_screenshot_width;
    act_config->original_texture_screenshot_height = original_config->original_texture_screenshot_height;
    act_config->original_texture_box_width = original_config->original_texture_box_width;
    act_config->original_texture_box_height = original_config->original_texture_box_height;
    act_config->texture_screenshot = original_config->texture_screenshot;
    act_config->texture_box = original_config->texture_box;
}

void reset_rom_config(rom_config_struct_t *act_rom_config, uint32_t crc32) {
    act_rom_config->description = nullptr;
    act_rom_config->file_name = nullptr;
    act_rom_config->original_texture_box_height = 0;
    act_rom_config->original_texture_box_width = 0;
    act_rom_config->original_texture_screenshot_height = 0;
    act_rom_config->original_texture_screenshot_width = 0;
    act_rom_config->crc32 = 0;
    act_rom_config->double_row = false;
    act_rom_config->available_status = 0;
    act_rom_config->image_file_name = nullptr;
    act_rom_config->texture_screenshot = 0;
    act_rom_config->texture_box = 0;
    act_rom_config->box_file_name = nullptr;
    act_rom_config->crc32 = crc32;
    act_rom_config->game_name = nullptr;
    act_rom_config->keyboard_hack_file = nullptr;
    act_rom_config->palette_file = nullptr;
    act_rom_config->use_tutorvision_exec = false;
    act_rom_config->use_tutorvision_grom = false;
    act_rom_config->use_tutorvision_gram = false;
    act_rom_config->ecs_tape_name_auto = false;
    act_rom_config->jlp_save_file_auto = false;

    clear_sdl_frect(&act_rom_config->mobile_landscape_rect);
    clear_sdl_frect(&act_rom_config->mobile_portrait_rect);

    act_rom_config->custom_commands = nullptr;
}

static bool compareByName(const rom_config_struct_t &a, const rom_config_struct_t &b) {
    string aa = a.game_name;
    std::transform(aa.begin(), aa.end(), aa.begin(), ::toupper);
    string bb = b.game_name;
    std::transform(bb.begin(), bb.end(), bb.begin(), ::toupper);
    return aa < bb;
}

static void manage_new_frame() {
    try {
        new_frame();
    } catch (const std::exception &ex) {
        char *title = strdup(ex.what());
        if (!strcmp(title, WRONG_FONT_TITLE)) {
            ADD_POPUP("Invalid font", "An invalid font has been specified:" << app_config_struct.font_filename);
            custom_font = nullptr;
            ImGuiIO &io = ImGui::GetIO();
            io.Fonts->Fonts.clear();
            io.Fonts->AddFontDefault();
            app_config_struct.custom_font_loaded = false;
            new_frame();
        }
        free(title);
    }
}

static void step_loading(int w, int h) {
    static int count = 0;
    if (!gui_util_str.show_loading) {
        count++;
        if (count % 10 == 0) {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            draw_loading(ImVec2(w, h));
            ImGui::PopStyleColor();
            render();
            manage_new_frame();
        }
    }
}

static void load_roms(struct app_config_struct_t *app_conf, struct roms_list_struct_t *roms_list_struct_ptr, vector<rom_config_struct_t> roms_config, bool hide_unavailable) {

    // Non troppo corretto funzionalmente...
    for (int i = 0; i < app_conf->num_valid_crc32s; i++) {
        roms_config[i].available_status = ROM_AVAILABLE_STATUS_NOT_FOUND;
    }

    vector<rom_config_struct_t> found_roms;
    struct dirent *Dirent;
    DIR *dir;
    char *p;

    dir = opendir(roms_list_struct_ptr->folder);
    Log(LOG_INFO) << "Reloading roms list from " << roms_list_struct_ptr->folder;

    roms_list_struct_ptr->execBinStatus = 0;
    roms_list_struct_ptr->gromBinStatus = 0;

    roms_list_struct_ptr->tutorvisionExecBinStatus = 0;
    roms_list_struct_ptr->tutorvisionGromBinStatus = 0;
    roms_list_struct_ptr->ecsBinStatus = 0;

    if (dir != nullptr) {
        int w;
        int h;
        if (!gui_util_str.show_loading) {
            if (loading_millis == -1) {
                loading_millis = get_act_millis();
            }
            get_window_size(&w, &h);
            if (app_config_struct.custom_font_loaded) {
                ImGui::PopFont();
            }
            app_config_struct.custom_font_loaded = false;
        }

        while ((Dirent = readdir(dir)) != nullptr) {
            step_loading(w, h);
            if (Dirent->d_type == DT_DIR && strcmp(Dirent->d_name, ".")) {
            } else {
                if (strlen(Dirent->d_name) < 4) continue;
                p = Dirent->d_name + (strlen(Dirent->d_name) - 4);
                if ((*p) != '.')continue;

                char complete_filename[FILENAME_MAX];
                sprintf(complete_filename, "%s/%s", roms_list_struct_ptr->folder, Dirent->d_name);
                uint32_t crc32 = file_crc32(complete_filename);

                if (crc32 == app_conf->execBinCrc32) {
                    roms_list_struct_ptr->exec_bin_crc32 = crc32;
                    bool save_name = false;
                    if (!strcmp(Dirent->d_name, "exec.bin")) {
                        roms_list_struct_ptr->execBinStatus = 1;
                        save_name = true;
                    } else if (roms_list_struct_ptr->execBinStatus != 1) {
                        roms_list_struct_ptr->execBinStatus = 2;
                        save_name = true;
                    }
                    if (save_name) {
                        sprintf(roms_list_struct_ptr->exec_bin_file_name, "%s", Dirent->d_name);
                    }
                    continue;
                } else if (crc32 == app_conf->gromBinCrc32) {
                    roms_list_struct_ptr->grom_bin_crc32 = crc32;
                    bool save_name = false;
                    if (!strcmp(Dirent->d_name, "grom.bin")) {
                        roms_list_struct_ptr->gromBinStatus = 1;
                        save_name = true;
                    } else if (roms_list_struct_ptr->gromBinStatus != 1) {
                        roms_list_struct_ptr->gromBinStatus = 2;
                        save_name = true;
                    }
                    if (save_name) {
                        sprintf(roms_list_struct_ptr->grom_bin_file_name, "%s", Dirent->d_name);
                    }
                    continue;
                } else if (crc32 == app_conf->tutorvisionExecBinCrc32) {
                    roms_list_struct_ptr->tutorvision_exec_bin_crc32 = crc32;
                    bool save_name = false;
                    if (!strcmp(Dirent->d_name, "wbexec.bin")) {
                        roms_list_struct_ptr->tutorvisionExecBinStatus = 1;
                        save_name = true;
                    } else if (roms_list_struct_ptr->tutorvisionExecBinStatus != 1) {
                        roms_list_struct_ptr->tutorvisionExecBinStatus = 2;
                        save_name = true;
                    }
                    if (save_name) {
                        sprintf(roms_list_struct_ptr->tutorvision_exec_bin_file_name, "%s", Dirent->d_name);
                    }
                    continue;
                } else if (crc32 == app_conf->tutorvisionGromBinCrc32) {
                    roms_list_struct_ptr->tutorvision_grom_bin_crc32 = crc32;
                    bool save_name = false;
                    if (!strcmp(Dirent->d_name, "gromintv.bin")) {
                        roms_list_struct_ptr->tutorvisionGromBinStatus = 1;
                        save_name = true;
                    } else if (roms_list_struct_ptr->tutorvisionGromBinStatus != 1) {
                        roms_list_struct_ptr->tutorvisionGromBinStatus = 2;
                        save_name = true;
                    }
                    if (save_name) {
                        sprintf(roms_list_struct_ptr->tutorvision_grom_bin_file_name, "%s", Dirent->d_name);
                    }
                    continue;
                } else if (crc32 == app_conf->ecsBinCrc32) {
                    roms_list_struct_ptr->ecs_bin_crc32 = crc32;
                    bool save_name = false;
                    if (!strcmp(Dirent->d_name, "ecs.bin")) {
                        roms_list_struct_ptr->ecsBinStatus = 1;
                        save_name = true;
                    } else if (roms_list_struct_ptr->ecsBinStatus != 1) {
                        roms_list_struct_ptr->ecsBinStatus = 2;
                        save_name = true;
                    }
                    if (save_name) {
                        sprintf(roms_list_struct_ptr->ecs_bin_file_name, "%s", Dirent->d_name);
                    }
                    continue;
                }

                if (!strcmp(Dirent->d_name, "ecs.bin")) {
                    roms_list_struct_ptr->ecsBinStatus = 3;
                    roms_list_struct_ptr->ecs_bin_crc32 = crc32;
                } else if (!strcmp(Dirent->d_name, "grom.bin")) {
                    roms_list_struct_ptr->gromBinStatus = 3;
                    roms_list_struct_ptr->grom_bin_crc32 = crc32;
                } else if (!strcmp(Dirent->d_name, "exec.bin")) {
                    roms_list_struct_ptr->execBinStatus = 3;
                    roms_list_struct_ptr->exec_bin_crc32 = crc32;
                } else if (!strcmp(Dirent->d_name, "wbexec.bin")) {
                    roms_list_struct_ptr->tutorvisionExecBinStatus = 3;
                    roms_list_struct_ptr->tutorvision_exec_bin_crc32 = crc32;
                } else if (!strcmp(Dirent->d_name, "gromintv.bin")) {
                    roms_list_struct_ptr->tutorvisionGromBinStatus = 3;
                    roms_list_struct_ptr->tutorvision_grom_bin_crc32 = crc32;
                }

                if (((strcasecmp(p, ".bin")) &&
                     (strcasecmp(p, ".rom")) &&
                     (strcasecmp(p, ".int")) &&
                     (strcasecmp(p, ".cc3")) &&
                     (strcasecmp(p, ".luigi")) &&
                     (strcasecmp(p, ".itv"))) || (
                            (!strcasecmp(Dirent->d_name, "exec.bin")) ||
                            (!strcasecmp(Dirent->d_name, "grom.bin")) ||
                            (!strcasecmp(Dirent->d_name, "wbexec.bin")) ||
                            (!strcasecmp(Dirent->d_name, "gromintv.bin")) ||
                            (!strcasecmp(Dirent->d_name, "ecs.bin"))) || crc32 == 0)
                    continue;

                rom_config_struct_t config;
                reset_rom_config(&config, crc32);
                config.file_name = strdup(Dirent->d_name);
                config.available_status = ROM_AVAILABLE_STATUS_UNKNOWN;
                clear_sdl_frect(&config.mobile_landscape_rect);
                clear_sdl_frect(&config.mobile_portrait_rect);
                found_roms.push_back(config);
            }
        }
        if (!gui_util_str.show_loading) {
            if (custom_font != nullptr) {
                ImGui::PushFont(custom_font);
                app_config_struct.custom_font_loaded = true;
            }
        }
    } else {
        ADD_POPUP("Path invalid", "Roms path invalid:" << roms_list_struct_ptr->folder);
    }

    closedir(dir);
    sort_config_by_crc_32(&found_roms);
    remove_duplicates(&found_roms, false);

    // Check cross infos
    int q = 0;
    int unknown = 0;
    uint32_t prev_crc32 = 0;
    char prev_name[100];
    for (int i = 0; i < found_roms.size(); i++) {
        uint32_t crc32 = found_roms[i].crc32;
        if (app_config_struct.num_valid_crc32s > 0) {
            while (roms_config[q].crc32 < crc32) {
                if (q >= app_config_struct.num_valid_crc32s - 1) {
                    break;
                }
                q++;
            }
        }

        if (app_config_struct.num_valid_crc32s > 0 && roms_config[q].crc32 == crc32) {
            // Trovato in config file
            roms_config[q].available_status = ROM_AVAILABLE_STATUS_FOUND;
            found_roms[i].available_status = ROM_AVAILABLE_STATUS_FOUND;

            // Anche qui non troppo corretto funzionalmente, ma è temporaneo
            roms_config[q].file_name = found_roms[i].file_name;
        } else {
            // Non trovato in config file
            if (crc32 != prev_crc32) {
                found_roms[i].game_name = strdup(found_roms[i].file_name);
                unknown++;
            } else {
                found_roms[i].game_name = strdup("");
            }
        }
        if (crc32 == prev_crc32) {
            ADD_WARNING("Same binary data for files '" << prev_name << "' and '" << found_roms[i].file_name << "'");
        }
        prev_crc32 = crc32;
        sprintf(prev_name, "%s", found_roms[i].file_name);
    }

    int shown_valid_crc32s = app_config_struct.num_valid_crc32s;
    if (hide_unavailable) {
        shown_valid_crc32s = 0;
        for (int i = 0; i < app_config_struct.num_valid_crc32s; i++) {
            if (roms_config[i].available_status != ROM_AVAILABLE_STATUS_NOT_FOUND) {
                shown_valid_crc32s++;
            }
        }
    }

    // Merge infos
    roms_list_struct_ptr->total_roms_num = shown_valid_crc32s + unknown;
    roms_list_struct_ptr->list.clear();
    struct rom_config_struct_t *act_config;

    q = -1;
    for (int i = 0; i < app_config_struct.num_valid_crc32s; i++) {
        rom_config_struct_t *cfg = &(roms_config[i]);
        if (!hide_unavailable || cfg->available_status != ROM_AVAILABLE_STATUS_NOT_FOUND) {
            q++;
            rom_config_struct_t config;
            act_config = &config;
            reset_rom_config(act_config, 0);
            act_config->controls[0].clear();
            act_config->controls[1].clear();
            act_config->controls_delta[0].clear();
            act_config->controls_delta[1].clear();
            copy_config(cfg, act_config);
            roms_list_struct_ptr->list.push_back(config);
        }
        // Ripristino config da ini
        cfg->file_name = nullptr;
    }

    int offs = 0;
    for (int i = 0; i < found_roms.size(); i++) {
        rom_config_struct_t *cfg = &(found_roms[i]);
        if (cfg->available_status == ROM_AVAILABLE_STATUS_UNKNOWN && strcmp(cfg->game_name, "")) {
            rom_config_struct_t config;
            act_config = &config;
            reset_rom_config(act_config, 0);
            act_config->controls[0].clear();
            act_config->controls[1].clear();
            act_config->controls_delta[0].clear();
            act_config->controls_delta[1].clear();
            copy_config(cfg, act_config);
            roms_list_struct_ptr->list.push_back(config);
            offs++;
        }
    }

    for (int i = 0; i < found_roms.size(); i++) {
        free_rom_config_struct(&(found_roms[i]));
    }
    found_roms.clear();

    std::sort(roms_list_struct_ptr->list.begin(), roms_list_struct_ptr->list.end(), compareByName);
    if (!app_config_struct.mobile_mode) {
        remove_custom_controls();
    }
}

ImGuiWindowFlags get_window_flags(bool no_title_bar, bool no_scroll_bar, bool noScrollWithMouse, bool no_focus) {
    ImGuiWindowFlags window_flags = 0;
    if (no_title_bar) {
        window_flags |= ImGuiWindowFlags_NoTitleBar;
    }
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoMove;
    if (no_scroll_bar) {
        window_flags |= ImGuiWindowFlags_NoScrollbar;
    }
    if (noScrollWithMouse) {
        window_flags |= ImGuiWindowFlags_NoScrollWithMouse;
    }
    if (no_focus) {
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    }
    return window_flags;
}

static void apply_custom_style() {
    ImGui::StyleColorsDark();
    ImGuiStyle *style = &ImGui::GetStyle();
    ImVec4 *colors = style->Colors;
    colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(9, 13, 60, 190));
    colors[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(48, 29, 11, 138));
    colors[ImGuiCol_TitleBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(105, 7, 5, 255));
}

void apply_style(int val) {
    if (val == -1) {
        val = app_config_struct.style_index;
    }
    switch (val) {
        case 0:
            apply_custom_style();
            break;
        case 1:
            ImGui::StyleColorsClassic();
            break;
        case 2:
            ImGui::StyleColorsDark();
            break;
        case 3:
            ImGui::StyleColorsLight();
            break;
        default:
            app_config_struct.style_index = 0;
            apply_custom_style();
            break;
    }
}

static void add_style() {
    ImGuiStyle &style = ImGui::GetStyle();
    style.FrameBorderSize = 1;
    style.FrameRounding = 4;
    style.WindowBorderSize = 1;
    style.WindowRounding = 6;
    style.ScrollbarRounding = 4;
    style.TabRounding = 2;
    style.GrabMinSize = 40;
    style.ScrollbarSize = app_config_struct.scrollbar_size;
    style.GrabRounding = 12;
    style.ButtonTextAlign = ImVec2(0, 0.5);
    style.SelectableTextAlign = ImVec2(0, 0.5);

    if (strcmp(app_config_struct.font_filename, "")) {
        ImGuiIO &io = ImGui::GetIO();
        char fileName[FILENAME_MAX];
        sprintf(fileName, "%s%s/%s", app_config_struct.resource_folder_absolute_path, "Fonts", app_config_struct.font_filename);
        if (!exist_file(fileName)) {
            ADD_POPUP("Font not found", "Unable to find font file specified in ini file: " << fileName);
        } else {
            try {
                custom_font = io.Fonts->AddFontFromFileTTF(fileName, app_config_struct.font_size);
            } catch (...) {
                custom_font = nullptr;
                app_config_struct.custom_font_loaded = false;
                ADD_POPUP("Font not valid", "Unable to load font file:" << fileName);
            }
        }
    }
    apply_style();
}

static void add_jzintv_command(vector<string> *commands_list, std::basic_ostream<char, std::char_traits<char>> &ostream) {
    std::stringstream ss;
    ss << ostream.rdbuf();
    string command = ss.str();

    char *formatted;
#ifdef WIN32
    formatted = replaceWord(command.c_str(), "/", "\\");
#else
    formatted = replaceWord(command.c_str(), "sdmc:", "");
#endif
    commands_list->push_back(formatted);
    free(formatted);
}

static bool check_file_presence(const char *main_message, char *fileName) {
    std::stringstream file;
    file << app_config_struct.roms_folder_absolute_path << fileName;
    if (!exist_file(file.str().c_str())) {
        ADD_POPUP(main_message, main_message << ":\n" << fileName << "\n\n");
        return false;
    }
    return true;
}

static char **convert_to_argv_argc(vector<string> *commands, int *argc) {
    set<string> set_of_string;
    for (int i = 0; i < commands->size(); i++) {
        set_of_string.insert((*commands)[i]);
    }

    *argc = set_of_string.size();
    char **argv = static_cast<char **>(malloc((*argc) * sizeof(char *)));
    set_of_string.clear();

    int offs = -1;
    for (int i = 0; i < commands->size(); i++) {
        if (set_of_string.find((*commands)[i]) == set_of_string.end()) {
            set_of_string.insert((*commands)[i]);
            offs++;
            char *new_command = strdup(((*commands)[i]).c_str());
            argv[offs] = new_command;
        }
    }
    return argv;
}

string get_custom_command(int rom_index, int cc_index, bool *overridden) {
    string res = "";
    bool override = false;

    vector<string> custom_cc_vec;
    string custom_cc_string;

    if (roms_list_struct.list[rom_index].custom_commands != nullptr) {
        custom_cc_vec = *(roms_list_struct.list[rom_index].custom_commands);
        custom_cc_string = custom_cc_vec.size() > cc_index ? custom_cc_vec[cc_index] : "";
    }

    vector<string> global_cc_vec;
    string global_cc_string;

    if (app_config_struct.custom_commands != nullptr) {
        global_cc_vec = *(app_config_struct.custom_commands);
        global_cc_string = global_cc_vec.size() > cc_index ? global_cc_vec[cc_index] : "";
    }

    if (!custom_cc_string.empty()) {
        if (custom_cc_string.compare(DISABLE_GLOBAL_VALUE)) {
            override = true;
            res = custom_cc_string;
        }
    } else if (!global_cc_string.empty()) {
        res = global_cc_string;
    }
    if (overridden != nullptr) {
        *overridden = override;
    }
    return res;
}

char *get_keyboard_hack_file(int index, bool *overridden) {
    char buff[100];
    sprintf(buff, "%s", "");
    bool override = false;
    if (!is_memory_empty(roms_list_struct.list[index].keyboard_hack_file)) {
        if (strcmp(roms_list_struct.list[index].keyboard_hack_file, DISABLE_GLOBAL_VALUE)) {
            override = true;
            sprintf(buff, "%s", roms_list_struct.list[index].keyboard_hack_file);
        }
    } else if (!is_memory_empty(app_config_struct.keyboard_hack_file)) {
        sprintf(buff, "%s", app_config_struct.keyboard_hack_file);
    }
    if (overridden != nullptr) {
        *overridden = override;
    }
    return strdup(buff);
}

char *get_palette_file(int index, bool *overridden) {
    char buff[100];
    sprintf(buff, "%s", "");
    bool override = false;
    if (!is_memory_empty(roms_list_struct.list[index].palette_file)) {
        if (strcmp(roms_list_struct.list[index].palette_file, DISABLE_GLOBAL_VALUE)) {
            override = true;
            sprintf(buff, "%s", roms_list_struct.list[index].palette_file);
        }
    } else if (!is_memory_empty(app_config_struct.palette_file)) {
        sprintf(buff, "%s", app_config_struct.palette_file);
    }
    if (overridden != nullptr) {
        *overridden = override;
    }
    return strdup(buff);
}

extern "C" void manage_screenshot_file(char *filename) {
    ostringstream message;
    if (exist_file(filename)) {
        vector<string> prefixes;

        string prefix_file = roms_list_struct.list[gui_util_str.rom_index_selected].file_name;
        prefix_file = prefix_file.substr(0, prefix_file.length() - 4);
        prefixes.push_back(prefix_file);

        int index = find_roms_config_index(gui_util_str.rom_index_selected);
        if (index != -1) {
            prefix_file = roms_configuration[index].game_name;
            char *tmp = replaceWord(prefix_file.c_str(), ":", "");
            prefix_file = tmp;
            free(tmp);
            tmp = replaceWord(prefix_file.c_str(), "/", "");
            prefix_file = tmp;
            free(tmp);
            tmp = replaceWord(prefix_file.c_str(), "\?", "");
            prefix_file = tmp;
            free(tmp);
            tmp = replaceWord(prefix_file.c_str(), "\\", "");
            prefix_file = tmp;
            free(tmp);
            prefixes.push_back(prefix_file);
        }

        ostringstream final_file;
        bool copied = false;

        for (int q = prefixes.size() - 1; q >= 0; q--) {
            prefix_file = prefixes.at(q);
            ostringstream oss_complete_prefix;
            oss_complete_prefix << app_config_struct.resource_folder_absolute_path << "Images/Screenshots/" << prefix_file;
            final_file.str("");
            final_file.clear();
            final_file << oss_complete_prefix.str() << ".gif";
            int i = 0;
            while (exist_file(final_file.str().c_str())) {
                i++;
                final_file.str("");
                final_file.clear();
                final_file << oss_complete_prefix.str() << "_" << i << ".gif";
            }
            if (copy_file(filename, final_file.str())) {
                copied = true;
                break;
            }
        }
        if (copied) {
            if (remove(filename) != 0) {
                message << "Unable to delete file " << filename;
            } else {
                string ss = final_file.str();
                std::string base_filename;
#ifdef WIN32
                base_filename = ss.substr(ss.find_last_of("/\\") + 1);
#else
                base_filename = ss.substr(ss.find_last_of("/") + 1);
#endif
                message << "Created screenshot " << base_filename;
            }
        } else {
            message << "Unable to copy " << filename << " to screenshot folder";
        }
    } else {
        message << "Gif file " << filename << " not found";
    }
    custom_show_message(message.str());
}

#define JZINTV_MOUSE_COMMAND "--enable-mouse"

bool start_emulation(int index) {
    std::cout << "\n\nEmulation requested with patameters:" << std::endl;
    custom_emulation_paused = false;
    vector<string> commands;
    bool launch = true;

    bool pres = true;

    char *execBinFileName = roms_list_struct.list[index].use_tutorvision_exec ? roms_list_struct.tutorvision_exec_bin_file_name : roms_list_struct.exec_bin_file_name;
    char *gromBinFileName = roms_list_struct.list[index].use_tutorvision_grom ? roms_list_struct.tutorvision_grom_bin_file_name : roms_list_struct.grom_bin_file_name;

    if (is_memory_empty(execBinFileName)) {
        ADD_POPUP("Bios not found", "Bios exec.bin not found or found with wrong crc32");
        pres = false;
    } else {
        pres &= check_file_presence("Bios not found", execBinFileName);
    }
    if (is_memory_empty(gromBinFileName)) {
        ADD_POPUP("Bios not found", "Bios grom.bin not found or found with wrong crc32");
        pres = false;
    } else {
        pres &= check_file_presence("Bios not found", gromBinFileName);
    }

    pres &= check_file_presence("Rom not found", roms_list_struct.list[index].file_name);
    if (!pres) {
        return false;
    }

    ADD_JZINTV_COMMAND(&commands, "jzintv");
    if (roms_list_struct.list[index].use_tutorvision_gram) {
        ADD_JZINTV_COMMAND(&commands, "-G2");
    }
    ADD_JZINTV_COMMAND(&commands, "-p" << app_config_struct.roms_folder_absolute_path);
    ADD_JZINTV_COMMAND(&commands, "-e" << app_config_struct.roms_folder_absolute_path << execBinFileName);
    ADD_JZINTV_COMMAND(&commands, "-g" << app_config_struct.roms_folder_absolute_path << gromBinFileName);

    ostringstream resolutionOss;
    resolutionOss << "-z" << app_config_struct.jzintv_resolution_index;
    Log(LOG_INFO) << "Combo resolution argument: -z" << app_config_struct.jzintv_resolution_index;

    if (app_config_struct.jzintv_fullscreen) {
        ADD_JZINTV_COMMAND(&commands, "--fullscreen");
    }
    ADD_JZINTV_COMMAND(&commands, roms_list_struct.list[index].file_name);

    bool ecs_tape_auto = roms_list_struct.list[index].ecs_tape_name_auto;
    bool jlp_save_file_auto = roms_list_struct.list[index].jlp_save_file_auto;
    uint32_t crc32 = roms_list_struct.list[index].crc32;
    string gameName = roms_list_struct.list[index].game_name;
    char *data;
    ostringstream msg;

    if (index < roms_list_struct.total_roms_num && launch) {
        bool found_mouse_command = false;

        // Custom commands
        for (int i = 0; i < MAX_CUSTOM_COMMANDS; i++) {
            string cc = get_custom_command(index, i, nullptr);
            if (!cc.compare(JZINTV_MOUSE_COMMAND)) {
                found_mouse_command = true;
            }
            if (!cc.empty()) {
                if (startsWith(cc.c_str(), "-z")) {
                    resolutionOss.str("");
                    resolutionOss.clear();
                    resolutionOss << cc;
                    Log(LOG_INFO) << "Custom command - Actual resolution argument: " << cc;
                } else if (startsWith(cc.c_str(), "--ecs-tape")) {
                    if (!ecs_tape_auto) {
                        const vector<std::string> &vec = split(cc.c_str(), "=", true);
                        ostringstream oss_command;
                        ostringstream oss_folder;
                        oss_command << "--ecs-tape=";
                        oss_folder << app_config_struct.resource_folder_absolute_path << "Ecs/";
                        create_folder(oss_folder.str());
                        oss_command << oss_folder.str();
                        if (vec.size() < 2) {
                            ADD_POPUP("Command invalid", "Ecs tape command invalid:\n\n" << cc);
                            return false;
                        }
                        oss_command << vec[1];
                        msg << "\nEcs tape file: " << vec[1] << "\n";
                        Log(LOG_INFO) << "Ecs tape command:" << oss_command.str();
                        ADD_JZINTV_COMMAND(&commands, oss_command.str());
                    } else {
                        Log(LOG_INFO) << "Skipped custom Ecs tape command:" << cc;
                    }
                } else if (startsWith(cc.c_str(), "--jlp-savegame")) {
                    if (!jlp_save_file_auto) {
                        const vector<std::string> &vec = split(cc.c_str(), "=", true);
                        ostringstream oss_command;
                        ostringstream oss_folder;
                        oss_command << "--jlp-savegame=";
                        oss_folder << app_config_struct.resource_folder_absolute_path << "Jlp/";
                        create_folder(oss_folder.str());
                        oss_command << oss_folder.str();
                        if (vec.size() < 2) {
                            ADD_POPUP("Command invalid", "Jlp save command invalid:\n\n" << cc);
                            return false;
                        }
                        oss_command << vec[1];
                        msg << "\nJlp save file:" << vec[1] << "\n";
                        Log(LOG_INFO) << "Jlp save command:" << oss_command.str();
                        ADD_JZINTV_COMMAND(&commands, oss_command.str());
                    } else {
                        Log(LOG_INFO) << "Skipped custom Jlp save command:" << cc;
                    }
                } else {
                    ADD_JZINTV_COMMAND(&commands, cc);
                }
            }
        }

        std::replace(gameName.begin(), gameName.end(), '\'', '_');
        std::replace(gameName.begin(), gameName.end(), ':', '_');
        std::replace(gameName.begin(), gameName.end(), ';', '_');
        std::replace(gameName.begin(), gameName.end(), ',', '_');
        std::replace(gameName.begin(), gameName.end(), '/', '_');
        std::replace(gameName.begin(), gameName.end(), '\?', '_');
        std::replace(gameName.begin(), gameName.end(), '\\', '_');
        std::replace(gameName.begin(), gameName.end(), ' ', '_');

        if (ecs_tape_auto) {
            ostringstream oss_command;
            ostringstream oss_folder;
            oss_command << "--ecs-tape=";
            oss_folder << app_config_struct.resource_folder_absolute_path << "Ecs/";
            create_folder(oss_folder.str());
            oss_command << oss_folder.str();
            ostringstream oss_file;
            switch (crc32) {
                case 0xCE8FC699:
                    // Game Factory
                    oss_file << "ecs_tape_no.ecs";
                    break;
                case 0xEE5F1BE2:
                    // Jetsons' Ways with Words, The
                    oss_file << "ecs_tape_jwww.ecs";
                    break;
                case 0xBE4D7996:
                case 0x3207B408:
                    // Super NFL Football
                    oss_file << "ecs_tape_0w.ecs";
                    break;
                case 0xC2063C08:
                    // World Series Major League Baseball
                    oss_file << "ecs_tape_wmlb.ecs";
                    break;
                default:
                    oss_file << "ecs_tape_" << gameName.substr(0, 4) << ".ecs";
            }
            oss_command << oss_file.str();
            msg << "\nEcs tape file: " << oss_file.str() << "\n";
            Log(LOG_INFO) << "Automatic Ecs tape command:" << oss_command.str();
            ADD_JZINTV_COMMAND(&commands, oss_command.str());
        }
        if (jlp_save_file_auto) {
            ostringstream oss_command;
            ostringstream oss_folder;
            oss_command << "--jlp-savegame=";
            oss_folder << app_config_struct.resource_folder_absolute_path << "Jlp/";
            create_folder(oss_folder.str());
            oss_command << oss_folder.str();
            ostringstream oss_file;
            oss_file << "jlp_" << gameName << ".jlp";
            oss_command << oss_file.str();
            msg << "\nJlp save file: " << oss_file.str() << "\n";
            Log(LOG_INFO) << "Automatic Jlp save command:" << oss_command.str();
            ADD_JZINTV_COMMAND(&commands, oss_command.str());
        }
        if (msg.str().length() > 0) {
            string trimmed = msg.str().c_str();
            trim(trimmed);
            custom_show_message(trimmed);
        }

        char *resolution_arg = get_forced_resolution_argument();
        if (resolution_arg != nullptr) {
            Log(LOG_INFO) << "Using forced platform resolution argument: " << resolution_arg;
            resolutionOss.str("");
            resolutionOss.clear();
            resolutionOss << resolution_arg;
            free(resolution_arg);
        }

        ADD_JZINTV_COMMAND(&commands, resolutionOss.str());

        if (app_config_struct.mobile_mode && app_config_struct.mobile_show_controls && !found_mouse_command) {
            app_config_struct.consume_mouse_events_only_for_simulate_controls = true;
            ADD_JZINTV_COMMAND(&commands, JZINTV_MOUSE_COMMAND);
        }

        // Keyboard hack file
        data = get_keyboard_hack_file(index, nullptr);
        if (!is_memory_empty(data)) {
            char file_hack[FILENAME_MAX];
            sprintf(file_hack, "%sConfigs/%s", app_config_struct.resource_folder_absolute_path, data);
            if (!exist_file(file_hack)) {
                ADD_POPUP("File not found", "Cannot find specified hack file:\n\n" << file_hack << "\n\nCheck resources path or fix property file.");
                return false;
            }
            ADD_JZINTV_COMMAND(&commands, "--kbdhackfile=" << file_hack);
        }
        free(data);

        // Palette file
        data = get_palette_file(index, nullptr);
        if (!is_memory_empty(data)) {
            char file_palette[FILENAME_MAX];
            sprintf(file_palette, "%sConfigs/%s", app_config_struct.resource_folder_absolute_path, data);
            if (!exist_file(file_palette)) {
                ADD_POPUP("File not found", "Cannot find specified palette file:\n\n" << file_palette << "\n\nCheck resources path or fix property file.");
                return false;
            }
            ADD_JZINTV_COMMAND(&commands, "--gfx-palette=" << file_palette);
        }
        free(data);
    }

    if (launch) {
        std::cout << "Starting...\n" << std::endl;
        int argc;
        char **argv = convert_to_argv_argc(&commands, &argc);
        if (-10 == jzintv_entry_point(argc, argv)) {
            ADD_POPUP("Emulation error", "Emulation error. Check console window for details");
        }
        for (int i = 0; i < argc; i++) {
            free(argv[i]);
        }
        free(argv);
    }
    return true;
}

static int init_gui() {
    char title[20];
    sprintf(title, "%s %s", "jzIntvImGui", VERSION);
    if (-1 ==
        setup_window(app_config_struct.window_width, app_config_struct.window_height, app_config_struct.window_maximized, title)) {
        return -1;
    }

    // Setup style
    add_style();
    load_images(&app_config_struct);
    return 0;
}

static void reset_all_millis() {
    gui_util_str.last_key_released_millis = 0;
    gui_util_str.last_rom_index_millis = 0;
    gui_util_str.last_roms_checked_millis = 0;
}

void suspend_gui() {
    clear_general_textures();
    clear_roms_textures(&roms_list_struct);
    reset_all_millis();
    gui_util_str.roms_list_scrollable.window = nullptr;
    gui_util_str.description_scrollable.window = nullptr;
    gui_util_str.options_scrollable.window = nullptr;
    gui_util_str.options_sub_scrollable.window = nullptr;
    gui_util_str.options_desc_scrollable.window = nullptr;
    clean(BEFORE_EMULATION);
}

void resume_gui() {
    clean(AFTER_EMULATION);
    init_gui();
}

// It seems that Dear Imgui don't allow to change font, hacking..
void refresh_for_font_change() {
    custom_font = nullptr;
    app_config_struct.custom_font_loaded = false;
    font_size = app_config_struct.font_size;
    gui_util_str.reference_window_width = gui_util_str.act_window_width;
    gui_util_str.reference_window_height = gui_util_str.act_window_height;
    app_config_struct.window_width = gui_util_str.act_window_width;
    app_config_struct.window_height = gui_util_str.act_window_height;
    Log(LOG_INFO) << "Font data changed, setting reference size: " << gui_util_str.reference_window_width << "x" << gui_util_str.reference_window_height;
    suspend_gui();
    resume_gui();
}

void Push_buttons_size(bool original) {
    ImGuiStyle &style = ImGui::GetStyle();
    ImGuiContext &g = *GImGui;
    ImVec2 old_fp = style.FramePadding;
    style.FramePadding.y = ((original ? app_config_struct.buttons_size : gui_util_str.backup.app_flags.buttons_size) - g.FontSize) / 2;
    frame_padding = old_fp;
}

void Pop_buttons_size() {
    ImGuiStyle &style = ImGui::GetStyle();
    style.FramePadding = frame_padding;
}

void free_popup() {
    if (popup != nullptr) {
        delete popup;
    }
    popup = nullptr;
}

int find_roms_config_index(int index) {
    uint32_t crc32 = roms_list_struct.list[index].crc32;
    for (int i = 0; i < app_config_struct.num_valid_crc32s; i++) {
        uint32_t act_crc32 = roms_configuration[i].crc32;
        if (crc32 == act_crc32) {
            return i;
        }
    }
    return -1;
}

void update_roms_list() {
    bool hide_unavailable = app_config_struct.hide_unavailable_roms;
    free_roms_list(&roms_list_struct);
    roms_list_struct.folder = strdup(app_config_struct.roms_folder_absolute_path);
    load_roms(&app_config_struct, &roms_list_struct, roms_configuration, hide_unavailable);
}

static int check_last_key_released(int key) {
    int res = key;
    long millis = get_act_millis();
    if (gui_util_str.last_key_released == key) {
        if (millis - gui_util_str.last_key_released_millis < 20) {
            res = 0;
        }
    }
    gui_util_str.last_key_released = key;
    gui_util_str.last_key_released_millis = millis;

    return res;
}

int get_key_pressed() {
    // return values:
    // 0:None
    // 1:right
    // 2:left
    // 3:down
    // 4:up
    // 5:select
    // 6:change_image
    // 7:scroll_page_down
    // 8:scroll_page_up
    // 9:search_by_letter
    ImGuiIO &io = ImGui::GetIO();
    if (!gui_util_str.show_config_window && ImGui::IsKeyPressed(262)) {
        return 1;
    } else if (!gui_util_str.show_config_window && ImGui::IsKeyPressed(263)) {
        return 2;
    } else if (!gui_util_str.show_config_window && ImGui::IsKeyPressed(264)) {
        return 3;
    } else if (!gui_util_str.show_config_window && ImGui::IsKeyPressed(265)) {
        return 4;
    } else if (!gui_util_str.show_config_window && ImGui::IsKeyReleased(88)) {
        return check_last_key_released(5);
    } else if (ImGui::IsKeyReleased(256)) {
        return check_last_key_released(TAB_KEY_INDEX);
    } else if (ImGui::IsKeyReleased(257)) {
        return check_last_key_released(TAB_KEY_INDEX);
    } else if (!gui_util_str.show_config_window && ImGui::IsKeyPressed(SDL_SCANCODE_PAGEDOWN)) {
        return 7;
    } else if (!gui_util_str.show_config_window && ImGui::IsKeyPressed(SDL_SCANCODE_PAGEUP)) {
        return 8;
    } else if (!gui_util_str.show_config_window && io.InputQueueCharacters.Size > 0) {
        return 9;
    }
    return 0;
}

static void get_size() {
    get_window_size(&(gui_util_str.act_window_width), &(gui_util_str.act_window_height));
    gui_util_str.portrait = gui_util_str.act_window_width < gui_util_str.act_window_height;
    if (app_config_struct.mobile_mode) {
        // If we have navigation bar, two different window size are found, we need the larger
        if (gui_util_str.portrait) {
            if (gui_util_str.act_window_height != gui_util_str.mobile_major_size ||
                gui_util_str.act_window_width != gui_util_str.mobile_minor_size) {
                if (gui_util_str.mobile_major_size > 0) {
                    Log(LOG_INFO) << "Forcing computation from " << gui_util_str.act_window_width << "x" << gui_util_str.act_window_height << " to " << gui_util_str.mobile_minor_size << "x" << gui_util_str.mobile_major_size;
                }
            }
            if (gui_util_str.act_window_height > gui_util_str.mobile_major_size) {
                gui_util_str.mobile_major_size = gui_util_str.act_window_height;
            }
            if (gui_util_str.act_window_width > gui_util_str.mobile_minor_size) {
                gui_util_str.mobile_minor_size = gui_util_str.act_window_width;
            }
            gui_util_str.act_window_width = gui_util_str.mobile_minor_size;
            gui_util_str.act_window_height = gui_util_str.mobile_major_size;
        } else {
            if (gui_util_str.act_window_width != gui_util_str.mobile_major_size ||
                gui_util_str.act_window_height != gui_util_str.mobile_minor_size) {
                if (gui_util_str.mobile_major_size > 0) {
                    Log(LOG_INFO) << "Forcing computation from " << gui_util_str.act_window_width << "x" << gui_util_str.act_window_height << " to " << gui_util_str.mobile_major_size << "x" << gui_util_str.mobile_minor_size;
                }
            }
            if (gui_util_str.act_window_width > gui_util_str.mobile_major_size) {
                gui_util_str.mobile_major_size = gui_util_str.act_window_width;
            }
            if (gui_util_str.act_window_height > gui_util_str.mobile_minor_size) {
                gui_util_str.mobile_minor_size = gui_util_str.act_window_height;
            }
            gui_util_str.act_window_height = gui_util_str.mobile_minor_size;
            gui_util_str.act_window_width = gui_util_str.mobile_major_size;
        }
    }
}

void get_gap_pixels(int *l, int *r, int *t, int *b) {
    int left_gap_pixels = 0;
    int right_gap_pixels = 0;
    int top_gap_pixels = 0;
    int bottom_gap_pixels = 0;
    if (app_config_struct.mobile_mode) {
        if (gui_util_str.portrait) {
            top_gap_pixels = get_pixel_percentage(gui_util_str.act_window_height, app_config_struct.mobile_portrait_top_gap_percentage);
            bottom_gap_pixels = get_pixel_percentage(gui_util_str.act_window_height, app_config_struct.mobile_portrait_bottom_gap_percentage);
            gui_util_str.left_gap_pixels = 0;
        } else {
            left_gap_pixels = get_pixel_percentage(gui_util_str.act_window_width, app_config_struct.mobile_landscape_left_gap_percentage);
            right_gap_pixels = get_pixel_percentage(gui_util_str.act_window_width, app_config_struct.mobile_landscape_right_gap_percentage);
            gui_util_str.left_gap_pixels = left_gap_pixels;
        }
    }
    *l = left_gap_pixels;
    *r = right_gap_pixels;
    *t = top_gap_pixels;
    *b = bottom_gap_pixels;
}

extern long start_millis;
static int start_gui() {
    if (-1 == init_gui()) {
        return -1;
    }

    /////////////////////////////////////////////////////////
    ///////////// Start jzintvGui ///////////////////////////
    /////////////////////////////////////////////////////////
    memset(&roms_list_struct, 0, sizeof(roms_list_struct_t));

    // Main loop
    while (true) {
        bool refresh_for_text = false;
        bool exit;
        check_for_special_event(&exit, &app_config_struct);
        if (exit) {
            save_config_file();
            break;
        }
        get_size();
        if (gui_util_str.reference_window_width == -1) {
            // Very first iteration
            gui_util_str.reference_window_width = gui_util_str.act_window_width;
            gui_util_str.reference_window_height = gui_util_str.act_window_height;
            Log(LOG_INFO) << "Initial reference size: " << gui_util_str.reference_window_width << "x" << gui_util_str.reference_window_height;
        }
        manage_new_frame();
        if (custom_font != nullptr) {
            ImGui::PushFont(custom_font);
            app_config_struct.custom_font_loaded = true;
        }

        if (!gui_util_str.show_config_window) {
            check_for_update_list();
            manage_key_pressed_main(popup != nullptr);
            draw_main_window();
        } else {
            manage_key_pressed_config();
            if (gui_util_str.reset_backup_data) {
                reset_backup_data();
                gui_util_str.reset_backup_data = false;
            }
            if (app_config_struct.dialog_reference == nullptr) {
                show_configuration_window(&refresh_for_text);
            }
#ifndef WIN32
            else {
#ifdef __ANDROID__
                if (!is_memory_empty(app_config_struct.internal_sd_path)) {
                    ImGuiFileDialog::Instance()->addForcedBookmark("Internal SD", app_config_struct.internal_sd_path);
                }
                if (!is_memory_empty(app_config_struct.external_sd_path)) {
                   ImGuiFileDialog::Instance()->addForcedBookmark("External SD", app_config_struct.external_sd_path);
                }
#endif
                int left_gap_pixels = 0;
                int right_gap_pixels = 0;
                int top_gap_pixels = 0;
                int bottom_gap_pixels = 0;
                get_gap_pixels(&left_gap_pixels, &right_gap_pixels, &top_gap_pixels, &bottom_gap_pixels);

                ImVec2 window_pos;
                window_pos.x = left_gap_pixels;
                window_pos.y = top_gap_pixels;
                ImGui::SetNextWindowPos(window_pos);

                ImVec2 window_size_vec_full;
                window_size_vec_full.x = gui_util_str.act_window_width - left_gap_pixels - right_gap_pixels;
                window_size_vec_full.y = gui_util_str.act_window_height - top_gap_pixels - bottom_gap_pixels;
                ImGui::SetNextWindowSize(window_size_vec_full, ImGuiCond_Always);
                Push_buttons_size(true);
                ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                if (ImGuiFileDialog::Instance()->Display("ChooseDlgKey", ImGuiWindowFlags_NoResize)) {
                    // action if OK
                    if (ImGuiFileDialog::Instance()->IsOk()) {
                        std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                        std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
                        strcpy(app_config_struct.dialog_reference, filePathName.c_str());
                        // action
                    }
                    // close
                    ImGuiFileDialog::Instance()->Close();
                    app_config_struct.dialog_reference = nullptr;
                }
                ImGui::PopStyleColor();
                Pop_buttons_size();
            }
#endif
        }
        /////// Demo Window ////////////////////////
        if (gui_util_str.show_demo_window) {
            ImGui::SetNextWindowPos(ImVec2(650, 20),
                                    ImGuiCond_FirstUseEver); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
            ImGui::ShowDemoWindow(&gui_util_str.show_demo_window);
        } else {
            if (gui_util_str.flip_demo_window) {
                gui_util_str.show_demo_window = true;
                gui_util_str.flip_demo_window = false;
            }
        }

        if (app_config_struct.custom_font_loaded) {
            ImGui::PopFont();
        }

        if (!app_config_struct.ending_game) {
            // Rendering
            render();
            on_render();
        }

        app_config_struct.ending_game = false;

        if (refresh_for_text) {
            refresh_for_font_change();
            SDL_StopTextInput();
            start_millis = 0;
            on_font_change();
        }
        refresh_for_text = false;
    }

    // Cleanup
    clean(EXITING);
    return 0;
}

void set_tab_index(int index) {
    ImGuiContext &g = *GImGui;
    ImGuiTabBar *tab_bar = g.CurrentTabBar;
    ImGuiTabItem *tab_selected = &tab_bar->Tabs[index];
    tab_bar->SelectedTabId = tab_selected->ID;
}

int get_tab_index() {
    ImGuiContext &g = *GImGui;
    ImGuiTabBar *tab_bar = g.CurrentTabBar;
    for (int i = 0; i < tab_bar->Tabs.size(); i++) {
        ImGuiTabItem *act_tab = &tab_bar->Tabs[i];
        if (tab_bar->SelectedTabId == act_tab->ID) {
            return i;
        }
    }
    return 0;
}

void free_all() {
    free_roms_list(&roms_list_struct);
    free_popup();
    free(app_config_struct.roms_folder_ini);
    free(app_config_struct.root_folder_for_configuration);
    free(app_config_struct.resource_folder_absolute_path);
    free(app_config_struct.roms_folder_absolute_path);
    free(app_config_struct.font_filename);
    free(app_config_struct.keyboard_hack_file);
    free(app_config_struct.palette_file);
    if (app_config_struct.internal_sd_path != nullptr) {
        free(app_config_struct.internal_sd_path);
    }
    if (app_config_struct.external_sd_path != nullptr) {
        free(app_config_struct.external_sd_path);
    }

    free_message(INFOS_INDEX);
    free_message(WARNINGS_INDEX);
    free_message(ERRORS_INDEX);
    free_message(CONFIG_WARNINGS_INDEX);

    if (app_config_struct.custom_commands != nullptr) {
        vector<string> &vec = *(app_config_struct.custom_commands);
        vec.clear();
        delete app_config_struct.custom_commands;
    }
    app_config_struct.custom_commands = nullptr;

    for (int i = 0; i < app_config_struct.num_valid_crc32s; i++) {
        free_rom_config_struct(&roms_configuration[i]);
    }
    if (roms_configuration.size() > 0) {
        roms_configuration.clear();
    }

    clear_general_textures();
    clear_all_default_game_controls();
//    check_textures_freed();
    clear_events();
}

int main(int argc, char **argv) {
    init_platform(argc, argv);
    init_controls();
    init_events();
    load_configuration();
    if (app_config_struct.mobile_mode) {
        normalize_default_controls();
        normalize_events();
    }

    // Start all
    start_gui();
    free_all();
    return 0;
}

int get_pixel_percentage(int whole, double perc) {
    float whole_float = (float) whole;
    float perc_float = (float) perc;
    float res_float = (whole_float / (float) 100) * (perc_float);
    return (int) round(res_float);
}
