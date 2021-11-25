#include "main.h"

extern gui_util_struct_t gui_util_str;
extern Popup *popup;
extern long loading_millis;

#define TAB_ID_STRING "##images_tabs"

void request_for_scroll(int mode) {
    gui_util_str.request_for_update_list_view = true;
    if (mode == RELOAD_ROMS_AND_RECOMPILE_MODE) {
        Log(LOG_INFO) << "Request for reload roms, cache size: " << gui_util_str.reference_window_width << "x" << gui_util_str.reference_window_height;
        gui_util_str.reload_roms_on_refresh = true;
    } else if (mode == RECOMPILE_MODE) {
        Log(LOG_INFO) << "Request for list dirty, cache size: " << gui_util_str.reference_window_width << "x" << gui_util_str.reference_window_height;
        gui_util_str.force_list_to_dirty = true;
    }
}

// Mobile extreme case
void request_for_recompilation_on_change_size_while_dirty() {
    gui_util_str.changed_size_while_cleaning = true;
    gui_util_str.reference_window_width = gui_util_str.act_window_width;
    gui_util_str.reference_window_height = gui_util_str.act_window_height;
    Log(LOG_INFO) << "New reference size: " << gui_util_str.reference_window_width << "x" << gui_util_str.reference_window_height;
    request_for_scroll(RECOMPILE_MODE);
}

float get_position_at_index(int index) {
    if (roms_list_struct.total_roms_num == 0) {
        return 0;
    } else if (gui_util_str.positions->empty()) {
        // Orientation or size change, recompute all..
        Log(LOG_INFO) << "Quick change size..resynch..";
        request_for_recompilation_on_change_size_while_dirty();
        return 1;
    } else {
        return (*gui_util_str.positions)[index];
    }
}

static int get_index_at_scroll_position(float index) {
    if (index < 0) {
        index = 0;
    }
    int res = roms_list_struct.total_roms_num - 1;
    for (int i = 0; i < roms_list_struct.total_roms_num; i++) {
        if (i < gui_util_str.positions->size()) {
            if (get_position_at_index(i) >= index) {
                res = get_position_at_index(i) == index ? i : i - 1;
                break;
            }
        }
    }
    return res;
}

static bool is_selected_game_visible(float scrollbar_pos, int ref_index) {
    if (roms_list_struct.total_roms_num > 0) {
        float max = scrollbar_pos + gui_util_str.roms_child_size.y;
        float act_pos = get_position_at_index(ref_index);
        float act_next_pos = get_position_at_index(ref_index + 1);
        if (act_next_pos >= max || act_pos + 5 < scrollbar_pos) {
            return false;
        }
    }
    return true;
}

static int get_num_jump_for_page_up_or_down(bool down) {
    int num_jump;
    if (!is_selected_game_visible(gui_util_str.roms_list_scrollable.window->Scroll.y, gui_util_str.rom_index_selected)) {
        num_jump = (down ? gui_util_str.roms_per_page : -gui_util_str.roms_per_page);
    } else {
        float act_index_pos = get_position_at_index(gui_util_str.rom_index_selected);
        float next_index_pos = get_position_at_index(gui_util_str.rom_index_selected + 1);
        float diff = (next_index_pos - act_index_pos) / 2;
        int index = get_index_at_scroll_position(act_index_pos + diff + (down ? gui_util_str.roms_child_size.y : -gui_util_str.roms_child_size.y));
        if (index < 0) {
            index = 0;
        } else if (index > roms_list_struct.total_roms_num - 1) {
            index = roms_list_struct.total_roms_num - 1;
        }
        num_jump = index - gui_util_str.rom_index_selected;
    }
    return num_jump;
}

static float get_scroll_position_for_key_pressed(bool down) {
    float res;
    float act_pos = get_position_at_index(gui_util_str.rom_index_selected);
    if (down) {
        float act_next_pos = get_position_at_index(gui_util_str.rom_index_selected + 1);
        res = act_next_pos + ((act_next_pos - act_pos) / 2) - gui_util_str.roms_child_size.y;
    } else {
        res = act_pos - 5;
    }
    return res;
}

static bool custom_is_dragging() {
    // Must be after manage_window_size_changes
    int x, y;

    // To be sure drag is over
    return SDL_GetGlobalMouseState(&x, &y) != 0;
}

void update_positions_ptr() {
    // We cannot update positions before! That's because:
    // In "manage_window_size_changes", we check for window size change. In linux, size change event come out even while
    // we're dragging, but since there would be toooo much events, we want to check size change only when we release mouse.
    // So when it happens, we calculate the center rom index.. but we need to do this using OLD positions, otherwise if our event
    // is generated from an orientation change, we would compute center rom index with wrong positions.
    // This implies that we change positions here, and of course we must be sure we're not dragging mouse, otherwise we would
    // update positions related new orientation, and when we release mouse, manage_window_size_changes would use again wrong positions.
    if (!custom_is_dragging()) {
        if (gui_util_str.portrait) {
            gui_util_str.positions = &(gui_util_str.portrait_positions_vec);
            gui_util_str.landscape_positions_vec.clear();
        } else {
            gui_util_str.positions = &(gui_util_str.landscape_positions_vec);
            gui_util_str.portrait_positions_vec.clear();
        }
    }
}

static bool list_is_dirty() {
    return gui_util_str.request_for_update_list_view || gui_util_str.list_is_dirty;
}

long start_millis = 0;
void search_by_name(bool *change_rom, int *num_jump, int *last_char, bool *last_char_released) {
    static ostringstream oss;

    ImGuiIO &io = ImGui::GetIO();
    ImWchar c = io.InputQueueCharacters[0];
    int act_char;
    for (int i = 0; i < IM_ARRAYSIZE(io.KeysDown); i++) {
        SDL_Keycode x = SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(i));
        if ((int) c == (int) x) {
            act_char = i;
            break;
        }
    }
    bool concat = true;
    if (act_char == *last_char && !(*last_char_released)) {
        concat = false;
    }
    if (concat) {
        long act_millis = get_act_millis();
        if (act_millis - start_millis > 300) {
            oss.str("");
            oss.clear();
        }
        *last_char = act_char;
        char ch = (c > ' ' && c <= 255) ? (char) c : '?';
        if (roms_list_struct.total_roms_num > 0 && isalnum(ch)) {
            oss << ch;
        }
        start_millis = act_millis;
    }

    string data = oss.str();
    std::transform(data.begin(), data.end(), data.begin(), [](unsigned char c) { return std::tolower(c); });
    const char *search_string = data.c_str();
    int len = strlen(search_string);
    if (search_string && len > 0) {
        int index = -1;
        bool forward = false;
        for (int ii = gui_util_str.rom_index_selected + 1; ii < roms_list_struct.total_roms_num; ii++) {
            string game_name_string = roms_list_struct.list[ii].game_name;
            std::transform(game_name_string.begin(), game_name_string.end(), game_name_string.begin(), [](unsigned char c) { return std::tolower(c); });
            const char *game_name_const = game_name_string.c_str();
            if (!strncmp(game_name_const, search_string, len)) {
                index = ii - gui_util_str.rom_index_selected;
                forward = true;
                break;
            }
        }
        if (index == -1) {
            for (int ii = 0; ii < gui_util_str.rom_index_selected; ii++) {
                string game_name_string = roms_list_struct.list[ii].game_name;
                std::transform(game_name_string.begin(), game_name_string.end(), game_name_string.begin(), [](unsigned char c) { return std::tolower(c); });
                const char *game_name_const = game_name_string.c_str();
                if (!strncmp(game_name_const, search_string, len)) {
                    index = gui_util_str.rom_index_selected - ii;
                    forward = false;
                    break;
                }
            }
        }
        if (index != -1) {
            *change_rom = true;
            *num_jump = forward ? index : -index;
        }
    }
}

static void send_linux_fake_event() {
// In Linux, first mouseButtonDown is ignored (why?!) This fixes the problem..
    SDL_Event new_event;
    new_event.type = SDL_MOUSEBUTTONUP;
    SDL_PushEvent(&new_event);
    SDL_Delay(100);
}

void manage_key_pressed_main(bool any_popup_visible) {
    static int last_char = 0;
    static bool last_char_released = true;
    last_char_released = last_char == 0 || !ImGui::IsKeyDown(last_char);
    if (last_char_released && last_char != 0) {
        last_char = 0;
    }

    if (!list_is_dirty() && gui_events.empty()) {
        int rom_index_sel = gui_util_str.rom_index_selected;
        bool change_rom = false;
        int num_jump = 0;
        bool scroll_before = false;
        int pressed = get_key_pressed();
        if (pressed == TAB_KEY_INDEX || roms_list_struct.total_roms_num > 0) {
            if (pressed != 0) {
                if (!any_popup_visible) {
                    switch (pressed) {
                        case RIGHT_KEY_INDEX:
                            // Right
                            change_rom = true;
                            num_jump = app_config_struct.num_roms_jump;
                            break;
                        case LEFT_KEY_INDEX:
                            // Left
                            change_rom = true;
                            num_jump = -app_config_struct.num_roms_jump;
                            break;
                        case DOWN_KEY_INDEX:
                            // Down
                            change_rom = true;
                            num_jump = +1;
                            break;
                        case UP_KEY_INDEX:
                            // Up
                            change_rom = true;
                            num_jump = -1;
                            break;
                        case ENTER_KEY_INDEX:
                            // Enter
                            if (roms_list_struct.list[rom_index_sel].available_status == ROM_AVAILABLE_STATUS_NOT_FOUND) {
                                ADD_POPUP("Game not found", "Unable to start, game is not in roms path");
                            } else {
                                send_linux_fake_event();
                                submit_gui_event(PREPARE_FOR_LAUNCH_GAME_EVENT, rom_index_sel, 0);
                            }
                            break;
                        case TAB_KEY_INDEX:
                            // Tab
//                            if (gui_util_str.show_config_window) {
//                                gui_util_str.configuration_act_tab_index++;
//                                if (gui_util_str.configuration_act_tab_index > gui_util_str.num_configs_tabs - 1) {
//                                    gui_util_str.configuration_act_tab_index = 0;
//                                }
//                                gui_util_str.configuration_change_tab_index = true;
//                            } else {
                            gui_util_str.act_tab_index = 1 - gui_util_str.act_tab_index;
                            gui_util_str.change_tab_index = true;
//                            }
                            break;
                        case PGDOWN_KEY_INDEX:
                            // PgDown
                            change_rom = true;
                            num_jump = get_num_jump_for_page_up_or_down(true);
                            scroll_before = true;
                            break;
                        case PGUP_KEY_INDEX:
                            // PgUp
                            change_rom = true;
                            num_jump = get_num_jump_for_page_up_or_down(false);
                            scroll_before = true;
                            break;
                        case 9:
                            search_by_name(&change_rom, &num_jump, &last_char, &last_char_released);
                            // Search by name
                            break;
                        default:
                            break;
                    }
                    if (change_rom) {
                        gui_util_str.rom_index_selected += num_jump;
                        if ((gui_util_str.rom_index_selected) < 0) {
                            (gui_util_str.rom_index_selected) = 0;
                        } else if ((gui_util_str.rom_index_selected) > roms_list_struct.total_roms_num - 1) {
                            (gui_util_str.rom_index_selected) = roms_list_struct.total_roms_num - 1;
                        }
                        app_config_struct.last_crc32 = roms_list_struct.list[gui_util_str.rom_index_selected].crc32;
                        selected_rom = &(roms_list_struct.list[gui_util_str.rom_index_selected]);

                        // We could automatically scroll here (= call force_scroll), but we launch event just to avoid increment of callers
                        float next_scroll = -1;
                        if (!scroll_before) {
                            // Up, down, left or right pressed
                            if (!is_selected_game_visible(gui_util_str.roms_list_scrollable.window->Scroll.y, gui_util_str.rom_index_selected)) {
                                next_scroll = get_scroll_position_for_key_pressed(num_jump > 0);
                            }
                        } else {
                            // Page down or page up pressed
                            float act_scroll_pos = gui_util_str.roms_list_scrollable.window->Scroll.y;
                            next_scroll = act_scroll_pos + (num_jump > 0 ? gui_util_str.roms_child_size.y : -gui_util_str.roms_child_size.y);
                            if (!is_selected_game_visible(next_scroll, gui_util_str.rom_index_selected)) {
                                next_scroll = get_scroll_position_for_key_pressed(num_jump > 0);
                            }
                        }
                        if (next_scroll != -1) {
                            submit_gui_event(SCROLL_EVENT, GO_EXACTLY_AT_THIS_POSITION, next_scroll);
                        }
                    }
                } else {
                    // Popup
                    if (pressed == ENTER_KEY_INDEX) {
                        // Enter
                        submit_gui_event(CLOSE_POPUP_EVENT, 0, 0);
                    }
                }
            }
        }
    }
}

static void get_center_rom_index(int *dest_rom_index, float *dest_rom_scroll_offset) {
    gui_util_str.reference_window_width = gui_util_str.act_window_width;
    gui_util_str.reference_window_height = gui_util_str.act_window_height;
    float act_scrollbar_pos = gui_util_str.roms_list_scrollable.window->Scroll.y;
    float middle = act_scrollbar_pos + (gui_util_str.roms_child_size.y / 2);
    int res_int = get_index_at_scroll_position(middle);
    float diff = 1;
    float max = 1;
    *dest_rom_index = res_int;
    if (res_int >= 0) {
        diff = middle - get_position_at_index(res_int);
        max = get_position_at_index(res_int + 1) - get_position_at_index(res_int);
    }
    *dest_rom_scroll_offset = diff / max;
    Log(LOG_INFO) << "Computed center for size " << app_config_struct.window_width << "x" << app_config_struct.window_height << " ---> " << *dest_rom_index;
}

static void force_scroll(int rom_index, float scrollOffset) {
    ImGuiWindow *window = gui_util_str.roms_list_scrollable.window;
    float dest;
    int index = rom_index;
    if (index == GO_EXACTLY_AT_THIS_POSITION) {
        Log(LOG_INFO) << "Scrolling to exact position:" << scrollOffset;
        dest = scrollOffset;
    } else {
        if (index == USE_CURRENT_ROM_INDEX) {
            Log(LOG_INFO) << "Scrolling keeping centered index " << USE_CURRENT_ROM_INDEX << " (centering selected rom)";
            index = gui_util_str.rom_index_selected;
            // On android, at startup there's the glitch due to the fact that resolution changes
            // (from the one with nav/status bars to the other one without nav/status bars).
            // So a double scroll is triggered.
            // We set here as center the selected rom index, that will be used in the second scroll to
            // have the correct rom selected
            gui_util_str.par_int = index;
            gui_util_str.par_float = 0.5;
            scrollOffset = 0.5;
        } else {
            Log(LOG_INFO) << "Scrolling keeping centered index " << index;
        }
        float middle = (gui_util_str.roms_child_size.y / 2);
        float pos = get_position_at_index(index);
        float wanted_scrollbar_pos = pos - middle;
        float max = get_position_at_index(index + 1) - get_position_at_index(index);
        float diff = max * scrollOffset;
        dest = wanted_scrollbar_pos + diff;
    }
    window->ScrollTargetCenterRatio.y = 0.0f;
    window->ScrollTarget.y = dest;
}

static void update_clean_data() {
    app_config_struct.window_width = gui_util_str.act_window_width;
    app_config_struct.window_height = gui_util_str.act_window_height;
    gui_util_str.last_splitter_size_left = gui_util_str.splitter_size_left;
}

static bool manage_window_size_changes() {
    bool mouse_released = false;
    bool res = false;

    if (app_config_struct.window_width != gui_util_str.act_window_width ||
        app_config_struct.window_height != gui_util_str.act_window_height) {

        mouse_released = !custom_is_dragging();
    }

    if (mouse_released) {
        // Window size changed and we're no more dragging window: we need to update list!

        if (app_config_struct.mobile_mode) {
            gui_util_str.roms_list_scrollable.stop_scroll();
        }

        request_for_scroll(RECOMPILE_MODE);
        res = true;
    }
    return res;
}

static void set_pixel_percentage(int whole, float val_float, double *perc) {
    float whole_float = (float) whole;
    float ratio = 100 * (val_float / whole_float);
    *perc = ratio;
}

static bool manage_left_right_splitter_changes() {
    bool res = false;
    if (gui_util_str.last_splitter_size_left != gui_util_str.splitter_size_left) {
        bool mouse_released = !ImGui::IsMouseDown(0);
        if (!mouse_released) {
            // Temporary position
            set_pixel_percentage(gui_util_str.main_window_size.x, gui_util_str.splitter_size_left, &(app_config_struct.roms_list_width_percentage));
            if (app_config_struct.roms_list_width_percentage > 80) {
                app_config_struct.roms_list_width_percentage = 80;
            } else if (app_config_struct.roms_list_width_percentage < 20) {
                app_config_struct.roms_list_width_percentage = 20;
            }
        } else {
            // Definitive position
            // Left/right splitter: we need to recalculate positions
            set_pixel_percentage(gui_util_str.main_window_size.x, gui_util_str.splitter_size_left, &(app_config_struct.roms_list_width_percentage));
            Log(LOG_INFO) << "Left-Right Splitter moved";
            request_for_scroll(RECOMPILE_MODE);
            res = true;
        }
    }
    return res;
}

static void manage_up_down_splitter_changes() {
    if (gui_util_str.last_splitter_size_up == -1) {
        gui_util_str.last_splitter_size_up = gui_util_str.splitter_size_up;
    }
    if (gui_util_str.last_splitter_size_up != gui_util_str.splitter_size_up) {
        bool mouse_released = !ImGui::IsMouseDown(0);

        if (!mouse_released) {
            // Temporary position
            set_pixel_percentage(gui_util_str.main_window_size.y, gui_util_str.splitter_size_up, &(app_config_struct.image_height_percentage));
            if (app_config_struct.image_height_percentage > 80) {
                app_config_struct.image_height_percentage = 80;
            } else if (app_config_struct.image_height_percentage < 20) {
                app_config_struct.image_height_percentage = 20;
            }
        } else {
            // Definitive position
            set_pixel_percentage(gui_util_str.main_window_size.y, gui_util_str.splitter_size_up, &(app_config_struct.image_height_percentage));
            // Up/Down splitter: we don't need to recalculate positions
            Log(LOG_INFO) << "Up-Down Splitter moved";
            gui_util_str.update_graphic_child = true;
            gui_util_str.last_splitter_size_up = gui_util_str.splitter_size_up;
        }
    }
}

static void compute_roms_per_page() {
    gui_util_str.roms_per_page = 0;
    if (roms_list_struct.total_roms_num > 0) {
        float act_scrollbar_pos = gui_util_str.roms_list_scrollable.window->Scroll.y;
        int first = get_index_at_scroll_position(act_scrollbar_pos);
        float max = act_scrollbar_pos + (gui_util_str.roms_child_size.y);
        int last = get_index_at_scroll_position(max);

        int roms_per_page = 0;
        for (int i = first; i < last; i++) {
            roms_per_page++;
            if (roms_list_struct.list[i].double_row) {
                roms_per_page++;
            }
        }
        gui_util_str.roms_per_page = roms_per_page;
    }
}

static void check_for_clean_status() {
    // Dear Imgui compute wrongly scrollbar position..we need to fill the list a couple of times in order to have correct values
    float max_scroll = gui_util_str.roms_list_scrollable.window->ScrollMax.y;
    bool clean = false;
    if (gui_util_str.portrait) {
        int diff = max_scroll - gui_util_str.portrait_max_scrollbar_pos;
        if (!diff) {
            clean = true;
//            gui_util_str.mobile_portrait_positions = gui_util_str.positions_ptr;
        } else {
            gui_util_str.portrait_max_scrollbar_pos = max_scroll;
        }
    } else {
        int diff = max_scroll - gui_util_str.landscape_max_scrollbar_pos;
        if (!diff) {
            clean = true;
//            gui_util_str.mobile_landscape_positions = gui_util_str.positions_ptr;
        } else {
            gui_util_str.landscape_max_scrollbar_pos = max_scroll;
        }
    }

    if (clean) {
        if (gui_util_str.reference_window_width != gui_util_str.act_window_width ||
            gui_util_str.reference_window_height != gui_util_str.act_window_height) {
            Log(LOG_INFO) << "Size changed while list dirty";
            request_for_recompilation_on_change_size_while_dirty();
        } else {
            Log(LOG_INFO) << "List clean";
            if (!custom_is_dragging()) {
                update_clean_data();
            }
            gui_util_str.list_is_dirty = false;
            gui_util_str.show_loading = false;
            gui_util_str.force_list_to_dirty = false;

            compute_roms_per_page();
        }
    }
}

static bool show_popup(Popup *p, int act_window_width, int act_window_height) {
    bool res = false;

    string ok_button_message_string = "OK";
    const char *ok_button_message = ok_button_message_string.c_str();

    ImVec2 window_size_vec;
    window_size_vec.x = act_window_width / 2;
    window_size_vec.y = act_window_height / 3;
    ImGui::SetNextWindowPos(ImVec2(act_window_width / 4, act_window_height / 3));
    ImGui::SetNextWindowSize(window_size_vec, ImGuiCond_Always);
    ImGui::OpenPopup(p->title);
    ImGuiWindowFlags flags = get_window_flags(true, true, true, false);
    flags |= ImGuiWindowFlags_Popup;
    flags |= ImGuiWindowFlags_Modal;
    ImGui::Begin(p->title, nullptr, flags);

    // Messages
    ImGui::BeginChild("messagesPopup", ImVec2{0, window_size_vec.y - app_config_struct.buttons_size - 3 * gui_util_str.border.y});
    std::set<std::string>::iterator messages_it = p->messages.begin();
    while (messages_it != p->messages.end()) {
        ImGui::TextWrapped("%s", (*messages_it).c_str());
        messages_it++;
    }
    ImGui::EndChild();

    ImGui::Separator();

    // Ok Button
    ImGui::BeginChild("okPopup", ImVec2{0, 0});
    Push_buttons_size(true);
    const ImVec2 &ok_button_message_size = ImGui::CalcTextSize(ok_button_message, nullptr, true);
    ImGui::SetCursorPos(ImVec2((ImGui::GetCurrentWindow()->Size.x - ok_button_message_size.x) / 2, ImGui::GetCurrentWindowRead()->Size.y - app_config_struct.buttons_size));
    if (ImGui::Button("OK")) {
        res = true;
    }
    Pop_buttons_size();
    ImGui::EndChild();
    ImGui::SetItemDefaultFocus();
    ImGui::End();
    return res;
}

static void manage_popup() {
    if (popup != nullptr) {
        if (show_popup(popup, gui_util_str.act_window_width, gui_util_str.act_window_height)) {
            if (app_config_struct.mobile_mode) {
                gui_util_str.force_unhover = true;
            }
            submit_gui_event(CLOSE_POPUP_EVENT, 0, 0);
        }
    }
}

static bool Splitter(bool split_vertically, float thickness, float *size1, float *size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f, const char *ide = "") {
    using namespace ImGui;
    ImGuiContext &g = *GImGui;
    ImGuiWindow *window = g.CurrentWindow;
    ImGuiID id = window->GetID(ide);
    ImRect bb;
    bb.Min.x = window->DC.CursorPos.x + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1)).x;
    bb.Min.y = window->DC.CursorPos.y + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1)).y;
    bb.Max.x = bb.Min.x + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f).x;
    bb.Max.y = bb.Min.y + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f).y;
    return SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
}

static void draw_left_right_splitter() {
    if (list_is_dirty()) {
        ImVec2 size = gui_util_str.main_window_size;
        if (app_config_struct.mobile_mode && gui_util_str.portrait) {
            gui_util_str.splitter_size_left = gui_util_str.act_window_width;
            gui_util_str.splitter_size_right = gui_util_str.act_window_width;
            gui_util_str.splitter_left_right_percent_in_pixels = gui_util_str.act_window_width / 2;
        } else {
            int splitter_size_x = get_pixel_percentage(size.x, app_config_struct.roms_list_width_percentage);
            gui_util_str.splitter_size_left = splitter_size_x;
            gui_util_str.splitter_size_right = size.x - splitter_size_x;
            gui_util_str.splitter_left_right_percent_in_pixels = ((int) 20 * size.x / 100) + MY_FAVOURITE_BORDER;
        }
    }
    Splitter(true, gui_util_str.splitter_tickness, &(gui_util_str.splitter_size_left), &gui_util_str.splitter_size_right, gui_util_str.splitter_left_right_percent_in_pixels, gui_util_str.splitter_left_right_percent_in_pixels, -1, "##Splitter_left_right");
    if (list_is_dirty()) {
        ImGuiContext &g = *GImGui;
        ImGuiWindow *window = g.CurrentWindow;
        ImRect rect = window->DC.LastItemRect;
        ImVec2 pos = ImGui::GetCursorPos();
        rect.Min.x -= pos.x;
        rect.Min.x -= gui_util_str.main_window_pos.x;
        rect.Max.x -= pos.x;
        rect.Max.x -= gui_util_str.main_window_pos.x;
        gui_util_str.left_right_splitter_rect = rect;
    }
}

static void draw_up_down_splitter() {
    if (list_is_dirty() || gui_util_str.update_graphic_child) {
        ImVec2 size = gui_util_str.main_window_size;
        int splitter_size_y = get_pixel_percentage(size.y, app_config_struct.image_height_percentage);
        gui_util_str.splitter_size_up = splitter_size_y;
        gui_util_str.splitter_size_down = size.y - splitter_size_y;
        gui_util_str.splitter_up_down_percent_in_pixels = ((int) 20 * size.y / 100) + MY_FAVOURITE_BORDER;
    }
    Splitter(false, gui_util_str.splitter_tickness, &gui_util_str.splitter_size_up, &gui_util_str.splitter_size_down, gui_util_str.splitter_up_down_percent_in_pixels, gui_util_str.splitter_up_down_percent_in_pixels, -1, "##Splitter_up_down");
    if (list_is_dirty() || gui_util_str.update_graphic_child) {
        ImGuiContext &g = *GImGui;
        ImGuiWindow *window = g.CurrentWindow;
        ImRect rect = window->DC.LastItemRect;
        ImVec2 pos = ImGui::GetCursorPos();
        rect.Min.y -= pos.y;
        rect.Min.y -= gui_util_str.main_window_pos.y;
        rect.Max.y -= pos.y;
        rect.Max.y -= gui_util_str.main_window_pos.y;
        gui_util_str.up_down_splitter_rect = rect;
    }
}

static bool create_next_window(ImVec2 pos, ImVec2 window_size_vec, bool no_title_bar, bool no_scroll_bar, bool no_scroll_with_mouse, bool no_focus, const char *title) {
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(window_size_vec, ImGuiCond_Always);
    ImGuiWindowFlags window_flags = get_window_flags(no_title_bar, no_scroll_bar, no_scroll_with_mouse, no_focus);

    if (!ImGui::Begin(title, nullptr, window_flags)) {
        // Early out if the window is collapsed, as an optimization.
        ImGui::End();
        return false;
    }
    return true;
}

static void draw_left_child() {
    float selectable_height;
    ImGuiSelectableFlags mobile_selectable_flags = ImGuiSelectableFlags_SelectOnClick | ImGuiSelectableFlags_AllowDoubleClick;

    ImVec2 pos_vec2 = {0, 0};

    if (list_is_dirty()) {
        int width = gui_util_str.splitter_size_left - MY_FAVOURITE_BORDER + gui_util_str.border.x;
        int height = gui_util_str.main_window_size.y;
        gui_util_str.left_child_size.x = width;
        gui_util_str.left_child_size.y = height;

        ImGui::SetCursorPos(pos_vec2);
        ImGui::BeginChild("left_child", gui_util_str.left_child_size, false);
        ImGuiWindow *win = ImGui::GetCurrentWindowRead();
        gui_util_str.left_child_pos.x = win->Pos.x;
        gui_util_str.left_child_pos.y = win->Pos.y;
    } else {
        ImGui::SetCursorPos(pos_vec2);
        ImGui::BeginChild("left_child", gui_util_str.left_child_size, false);
    }

    create_next_window(gui_util_str.left_child_pos, gui_util_str.left_child_size, false, true, true, false, "Roms");
    const ImVec2 &title_bar_size = ImGui::GetCursorPos();
    if (list_is_dirty() || gui_util_str.update_graphic_child) {
        int width = gui_util_str.left_child_size.x - (2 * (float) gui_util_str.border.x);
        int height = gui_util_str.left_child_size.y - title_bar_size.y - app_config_struct.buttons_size - gui_util_str.border.y - 2 * MY_FAVOURITE_BORDER;
        gui_util_str.roms_child_size.x = width;
        gui_util_str.roms_child_size.y = height;
        gui_util_str.title_bar_size = title_bar_size;
    }

    /////////////// Roms child  ////////////////////
    ImGui::BeginChild("roms_child", gui_util_str.roms_child_size, true);
    if (gui_util_str.border.x == -1) {
        ImVec2 border = ImGui::GetCursorPos();
        gui_util_str.border = border;
    }
    if (gui_util_str.roms_list_scrollable.window == nullptr) {
        gui_util_str.roms_list_scrollable.window = ImGui::GetCurrentWindowRead();
    }

    if (roms_list_struct.total_roms_num > 0) {
        if (list_is_dirty()) {
            gui_util_str.positions->clear();
            gui_util_str.rom_index_selected = -1;
            selectable_height = round(gui_util_str.roms_list_scrollable.window->Size.y / 10);
            if (selectable_height < font_size * 2 + MY_FAVOURITE_BORDER) {
                selectable_height = font_size * 2 + MY_FAVOURITE_BORDER;
            }
            gui_util_str.selectable_imvec.x = gui_util_str.roms_list_scrollable.window->Size.x;
            gui_util_str.selectable_imvec.y = selectable_height;

            gui_util_str.roms_names.clear();
            for (int i = 0; i < roms_list_struct.total_roms_num; i++) {
                char *key_game_name = wrap_string_by_size(roms_list_struct.list[i].game_name,
                                                          gui_util_str.left_child_size.x -
                                                          app_config_struct.scrollbar_size -
                                                          2 * gui_util_str.border.x -
                                                          4 * MY_FAVOURITE_BORDER);
                char key[100];
                sprintf(key, "%s##%d", key_game_name, roms_list_struct.list[i].crc32);
                free(key_game_name);
                gui_util_str.roms_names.emplace_back(key);
                if (strstr(key, "\n") != nullptr) {
                    roms_list_struct.list[i].double_row = true;
                } else {
                    roms_list_struct.list[i].double_row = false;
                }

                if (roms_list_struct.list[i].crc32 == app_config_struct.last_crc32) {
                    gui_util_str.rom_index_selected = i;
                }
            }
            if (gui_util_str.rom_index_selected == -1) {
                gui_util_str.rom_index_selected = 0;
                app_config_struct.last_crc32 = roms_list_struct.list[gui_util_str.rom_index_selected].crc32;
            }
            selected_rom = &(roms_list_struct.list[gui_util_str.rom_index_selected]);
        }

        for (int i = 0; i < roms_list_struct.total_roms_num; i++) {
            if (roms_list_struct.list[i].available_status == ROM_AVAILABLE_STATUS_FOUND) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            } else if (roms_list_struct.list[i].available_status == ROM_AVAILABLE_STATUS_NOT_FOUND) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
            }

            // While scrolling (mobile mode), don't highlight
            if (!gui_util_str.force_unhover) {
                gui_util_str.force_unhover = (gui_util_str.roms_list_scrollable.is_dragging ||
                                              gui_util_str.roms_list_scrollable.is_auto_scrolling);
            }
            bool selectable_res = false;
            if (!gui_util_str.roms_names.empty()) {
                const char *label = gui_util_str.roms_names[i].c_str();
                if (app_config_struct.mobile_mode) {
                    bool stop_highlight = (gui_util_str.roms_list_scrollable.is_dragging ||
                                           gui_util_str.roms_list_scrollable.is_auto_scrolling);
                    selectable_res = ImGui::Selectable(label,
                                                       // While scrolling, don't highlight selected
                                                       !stop_highlight && gui_util_str.rom_index_selected == i,
                                                       mobile_selectable_flags,
                                                       gui_util_str.selectable_imvec,
                                                       // Don't highlight mouse position
                                                       true
                                                       );
                } else {
                    selectable_res = ImGui::Selectable(label,
                                                       gui_util_str.rom_index_selected == i,
                                                       ImGuiSelectableFlags_SelectOnClick | ImGuiSelectableFlags_AllowDoubleClick);
                }
            }
            if (selectable_res) {
                bool launch_game = (gui_util_str.portrait && app_config_struct.mobile_mode)
                                   ? ImGui::IsMouseReleased(0) : ImGui::IsMouseDoubleClicked(0);

                launch_game &= !gui_util_str.force_unhover; // Prevent selection on mobile click while scrolling

                gui_util_str.rom_index_selected = i;
                selected_rom = &(roms_list_struct.list[gui_util_str.rom_index_selected]);
                app_config_struct.last_crc32 = selected_rom->crc32;
                gui_util_str.force_unhover = false;

                // A double click must be done in the exact mouse position for both clicks.
                // This is quiet impossible in mobile mode.
                // So, in mobile mode (only landscape) we force a double click when second click is done
                // in the same item of first click, even if it's not in the same exact mouse position
                if (!launch_game &&
                    ImGui::IsItemHovered() &&
                    app_config_struct.mobile_mode &&
                    !gui_util_str.portrait &&
                    ImGui::IsMouseClicked(0)) {
                    long millis = get_act_millis();
                    if (gui_util_str.rom_index_selected == gui_util_str.last_rom_index_selected) {
                        if (millis - gui_util_str.last_rom_index_millis < 500) {
                            launch_game = true;
                        }
                    }
                    gui_util_str.last_rom_index_selected = gui_util_str.rom_index_selected;
                    gui_util_str.last_rom_index_millis = millis;
                }

                if (launch_game && ImGui::IsItemHovered()) {
                    // OK, really start game
                    int execbinstatus = selected_rom->use_tutorvision_exec? roms_list_struct.tutorvisionExecBinStatus :roms_list_struct.execBinStatus;
                    int grombinstatus = selected_rom->use_tutorvision_grom? roms_list_struct.tutorvisionGromBinStatus :roms_list_struct.gromBinStatus;
                    if (execbinstatus != 0 && grombinstatus != 0) {
                        if (selected_rom->available_status == ROM_AVAILABLE_STATUS_NOT_FOUND) {
                            ADD_POPUP("Game not found", "Unable to start, game is not in roms path");
                        } else {
                            send_linux_fake_event();
                            submit_gui_event(PREPARE_FOR_LAUNCH_GAME_EVENT, gui_util_str.rom_index_selected, 0);
                        }
                    } else {
                        ADD_POPUP("Bios not found", "Unable to start, at least one bios not found in roms path");
                    }
                }
            }
            if (list_is_dirty()) {
                float pos = gui_util_str.roms_list_scrollable.window->DC.CursorPosPrevLine.y -
                            gui_util_str.roms_list_scrollable.window->DC.CursorStartPos.y;
                gui_util_str.positions->push_back(pos);
            }

            ImGui::Separator();
            ImGui::PopStyleColor();
        }

        if (list_is_dirty()) {
            // Position of (n+1)th element
            float pos = gui_util_str.roms_list_scrollable.window->DC.CursorPos.y -
                        gui_util_str.roms_list_scrollable.window->DC.CursorStartPos.y;
            gui_util_str.positions->push_back(pos);
        }
    } else {
        ImGui::Text("No games found. \nPlease select roms folder.");
    }

    if (app_config_struct.mobile_mode) {
        gui_util_str.roms_list_scrollable.scroll();
    }

    ImGui::EndChild();

    //////////  Little offset   /////////
    const ImVec2 &old_pos = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(old_pos.x, old_pos.y + MY_FAVOURITE_BORDER));

    //////////  Config window   /////////
    Push_buttons_size(true);

    if (ImGui::Button("Options")) {
        // This will be used on exit config window, to scroll in the correct position
        get_center_rom_index(&gui_util_str.par_int, &gui_util_str.par_float);
        gui_util_str.show_config_window = true;
        gui_util_str.show_demo_window = false;
        gui_util_str.flip_demo_window = false;
    }

    if (roms_list_struct.total_roms_num > 0 && !list_is_dirty()) {
        float act_scrollbar_pos = gui_util_str.roms_list_scrollable.window->Scroll.y;
        bool res = is_selected_game_visible(act_scrollbar_pos, gui_util_str.rom_index_selected);
        if (!res) {
            ImGui::SameLine();
            if (ImGui::Button("Go to game")) {
                gui_util_str.center_at_rom_index_if_needed = true;
                request_for_scroll(SIMPLE_SCROLL_MODE);
            }
        }
    }

    Pop_buttons_size();

    ImGui::End();
    ImGui::EndChild();
}

// Image
static void draw_right_up_child() {
    ImVec2 pos = {0, 0};

    if (list_is_dirty() || gui_util_str.update_graphic_child) {
        int width = gui_util_str.splitter_size_right - MY_FAVOURITE_BORDER - gui_util_str.border.x - gui_util_str.splitter_tickness;
        int height = gui_util_str.splitter_size_up - MY_FAVOURITE_BORDER;
        gui_util_str.right_up_child_size.x = width;
        gui_util_str.right_up_child_size.y = height;

        ImGui::SetCursorPos(pos);
        ImGui::BeginChild("right_up_child", gui_util_str.right_up_child_size, false);
        ImGuiWindow *win = ImGui::GetCurrentWindowRead();
        gui_util_str.right_up_child_pos.x = win->Pos.x;
        gui_util_str.right_up_child_pos.y = win->Pos.y;
    } else {
        ImGui::SetCursorPos(pos);
        ImGui::BeginChild("right_up_child", gui_util_str.right_up_child_size, false);
    }

    create_next_window(gui_util_str.right_up_child_pos, gui_util_str.right_up_child_size, false, true, true, false, "Image");

    if (roms_list_struct.total_roms_num > 0) {
        int tab_selected = 0;
        ImGuiID id = ImGui::GetCurrentWindow()->GetID(TAB_ID_STRING);
        int tab_size_pixel;
        if (ImGui::BeginTabBar(TAB_ID_STRING, ImGuiTabBarFlags_None)) {
            ImGuiTabBar *tab_bar = (*GImGui).TabBars.GetOrAddByKey(id);
            tab_size_pixel = tab_bar->BarRect.Max.y - tab_bar->BarRect.Min.y;
            if (ImGui::BeginTabItem("Screenshots")) {
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Boxes")) {
                ImGui::EndTabItem();
            }

            ImGuiID id0 = tab_bar->Tabs[0].ID;
            ImGuiID id1 = tab_bar->Tabs[1].ID;
            ImGuiID act_id = tab_bar->SelectedTabId;
            int actual_id = (act_id == id0) ? 0 : (act_id == id1) ? 1 : -1;
            if (actual_id != -1 && gui_util_str.act_tab_index != actual_id && !gui_util_str.change_tab_index) {
                gui_util_str.change_tab_index = true;
                gui_util_str.act_tab_index = actual_id;
            }

            if (gui_util_str.change_tab_index) {
                set_tab_index(gui_util_str.act_tab_index);
                gui_util_str.change_tab_index = false;
            }
            tab_selected = get_tab_index();
            ImGui::EndTabBar();
        }

        ImVec2 siz = gui_util_str.right_up_child_size;
        siz.x -= 2 * gui_util_str.border.x;
        siz.y -= tab_size_pixel;
        siz.y -= gui_util_str.title_bar_size.y;
        siz.y -= gui_util_str.border.y;
        manage_image_window(siz,
                            &roms_list_struct,
                            &app_config_struct,
                            tab_selected,
                            gui_util_str.rom_index_selected,
                            gui_util_str.roms_list_scrollable.window->Scroll.y,
                            &(gui_util_str.roms_list_scrollable),
                            &(gui_util_str.last_roms_checked_millis),
                            &(gui_util_str.last_rom_index_checked)
        );
    }
    ImGui::End();
    ImGui::EndChild();
}

// Description
static void draw_right_down_child() {

    float pos_x = 0;
    float pos_y = gui_util_str.up_down_splitter_rect.Max.y + MY_FAVOURITE_BORDER;
    ImVec2 pos = {pos_x, pos_y};

    if (list_is_dirty() || gui_util_str.update_graphic_child) {
        int width = gui_util_str.splitter_size_right - MY_FAVOURITE_BORDER - gui_util_str.border.x - gui_util_str.splitter_tickness;
        int height = gui_util_str.splitter_size_down - MY_FAVOURITE_BORDER - gui_util_str.splitter_tickness;
        gui_util_str.right_down_child_size.x = width;
        gui_util_str.right_down_child_size.y = height;

        ImGui::SetCursorPos(pos);
        ImGui::BeginChild("right_down_child", gui_util_str.right_down_child_size, false);
        ImGuiWindow *win = ImGui::GetCurrentWindowRead();
        gui_util_str.right_down_child_pos.x = win->Pos.x;
        gui_util_str.right_down_child_pos.y = win->Pos.y;
    } else {
        ImGui::SetCursorPos(pos);
        ImGui::BeginChild("right_down_child", gui_util_str.right_down_child_size, false);
    }

    create_next_window(gui_util_str.right_down_child_pos, gui_util_str.right_down_child_size, false, false, false, false, "Description");

    if (gui_util_str.description_scrollable.window == nullptr) {
        gui_util_str.description_scrollable.window = ImGui::GetCurrentWindow();
    }

    if (roms_list_struct.total_roms_num > 0) {
        ImGui::TextWrapped(
                "%s", is_memory_blank(selected_rom->description)
                      ? "No description available"
                      : selected_rom->description);
    }
    if (app_config_struct.mobile_mode) {
        gui_util_str.description_scrollable.scroll();
    }
    ImGui::End();
    ImGui::EndChild();
}

static void draw_right_child() {
    ImGui::SameLine();

    float pos_x = gui_util_str.left_right_splitter_rect.Max.x + MY_FAVOURITE_BORDER + gui_util_str.border.x;
    float pos_y = 0;
    ImVec2 pos = {pos_x, pos_y};

    if (list_is_dirty() || gui_util_str.update_graphic_child) {
        int width = gui_util_str.splitter_size_right - MY_FAVOURITE_BORDER - gui_util_str.border.x - gui_util_str.splitter_tickness;
        int height = gui_util_str.main_window_size.y;
        gui_util_str.right_child_size.x = width;
        gui_util_str.right_child_size.y = height;

        ImGui::SetCursorPos(pos);
        ImGui::BeginChild("right_child", gui_util_str.right_child_size, false);
    } else {
        ImGui::SetCursorPos(pos);
        ImGui::BeginChild("right_child", gui_util_str.right_child_size, false);
    }

    draw_up_down_splitter();
    draw_right_up_child();   // Image
    draw_right_down_child(); // Description
    ImGui::EndChild();
    gui_util_str.update_graphic_child = false;
}

static void draw_img(bool background) {
    const ImVec2 &old_pos = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(0, 0));
    if (background) {
        draw_background(gui_util_str.main_window_size);
    } else {
        draw_loading(gui_util_str.main_window_size);
    }
    ImGui::SetCursorPos(old_pos);
}

static void draw_window() {
    if (gui_util_str.show_loading && list_is_dirty()) {
        if (loading_millis == -1) {
            loading_millis = get_act_millis();
        }
        draw_img(false);
    } else if (loading_millis != -1) {
        long act_millis = get_act_millis();
        if (act_millis - loading_millis < 250) {
            draw_img(false);
        } else {
            loading_millis = -1;
            gui_util_str.show_loading = false;
        }
    }
    if (list_is_dirty()) {
        gui_util_str.main_window_size.x = gui_util_str.act_window_width;
        gui_util_str.main_window_size.y = gui_util_str.act_window_height;
        gui_util_str.main_window_pos.x = 0;
        gui_util_str.main_window_pos.y = 0;

        int left_gap_pixels = 0;
        int right_gap_pixels = 0;
        int top_gap_pixels = 0;
        int bottom_gap_pixels = 0;
        get_gap_pixels(&left_gap_pixels, &right_gap_pixels, &top_gap_pixels, &bottom_gap_pixels);

        if (app_config_struct.mobile_mode) {
            if (gui_util_str.portrait) {
                gui_util_str.main_window_size.y -= bottom_gap_pixels;
                gui_util_str.main_window_size.y -= top_gap_pixels;
                gui_util_str.main_window_pos.y = top_gap_pixels;
            } else {
                gui_util_str.main_window_size.x -= right_gap_pixels;
                gui_util_str.main_window_size.x -= left_gap_pixels;
                gui_util_str.main_window_pos.x = left_gap_pixels;
            }
        }
    }
    create_next_window(gui_util_str.main_window_pos, gui_util_str.main_window_size, true, true, true, true, "jzIintv");
    draw_img(true);
    draw_left_right_splitter();
    draw_left_child();
    if (!(app_config_struct.mobile_mode && gui_util_str.portrait)) {
        draw_right_child();
    }
    ImGui::End();
}

static bool manage_next_gui_event() {
    bool res = false;
    static long millis;
    GuiEvent *act_gui_event = nullptr;

    if (!gui_events.empty()) {
        act_gui_event = gui_events.front();
        gui_events.erase(gui_events.begin());
    }

    if (act_gui_event != nullptr) {
        res = true;
        int par_int = act_gui_event->param_int;
        float par_float = act_gui_event->param_float;
        bool emulation_ok;
        switch (act_gui_event->event) {
            case SCROLL_EVENT:
                force_scroll(par_int, par_float);
                break;
            case START_GAME_EVENT:
                if ((get_act_millis() - millis) < 10) {
                    submit_gui_event(START_GAME_EVENT, par_int, par_float);
                } else {
                    app_config_struct.starting_game = false;
                    save_config_file();
                    if (app_config_struct.custom_font_loaded) {
                        ImGui::PopFont();
                        app_config_struct.custom_font_loaded = false;
                    }
                    suspend_gui();

                    // Must be resetted otherwise changing resolution by command line (-z2, -z3) would fail between multiple emulations
                    window_x = 0;
                    window_y = 0;
                    if (app_config_struct.mobile_mode) {
                        init_effective_and_config_game_controls(find_roms_config_index(gui_util_str.rom_index_selected));
                    }
                    init_jzintv_screen_references();

                    emulation_ok = start_emulation(gui_util_str.rom_index_selected);
                    gui_util_str.center_at_rom_index_if_needed = true;
                    app_config_struct.act_player = 0;
                    if (app_config_struct.mobile_mode) {
                        clear_effective_and_config_game_controls();
                    }
                    reset_mappings();
                    resume_gui();
                    app_config_struct.ending_game = true;

                    if (!emulation_ok) {
                        gui_util_str.par_int = gui_util_str.rom_index_selected;
                        gui_util_str.par_float = 0;
                        request_for_scroll(RELOAD_ROMS_AND_RECOMPILE_MODE);
                    } else {
                        request_for_scroll(SIMPLE_SCROLL_MODE);
                    }
                    start_millis = 0;
                    emulation_end();
                }
                break;
            case PREPARE_FOR_LAUNCH_GAME_EVENT:
                app_config_struct.starting_game = true;
                get_center_rom_index(&gui_util_str.par_int, &gui_util_str.par_float);
                submit_gui_event(START_GAME_EVENT, par_int, par_float);
                millis = get_act_millis();
                emulation_start();
                SDL_StopTextInput();
                break;
            case CLOSE_POPUP_EVENT:
                free_popup();
                break;
        }
        delete (act_gui_event);
    }
    return res;
}

static bool need_center_rom_index() {
    float act_scrollbar_pos = gui_util_str.roms_list_scrollable.window->Scroll.y;
    bool res = !is_selected_game_visible(act_scrollbar_pos, gui_util_str.rom_index_selected);
    return res;
}

void check_for_update_list() {
    if (!list_is_dirty()) {
        bool sent_request = manage_window_size_changes();
        if (!app_config_struct.mobile_mode || !gui_util_str.portrait) {
            if (!sent_request) {
                sent_request = manage_left_right_splitter_changes();
            }
            if (!sent_request) {
                if (!(app_config_struct.mobile_mode && gui_util_str.portrait)) {
                    manage_up_down_splitter_changes();
                }
            }
        }
        if (sent_request) {
            // List is dirty now
            gui_util_str.reference_window_width = gui_util_str.act_window_width;
            gui_util_str.reference_window_height = gui_util_str.act_window_height;
            Log(LOG_INFO) << "New reference size will be: " << gui_util_str.reference_window_width << "x" << gui_util_str.reference_window_height;
            if (gui_util_str.par_int == -1) {
                if (gui_util_str.positions != nullptr) {
                    get_center_rom_index(&gui_util_str.par_int, &gui_util_str.par_float);
                } else {
                    gui_util_str.par_int = gui_util_str.rom_index_selected;
                    gui_util_str.par_float = 0;
                }
            }
        }
    }

    // Manage list data
    if (gui_util_str.request_for_update_list_view) {
        int parInt;
        float parFloat;
        // Need to reload roms ( = complete refresh: reload roms, update list graphics and scroll)
        // Need to update list graphics, and then scroll
        if (gui_util_str.reload_roms_on_refresh) {
            // Reloading list of roms from fileSystem
            // 1 - At beginning of app
            // 2 - When we switch flag "Hide unavailable games"
            // 3 - When jzIntv emulation fails
            // 4 - When we add a game to DB
            gui_util_str.list_is_dirty = true;
            update_roms_list();
            gui_util_str.show_loading = true;
            gui_util_str.reload_roms_on_refresh = false;
            parInt = USE_CURRENT_ROM_INDEX;
            parFloat = NOT_NEEDED;
        } else if (gui_util_str.force_list_to_dirty) {
            // On window size change
            // On splitter move
            // On text size, on buttons size or on scrollbar size change
            gui_util_str.list_is_dirty = true;
            gui_util_str.force_list_to_dirty = false;
            parInt = gui_util_str.par_int;
            parFloat = gui_util_str.par_float;
        } else {
            // Simple scroll
            parInt = gui_util_str.force_center_at_rom_index ? USE_CURRENT_ROM_INDEX : gui_util_str.par_int;
            parFloat = gui_util_str.force_center_at_rom_index ? NOT_NEEDED : gui_util_str.par_float;
        }

        gui_util_str.force_center_at_rom_index = false;

        if (list_is_dirty()) {
            if (gui_util_str.reload_roms_on_refresh || gui_util_str.portrait) {
                gui_util_str.portrait_max_scrollbar_pos = -1;
//                    gui_util_str.mobile_portrait_scrollbar_pos = -1;
//                    gui_util_str.mobile_portrait_positions = nullptr;
//                    gui_util_str.mobile_portrait_scrollbar_index = -1;
//                    gui_util_str.mobile_portrait_scrollbar_index_offset = -1;
            }
            if (gui_util_str.reload_roms_on_refresh || !gui_util_str.portrait) {
                gui_util_str.landscape_max_scrollbar_pos = -1;
//                    gui_util_str.mobile_landscape_scrollbar_pos = -1;
//                    gui_util_str.mobile_landscape_positions = nullptr;
//                    gui_util_str.mobile_landscape_scrollbar_index = -1;
//                    gui_util_str.mobile_landscape_scrollbar_index_offset = -1;
            }
        }

        // Add scroll event: it will be used when list will be clean again
        submit_gui_event(SCROLL_EVENT, parInt, parFloat);
        gui_util_str.request_for_update_list_view = false;
    }
    gui_util_str.changed_size_while_cleaning = false;
}

void draw_main_window() {
    if (!app_config_struct.starting_game) {
        update_positions_ptr();
        manage_popup(); // Show popup if needed
        draw_window();
    }

    if (list_is_dirty()) {
        if (!gui_util_str.request_for_update_list_view) {
            // This condition can happen when returning from emulation or config window
            check_for_clean_status();
        }
    } else if (!gui_util_str.changed_size_while_cleaning) {
        if (!manage_next_gui_event()) {
            if (gui_util_str.center_at_rom_index_if_needed) {
                if (need_center_rom_index()) {
                    Log(LOG_INFO) << "Selected rom not visible, requesting scroll";
                    gui_util_str.force_center_at_rom_index = true;
                    request_for_scroll(SIMPLE_SCROLL_MODE);
                }
                gui_util_str.center_at_rom_index_if_needed = false;
            } else {
                if (gui_util_str.par_int != -1 && !gui_util_str.show_config_window) {
                    Log(LOG_INFO) << "____________________";
                    gui_util_str.par_int = -1;
                }
            }
        }
    }
}
