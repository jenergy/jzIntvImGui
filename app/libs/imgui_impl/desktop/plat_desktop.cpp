#include "imgui.h"
#include "SDL.h"
#include "logger.h"
#include <glad/glad.h>
#include "imgui_impl_sdl_gl3.h"
#include "main.h"

static SDL_Window *window;
static SDL_GLContext ctx;
static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

typedef bool(initImgui_t)(SDL_Window *, const char *);

typedef bool(processEvent_t)(SDL_Event *);

typedef void(newFrame_t)(SDL_Window *);

typedef void(shutdown_t)();

static initImgui_t *initImgui;
static processEvent_t *processEvent;
static newFrame_t *newFrame;
static shutdown_t *shutdownGui;

extern char *get_curr_folder();

SDL_GLContext get_context() {
    return ctx;
}

extern "C" void set_window(SDL_Window *w) {
    window = w;
}

SDL_Window *get_window() {
    return window;
}

static SDL_GLContext createCtx(SDL_Window *w) {
    // Prepare and create context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    ctx = SDL_GL_CreateContext(w);

    if (!ctx) {
        Log(LOG_ERROR) << "Could not create context! SDL reports error: " << SDL_GetError();
        return ctx;
    }
    if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress)) {
        Log(LOG_ERROR) << "[ERROR] Couldn't initialize glad" << std::endl;
    } else {
        Log(LOG_INFO) << "[INFO] glad initialized\n";
    }
    ImGui::CreateContext();
    int major, minor, mask;
    int r, g, b, a, depth;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &mask);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
    SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &r);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &g);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &b);
    SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &a);
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depth);

    const char *mask_desc;

    if (mask & SDL_GL_CONTEXT_PROFILE_CORE) {
        mask_desc = "core";
    } else if (mask & SDL_GL_CONTEXT_PROFILE_COMPATIBILITY) {
        mask_desc = "compatibility";
    } else if (mask & SDL_GL_CONTEXT_PROFILE_ES) {
        mask_desc = "es";
    } else {
        mask_desc = "?";
    }

    Log(LOG_INFO) << "Got context: " << major << "." << minor << mask_desc
                  << ", R" << r << "G" << g << "B" << b << "A" << a << ", depth bits: " << depth;

    SDL_GL_MakeCurrent(w, ctx);
    initImgui = ImGui_ImplSdlGL3_Init;
    processEvent = ImGui_ImplSdlGL3_ProcessEvent;
    newFrame = ImGui_ImplSdlGL3_NewFrame;
    shutdownGui = ImGui_ImplSdlGL3_Shutdown;
    Log(LOG_INFO) << "Finished initialization";
    return ctx;
}

void render() {
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui::Render();
    ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
}

int setup_window(int desired_w, int desired_h, bool maximized, char *title) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    Log(LOG_INFO) << "Creating SDL_Window";
    int maximized_flag = maximized ? SDL_WINDOW_MAXIMIZED : 0;
    window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, desired_w,
                              desired_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | maximized_flag);
    SDL_SetWindowMinimumSize(window, 640, 480);
    ctx = createCtx(window);
    initImgui(window, NULL);
    return true;
}

void move_mouse(int x, int y) {
    SDL_WarpMouseInWindow(window, x, y);
}

void new_frame() {
    newFrame(window);
}

void get_window_size(int *w, int *h) {
    SDL_GetWindowSize(window, w, h);
}

void clean(int mode) {
    if (mode == AFTER_EMULATION || mode == EXITING) {
        shutdownGui();
        ImGui::DestroyContext();
        SDL_GL_DeleteContext(ctx);
    }
    if (mode == BEFORE_EMULATION || mode == EXITING) {
        SDL_DestroyWindow(window);
    }
    if (mode == AFTER_EMULATION || mode == EXITING) {
        SDL_Quit();
    }
}

bool escape_pressed = false;
void check_for_special_event(bool *exit, app_config_struct_t *config) {
    *exit = false;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        processEvent(&event);
        if (event.type == SDL_QUIT) {
            *exit = true;
        } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE && !escape_pressed) {
            *exit = !check_back_config_window();
            escape_pressed = true;
        } else if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE && escape_pressed) {
            escape_pressed = false;
        } else {
            switch (event.window.event) {
                case SDL_WINDOWEVENT_MAXIMIZED:
                    config->window_maximized = true;
                    break;
                case SDL_WINDOWEVENT_RESTORED:
                    config->window_maximized = false;
                    break;
                default:
                    break;
            }
        }
    }
}

char *get_root_folder_for_configuration() {
    return get_curr_folder();
}

bool is_ok_permission() {
    return true;
}

bool get_mobile_mode() {
    return false;
}

bool get_default_mobile_show_controls() {
    return false;
}

bool get_default_mobile_show_configuration_controls() {
    return false;
}

bool get_force_fullscreen() {
    return false;
}

bool can_launch_external_jzintv() {
    return true;
}

char *get_forced_resolution_argument() {
    return NULL;
}

SDL_FRect get_default_jzintv_rendering_frect(bool is_portrait) {
    SDL_FRect tmp;
    tmp.x = 0;
    tmp.y = 0;
    tmp.w = 100;
    tmp.h = 100;
    return tmp;
}

int get_default_font_size() {
    return 15;
}

int get_default_scrollbar_size() {
    return 18;
}

int get_default_buttons_size() {
    return 22;
}

#ifdef WIN32

#include "winuser.h"
#include "shellapi.h"

void openUrl(string url) {
    ShellExecute(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

#endif

void custom_show_message(string message) {
    Log(LOG_INFO) << message;
}

void on_font_change() {
    SDL_StartTextInput();
}

void init_platform(int argc, char **argv) {}
void emulation_start() {}
void emulation_end() {}
void on_render() {}
