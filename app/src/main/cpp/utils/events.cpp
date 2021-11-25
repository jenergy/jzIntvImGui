#include "main.h"

bool manage_pause = false;
Uint32 pause_start_ticks;
bool custom_emulation_paused = false;
vector<vector<string>> mapping_directions;
vector<string> events_needed_vec;
set<string> events_needed;
vector<string> events_directions_needed_vec;
vector<vector<string>> mapping_events;

void add_event_to_monitor(const char *event) {
    events_needed.insert(event);
}

void finalize_events_to_monitor() {
    std::set<std::string>::iterator events_it = events_needed.begin();
    while (events_it != events_needed.end()) {
        string str = *events_it;
        events_needed_vec.emplace_back(str);
        events_it++;
    }
    events_needed.clear();
}

static void init_direction_events() {
    events_directions_needed_vec.clear();

    // P1 Disc
    events_directions_needed_vec.emplace_back(PD0L_J_E);
    events_directions_needed_vec.emplace_back(PD0L_J_ENE);
    events_directions_needed_vec.emplace_back(PD0L_J_NE);
    events_directions_needed_vec.emplace_back(PD0L_J_NNE);
    events_directions_needed_vec.emplace_back(PD0L_J_N);
    events_directions_needed_vec.emplace_back(PD0L_J_NNW);
    events_directions_needed_vec.emplace_back(PD0L_J_NW);
    events_directions_needed_vec.emplace_back(PD0L_J_WNW);
    events_directions_needed_vec.emplace_back(PD0L_J_W);
    events_directions_needed_vec.emplace_back(PD0L_J_WSW);
    events_directions_needed_vec.emplace_back(PD0L_J_SW);
    events_directions_needed_vec.emplace_back(PD0L_J_SSW);
    events_directions_needed_vec.emplace_back(PD0L_J_S);
    events_directions_needed_vec.emplace_back(PD0L_J_SSE);
    events_directions_needed_vec.emplace_back(PD0L_J_ESE);
    events_directions_needed_vec.emplace_back(PD0L_J_SE);

    // P2 Disc
    events_directions_needed_vec.emplace_back(PD0R_J_E);
    events_directions_needed_vec.emplace_back(PD0R_J_ENE);
    events_directions_needed_vec.emplace_back(PD0R_J_NE);
    events_directions_needed_vec.emplace_back(PD0R_J_NNE);
    events_directions_needed_vec.emplace_back(PD0R_J_N);
    events_directions_needed_vec.emplace_back(PD0R_J_NNW);
    events_directions_needed_vec.emplace_back(PD0R_J_NW);
    events_directions_needed_vec.emplace_back(PD0R_J_WNW);
    events_directions_needed_vec.emplace_back(PD0R_J_W);
    events_directions_needed_vec.emplace_back(PD0R_J_WSW);
    events_directions_needed_vec.emplace_back(PD0R_J_SW);
    events_directions_needed_vec.emplace_back(PD0R_J_SSW);
    events_directions_needed_vec.emplace_back(PD0R_J_S);
    events_directions_needed_vec.emplace_back(PD0R_J_SSE);
    events_directions_needed_vec.emplace_back(PD0R_J_ESE);
    events_directions_needed_vec.emplace_back(PD0R_J_SE);
}

void init_events() {
    init_direction_events();
}

void normalize_events() {
    finalize_events_to_monitor();
}

static SDL_Event get_simulated_finger_event(SDL_Event *old_event, int type) {
    SDL_Event new_event;
    new_event.type = type;

    new_event.tfinger.x = (float) old_event->motion.x / (float) window_x;
    new_event.tfinger.y = (float) old_event->motion.y / (float) window_y;
    return new_event;
}

static bool check_for_custom_pause(event_num_t num) {
    const char *tmp = event_num_to_name(num);
    if (tmp != NULL && !strcmp(tmp, PAUSE)) {
        return true;
    }
    return false;
}

static bool is_pause_event(SDL_Event *event, const event_num_t event_num[2], bool may_combo, bool *send_pause) {
    switch (event->type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            // Special Events (only "PAUSE" for the moment)
            *send_pause = event->type != SDL_KEYUP;
            break;
        case SDL_JOYAXISMOTION:
        case SDL_JOYHATMOTION:
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
            *send_pause = event->type != SDL_JOYBUTTONUP;
            break;
    }

    bool res = false;
    if (!may_combo) {
        for (int i = 0; i < 2; i++) {
            if (event_num[i] != EVENT_IGNORE) {
                res |= check_for_custom_pause(event_num[i]);
            }
        }
    }
    return res;
}

extern "C" bool check_pause_event(SDL_Event *event, const event_num_t event_num[2], bool may_combo) {
    bool send = false;
    if (is_pause_event(event, event_num, may_combo, &send)) {
        if (send) {
            custom_pause_event();
        }
        return true;
    }
    return false;
}

extern int act_map;
extern "C" bool consume_special_event(int map, SDL_Event *event) {
    act_map = map;
    Control *c;
    SDL_Event new_event;
    switch (event->type) {
        case SDL_KEYDOWN:
            if (event->key.keysym.sym == SDLK_AC_BACK) {
                // Android back key
                if (!check_switch_mode()) {
                    // Quitting
                    new_event.type = SDL_QUIT;
                    SDL_PushEvent(&new_event);
                    return true;
                } else {
                    return false;
                }
            }
            break;
            //////////////////////////////////////////////////////////////////////////////////////////////////////////
            // Simulate Android Finger Events on destkop devices. In order to work, option --enable_mouse must be used
            // (Not sure of this)
            // For disc simulation, a real Joystick must be connected, because a fake JOYAXISMOTION event is generated
            //////////////////////////////////////////////////////////////////////////////////////////////////////////
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            if (app_config_struct.mobile_show_controls) {
                release_all_controls();
                new_event = get_simulated_finger_event(event, event->type == SDL_MOUSEBUTTONDOWN ? SDL_FINGERDOWN : SDL_FINGERUP);
                manage_button_press_or_release_by_event(&new_event, event->type == SDL_MOUSEBUTTONDOWN);
            }
            return app_config_struct.consume_mouse_events_only_for_simulate_controls;
        case SDL_MOUSEMOTION:
            if (app_config_struct.mobile_show_controls) {
                int x, y;
                SDL_GetMouseState(&x, &y);
                c = get_control_by_touch_point((float) x / (float) window_x, (float) y / (float) window_y, NULL);
                if (c == NULL) {
                    release_all_controls();
                } else {
                    Control *old_pressed = get_old_control_pressed();
                    if (old_pressed != NULL && old_pressed != c) {
                        change_control_press_status(old_pressed, false);
                    }
                    if (c->is_disc && c->is_pressed) {
                        new_event = get_simulated_finger_event(event, SDL_FINGERDOWN);
                        manage_button_press_or_release_by_event(&new_event, true);
                    }
                }
            }
            return app_config_struct.consume_mouse_events_only_for_simulate_controls;
            ///////////////////////////////////////////////////////////////////////
            // Desktop -> end simulating Android Finger Events.
            //////////////////////////////////////////////////////////////////////
        case SDL_FINGERDOWN:
        case SDL_FINGERUP:
            if (app_config_struct.mobile_show_controls) {
                manage_button_press_or_release_by_event(event, event->type == SDL_FINGERDOWN);
            }
            return true;
        case SDL_FINGERMOTION:
            if (app_config_struct.mobile_show_controls) {
                manage_button_motion(event);
            }
            return true;
    }
    return false;
}

// When pause is triggered, on some devices the same audio sample is played continously during pause
// So we send attenuation event here, and later the real pause event
void custom_pause_event() {
    // Later we'll send pause
    manage_pause = true;
    pause_start_ticks = SDL_GetTicks();
    if (!custom_emulation_paused) {
        // Enter pause
        force_sound_atten = 1;
        custom_emulation_paused = true;
    } else {
        // Resume
        force_sound_atten = 2;
        custom_emulation_paused = false;
    }
}

extern "C" void tick_called() {
    check_screen_change();
    if (manage_pause) {
        Uint32 act_ticks = SDL_GetTicks();
        if (act_ticks - pause_start_ticks >= 3) {
            for (int i = 0; i < events_needed_vec.size(); i++) {
                if (!events_needed_vec[i].compare(PAUSE)) {
                    event_enqueue_custom(SDL_KEYDOWN, mapping_events[act_map][i].c_str());
                    manage_pause = false;
                    break;
                }
            }
        }
    } else {
        manage_on_release_controls_queue();
    }
}

void reset_mappings() {
    for (int i = 0; i < mapping_events.size(); i++) {
        mapping_events[i].clear();
    }
    mapping_events.clear();

    for (int i = 0; i < mapping_directions.size(); i++) {
        mapping_directions[i].clear();
    }
    mapping_directions.clear();
}

extern "C" void map_event(const char *jzintv_event_name, const char *event_num_name, int map) {
    if (startsWith(event_num_name, "COMBO")) {
        return;
    }
    bool found = false;
    for (int i = 0; i < events_needed_vec.size(); i++) {
        const char *act_event = events_needed_vec[i].c_str();

        if (!strcmp(act_event, jzintv_event_name)) {
            if (mapping_events.size() < map + 1) {
                vector<string> vec;
                mapping_events.push_back(vec);
            }
            while (mapping_events[map].size() < i + 1) {
                mapping_events[map].push_back("");
            }
            mapping_events[map][i] = event_num_name;
            found = true;
            break;
        }
    }

    if (!found) {
        for (int i = 0; i < events_directions_needed_vec.size(); i++) {
            const char *act_event = events_directions_needed_vec[i].c_str();
            if (!strcmp(act_event, jzintv_event_name)) {
                if (mapping_directions.size() < map + 1) {
                    vector<string> vec;
                    mapping_directions.push_back(vec);
                }
                while (mapping_directions[map].size() < i + 1) {
                    mapping_directions[map].push_back("");
                }
                mapping_directions[map][i] = event_num_name;
                break;
            }
        }
    }
}

void clear_events() {
    events_needed_vec.clear();
    events_directions_needed_vec.clear();
}