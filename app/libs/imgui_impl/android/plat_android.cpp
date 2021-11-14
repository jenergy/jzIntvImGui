#include "imgui.h"
#include "SDL.h"
#include "logger.h"
#include <GLES2/gl2.h>
#include "main.h"
#include "imgui_impl_sdl_es2.h"
#include "imgui_impl_sdl_es3.h"
#include "jni.h"
#include <unistd.h>

static SDL_Window *window;
static SDL_GLContext ctx;
static ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
static bool is_ok_perm = false;

typedef bool(initImgui_t)(SDL_Window *);

typedef bool(processEvent_t)(SDL_Event *);

typedef void(newFrame_t)(SDL_Window *);

typedef void(shutdown_t)();

static initImgui_t *initImgui;
static processEvent_t *processEvent;
static newFrame_t *newFrame;
static shutdown_t *shutdown;

SDL_GLContext get_context() {
    return ctx;
}

extern "C" void set_window(SDL_Window * w) {
    window = w;
}

SDL_Window *get_window() {
    return window;
}

bool external_keyboard = false;
void check_release_backspace(bool mobile_mode) {
    if (mobile_mode && external_keyboard) {
       const Uint8* kbState = SDL_GetKeyboardState(NULL);
        if (!kbState[SDL_SCANCODE_BACKSPACE]) {
          ImGuiIO &io = ImGui::GetIO();
          io.KeysDown[SDL_SCANCODE_BACKSPACE] = 0;
        }
    }
}

static SDL_GLContext createCtx(SDL_Window *w) {
    // Prepare and create context
//    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
//    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
//    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
//    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
//    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
//    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
//    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
//    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
//    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    ctx = SDL_GL_CreateContext(w);

    if (!ctx) {
        Log(LOG_ERROR) << "Could not create context! SDL reports error: " << SDL_GetError();
        return ctx;
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

    if (major == 3) {
        Log(LOG_INFO) << "Initializing ImGui for GLES3";
        initImgui = ImGui_ImplSdlGLES3_Init;
        Log(LOG_INFO) << "Setting processEvent and newFrame functions appropriately";
        processEvent = ImGui_ImplSdlGLES3_ProcessEvent;
        newFrame = ImGui_ImplSdlGLES3_NewFrame;
        shutdown = ImGui_ImplSdlGLES3_Shutdown;
    } else {
        Log(LOG_INFO) << "Initializing ImGui for GLES2";
        initImgui = ImGui_ImplSdlGLES2_Init;
        Log(LOG_INFO) << "Setting processEvent and newFrame functions appropriately";
        processEvent = ImGui_ImplSdlGLES2_ProcessEvent;
        newFrame = ImGui_ImplSdlGLES2_NewFrame;
        shutdown = ImGui_ImplSdlGLES2_Shutdown;
    }
    Log(LOG_INFO) << "Finished initialization";
    return ctx;
}

extern void ImGui_ImplSdlGLES2_RenderDrawLists(ImDrawData *draw_data);
void render() {
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui::Render();
    ImGui_ImplSdlGLES2_RenderDrawLists(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
}

int setup_window(int desired_w, int desired_h, bool maximized, char *title) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    Log(LOG_INFO) << "Creating SDL_Window";
    int maximized_flag = maximized ? SDL_WINDOW_MAXIMIZED : 0;
    window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, desired_w,
                              desired_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | maximized_flag);
    ctx = createCtx(window);
    initImgui(window);
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
        shutdown();
        ImGui::DestroyContext();
        SDL_GL_DeleteContext(ctx);
        SDL_Quit();
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
        switch (event.type)
        {
            case SDL_KEYUP:
                int key = event.key.keysym.scancode;
                if (key == SDL_SCANCODE_BACKSPACE) {
                    check_release_backspace(config->mobile_mode);
                }
                break;
        }
        if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && (event.key.keysym.sym == SDLK_AC_BACK || (event.key.keysym.sym == SDLK_ESCAPE && !escape_pressed )))) {
            *exit = !check_back_config_window();
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                escape_pressed = true;
            }
        } else if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE && escape_pressed) {
            escape_pressed = false;
        }
    }
}

JavaVM *javaVM = NULL;
jclass activityClass;
jobject activityObj;

extern "C" JNIEXPORT jstring

JNICALL Java_com_jzintvimgui_MainActivity_initApplicationNative(JNIEnv *env, jobject obj) {
    env->GetJavaVM(&javaVM);
    jclass cls = env->GetObjectClass(obj);
    activityClass = (jclass) env->NewGlobalRef(cls);
    activityObj = env->NewGlobalRef(obj);
    return NULL;
}

extern "C" JNIEXPORT jstring

JNICALL Java_com_jzintvimgui_MainActivity_setOkPermission(JNIEnv *env, jobject obj) {
    is_ok_perm = true;
    return NULL;
}

extern bool wait_for_click_to_reopen_keyb;
extern "C" JNIEXPORT jstring
JNICALL Java_com_jzintvimgui_MainActivity_setClosedSoftKeyboard(JNIEnv *env, jobject obj) {
    wait_for_click_to_reopen_keyb = true;
    return NULL;
}

extern "C" JNIEXPORT jstring

JNICALL Java_com_jzintvimgui_MainActivity_sendQuit(JNIEnv *env, jobject obj) {
    SDL_Event event;
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = SDLK_AC_BACK;
    SDL_PushEvent(&event);
    return NULL;
}

char *get_root_folder_for_configuration() {
    JNIEnv *env;
    javaVM->AttachCurrentThread(&env, NULL);
    jmethodID method = env->GetMethodID(activityClass, "getRootFolderForConfiguration", "()Ljava/lang/String;");
    jstring s = (jstring) env->CallObjectMethod(activityObj, method);
    jboolean isCopy;
    const char *res = env->GetStringUTFChars(s, &isCopy);
    env->DeleteLocalRef(s);
    return strdup(res);
}

char *get_internal_sd_path() {
    JNIEnv *env;
    javaVM->AttachCurrentThread(&env, NULL);
    jmethodID method = env->GetMethodID(activityClass, "getInternalSdPath", "()Ljava/lang/String;");
    jstring s = (jstring) env->CallObjectMethod(activityObj, method);
    jboolean isCopy;
    const char *res = env->GetStringUTFChars(s, &isCopy);
    env->DeleteLocalRef(s);
    return strdup(res);
}

char *get_external_sd_path() {
    JNIEnv *env;
    javaVM->AttachCurrentThread(&env, NULL);
    jmethodID method = env->GetMethodID(activityClass, "getExternalSdPath", "()Ljava/lang/String;");
    jstring s = (jstring) env->CallObjectMethod(activityObj, method);
    jboolean isCopy;
    const char *res = env->GetStringUTFChars(s, &isCopy);
    env->DeleteLocalRef(s);
    return strdup(res);
}

bool is_ok_permission() {
    return is_ok_perm;
}

bool get_mobile_mode() {
    return true;
}

bool get_default_mobile_show_controls() {
    return true;
}

bool get_default_mobile_show_configuration_controls() {
    return true;
}

bool get_force_fullscreen() {
    return true;
}

char* get_forced_resolution_argument() {
    return strdup("-z3");
}

extern "C" void update_screen_size();
extern "C" JNIEXPORT jstring
JNICALL Java_com_jzintvimgui_MainActivity_updateScreenSize(JNIEnv *env, jobject obj) {
    update_screen_size();
    return NULL;
}

extern "C" JNIEXPORT jstring
JNICALL Java_com_jzintvimgui_MainActivity_enableTextInputForPhysicalKeyboard(JNIEnv *env) {
    external_keyboard = true;
    SDL_StartTextInput();
    return NULL;
}

extern "C" JNIEXPORT jstring
JNICALL Java_com_jzintvimgui_MainActivity_disableTextInputForPhysicalKeyboard(JNIEnv *env) {
    external_keyboard = false;
    SDL_StartTextInput();
    return NULL;
}

void init_platform(int argc, char **argv) {
    while (!is_ok_permission()) {
        sleep(1);
    }
    if (argc > 1) {
        Log(LOG_INFO) << "Arrivato:" << argv[1];
        if (chdir(argv[1])) {
            Log(LOG_ERROR) << "Could not change directory properly!";
        }
    }
}

SDL_FRect get_default_jzintv_rendering_frect(bool is_portrait) {
    SDL_FRect tmp;
    float ratio = 1.20;
    float real_win_x;
    float real_win_y;
    if (!is_portrait) {
        // Landscape
        tmp.h = 100;
        tmp.y = 0;
        if (window_x > 0) {
            real_win_x = window_x > window_y ? window_x : window_y;
            real_win_y = window_x > window_y ? window_y : window_x;
            float real_pixel = ((float) real_win_y) * ratio;
            tmp.w = (((float) 100) * real_pixel) / (float) real_win_x;
            tmp.x = (100 - tmp.w) / 2;
        } else {
            tmp.w = -1;
            tmp.x = -1;
        }
    } else {
        // Portrait
        tmp.w = 100;
        tmp.x = 0;
        tmp.y = 5;
        if (window_x > 0) {
            real_win_x = window_x > window_y ? window_y : window_x;
            real_win_y = window_x > window_y ? window_x : window_y;
            float real_pixel = ((float) real_win_x) / ratio;
            tmp.h = (((float) 100) * real_pixel) / (float) real_win_y;
        } else {
            tmp.h = -1;
        }
    }
    return tmp;
}

int get_default_font_size() {
    return 55;
}

int get_default_scrollbar_size() {
    return 85;
}

int get_default_buttons_size() {
    return 90;
}

void openUrl(string url) {
    JNIEnv *env;
    javaVM->AttachCurrentThread(&env, NULL);
    jmethodID method = env->GetMethodID(activityClass, "openUrl", "(Ljava/lang/String;)Ljava/lang/String;");
    jstring jStringParam = env->NewStringUTF( url.c_str() );
    jstring s = (jstring) env->CallObjectMethod(activityObj, method, jStringParam);
    jboolean isCopy;
    const char *res = env->GetStringUTFChars(s, &isCopy);
    env->DeleteLocalRef(s);
    env->DeleteLocalRef(jStringParam);
}

void custom_show_message(string message) {
    Log(LOG_INFO) << message;
    JNIEnv *env;
    javaVM->AttachCurrentThread(&env, NULL);
    jmethodID method = env->GetMethodID(activityClass, "showToast", "(Ljava/lang/String;)Ljava/lang/String;");
    jstring jStringParam = env->NewStringUTF( message.c_str() );
    jstring s = (jstring) env->CallObjectMethod(activityObj, method, jStringParam);
    jboolean isCopy;
    const char *res = env->GetStringUTFChars(s, &isCopy);
    env->DeleteLocalRef(s);
    env->DeleteLocalRef(jStringParam);
}

void emulation_start() {
    SDL_StopTextInput();
    JNIEnv *env;
    javaVM->AttachCurrentThread(&env, NULL);
    jmethodID method = env->GetMethodID(activityClass, "emulationStart", "()Ljava/lang/String;");
    jstring s = (jstring) env->CallObjectMethod(activityObj, method);
    jboolean isCopy;
    const char *res = env->GetStringUTFChars(s, &isCopy);
    env->DeleteLocalRef(s);
}

void emulation_end() {
    JNIEnv *env;
    javaVM->AttachCurrentThread(&env, NULL);
    jmethodID method = env->GetMethodID(activityClass, "emulationEnd", "()Ljava/lang/String;");
    jstring s = (jstring) env->CallObjectMethod(activityObj, method);
    jboolean isCopy;
    const char *res = env->GetStringUTFChars(s, &isCopy);
    env->DeleteLocalRef(s);
}

void on_render(bool mobile_mode) {
    ImGuiIO &io = ImGui::GetIO();
    io.KeysDown[SDL_SCANCODE_BACKSPACE] = 0;
}