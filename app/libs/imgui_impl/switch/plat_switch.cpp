#include "imgui_impl_glfw_switch.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "main.h"

#define GLM_FORCE_PURE
#define GLM_ENABLE_EXPERIMENTAL

#include <switch.h>
#include <chrono>
#include <set>
#include <thread>

static GLFWwindow *window;
static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

extern char *get_curr_folder();

static void errorCallback(int errorCode, const char *description) {
    fprintf(stderr, "Glfw Error %d: %s\n", errorCode, description);
}

int setup_window(int desired_w, int desired_h, bool maximized, char *title) {
    socketInitializeDefault();

    nxlinkStdio();

    // Init rng
    std::srand(std::time(nullptr));

    // Init glfw
    glfwSetErrorCallback(errorCallback);

    // glfwInitHint(GLFW_JOYSTICK_HAT_BUTTONS, GLFW_FALSE);

    if (!glfwInit()) {
        return false;
    }
    const char *glsl_version = "#version 430 core";

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (maximized) {
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    } else {
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);
    }

    window = glfwCreateWindow(desired_w, desired_h, title, nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return false;
    }

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwMakeContextCurrent(window);

    // Load OpenGL routines using glad
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval(1);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    io.Fonts->AddFontDefault();
    {
        plInitialize(PlServiceType_System);
        static PlFontData stdFontData, extFontData;

        PlFontData fonts_std;
        PlFontData fonts_ext;

        plGetSharedFontByType(&fonts_std, PlSharedFontType_Standard);
        plGetSharedFontByType(&fonts_ext, PlSharedFontType_NintendoExt);

        ImFontConfig config;
        config.FontDataOwnedByAtlas = false;

        strcpy(config.Name, "Nintendo Standard");
        io.Fonts->AddFontFromMemoryTTF(fonts_std.address, fonts_std.size, 24.0f, &config,
                                       io.Fonts->GetGlyphRangesCyrillic());

        strcpy(config.Name, "Nintendo Ext");
        static const ImWchar ranges[] =
                {
                        0xE000, 0xE06B,
                        0xE070, 0xE07E,
                        0xE080, 0xE099,
                        0xE0A0, 0xE0BA,
                        0xE0C0, 0xE0D6,
                        0xE0E0, 0xE0F5,
                        0xE100, 0xE105,
                        0xE110, 0xE116,
                        0xE121, 0xE12C,
                        0xE130, 0xE13C,
                        0xE140, 0xE14D,
                        0xE150, 0xE153,
                        0,
                };

        io.Fonts->AddFontFromMemoryTTF(fonts_ext.address, fonts_ext.size, 24.0f, &config, ranges);
        io.Fonts->Build();

        plExit();
    }
    return 0;
}

void move_mouse(int x, int y) {
    //SDL_WarpMouseInWindow(window, x, y);
}

void new_frame() {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void get_window_size(int *w, int *h) {
    glfwGetFramebufferSize(window, w, h);
}

void render() {
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

void clean(int mode) {
    if (mode == AFTER_EMULATION || mode == EXITING) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    if (mode == BEFORE_EMULATION || mode == EXITING) {
        glfwDestroyWindow(window);
    }
    if (mode == AFTER_EMULATION || mode == EXITING) {
        glfwTerminate();
        socketExit();
    }
}

void check_for_special_event(bool *exit, app_config_struct_t* config) {
    *exit = glfwWindowShouldClose(window);
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

char* get_forced_resolution_argument() {
    return strdup("-z4");
}

bool get_force_fullscreen() {
    return true;
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
