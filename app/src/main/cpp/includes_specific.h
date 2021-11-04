#ifndef WIN32
#include <ImGuiFileDialog.h>
#endif

#if defined(WIN32) || defined(LINUX)
#include <unistd.h>

#ifdef __ANDROID__
#include <unistd.h>
#include <SDL_opengl.h>
#include <SDL.h>
#include "jni.h"
#include <android/asset_manager.h>
extern "C" {
extern JNIEnv *Android_JNI_GetEnv(void);
extern const char *SDL_AndroidGetExternalStoragePath(void);
}
extern jobject assetManager;
#else
#include <imgui_impl_sdl_gl3.h>
#include <glad/glad.h>
#endif

#elif __SWITCH__
#include "imgui_impl_glfw_switch.h"
#include "imgui_impl_opengl3.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#define GLM_FORCE_PURE
#define GLM_ENABLE_EXPERIMENTAL
#include <switch.h>

#endif