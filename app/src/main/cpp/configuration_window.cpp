#include "main.h"

#define TAB_CONFIGURATION_ID_STRING "##configuration_tabs"

extern void request_for_scroll(int mode);

extern gui_util_struct_t gui_util_str;

bool old_held[5] = {false, false, false, false, false};
bool new_held[5] = {false, false, false, false, false};

static bool is_in_folder(string full_file_path, const char *folder) {
    bool res = false;
    if (full_file_path.find(folder) != std::string::npos) {
        res = true;
    }
#ifdef WIN32
    else {
        string full_file_path_uppercase = full_file_path.c_str();
        std::transform(full_file_path_uppercase.begin(), full_file_path_uppercase.end(), full_file_path_uppercase.begin(), ::toupper);
        string folder_uppercase = folder;
        std::transform(folder_uppercase.begin(), folder_uppercase.end(), folder_uppercase.begin(), ::toupper);
        if (full_file_path_uppercase.find(folder_uppercase) != std::string::npos) {
            res = true;
        }
    }
#endif
    return res;
}

static bool update_on_change(char **backup_ref,
                             char **final_ref,
                             char **final_absolute_ref,
                             const char *root_folder,
                             string popup_message_on_fail,
                             bool is_directory,
                             bool copy = false) {
    bool res = false;

    // Trim and manage new value
    string trimmed = *backup_ref;
    trim(trimmed);
    free(*backup_ref);
    if (trimmed.empty()) {
        if (is_directory) {
            *backup_ref = strdup(".");
        } else {
            *backup_ref = strdup("");
        }
    } else {
        *backup_ref = strdup(trimmed.c_str());
    }

    if (!(*final_ref) || strcmp(*backup_ref, *final_ref)) {
        res = true;
        bool accepted = false;
        string backup_absolute_ref;

        if (root_folder != NULL) {

            if (is_memory_empty(*backup_ref)) {
                // Inutile cercare absolute path, l'utente ha svuotato il campo
                if (*final_ref != NULL) {
                    free(*final_ref);
                }
                *final_ref = strdup("");
                return true;
            }

            int result;
            char *tmp2 = get_absolute_path(root_folder, *backup_ref, true, &result);
            if (!is_directory) {
                // Remove final slash
                tmp2[strlen(tmp2) - 1] = 0;
            }
            backup_absolute_ref = tmp2;
            free(tmp2);

            if (result != -1) {
                accepted = is_directory ? exist_folder(backup_absolute_ref) : exist_file(backup_absolute_ref.c_str());
            }
        } else {
            accepted = true;
        }

        if (accepted) {
            if (copy) {
                if (!exist_folder(root_folder)) {
                    ADD_POPUP("Destination folder does not exist", "Destination folder does not exist:" << root_folder);
                    return false;
                }
                char *_filename = get_file_name(backup_absolute_ref);
                string filename = _filename;
                free(_filename);
                ostringstream original_file;
                original_file << root_folder << "/";
                if (*final_ref != NULL) {
                    original_file << *final_ref;
                }
                ostringstream final_file;
                final_file << root_folder << "/" << filename;
                if (backup_absolute_ref.compare(original_file.str())) {
                    int i = 0;
                    bool filename_changed = false;
                    if (is_in_folder(backup_absolute_ref, root_folder)) {
                        if (*final_ref != NULL) {
                            free(*final_ref);
                        }
                        *final_ref = get_file_name(final_file.str().c_str());
                    } else {
                        while (exist_file(final_file.str().c_str())) {
                            i++;
                            final_file.str("");
                            final_file.clear();
                            std::string::size_type const p(filename.find_last_of('.'));
                            std::string file_without_extension;
                            std::string file_extension;
                            if (p != std::string::npos) {
                                file_without_extension = filename.substr(0, p);
                                file_extension = filename.substr(p);
                            } else {
                                file_without_extension = filename;
                            }

                            final_file << root_folder << "/" << file_without_extension << "_" << i << file_extension;
                            filename_changed = true;
                        }
                        if (copy_file(backup_absolute_ref, final_file.str())) {
                            if (*final_ref != NULL) {
                                free(*final_ref);
                            }
                            *final_ref = get_file_name(final_file.str().c_str());
                            ADD_POPUP("File copied", "\n\nFile:\n\n" << backup_absolute_ref << "\n\nhas been copied to:\n\n" << final_file.str());
                        } else {
                            ADD_POPUP("Failed to copy file to destination folder", "Failed to copy file to destination folder:\n\n" << root_folder);
                        }

                        if (filename_changed) {
                            ADD_POPUP("Filename changed", "(File " << filename << " already exists, not overwritten)");
                        }
                    }
                }
            } else {
                if (*final_ref != NULL) {
                    free(*final_ref);
                }
                *final_ref = strdup(*backup_ref);
                if (final_absolute_ref != NULL && *final_absolute_ref != NULL) {
                    free(*final_absolute_ref);
                    *final_absolute_ref = strdup(backup_absolute_ref.c_str());
                }
            }
        } else {
            ADD_POPUP(popup_message_on_fail.c_str(), popup_message_on_fail.c_str());
        }
    }
    return res;
}

static bool clickableTextUrl(const char *label, const char *url, ImVec4 vColor, ImVec4 vHoveredColor, ImVec4 vClickColor, bool *old_held, bool *new_held) {
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;
    ImGuiContext &g = *GImGui;
    const ImGuiStyle &style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = ImGui::CalcItemSize(ImVec2(0.0f, 0.0f), label_size.x + style.FramePadding.x * 1.0f, label_size.y);

    ImVec2 size2;
    size2.x = pos.x + size.x;
    size2.y = pos.y + size.y;
    const ImRect bb(pos, size2);

    ImGui::ItemSize(bb, 0.0f);
    if (!ImGui::ItemAdd(bb, id))
        return false;
    ImGuiButtonFlags flags = 0;
    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);
    //ImGuiMouseCursor_Hand
    if (held || (g.HoveredId == id && g.HoveredIdPreviousFrame == id))
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    // Render
    ImGui::RenderNavHighlight(bb, id);
    ImVec4 col = (hovered && held) ? vClickColor : hovered ? vHoveredColor : vColor;
    ImVec2 p0 = bb.Min;
    ImVec2 p1 = bb.Max;
#if defined(WIN32) || defined(ANDROID)
    // Cannot open url from here web browser on Linux..why? :-(
    if ((*old_held) || (hovered && held)) {
        p0.x += 1;
        p0.y += 1;
        p1.x += 1;
        p1.y += 1;
        if (hovered && held) {
            *new_held = true;
        } else {
            *new_held = false;
        }
    }

    window->DrawList->AddLine(ImVec2(p0.x, p1.y), ImVec2(p1.x - style.FramePadding.x, p1.y), ImGui::GetColorU32(col));
#endif
    ImGui::PushStyleColor(ImGuiCol_Text, col);
    ImGui::RenderTextClipped(p0, p1, label, NULL, &label_size, style.ButtonTextAlign, &bb);
    ImGui::PopStyleColor(1);
#if defined(WIN32) || defined(ANDROID)
    // Cannot open url from here web browser on Linux..why? :-(
    if (pressed) {
        openUrl(url);
    }
#endif
    return pressed;
}

static void add_section_info_thanks(string title, string description, string url, bool border, int index) {
    ImVec4 col = {0, 120, 20, 140};

    if (border) {
        ImGui::Separator();
        ImGui::Text("%s", "\n");
    }

    ImVec2 size = ImGui::CalcTextSize(title.c_str(), NULL, true);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    ImGui::PushStyleColor(ImGuiCol_Text, green_col);
    ImGui::TextWrapped("%s", title.c_str());
    ImGui::PopStyleColor();
    ImGui::TextWrapped("%s\n", description.c_str());
    if (!url.empty()) {
        ImVec2 siz = ImGui::GetCurrentWindowRead()->Size;
        const vector<std::string> &tmp = get_vector_string(url.c_str(), siz.x - 2 * gui_util_str.border.x);
        bool found = false;
        bool res = false;
        for (int i = 0; i < tmp.size(); i++) {
            res |= clickableTextUrl(tmp[i].c_str(), url.c_str(), col, col, col, &(old_held[index]), &(new_held[index]));
            if (new_held[index]) {
                found = true;
                old_held[index] = true;
            }
        }
        if (!found || res) {
            old_held[index] = false;
        }
    }
}

static void show_info_and_thanks() {
    string jzIntvImGuiTitle = "jzIntvImGui";
    jzIntvImGuiTitle.append(" (V. ");
    jzIntvImGuiTitle.append(VERSION);
    jzIntvImGuiTitle.append(") - by jenergy (jenergy@tiscali.it)");
    string jzIntvImGuiDesc = "This is a cross-platform frontend for jzIntv, based on Dear Imgui. \n\n"
                             "With jzIntvImGui you can easily manage and play your collection of Intellivision games. You can configure global keyboard hack file, palette file and up to 10 jzIntv commands, but for every game you can override them in order to have full customization. Obviously for every game, you can also assign images (screenshots and box), and its description.\n\n"
                             "Games are managed through a local database, if a new or unknown game is found in roms folder, it can be easily added and configured in database.\n\n"
                             "You can launch games using embedded jzIntv, which is a little modified version of the official one, allowing the use of configurable on-screen controls (useful for mobile devices), configurable globally or on every single game. \n"
                             // "On desktop systems, you can also configure interface in order to launch any external version of jzIntv.\n"
                             "\nIt actually works on Windows, Linux, Android and Nintendo Switch.";
    add_section_info_thanks(jzIntvImGuiTitle, jzIntvImGuiDesc, "", false, -1);

    char buffer[2048];
    sprintf(buffer, "Thanks:");
    ImGui::Separator();
    ImGui::TextWrapped("%s\n\n", buffer);

    string jzintvTitle = "jzIntv";
    jzintvTitle.append(" (Embedded version: ");
    jzintvTitle.append(EMBEDDED_JZINTV_VERSION);
    jzintvTitle.append(")");
    string jzintvDesc = "Terrific emulator of Intellivision written by Joe Zbiciak!";
    add_section_info_thanks(jzintvTitle, jzintvDesc, JZINTV_URL, false, 0);
    ImGui::TextWrapped("%s\n\n", "");

    string dearImguiTitle = "Dear ImGui";
    dearImguiTitle.append(" (V. ");
    dearImguiTitle.append(DEAR_IMGUI_VERSION);
    dearImguiTitle.append(")");
    string dearImguiDesc = "A bloat-free graphical user interface library for C++.";
    add_section_info_thanks(dearImguiTitle, dearImguiDesc, DEAR_IMGUI_URL, false, 1);
    if (ImGui::Button(SHOW_DEAR_IMGUI_DEMO_TEXT)) {
        if (!gui_util_str.show_demo_window) {
            gui_util_str.show_demo_window = true;
        } else {
            gui_util_str.flip_demo_window = true;
            gui_util_str.show_demo_window = false;
        }
    }
    ImGui::TextWrapped("%s\n\n", "");

    string libsdlTitle = "libSDL";
    libsdlTitle.append(" (V. ");
    libsdlTitle.append(LIBSDL2_VERSION);
    libsdlTitle.append(")");
    string libsdlDesc = "A cross-platform development library designed to provide low level access to audio, keyboard, mouse, joystick, and graphics hardware via OpenGL and Direct3D.";
    add_section_info_thanks(libsdlTitle, libsdlDesc, LIBSDL2_URL, false, 2);
    ImGui::TextWrapped("%s\n", "");

    add_section_info_thanks("Zendocon", "For his support, suggestions and tests for android port, images and much more!", ZENDOCON_URL, false, 3);
    ImGui::TextWrapped("%s\n", "");

    add_section_info_thanks("larryvgs", "For his support, suggestions and tests for Windows / Amazon fire sticks ports!", LARRYVGS_URL, false, 4);
    ImGui::TextWrapped("%s\n", "");

    add_section_info_thanks("Emanuele Zangara", "For creation of terrific configurations buttons and disc button too\nemanuele.zangara@yahoo.it", "",false, 5);

//    show_messages(&roms_list_struct);
}

static void reset_jzintv_backup_data(backup_config_struct_t *backup_struct,
                                     char *keyboard_hack_file,
                                     char *palette_file,
                                     vector<string> *custom_commands) {

    // Keyboard hack file
    backup_struct->app_flags.keyboard_hack_file = (char *) malloc(sizeof(char) * (1 + FILENAME_MAX));
    if (keyboard_hack_file != NULL) {
        strcpy(backup_struct->app_flags.keyboard_hack_file, keyboard_hack_file);
    } else {
        backup_struct->app_flags.keyboard_hack_file[0] = 0;
    }

    // Palette file
    backup_struct->app_flags.palette_file = (char *) malloc(sizeof(char) * (1 + FILENAME_MAX));
    if (palette_file != NULL) {
        strcpy(backup_struct->app_flags.palette_file, palette_file);
    } else {
        backup_struct->app_flags.palette_file[0] = 0;
    }

    // Custom commands
    backup_struct->custom_commands_array_data = (char **) malloc(sizeof(char *) * MAX_CUSTOM_COMMANDS);
    backup_struct->custom_commands_array_data_backup = (char **) malloc(sizeof(char *) * MAX_CUSTOM_COMMANDS);
    for (int i = 0; i < MAX_CUSTOM_COMMANDS; i++) {
        backup_struct->custom_commands_array_data[i] = (char *) malloc(sizeof(char) * (1 + FILENAME_MAX));
        backup_struct->custom_commands_array_data_backup[i] = (char *) malloc(sizeof(char) * (1 + FILENAME_MAX));
        if (i < custom_commands->size()) {
            strcpy(backup_struct->custom_commands_array_data[i], custom_commands->at(i).c_str());
            strcpy(backup_struct->custom_commands_array_data_backup[i], custom_commands->at(i).c_str());
        } else {
            backup_struct->custom_commands_array_data[i][0] = 0;
            backup_struct->custom_commands_array_data_backup[i][0] = 0;
        }
    }
}

void reset_backup_data_for_selected_game() {
    reset_jzintv_backup_data(&gui_util_str.backup_game,
                             selected_rom->keyboard_hack_file,
                             selected_rom->palette_file,
                             selected_rom->custom_commands);

    gui_util_str.backup_game.hack_file_use_global = is_memory_empty(selected_rom->keyboard_hack_file);
    gui_util_str.backup_game.hack_file_was_global = gui_util_str.backup_game.hack_file_use_global;
    if (!strcmp(selected_rom->keyboard_hack_file, DISABLE_GLOBAL_VALUE)) {
        gui_util_str.backup_game.app_flags.keyboard_hack_file[0] = 0;
    }

    gui_util_str.backup_game.palette_file_use_global = is_memory_empty(selected_rom->palette_file);
    gui_util_str.backup_game.palette_file_was_global = gui_util_str.backup_game.palette_file_use_global;
    if (!strcmp(selected_rom->palette_file, DISABLE_GLOBAL_VALUE)) {
        gui_util_str.backup_game.app_flags.palette_file[0] = 0;
    }

    for (int i = 0; i < MAX_CUSTOM_COMMANDS; i++) {
        gui_util_str.backup_game.custom_command_use_global[i] = is_memory_empty(gui_util_str.backup_game.custom_commands_array_data[i]);
        gui_util_str.backup_game.custom_command_was_global[i] = gui_util_str.backup_game.custom_command_use_global[i];
        if (!strcmp(gui_util_str.backup_game.custom_commands_array_data[i], DISABLE_GLOBAL_VALUE)) {
            gui_util_str.backup_game.custom_commands_array_data_backup[i][0] = 0;
        }
    }

    // Game Name
    gui_util_str.backup_game.rom_flags.game_name = (char *) malloc(sizeof(char) * (1 + FILENAME_MAX));
    if (selected_rom->game_name != NULL) {
        strcpy(gui_util_str.backup_game.rom_flags.game_name, selected_rom->game_name);
    } else {
        gui_util_str.backup_game.rom_flags.game_name[0] = 0;
    }

    // Image file name
    gui_util_str.backup_game.rom_flags.image_file_name = (char *) malloc(sizeof(char) * (1 + FILENAME_MAX));
    if (selected_rom->image_file_name != NULL) {
        strcpy(gui_util_str.backup_game.rom_flags.image_file_name, selected_rom->image_file_name);
    } else {
        gui_util_str.backup_game.rom_flags.image_file_name[0] = 0;
    }

    // Box file name
    gui_util_str.backup_game.rom_flags.box_file_name = (char *) malloc(sizeof(char) * (1 + FILENAME_MAX));
    if (selected_rom->box_file_name != NULL) {
        strcpy(gui_util_str.backup_game.rom_flags.box_file_name, selected_rom->box_file_name);
    } else {
        gui_util_str.backup_game.rom_flags.box_file_name[0] = 0;
    }

    // Tutorvision
    gui_util_str.backup_game.rom_flags.use_tutorvision_exec = selected_rom->use_tutorvision_exec;
    gui_util_str.backup_game.rom_flags.use_tutorvision_grom = selected_rom->use_tutorvision_grom;
    gui_util_str.backup_game.rom_flags.use_tutorvision_gram = selected_rom->use_tutorvision_gram;

    // Jlp
    gui_util_str.backup_game.rom_flags.jlp_save_file_auto = selected_rom->jlp_save_file_auto;

    // Ecs
    gui_util_str.backup_game.rom_flags.ecs_tape_name_auto = selected_rom->ecs_tape_name_auto;

    // Description
    gui_util_str.backup_game.rom_flags.description = (char *) malloc(sizeof(char) * (1 + DESCRIPTION_MAX_LENGTH));
    if (selected_rom->description != NULL) {
        strcpy(gui_util_str.backup_game.rom_flags.description, selected_rom->description);
    } else {
        gui_util_str.backup_game.rom_flags.description[0] = 0;
    }
}

static void draw_section(string section, ImVec4 border_col, int zindex, bool general_section, int *pos_y);

static void show_game_data() {
    char buffer[2048];
    int pos_y;

    bool gameNotFound = selected_rom->available_status == ROM_AVAILABLE_STATUS_NOT_FOUND;
    if (gameNotFound) {
        selected_rom->file_name = strdup("<Game not found>");
    }

    draw_section(INTERFACE_GAME_INFO_SECTION_NAME, darkcian_col, 0, false, &pos_y);

    if (selected_rom->available_status == ROM_AVAILABLE_STATUS_UNKNOWN) {
        sprintf(buffer, "Game not in database");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", buffer);
        Push_buttons_size();
        if (ImGui::Button("Add to database")) {
            uint32_t act_crc_32 = selected_rom->crc32;
            rom_config_struct_t *act_rom_config = add_new_game_by_crc32(act_crc_32);
            string filename = selected_rom->file_name;
            string game_name = filename.substr(0, filename.length() - 4);
            free(act_rom_config->game_name);
            act_rom_config->game_name = (char *) malloc(sizeof(char) * (1 + FILENAME_MAX));
            strcpy(act_rom_config->game_name, game_name.c_str());
            sort_config_by_crc_32(&roms_configuration);
            app_config_struct.num_valid_crc32s++;
            reset_backup_data_for_selected_game();
            // Updating name and status
            free(gui_util_str.backup_game.rom_flags.game_name);
            gui_util_str.backup_game.rom_flags.game_name = (char *) malloc(sizeof(char) * (1 + FILENAME_MAX));
            strcpy(gui_util_str.backup_game.rom_flags.game_name, game_name.c_str());
            selected_rom->available_status = ROM_AVAILABLE_STATUS_FOUND;
            request_for_scroll(RELOAD_ROMS_AND_RECOMPILE_MODE);
        }
        Pop_buttons_size();
    }

    if (selected_rom->available_status != ROM_AVAILABLE_STATUS_UNKNOWN) {
        draw_section(JZINTV_OPTIONS_SECTION_NAME, darkcian_col, 0, false, &pos_y);
    }

    if (gameNotFound) {
        free(selected_rom->file_name);
        selected_rom->file_name = NULL;
    }
}

#define ALIGNMENT_STRING SHOW_CONFIGURATION_CONTROLS_TEXT

bool init_multiline = true;
#define CHECKBOX_TYPE 0
#define SCALAR_TYPE 1
#define COMBO_TYPE 2
#define MULTI_LINE_TYPE 3
#define SINGLE_LINE_TYPE 4
#define DOUBLE_TYPE 5

#define STYLE_COMBO_BEHAVIOUR 0
#define JZINTV_RESOLUTION_COMBO_BEHAVIOUR 1
ImGuiWindow *lastChild = NULL;

static bool draw_component(int component_type,
                           void *refer,
                           string message,
                           int start_x,
                           int end_x,
                           int *pos_y,
                           int min = 0,
                           int max = 0,
                           const char *restore_reference = NULL,
                           bool show_choose = false,
                           bool show_clear = false,
                           bool show_reset = false,
                           bool directory = false,
                           bool *use_global = NULL,
                           bool was_global = false,
                           bool force_disabled = false,
                           char **filter_descriptions = NULL,
                           char **filter_extensions = NULL,
                           int numFilters = 0,
                           const char *root_folder = NULL,
                           const char *combo_items = NULL,
                           uint64_t *combo_value = NULL,
                           int combo_behaviour = -1,
                           int size = -1
) {

    const ImU64 u64_zero = 0, u64_one = 1, u64_fifty = 50, u64_min = min, u64_max = max;
    const double f64_zero = 0., f64_one = 1., f64_lo_a = -1000000000000000.0, f64_hi_a = +1000000000000000.0;
    bool res = false;
    float size_x = (float) (end_x - start_x);
    float text_size_y = 0;
    float act_text_size_y;
    int next_width;
    int combo_idx;
    int *pInt;
    double *pDouble;
    char *reference = (char *) (refer);
    string label_string = "##";
    label_string.append(message);
    if (use_global) {
        label_string.append("_override");
    }
    const char *label = label_string.c_str();

    ImVec2 offs = ImGui::GetCursorPos();
    offs.x = start_x + gui_util_str.border.x;
    ImGui::SetCursorPos(offs);

    Push_buttons_size();

    vector<std::string> tmp;
    ImVec2 text_size;
    if (gui_util_str.portrait) {
        tmp = get_vector_string(message.c_str(), size_x - 3 * gui_util_str.border.x);
        for (int i = 0; i < tmp.size(); i++) {
            act_text_size_y = ImGui::CalcTextSize(tmp[i].c_str(), NULL, true).y;
            if (i == 0) {
                text_size_y += act_text_size_y;
            } else {
                text_size_y += 1.25 * act_text_size_y;
            }
        }
    } else {
        text_size = ImGui::CalcTextSize(message.c_str(), NULL, true);
        text_size_y = text_size.y;
    }

    ImVec2 button_ci_size = ImGui::CalcTextSize(CHOOSE_ITEM_TEXT, NULL, true);
    ImVec2 button_ct_size = ImGui::CalcTextSize(CLEAR_TEXT, NULL, true);
    ImVec2 button_rt_size = ImGui::CalcTextSize(RESET_TEXT, NULL, true);
    ImVec2 buttons_size = {0, 0};
    buttons_size.x += button_ci_size.x;
    buttons_size.y += ImGui::GetFrameHeight();
    buttons_size.x += button_ct_size.x;
    buttons_size.x += button_rt_size.x;
    buttons_size.x += 6 * gui_util_str.border.x;

    bool use_global_is_present_and_false = use_global != NULL && !(*use_global);
    bool use_global_is_present_and_true = use_global != NULL && *use_global;
    bool commands_visible = use_global_is_present_and_false || !use_global;

    int mmax = ImGui::GetFrameHeight();
    float additional_total_size_portrait = 0;
    float additional_total_size_landscape = 0;

    switch (component_type) {
        case MULTI_LINE_TYPE: {
            ImGuiContext &g = *GImGui;
            ImGuiStyle &style = ImGui::GetStyle();
            float frame_size_y = g.FontSize * 12.0f + style.FramePadding.y * 2.0f;
            mmax = frame_size_y;
            additional_total_size_landscape = gui_util_str.border.y + ImAbs(style.FramePadding.y);
            additional_total_size_portrait = additional_total_size_landscape + buttons_size.y;
        }
            break;
        default: {
            if (use_global_is_present_and_false) {
                additional_total_size_landscape = app_config_struct.buttons_size + gui_util_str.border.y;
            }
            additional_total_size_portrait = additional_total_size_landscape;
            if (commands_visible && (show_choose || show_clear || show_reset)) {
                additional_total_size_portrait += buttons_size.y;
                additional_total_size_portrait += gui_util_str.border.y;
            }
        }
    }

    if (mmax < app_config_struct.buttons_size) {
        mmax = app_config_struct.buttons_size;
    }

    float total_size_y = gui_util_str.border.y;
    if (gui_util_str.portrait) {
        total_size_y += text_size_y;
        total_size_y += mmax;
        total_size_y += gui_util_str.border.y;
        total_size_y += additional_total_size_portrait;
    } else {
        total_size_y += ImMax((int) text_size_y, (int) mmax);
        total_size_y += additional_total_size_landscape;
    }

    ImGui::BeginChild(message.c_str(), ImVec2{size_x - 3 * gui_util_str.border.x, total_size_y}, false);
    lastChild = ImGui::GetCurrentWindow();
    int new_pos_y = ImGui::GetCurrentWindowRead()->Pos.y + ImGui::GetCurrentWindowRead()->Size.y;
    *pos_y = new_pos_y;

    ImGui::PushStyleColor(ImGuiCol_Text, green_col);
    ImVec2 message_pos = ImGui::GetCursorPos();
    message_pos.y += (ImGui::GetCurrentWindowRead()->DC.LastItemRect.Max.y - ImGui::GetCurrentWindowRead()->DC.LastItemRect.Min.y) / 2;
    ImGui::SetCursorPos(message_pos);
    int diff_y = (app_config_struct.font_size - app_config_struct.buttons_size);
    ImVec2 alignment_size;

    if (gui_util_str.portrait) {
        for (int i = 0; i < tmp.size(); i++) {
            ImVec2 act_pos = ImGui::GetCursorPos();
            act_pos.x = message_pos.x;
            ImGui::SetCursorPos(act_pos);
            ImGui::Text("%s", tmp[i].c_str());
        }
    } else {
        alignment_size = ImGui::CalcTextSize(ALIGNMENT_STRING, NULL, true);
        message_pos.x += alignment_size.x + gui_util_str.border.x;
        message_pos.y = 0;
        if (diff_y > 0) {
            message_pos.y += ((float) diff_y / 1.5);
        }
        ImGui::SetCursorPos(message_pos);
        message_pos.y = 0;
    }
    ImGui::PopStyleColor();

    if (use_global_is_present_and_true) {
        reference[0] = 0;
    }

    if (force_disabled) {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }

    if (use_global != NULL) {
        ImVec2 cb = ImGui::GetCursorPos();
        ImGui::Checkbox("Use global", use_global);
        ImVec2 cb2 = ImGui::GetCursorPos();
        cb2.x = cb.x;
        ImGui::SetCursorPos(cb2);
    }

    if (commands_visible) {
        switch (component_type) {
            case CHECKBOX_TYPE:
                if (gui_util_str.portrait) {
                    ImGui::PushItemWidth(size_x - 4 * gui_util_str.border.x);
                } else {
                    ImGui::PushItemWidth(size_x - alignment_size.x - buttons_size.x - 5 * gui_util_str.border.x);
                }
                res = ImGui::Checkbox(label, static_cast<bool *>(refer));
                break;
            case SCALAR_TYPE:
                ImGui::PushItemWidth(gui_util_str.border.x + 2 * app_config_struct.buttons_size + ImGui::CalcTextSize("9999", NULL, true).x);
                pInt = static_cast<int *>(refer);
                if (ImGui::InputScalar(label, ImGuiDataType_U64, pInt, &u64_one)) {
                    if (*pInt < min) {
                        *pInt = min;
                    } else if (*pInt > max) {
                        *pInt = max;
                    }
                }
                break;
            case DOUBLE_TYPE:
                ImGui::PushItemWidth(gui_util_str.border.x + 2 * app_config_struct.buttons_size + ImGui::CalcTextSize("999999999", NULL, true).x);
                pDouble = static_cast<double *>(refer);
                if (ImGui::InputScalar(label, ImGuiDataType_Double, pDouble, &f64_one)) {
                    if (*pDouble < min) {
                        *pDouble = min;
                    } else if (*pDouble > max) {
                        *pDouble = max;
                    }
                }
                break;
            case COMBO_TYPE:
                if (gui_util_str.portrait) {
                    next_width = (size_x - 4 * gui_util_str.border.x);
                } else {
                    next_width = (size_x - alignment_size.x - buttons_size.x - 5 * gui_util_str.border.x);
                    int next_max_width = gui_util_str.act_window_width / 2;
                    if (next_width > next_max_width) {
                        next_width = next_max_width;
                    }
                }

                ImGui::PushItemWidth(next_width);

                combo_idx = *combo_value;
                if (ImGui::Combo(label, &combo_idx, combo_items)) {
                    *combo_value = combo_idx;
                    switch (combo_behaviour) {
                        case STYLE_COMBO_BEHAVIOUR:
                            apply_style(*combo_value);
                            break;
                        case JZINTV_RESOLUTION_COMBO_BEHAVIOUR:
                            break;
                    }
                }
                break;
            case MULTI_LINE_TYPE:
                if (gui_util_str.portrait) {
                    next_width = (size_x - 4 * gui_util_str.border.x);
                } else {
                    next_width = (size_x - alignment_size.x - buttons_size.x - 5 * gui_util_str.border.x);
                    int next_max_width = gui_util_str.act_window_width / 2;
                    if (next_width > next_max_width) {
                        next_width = next_max_width;
                    }
                }

                ImGui::PushItemWidth(next_width);
                ImGui::InputTextMultiline(label, reference, size, ImVec2(next_width, 0), 0, NULL, NULL, true, &init_multiline);
                ImGui::PopItemWidth();
                break;
            case SINGLE_LINE_TYPE:
                if (gui_util_str.portrait) {
                    next_width = (size_x - 4 * gui_util_str.border.x);
                } else {
                    next_width = (size_x - alignment_size.x - buttons_size.x - 5 * gui_util_str.border.x);
                    int next_max_width = gui_util_str.act_window_width / 2;
                    if (next_width > next_max_width) {
                        next_width = next_max_width;
                    }
                }

                ImGui::PushItemWidth(next_width);
                ImGui::InputText(label, reference, size);
                ImGui::PopItemWidth();
                break;
            default:
                break;
        }
    }

    if (!gui_util_str.portrait && (show_choose || show_clear || show_reset)) {
        ImGui::SameLine();
    }
    if (commands_visible) {
        if (show_choose) {
            if (ImGui::Button(CHOOSE_ITEM_TEXT)) {
                int result;
                string ref = directory ? reference : "";
                trim(ref);
                if (ref.empty()) {
                    ref.append(".");
                }
                char *canonical_path = get_absolute_path(root_folder, ref.c_str(), true, &result);
                char *canonical_path_formatted = normalize_path(canonical_path, false);
                free(canonical_path);
#ifdef WIN32
                char *tmp = replaceWord(canonical_path_formatted, "/", "\\");
                free(canonical_path_formatted);
                canonical_path_formatted = tmp;
#endif
                string ref2 = reference;
                trim(ref2);
                string selected_item = browse_item(canonical_path_formatted, ref2.c_str(), directory, filter_descriptions, filter_extensions, numFilters);
#ifndef WIN32
                app_config_struct.dialog_reference = reference;
#endif
                free(canonical_path_formatted);
                trim(selected_item);
                if (!selected_item.empty()) {
                    strcpy(reference, selected_item.c_str());
                }
            }
        }
        if (show_clear) {
            if (show_choose) {
                ImGui::SameLine();
            }
            if (ImGui::Button(CLEAR_TEXT)) {
                reference[0] = 0;
                if (component_type == MULTI_LINE_TYPE) {
                    init_multiline = true;
                }
            }
        }
    }
    if (show_reset) {
        if (show_clear || show_choose) {
            ImGui::SameLine();
        }
        if (use_global == NULL || (was_global != *use_global) || !*use_global) {
            if (ImGui::Button(RESET_TEXT)) {
                if (use_global != NULL) {
                    *use_global = was_global;
                }

                if (strcmp(restore_reference == NULL ? "" : restore_reference, DISABLE_GLOBAL_VALUE)) {
                    if (component_type == MULTI_LINE_TYPE) {
                        init_multiline = true;
                    }
                    // Sigh..
                    if (restore_reference != NULL) {
                        strcpy(reference, restore_reference);
                    } else {
                        strcpy(reference, "");
                    }
                } else {
                    strcpy(reference, "");
                }
            }
        }
    }
    if (force_disabled) {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }

    if (!gui_util_str.portrait) {
        message_pos.x -= text_size.x + gui_util_str.border.x;
        if (use_global == NULL || (was_global != *use_global) || !*use_global) {
            message_pos.y -= diff_y / 2;
        }
        if (message_pos.y < 0) {
            message_pos.y = 0;
        }
        ImGui::SetCursorPos(message_pos);

        ImGui::PushStyleColor(ImGuiCol_Text, green_col);
        ImGui::Text("%s", message.c_str());
        ImGui::PopStyleColor();
    }

    Pop_buttons_size();
    ImGui::EndChild();
    return res;
}

static bool draw_checkbox(bool *reference, string message, int start_x, int end_x, int *pos_y) {
    return draw_component(CHECKBOX_TYPE, reference, message, start_x, end_x, pos_y);
}

static void draw_scalar_double(double *pdouble, string message, int start_x, int end_x, int *pos_y, int min, int max) {
    draw_component(DOUBLE_TYPE, pdouble, message, start_x, end_x, pos_y, min, max);
}

static void draw_scalar_64(uint64_t *pInt, string message, int start_x, int end_x, int *pos_y, int min, int max) {
    draw_component(SCALAR_TYPE, pInt, message, start_x, end_x, pos_y, min, max);
}

static void draw_combo(string message, int start_x, int end_x, int *pos_y, const char *combo_items, uint64_t *combo_value, int combo_behaviour) {
    draw_component(COMBO_TYPE, NULL, message, start_x, end_x, pos_y, 0, 0, NULL, false, false, false, false, NULL, false, false, NULL, NULL, 0, NULL, combo_items, combo_value, combo_behaviour);
}

static void draw_multiline(char *reference, string message, int start_x, int end_x, int *pos_y, const char *restore_reference) {
    draw_component(MULTI_LINE_TYPE, reference, message, start_x, end_x, pos_y, 0, 0, restore_reference, false, true, true, false, NULL, false, false, NULL, NULL, 0, NULL, NULL, NULL, -1, DESCRIPTION_MAX_LENGTH);
}

static void draw_select_item(char *reference,
                             const char *restore_reference,
                             const char *root_folder,
                             string message,
                             int start_x,
                             int end_x,
                             bool show_choose,
                             bool show_clear,
                             bool show_reset,
                             bool directory,
                             bool *use_global,
                             bool was_global,
                             int *pos_y,
                             bool force_disabled = false,
                             char **filter_descriptions = NULL,
                             char **filter_extensions = NULL,
                             int numFilters = 0) {
    draw_component(SINGLE_LINE_TYPE, reference, message, start_x, end_x, pos_y, 0, 0, restore_reference, show_choose, show_clear, show_reset, directory, use_global, was_global, force_disabled, filter_descriptions, filter_extensions, numFilters, root_folder, NULL, NULL, -1, FILENAME_MAX);
}

static void show_general_data() {
    int pos_y = 0;
    draw_section(INTERFACE_OPTIONS_SECTION_NAME, darkcian_col, 0, true, &pos_y);
    draw_section(JZINTV_OPTIONS_SECTION_NAME, darkcian_col, 0, true, &pos_y);
}

static void draw_custom_commands(string message, int start_x, int end_x, char **backup_custom_commands, char **original_custom_commands, bool *custom_command_use_global_data, bool *custom_command_was_global_data, int *res) {
    ImGuiWindow *win = ImGui::GetCurrentWindowRead();
    ImVec2 new_size;
    new_size.x = win->Size.x - 2 * gui_util_str.border.x;
    if (win->ScrollbarY) {
        new_size.x -= app_config_struct.scrollbar_size;
    }
    new_size.y = 7 * (gui_util_str.border.y + ImGui::GetFrameHeight());

    ImVec2 offs = ImGui::GetCursorPos();
    offs.x = 0;
    ImGui::SetCursorPos(offs);
    ImGui::BeginChild(message.c_str(), new_size, false);
    ImGuiWindow *container_window = ImGui::GetCurrentWindowRead();
    gui_util_str.options_sub_scrollable.window = ImGui::GetCurrentWindow();
    for (int i = 0; i < MAX_CUSTOM_COMMANDS; i++) {
        ostringstream param;
        param << "Parameter " << i + 1;
        bool *glob_data = NULL;
        bool was_glob = false;
        if (custom_command_use_global_data != NULL) {
            glob_data = &(custom_command_use_global_data[i]);
            was_glob = custom_command_was_global_data[i];
        }
        draw_select_item(backup_custom_commands[i], original_custom_commands[i], NULL, param.str().c_str(), start_x, end_x - (gui_util_str.portrait ? app_config_struct.scrollbar_size : -gui_util_str.border.x), false, true, true, false, glob_data, was_glob, res);

        if (*res < container_window->Pos.y) {
            *res = container_window->Pos.y;
        }

        if (*res > container_window->Pos.y + container_window->Size.y) {
            *res = container_window->Pos.y + container_window->Size.y;
        }
    }
    ImGui::EndChild();
}

static void draw_jzintv_sections(char **backup_hack_file,
                                 char *original_hack_file,
                                 char **backup_palette_file,
                                 char *original_palette_file,
                                 bool global_section,
                                 int start_x,
                                 int end_x,
                                 bool *hack_file_use_global,
                                 bool hack_file_was_global,
                                 bool *palette_file_use_global,
                                 bool palette_file_was_global,
                                 int *res) {
    ostringstream res_path;
    res_path << app_config_struct.resource_folder_absolute_path << "/Configs";

    draw_select_item(*backup_hack_file, original_hack_file, res_path.str().c_str(), "Keyboard hack file", start_x, end_x, true, true, true, false, hack_file_use_global, hack_file_was_global, res);
    draw_select_item(*backup_palette_file, original_palette_file, res_path.str().c_str(), "Palette file", start_x, end_x, true, true, true, false, palette_file_use_global, palette_file_was_global, res);
    draw_section(JZINTV_CUSTOM_COMMANDS_SECTION_NAME, blue_col, 1, global_section, res);
    if (!global_section) {
        draw_section(TUTORVISION_OPTIONS_SECTION_NAME, blue_col, 1, global_section, res);
    } else {
        if (app_config_struct.mobile_mode) {
            draw_section(ECS_OPTIONS_SECTION_NAME, blue_col, 1, global_section, res);
        }
    }
}

static void alloc_filters(const char *descriptions, const char *extensions, char ***out_descs, char ***out_exts, int *num) {
    const vector<std::string> &vec = split(descriptions, "_", true);
    *out_descs = (char **) malloc(sizeof(char *) * vec.size());
    for (int i = 0; i < vec.size(); i++) {
        (*out_descs)[i] = strdup(vec[i].c_str());
    }

    const vector<std::string> &vec2 = split(extensions, "_", true);
    *out_exts = (char **) malloc(sizeof(char *) * vec2.size());
    for (int i = 0; i < vec2.size(); i++) {
        (*out_exts)[i] = strdup(vec2[i].c_str());
    }

    *num = vec.size();
}

static void free_filters(char **out_descs, char **out_exts, int num) {
    for (int i = 0; i < num; i++) {
        free(out_descs[i]);
        free(out_exts[i]);
    }
    free(out_descs);
    free(out_exts);
}

string jzintv_resolution_string;
const char *jzintv_resolutions = NULL;

static void draw_section_content(string section, int start_x, int end_x, int zindex, bool general_section, int *res) {
    if (!section.compare(INTERFACE_OPTIONS_SECTION_NAME)) {
        draw_combo("Style", start_x, end_x, res, "Blue\0Classic\0Dark\0Light\0", &gui_util_str.backup.app_flags.style_index, STYLE_COMBO_BEHAVIOUR);
        draw_select_item(gui_util_str.backup.app_flags.roms_folder_ini, app_config_struct.roms_folder_ini, app_config_struct.root_folder_for_configuration, "Roms folder", start_x, end_x, true, false, true, true, NULL, false, res);

        ostringstream ttf_path;
        ttf_path << app_config_struct.resource_folder_absolute_path << "Fonts";
        char **filters_desc;
        char **filters_ext;
        int num;
        alloc_filters("Font files_All files", "*.ttf;*.otf_*.*", &filters_desc, &filters_ext, &num);
        draw_select_item(gui_util_str.backup.app_flags.font_filename,
                         app_config_struct.font_filename,
                         ttf_path.str().c_str(),
                         "Font file", start_x, end_x, true, true, true, false, NULL, false, res, false, filters_desc, filters_ext, 2);
        free_filters(filters_desc, filters_ext, num);

        int max_size = app_config_struct.mobile_mode ? 90 : 48;
        draw_scalar_64(&gui_util_str.backup.app_flags.font_size, "Font size", start_x, end_x, res, app_config_struct.mobile_mode ? 30 : 15, max_size);
        draw_scalar_64(&gui_util_str.backup.app_flags.scrollbar_size, "Scrollbar size", start_x, end_x, res, app_config_struct.mobile_mode ? 40 : 15, max_size);
        draw_scalar_64(&gui_util_str.backup.app_flags.buttons_size, "Buttons size", start_x, end_x, res, app_config_struct.mobile_mode ? 30 : 15, max_size);
        if (app_config_struct.mobile_mode) {
            draw_scalar_double(&gui_util_str.backup.app_flags.mobile_landscape_left_gap_percentage, "% left offset (landscape)", start_x, end_x, res, 0, 15);
            draw_scalar_double(&gui_util_str.backup.app_flags.mobile_landscape_right_gap_percentage, "% right offset (landscape)", start_x, end_x, res, 0, 15);
            draw_scalar_double(&gui_util_str.backup.app_flags.mobile_portrait_top_gap_percentage, "% top offset (portrait)", start_x, end_x, res, 0, 15);
            draw_scalar_double(&gui_util_str.backup.app_flags.mobile_portrait_bottom_gap_percentage, "% bottom offset (portrait)", start_x, end_x, res, 0, 15);
        }
        draw_checkbox(&gui_util_str.backup.app_flags.hide_unavailable_roms, HIDE_UNAVAILABLE_GAMES_TEXT, start_x, end_x, res);
    } else if (!section.compare(TUTORVISION_OPTIONS_SECTION_NAME)) {
        draw_checkbox(&gui_util_str.backup_game.rom_flags.use_tutorvision_exec, "Use Tutorvision EXEC", start_x, end_x, res);
        draw_checkbox(&gui_util_str.backup_game.rom_flags.use_tutorvision_grom, "Use Tutorvision GROM", start_x, end_x, res);
        draw_checkbox(&gui_util_str.backup_game.rom_flags.use_tutorvision_gram, "Use Tutorvision GRAM", start_x, end_x, res);
    } else if (!section.compare(ECS_OPTIONS_SECTION_NAME)) {
        draw_scalar_64(&gui_util_str.backup_game.app_flags.mobile_ecs_portrait_alpha, "Portrait transparency", start_x, end_x, res, 0, 255);
        draw_scalar_64(&gui_util_str.backup_game.app_flags.mobile_ecs_landscape_alpha, "Landscape transparency", start_x, end_x, res, 0, 255);
    } else if (!section.compare(JZINTV_OPTIONS_SECTION_NAME)) {
        if (general_section) {
            if (app_config_struct.mobile_mode) {
                if (draw_checkbox(&gui_util_str.backup.app_flags.mobile_show_controls, SHOW_ON_SCREEN_CONTROLS_TEXT, start_x, end_x, res)) {
                    if (!gui_util_str.backup.app_flags.mobile_show_controls) {
                        gui_util_str.backup.app_flags.mobile_show_configuration_controls = false;
                    }
                }
                if (draw_checkbox(&gui_util_str.backup.app_flags.mobile_show_configuration_controls, SHOW_CONFIGURATION_CONTROLS_TEXT, start_x, end_x, res)) {
                    if (gui_util_str.backup.app_flags.mobile_show_configuration_controls) {
                        gui_util_str.backup.app_flags.mobile_show_controls = true;
                    }
                }
                draw_checkbox(&gui_util_str.backup.reset_custom_controls, RESET_CUSTOM_CONTROLS, start_x, end_x, res);
                draw_scalar_64(&gui_util_str.backup.app_flags.mobile_default_portrait_controls_size, "Portrait controls size", start_x, end_x, res, 1, 4);
                draw_scalar_64(&gui_util_str.backup.app_flags.mobile_default_landscape_controls_size, "Landscape controls size", start_x, end_x, res, 1, 4);
            }

            if (can_launch_external_jzintv()) {
                draw_checkbox(&gui_util_str.backup.app_flags.use_external_jzintv, USE_EXTERNAL_JZINTV_TEXT, start_x, end_x, res);
            }
            if (!get_force_fullscreen()) {
                draw_checkbox(&gui_util_str.backup.app_flags.jzintv_fullscreen, FULLSCREEN_TEXT, start_x, end_x, res);
            }

            char *forced_resolution = get_forced_resolution_argument();
            if (forced_resolution == NULL) {
                if (jzintv_resolutions == NULL) {
                    jzintv_resolution_string.append("320x200x8");
                    jzintv_resolution_string.push_back('\0');
                    jzintv_resolution_string.append("640x480x8");
                    jzintv_resolution_string.push_back('\0');
                    jzintv_resolution_string.append("320x240x16");
                    jzintv_resolution_string.push_back('\0');
                    jzintv_resolution_string.append("1024x768x8");
                    jzintv_resolution_string.push_back('\0');
                    jzintv_resolution_string.append("1680x1050x8");
                    jzintv_resolution_string.push_back('\0');
                    jzintv_resolution_string.append("800x400x16");
                    jzintv_resolution_string.push_back('\0');
                    jzintv_resolution_string.append("1600x1200x32");
                    jzintv_resolution_string.push_back('\0');
                    jzintv_resolution_string.append("3280x1200x32");
                    jzintv_resolution_string.push_back('\0');
                    jzintv_resolutions = jzintv_resolution_string.c_str();
                }
                draw_combo("Resolution", start_x, end_x, res, jzintv_resolutions, &gui_util_str.backup.app_flags.jzintv_resolution_index, JZINTV_RESOLUTION_COMBO_BEHAVIOUR);
            } else {
                free(forced_resolution);
            }

            draw_jzintv_sections(&(gui_util_str.backup.app_flags.keyboard_hack_file),
                                 app_config_struct.keyboard_hack_file,
                                 &(gui_util_str.backup.app_flags.palette_file),
                                 app_config_struct.palette_file,
                                 general_section,
                                 start_x,
                                 end_x,
                                 NULL,
                                 false,
                                 NULL,
                                 false,
                                 res);
        } else {
            draw_jzintv_sections(&(gui_util_str.backup_game.app_flags.keyboard_hack_file),
                                 selected_rom->keyboard_hack_file,
                                 &(gui_util_str.backup_game.app_flags.palette_file),
                                 selected_rom->palette_file,
                                 general_section,
                                 start_x,
                                 end_x,
                                 &(gui_util_str.backup_game.hack_file_use_global),
                                 gui_util_str.backup_game.hack_file_was_global,
                                 &(gui_util_str.backup_game.palette_file_use_global),
                                 gui_util_str.backup_game.palette_file_was_global,
                                 res);

            draw_checkbox(&gui_util_str.backup_game.rom_flags.ecs_tape_name_auto, "Ecs tape (automatic name)", start_x, end_x, res);
            draw_checkbox(&gui_util_str.backup_game.rom_flags.jlp_save_file_auto, "Save JLP (automatic name)", start_x, end_x, res);
        }

    } else if (!section.compare(JZINTV_CUSTOM_COMMANDS_SECTION_NAME)) {
        if (general_section) {
            draw_custom_commands("General", start_x - gui_util_str.border.x, end_x, gui_util_str.backup.custom_commands_array_data_backup, gui_util_str.backup.custom_commands_array_data, NULL, NULL, res);
        } else {
            draw_custom_commands("Game", start_x - gui_util_str.border.x, end_x, gui_util_str.backup_game.custom_commands_array_data_backup, gui_util_str.backup_game.custom_commands_array_data, gui_util_str.backup_game.custom_command_use_global, gui_util_str.backup_game.custom_command_was_global, res);
        }
    } else if (!section.compare(INTERFACE_GAME_INFO_SECTION_NAME)) {
        draw_select_item(selected_rom->file_name,
                         selected_rom->file_name,
                         NULL,
                         "File name", start_x, end_x, false, false, false, false, NULL, false, res, true);

        if (selected_rom->available_status != ROM_AVAILABLE_STATUS_UNKNOWN) {
            draw_select_item(gui_util_str.backup_game.rom_flags.game_name,
                             selected_rom->game_name,
                             NULL,
                             "Name", start_x, end_x, false, false, true, false, NULL, false, res, false);

            ostringstream res_path;
            res_path << app_config_struct.resource_folder_absolute_path << "/Images/Screenshots";

            char **filters_desc;
            char **filters_ext;
            int num;
            alloc_filters("Image files_All files", "*.gif;*.png;*.jpg;*.bmp_*.*", &filters_desc, &filters_ext, &num);
            draw_select_item(gui_util_str.backup_game.rom_flags.image_file_name,
                             selected_rom->image_file_name,
                             res_path.str().c_str(),
                             "Image file name", start_x, end_x, true, true, true, false, NULL, false, res, false, filters_desc, filters_ext, num);

            res_path.str("");
            res_path.clear();
            res_path << app_config_struct.resource_folder_absolute_path << "/Images/Boxes";

            draw_select_item(gui_util_str.backup_game.rom_flags.box_file_name,
                             selected_rom->box_file_name,
                             res_path.str().c_str(),
                             "Box file name", start_x, end_x, true, true, true, false, NULL, false, res, false, filters_desc, filters_ext, num);

            free_filters(filters_desc, filters_ext, num);

            draw_multiline(gui_util_str.backup_game.rom_flags.description,
                           "Description",
                           start_x,
                           end_x,
                           res,
                           selected_rom->description);
            gui_util_str.options_desc_scrollable.window = lastChild;
        }
    }

    if (zindex == 0) {
        ImGuiWindow *container_window = ImGui::GetCurrentWindowRead();

        if (*res > container_window->Pos.y + container_window->Size.y) {
            *res = container_window->Pos.y + container_window->Size.y;
        }
    }
}

static void draw_section(string section, ImVec4 border_col, int zindex, bool general_section, int *pos_y) {
    int y_gap = 2 * gui_util_str.border.y;
    ImGuiWindow *win = ImGui::GetCurrentWindowRead();
    ImVec2 window_size = win->Size;
    window_size.x -= 2 * gui_util_str.border.x * zindex;
    ImVec2 window_pos = win->Pos;
    window_pos.y += ImGui::GetCursorPosY();

    int x_offset = zindex * gui_util_str.border.x;
    int y_offset = zindex * gui_util_str.border.y;

    window_pos.x += x_offset;
    window_pos.y += y_offset;

    ImRect text_rect;

    // Section title
    ImVec2 text_section_size = ImGui::CalcTextSize(section.c_str(), NULL, true);
    ImVec2 start_title_pos;
    start_title_pos.x = x_offset + gui_util_str.border.x + NUM_PIXELS_BEFORE_SECTION_NAME + NUM_PIXELS_SPACE_SECTION_NAME;
    start_title_pos.y = y_offset + ImGui::GetCursorPosY();
    ImGui::SetCursorPos(start_title_pos);
    ImVec2 act_title_pos = start_title_pos;

    ImGui::PushStyleColor(ImGuiCol_Text, lightblue_col);
    const vector<std::string> &tmp = get_vector_string(section.c_str(), window_size.x - start_title_pos.x - gui_util_str.border.x - NUM_PIXELS_BEFORE_SECTION_NAME - NUM_PIXELS_SPACE_SECTION_NAME);
    for (int i = 0; i < tmp.size(); i++) {
        float act_text_size_y = ImGui::CalcTextSize(tmp[i].c_str(), NULL, true).y;
        ImGui::Text("%s", tmp[i].c_str());
        if (i == 0) {
            text_rect = win->DC.LastItemRect;
        }
        act_title_pos.y += (float) act_text_size_y;
        ImGui::SetCursorPos(act_title_pos);
    }
    ImGui::PopStyleColor();

    // First line
    ImVec2 start_line_1;
    start_line_1.x = window_pos.x + gui_util_str.border.x;
    start_line_1.y = window_pos.y + gui_util_str.border.y * (text_section_size.y / (float(16))) - win->Scroll.y;
    ImVec2 end_line_1;
    end_line_1.x = start_line_1.x + NUM_PIXELS_BEFORE_SECTION_NAME;
    end_line_1.y = start_line_1.y;
    win->DrawList->AddLine(start_line_1, end_line_1, ImGui::GetColorU32(border_col));

    // Second line
    ImVec2 start_line_2;
    start_line_2.x += text_rect.Max.x + NUM_PIXELS_SPACE_SECTION_NAME;
    start_line_2.y = start_line_1.y;
    ImVec2 end_line_2;
    end_line_2.x = window_pos.x + window_size.x - gui_util_str.border.x;
    if (win->ScrollbarY) {
        end_line_2.x -= app_config_struct.scrollbar_size;
    }
    end_line_2.y = start_line_2.y;
    win->DrawList->AddLine(start_line_2, end_line_2, ImGui::GetColorU32(border_col));

    // Do specific section stuff
    ImVec2 offs = ImGui::GetCursorPos();
    offs.y += y_gap;
    ImGui::SetCursorPos(offs);
    draw_section_content(section, start_line_1.x - gui_util_str.left_gap_pixels, end_line_2.x - gui_util_str.left_gap_pixels, zindex, general_section, pos_y);

    // Third line
    ImVec2 start_line_3;
    start_line_3.x = start_line_1.x;
    start_line_3.y = start_line_1.y;
    ImVec2 end_line_3;
    end_line_3.x = start_line_1.x;
    end_line_3.y = *pos_y + gui_util_str.border.y;
    win->DrawList->AddLine(start_line_3, end_line_3, ImGui::GetColorU32(border_col));

    // Fourth line
    ImVec2 start_line_4;
    start_line_4.x = end_line_2.x;
    start_line_4.y = end_line_2.y;
    ImVec2 end_line_4;
    end_line_4.x = end_line_2.x;
    end_line_4.y = end_line_3.y;
    win->DrawList->AddLine(start_line_4, end_line_4, ImGui::GetColorU32(border_col));

    // Fifth line
    ImVec2 start_line_5;
    start_line_5.x = end_line_3.x;
    start_line_5.y = end_line_3.y;
    ImVec2 end_line_5;
    end_line_5.x = end_line_4.x;
    end_line_5.y = end_line_4.y;
    win->DrawList->AddLine(start_line_5, end_line_5, ImGui::GetColorU32(border_col));

    ostringstream s;
    s << "Offset_" << section;
    ImGui::BeginChild(s.str().c_str(), ImVec2{0, 2 * gui_util_str.border.y}, false);
    ImGui::EndChild();
    *pos_y += 2 * gui_util_str.border.y;
}

void reset_backup_data() {
    gui_util_str.backup.font_required = !is_memory_blank(app_config_struct.font_filename);
    gui_util_str.backup.app_flags.window_width = gui_util_str.act_window_width;
    gui_util_str.backup.app_flags.window_height = gui_util_str.act_window_height;
    gui_util_str.backup.app_flags.hide_unavailable_roms = app_config_struct.hide_unavailable_roms;
    gui_util_str.backup.app_flags.font_size = app_config_struct.font_size;
    gui_util_str.backup.app_flags.style_index = app_config_struct.style_index;
    gui_util_str.backup.app_flags.jzintv_resolution_index = app_config_struct.jzintv_resolution_index;
    gui_util_str.backup.app_flags.buttons_size = app_config_struct.buttons_size;
    gui_util_str.backup.app_flags.scrollbar_size = app_config_struct.scrollbar_size;
    gui_util_str.backup.app_flags.mobile_show_controls = app_config_struct.mobile_show_controls;
    gui_util_str.backup.app_flags.mobile_show_configuration_controls = app_config_struct.mobile_show_configuration_controls;
    gui_util_str.backup.app_flags.jzintv_fullscreen = app_config_struct.jzintv_fullscreen;
    gui_util_str.backup.app_flags.use_external_jzintv = app_config_struct.use_external_jzintv;
    gui_util_str.backup.app_flags.mobile_portrait_top_gap_percentage = app_config_struct.mobile_portrait_top_gap_percentage;
    gui_util_str.backup.app_flags.mobile_portrait_bottom_gap_percentage = app_config_struct.mobile_portrait_bottom_gap_percentage;
    gui_util_str.backup.app_flags.mobile_landscape_right_gap_percentage = app_config_struct.mobile_landscape_right_gap_percentage;
    gui_util_str.backup.app_flags.mobile_landscape_left_gap_percentage = app_config_struct.mobile_landscape_left_gap_percentage;
    gui_util_str.backup.app_flags.mobile_default_portrait_controls_size = app_config_struct.mobile_default_portrait_controls_size;
    gui_util_str.backup.app_flags.mobile_default_landscape_controls_size = app_config_struct.mobile_default_landscape_controls_size;

    gui_util_str.backup.reset_custom_controls = false;

    // Roms folder
    gui_util_str.backup.app_flags.roms_folder_ini = (char *) malloc(sizeof(char) * (1 + FILENAME_MAX));
    strcpy(gui_util_str.backup.app_flags.roms_folder_ini, app_config_struct.roms_folder_ini);

    // Ttf
    gui_util_str.backup.app_flags.font_filename = (char *) malloc(sizeof(char) * (1 + FILENAME_MAX));
    strcpy(gui_util_str.backup.app_flags.font_filename, app_config_struct.font_filename);

    // Ecs
    gui_util_str.backup_game.app_flags.mobile_ecs_portrait_alpha = app_config_struct.mobile_ecs_portrait_alpha;
    gui_util_str.backup_game.app_flags.mobile_ecs_landscape_alpha = app_config_struct.mobile_ecs_landscape_alpha;

    reset_jzintv_backup_data(&gui_util_str.backup,
                             app_config_struct.keyboard_hack_file,
                             app_config_struct.palette_file,
                             app_config_struct.custom_commands);

    if (roms_list_struct.total_roms_num > 0) {
        if (selected_rom->available_status != ROM_AVAILABLE_STATUS_UNKNOWN) {
            reset_backup_data_for_selected_game();
        }
    }
}

static bool check_basic_refresh() {
    bool res = false;
    bool portrait = gui_util_str.act_window_width < gui_util_str.act_window_height;
    bool was_portrait = gui_util_str.backup.app_flags.window_width < gui_util_str.backup.app_flags.window_height;

    if (!app_config_struct.mobile_mode || (portrait != was_portrait)) {
        res |= gui_util_str.act_window_width != gui_util_str.backup.app_flags.window_width;
        res |= gui_util_str.act_window_height != gui_util_str.backup.app_flags.window_height;
    }
    return res;
}

static void free_jzintv_backup_data(backup_config_struct_t *backup_struct) {
    // Keyboard hack file
    if (backup_struct->app_flags.keyboard_hack_file != NULL) {
        free(backup_struct->app_flags.keyboard_hack_file);
    }

    // Palette file
    if (backup_struct->app_flags.palette_file != NULL) {
        free(backup_struct->app_flags.palette_file);
    }

    // Custom commands
    for (int i = 0; i < MAX_CUSTOM_COMMANDS; i++) {
        free(backup_struct->custom_commands_array_data[i]);
        free(backup_struct->custom_commands_array_data_backup[i]);
    }
    free(backup_struct->custom_commands_array_data);
    free(backup_struct->custom_commands_array_data_backup);
}

static void free_backup_data() {
    // Roms folder
    if (gui_util_str.backup.app_flags.roms_folder_ini != NULL) {
        free(gui_util_str.backup.app_flags.roms_folder_ini);
    }

    // Ttf
    if (gui_util_str.backup.app_flags.font_filename != NULL) {
        free(gui_util_str.backup.app_flags.font_filename);
    }

    free_jzintv_backup_data(&gui_util_str.backup);

    if (roms_list_struct.total_roms_num > 0) {
        if (selected_rom->available_status != ROM_AVAILABLE_STATUS_UNKNOWN) {
            free_jzintv_backup_data(&gui_util_str.backup_game);
            free(gui_util_str.backup_game.rom_flags.game_name);
            free(gui_util_str.backup_game.rom_flags.description);
            free(gui_util_str.backup_game.rom_flags.image_file_name);
            free(gui_util_str.backup_game.rom_flags.box_file_name);
        }
    }
}

static void manage_backup_jzintv_data_on_save(backup_config_struct_t *backup_struct,
                                              char **keyboard_hack_file,
                                              char **palette_file,
                                              vector<string> *custom_commands) {

    ostringstream configs_path;
    configs_path << app_config_struct.resource_folder_absolute_path << "Configs";

    // Keyboard hack file
    update_on_change(&(backup_struct->app_flags.keyboard_hack_file),
                     keyboard_hack_file,
                     NULL,
                     configs_path.str().c_str(),
                     "Specified keyboard hack file is not valid",
                     false,
                     true);

    // Palette file
    update_on_change(&(backup_struct->app_flags.palette_file),
                     palette_file,
                     NULL,
                     configs_path.str().c_str(),
                     "Specified palette file is not valid",
                     false,
                     true);

    // Custom commands
    if (custom_commands->size() > 0) {
        custom_commands->clear();
    }
    for (int i = 0; i < MAX_CUSTOM_COMMANDS; i++) {
        string new_command = backup_struct->custom_commands_array_data_backup[i];
        trim(new_command);
        custom_commands->push_back(new_command);
    }
}

static bool manage_backup_data_on_save(bool *refresh_for_text) {
    bool res = false;
    bool roms_folder_changed = false;
    ostringstream err;

    // Roms folder
    if (is_memory_blank(gui_util_str.backup.app_flags.roms_folder_ini)) {
        ADD_POPUP("Specified roms folder is not valid", "Empty roms folder is not valid");
    } else {
        err.str("");
        err.clear();
        err << "Specified roms folder is not valid (" << gui_util_str.backup.app_flags.roms_folder_ini << ")";
        roms_folder_changed |= update_on_change(&(gui_util_str.backup.app_flags.roms_folder_ini),
                                                &(app_config_struct.roms_folder_ini),
                                                &(app_config_struct.roms_folder_absolute_path),
                                                app_config_struct.root_folder_for_configuration,
                                                err.str().c_str(),
                                                true);
        res |= roms_folder_changed;
        if (roms_folder_changed) {
            gui_util_str.reload_roms_on_refresh = true;
        }
    }

    // Ttf
    ostringstream ttf_path;
    ttf_path << app_config_struct.resource_folder_absolute_path << "Fonts";
    err.str("");
    err.clear();
    err << "Specified ttf file is not valid (" << gui_util_str.backup.app_flags.font_filename << ")";
    *refresh_for_text |= update_on_change(&(gui_util_str.backup.app_flags.font_filename),
                                          &(app_config_struct.font_filename),
                                          NULL,
                                          ttf_path.str().c_str(),
                                          err.str().c_str(),
                                          false,
                                          true);

    manage_backup_jzintv_data_on_save(&(gui_util_str.backup),
                                      &(app_config_struct.keyboard_hack_file),
                                      &(app_config_struct.palette_file),
                                      app_config_struct.custom_commands);

    // Hide unavailable roms
    bool tmp = app_config_struct.hide_unavailable_roms != gui_util_str.backup.app_flags.hide_unavailable_roms;
    res |= tmp;
    app_config_struct.hide_unavailable_roms = gui_util_str.backup.app_flags.hide_unavailable_roms;
    if (tmp) {
        gui_util_str.reload_roms_on_refresh = true;
    }

    // Buttons size
    res |= app_config_struct.buttons_size != gui_util_str.backup.app_flags.buttons_size;
    app_config_struct.buttons_size = gui_util_str.backup.app_flags.buttons_size;

    // Scrollbar size
    res |= app_config_struct.scrollbar_size != gui_util_str.backup.app_flags.scrollbar_size;
    app_config_struct.scrollbar_size = gui_util_str.backup.app_flags.scrollbar_size;

    // Font size
    *refresh_for_text |= app_config_struct.font_size != gui_util_str.backup.app_flags.font_size;
    app_config_struct.font_size = gui_util_str.backup.app_flags.font_size;

    app_config_struct.style_index = gui_util_str.backup.app_flags.style_index;
    app_config_struct.jzintv_resolution_index = gui_util_str.backup.app_flags.jzintv_resolution_index;

    app_config_struct.mobile_show_controls = gui_util_str.backup.app_flags.mobile_show_controls;
    app_config_struct.mobile_show_configuration_controls = gui_util_str.backup.app_flags.mobile_show_configuration_controls;
    app_config_struct.jzintv_fullscreen = gui_util_str.backup.app_flags.jzintv_fullscreen;
    app_config_struct.use_external_jzintv = gui_util_str.backup.app_flags.use_external_jzintv;

    // Mobile gaps
    res |= gui_util_str.backup.app_flags.mobile_portrait_top_gap_percentage != app_config_struct.mobile_portrait_top_gap_percentage;
    app_config_struct.mobile_portrait_top_gap_percentage = gui_util_str.backup.app_flags.mobile_portrait_top_gap_percentage;

    res |= gui_util_str.backup.app_flags.mobile_portrait_bottom_gap_percentage != app_config_struct.mobile_portrait_bottom_gap_percentage;
    app_config_struct.mobile_portrait_bottom_gap_percentage = gui_util_str.backup.app_flags.mobile_portrait_bottom_gap_percentage;

    res |= gui_util_str.backup.app_flags.mobile_landscape_right_gap_percentage != app_config_struct.mobile_landscape_right_gap_percentage;
    app_config_struct.mobile_landscape_right_gap_percentage = gui_util_str.backup.app_flags.mobile_landscape_right_gap_percentage;

    res |= gui_util_str.backup.app_flags.mobile_landscape_left_gap_percentage != app_config_struct.mobile_landscape_left_gap_percentage;
    app_config_struct.mobile_landscape_left_gap_percentage = gui_util_str.backup.app_flags.mobile_landscape_left_gap_percentage;

    app_config_struct.mobile_default_portrait_controls_size = gui_util_str.backup.app_flags.mobile_default_portrait_controls_size;
    app_config_struct.mobile_default_landscape_controls_size = gui_util_str.backup.app_flags.mobile_default_landscape_controls_size;

    // Ecs
    app_config_struct.mobile_ecs_portrait_alpha = gui_util_str.backup_game.app_flags.mobile_ecs_portrait_alpha;
    app_config_struct.mobile_ecs_landscape_alpha = gui_util_str.backup_game.app_flags.mobile_ecs_landscape_alpha;

    if (roms_list_struct.total_roms_num > 0) {
        if (selected_rom->available_status != ROM_AVAILABLE_STATUS_UNKNOWN) {

            manage_backup_jzintv_data_on_save(&(gui_util_str.backup_game),
                                              &(selected_rom->keyboard_hack_file),
                                              &(selected_rom->palette_file),
                                              selected_rom->custom_commands);

            int config_index = find_roms_config_index(gui_util_str.rom_index_selected);

            if (is_memory_empty(selected_rom->keyboard_hack_file) && !gui_util_str.backup_game.hack_file_use_global) {
                free(selected_rom->keyboard_hack_file);
                selected_rom->keyboard_hack_file = strdup(DISABLE_GLOBAL_VALUE);
            }
            if (roms_configuration[config_index].keyboard_hack_file != NULL) {
                free(roms_configuration[config_index].keyboard_hack_file);
                roms_configuration[config_index].keyboard_hack_file = NULL;
            }
            if (selected_rom->keyboard_hack_file != NULL) {
                roms_configuration[config_index].keyboard_hack_file = strdup(selected_rom->keyboard_hack_file);
            }

            if (is_memory_empty(selected_rom->palette_file) && !gui_util_str.backup_game.palette_file_use_global) {
                free(selected_rom->palette_file);
                selected_rom->palette_file = strdup(DISABLE_GLOBAL_VALUE);
            }
            if (roms_configuration[config_index].palette_file != NULL) {
                free(roms_configuration[config_index].palette_file);
                roms_configuration[config_index].palette_file = NULL;
            }
            if (selected_rom->palette_file != NULL) {
                roms_configuration[config_index].palette_file = strdup(selected_rom->palette_file);
            }

            vector<string> tmp;
            for (int i = 0; i < MAX_CUSTOM_COMMANDS; i++) {
                if (is_memory_empty(selected_rom->custom_commands->at(i).c_str()) && !gui_util_str.backup_game.custom_command_use_global[i]) {
                    tmp.push_back(DISABLE_GLOBAL_VALUE);
                } else {
                    tmp.push_back(selected_rom->custom_commands->at(i));
                }
            }

            selected_rom->custom_commands->clear();
            roms_configuration[config_index].custom_commands->clear();
            for (int i = 0; i < MAX_CUSTOM_COMMANDS; i++) {
                selected_rom->custom_commands->push_back(tmp[i]);
                roms_configuration[config_index].custom_commands->push_back(tmp[i]);
            }

            // Game name
            bool name_changed = update_on_change(&(gui_util_str.backup_game.rom_flags.game_name),
                                                 &selected_rom->game_name,
                                                 NULL,
                                                 NULL,
                                                 "",
                                                 false,
                                                 false);

            if (is_memory_empty(selected_rom->game_name)) {
                free(selected_rom->game_name);
                selected_rom->game_name = strdup(selected_rom->file_name);
            }

            if (roms_configuration[config_index].game_name != NULL) {
                free(roms_configuration[config_index].game_name);
                roms_configuration[config_index].game_name = NULL;
            }
            if (selected_rom->game_name != NULL) {
                roms_configuration[config_index].game_name = strdup(selected_rom->game_name);
            }

            res |= name_changed;

            // Image file name
            ostringstream images_path;
            images_path << app_config_struct.resource_folder_absolute_path << "Images/Screenshots";

            bool image_changed = false;
            bool backup_blank = is_memory_blank(gui_util_str.backup_game.rom_flags.image_file_name);
            bool origin_blank = is_memory_blank(selected_rom->image_file_name);
            if (!backup_blank || !origin_blank) {
                image_changed = update_on_change(&(gui_util_str.backup_game.rom_flags.image_file_name),
                                                 &selected_rom->image_file_name,
                                                 NULL,
                                                 images_path.str().c_str(),
                                                 "Specified image screenshot is not valid",
                                                 false,
                                                 true);

                if (roms_configuration[config_index].image_file_name != NULL) {
                    free(roms_configuration[config_index].image_file_name);
                    roms_configuration[config_index].image_file_name = NULL;
                }
                if (selected_rom->image_file_name != NULL) {
                    roms_configuration[config_index].image_file_name = strdup(selected_rom->image_file_name);
                }
            }

            // Box file name
            images_path.str("");
            images_path.clear();
            images_path << app_config_struct.resource_folder_absolute_path << "Images/Boxes";

            backup_blank = is_memory_blank(gui_util_str.backup_game.rom_flags.box_file_name);
            origin_blank = is_memory_blank(selected_rom->box_file_name);
            if (!backup_blank || !origin_blank) {
                image_changed |= update_on_change(&(gui_util_str.backup_game.rom_flags.box_file_name),
                                                  &selected_rom->box_file_name,
                                                  NULL,
                                                  images_path.str().c_str(),
                                                  "Specified image box is not valid",
                                                  false,
                                                  true);

                if (roms_configuration[config_index].box_file_name != NULL) {
                    free(roms_configuration[config_index].box_file_name);
                    roms_configuration[config_index].box_file_name = NULL;
                }
                if (selected_rom->box_file_name != NULL) {
                    roms_configuration[config_index].box_file_name = strdup(selected_rom->box_file_name);
                }
            }

            if (image_changed) {
                gui_util_str.reload_roms_on_refresh = true;
            }
            res |= image_changed;

            // Description
            bool description_changed = update_on_change(&(gui_util_str.backup_game.rom_flags.description),
                                                        &selected_rom->description,
                                                        NULL,
                                                        NULL,
                                                        "",
                                                        false,
                                                        false);

            if (roms_configuration[config_index].description != NULL) {
                free(roms_configuration[config_index].description);
                roms_configuration[config_index].description = NULL;
            }
            if (selected_rom->description != NULL) {
                roms_configuration[config_index].description = strdup(selected_rom->description);
            }

            // Tutorvision
            selected_rom->use_tutorvision_exec = gui_util_str.backup_game.rom_flags.use_tutorvision_exec;
            selected_rom->use_tutorvision_grom = gui_util_str.backup_game.rom_flags.use_tutorvision_grom;
            selected_rom->use_tutorvision_gram = gui_util_str.backup_game.rom_flags.use_tutorvision_gram;
            roms_configuration[config_index].use_tutorvision_exec = selected_rom->use_tutorvision_exec;
            roms_configuration[config_index].use_tutorvision_grom = selected_rom->use_tutorvision_grom;
            roms_configuration[config_index].use_tutorvision_gram = selected_rom->use_tutorvision_gram;

            // Ecs
            selected_rom->ecs_tape_name_auto = gui_util_str.backup_game.rom_flags.ecs_tape_name_auto;
            roms_configuration[config_index].ecs_tape_name_auto = selected_rom->ecs_tape_name_auto;

            // Jlp
            selected_rom->jlp_save_file_auto = gui_util_str.backup_game.rom_flags.jlp_save_file_auto;
            roms_configuration[config_index].jlp_save_file_auto = selected_rom->jlp_save_file_auto;
        }
    }

    if ((gui_util_str.backup.font_required && is_memory_blank(app_config_struct.font_filename)) ||
        (!gui_util_str.backup.font_required && !is_memory_blank(app_config_struct.font_filename))) {
        *refresh_for_text = true;
    }

    return res |= *refresh_for_text;
}

static void on_configuration_window_close(bool save, bool *refresh_for_text) {
    gui_util_str.center_at_rom_index_if_needed = true;
    init_multiline = true;
    gui_util_str.configuration_change_tab_index = true;
    gui_util_str.configuration_act_tab_index = 0;
    gui_util_str.show_demo_window = false;
    gui_util_str.flip_demo_window = false;
    gui_util_str.show_config_window = false;
    bool refresh = check_basic_refresh();

    if (save) {
        refresh |= manage_backup_data_on_save(refresh_for_text);
        if (gui_util_str.backup.reset_custom_controls) {
            remove_custom_controls();
        }
    }
    gui_util_str.backup.reset_custom_controls = false;
    apply_style();
    free_backup_data();
    save_config_file();
    if (refresh) {
        request_for_scroll(RECOMPILE_MODE);
    }
    ImGuiStyle &style = ImGui::GetStyle();
    style.ScrollbarSize = app_config_struct.scrollbar_size;
    gui_util_str.reset_backup_data = true;
}

#ifndef WIN32
#include <ImGuiFileDialog.h>
bool check_back_config_window() {
     if (gui_util_str.show_config_window) {
        if (app_config_struct.dialog_reference != NULL) {
            ImGuiFileDialog::Instance()->Close();
            app_config_struct.dialog_reference = NULL;
        } else {
            on_configuration_window_close(false, NULL);
        }
        return true;
     }
    return false;
}
#else
bool check_back_config_window() {
    if (gui_util_str.show_config_window) {
        on_configuration_window_close(false, NULL);
        return true;
    }
    return false;
}
#endif

bool wait_for_click_to_reopen_keyb = false;

void soft_keyb_management() {
    if (wait_for_click_to_reopen_keyb) {
        if (ImGui::IsMouseClicked(0)) {
            ImGuiContext &g = *GImGui;
            ImGuiID itID = g.InputTextState.ID;
            ImGuiID actID = g.ActiveId;
            if (itID == actID && 1 == g.WantTextInputNextFrame && g.HoveredId > 0) {
                wait_for_click_to_reopen_keyb = false;
            }
        }
    }
    if (ImGui::GetIO().WantTextInput && !wait_for_click_to_reopen_keyb) {
        SDL_StartTextInput();
        wait_for_click_to_reopen_keyb = true;
    }
}

void show_configuration_window(bool *refresh_for_text) {
    bool old_portrait = gui_util_str.portrait;
    int old_win_width = gui_util_str.act_window_width;
    int old_win_height = gui_util_str.act_window_height;
    int left_gap_pixels = 0;
    int right_gap_pixels = 0;
    int top_gap_pixels = 0;
    int bottom_gap_pixels = 0;
    get_gap_pixels(&left_gap_pixels, &right_gap_pixels, &top_gap_pixels, &bottom_gap_pixels);

    const ImVec2 &alignment_size = ImGui::CalcTextSize(ALIGNMENT_STRING, NULL, true);
    if (alignment_size.x > ((float) (gui_util_str.act_window_width - left_gap_pixels - right_gap_pixels) / 3.5)) {
        gui_util_str.portrait = true;
    }

    string save_and_close_button_message_string = "Save & Close";
    const char *save_and_close_button_message = save_and_close_button_message_string.c_str();

    string cancel_button_message_string = "Cancel";
    const char *cancel_button_message = cancel_button_message_string.c_str();

    // Background window
    ImVec2 window_size_vec_full_bg;
    window_size_vec_full_bg.x = gui_util_str.act_window_width - left_gap_pixels - right_gap_pixels;
    window_size_vec_full_bg.y = gui_util_str.act_window_height - top_gap_pixels - bottom_gap_pixels;

    ImGui::SetNextWindowPos(ImVec2{static_cast<float>(left_gap_pixels), static_cast<float>(top_gap_pixels)});
    ImGui::SetNextWindowSize(window_size_vec_full_bg, ImGuiCond_Always);
    ImGuiWindowFlags flags = get_window_flags(false, true, true, false);
    flags |= ImGuiWindowFlags_Modal;
    ImGui::Begin("##Background", NULL, flags);

    // Background image
    ImVec2 window_size_vec_full_bg_img;
    window_size_vec_full_bg_img.x = window_size_vec_full_bg.x;
    window_size_vec_full_bg_img.y = window_size_vec_full_bg.y;
    const ImVec2 &old_pos = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(0, 0));
    draw_background(window_size_vec_full_bg_img);
    ImGui::SetCursorPos(old_pos);

    // Options window
    ImVec2 options_window_size;
    options_window_size.x = gui_util_str.act_window_width;
    options_window_size.x -= left_gap_pixels;
    options_window_size.x -= right_gap_pixels;
    options_window_size.y = gui_util_str.act_window_height;
    options_window_size.y -= bottom_gap_pixels;
    options_window_size.y -= top_gap_pixels;

    ImVec2 options_window_pos;
    options_window_pos.x = left_gap_pixels;
    options_window_pos.y = top_gap_pixels;
    ImGui::SetNextWindowPos(options_window_pos);
    ImGui::SetNextWindowSize(options_window_size, ImGuiCond_Always);
    ImGui::Begin("Options", NULL, flags);

    int tab_selected = 0;
    ImGuiID id = ImGui::GetCurrentWindow()->GetID(TAB_CONFIGURATION_ID_STRING);
    int tab_size_pixel;
    if (ImGui::BeginTabBar(TAB_CONFIGURATION_ID_STRING, ImGuiTabBarFlags_None)) {
        ImGuiTabBar *tab_bar = (*GImGui).TabBars.GetOrAddByKey(id);
        tab_size_pixel = tab_bar->BarRect.Max.y - tab_bar->BarRect.Min.y;
        gui_util_str.num_configs_tabs = 0;
        if (ImGui::BeginTabItem("General")) {
            ImGui::EndTabItem();
        }
        gui_util_str.num_configs_tabs++;

        if (roms_list_struct.total_roms_num > 0) {
            if (ImGui::BeginTabItem("Game")) {
                ImGui::EndTabItem();
            }
            gui_util_str.num_configs_tabs++;
        }

        if (ImGui::BeginTabItem("About")) {
            ImGui::EndTabItem();
        }
        gui_util_str.num_configs_tabs++;

        ImGuiID act_id = tab_bar->SelectedTabId;
        int actual_id = -1;
        for (int i = 0; i < tab_bar->Tabs.Size; i++) {
            if (act_id == tab_bar->Tabs[i].ID) {
                actual_id = i;
                break;
            }
        }
        if (actual_id != -1 && gui_util_str.configuration_act_tab_index != actual_id && !gui_util_str.configuration_change_tab_index) {
            gui_util_str.configuration_change_tab_index = true;
            gui_util_str.configuration_act_tab_index = actual_id;
        }

        if (gui_util_str.configuration_change_tab_index) {
            if (gui_util_str.options_scrollable.window != NULL) {
                gui_util_str.options_scrollable.window->ScrollTarget.y = 0;
            }
            set_tab_index(gui_util_str.configuration_act_tab_index);
            gui_util_str.show_demo_window = false;
            gui_util_str.flip_demo_window = false;
            gui_util_str.configuration_change_tab_index = false;
        }
        tab_selected = get_tab_index();
        ImGui::EndTabBar();
    }

    ImVec2 configuration_window_size;
    configuration_window_size.x = gui_util_str.act_window_width;
    configuration_window_size.x -= 2 * gui_util_str.border.x;

    configuration_window_size.y = gui_util_str.act_window_height;
    configuration_window_size.y -= 4 * gui_util_str.border.y;
    configuration_window_size.y -= tab_size_pixel;
    configuration_window_size.y -= gui_util_str.title_bar_size.y;
    configuration_window_size.y -= app_config_struct.buttons_size;

    configuration_window_size.x -= left_gap_pixels;
    configuration_window_size.x -= right_gap_pixels;
    configuration_window_size.y -= bottom_gap_pixels;
    configuration_window_size.y -= top_gap_pixels;

    gui_util_str.act_window_width -= left_gap_pixels + right_gap_pixels;
    gui_util_str.act_window_height -= top_gap_pixels + bottom_gap_pixels;
    ImGui::BeginChild("Configuration_info", configuration_window_size, true);
    gui_util_str.options_scrollable.window = ImGui::GetCurrentWindow();
    gui_util_str.options_scrollable.hover_unneeded = true;
    switch (tab_selected) {
        case 0:
            show_general_data();
            break;
        case 1:
            if (roms_list_struct.total_roms_num > 0) {
                show_game_data();
            } else {
                show_info_and_thanks();
            }
            break;
        case 2:
            show_info_and_thanks();
            break;
    }

    ImGui::EndChild();

    // Close buttons
    ImGui::BeginChild("Close_info", ImVec2{0, 0});
    Push_buttons_size();
    const ImVec2 &save_and_close_button_message_size = ImGui::CalcTextSize(save_and_close_button_message, NULL, true);
    const ImVec2 &cancel_button_message_size = ImGui::CalcTextSize(cancel_button_message, NULL, true);
    float offset_x = save_and_close_button_message_size.x + 3 * gui_util_str.border.x + cancel_button_message_size.x;
    ImGui::SetCursorPos(ImVec2((ImGui::GetCurrentWindow()->Size.x - offset_x) / 2, ImGui::GetCurrentWindowRead()->Size.y - app_config_struct.buttons_size - gui_util_str.border.y));
    if (ImGui::Button(save_and_close_button_message)) {
        on_configuration_window_close(true, refresh_for_text);
    }
    ImGui::SameLine();
    if (ImGui::Button(cancel_button_message)) {
        on_configuration_window_close(false, refresh_for_text);
    }
    Pop_buttons_size();
    ImGui::EndChild();

    ImGui::End();
    ImGui::End();
    if (app_config_struct.mobile_mode) {
        bool canScroll = gui_util_str.options_sub_scrollable.window == NULL || !gui_util_str.options_sub_scrollable.scroll(true);
        canScroll &= gui_util_str.options_desc_scrollable.window == NULL || !gui_util_str.options_desc_scrollable.scroll(true);
        if (canScroll) {
            gui_util_str.options_scrollable.scroll();
        }
    }

    gui_util_str.portrait = old_portrait;
    gui_util_str.act_window_width = old_win_width;
    gui_util_str.act_window_height = old_win_height;
    if (gui_util_str.show_config_window && app_config_struct.mobile_mode) {
        soft_keyb_management();
    }
}

void manage_key_pressed_config() {
    static ostringstream oss;
    static int last_char = 0;
    static bool last_char_released = true;

    last_char_released = last_char == 0 || !ImGui::IsKeyDown(last_char);
    if (last_char_released && last_char != 0) {
        last_char = 0;
    }

    int pressed = get_key_pressed();
    if (pressed == TAB_KEY_INDEX) {
        if (pressed != 0) {
            switch (pressed) {
                case TAB_KEY_INDEX:
                    // Tab
                    gui_util_str.configuration_act_tab_index++;
                    if (gui_util_str.configuration_act_tab_index > gui_util_str.num_configs_tabs - 1) {
                        gui_util_str.configuration_act_tab_index = 0;
                    }
                    gui_util_str.configuration_change_tab_index = true;
                    break;
            }
        }
    }
}
