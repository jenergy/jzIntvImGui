#include "main.h"
#include "SDL_image.h"

extern bool screen_is_portrait;
int act_map = 0;

// 0:Left 1:Right
vector<Control *> device_default_controls[2];
vector<Control *> delta_default_controls[2];
vector<Control *> effective_game_controls[2];
vector<Control *> configuration_controls;
Control *pause_control;
extern set<string> events_needed;
extern vector<vector<string>> mapping_events;
extern vector<vector<string>> mapping_directions;
extern vector<string> events_needed_vec;
extern vector<string> events_directions_needed_vec;
bool configuration_mode = false;
bool finished_controls_init = false;
int config_index;
bool manage_show_saved = false;
bool manage_show_saved_global = false;
Uint32 saved_start_ticks;
int act_configuration_control_index;
int first_valid_configuration_control_index;
int last_control_index;
bool last_screen_is_portrait;
static vector<string> controls_keys;
static vector<string> device_default_keys;

static map<string, vector<string>> children_map;

static void update_configuration_mode_controls() {
    if (configuration_mode) {

        // Brightness and eye controls
        Control *brightness_less_control = get_control_by_event(CONFIGURATION_BRIGHTNESS_LESS, &configuration_controls);
        Control *brightness_more_control = get_control_by_event(CONFIGURATION_BRIGHTNESS_MORE, &configuration_controls);
        Control *eye_control = get_control_by_event(CONFIGURATION_VISIBLE, &configuration_controls);

        if (act_configuration_control_index == -3) {
            if (brightness_less_control != nullptr) {
                brightness_less_control->alpha_portrait = CONTROLS_ALPHA_DISABLED;
                brightness_less_control->alpha_landscape = CONTROLS_ALPHA_DISABLED;
                brightness_less_control->is_pressable = false;
                brightness_less_control->update_textures_alpha();
            }
            if (brightness_more_control != nullptr) {
                brightness_more_control->alpha_portrait = CONTROLS_ALPHA_DISABLED;
                brightness_more_control->alpha_landscape = CONTROLS_ALPHA_DISABLED;
                brightness_more_control->is_pressable = false;
                brightness_more_control->update_textures_alpha();
            }
            if (eye_control != nullptr) {
                eye_control->alpha_portrait = CONTROLS_ALPHA_DISABLED;
                eye_control->alpha_landscape = CONTROLS_ALPHA_DISABLED;
                eye_control->is_pressable = false;
                eye_control->update_textures_alpha();
            }
        } else {
            if (brightness_less_control != nullptr) {
                brightness_less_control->alpha_portrait = CONTROLS_ALPHA_ENABLED;
                brightness_less_control->alpha_landscape = CONTROLS_ALPHA_ENABLED;
                brightness_less_control->is_pressable = true;
                brightness_less_control->update_textures_alpha();
            }
            if (brightness_more_control != nullptr) {
                brightness_more_control->alpha_portrait = CONTROLS_ALPHA_ENABLED;
                brightness_more_control->alpha_landscape = CONTROLS_ALPHA_ENABLED;
                brightness_more_control->is_pressable = true;
                brightness_more_control->update_textures_alpha();
            }
            if (eye_control != nullptr) {
                eye_control->alpha_portrait = CONTROLS_ALPHA_ENABLED;
                eye_control->alpha_landscape = CONTROLS_ALPHA_ENABLED;
                eye_control->is_pressable = true;
                eye_control->update_textures_alpha();
            }
        }

        // Selection control
        Control *selection_control = get_control_by_event(CONFIGURATION_SELECTION, &configuration_controls);
        if (selection_control != nullptr) {
            if (act_configuration_control_index != -3) {
                // Buttons
                memcpy(selection_control->get_control_frect(), effective_game_controls[app_config_struct.mobile_use_inverted_controls ? 1 : 0][act_configuration_control_index]->get_control_frect(), sizeof(SDL_FRect));
            } else {
                // Screen
                SDL_FRect *ref = *get_act_jzintv_rendering_frect_ref(screen_is_portrait);
                if (ref != nullptr) {
                    memcpy(selection_control->get_control_frect(), ref, sizeof(SDL_FRect));
                }
            }
            selection_control->portrait_frect.x -= 0.5;
            selection_control->portrait_frect.y -= 0.5;
            selection_control->portrait_frect.w += 1;
            selection_control->portrait_frect.h += 1;
            selection_control->landscape_frect.x -= 0.5;
            selection_control->landscape_frect.y -= 0.5;
            selection_control->landscape_frect.w += 1;
            selection_control->landscape_frect.h += 1;
        }
    }
}

bool is_compatible_with_act_configuration(Control *c);

static bool is_ok_selection_control_index(int new_index) {
    if (new_index == -3) {
        return true;
    }
    if (new_index < 0 || new_index >= effective_game_controls[app_config_struct.mobile_use_inverted_controls ? 1 : 0].size()) {
        return false;
    }
    Control *eff_control = effective_game_controls[app_config_struct.mobile_use_inverted_controls ? 1 : 0][new_index];
    if (!is_compatible_with_act_configuration(eff_control)) {
        return false;
    }
    return eff_control->is_configurable;
}

static int increment_decrement_selection_control(bool increment, int ref_val) {
    int next;
    if (ref_val == -3) {
        if (increment) {
            next = 0;
        } else {
            next = effective_game_controls[app_config_struct.mobile_use_inverted_controls ? 1 : 0].size() - 1;
        }
    } else {
        next = increment ? ref_val + 1 : ref_val - 1;
        int siz = effective_game_controls[app_config_struct.mobile_use_inverted_controls ? 1 : 0].size();
        if (next > siz || next < 0) {
            next = -3;
        }
    }
    return next;
}

static int find_next_increment_decrement_selection_control(bool increment) {
    int start_point = act_configuration_control_index;
    int act_point = increment_decrement_selection_control(increment, start_point);
    while (!is_ok_selection_control_index(act_point)) {
        act_point = increment_decrement_selection_control(increment, act_point);
        if (act_point == start_point) {
            break;
        }
    }
    return act_point;
}

static bool is_equals_name(int point, string name) {
    Control *c = effective_game_controls[app_config_struct.mobile_use_inverted_controls ? 1 : 0][point];
    return !c->original_event.compare(name);
}

static int find_next_increment_decrement_selection_control_by_name(bool increment, string name) {
    int start_point = act_configuration_control_index;
    int act_point = increment_decrement_selection_control(increment, start_point);
    while (!is_ok_selection_control_index(act_point) || !is_equals_name(act_point, name)) {
        act_point = increment_decrement_selection_control(increment, act_point);
        if (act_point == start_point) {
            break;
        }
    }
    return act_point;
}

static int find_first_valid_configuration_control_index_by_name(string name) {
    act_configuration_control_index = -3;
    int next = find_next_increment_decrement_selection_control_by_name(true, name);
    if (!is_ok_selection_control_index(next)) {
        return -2;
    }
    return next;
}

static int find_first_valid_configuration_control_index() {
    act_configuration_control_index = -3;
    int next = find_next_increment_decrement_selection_control(true);
    if (!is_ok_selection_control_index(next)) {
        return -2;
    }
    return next;
}

void custom_show_saved(bool global) {
    // Later we'll send pause
    manage_show_saved = true;
    manage_show_saved_global = global;
    Control *save_control;
    Control *saved_as_default = get_control_by_event(CONFIGURATION_SAVED_GLOBAL_TOASTER, &configuration_controls);
    Control *saved_game = get_control_by_event(CONFIGURATION_SAVED_FOR_GAME_TOASTER, &configuration_controls);
    if (manage_show_saved_global) {
        save_control = saved_as_default;
        if (saved_game != nullptr) {
            saved_game->is_visible = 0;
        }
    } else {
        save_control = saved_game;
        if (saved_as_default != nullptr) {
            saved_as_default->is_visible = 0;
        }
    }
    if (save_control != nullptr) {
        save_control->is_visible = 1;
        save_control->alpha_portrait = 255;
        save_control->alpha_landscape = 255;
    }
    saved_start_ticks = SDL_GetTicks();
}

bool was_paused;

static bool manage_configuration_mode_change(bool next_mode_is_configuration_mode) {
    bool res = false;
    bool at_least_a_control_to_configure_is_found = false;
    if (next_mode_is_configuration_mode && first_valid_configuration_control_index == -1) {
        first_valid_configuration_control_index = find_first_valid_configuration_control_index();
        last_control_index = first_valid_configuration_control_index;
    }
    at_least_a_control_to_configure_is_found = first_valid_configuration_control_index != -2;

    if (at_least_a_control_to_configure_is_found) {
        if (next_mode_is_configuration_mode) {
            was_paused = custom_emulation_paused;
            if (!was_paused) {
                custom_pause_event();
            }
            // Entering configuration mode
            for (int hand_index = 0; hand_index < 2; hand_index++) {
                for (int i = 0; i < effective_game_controls[hand_index].size(); i++) {
                    Control *c = effective_game_controls[hand_index][i];
                    c->is_pressable = false;
                    c->was_pressable = false;
                }
            }
            Control *saved_as_default = get_control_by_event(CONFIGURATION_SAVED_GLOBAL_TOASTER, &configuration_controls);
            Control *saved_game = get_control_by_event(CONFIGURATION_SAVED_FOR_GAME_TOASTER, &configuration_controls);
            for (int i = 0; i < configuration_controls.size(); i++) {
                Control *c = configuration_controls[i];
                if (saved_as_default != c && saved_game != c) {
                    c->is_visible = 1;
                }
            }

            if (!is_ok_selection_control_index(last_control_index)) {
                last_control_index = find_first_valid_configuration_control_index();
            }
            act_configuration_control_index = last_control_index;
        } else {
            // Entering game mode
            for (int hand_index = 0; hand_index < 2; hand_index++) {
                for (int i = 0; i < effective_game_controls[hand_index].size(); i++) {
                    Control *c = effective_game_controls[hand_index][i];
                    c->is_pressable = true;
                    c->was_pressable = true;
                    c->update_textures_alpha();
                }
            }
            Control *switch_mode_control = get_control_by_event(CONFIGURATION_SWITCH_MODE, &configuration_controls);
            for (int i = 0; i < configuration_controls.size(); i++) {
                Control *act = configuration_controls[i];
                if (act != switch_mode_control) {
                    configuration_controls[i]->is_visible = 0;
                }
            }
            if (!was_paused) {
                custom_pause_event();
            }
        }
        res = next_mode_is_configuration_mode;
    }
    return res;
}

static void idle(void *val) {

}

static void switch_ecs_mode(bool active) {
    string key = active ? "F7" : "F5";
    const char *key_c = key.c_str();
    event_enqueue_custom(SDL_KEYDOWN, key_c);
    event_enqueue_custom(SDL_KEYUP, key_c);
    // Forced, because if you don't generate other events, act_map is not 'naturally' updated (which means consume_special_event is not called)
    act_map = active ? 3 : 0;
}

bool switching_mode = false;
int old_act_map = 0;

static void switch_mode_callback(void *val) {
    switching_mode = true;
    release_all_controls();
    switching_mode = false;
    configuration_mode = manage_configuration_mode_change(!configuration_mode);
    update_configuration_mode_controls();
    if (configuration_mode) {
        old_act_map = act_map;
        if (act_map != 0) {
            switch_ecs_mode(false);
        }
    }
}

static void change_player_callback(void *val) {
    app_config_struct.act_player = 1 - app_config_struct.act_player;
}

static void toggle_ecs_keyboard_callback(void *val) {
    if (act_map == 0) {
        // Enable ECS map
        switch_ecs_mode(true);
    } else {
        // Back to first map
        switch_ecs_mode(false);
    }
}

static void send_quit(void *val) {
    int hand_index = *((int *) val);

    const SDL_MessageBoxButtonData buttons[] = {
            {0,                                       1, "Yes"},
            {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "No"}
    };
    const SDL_MessageBoxColorScheme colorScheme = {
            { /* .colors (.r, .g, .b) */
                    /* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
                    {125, 125, 125},
                    /* [SDL_MESSAGEBOX_COLOR_TEXT] */
                    {0, 255, 0},
                    /* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
                    {255, 255, 0},
                    /* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] */
                    {0, 0, 255},
                    /* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] */
                    {255, 0, 255}
            }
    };
    const SDL_MessageBoxData messageboxdata = {
            SDL_MESSAGEBOX_INFORMATION, /* .flags */
            nullptr, /* .window */
            "Confirmation", /* .title */
            "Quit game?", /* .message */
            SDL_arraysize(buttons), /* .numbuttons */
            buttons, /* .buttons */
            &colorScheme /* .colorScheme */
    };
    int buttonid;
    if (SDL_ShowMessageBox(&messageboxdata, &buttonid) < 0) {
        SDL_Log("error displaying message box");
        return;
    }
    if (buttonid == -1) {
        SDL_Log("no selection");
    } else {
        release_all_controls();
        if (buttonid == 1) {
            Control *quit_control = get_control_by_event(QUIT, &device_default_controls[hand_index]);
            event_enqueue_custom(SDL_KEYDOWN, mapping_events[act_map][quit_control->jzintv_event_index].c_str());
        }
    }
}

bool check_switch_mode() {
    if (configuration_mode) {
        switch_mode_callback(nullptr);
        return true;
    }
    return false;
}

static int get_direction_position(const char *key) {
    for (int i = 0; i < events_directions_needed_vec.size(); i++) {
        const char *ev = events_directions_needed_vec[i].c_str();
        if (!strcmp(ev, key)) {
            return i;
        }
    }
    return -1;
}

void refresh_rect_on_screen_size_change() {
    SDL_FRect *frect = *get_act_jzintv_rendering_frect_ref(screen_is_portrait);
    if (frect != nullptr) {
        SDL_Rect *rect_ref = *get_act_jzintv_rendering_rect_ref(screen_is_portrait);
        if (rect_ref != nullptr) {
            *rect_ref = transform_to_sdl_rect(frect);
        }
    }
}

static void fill_hierarchy_controls(vector<Control *> *hier) {
    int hand_index = app_config_struct.mobile_use_inverted_controls ? 1 : 0;
    Control *parent = effective_game_controls[hand_index][act_configuration_control_index];
    hier->push_back(parent);

    vector<Control *>::iterator it;
    for (it = parent->children.begin(); it != parent->children.end(); it++) {
        Control *child = (*it);
        hier->push_back(child);
    }
}

static void change_size_and_position_callback(void *val) {
    int direction = *((int *) val);
    float min_len = 0.05;
    SDL_FRect *frect;
    if (act_configuration_control_index != -3) {
        // Buttons
        frect = effective_game_controls[app_config_struct.mobile_use_inverted_controls ? 1 : 0][act_configuration_control_index]->get_control_frect();
    } else {
        // Screen
        frect = *get_act_jzintv_rendering_frect_ref(screen_is_portrait);
    }
    if (direction == get_direction_position(PD0L_J_N)) {
        // Up
        if (frect->y > min_len) {
            frect->y -= min_len;
            if (frect->y < min_len) {
                frect->y = 0;
            }
        }
    } else if (direction == get_direction_position(PD0L_J_S)) {
        // Down
        if (frect->y + frect->h < ((float) 100 - min_len)) {
            frect->y += min_len;
        }
    } else if (direction == get_direction_position(PD0L_J_E)) {
        // Right
        if (frect->x + frect->w < ((float) 100 - min_len)) {
            frect->x += min_len;
        }
    } else if (direction == get_direction_position(PD0L_J_W)) {
        // Left
        if (frect->x > min_len) {
            frect->x -= min_len;
        }
        if (frect->x < min_len) {
            frect->x = 0;
        }
    } else if (direction == get_direction_position(PD0L_J_NE)) {
        // Up Right
        if (frect->x + frect->w < ((float) 100 - min_len)) {
            frect->w += min_len;
        }
    } else if (direction == get_direction_position(PD0L_J_NW)) {
        // Up Left
        if (frect->w > 5) {
            frect->w -= min_len;
        }
    } else if (direction == get_direction_position(PD0L_J_SW)) {
        // Down Left
        if (frect->h > 5) {
            frect->h -= min_len;
        }
    } else if (direction == get_direction_position(PD0L_J_SE)) {
        // Down Right
        if (frect->y + frect->h < ((float) 100 - min_len)) {
            frect->h += min_len;
        }
    }
    if (act_configuration_control_index == -3) {
        SDL_Rect *rect_ref = *get_act_jzintv_rendering_rect_ref(screen_is_portrait);
        *rect_ref = transform_to_sdl_rect(frect);
    }
}

static void change_control_callback(void *val) {
    int inc_val = *((int *) val);
    act_configuration_control_index = find_next_increment_decrement_selection_control(inc_val == 1);
    last_control_index = act_configuration_control_index;
}

static void change_visibility_callback(void *val) {
    vector<Control *> all_controls_hierarchy;
    fill_hierarchy_controls(&all_controls_hierarchy);
    vector<Control *>::iterator it;
    for (it = all_controls_hierarchy.begin(); it != all_controls_hierarchy.end(); it++) {
        Control *c = *it;
        c->is_visible = 1 - c->is_visible;
    }
    all_controls_hierarchy.clear();
}

void deallocate_control_texture(SDL_Texture *ptr) {
    SDL_DestroyTexture(ptr);
}

static void copy_control(Control *source, Control *destination);

static void reset_to_default_callback(void *val) {
    if (act_configuration_control_index != -3) {
        // Buttons
        vector<Control *> all_controls_hierarchy;
        fill_hierarchy_controls(&all_controls_hierarchy);
        vector<Control *>::iterator it;
        for (it = all_controls_hierarchy.begin(); it != all_controls_hierarchy.end(); it++) {
            Control *c = *it;
            int old_position = c->jzintv_event_index;
            if (c->tx_released != nullptr) {
                deallocate_control_texture(c->tx_released);
            }
            c->tx_released = nullptr;

            if (c->tx_pressed != nullptr) {
                deallocate_control_texture(c->tx_pressed);
            }
            c->tx_pressed = nullptr;

            int hand_index = app_config_struct.mobile_use_inverted_controls ? 1 : 0;
            Control *delta_ref = get_control_by_event(c->original_event, &(delta_default_controls[hand_index]));
            copy_control(delta_ref, c);

            if (c->jzintv_event_index == -1) {
                c->jzintv_event_index = old_position;
            }
            c->update_textures_alpha();
        }
        all_controls_hierarchy.clear();
    } else {
        // Screen
        init_jzintv_rendering_rect(false);
    }
}

static void switch_hand_index_callback(void *val) {
    if (act_configuration_control_index != -3) {
        Control *c = effective_game_controls[app_config_struct.mobile_use_inverted_controls ? 1 : 0][act_configuration_control_index];
        app_config_struct.mobile_use_inverted_controls = !app_config_struct.mobile_use_inverted_controls;
        act_configuration_control_index = find_first_valid_configuration_control_index_by_name(c->original_event);
    } else {
        app_config_struct.mobile_use_inverted_controls = !app_config_struct.mobile_use_inverted_controls;
    }
    last_control_index = act_configuration_control_index;
}

static void change_brightness_callback(void *val) {
    int inc_val = *((int *) val);
    vector<Control *> all_controls_hierarchy;
    fill_hierarchy_controls(&all_controls_hierarchy);
    vector<Control *>::iterator it;
    for (it = all_controls_hierarchy.begin(); it != all_controls_hierarchy.end(); it++) {
        Control *c = *it;
        int64_t *ptr = screen_is_portrait ? &(c->alpha_portrait) : &(c->alpha_landscape);
        *ptr = inc_val == 1 ? (*ptr) + 5 : (*ptr) - 5;
        if (*ptr > 255) {
            *ptr = 255;
        } else if (*ptr < 0) {
            *ptr = 0;
        }
        c->update_textures_alpha();
    }
    all_controls_hierarchy.clear();
}

static void save_jzintv_screen_data(bool is_global) {
    for (int i = 0; i < 2; i++) {
        bool act_orientation = i == 0;
        SDL_FRect *dest_frect;
        SDL_FRect *frect_ref = *get_act_jzintv_rendering_frect_ref(act_orientation);

        if (is_global) {
            dest_frect = get_delta_screen_frect(act_orientation);
        } else {
            dest_frect = get_rom_screen_frect(&(roms_configuration[config_index]), act_orientation);
            memcpy(dest_frect, frect_ref, sizeof(SDL_FRect));
            dest_frect = act_orientation ? &selected_rom->mobile_portrait_rect : &selected_rom->mobile_landscape_rect;
        }

        if (dest_frect != nullptr && frect_ref != nullptr) {
            memcpy(dest_frect, frect_ref, sizeof(SDL_FRect));
        }
    }
}

Control *allocate_control() {
    Control *c = new Control();
    return c;
}

void deallocate_control(Control *ptr) {
    delete ptr;
}

/*
vector<SDL_Texture *> textures;
vector<Control *> allocControls;

SDL_Texture *debug_allocate_control_texture(const char *name, SDL_Renderer *renderer, int64_t ptr) {
    SDL_Texture *tx = get_texture(name, renderer, ptr);
    if (tx != nullptr) {
        Log(LOG_INFO) << "Alloco allocate_control_texture ptr " << std::hex << tx;
        textures.push_back(tx);
    }
    return tx;
}

void debug_deallocate_control_texture(SDL_Texture *ptr) {
    vector<SDL_Texture *>::iterator it = textures.begin();

    bool found = false;
    while (it != textures.end()) {
        SDL_Texture *teex = (*it);
        if (teex == ptr) {
            if (found) {
                string ss = "Troppi ptr identici";
                Log(LOG_INFO) << ss;
                std::stringstream new_ex(ss);
                throw new_ex;
            }
            SDL_DestroyTexture(ptr);
            it = textures.erase(it);
            found = true;
        } else {
            ++it;
        }
    }
    if (!found) {
        string ss = "Ptr non trovato";
        Log(LOG_INFO) << ss;
        std::stringstream new_ex(ss);
        throw new_ex;
    }
}


Control * debug_allocate_control() {
    Control *c = new Control();
    allocControls.push_back(c);
    return c;
}

void debug_deallocate_control(Control *ptr) {
    vector<Control *>::iterator it = allocControls.begin();

    bool found = false;
    while (it != allocControls.end()) {
        Control *teex = (*it);
        if (teex == ptr) {
            if (found) {
                string ss = "deallocate_control Troppi ptr identici";
                Log(LOG_INFO) << ss;
                std::stringstream new_ex(ss);
                throw new_ex;
            }
            delete ptr;
            it = allocControls.erase(it);
            found = true;
        } else {
            ++it;
        }
    }
    if (!found) {
        string ss = "deallocate_control Ptr non trovato";
        Log(LOG_INFO) << ss;
        std::stringstream new_ex(ss);
        throw new_ex;
    }
}

 void debug_print_controls_ptrs() {
    vector<Control *>::iterator it2 = allocControls.begin();
    ostringstream oss;
    oss << "Presenti controls:\n";
    while (it2 != allocControls.end()) {
        Control *teex2 = (*it2);
        if (teex2==test) {
            oss << std::hex << teex2 << "  ";
        }
        ++it2;
    }
    Log(LOG_INFO) << oss.str();
}

void debug_check_textures_freed() {
    vector<SDL_Texture *>::iterator it = textures.begin();
    while (it != textures.end()) {
        SDL_Texture *teex = (*it);
        Log(LOG_INFO) << "Non è stato liberato (texture): " << std::hex << teex;
        ++it;
    }

    vector<Control *>::iterator it2 = allocControls.begin();
    while (it2 != allocControls.end()) {
        Control *teex2 = (*it2);
        Log(LOG_INFO) << "Non è stato liberato (control): " << std::hex << teex2;
        ++it2;
    }
}
*/

void duplicate_controls(vector<Control *> *source, vector<Control *> *destination) {
    clear_controls(destination);

    for (int i = 0; i < source->size(); i++) {
        Control *c = allocate_control();
        copy_control(source->at(i), c);
        destination->push_back(c);
    }
}

void deallocate_control(Control *ptr);

//  Se siamo nel caso del delta (quindi con parent array device), visible a 0 non lo mettiamo mai a -1, anche se il valore coincide con il parent.
//    Nel caso di controllo standard, se visible = 0 vuol dire che è un delta rispetto all'array device, che ha tutti i controlli standard con visible=1. Quindi come da comportamento normale, non viene messo a -1.
//    Nel caso di un controllo NON standard invece vogliamo ricordarci che cmq non è visibile, altrimenti, combaciando con il valore visible=0 dell'array device (visto che non è standard), sparirebbe dall'ini,
//    e al giro dopo sarebbe ancora visible.
//  Se siamo invece nel caso dei giochi (quindi con parent array delta):
//    L'approccio di tenere solo il delta non è ottimale: se cambio la configurazione del controllo di un gioco, e questa viene a coincidere con quella standard dell'array delta, automaticamente sparisce dall'ini,
//    con la conseguenza che il gioco torna ad avere la configurazione standard per quel controllo, anche se avevo fatto un salvataggio custom esplicito.
//    Non è il massimo. Per la maggior parte delle property (posizione, grandezza, ALPHA..) è accettabile. Per la property visible invece non tanto :-( Se scelgo di nascondere un pulsante in un gioco, e la scelta
//    coincide con delta, l'opzione sparisce dall'ini, e se si cambia il delta per quel gioco torna a vedersi
static void purge_duplicate_elements(vector<Control *> *parent, vector<Control *> *destination, bool keep_univisible = true) {
    vector<Control *>::iterator it = destination->begin();

    while (it != destination->end()) {
        Control *dest_control = (*it);
        Control *parent_control = get_control_by_event(dest_control->original_event, parent);
        if (dest_control->normalize_to_delta(parent_control, keep_univisible)) {
            deallocate_control(dest_control);
            it = destination->erase(it);
        } else {
            ++it;
        }
    }
}

void Control::print() {
    const char *ee = original_event.c_str();
    Log(LOG_INFO) << "*********************************************";
    Log(LOG_INFO) << "original_event=" << original_event;
    Log(LOG_INFO) << "portrait_frect.x=" << portrait_frect.x;
    Log(LOG_INFO) << "portrait_frect.y=" << portrait_frect.y;
    Log(LOG_INFO) << "portrait_frect.w=" << portrait_frect.w;
    Log(LOG_INFO) << "portrait_frect.h=" << portrait_frect.h;
    Log(LOG_INFO) << "landscape_frect.x=" << landscape_frect.x;
    Log(LOG_INFO) << "landscape_frect.y=" << landscape_frect.y;
    Log(LOG_INFO) << "landscape_frect.w=" << landscape_frect.w;
    Log(LOG_INFO) << "landscape_frect.h=" << landscape_frect.h;
    Log(LOG_INFO) << "is_visible=" << is_visible;
    Log(LOG_INFO) << "alpha_portrait=" << alpha_portrait;
    Log(LOG_INFO) << "alpha_landscape=" << alpha_landscape;
    Log(LOG_INFO) << "file_name_released=" << file_name_released;
    Log(LOG_INFO) << "file_name_pressed=" << file_name_pressed;
    Log(LOG_INFO) << "override_event=" << override_event;
}

void print_controls(string message, vector<Control *> *container) {
    Log(LOG_INFO) << "\n\n------------------------------------------------";
    Log(LOG_INFO) << message;
    Log(LOG_INFO) << "------------------------------------------------";
    for (int i = 0; i < container->size(); i++) {
        Control *cc = (*container)[i];
        cc->print();
    }
}

void normalize_controls_from_delta(int hand_index);

static void save_changes_callback(void *val) {
    // 1: save global
    // 2: save game
    int save_val = *((int *) val);
    bool is_global = save_val == 1;
    vector<Control *> *controls = nullptr;
    for (int hand_index = 0; hand_index < 2; hand_index++) {
        if (!is_global) {
            duplicate_controls(&(effective_game_controls[hand_index]), &(selected_rom->controls[hand_index]));
            duplicate_controls(&(effective_game_controls[hand_index]), &(roms_configuration[config_index].controls[hand_index]));
            save_jzintv_screen_data(is_global);
        } else {
            normalize_controls_to_delta(nullptr, hand_index, true);
            normalize_screen_to_delta();
            save_jzintv_screen_data(is_global);
            duplicate_controls(&(effective_game_controls[hand_index]), &(delta_default_controls[hand_index]));
            normalize_controls_from_delta(hand_index);
        }
    }

    save_config_file();

    if (is_global) {
        // Restoring default values
        init_jzintv_rendering_rect(false);
    }
    custom_show_saved(is_global);
}

// ctor
Control::Control() {
    portrait_frect.x = -1;
    portrait_frect.y = -1;
    portrait_frect.w = -1;
    portrait_frect.h = -1;
    landscape_frect.x = -1;
    landscape_frect.y = -1;
    landscape_frect.w = -1;
    landscape_frect.h = -1;

    last_direction = -1;
    last_pressed = false;
    is_pressed = false;
    is_disc = false;
    is_visible = 1;
    is_drawn = false;
    file_name_released = "";
    tx_released = nullptr;
    file_name_pressed = "";
    tx_pressed = nullptr;
    alpha_portrait = DEFAULT_ALPHA_CONFIG;
    alpha_landscape = DEFAULT_ALPHA_CONFIG;
    is_pressable = true;
    jzintv_event_index = -1;
    original_event = "";
    override_event = "";
    config_func = nullptr;
    config_parameter = 0;
    continuous_click = false;
    is_configurable = true;
    act_on_release = false;
}


// dtor
Control::~Control() {
    file_name_pressed = "";
    file_name_released = "";
    if (tx_released != nullptr) {
        // Already deallocated by LIBSDL?
//        deallocate_control_texture(tx_released);
    }
    tx_released = nullptr;

    if (tx_pressed != nullptr) {
        // Already deallocated by LIBSDL?
//        deallocate_control_texture(tx_pressed);
    }
    tx_pressed = nullptr;
    original_event = "";
    override_event = "";
    children.clear();
}

void Control::update_textures_alpha() {
    if (tx_released != nullptr && tx_released != (SDL_Texture *) -1) {
        SDL_SetTextureAlphaMod(tx_released, screen_is_portrait ? alpha_portrait : alpha_landscape);
    }
    if (tx_pressed != nullptr && tx_pressed != (SDL_Texture *) -1) {
        SDL_SetTextureAlphaMod(tx_pressed, screen_is_portrait ? alpha_portrait : alpha_landscape);
    }
}

SDL_FRect *Control::get_control_frect() {
    return screen_is_portrait ? &portrait_frect : &landscape_frect;
}

bool Control::check_and_fix_size() {
    bool res = true;
    if (portrait_frect.x + portrait_frect.w > 100) {
        portrait_frect.x = 0;
        portrait_frect.w = 10;
        res = false;
    }
    if (portrait_frect.y + portrait_frect.h > 100) {
        portrait_frect.y = 0;
        portrait_frect.h = 10;
        res = false;
    }
    if (landscape_frect.x + landscape_frect.w > 100) {
        landscape_frect.x = 0;
        landscape_frect.w = 10;
        res = false;
    }
    if (landscape_frect.y + landscape_frect.h > 100) {
        landscape_frect.y = 0;
        landscape_frect.h = 10;
        res = false;
    }
    return res;
}

float get_portrait_width(float ref) {
    return (99 - (ref / window_ratio_portrait) - 2.5) / 3.9;
}

#define CONTROLS_LEFT_OFFSET 3.0f
#define CONTROLS_RIGHT_OFFSET 3.0f
#define CONTROLS_BOTTOM_OFFSET 7.0f

static void compute_position_and_size(SDL_FRect *frect, int pos_x, int pos_y, bool is_button_in_controller, bool thin_button, int hand_index, bool portrait, bool is_disc = false) {
    float original_size = 11 + (portrait ? app_config_struct.mobile_default_portrait_controls_size : app_config_struct.mobile_default_landscape_controls_size);
    float size = original_size;
    if (is_disc) {
        size = 3 * size;
    }
    float size_w;
    float size_h;
    float size_offset_reference_w;
    float size_offset_reference_h;
    if (portrait) {
        size_w = size;
        size_offset_reference_w = original_size;
        if (!thin_button || is_disc) {
            size_h = size * window_ratio_portrait;
        } else {
            size_h = size * window_ratio_portrait / 2;
        }
        if (is_button_in_controller) {
            size_offset_reference_h = original_size * window_ratio_portrait;
        } else {
            size_offset_reference_h = (original_size / 2) * window_ratio_portrait;
        }
    } else {
        if (!thin_button || is_disc) {
            size_h = size;
        } else {
            size_h = size / 2;
        }
        if (is_button_in_controller) {
            size_offset_reference_h = original_size;
        } else {
            size_offset_reference_h = original_size / 2;
        }
        size_w = size / window_ratio_landscape;
        size_offset_reference_w = original_size / window_ratio_landscape;
    }

    frect->w = size_w;
    frect->h = size_h;
    switch (hand_index) {
        case 0:
            frect->x = is_button_in_controller ? (100 - CONTROLS_RIGHT_OFFSET - (3 - pos_x) * size_offset_reference_w) : (CONTROLS_LEFT_OFFSET + pos_x * size_w);
            break;
        case 1:
            frect->x = !is_button_in_controller ? (100 - CONTROLS_RIGHT_OFFSET - (3 - pos_x) * size_offset_reference_w) : (CONTROLS_LEFT_OFFSET + pos_x * size_w);
            break;
    }
    frect->y = 100 - CONTROLS_BOTTOM_OFFSET - ((is_button_in_controller ? 5 : 10) - pos_y) * size_offset_reference_h;
}

static void compute_ecs_position_and_size(SDL_FRect *frect, int pos_x, int pos_y, bool portrait, bool is_space = false) {
    float original_size = portrait ? 8.3 : 6;
    float space_perc = 7;
    float size = original_size;
    if (is_space) {
        size = space_perc * size;
    }
    float size_w;
    float size_h;
    if (portrait) {
        size_w = size;
        size_h = original_size * window_ratio_portrait * 1.2;
    } else {
        size_w = size;
        size_h = original_size * window_ratio_landscape / 1.5;
    }

    frect->w = size_w;
    frect->h = size_h;

    int x_offset = (100 - (original_size * 11)) / 2;

    frect->x = x_offset + pos_x * original_size;

    switch (pos_y) {
        case 0:
//            frect->x += original_size / 1.1;
            break;
        case 1:
//            frect->x += original_size / 2;
            break;
        case 2:
//            frect->x += original_size / 1.6;
            break;
        case 4:
//            frect->x += original_size / 1.6;
            if (pos_x > 2) {
                frect->x += (space_perc - 0.99) * original_size;
            }
            break;
        default:
            break;
    }
    frect->y = 100 - CONTROLS_BOTTOM_OFFSET - (5 - pos_y) * size_h;
}

void Control::set_default_position_and_size(int hand_index) {
    float x_controls_offset_portrait = 1;
    float x_controls_offset_landscape = 1;
    float w_controls_portrait_disc_size = 20;
    float h_controls_landscape_disc_size = 20;

    if (!original_event.compare(PD0L_A_T) || !original_event.compare(PD0R_A_T)) {
        compute_position_and_size(&portrait_frect, 0, 0, true, false, hand_index, true);
        compute_position_and_size(&landscape_frect, 0, 0, true, false, hand_index, false);
    } else if (!original_event.compare(PD0L_A_L) || !original_event.compare(PD0R_A_L)) {
        compute_position_and_size(&portrait_frect, 1, 0, true, false, hand_index, true);
        compute_position_and_size(&landscape_frect, 1, 0, true, false, hand_index, false);
    } else if (!original_event.compare(PD0L_A_R) || !original_event.compare(PD0R_A_R)) {
        compute_position_and_size(&portrait_frect, 2, 0, true, false, hand_index, true);
        compute_position_and_size(&landscape_frect, 2, 0, true, false, hand_index, false);
    } else if (!original_event.compare(PD0L_KP1) || !original_event.compare(PD0R_KP1)) {
        compute_position_and_size(&portrait_frect, 0, 1, true, false, hand_index, true);
        compute_position_and_size(&landscape_frect, 0, 1, true, false, hand_index, false);
    } else if (!original_event.compare(PD0L_KP2) || !original_event.compare(PD0R_KP2)) {
        compute_position_and_size(&portrait_frect, 1, 1, true, false, hand_index, true);
        compute_position_and_size(&landscape_frect, 1, 1, true, false, hand_index, false);
    } else if (!original_event.compare(PD0L_KP3) || !original_event.compare(PD0R_KP3)) {
        compute_position_and_size(&portrait_frect, 2, 1, true, false, hand_index, true);
        compute_position_and_size(&landscape_frect, 2, 1, true, false, hand_index, false);
    } else if (!original_event.compare(PD0L_KP4) || !original_event.compare(PD0R_KP4)) {
        compute_position_and_size(&portrait_frect, 0, 2, true, false, hand_index, true);
        compute_position_and_size(&landscape_frect, 0, 2, true, false, hand_index, false);
    } else if (!original_event.compare(PD0L_KP5) || !original_event.compare(PD0R_KP5)) {
        compute_position_and_size(&portrait_frect, 1, 2, true, false, hand_index, true);
        compute_position_and_size(&landscape_frect, 1, 2, true, false, hand_index, false);
    } else if (!original_event.compare(PD0L_KP6) || !original_event.compare(PD0R_KP6)) {
        compute_position_and_size(&portrait_frect, 2, 2, true, false, hand_index, true);
        compute_position_and_size(&landscape_frect, 2, 2, true, false, hand_index, false);
    } else if (!original_event.compare(PD0L_KP7) || !original_event.compare(PD0R_KP7)) {
        compute_position_and_size(&portrait_frect, 0, 3, true, false, hand_index, true);
        compute_position_and_size(&landscape_frect, 0, 3, true, false, hand_index, false);
    } else if (!original_event.compare(PD0L_KP8) || !original_event.compare(PD0R_KP8)) {
        compute_position_and_size(&portrait_frect, 1, 3, true, false, hand_index, true);
        compute_position_and_size(&landscape_frect, 1, 3, true, false, hand_index, false);
    } else if (!original_event.compare(PD0L_KP9) || !original_event.compare(PD0R_KP9)) {
        compute_position_and_size(&portrait_frect, 2, 3, true, false, hand_index, true);
        compute_position_and_size(&landscape_frect, 2, 3, true, false, hand_index, false);
    } else if (!original_event.compare(PD0L_KPC) || !original_event.compare(PD0R_KPC)) {
        compute_position_and_size(&portrait_frect, 0, 4, true, false, hand_index, true);
        compute_position_and_size(&landscape_frect, 0, 4, true, false, hand_index, false);
    } else if (!original_event.compare(PD0L_KP0) || !original_event.compare(PD0R_KP0)) {
        compute_position_and_size(&portrait_frect, 1, 4, true, false, hand_index, true);
        compute_position_and_size(&landscape_frect, 1, 4, true, false, hand_index, false);
    } else if (!original_event.compare(PD0L_KPE) || !original_event.compare(PD0R_KPE)) {
        compute_position_and_size(&portrait_frect, 2, 4, true, false, hand_index, true);
        compute_position_and_size(&landscape_frect, 2, 4, true, false, hand_index, false);
    } else if (!original_event.compare(PAUSE)) {
        compute_position_and_size(&portrait_frect, 0, 0, false, true, hand_index, true);
        compute_position_and_size(&landscape_frect, 0, 0, false, true, hand_index, false);
    } else if (!original_event.compare(RESET)) {
        compute_position_and_size(&portrait_frect, 1, 0, false, true, hand_index, true);
        compute_position_and_size(&landscape_frect, 1, 0, false, true, hand_index, false);
    } else if (!original_event.compare(QUIT)) {
        compute_position_and_size(&portrait_frect, 0, 1, false, true, hand_index, true);
        compute_position_and_size(&landscape_frect, 0, 1, false, true, hand_index, false);
    } else if (!original_event.compare(CHANGE_PLAYER)) {
        compute_position_and_size(&portrait_frect, 1, 1, false, true, hand_index, true);
        compute_position_and_size(&landscape_frect, 1, 1, false, true, hand_index, false);
    } else if (!original_event.compare(KEYBOARD)) {
        compute_position_and_size(&portrait_frect, 2, 0, false, true, hand_index, true);
        compute_position_and_size(&landscape_frect, 2, 0, false, true, hand_index, false);
    } else if (!original_event.compare(SHOT)) {
        compute_position_and_size(&portrait_frect, 2, 1, false, true, hand_index, true);
        compute_position_and_size(&landscape_frect, 2, 1, false, true, hand_index, false);
    } else if (!original_event.compare(DISC_KEY)) {
        compute_position_and_size(&portrait_frect, 0, 3, false, false, hand_index, true, true);
        compute_position_and_size(&landscape_frect, 0, 3, false, false, hand_index, false, true);
    } else if (!original_event.compare(CONFIGURATION_PREV)) {
        portrait_frect.w = get_portrait_width(w_controls_portrait_disc_size);
        portrait_frect.h = portrait_frect.w * window_ratio_portrait;
        portrait_frect.x = x_controls_offset_portrait;
        portrait_frect.y = 99 - w_controls_portrait_disc_size;
        landscape_frect.h = h_controls_landscape_disc_size * window_ratio_landscape / 3;
        landscape_frect.w = landscape_frect.h / window_ratio_landscape;
        landscape_frect.y = 98 - 3 * landscape_frect.h;
        landscape_frect.x = x_controls_offset_landscape;
    } else if (!original_event.compare(CONFIGURATION_NEXT)) {
        portrait_frect.w = get_portrait_width(w_controls_portrait_disc_size);
        portrait_frect.h = portrait_frect.w * window_ratio_portrait;
        portrait_frect.x = x_controls_offset_portrait + portrait_frect.w;
        portrait_frect.y = 99 - w_controls_portrait_disc_size;
        landscape_frect.h = h_controls_landscape_disc_size * window_ratio_landscape / 3;
        landscape_frect.w = landscape_frect.h / window_ratio_landscape;
        landscape_frect.y = 98 - 3 * landscape_frect.h;
        landscape_frect.x = landscape_frect.w + x_controls_offset_landscape;
    } else if (!original_event.compare(CONFIGURATION_VISIBLE)) {
        portrait_frect.w = get_portrait_width(w_controls_portrait_disc_size);
        portrait_frect.h = portrait_frect.w * window_ratio_portrait;
        portrait_frect.x = x_controls_offset_portrait;
        portrait_frect.y = 99 - w_controls_portrait_disc_size + portrait_frect.h;
        landscape_frect.h = h_controls_landscape_disc_size * window_ratio_landscape / 3;
        landscape_frect.w = landscape_frect.h / window_ratio_landscape;
        landscape_frect.y = 98 - 2 * landscape_frect.h;
        landscape_frect.x = x_controls_offset_landscape;
    } else if (!original_event.compare(CONFIGURATION_BRIGHTNESS_LESS)) {
        portrait_frect.w = get_portrait_width(w_controls_portrait_disc_size);
        portrait_frect.h = portrait_frect.w * window_ratio_portrait;
        portrait_frect.x = x_controls_offset_portrait + portrait_frect.w;
        portrait_frect.y = 99 - w_controls_portrait_disc_size + portrait_frect.h;
        landscape_frect.h = h_controls_landscape_disc_size * window_ratio_landscape / 3;
        landscape_frect.w = landscape_frect.h / window_ratio_landscape;
        landscape_frect.y = 98 - 2 * landscape_frect.h;
        landscape_frect.x = landscape_frect.w + x_controls_offset_landscape;
    } else if (!original_event.compare(CONFIGURATION_BRIGHTNESS_MORE)) {
        portrait_frect.w = get_portrait_width(w_controls_portrait_disc_size);
        portrait_frect.h = portrait_frect.w * window_ratio_portrait;
        portrait_frect.x = x_controls_offset_portrait + 2 * portrait_frect.w;
        portrait_frect.y = 99 - w_controls_portrait_disc_size + portrait_frect.h;
        landscape_frect.h = h_controls_landscape_disc_size * window_ratio_landscape / 3;
        landscape_frect.w = landscape_frect.h / window_ratio_landscape;
        landscape_frect.y = 98 - 2 * landscape_frect.h;
        landscape_frect.x = 2 * landscape_frect.w + x_controls_offset_landscape;
    } else if (!original_event.compare(CONFIGURATION_DEFAULT)) {
        portrait_frect.w = get_portrait_width(w_controls_portrait_disc_size);
        portrait_frect.h = portrait_frect.w * window_ratio_portrait;
        portrait_frect.x = x_controls_offset_portrait + 2 * portrait_frect.w;
        portrait_frect.y = 99 - w_controls_portrait_disc_size;
        landscape_frect.h = h_controls_landscape_disc_size * window_ratio_landscape / 3;
        landscape_frect.w = landscape_frect.h / window_ratio_landscape;
        landscape_frect.y = 98 - 3 * landscape_frect.h;
        landscape_frect.x = 2 * landscape_frect.w + x_controls_offset_landscape;
    } else if (!original_event.compare(CONFIGURATION_SAVE_GLOBAL)) {
        portrait_frect.w = get_portrait_width(w_controls_portrait_disc_size);
        portrait_frect.h = portrait_frect.w * window_ratio_portrait;
        portrait_frect.x = x_controls_offset_portrait;
        portrait_frect.y = 99 - w_controls_portrait_disc_size + 2 * portrait_frect.h;
        landscape_frect.h = h_controls_landscape_disc_size * window_ratio_landscape / 3;
        landscape_frect.w = landscape_frect.h / window_ratio_landscape;
        landscape_frect.y = 98 - landscape_frect.h;
        landscape_frect.x = x_controls_offset_landscape;
    } else if (!original_event.compare(CONFIGURATION_SAVE_GAME)) {
        portrait_frect.w = get_portrait_width(w_controls_portrait_disc_size);
        portrait_frect.h = portrait_frect.w * window_ratio_portrait;
        portrait_frect.x = x_controls_offset_portrait + portrait_frect.w;
        portrait_frect.y = 99 - w_controls_portrait_disc_size + 2 * portrait_frect.h;
        landscape_frect.h = h_controls_landscape_disc_size * window_ratio_landscape / 3;
        landscape_frect.w = landscape_frect.h / window_ratio_landscape;
        landscape_frect.y = 98 - landscape_frect.h;
        landscape_frect.x = landscape_frect.w + x_controls_offset_landscape;
    } else if (!original_event.compare(CONFIGURATION_X_FLIP)) {
        portrait_frect.w = get_portrait_width(w_controls_portrait_disc_size);
        portrait_frect.h = portrait_frect.w * window_ratio_portrait;
        portrait_frect.x = x_controls_offset_portrait + 2 * portrait_frect.w;
        portrait_frect.y = 99 - w_controls_portrait_disc_size + 2 * portrait_frect.h;
        landscape_frect.h = h_controls_landscape_disc_size * window_ratio_landscape / 3;
        landscape_frect.w = landscape_frect.h / window_ratio_landscape;
        landscape_frect.y = 98 - landscape_frect.h;
        landscape_frect.x = 2 * landscape_frect.w + x_controls_offset_landscape;
    } else if (!original_event.compare(CONFIGURATION_DISC_KEY)) {
        portrait_frect.w = w_controls_portrait_disc_size / window_ratio_portrait;
        portrait_frect.h = portrait_frect.w * window_ratio_portrait;
        portrait_frect.x = 99 - portrait_frect.w;
        portrait_frect.y = 99 - portrait_frect.h;
        landscape_frect.h = h_controls_landscape_disc_size * window_ratio_landscape;
        landscape_frect.w = landscape_frect.h / window_ratio_landscape;
        landscape_frect.y = 98 - landscape_frect.h;
        landscape_frect.x = 98 - landscape_frect.w;
    } else if (!original_event.compare(CONFIGURATION_COVER)) {
        portrait_frect.w = w_controls_portrait_disc_size / window_ratio_portrait;
        portrait_frect.h = portrait_frect.w * window_ratio_portrait;
        portrait_frect.x = 99 - portrait_frect.w;
        portrait_frect.y = 99 - portrait_frect.h;
        portrait_frect.x = 0;
        portrait_frect.w = 100;
        portrait_frect.h += 5;
        landscape_frect.h = h_controls_landscape_disc_size * window_ratio_landscape;
        landscape_frect.w = landscape_frect.h / window_ratio_landscape;
        landscape_frect.y = 98 - landscape_frect.h;
        landscape_frect.x = 98 - landscape_frect.w;
        landscape_frect.x = 0;
        landscape_frect.w = 100;
        landscape_frect.h += 5;
        landscape_frect.y -= 1;
    } else if (!original_event.compare(CONFIGURATION_SAVED_GLOBAL_TOASTER) || !original_event.compare(CONFIGURATION_SAVED_FOR_GAME_TOASTER)) {
        portrait_frect.w = 80;
        portrait_frect.h = 4;
        portrait_frect.x = 10;
        portrait_frect.y = 95;
        landscape_frect.w = 50;
        landscape_frect.h = 8;
        landscape_frect.x = 25;
        landscape_frect.y = 90;
    } else if (!original_event.compare(CONFIGURATION_SWITCH_MODE)) {
        portrait_frect.w = 9;
        portrait_frect.h = portrait_frect.w * window_ratio_portrait;
        portrait_frect.x = 90;
        portrait_frect.y = 0.5;
        landscape_frect.h = 9;
        landscape_frect.w = landscape_frect.h / window_ratio_landscape;
        landscape_frect.y = 1;
        landscape_frect.x = 92;
        // ECS
    } else if (startsWith(original_event.c_str(), "KEYB")) {
        alpha_portrait = 255 - app_config_struct.mobile_ecs_portrait_alpha;
        alpha_landscape = 255 - app_config_struct.mobile_ecs_landscape_alpha;
        if (!original_event.compare(KEYB_1)) {
            compute_ecs_position_and_size(&portrait_frect, 0, 0, true);
            compute_ecs_position_and_size(&landscape_frect, 0, 0, false);
        } else if (!original_event.compare(KEYB_2)) {
            compute_ecs_position_and_size(&portrait_frect, 1, 0, true);
            compute_ecs_position_and_size(&landscape_frect, 1, 0, false);
        } else if (!original_event.compare(KEYB_3)) {
            compute_ecs_position_and_size(&portrait_frect, 2, 0, true);
            compute_ecs_position_and_size(&landscape_frect, 2, 0, false);
        } else if (!original_event.compare(KEYB_4)) {
            compute_ecs_position_and_size(&portrait_frect, 3, 0, true);
            compute_ecs_position_and_size(&landscape_frect, 3, 0, false);
        } else if (!original_event.compare(KEYB_5)) {
            compute_ecs_position_and_size(&portrait_frect, 4, 0, true);
            compute_ecs_position_and_size(&landscape_frect, 4, 0, false);
        } else if (!original_event.compare(KEYB_6)) {
            compute_ecs_position_and_size(&portrait_frect, 5, 0, true);
            compute_ecs_position_and_size(&landscape_frect, 5, 0, false);
        } else if (!original_event.compare(KEYB_7)) {
            compute_ecs_position_and_size(&portrait_frect, 6, 0, true);
            compute_ecs_position_and_size(&landscape_frect, 6, 0, false);
        } else if (!original_event.compare(KEYB_8)) {
            compute_ecs_position_and_size(&portrait_frect, 7, 0, true);
            compute_ecs_position_and_size(&landscape_frect, 7, 0, false);
        } else if (!original_event.compare(KEYB_9)) {
            compute_ecs_position_and_size(&portrait_frect, 8, 0, true);
            compute_ecs_position_and_size(&landscape_frect, 8, 0, false);
        } else if (!original_event.compare(KEYB_0)) {
            compute_ecs_position_and_size(&portrait_frect, 9, 0, true);
            compute_ecs_position_and_size(&landscape_frect, 9, 0, false);
        } else if (!original_event.compare(KEYB_ESC)) {
            compute_ecs_position_and_size(&portrait_frect, 10, 0, true);
            compute_ecs_position_and_size(&landscape_frect, 10, 0, false);
        } else if (!original_event.compare(KEYB_CTRL)) {
            compute_ecs_position_and_size(&portrait_frect, 0, 1, true);
            compute_ecs_position_and_size(&landscape_frect, 0, 1, false);
        } else if (!original_event.compare(KEYB_Q)) {
            compute_ecs_position_and_size(&portrait_frect, 1, 1, true);
            compute_ecs_position_and_size(&landscape_frect, 1, 1, false);
        } else if (!original_event.compare(KEYB_W)) {
            compute_ecs_position_and_size(&portrait_frect, 2, 1, true);
            compute_ecs_position_and_size(&landscape_frect, 2, 1, false);
        } else if (!original_event.compare(KEYB_E)) {
            compute_ecs_position_and_size(&portrait_frect, 3, 1, true);
            compute_ecs_position_and_size(&landscape_frect, 3, 1, false);
        } else if (!original_event.compare(KEYB_R)) {
            compute_ecs_position_and_size(&portrait_frect, 4, 1, true);
            compute_ecs_position_and_size(&landscape_frect, 4, 1, false);
        } else if (!original_event.compare(KEYB_T)) {
            compute_ecs_position_and_size(&portrait_frect, 5, 1, true);
            compute_ecs_position_and_size(&landscape_frect, 5, 1, false);
        } else if (!original_event.compare(KEYB_Y)) {
            compute_ecs_position_and_size(&portrait_frect, 6, 1, true);
            compute_ecs_position_and_size(&landscape_frect, 6, 1, false);
        } else if (!original_event.compare(KEYB_U)) {
            compute_ecs_position_and_size(&portrait_frect, 7, 1, true);
            compute_ecs_position_and_size(&landscape_frect, 7, 1, false);
        } else if (!original_event.compare(KEYB_I)) {
            compute_ecs_position_and_size(&portrait_frect, 8, 1, true);
            compute_ecs_position_and_size(&landscape_frect, 8, 1, false);
        } else if (!original_event.compare(KEYB_O)) {
            compute_ecs_position_and_size(&portrait_frect, 9, 1, true);
            compute_ecs_position_and_size(&landscape_frect, 9, 1, false);
        } else if (!original_event.compare(KEYB_P)) {
            compute_ecs_position_and_size(&portrait_frect, 10, 1, true);
            compute_ecs_position_and_size(&landscape_frect, 10, 1, false);
        } else if (!original_event.compare(KEYB_UP)) {
            compute_ecs_position_and_size(&portrait_frect, 0, 2, true);
            compute_ecs_position_and_size(&landscape_frect, 0, 2, false);
        } else if (!original_event.compare(KEYB_A)) {
            compute_ecs_position_and_size(&portrait_frect, 1, 2, true);
            compute_ecs_position_and_size(&landscape_frect, 1, 2, false);
        } else if (!original_event.compare(KEYB_S)) {
            compute_ecs_position_and_size(&portrait_frect, 2, 2, true);
            compute_ecs_position_and_size(&landscape_frect, 2, 2, false);
        } else if (!original_event.compare(KEYB_D)) {
            compute_ecs_position_and_size(&portrait_frect, 3, 2, true);
            compute_ecs_position_and_size(&landscape_frect, 3, 2, false);
        } else if (!original_event.compare(KEYB_F)) {
            compute_ecs_position_and_size(&portrait_frect, 4, 2, true);
            compute_ecs_position_and_size(&landscape_frect, 4, 2, false);
        } else if (!original_event.compare(KEYB_G)) {
            compute_ecs_position_and_size(&portrait_frect, 5, 2, true);
            compute_ecs_position_and_size(&landscape_frect, 5, 2, false);
        } else if (!original_event.compare(KEYB_H)) {
            compute_ecs_position_and_size(&portrait_frect, 6, 2, true);
            compute_ecs_position_and_size(&landscape_frect, 6, 2, false);
        } else if (!original_event.compare(KEYB_J)) {
            compute_ecs_position_and_size(&portrait_frect, 7, 2, true);
            compute_ecs_position_and_size(&landscape_frect, 7, 2, false);
        } else if (!original_event.compare(KEYB_K)) {
            compute_ecs_position_and_size(&portrait_frect, 8, 2, true);
            compute_ecs_position_and_size(&landscape_frect, 8, 2, false);
        } else if (!original_event.compare(KEYB_L)) {
            compute_ecs_position_and_size(&portrait_frect, 9, 2, true);
            compute_ecs_position_and_size(&landscape_frect, 9, 2, false);
        } else if (!original_event.compare(KEYB_SEMI)) {
            compute_ecs_position_and_size(&portrait_frect, 10, 2, true);
            compute_ecs_position_and_size(&landscape_frect, 10, 2, false);
        } else if (!original_event.compare(KEYB_LEFT)) {
            compute_ecs_position_and_size(&portrait_frect, 0, 3, true);
            compute_ecs_position_and_size(&landscape_frect, 0, 3, false);
        } else if (!original_event.compare(KEYB_RIGHT)) {
            compute_ecs_position_and_size(&portrait_frect, 1, 3, true);
            compute_ecs_position_and_size(&landscape_frect, 1, 3, false);
        } else if (!original_event.compare(KEYB_Z)) {
            compute_ecs_position_and_size(&portrait_frect, 2, 3, true);
            compute_ecs_position_and_size(&landscape_frect, 2, 3, false);
        } else if (!original_event.compare(KEYB_X)) {
            compute_ecs_position_and_size(&portrait_frect, 3, 3, true);
            compute_ecs_position_and_size(&landscape_frect, 3, 3, false);
        } else if (!original_event.compare(KEYB_C)) {
            compute_ecs_position_and_size(&portrait_frect, 4, 3, true);
            compute_ecs_position_and_size(&landscape_frect, 4, 3, false);
        } else if (!original_event.compare(KEYB_V)) {
            compute_ecs_position_and_size(&portrait_frect, 5, 3, true);
            compute_ecs_position_and_size(&landscape_frect, 5, 3, false);
        } else if (!original_event.compare(KEYB_B)) {
            compute_ecs_position_and_size(&portrait_frect, 6, 3, true);
            compute_ecs_position_and_size(&landscape_frect, 6, 3, false);
        } else if (!original_event.compare(KEYB_N)) {
            compute_ecs_position_and_size(&portrait_frect, 7, 3, true);
            compute_ecs_position_and_size(&landscape_frect, 7, 3, false);
        } else if (!original_event.compare(KEYB_M)) {
            compute_ecs_position_and_size(&portrait_frect, 8, 3, true);
            compute_ecs_position_and_size(&landscape_frect, 8, 3, false);
        } else if (!original_event.compare(KEYB_COMMA)) {
            compute_ecs_position_and_size(&portrait_frect, 9, 3, true);
            compute_ecs_position_and_size(&landscape_frect, 9, 3, false);
        } else if (!original_event.compare(KEYB_PERIOD)) {
            compute_ecs_position_and_size(&portrait_frect, 10, 3, true);
            compute_ecs_position_and_size(&landscape_frect, 10, 3, false);
        } else if (!original_event.compare(KEYB_DOWN)) {
            compute_ecs_position_and_size(&portrait_frect, 0, 4, true);
            compute_ecs_position_and_size(&landscape_frect, 0, 4, false);
        } else if (!original_event.compare(KEYB_SHIFT)) {
            compute_ecs_position_and_size(&portrait_frect, 1, 4, true);
            compute_ecs_position_and_size(&landscape_frect, 1, 4, false);
        } else if (!original_event.compare(KEYB_SPACE)) {
            compute_ecs_position_and_size(&portrait_frect, 2, 4, true, true);
            compute_ecs_position_and_size(&landscape_frect, 2, 4, false, true);
        } else if (!original_event.compare(KEYB_SHIFT_2)) {
            compute_ecs_position_and_size(&portrait_frect, 3, 4, true);
            compute_ecs_position_and_size(&landscape_frect, 3, 4, false);
        } else if (!original_event.compare(KEYB_ENTER)) {
            compute_ecs_position_and_size(&portrait_frect, 4, 4, true);
            compute_ecs_position_and_size(&landscape_frect, 4, 4, false);
        }
    } else {
        portrait_frect.x = 0;
        portrait_frect.y = 0;
        portrait_frect.w = 20;
        portrait_frect.h = 20;
        landscape_frect.x = 0;
        landscape_frect.y = 0;
        landscape_frect.w = 20;
        landscape_frect.h = 20;
    }
}

string Control::get_effective_event() {
    string eve = override_event.length() > 0 ? override_event : original_event;
    return eve;
}

bool Control::is_in_control(float checked_x, float checked_y, SDL_FRect *real_pos) {
    SDL_FRect *act_frect = get_control_frect();
    bool is_in_square = (checked_x >= act_frect->x && checked_x <= act_frect->x + act_frect->w) &&
                        (checked_y >= act_frect->y && checked_y <= act_frect->y + act_frect->h);
    if (!is_disc) {
        if (is_in_square) {
            return true;
        }
    } else if (is_in_square) {
        // Checking ellipse formula
        //  ( x_p - x_c )^2 / a^2 + ( y_p - y_c )^2 / b^2

        float centerX = (act_frect->w / 2 + act_frect->x);
        float a = ((float) act_frect->w) / 2; // a
        float discX = checked_x - centerX; // ( x_p - x_c )
        float distanceEllipseX = discX * discX; // ( x_p - x_c )^2
        float a2 = a * a; // a^2

        float centerY = (act_frect->h / 2 + act_frect->y);
        float b = ((float) act_frect->h) / 2; // b
        float discY = checked_y - centerY; // ( y_p - y_c )
        float distanceEllipseY = discY * discY; // ( y_p - y_c )^2
        float b2 = b * b; // b^2

        float result = (((float) distanceEllipseX) / a2) + (((float) distanceEllipseY) / b2);
        if (result < 1) {
            if (real_pos != nullptr) {
                real_pos->x = discX;
                real_pos->y = discY;
            }
            return true;
        }
    }
    return false;
}

string get_default_file(string event, bool pressed);

bool Control::normalize_to_delta(Control *parent_control, bool keep_univisible) {
    bool res = true;
    if (portrait_frect.x == parent_control->portrait_frect.x || !original_event.compare(DISC_DIRECTION_KEY)) {
        portrait_frect.x = -1;
    } else {
        res = false;
    }

    if (portrait_frect.y == parent_control->portrait_frect.y || !original_event.compare(DISC_DIRECTION_KEY)) {
        portrait_frect.y = -1;
    } else {
        res = false;
    }

    if (portrait_frect.w == parent_control->portrait_frect.w || !original_event.compare(DISC_DIRECTION_KEY)) {
        portrait_frect.w = -1;
    } else {
        res = false;
    }

    if (portrait_frect.h == parent_control->portrait_frect.h || !original_event.compare(DISC_DIRECTION_KEY)) {
        portrait_frect.h = -1;
    } else {
        res = false;
    }

    if (landscape_frect.x == parent_control->landscape_frect.x || !original_event.compare(DISC_DIRECTION_KEY)) {
        landscape_frect.x = -1;
    } else {
        res = false;
    }

    if (landscape_frect.y == parent_control->landscape_frect.y || !original_event.compare(DISC_DIRECTION_KEY)) {
        landscape_frect.y = -1;
    } else {
        res = false;
    }

    if (landscape_frect.w == parent_control->landscape_frect.w || !original_event.compare(DISC_DIRECTION_KEY)) {
        landscape_frect.w = -1;
    } else {
        res = false;
    }

    if (landscape_frect.h == parent_control->landscape_frect.h || !original_event.compare(DISC_DIRECTION_KEY)) {
        landscape_frect.h = -1;
    } else {
        res = false;
    }

    if ((!keep_univisible || is_visible != 0) && (is_visible == parent_control->is_visible || (parent_control->is_visible == -1 && is_visible == 1))) {
        is_visible = -1;
    } else {
        if (!original_event.compare(DISC_DIRECTION_KEY)) {
            is_visible = -1;
        } else {
            res = false;
        }
    }

    if (alpha_portrait == parent_control->alpha_portrait || (parent_control->alpha_portrait == -1 && alpha_portrait == DEFAULT_ALPHA_CONFIG)) {
        alpha_portrait = -1;
    } else {
        res = false;
    }

    if (alpha_landscape == parent_control->alpha_landscape || (parent_control->alpha_landscape == -1 && alpha_landscape == DEFAULT_ALPHA_CONFIG)) {
        alpha_landscape = -1;
    } else {
        res = false;
    }

    string default_released = get_default_file(original_event, 1);
    if (!file_name_released.compare(parent_control->file_name_released) || (parent_control->file_name_released.empty() && !file_name_released.compare(default_released))) {
        file_name_released = "";
    } else {
        res = false;
    }

    string default_pressed = get_default_file(original_event, 0);
    if (!file_name_pressed.compare(parent_control->file_name_pressed) || (parent_control->file_name_pressed.empty() && !file_name_pressed.compare(default_pressed))) {
        file_name_pressed = "";
    } else {
        res = false;
    }

    if (!override_event.compare(parent_control->override_event)) {
        override_event = "";
    } else {
        res = false;
    }
    return res;
}

bool Control::normalize_from_delta(Control *parent_control) {
    bool res = true;
    if (portrait_frect.x == -1) {
        portrait_frect.x = parent_control->portrait_frect.x;
    } else {
        res = false;
    }

    if (portrait_frect.y == -1) {
        portrait_frect.y = parent_control->portrait_frect.y;
    } else {
        res = false;
    }

    if (portrait_frect.w == -1) {
        portrait_frect.w = parent_control->portrait_frect.w;
    } else {
        res = false;
    }

    if (portrait_frect.h == -1) {
        portrait_frect.h = parent_control->portrait_frect.h;
    } else {
        res = false;
    }

    if (landscape_frect.x == -1) {
        landscape_frect.x = parent_control->landscape_frect.x;
    } else {
        res = false;
    }

    if (landscape_frect.y == -1) {
        landscape_frect.y = parent_control->landscape_frect.y;
    } else {
        res = false;
    }

    if (landscape_frect.w == -1) {
        landscape_frect.w = parent_control->landscape_frect.w;
    } else {
        res = false;
    }

    if (landscape_frect.h == -1) {
        landscape_frect.h = parent_control->landscape_frect.h;
    } else {
        res = false;
    }

    if (is_visible == -1) {
        is_visible = parent_control->is_visible;
    } else {
        res = false;
    }

    if (alpha_portrait == -1) {
        alpha_portrait = parent_control->alpha_portrait;
    } else {
        res = false;
    }

    if (alpha_landscape == -1) {
        alpha_landscape = parent_control->alpha_landscape;
    } else {
        res = false;
    }

    if (file_name_released.empty()) {
        file_name_released = parent_control->file_name_released.c_str();
    } else {
        res = false;
    }

    if (file_name_pressed.empty()) {
        file_name_pressed = parent_control->file_name_pressed.c_str();
    } else {
        res = false;
    }

    if (override_event.empty()) {
        override_event = parent_control->override_event.c_str();
    } else {
        res = false;
    }
    return res;
}

static SDL_Texture *IMG_LoadTexture_custom(SDL_Renderer *renderer, const char *file) {
    SDL_Texture *texture = nullptr;
    SDL_Surface *surface = IMG_Load(file);
    if (surface) {
        SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 0xFF, 0xFF, 0xFF));
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }
    return texture;
}

static SDL_Texture *get_texture(const char *file, SDL_Renderer *renderer, int alpha) {
    char buf[FILENAME_MAX];
    SDL_Texture *res = nullptr;
    sprintf(buf, "%s%s/%s", app_config_struct.resource_folder_absolute_path, "Images/Controls", file);
    if (!exist_file(buf)) {
        ADD_POPUP("File not found", "File not found:'" << file << "'");
    } else {
        res = IMG_LoadTexture_custom(renderer, buf);
        if (res == nullptr) {
            ADD_POPUP("File not loaded", "Unable to load file:'" << file << "'\nError:" << SDL_GetError());
        } else {
            SDL_SetTextureBlendMode(res, SDL_BLENDMODE_BLEND);
            SDL_SetTextureAlphaMod(res, alpha);
        }
    }
    return res;
}

Control *add_new_control(string event, vector<Control *> *container, bool set_position_and_size = true) {
    if (container != nullptr) {
        Control *c = allocate_control();

        container->push_back(c);
        bool is_disc = !(event.compare(DISC_KEY));
        bool is_disc_direction_control_position = !(event.compare(DISC_DIRECTION_KEY));
        if (is_disc) {
            c->is_disc = true;
            c->num_positions = 16;
        } else if (is_disc_direction_control_position) {
            c->is_pressable = false;
        }
        c->original_event = event.c_str();
        if (set_position_and_size) {
            c->set_default_position_and_size(0);
        }
        return c;
    }
    return nullptr;
}

Control *get_control_by_event(string event, vector<Control *> *container) {
    if (container != nullptr) {
        for (int i = 0; i < container->size(); i++) {
            if (!((*container)[i]->original_event.compare(event))) {
                return (*container)[i];
            }
        }
    }
    return nullptr;
}

Control *get_effective_game_control_by_event(string event, int hand_index) {
    return get_control_by_event(event, &(effective_game_controls[hand_index]));
}


std::map<string, string> files_map;
std::map<string, string> files_pressed_map;

void init_files_default_map() {
    files_map.insert({PD0L_KP1, "button_1.png"});
    files_pressed_map.insert({PD0L_KP1, "button_1_pressed.png"});

    files_map.insert({PD0L_KP2, "button_2.png"});
    files_pressed_map.insert({PD0L_KP2, "button_2_pressed.png"});

    files_map.insert({PD0L_KP3, "button_3.png"});
    files_pressed_map.insert({PD0L_KP3, "button_3_pressed.png"});

    files_map.insert({PD0L_KP4, "button_4.png"});
    files_pressed_map.insert({PD0L_KP4, "button_4_pressed.png"});

    files_map.insert({PD0L_KP5, "button_5.png"});
    files_pressed_map.insert({PD0L_KP5, "button_5_pressed.png"});

    files_map.insert({PD0L_KP6, "button_6.png"});
    files_pressed_map.insert({PD0L_KP6, "button_6_pressed.png"});

    files_map.insert({PD0L_KP7, "button_7.png"});
    files_pressed_map.insert({PD0L_KP7, "button_7_pressed.png"});

    files_map.insert({PD0L_KP8, "button_8.png"});
    files_pressed_map.insert({PD0L_KP8, "button_8_pressed.png"});

    files_map.insert({PD0L_KP9, "button_9.png"});
    files_pressed_map.insert({PD0L_KP9, "button_9_pressed.png"});

    files_map.insert({PD0L_KPC, "button_clear.png"});
    files_pressed_map.insert({PD0L_KPC, "button_clear_pressed.png"});

    files_map.insert({PD0L_KP0, "button_0.png"});
    files_pressed_map.insert({PD0L_KP0, "button_0_pressed.png"});

    files_map.insert({PD0L_KPE, "button_enter.png"});
    files_pressed_map.insert({PD0L_KPE, "button_enter_pressed.png"});

    files_map.insert({PD0L_A_T, "button_side.png"});
    files_pressed_map.insert({PD0L_A_T, "button_side_pressed.png"});

    files_map.insert({PD0L_A_L, "button_side.png"});
    files_pressed_map.insert({PD0L_A_L, "button_side_pressed.png"});

    files_map.insert({PD0L_A_R, "button_side.png"});
    files_pressed_map.insert({PD0L_A_R, "button_side_pressed.png"});

    files_map.insert({PD0R_KP1, "button_1.png"});
    files_pressed_map.insert({PD0R_KP1, "button_1_pressed.png"});

    files_map.insert({PD0R_KP2, "button_2.png"});
    files_pressed_map.insert({PD0R_KP2, "button_2_pressed.png"});

    files_map.insert({PD0R_KP3, "button_3.png"});
    files_pressed_map.insert({PD0R_KP3, "button_3_pressed.png"});

    files_map.insert({PD0R_KP4, "button_4.png"});
    files_pressed_map.insert({PD0R_KP4, "button_4_pressed.png"});

    files_map.insert({PD0R_KP5, "button_5.png"});
    files_pressed_map.insert({PD0R_KP5, "button_5_pressed.png"});

    files_map.insert({PD0R_KP6, "button_6.png"});
    files_pressed_map.insert({PD0R_KP6, "button_6_pressed.png"});

    files_map.insert({PD0R_KP7, "button_7.png"});
    files_pressed_map.insert({PD0R_KP7, "button_7_pressed.png"});

    files_map.insert({PD0R_KP8, "button_8.png"});
    files_pressed_map.insert({PD0R_KP8, "button_8_pressed.png"});

    files_map.insert({PD0R_KP9, "button_9.png"});
    files_pressed_map.insert({PD0R_KP9, "button_9_pressed.png"});

    files_map.insert({PD0R_KPC, "button_clear.png"});
    files_pressed_map.insert({PD0R_KPC, "button_clear_pressed.png"});

    files_map.insert({PD0R_KP0, "button_0.png"});
    files_pressed_map.insert({PD0R_KP0, "button_0_pressed.png"});

    files_map.insert({PD0R_KPE, "button_enter.png"});
    files_pressed_map.insert({PD0R_KPE, "button_enter_pressed.png"});

    files_map.insert({PD0R_A_T, "button_side.png"});
    files_pressed_map.insert({PD0R_A_T, "button_side_pressed.png"});

    files_map.insert({PD0R_A_L, "button_side.png"});
    files_pressed_map.insert({PD0R_A_L, "button_side_pressed.png"});

    files_map.insert({PD0R_A_R, "button_side.png"});
    files_pressed_map.insert({PD0R_A_R, "button_side_pressed.png"});

    files_map.insert({DISC_KEY, "disc.png"});
    files_pressed_map.insert({DISC_KEY, "disc.png"});

    files_map.insert({DISC_DIRECTION_KEY, "disc_direction.png"});

    files_map.insert({PAUSE, "button_pause.png"});
    files_pressed_map.insert({PAUSE, "button_pause_pressed.png"});

    files_map.insert({RESET, "button_reset.png"});
    files_pressed_map.insert({RESET, "button_reset_pressed.png"});

    files_map.insert({QUIT, "button_quit.png"});
    files_pressed_map.insert({QUIT, "button_quit_pressed.png"});

    files_map.insert({CHANGE_PLAYER, "button_change_player.png"});
    files_pressed_map.insert({CHANGE_PLAYER, "button_change_player_pressed.png"});

    files_map.insert({KEYBOARD, "button_keyboard.png"});
    files_pressed_map.insert({KEYBOARD, "button_keyboard_pressed.png"});

    files_map.insert({SHOT, "button_shot.png"});
    files_pressed_map.insert({SHOT, "button_shot_pressed.png"});

    files_map.insert({PLAYER_SELECTED, "button_player_selected.png"});
    files_pressed_map.insert({PLAYER_SELECTED, "button_player_selected.png"});

    files_map.insert({PLAYER_NOT_SELECTED, "button_player_not_selected.png"});
    files_pressed_map.insert({PLAYER_NOT_SELECTED, "button_player_not_selected.png"});

    files_map.insert({KEYB_1, "Ecs/ecs_1.png"});
    files_pressed_map.insert({KEYB_1, "Ecs/ecs_1_pressed.png"});

    files_map.insert({KEYB_2, "Ecs/ecs_2.png"});
    files_pressed_map.insert({KEYB_2, "Ecs/ecs_2_pressed.png"});

    files_map.insert({KEYB_3, "Ecs/ecs_3.png"});
    files_pressed_map.insert({KEYB_3, "Ecs/ecs_3_pressed.png"});

    files_map.insert({KEYB_4, "Ecs/ecs_4.png"});
    files_pressed_map.insert({KEYB_4, "Ecs/ecs_4_pressed.png"});

    files_map.insert({KEYB_5, "Ecs/ecs_5.png"});
    files_pressed_map.insert({KEYB_5, "Ecs/ecs_5_pressed.png"});

    files_map.insert({KEYB_6, "Ecs/ecs_6.png"});
    files_pressed_map.insert({KEYB_6, "Ecs/ecs_6_pressed.png"});

    files_map.insert({KEYB_7, "Ecs/ecs_7.png"});
    files_pressed_map.insert({KEYB_7, "Ecs/ecs_7_pressed.png"});

    files_map.insert({KEYB_8, "Ecs/ecs_8.png"});
    files_pressed_map.insert({KEYB_8, "Ecs/ecs_8_pressed.png"});

    files_map.insert({KEYB_9, "Ecs/ecs_9.png"});
    files_pressed_map.insert({KEYB_9, "Ecs/ecs_9_pressed.png"});

    files_map.insert({KEYB_0, "Ecs/ecs_0.png"});
    files_pressed_map.insert({KEYB_0, "Ecs/ecs_0_pressed.png"});

    files_map.insert({KEYB_ESC, "Ecs/ecs_esc.png"});
    files_pressed_map.insert({KEYB_ESC, "Ecs/ecs_esc_pressed.png"});

    files_map.insert({KEYB_CTRL, "Ecs/ecs_ctl.png"});
    files_pressed_map.insert({KEYB_CTRL, "Ecs/ecs_ctl_pressed.png"});

    files_map.insert({KEYB_Q, "Ecs/ecs_q.png"});
    files_pressed_map.insert({KEYB_Q, "Ecs/ecs_q_pressed.png"});

    files_map.insert({KEYB_W, "Ecs/ecs_w.png"});
    files_pressed_map.insert({KEYB_W, "Ecs/ecs_w_pressed.png"});

    files_map.insert({KEYB_E, "Ecs/ecs_e.png"});
    files_pressed_map.insert({KEYB_E, "Ecs/ecs_e_pressed.png"});

    files_map.insert({KEYB_R, "Ecs/ecs_r.png"});
    files_pressed_map.insert({KEYB_R, "Ecs/ecs_r_pressed.png"});

    files_map.insert({KEYB_T, "Ecs/ecs_t.png"});
    files_pressed_map.insert({KEYB_T, "Ecs/ecs_t_pressed.png"});

    files_map.insert({KEYB_Y, "Ecs/ecs_y.png"});
    files_pressed_map.insert({KEYB_Y, "Ecs/ecs_y_pressed.png"});

    files_map.insert({KEYB_U, "Ecs/ecs_u.png"});
    files_pressed_map.insert({KEYB_U, "Ecs/ecs_u_pressed.png"});

    files_map.insert({KEYB_I, "Ecs/ecs_i.png"});
    files_pressed_map.insert({KEYB_I, "Ecs/ecs_i_pressed.png"});

    files_map.insert({KEYB_O, "Ecs/ecs_o.png"});
    files_pressed_map.insert({KEYB_O, "Ecs/ecs_o_pressed.png"});

    files_map.insert({KEYB_P, "Ecs/ecs_p.png"});
    files_pressed_map.insert({KEYB_P, "Ecs/ecs_p_pressed.png"});

    files_map.insert({KEYB_UP, "Ecs/ecs_up.png"});
    files_pressed_map.insert({KEYB_UP, "Ecs/ecs_up_pressed.png"});

    files_map.insert({KEYB_A, "Ecs/ecs_a.png"});
    files_pressed_map.insert({KEYB_A, "Ecs/ecs_a_pressed.png"});

    files_map.insert({KEYB_S, "Ecs/ecs_s.png"});
    files_pressed_map.insert({KEYB_S, "Ecs/ecs_s_pressed.png"});

    files_map.insert({KEYB_D, "Ecs/ecs_d.png"});
    files_pressed_map.insert({KEYB_D, "Ecs/ecs_d_pressed.png"});

    files_map.insert({KEYB_F, "Ecs/ecs_f.png"});
    files_pressed_map.insert({KEYB_F, "Ecs/ecs_f_pressed.png"});

    files_map.insert({KEYB_G, "Ecs/ecs_g.png"});
    files_pressed_map.insert({KEYB_G, "Ecs/ecs_g_pressed.png"});

    files_map.insert({KEYB_H, "Ecs/ecs_h.png"});
    files_pressed_map.insert({KEYB_H, "Ecs/ecs_h_pressed.png"});

    files_map.insert({KEYB_J, "Ecs/ecs_j.png"});
    files_pressed_map.insert({KEYB_J, "Ecs/ecs_j_pressed.png"});

    files_map.insert({KEYB_K, "Ecs/ecs_k.png"});
    files_pressed_map.insert({KEYB_K, "Ecs/ecs_k_pressed.png"});

    files_map.insert({KEYB_L, "Ecs/ecs_l.png"});
    files_pressed_map.insert({KEYB_L, "Ecs/ecs_l_pressed.png"});

    files_map.insert({KEYB_SEMI, "Ecs/ecs_semi.png"});
    files_pressed_map.insert({KEYB_SEMI, "Ecs/ecs_semi_pressed.png"});

    files_map.insert({KEYB_LEFT, "Ecs/ecs_left.png"});
    files_pressed_map.insert({KEYB_LEFT, "Ecs/ecs_left_pressed.png"});

    files_map.insert({KEYB_RIGHT, "Ecs/ecs_right.png"});
    files_pressed_map.insert({KEYB_RIGHT, "Ecs/ecs_right_pressed.png"});

    files_map.insert({KEYB_Z, "Ecs/ecs_z.png"});
    files_pressed_map.insert({KEYB_Z, "Ecs/ecs_z_pressed.png"});

    files_map.insert({KEYB_X, "Ecs/ecs_x.png"});
    files_pressed_map.insert({KEYB_X, "Ecs/ecs_x_pressed.png"});

    files_map.insert({KEYB_C, "Ecs/ecs_c.png"});
    files_pressed_map.insert({KEYB_C, "Ecs/ecs_c_pressed.png"});

    files_map.insert({KEYB_V, "Ecs/ecs_v.png"});
    files_pressed_map.insert({KEYB_V, "Ecs/ecs_v_pressed.png"});

    files_map.insert({KEYB_B, "Ecs/ecs_b.png"});
    files_pressed_map.insert({KEYB_B, "Ecs/ecs_b_pressed.png"});

    files_map.insert({KEYB_N, "Ecs/ecs_n.png"});
    files_pressed_map.insert({KEYB_N, "Ecs/ecs_n_pressed.png"});

    files_map.insert({KEYB_M, "Ecs/ecs_m.png"});
    files_pressed_map.insert({KEYB_M, "Ecs/ecs_m_pressed.png"});

    files_map.insert({KEYB_COMMA, "Ecs/ecs_comma.png"});
    files_pressed_map.insert({KEYB_COMMA, "Ecs/ecs_comma_pressed.png"});

    files_map.insert({KEYB_PERIOD, "Ecs/ecs_period.png"});
    files_pressed_map.insert({KEYB_PERIOD, "Ecs/ecs_period_pressed.png"});

    files_map.insert({KEYB_DOWN, "Ecs/ecs_down.png"});
    files_pressed_map.insert({KEYB_DOWN, "Ecs/ecs_down_pressed.png"});

    files_map.insert({KEYB_SHIFT, "Ecs/ecs_shift.png"});
    files_pressed_map.insert({KEYB_SHIFT, "Ecs/ecs_shift_pressed.png"});

    files_map.insert({KEYB_SPACE, "Ecs/ecs_space.png"});
    files_pressed_map.insert({KEYB_SPACE, "Ecs/ecs_space_pressed.png"});

    files_map.insert({KEYB_SHIFT_2, "Ecs/ecs_shift.png"});
    files_pressed_map.insert({KEYB_SHIFT_2, "Ecs/ecs_shift_pressed.png"});

    files_map.insert({KEYB_ENTER, "Ecs/ecs_return.png"});
    files_pressed_map.insert({KEYB_ENTER, "Ecs/ecs_return_pressed.png"});
}

string get_default_file(string event, bool pressed) {
    if (!pressed) {
        std::map<string, string>::iterator pos = files_map.find(event);
        if (pos != files_map.end()) {
            return pos->second;
        } else {
            return "default.png";
        }
    } else {
        std::map<string, string>::iterator pos = files_pressed_map.find(event);
        if (pos != files_pressed_map.end()) {
            return pos->second;
        } else {
            return "default_pressed.png";
        }
    }
}

int get_hand_index(const char *event) {
    string input_event = event;
    int len = input_event.length();
    if (len <= 3) {
        THROW_BY_STREAM("Invalid event:" << input_event);
    }
    string suffix = input_event.substr(input_event.length() - 3);
    if (suffix.compare("__L") && suffix.compare("__R")) {
        THROW_BY_STREAM("Invalid event:" << input_event);
    }
    int hand_index = suffix.compare("__L") ? 1 : 0;
    return hand_index;
}

static Control *add_control_to_container(const char *event, vector<Control *> *container, int hand_index) {
    if (get_control_by_event(event, &(container[hand_index])) != nullptr) {
        THROW_BY_STREAM("Duplicate section for controls");
    }
    add_event_to_monitor(event);
    Control *c = add_new_control(event, &(container[hand_index]), false);
    return c;
}

static Control *create_control_if_missing(const char *event, vector<Control *> container[2], int hand_index, bool invisible_if_missing, bool *was_missing) {
    Control *ctrl = get_control_by_event(event, &container[hand_index]);
    if (ctrl == nullptr) {
        *was_missing = true;
        ctrl = add_control_to_container(event, container, hand_index);
        ctrl->is_visible = invisible_if_missing ? 0 : 1;
    } else {
        *was_missing = false;
    }
    return ctrl;
}

static Control *add_new_delta_default_control(const char *event, int hand_index) {
    bool was_missing_device = true;
    bool was_missing_delta = true;
    Control *device_ref = create_control_if_missing(event, device_default_controls, hand_index, true, &was_missing_device);
    Control *delta_ref = create_control_if_missing(event, delta_default_controls, hand_index, false, &was_missing_delta);
    if (!was_missing_delta) {
        copy_control(device_ref, delta_ref);
    }
    return delta_ref;
}

static Control *add_new_custom_game_control(const char *event, int hand_index, vector<Control *> container[2]) {
    bool was_missing_device = true;
    bool was_missing_delta = true;
    bool was_missing_custom = true;
    Control *device_ref = create_control_if_missing(event, device_default_controls, hand_index, true, &was_missing_device);
    Control *delta_ref = create_control_if_missing(event, delta_default_controls, hand_index, true, &was_missing_delta);
    if (was_missing_delta) {
        copy_control(device_ref, delta_ref);
    }
    Control *custom_ref = create_control_if_missing(event, container, hand_index, false, &was_missing_custom);
    if (was_missing_custom) {
        copy_control(delta_ref, custom_ref);
    }
    return custom_ref;
}

Control *add_new_delta_default_control_by_undecoded_event(const char *event) {
    int hand_index = get_hand_index(event);
    string input_event = event;
    string real_event = input_event.substr(0, input_event.length() - 3);
    return add_new_delta_default_control(real_event.c_str(), hand_index);
}

void normalize_control_from_delta(string event, vector<Control *> container[2], int hand_index, Control *ref) {
    Control *new_control = get_control_by_event(event, &(container[hand_index]));
    if (new_control == nullptr) {
        new_control = add_new_control(event, &(container[hand_index]));
        copy_control(ref, new_control);
    } else {
        new_control->normalize_from_delta(ref);
    }
}

void normalize_controls_from_delta(int hand_index) {
    // Normalize delta from device
    for (int i = 0; i < device_default_controls[hand_index].size(); i++) {
        Control *device_ref = device_default_controls[hand_index][i];
        device_ref->set_default_position_and_size(hand_index);
        string event = device_ref->original_event;
        normalize_control_from_delta(event, delta_default_controls, hand_index, device_ref);
    }

    // Normalize games from delta
    for (int j = 0; j < delta_default_controls[hand_index].size(); j++) {
        Control *delta_ref = delta_default_controls[hand_index][j];
        string event = delta_ref->original_event;
        const char *control_event = event.c_str();
        for (int i = 0; i < app_config_struct.num_valid_crc32s; i++) {
            rom_config_struct_t &roms_config = roms_configuration[i];
            normalize_control_from_delta(event, roms_config.controls, hand_index, delta_ref);
        }

        for (int i = 0; i < roms_list_struct.total_roms_num; i++) {
            rom_config_struct_t *roms_config = &(roms_list_struct.list[i]);
            normalize_control_from_delta(event, roms_config->controls, hand_index, delta_ref);
        }
    }
}

map<uint32_t, map<int, map<string, string>>> control_filename_pressed_persistent_map;
map<uint32_t, map<int, map<string, string>>> control_filename_released_persistent_map;
map<uint32_t, map<int, map<string, string>>> control_override_persistent_map;

static void read_persistent_values_from_map(map<uint32_t, map<int, map<string, string>>> *map_ptr, uint32_t crc32, int hand_index, map<string, string> *my_map) {

    std::map<uint32_t, std::map<int, std::map<string, string>>>::iterator crc32_it;
    std::map<int, std::map<string, string>>::iterator hand_index_it;
    std::map<string, string>::iterator original_event_it;

    // Crc32
    crc32_it = map_ptr->find(crc32);
    if (crc32_it != map_ptr->end()) {
        map<int, map<string, string>> crc32_map = crc32_it->second;

        // Hand_index
        hand_index_it = crc32_map.find(hand_index);
        if (hand_index_it != crc32_map.end()) {
            map<string, string> hand_index_map = hand_index_it->second;
            *my_map = hand_index_map;
        }
    }
}

static void restore_persistent_values_from_map(map<uint32_t, map<int, map<string, string>>> *map_ptr, string option_name, rom_config_struct_t *config, int hand_index) {
    map<string, string> res;
    read_persistent_values_from_map(map_ptr, config->crc32, hand_index, &res);
    if (!res.empty()) {
        map<string, string>::iterator it;
        for (auto const &x: res) {
            string original_event = x.first;
            string override_event = x.second;
            Control *new_control = add_new_custom_game_control(original_event.c_str(), hand_index, config->controls);
            set_control_value((char *) option_name.c_str(), (char *) override_event.c_str(), new_control);
        }
    }
}

static void restore_persistent_values(rom_config_struct_t *config, int hand_index) {
    restore_persistent_values_from_map(&control_filename_pressed_persistent_map, CONTROL_FILE_NAME_PRESSED_OPTION, config, hand_index);
    restore_persistent_values_from_map(&control_filename_released_persistent_map, CONTROL_FILE_NAME_RELEASED_OPTION, config, hand_index);
    restore_persistent_values_from_map(&control_override_persistent_map, CONTROL_OVERRIDE_EVENT_OPTION, config, hand_index);
}

static void save_persistent_values(char *key, char *value, Control *act_control, int hand_index, struct rom_config_struct_t *rom_config) {
    if (!strcmp(key, CONTROL_FILE_NAME_PRESSED_OPTION)) {
        control_filename_pressed_persistent_map[rom_config->crc32][hand_index][act_control->original_event] = value;
    } else if (!strcmp(key, CONTROL_FILE_NAME_RELEASED_OPTION)) {
        control_filename_released_persistent_map[rom_config->crc32][hand_index][act_control->original_event] = value;
    } else if (!strcmp(key, CONTROL_OVERRIDE_EVENT_OPTION)) {
        control_override_persistent_map[rom_config->crc32][hand_index][act_control->original_event] = value;
    }
}

void remove_custom_controls() {
    clear_sdl_frect(&app_config_struct.mobile_landscape_rect);
    clear_sdl_frect(&app_config_struct.mobile_portrait_rect);
    for (int hand_index = 0; hand_index < 2; hand_index++) {
        duplicate_controls(&(device_default_controls[hand_index]), &delta_default_controls[hand_index]);
        for (int i = 0; i < app_config_struct.num_valid_crc32s; i++) {
            clear_controls(&(roms_configuration[i].controls[hand_index]));
            clear_controls(&(roms_configuration[i].controls_delta[hand_index]));
            restore_persistent_values(&(roms_configuration[i]), hand_index);
            clear_sdl_frect(&roms_configuration[i].mobile_landscape_rect);
            clear_sdl_frect(&roms_configuration[i].mobile_portrait_rect);
        }

        for (int i = 0; i < roms_list_struct.total_roms_num; i++) {
            clear_controls(&(roms_list_struct.list[i].controls[hand_index]));
            clear_controls(&(roms_list_struct.list[i].controls_delta[hand_index]));
            restore_persistent_values(&(roms_list_struct.list[i]), hand_index);
            clear_sdl_frect(&roms_list_struct.list[i].mobile_landscape_rect);
            clear_sdl_frect(&roms_list_struct.list[i].mobile_portrait_rect);
        }
    }
}

// We won't to have only the delta here. We assume that delta_default_controls is already filled (reading from ini will fill it if needed), and we're gonna keep delta just for config and files
void normalize_controls_to_delta(vector<Control *> new_delta_default_controls[2], int hand_index, bool source_is_destination) {
    // source_is_destination = false => when saving ini files, we don't want to change memory full data (controls Vector), so we just need to fill delta data (controls_delta Vector)
    // source_is_destination = true => when starting a game we have to fill full data (controls Vector)..we start to have only delta and later we will fill all
    // source_is_destination = true => when saving global config. We need to purge config and files BEFORE saving
    if (new_delta_default_controls != nullptr) {
        duplicate_controls(&(delta_default_controls[hand_index]), &new_delta_default_controls[hand_index]);
        purge_duplicate_elements(&(device_default_controls[hand_index]), &new_delta_default_controls[hand_index]);
    }

    for (int i = 0; i < app_config_struct.num_valid_crc32s; i++) {
        vector<Control *> *ctrls_to_purge = source_is_destination ? &(roms_configuration[i].controls[hand_index]) : &(roms_configuration[i].controls_delta[hand_index]);
        if (!source_is_destination) {
            vector<Control *> *ctrls_to_copy_from = &(roms_configuration[i].controls[hand_index]);
            duplicate_controls(ctrls_to_copy_from, ctrls_to_purge);
        }
        vector<Control *> *ctrls_from_purge = &(delta_default_controls[hand_index]);
        purge_duplicate_elements(ctrls_from_purge, ctrls_to_purge, false);
    }

    for (int i = 0; i < roms_list_struct.total_roms_num; i++) {
        vector<Control *> *ctrls_to_purge = source_is_destination ? &(roms_list_struct.list[i].controls[hand_index]) : &(roms_list_struct.list[i].controls_delta[hand_index]);
        if (!source_is_destination) {
            vector<Control *> *ctrls_to_copy_from = &(roms_list_struct.list[i].controls[hand_index]);
            duplicate_controls(ctrls_to_copy_from, ctrls_to_purge);
        }
        vector<Control *> *ctrls_from_purge = &(delta_default_controls[hand_index]);
        purge_duplicate_elements(ctrls_from_purge, ctrls_to_purge, false);
    }
}

// At end of ini read
static void normalize_default_control(vector<Control *> container[2], int hand_index) {
    bool has_disc = false;
    bool has_disc_direction = false;
    for (int i = 0; i < container[hand_index].size(); i++) {
        Control *ctrl = container[hand_index][i];
        if (!ctrl->original_event.compare(DISC_KEY)) {
            has_disc = true;
        } else if (!ctrl->original_event.compare(DISC_DIRECTION_KEY)) {
            has_disc_direction = true;
        }
        if (ctrl->file_name_pressed.empty()) {
            ctrl->file_name_pressed = get_default_file(ctrl->original_event, 1);
        }
        if (ctrl->file_name_released.empty()) {
            ctrl->file_name_released = get_default_file(ctrl->original_event, 0);
        }
    }
    if (has_disc && !has_disc_direction) {
        Control *cc = add_control_to_container(string(DISC_DIRECTION_KEY).c_str(), container, hand_index);
        cc->file_name_pressed = get_default_file(cc->original_event, 1);
        cc->file_name_released = get_default_file(cc->original_event, 0);
    }
}

// At end of ini read
void normalize_default_controls() {
    normalize_default_control(device_default_controls, LEFT_HAND_INDEX);
    normalize_default_control(device_default_controls, RIGHT_HAND_INDEX);
    normalize_default_control(delta_default_controls, LEFT_HAND_INDEX);
    normalize_default_control(delta_default_controls, RIGHT_HAND_INDEX);
}

static void copy_control(Control *source, Control *destination) {
    destination->is_visible = source->is_visible;
    destination->alpha_portrait = source->alpha_portrait;
    destination->alpha_landscape = source->alpha_landscape;
    destination->is_disc = source->is_disc;
    destination->num_positions = source->num_positions;
    destination->jzintv_event_index = source->jzintv_event_index;
    destination->config_func = source->config_func;
    destination->config_parameter = source->config_parameter;
    destination->act_on_release = source->act_on_release;

    if (!source->file_name_released.empty()) {
        destination->file_name_released = source->file_name_released.c_str();
    }
    if (!source->file_name_pressed.empty()) {
        destination->file_name_pressed = source->file_name_pressed.c_str();
    }
    if (source->override_event.length() > 0) {
        destination->override_event = source->override_event.c_str();
    }
    if (source->original_event.length() > 0) {
        destination->original_event = source->original_event.c_str();
    }

    destination->portrait_frect.x = source->portrait_frect.x;
    destination->portrait_frect.y = source->portrait_frect.y;
    destination->portrait_frect.w = source->portrait_frect.w;
    destination->portrait_frect.h = source->portrait_frect.h;
    destination->landscape_frect.x = source->landscape_frect.x;
    destination->landscape_frect.y = source->landscape_frect.y;
    destination->landscape_frect.w = source->landscape_frect.w;
    destination->landscape_frect.h = source->landscape_frect.h;
}

static void assign_event_index(Control *c) {
    string effec = c->get_effective_event();
    for (int j = 0; j < events_needed_vec.size(); j++) {
        if (!effec.compare(events_needed_vec[j])) {
            c->jzintv_event_index = j;
            break;
        }
    }
}

vector<Control *> on_release_controls;

static void add_control_on_release(Control *c) {
    c->act_on_release = true;
    on_release_controls.push_back(c);
}

static int get_draw_position_of_control(string event) {
    for (int i = 0; i < device_default_keys.size(); i++) {
        if (!device_default_keys.at(i).compare(event)) {
            return i;
        }
    }
    return 1000;
}

static bool compareByDefaultPosition(const Control *a, const Control *b) {
    return get_draw_position_of_control(a->original_event) < get_draw_position_of_control(b->original_event);
}

void sort_effective_game_controls(vector<Control *> *vec) {
    std::sort(vec->begin(), vec->end(), compareByDefaultPosition);
}

static void populate_effective_game_controls(vector<Control *> container[2]) {
    duplicate_controls(&container[0], &effective_game_controls[LEFT_HAND_INDEX]);
    sort_effective_game_controls(&effective_game_controls[LEFT_HAND_INDEX]);
    duplicate_controls(&container[1], &effective_game_controls[RIGHT_HAND_INDEX]);
    sort_effective_game_controls(&effective_game_controls[RIGHT_HAND_INDEX]);

    bool error_positions = false;
    ostringstream errors;

    // Assign position for events mapping and other stuff
    for (int hand_index = 0; hand_index < 2; hand_index++) {
        Control *change_player_control = get_effective_game_control_by_event(CHANGE_PLAYER, hand_index);
        add_control_on_release(change_player_control);
        Control *quit_control = get_effective_game_control_by_event(QUIT, hand_index);
        quit_control->config_parameter = hand_index;
        add_control_on_release(quit_control);
        Control *toggle_keyboard = get_effective_game_control_by_event(KEYBOARD, hand_index);
        add_control_on_release(toggle_keyboard);
        Control *ecs_second_shift = get_effective_game_control_by_event(KEYB_SHIFT_2, hand_index);
        ecs_second_shift->override_event = KEYB_SHIFT;

        for (int i = 0; i < device_default_controls[hand_index].size(); i++) {
            string event = device_default_controls[hand_index][i]->original_event;
            Control *c = get_effective_game_control_by_event(event, hand_index);

            std::map<string, vector<string>>::iterator pos = children_map.find(c->original_event);
            if (pos != children_map.end()) {
                vector<string> childrs = pos->second;
                for (int i = 0; i < childrs.size(); i++) {
                    string event = childrs.at(i);
                    Control *control = get_effective_game_control_by_event(event, hand_index);
                    if (control != nullptr) {
                        c->children.push_back(control);
                    }
                }
            }

            if (!c->check_and_fix_size() && c->original_event.compare(DISC_DIRECTION_KEY)) {
                if (!error_positions) {
                    errors << "These controls " << (hand_index == 0 ? "(default layout) " : "(inverted layout)") << " have wrong position/size configuration and have been normalized.\n";
                }
                error_positions = true;
                errors << "\n" << c->original_event;
            }
            assign_event_index(c);
            assign_event_index(device_default_controls[hand_index][i]);
            Control *delta = get_control_by_event(event, &(delta_default_controls[hand_index]));
            if (delta != nullptr) {
                assign_event_index(delta);
            }
        }
    }

    if (error_positions) {
        errors << "\n\nIf you didn't save configuration and you didn't manually fixed ini file, you'll get this warning again";
        string err_string = errors.str();
        trim(err_string);
        ADD_POPUP("Wrong config", err_string);
    }
}

vector<Control *> on_release_controls_queue;

static void enable_disable_pressable(bool enable) {
    for (int i = 0; i < on_release_controls.size(); i++) {
        Control *other_c = on_release_controls.at(i);
        if (enable) {
            other_c->is_pressable = other_c->was_pressable;
        } else {
            other_c->was_pressable = other_c->is_pressable;
            other_c->is_pressable = false;
        }
    }
}

static void add_control_on_release_queue(Control *c) {
    if (on_release_controls_queue.size() == 0) {
        c->millis_for_release = 0;
        on_release_controls_queue.push_back(c);
        enable_disable_pressable(false);
    }
}

void manage_on_release_controls_queue() {
    if (on_release_controls_queue.size() > 0) {
        Control *c = on_release_controls_queue.at(0);
        if (!c->is_pressed) {
            if (c->millis_for_release == 0) {
                c->config_func(&c->config_parameter);
                c->millis_for_release = get_act_millis();
            } else if (c->millis_for_release > 0) {
                long act_millis = get_act_millis();
                if (act_millis - c->millis_for_release >= 100) {
                    c->millis_for_release = -1;
                    enable_disable_pressable(true);
                    on_release_controls_queue.erase(on_release_controls_queue.begin());
                }
            }
        }
    }
};

static void populate_configuration_controls() {
    if (configuration_controls.size() > 0) {
        configuration_controls.clear();
    }
    Control *c;

    // Selection control
    c = add_new_control(CONFIGURATION_SELECTION, &configuration_controls);
    c->config_func = idle;
    c->config_parameter = 0;
    c->is_pressable = false;
    c->file_name_pressed = "configuration_selection.png";
    c->file_name_released = "configuration_selection.png";
    c->is_visible = 0;
    c->alpha_portrait = CONTROLS_ALPHA_ENABLED;
    c->alpha_landscape = CONTROLS_ALPHA_ENABLED;

    // Cover control
    c = add_new_control(CONFIGURATION_COVER, &configuration_controls);
    c->config_func = idle;
    c->config_parameter = 0;
    c->is_pressable = false;
    c->file_name_pressed = "configuration_cover.png";
    c->file_name_released = "configuration_cover.png";
    c->is_visible = 0;
    c->alpha_portrait = 205;
    c->alpha_landscape = 205;

    // Switch mode control
    c = add_new_control(CONFIGURATION_SWITCH_MODE, &configuration_controls);
    c->config_func = switch_mode_callback;
    c->config_parameter = 0;
    c->is_pressable = true;
    c->file_name_pressed = "configuration_pressed.png";
    c->file_name_released = "configuration.png";
    c->is_visible = app_config_struct.mobile_show_configuration_controls;
    c->alpha_portrait = 90;
    c->alpha_landscape = 90;
    add_control_on_release(c);

    // Prev control
    c = add_new_control(CONFIGURATION_PREV, &configuration_controls);
    c->config_func = change_control_callback;
    c->config_parameter = -1;
    c->is_pressable = true;
    c->file_name_pressed = "configuration_prev_pressed.png";
    c->file_name_released = "configuration_prev.png";
    c->is_visible = 0;
    c->alpha_portrait = CONTROLS_ALPHA_ENABLED;
    c->alpha_landscape = CONTROLS_ALPHA_ENABLED;

    // Next control
    c = add_new_control(CONFIGURATION_NEXT, &configuration_controls);
    c->config_func = change_control_callback;
    c->config_parameter = 1;
    c->is_pressable = true;
    c->file_name_pressed = "configuration_next_pressed.png";
    c->file_name_released = "configuration_next.png";
    c->is_visible = 0;
    c->alpha_portrait = CONTROLS_ALPHA_ENABLED;
    c->alpha_landscape = CONTROLS_ALPHA_ENABLED;

    // Visible control
    c = add_new_control(CONFIGURATION_VISIBLE, &configuration_controls);
    c->config_func = change_visibility_callback;
    c->config_parameter = 1;
    c->is_pressable = true;
    c->file_name_pressed = "configuration_visible_pressed.png";
    c->file_name_released = "configuration_visible.png";
    c->is_visible = 0;
    c->alpha_portrait = CONTROLS_ALPHA_ENABLED;
    c->alpha_landscape = CONTROLS_ALPHA_ENABLED;

    // Default control
    c = add_new_control(CONFIGURATION_DEFAULT, &configuration_controls);
    c->config_func = reset_to_default_callback;
    c->config_parameter = 1;
    c->is_pressable = true;
    c->file_name_pressed = "configuration_set_default_pressed.png";
    c->file_name_released = "configuration_set_default.png";
    c->is_visible = 0;
    c->alpha_portrait = CONTROLS_ALPHA_ENABLED;
    c->alpha_landscape = CONTROLS_ALPHA_ENABLED;

    // Brightness less control
    c = add_new_control(CONFIGURATION_BRIGHTNESS_LESS, &configuration_controls);
    c->config_func = change_brightness_callback;
    c->config_parameter = -1;
    c->is_pressable = true;
    c->file_name_pressed = "configuration_brightness_less_pressed.png";
    c->file_name_released = "configuration_brightness_less.png";
    c->is_visible = 0;
    c->continuous_click = true;
    c->alpha_portrait = CONTROLS_ALPHA_ENABLED;
    c->alpha_landscape = CONTROLS_ALPHA_ENABLED;

    // Brightness more control
    c = add_new_control(CONFIGURATION_BRIGHTNESS_MORE, &configuration_controls);
    c->config_func = change_brightness_callback;
    c->config_parameter = 1;
    c->is_pressable = true;
    c->file_name_pressed = "configuration_brightness_more_pressed.png";
    c->file_name_released = "configuration_brightness_more.png";
    c->is_visible = 0;
    c->continuous_click = true;
    c->alpha_portrait = CONTROLS_ALPHA_ENABLED;
    c->alpha_landscape = CONTROLS_ALPHA_ENABLED;

    // Save controls global
    c = add_new_control(CONFIGURATION_SAVE_GLOBAL, &configuration_controls);
    c->config_func = save_changes_callback;
    c->config_parameter = 1;
    c->is_pressable = true;
    c->file_name_pressed = "configuration_save_default_pressed.png";
    c->file_name_released = "configuration_save_default.png";
    c->is_visible = 0;
    c->alpha_portrait = CONTROLS_ALPHA_ENABLED;
    c->alpha_landscape = CONTROLS_ALPHA_ENABLED;
    add_control_on_release(c);

    // Save controls in game
    c = add_new_control(CONFIGURATION_SAVE_GAME, &configuration_controls);
    c->config_func = save_changes_callback;
    c->config_parameter = 2;
    c->is_pressable = true;
    c->file_name_pressed = "configuration_save_game_pressed.png";
    c->file_name_released = "configuration_save_game.png";
    c->is_visible = 0;
    add_control_on_release(c);
    if (config_index != -1) {
        c->alpha_portrait = CONTROLS_ALPHA_ENABLED;
        c->alpha_landscape = CONTROLS_ALPHA_ENABLED;
    } else {
        c->alpha_portrait = CONTROLS_ALPHA_DISABLED;
        c->alpha_landscape = CONTROLS_ALPHA_DISABLED;
        c->is_pressable = false;
    }

    // X-Flip
    c = add_new_control(CONFIGURATION_X_FLIP, &configuration_controls);
    c->config_func = switch_hand_index_callback;
    c->config_parameter = 1;
    c->is_pressable = true;
    c->file_name_pressed = "x_flip_pressed.png";
    c->file_name_released = "x_flip.png";
    c->is_visible = 0;
    c->alpha_portrait = CONTROLS_ALPHA_ENABLED;
    c->alpha_landscape = CONTROLS_ALPHA_ENABLED;
    add_control_on_release(c);

    // Change size control (disc)
    c = add_new_control(CONFIGURATION_DISC_KEY, &configuration_controls);
    c->config_func = change_size_and_position_callback;
    c->config_parameter = 1;
    c->is_disc = true;
    c->num_positions = 8;
    c->is_pressable = true;
    c->file_name_pressed = "configuration_disc.png";
    c->file_name_released = "configuration_disc.png";
    c->is_visible = 0;
    c->continuous_click = true;
    c->alpha_portrait = CONTROLS_ALPHA_ENABLED;
    c->alpha_landscape = CONTROLS_ALPHA_ENABLED;

    // Change size control (disc direction)
    c = add_new_control(CONFIGURATION_DISC_DIRECTION_KEY, &configuration_controls);
    c->config_func = idle;
    c->config_parameter = 1;
    c->is_pressable = true;
    c->file_name_pressed = "configuration_disc_direction.png";
    c->file_name_released = "configuration_disc_direction.png";
    c->is_visible = 0;
    c->alpha_portrait = CONTROLS_ALPHA_ENABLED;
    c->alpha_landscape = CONTROLS_ALPHA_ENABLED;

    // Banner configuration saved as global
    c = add_new_control(CONFIGURATION_SAVED_GLOBAL_TOASTER, &configuration_controls);
    c->config_func = idle;
    c->config_parameter = 0;
    c->is_pressable = false;
    c->file_name_pressed = "configuration_saved_default.png";
    c->file_name_released = "configuration_saved_default.png";
    c->is_visible = 0;

    // Banner configuration saved for game
    c = add_new_control(CONFIGURATION_SAVED_FOR_GAME_TOASTER, &configuration_controls);
    c->config_func = idle;
    c->config_parameter = 0;
    c->is_pressable = false;
    c->file_name_pressed = "configuration_saved_for_game.png";
    c->file_name_released = "configuration_saved_for_game.png";
    c->is_visible = 0;
}

static void populate_pause_control() {
    pause_control = allocate_control();
    pause_control->is_pressable = false;
//    pause_control->file_name_pressed = "pause.png";
//    pause_control->file_name_released = "pause.png";
    pause_control->is_visible = 0;
    pause_control->alpha_portrait = 245;
    pause_control->alpha_landscape = 245;
}

// At emulation start
void init_effective_and_config_game_controls(int conf_index) {
    if (app_config_struct.mobile_show_controls) {
        finished_controls_init = false;
        window_x = 0;
        window_y = 0;
        act_configuration_control_index = -1;
        first_valid_configuration_control_index = -1;
        configuration_mode = false;
        manage_show_saved = false;
        config_index = conf_index;
    }
    populate_pause_control();
}

// At emulation end
void clear_effective_and_config_game_controls() {
    if (app_config_struct.mobile_show_controls) {
        release_all_controls();
        on_release_controls.clear();
        on_release_controls_queue.clear();
        act_map = 0;
        clear_controls(&(effective_game_controls[LEFT_HAND_INDEX]));
        clear_controls(&(effective_game_controls[RIGHT_HAND_INDEX]));
        clear_controls(&configuration_controls);
    }
    if (pause_control != nullptr) {
        deallocate_control(pause_control);
    }
    pause_control = nullptr;
}

void clear_controls(vector<Control *> container[2]) {
    if (container != nullptr && container->size() > 0) {
        for (int i = 0; i < container->size(); i++) {
            Control *c = (*container)[i];
            if (c != nullptr) {
                deallocate_control(c);
            }
            c = nullptr;
        }
        container->clear();
    }
}

// At beginning of app
static void init_default_game_controls() {
    // Order is important, they will be drawn in order
    device_default_keys.push_back(PD0L_A_T);
    device_default_keys.push_back(PD0L_A_L);
    device_default_keys.push_back(PD0L_A_R);

    device_default_keys.push_back(PD0L_KP1);
    device_default_keys.push_back(PD0L_KP2);
    device_default_keys.push_back(PD0L_KP3);
    device_default_keys.push_back(PD0L_KP4);
    device_default_keys.push_back(PD0L_KP5);
    device_default_keys.push_back(PD0L_KP6);
    device_default_keys.push_back(PD0L_KP7);
    device_default_keys.push_back(PD0L_KP8);
    device_default_keys.push_back(PD0L_KP9);
    device_default_keys.push_back(PD0L_KPC);
    device_default_keys.push_back(PD0L_KP0);
    device_default_keys.push_back(PD0L_KPE);

    device_default_keys.push_back(PD0R_A_T);
    device_default_keys.push_back(PD0R_A_L);
    device_default_keys.push_back(PD0R_A_R);
    device_default_keys.push_back(PD0R_KP1);
    device_default_keys.push_back(PD0R_KP2);
    device_default_keys.push_back(PD0R_KP3);
    device_default_keys.push_back(PD0R_KP4);
    device_default_keys.push_back(PD0R_KP5);
    device_default_keys.push_back(PD0R_KP6);
    device_default_keys.push_back(PD0R_KP7);
    device_default_keys.push_back(PD0R_KP8);
    device_default_keys.push_back(PD0R_KP9);
    device_default_keys.push_back(PD0R_KPC);
    device_default_keys.push_back(PD0R_KP0);
    device_default_keys.push_back(PD0R_KPE);

    device_default_keys.push_back(PAUSE);
    device_default_keys.push_back(RESET);
    device_default_keys.push_back(KEYBOARD);
    device_default_keys.push_back(QUIT);
    device_default_keys.push_back(CHANGE_PLAYER);
    device_default_keys.push_back(PLAYER_SELECTED);
    device_default_keys.push_back(PLAYER_NOT_SELECTED);
    device_default_keys.push_back(SHOT);
    device_default_keys.push_back(DISC_KEY);

    // Ecs
    device_default_keys.push_back(KEYB_LEFT);
    device_default_keys.push_back(KEYB_PERIOD);
    device_default_keys.push_back(KEYB_SEMI);
    device_default_keys.push_back(KEYB_P);
    device_default_keys.push_back(KEYB_ESC);
    device_default_keys.push_back(KEYB_0);
    device_default_keys.push_back(KEYB_ENTER);
    device_default_keys.push_back(KEYB_COMMA);
    device_default_keys.push_back(KEYB_M);
    device_default_keys.push_back(KEYB_K);
    device_default_keys.push_back(KEYB_I);
    device_default_keys.push_back(KEYB_9);
    device_default_keys.push_back(KEYB_8);
    device_default_keys.push_back(KEYB_O);
    device_default_keys.push_back(KEYB_L);
    device_default_keys.push_back(KEYB_N);
    device_default_keys.push_back(KEYB_B);
    device_default_keys.push_back(KEYB_H);
    device_default_keys.push_back(KEYB_Y);
    device_default_keys.push_back(KEYB_7);
    device_default_keys.push_back(KEYB_6);
    device_default_keys.push_back(KEYB_U);
    device_default_keys.push_back(KEYB_J);
    device_default_keys.push_back(KEYB_V);
    device_default_keys.push_back(KEYB_C);
    device_default_keys.push_back(KEYB_F);
    device_default_keys.push_back(KEYB_R);
    device_default_keys.push_back(KEYB_5);
    device_default_keys.push_back(KEYB_4);
    device_default_keys.push_back(KEYB_T);
    device_default_keys.push_back(KEYB_G);
    device_default_keys.push_back(KEYB_X);
    device_default_keys.push_back(KEYB_Z);
    device_default_keys.push_back(KEYB_S);
    device_default_keys.push_back(KEYB_W);
    device_default_keys.push_back(KEYB_3);
    device_default_keys.push_back(KEYB_2);
    device_default_keys.push_back(KEYB_E);
    device_default_keys.push_back(KEYB_D);
    device_default_keys.push_back(KEYB_SPACE);
    device_default_keys.push_back(KEYB_DOWN);
    device_default_keys.push_back(KEYB_UP);
    device_default_keys.push_back(KEYB_Q);
    device_default_keys.push_back(KEYB_1);
    device_default_keys.push_back(KEYB_RIGHT);
    device_default_keys.push_back(KEYB_CTRL);
    device_default_keys.push_back(KEYB_A);
    device_default_keys.push_back(KEYB_SHIFT);
    device_default_keys.push_back(KEYB_SHIFT_2);

    vector<string> children_of_change_player;
    children_of_change_player.push_back(PLAYER_SELECTED);
    children_of_change_player.push_back(PLAYER_NOT_SELECTED);
    children_map.insert({CHANGE_PLAYER, children_of_change_player});

    for (int i = 0; i < device_default_keys.size(); i++) {
        add_control_to_container(device_default_keys[i].c_str(), device_default_controls, LEFT_HAND_INDEX);
        add_control_to_container(device_default_keys[i].c_str(), device_default_controls, RIGHT_HAND_INDEX);
    }
    for (int hand_index = 0; hand_index < 2; hand_index++) {
        Control *change_player_control = get_control_by_event(CHANGE_PLAYER, &device_default_controls[hand_index]);
        change_player_control->config_func = change_player_callback;

        Control *quit_control = get_control_by_event(QUIT, &device_default_controls[hand_index]);
        quit_control->config_func = send_quit;

        Control *keyboard_control = get_control_by_event(KEYBOARD, &device_default_controls[hand_index]);
        keyboard_control->config_func = toggle_ecs_keyboard_callback;
    }
}

// At exit of app
void clear_all_default_game_controls() {
    clear_controls(&(device_default_controls[LEFT_HAND_INDEX]));
    clear_controls(&(device_default_controls[RIGHT_HAND_INDEX]));
    clear_controls(&(delta_default_controls[LEFT_HAND_INDEX]));
    clear_controls(&(delta_default_controls[RIGHT_HAND_INDEX]));
    device_default_keys.clear();
    controls_keys.clear();

    std::map<string, vector<string>>::iterator it;
    for (it = children_map.begin(); it != children_map.end(); it++) {
        vector<string> vstring = it->second;
        vstring.clear();
    }
    children_map.clear();

    files_map.clear();
    files_pressed_map.clear();
}

Control *get_control_by_touch_point(float x_perc, float y_perc, SDL_FRect *real_pos) {
    float x = x_perc * (float) 100;
    float y = y_perc * (float) 100;
    for (int i = 0; i < configuration_controls.size(); i++) {
        Control *c = configuration_controls[i];
        if (c != nullptr && c->is_visible == 1 && is_compatible_with_act_configuration(c) && c->is_drawn && (c->is_pressable || c->is_pressed) && c->is_in_control(x, y, real_pos)) {
            // To avoid repeated click
            if (manage_pause && !c->get_effective_event().compare(CONFIGURATION_SWITCH_MODE)) {
                return nullptr;
            }
            return c;
        }
    }
    for (int i = 0; i < effective_game_controls[app_config_struct.mobile_use_inverted_controls ? 1 : 0].size(); i++) {
        Control *c = effective_game_controls[app_config_struct.mobile_use_inverted_controls ? 1 : 0][i];
        if (c != nullptr && c->is_visible == 1 && is_compatible_with_act_configuration(c) && c->is_drawn && (c->is_pressable || c->is_pressed) && c->is_in_control(x, y, real_pos)) {
            // To avoid repeated click
            if (manage_pause && !c->is_pressed && !c->get_effective_event().compare(PAUSE)) {
                return nullptr;
            }
            return c;
        }
    }
    return nullptr;
}

float pi_mezzi = (float) M_PI / (float) 2;
float pi_3_mezzi = (float) M_PI * 1.5;
float pi_x_2 = 2 * M_PI;

static int getRadZone(float x, float y, int num_positions) {
    float rad;
    if (x == 0) {
        if (y < 0) {
            rad = pi_mezzi;
        } else {
            rad = pi_3_mezzi;
        }
    } else {
        float coeff_ang = (float) -y / (float) x;
        rad = atan(coeff_ang);
        if (x < 0) {
            rad += M_PI;
        } else if (y > 0) {
            rad += pi_x_2;
        }
    }

//    float gradi = rad * (180/M_PI);

    int zones = num_positions * 2;
    float rads_for_zones = (float) 2 * M_PI / (float) zones;
//    float grads_for_zones = rads_for_zones * (180/M_PI);

    int final_zone = (rad / rads_for_zones) + 1;
//    int final_zone = (gradi / grads_for_zones) +1;
    if (final_zone >= zones) {
        final_zone = 0;
    }
    int res = final_zone / 2;
    return res;
}

int decode_angle_index(int rads_zone, int num_positions) {
    switch (num_positions) {
        case 16:
            if (rads_zone == 0) {
                // Right
                return get_direction_position(app_config_struct.act_player == 0 ? PD0L_J_E : PD0R_J_E);
            } else if (rads_zone == 1) {
                // Right Up Right PD0L_J_ENE
                return get_direction_position(app_config_struct.act_player == 0 ? PD0L_J_ENE : PD0R_J_ENE);
            } else if (rads_zone == 2) {
                // Right Up PD0L_J_NE
                return get_direction_position(app_config_struct.act_player == 0 ? PD0L_J_NE : PD0R_J_NE);
            } else if (rads_zone == 3) {
                // Right Up Up PD0L_J_NNE
                return get_direction_position(app_config_struct.act_player == 0 ? PD0L_J_NNE : PD0R_J_NNE);
            } else if (rads_zone == 4) {
                // Up PD0L_J_N
                return get_direction_position(app_config_struct.act_player == 0 ? PD0L_J_N : PD0R_J_N);
            } else if (rads_zone == 5) {
                // Left Up Up PD0L_J_NNW
                return get_direction_position(app_config_struct.act_player == 0 ? PD0L_J_NNW : PD0R_J_NNW);
            } else if (rads_zone == 6) {
                // Left Up PD0L_J_NW
                return get_direction_position(app_config_struct.act_player == 0 ? PD0L_J_NW : PD0R_J_NW);
            } else if (rads_zone == 7) {
                // Left Up Left PD0L_J_WNW
                return get_direction_position(app_config_struct.act_player == 0 ? PD0L_J_WNW : PD0R_J_WNW);
            } else if (rads_zone == 8) {
                // Left
                return get_direction_position(app_config_struct.act_player == 0 ? PD0L_J_W : PD0R_J_W);
            } else if (rads_zone == 9) {
                // Left Down Left PD0L_J_WSW
                return get_direction_position(app_config_struct.act_player == 0 ? PD0L_J_WSW : PD0R_J_WSW);
            } else if (rads_zone == 10) {
                // Left Down PD0L_J_SW
                return get_direction_position(app_config_struct.act_player == 0 ? PD0L_J_SW : PD0R_J_SW);
            } else if (rads_zone == 11) {
                // Left Down Down Up PD0L_J_SSW
                return get_direction_position(app_config_struct.act_player == 0 ? PD0L_J_SSW : PD0R_J_SSW);
            } else if (rads_zone == 12) {
                // Down PD0L_J_S
                return get_direction_position(app_config_struct.act_player == 0 ? PD0L_J_S : PD0R_J_S);
            } else if (rads_zone == 13) {
                // Right Down Down PD0L_J_SSE
                return get_direction_position(app_config_struct.act_player == 0 ? PD0L_J_SSE : PD0R_J_SSE);
            } else if (rads_zone == 14) {
                // Right Down PD0L_J_SE
                return get_direction_position(app_config_struct.act_player == 0 ? PD0L_J_SE : PD0R_J_SE);
            } else if (rads_zone == 15) {
                // Right Down Right PD0L_J_ESE
                return get_direction_position(app_config_struct.act_player == 0 ? PD0L_J_ESE : PD0R_J_ESE);
            }
            break;
        case 8:
            if (rads_zone == 0) {
                // Right
                return get_direction_position(PD0L_J_E);
            } else if (rads_zone == 1) {
                // Up Right PD0L_J_NE
                return get_direction_position(PD0L_J_NE);
            } else if (rads_zone == 2) {
                // Up PD0L_J_N
                return get_direction_position(PD0L_J_N);
            } else if (rads_zone == 3) {
                // Up Left PD0L_J_NW
                return get_direction_position(PD0L_J_NW);
            } else if (rads_zone == 4) {
                // Left PD0L_J_W
                return get_direction_position(PD0L_J_W);
            } else if (rads_zone == 5) {
                // Down Left PD0L_J_SW
                return get_direction_position(PD0L_J_SW);
            } else if (rads_zone == 6) {
                // Down PD0L_J_S
                return get_direction_position(PD0L_J_S);
            } else if (rads_zone == 7) {
                // Down Right PD0L_J_SE
                return get_direction_position(PD0L_J_SE);
            }
            break;
    }
    return -1;
}

void push_sdl_disc_state(Control *c, bool pressed) {
    float x = c->real_pos.x / c->get_control_frect()->w;
    float y = c->real_pos.y / c->get_control_frect()->h;
    int rad_zone = getRadZone(x, y, c->num_positions);
    int direction_position = decode_angle_index(rad_zone, c->num_positions);
    if (direction_position != -1) {
        bool send_event = true;

        if (pressed && (!c->continuous_click) && (c->last_direction == direction_position) && (c->last_pressed == pressed)) {
            send_event = false;
        }
        c->last_pressed = pressed;
        c->last_direction = direction_position;

        if (send_event) {
            if (c->config_func == nullptr) {
                event_enqueue_custom(pressed ? SDL_KEYDOWN : SDL_KEYUP, mapping_directions[act_map][direction_position].c_str());
            } else if (c->is_pressed) {
                c->config_func(&direction_position);
            }
        }
    }
}

// Used only in touch screen simulation by mouse
Control *get_old_control_pressed() {
    for (int i = 0; i < effective_game_controls[app_config_struct.mobile_use_inverted_controls ? 1 : 0].size(); i++) {
        Control *c = effective_game_controls[app_config_struct.mobile_use_inverted_controls ? 1 : 0][i];
        if (c != nullptr && c->is_pressed) {
            return c;
        }
    }
    return nullptr;
}

void change_control_press_status(Control *c, bool pressed) {
    c->is_pressed = pressed;
    if (!c->is_disc) {
        if (!c->get_effective_event().compare(PAUSE) && pressed) {
            custom_pause_event();
        } else if (c->config_func == nullptr) {
            event_enqueue_custom(pressed ? SDL_KEYDOWN : SDL_KEYUP, mapping_events[act_map][c->jzintv_event_index].c_str());
        } else if (c->is_pressed) {
            if (c->act_on_release) {
                add_control_on_release_queue(c);
            } else {
                c->config_func(&c->config_parameter);
            }
        }
    } else {
        if (!pressed) {
            // Release disc
            push_sdl_disc_state(c, false);
        }
    }
}

void manage_button_press_or_release(Control *c, bool pressed) {
    if (c->is_disc) {
        c->is_pressed = pressed;
        push_sdl_disc_state(c, pressed);
    } else {
        if (pressed != c->is_pressed || (pressed && c->continuous_click)) {
            change_control_press_status(c, pressed);
        }
    }
}

void manage_button_press_or_release_by_event(SDL_Event *old_event, bool pressed) {
    float x = old_event->tfinger.x;
    float y = old_event->tfinger.y;
    SDL_FRect real_pos;
    Control *c = get_control_by_touch_point(x, y, &real_pos);

    if (c != nullptr) {
        c->x_direction = x;
        c->y_direction = y;
        memcpy(&c->real_pos, &real_pos, sizeof(SDL_FRect));
        manage_button_press_or_release(c, pressed);
    }
}

static void release_effective_game_controls() {
    for (int hand_index = 0; hand_index < 2; hand_index++) {
        for (int i = 0; i < effective_game_controls[hand_index].size(); i++) {
            Control *c = effective_game_controls[hand_index][i];
            if (c != nullptr && c->is_pressed) {
                change_control_press_status(c, false);
            }
        }
    }
}

static void release_configuration_controls() {
    for (int i = 0; i < configuration_controls.size(); i++) {
        Control *c = configuration_controls[i];
        if (c != nullptr && c->is_pressed) {
            if (!switching_mode || c->get_effective_event().compare(CONFIGURATION_SWITCH_MODE)) {
                change_control_press_status(c, false);
            }
        }
    }
}

void release_all_controls() {
    release_effective_game_controls();
    release_configuration_controls();
}

void manage_button_motion(SDL_Event *old_event) {
    float x = old_event->tfinger.x;
    float y = old_event->tfinger.y;
    Control *c = get_control_by_touch_point(x, y, nullptr);
    if (c != nullptr && c->is_pressed) {
        // Motion in the pressed button itself, no actions for standard buttons
        if (c->is_disc) {
            // Update direction for disc
            manage_button_press_or_release_by_event(old_event, true);
        }
        return;
    }
    x = (old_event->tfinger.x - old_event->tfinger.dx);
    y = (old_event->tfinger.y - old_event->tfinger.dy);
    c = get_control_by_touch_point(x, y, nullptr);
    if (c != nullptr && c->is_pressed) {
        // Motion from a button to another point, forcing release of button
        change_control_press_status(c, false);
        return;
    }
}

string get_control_key(char *key, const char *reference) {
    std::string s = key;
    int len_ref = strlen(reference);
    int len_key = strlen(key);
    std::string token = s.substr(len_ref, len_key);
    string res = "";
    if (token.length() > 0) {
        std::string token_separator = token.substr(0, 1);
        if (strcmp(token_separator.c_str(), "_")) {
            res = "";
        } else {
            res = token.substr(1);
        }
    } else {
        res = "";
    }
    return res;
}

static void init_control_keys() {
    controls_keys.emplace_back(CONTROL_PORTRAIT_X_PERC_OPTION);
    controls_keys.emplace_back(CONTROL_PORTRAIT_Y_PERC_OPTION);
    controls_keys.emplace_back(CONTROL_PORTRAIT_W_PERC_OPTION);
    controls_keys.emplace_back(CONTROL_PORTRAIT_H_PERC_OPTION);
    controls_keys.emplace_back(CONTROL_LANDSCAPE_X_PERC_OPTION);
    controls_keys.emplace_back(CONTROL_LANDSCAPE_Y_PERC_OPTION);
    controls_keys.emplace_back(CONTROL_LANDSCAPE_W_PERC_OPTION);
    controls_keys.emplace_back(CONTROL_LANDSCAPE_H_PERC_OPTION);
    controls_keys.emplace_back(CONTROL_VISIBLE_OPTION);
    controls_keys.emplace_back(CONTROL_ALPHA_PORTRAIT_OPTION);
    controls_keys.emplace_back(CONTROL_ALPHA_LANDSCAPE_OPTION);
    controls_keys.emplace_back(CONTROL_FILE_NAME_PRESSED_OPTION);
    controls_keys.emplace_back(CONTROL_FILE_NAME_RELEASED_OPTION);
    controls_keys.emplace_back(CONTROL_OVERRIDE_EVENT_OPTION);
}

void init_controls() {
    init_default_game_controls();
    init_control_keys();
    init_files_default_map();
}

SDL_Texture *allocate_control_texture(const char *name, SDL_Renderer *renderer, int64_t ptr) {
    SDL_Texture *tx = get_texture(name, renderer, ptr);
    return tx;
}

bool manage_override_control_for_game(char *key, char *value, struct rom_config_struct_t *act_rom_config) {
    // Esempio: "control_alpha_landscape_PD0L_KP1__L"
    // ref_control_property: control_alpha_landscape
    // control_key: PD0L_KP1__L
    // real_event: PD0L_KP1
    bool res = false;
    for (int i = 0; i < controls_keys.size(); i++) {
        const char *ref_control_property = controls_keys[i].c_str();
        if (startsWith(key, ref_control_property)) {
            string control_key = get_control_key(key, ref_control_property);

            int hand_index = get_hand_index(control_key.c_str());
            string real_event = control_key.substr(0, control_key.length() - 3);

            Control *new_control = add_new_custom_game_control(real_event.c_str(), hand_index, act_rom_config->controls);
            char *ref_key_alloc = strdup(ref_control_property);
            res = set_control_value(ref_key_alloc, value, new_control);
            if (res) {
                save_persistent_values(ref_key_alloc, value, new_control, hand_index, act_rom_config);
            }
            free(ref_key_alloc);
            break;
        }
    }
    return res;
}

static void fix_player_selected_visible_status(vector<Control *> *container) {
    Control *player_selectable = get_control_by_event(PLAYER_SELECTED, container);
    if (player_selectable != nullptr) {
        player_selectable->is_visible = 0;
    }
    Control *player_not_selectable = get_control_by_event(PLAYER_NOT_SELECTED, container);
    if (player_not_selectable != nullptr) {
        player_not_selectable->is_visible = 0;
    }
    Control *change_player = get_control_by_event(CHANGE_PLAYER, container);
    if (change_player != nullptr && player_selectable != nullptr && player_not_selectable != nullptr) {
        player_selectable->is_visible = change_player->is_visible;
        player_not_selectable->is_visible = change_player->is_visible;

        if (player_selectable != nullptr && player_not_selectable != nullptr) {
            player_selectable->is_configurable = false;
            player_selectable->is_pressable = false;
            player_not_selectable->is_configurable = false;
            player_not_selectable->is_pressable = false;

            SDL_FRect *cp_frect = change_player->get_control_frect();
            SDL_FRect p1_frect;
            p1_frect.w = cp_frect->w / 10;
            p1_frect.h = cp_frect->h / 5;
            p1_frect.x = cp_frect->x + cp_frect->w / 5;
            p1_frect.y = cp_frect->y + cp_frect->h / 5;

            SDL_FRect p2_rect;
            p2_rect.w = p1_frect.w;
            p2_rect.h = p1_frect.h;
            p2_rect.y = p1_frect.y;
            p2_rect.x = p1_frect.x + cp_frect->w / 2;

            if (change_player->is_pressed) {
                p1_frect.x += 0.4;
                p1_frect.y += 0.2;
                p2_rect.x += 0.4;
                p2_rect.y += 0.2;
            }

            if (app_config_struct.act_player == 0) {
                memcpy(player_selectable->get_control_frect(), &p1_frect, sizeof(SDL_FRect));
                memcpy(player_not_selectable->get_control_frect(), &p2_rect, sizeof(SDL_FRect));
            } else {
                memcpy(player_not_selectable->get_control_frect(), &p1_frect, sizeof(SDL_FRect));
                memcpy(player_selectable->get_control_frect(), &p2_rect, sizeof(SDL_FRect));
            }
        }
    }
}

static void fix_disc_direction_visible_status(vector<Control *> *container, string disc_event_key, string disc_direction_event_key) {
    Control *disc_control = get_control_by_event(disc_event_key, container);
    Control *disc_direction_control = get_control_by_event(disc_direction_event_key, container);
    if (disc_direction_control != nullptr) {
        disc_direction_control->is_configurable = false;
        disc_direction_control->is_pressable = false;
        if (disc_control != nullptr) {
            if (!disc_control->is_pressed) {
                disc_direction_control->is_visible = 0;
            } else {
                disc_direction_control->is_visible = 1;
                SDL_FRect *rect = disc_direction_control->get_control_frect();
                rect->w = disc_control->get_control_frect()->w / 5;
                rect->h = disc_control->get_control_frect()->h / 5;
                rect->x = (disc_control->x_direction * 100 - rect->w / (float) 2);
                rect->y = (disc_control->y_direction * 100 - rect->h / (float) 2);
            }
        } else {
            disc_direction_control->is_visible = 0;
        }
    }
}

bool is_compatible_with_act_configuration(Control *c) {
    int ev_player = -1;
    const char *ev = c->original_event.c_str();
    if (act_map == 0) {
        if (startsWith(ev, "KEYB_")) {
            return false;
        }
        if (!startsWith(ev, "PD0")) {
            return true;
        } else if (strlen(ev) < 4) {
            return true;
        } else {
            char playerCh = ev[3];
            if (playerCh == 'L') {
                ev_player = 0;
            } else if (playerCh == 'R') {
                ev_player = 1;
            }
            return ev_player == app_config_struct.act_player;
        }
    } else {
        if (startsWith(ev, "KEYB_")) {
            return true;
        }
        if (startsWith(ev, "PD0") || startsWith(ev, "DISC")) {
            return false;
        }
    }
    return true;
}

void draw_control(Control *c, SDL_Renderer *renderer) {
    if (c != nullptr && (c->is_visible == 1 && is_compatible_with_act_configuration(c))) {
        SDL_FRect *frect = c->get_control_frect();
        SDL_Rect pixels_rect = transform_to_sdl_rect(frect);
        int64_t *ptr = screen_is_portrait ? &c->alpha_portrait : &c->alpha_landscape;
        if (c->tx_released == nullptr && !c->file_name_released.empty()) {
            c->tx_released = allocate_control_texture(c->file_name_released.c_str(), renderer, *ptr);
            if (c->tx_released == nullptr) {
                c->tx_released = (SDL_Texture *) -1;
            }
        }
        if (c->tx_pressed == nullptr && !c->file_name_pressed.empty() && c->tx_released != nullptr &&
            c->tx_released != (SDL_Texture *) -1) {
            c->tx_pressed = allocate_control_texture(c->file_name_pressed.c_str(), renderer, *ptr);
            if (c->tx_pressed == nullptr) {
                c->tx_pressed = (SDL_Texture *) -1;
            }
        }
        if (c->tx_released == nullptr || c->tx_released == (SDL_Texture *) -1 || c->tx_pressed == nullptr ||
            c->tx_pressed == (SDL_Texture *) -1) {
            c->is_visible = 0;
        } else {
            if (c->is_pressed) {
                SDL_RenderCopy(renderer, c->tx_pressed, nullptr, &pixels_rect);
            } else {
                SDL_RenderCopy(renderer, c->tx_released, nullptr, &pixels_rect);
            }
            c->is_drawn = true;
            if (c->continuous_click && c->is_pressed) {
                manage_button_press_or_release(c, true);
            }
        }
    }
}

void draw_controls(vector<Control *> container[2], SDL_Renderer *renderer, bool update_alpha) {
    if (manage_show_saved) {
        Control *save_control;
        if (manage_show_saved_global) {
            save_control = get_control_by_event(CONFIGURATION_SAVED_GLOBAL_TOASTER, &configuration_controls);
        } else {
            save_control = get_control_by_event(CONFIGURATION_SAVED_FOR_GAME_TOASTER, &configuration_controls);
        }
        if (save_control != nullptr) {
            Uint32 act_ticks = SDL_GetTicks();
            if (act_ticks - saved_start_ticks > 5) {
                int64_t *ptr = screen_is_portrait ? &save_control->alpha_portrait : &save_control->alpha_landscape;
                if (*ptr > 1) {
                    save_control->is_visible = 1;
                    *ptr -= 3;
                    save_control->update_textures_alpha();
                    saved_start_ticks = act_ticks;
                } else {
                    save_control->is_visible = 0;
                    *ptr = 255;
                    manage_show_saved = false;
                }
            }
        }
    }
    for (int i = 0; i < container->size(); i++) {
        Control *c = (*container)[i];
        if (update_alpha) {
            c->update_textures_alpha();
        }
        draw_control(c, renderer);
    }
}

void update_pause_control_position() {
    // Pause_control control
    if (pause_control != nullptr) {
        SDL_FRect *ref = *get_act_jzintv_rendering_frect_ref(screen_is_portrait);
        if (ref != nullptr) {
            memcpy(pause_control->get_control_frect(), ref, sizeof(SDL_FRect));
        }
    }
}

void print_all_controls(string message, bool print_games) {
    Log(LOG_INFO) << "____________-----------____________";
    string mes;
    if (!print_games) {
        mes = message;
        mes.append("device: ");
        print_controls(mes, &device_default_controls[0]);

        mes = message;
        mes.append("delta: ");
        print_controls(mes, &delta_default_controls[0]);
    }

    int num = print_games ? roms_list_struct.total_roms_num : app_config_struct.num_valid_crc32s;

    for (int hand_index = 0; hand_index < 2; hand_index++) {
        for (int i = 0; i < num; i++) {
            vector<Control *> ctrls = print_games ? roms_list_struct.list[i].controls[hand_index] : roms_configuration[i].controls[hand_index];
            ostringstream os;
            os << message;
            os << " Hand_index: ";
            os << hand_index;
            os << " - Game: ";
            os << (print_games ? roms_list_struct.list[i].game_name : roms_configuration[i].game_name);
            os << ": ";
            print_controls(os.str(), &ctrls);
        }
    }
}

extern "C" void manage_onscreen_controls(SDL_Renderer *renderer) {
    if (pause_control != nullptr) {
        if (custom_emulation_paused && pause_control->is_visible == 0) {
            pause_control->is_visible = 1;
        } else if (!custom_emulation_paused && pause_control->is_visible == 1) {
            pause_control->is_visible = 0;
        }
        if (custom_emulation_paused) {
            update_pause_control_position();
            draw_control(pause_control, renderer);
        }
    }
    if (app_config_struct.mobile_show_controls) {
        if (!finished_controls_init) {
            vector<Control *> new_delta_default_controls[2];
            for (int hand_index = 0; hand_index < 2; hand_index++) {
                normalize_controls_to_delta(new_delta_default_controls, hand_index, true);
                duplicate_controls(&(new_delta_default_controls[hand_index]), &(delta_default_controls[hand_index]));
                normalize_controls_from_delta(hand_index);
                clear_controls(&new_delta_default_controls[hand_index]);
            }
            populate_effective_game_controls(selected_rom->controls);
            populate_configuration_controls();
            finished_controls_init = true;
        }

        fix_player_selected_visible_status(&(effective_game_controls[app_config_struct.mobile_use_inverted_controls ? 1 : 0]));
        fix_disc_direction_visible_status(&(effective_game_controls[app_config_struct.mobile_use_inverted_controls ? 1 : 0]), DISC_KEY, DISC_DIRECTION_KEY);
        draw_controls(&(effective_game_controls[app_config_struct.mobile_use_inverted_controls ? 1 : 0]), renderer, last_screen_is_portrait != screen_is_portrait);
        if (app_config_struct.mobile_show_configuration_controls) {
            fix_disc_direction_visible_status(&configuration_controls, CONFIGURATION_DISC_KEY, CONFIGURATION_DISC_DIRECTION_KEY);
            draw_controls(&configuration_controls, renderer, last_screen_is_portrait != screen_is_portrait);
            if (configuration_mode) {
                update_configuration_mode_controls();
            }
        }
        last_screen_is_portrait = screen_is_portrait;
    }
}
