#include "main.h"

bool screen_is_portrait;
int window_x = 0;
int window_y = 0;
float window_ratio_portrait;
float window_ratio_landscape;
SDL_Rect jzintv_rendering_rect_portrait;
SDL_Rect jzintv_rendering_rect_landscape;
SDL_Rect *jzintv_rendering_rect_portrait_ref;
SDL_Rect *jzintv_rendering_rect_landscape_ref;

SDL_FRect jzintv_rendering_frect_portrait;
SDL_FRect jzintv_rendering_frect_landscape;
SDL_FRect *jzintv_rendering_frect_portrait_ref;
SDL_FRect *jzintv_rendering_frect_landscape_ref;

void init_jzintv_screen_references() {
    jzintv_rendering_rect_portrait_ref = NULL;
    jzintv_rendering_rect_landscape_ref = NULL;
    jzintv_rendering_frect_portrait_ref = NULL;
    jzintv_rendering_frect_landscape_ref = NULL;
}

SDL_FRect *get_delta_screen_frect(bool is_portrait) {
    return is_portrait ? &(app_config_struct.mobile_portrait_rect) : &(app_config_struct.mobile_landscape_rect);
}

SDL_FRect *get_rom_screen_frect(rom_config_struct_t *rom, bool is_portrait) {
    return is_portrait ? &(rom->mobile_portrait_rect) : &(rom->mobile_landscape_rect);
}

SDL_Rect transform_to_sdl_rect(SDL_FRect *act_rect) {
    SDL_Rect pixels_rect;
    pixels_rect.x = ((float) (act_rect->x) / (float) 100) * (float) window_x;
    pixels_rect.y = ((float) (act_rect->y) / (float) 100) * (float) window_y;
    pixels_rect.w = ((float) (act_rect->w) / (float) 100) * (float) window_x;
    pixels_rect.h = ((float) (act_rect->h) / (float) 100) * (float) window_y;
    return pixels_rect;
}

SDL_Rect **get_act_jzintv_rendering_rect_ref(bool is_portrait) {
    SDL_Rect **res = is_portrait ? &jzintv_rendering_rect_portrait_ref : &jzintv_rendering_rect_landscape_ref;
    return res;
}

SDL_FRect **get_act_jzintv_rendering_frect_ref(bool is_portrait) {
    SDL_FRect **res = is_portrait ? &jzintv_rendering_frect_portrait_ref : &jzintv_rendering_frect_landscape_ref;
    return res;
}

static SDL_Rect *get_act_jzintv_rendering_rect(bool is_portrait) {
    SDL_Rect *res = is_portrait ? &jzintv_rendering_rect_portrait : &jzintv_rendering_rect_landscape;
    return res;
}

static SDL_FRect *get_act_jzintv_rendering_frect(bool is_portrait) {
    SDL_FRect *res = is_portrait ? &jzintv_rendering_frect_portrait : &jzintv_rendering_frect_landscape;
    return res;
}

void normalize_single_to_delta(SDL_FRect *device_ref, SDL_FRect *ref, SDL_FRect *target) {
    if (target->x == ref->x || (ref->x == -1 && target->x == device_ref->x)) {
        target->x = -1;
    }
    if (target->y == ref->y || (ref->y == -1 && target->y == device_ref->y)) {
        target->y = -1;
    }
    if (target->w == ref->w || (ref->w == -1 && target->w == device_ref->w)) {
        target->w = -1;
    }
    if (target->h == ref->h || (ref->h == -1 && target->h == device_ref->h)) {
        target->h = -1;
    }
}

void normalize_screen_to_delta() {
    for (int i = 0; i < 2; i++) {
        bool act_orientation = i == 0;
        SDL_FRect *delta_frect = get_delta_screen_frect(act_orientation);
        SDL_FRect default_jzintv_rendering_frect = get_default_jzintv_rendering_frect(act_orientation);
        normalize_single_to_delta(&default_jzintv_rendering_frect, &default_jzintv_rendering_frect, delta_frect);

        for (int i = 0; i < app_config_struct.num_valid_crc32s; i++) {
            SDL_FRect *rom_frect = act_orientation ? &(roms_configuration[i].mobile_portrait_rect) : &(roms_configuration[i].mobile_landscape_rect);
            normalize_single_to_delta(&default_jzintv_rendering_frect, delta_frect, rom_frect);
        }

        for (int i = 0; i < roms_list_struct.total_roms_num; i++) {
            SDL_FRect *rom_frect = act_orientation ? &(roms_list_struct.list[i].mobile_portrait_rect) : &(roms_list_struct.list[i].mobile_landscape_rect);
            normalize_single_to_delta(&default_jzintv_rendering_frect, delta_frect, rom_frect);
        }
    }
}

void init_jzintv_rendering_rect(bool is_for_custom_game) {
    for (int i = 0; i < 2; i++) {
        bool act_orientation = i == 0;
        SDL_FRect *delta_frect = get_delta_screen_frect(act_orientation);
        SDL_FRect default_jzintv_rendering_frect = get_default_jzintv_rendering_frect(act_orientation);
        SDL_FRect jzintv_rendering_frect;

        if (delta_frect->x == -1) {
            jzintv_rendering_frect.x = default_jzintv_rendering_frect.x;
        } else {
            jzintv_rendering_frect.x = delta_frect->x;
        }
        if (delta_frect->y == -1) {
            jzintv_rendering_frect.y = default_jzintv_rendering_frect.y;
        } else {
            jzintv_rendering_frect.y = delta_frect->y;
        }
        if (delta_frect->w == -1) {
            jzintv_rendering_frect.w = default_jzintv_rendering_frect.w;
        } else {
            jzintv_rendering_frect.w = delta_frect->w;
        }
        if (delta_frect->h == -1) {
            jzintv_rendering_frect.h = default_jzintv_rendering_frect.h;
        } else {
            jzintv_rendering_frect.h = delta_frect->h;
        }

        if (is_for_custom_game) {
            SDL_FRect *rom_frect = get_rom_screen_frect(selected_rom, act_orientation);
            if (rom_frect->x != -1) {
                jzintv_rendering_frect.x = rom_frect->x;
            }
            if (rom_frect->y != -1) {
                jzintv_rendering_frect.y = rom_frect->y;
            }
            if (rom_frect->w != -1) {
                jzintv_rendering_frect.w = rom_frect->w;
            }
            if (rom_frect->h != -1) {
                jzintv_rendering_frect.h = rom_frect->h;
            }
        }

        SDL_Rect jzintv_rendering_rect = transform_to_sdl_rect(&jzintv_rendering_frect);

        memcpy(get_act_jzintv_rendering_rect(act_orientation), &jzintv_rendering_rect, sizeof(SDL_Rect));
        memcpy(get_act_jzintv_rendering_frect(act_orientation), &jzintv_rendering_frect, sizeof(SDL_FRect));
        *get_act_jzintv_rendering_rect_ref(act_orientation) = get_act_jzintv_rendering_rect(act_orientation);
        *get_act_jzintv_rendering_frect_ref(act_orientation) = get_act_jzintv_rendering_frect(act_orientation);
    }
}

extern "C" void update_jzintv_rendering_rect(SDL_Rect *rect2) {
    SDL_Rect **jzintv_rendering_rect_ptr = get_act_jzintv_rendering_rect_ref(screen_is_portrait);
    if (*jzintv_rendering_rect_ptr == NULL) {
        init_jzintv_rendering_rect(true);
    }
    memcpy(rect2, *jzintv_rendering_rect_ptr, sizeof(SDL_Rect));
}

extern "C" void update_screen_size() {
    get_window_size(&window_x, &window_y);
    Log(LOG_INFO) << "Screen size updated:" << window_x << "x" << window_y;
    if (window_x > window_y) {
        screen_is_portrait = false;
        window_ratio_portrait = (float) window_y / (float) window_x;
        window_ratio_landscape = (float) window_x / (float) window_y;
    } else {
        screen_is_portrait = true;
        window_ratio_portrait = (float) window_x / (float) window_y;
        window_ratio_landscape = (float) window_y / (float) window_x;
    }
    refresh_rect_on_screen_size_change();
    release_all_controls();
}

extern "C" void check_screen_change() {
    int x;
    int y;
    get_window_size(&x, &y);
    if (window_x != x || window_y != y) {
        #ifdef __ANDROID__
            // Sometimes (why??) status/navigation bars remain and this causes wrong computation for controls in LIBSDL
            mobile_force_fullscreen();
        #endif
        update_screen_size();
    }
}

//extern void change_screen_orientation(int wind_x, int wind_y) {
//    int max_ = max(wind_x, wind_y);
//    int min_ = min(wind_x, wind_y);
//    if (wind_x > wind_y) {
//        update_screen_orientation(max_, min_);
//    } else {
//        update_screen_orientation(min_, max_);
//    }
//    update_screen_size();
//}
