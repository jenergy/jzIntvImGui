#include "main.h"
#include "includes_specific.h"
#include "stb_image.h"

static GLuint game_image_texture;
static int game_image_width;
static int game_image_height;

static GLuint empty_game_image_texture = 0;
static int empty_game_image_width;
static int empty_game_image_height;

static GLuint background_image_texture = 0;
static int background_image_width;
static int background_image_height;

static GLuint loading_bt_image_texture[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int loading_bt_image_width;
static int loading_bt_image_height;

static GLuint black_image_texture = 0;
static int black_image_width;
static int black_image_height;

long loading_millis = -1;

// OpenGL, direttamente da Dear ImGui https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
// Simple helper function to load an image into a OpenGL texture with common settings
static bool LoadTextureFromFile(const char *filename, GLuint *out_texture, int *out_width, int *out_height) {
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char *image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

static void load_image(struct app_config_struct_t *app_conf, GLuint *out_texture, int *width, int *height, char *file_name) {
    char buf[FILENAME_MAX];
    *out_texture = 0;
    sprintf(buf, "%s%s/%s", app_conf->resource_folder_absolute_path, "Images", file_name);
    if (!exist_file(buf)) {
        ADD_WARNING("Unable to find asset file: " << buf);
    } else {
        if (!LoadTextureFromFile(buf, out_texture, width, height)) {
            ADD_WARNING("Error loading asset file:" << buf);
        }
    }
}

void load_images(struct app_config_struct_t *app_conf) {
    load_image(app_conf, &empty_game_image_texture, &empty_game_image_width, &empty_game_image_height,
               (char *) "Interface/empty_game.png");
    load_image(app_conf, &background_image_texture, &background_image_width, &background_image_height,
               (char *) "Interface/background.png");
    load_image(app_conf, &black_image_texture, &black_image_width, &black_image_height, (char *) "Interface/black.png");
    ostringstream ss;
    for (int i = 0; i < 10; i++) {
        ss.str("");
        ss.clear();
        ss << "Interface/loading_bt_" << (i + 1) << ".png";
        load_image(app_conf, &(loading_bt_image_texture[i]), &loading_bt_image_width, &loading_bt_image_height, (char *) ss.str().c_str());
    }
}

void clear_general_textures() {
    if (empty_game_image_texture != 0) {
        glDeleteTextures(1, &empty_game_image_texture);
    }
    empty_game_image_texture = 0;

    if (background_image_texture != 0) {
        glDeleteTextures(1, &background_image_texture);
    }
    background_image_texture = 0;

    if (black_image_texture != 0) {
        glDeleteTextures(1, &black_image_texture);
    }
    black_image_texture = 0;

    for (int i = 0; i < 10; i++) {
        if (loading_bt_image_texture[i] != 0) {
            glDeleteTextures(1, &(loading_bt_image_texture[i]));
        }
        loading_bt_image_texture[i] = 0;
    }
}

void clear_roms_textures(struct roms_list_struct_t *roms_list_struct) {
    for (int i = 0; i < roms_list_struct->total_roms_num; i++) {
        rom_config_struct_t *rom_config = &(roms_list_struct->list[i]);
        if (rom_config->texture_screenshot > 0) {
            glDeleteTextures(1, &(rom_config->texture_screenshot));
        }
        rom_config->texture_screenshot = 0;

        if (rom_config->texture_box > 0) {
            glDeleteTextures(1, &(rom_config->texture_box));
        }
        rom_config->texture_box = 0;
    }
}

void manage_image_window(ImVec2 size,
                         struct roms_list_struct_t *roms_list_struct,
                         struct app_config_struct_t *app_config_struct,
                         int tab_selected,
                         int rom_index_selected,
                         uint32_t mobile_last_landscape_scrollbar_pos,
                         ImguiScrollable *roms_list_scrollable,
                         long *last_millis_checked,
                         int *last_rom_index_checked) {

    char sub_folder[100];
    bool is_screenshot = tab_selected == 0;
    uint32_t *txtr;
    int *orig_width;
    int *orig_height;
    if (is_screenshot) {
        sprintf(sub_folder, "%s", "Images/Screenshots");
        game_image_texture = roms_list_struct->list[rom_index_selected].texture_screenshot;
        txtr = &roms_list_struct->list[rom_index_selected].texture_screenshot;
        orig_width = &roms_list_struct->list[rom_index_selected].original_texture_screenshot_width;
        orig_height = &roms_list_struct->list[rom_index_selected].original_texture_screenshot_height;
    } else {
        sprintf(sub_folder, "%s", "Images/Boxes");
        game_image_texture = roms_list_struct->list[rom_index_selected].texture_box;
        txtr = &roms_list_struct->list[rom_index_selected].texture_box;
        orig_width = &roms_list_struct->list[rom_index_selected].original_texture_box_width;
        orig_height = &roms_list_struct->list[rom_index_selected].original_texture_box_height;
    }

    if (game_image_texture == 0) {
        bool loadImage = false;
        if (!app_config_struct->mobile_mode) {
            loadImage = true;
        } else {
            // In mobile mode we wait for list stopped, before loading an image (only landscape)
            long millis = get_act_millis();
            if (*last_millis_checked == 0) {
                *last_millis_checked = millis;
            }
            if (roms_list_scrollable->window->Scroll.y == mobile_last_landscape_scrollbar_pos &&
                *last_rom_index_checked == rom_index_selected) {
                if (millis - *last_millis_checked > 150 && !roms_list_scrollable->is_dragging &&
                    !roms_list_scrollable->is_auto_scrolling) {
                    loadImage = true;
                }
            } else {
                *last_millis_checked = millis;
            }
            *last_rom_index_checked = rom_index_selected;
        }
        if (loadImage) {
            char *act_image_ptr = is_screenshot ? roms_list_struct->list[rom_index_selected].image_file_name : roms_list_struct->list[rom_index_selected].box_file_name;
            if (act_image_ptr != NULL) {
                char complete_filename[FILENAME_MAX];
                sprintf(complete_filename, "%s%s/%s", app_config_struct->resource_folder_absolute_path, sub_folder,
                        (is_screenshot ? roms_list_struct->list[rom_index_selected].image_file_name : roms_list_struct->list[rom_index_selected].box_file_name));
                if (exist_file(complete_filename)) {
                    bool res = LoadTextureFromFile(complete_filename, &game_image_texture, &game_image_width, &game_image_height);
                    if (res) {
                        *txtr = game_image_texture;
                        *orig_width = game_image_width;
                        *orig_height = game_image_height;
                    } else {
                        ADD_CONFIG_WARNING("Unable to load image: " << complete_filename);
                        *txtr = -1;
                    }
                } else {
                    ADD_CONFIG_WARNING("Unable to find image: " << complete_filename);
                    *txtr = -1;
                }
            } else {
                *txtr = -1;
            }
            game_image_texture = *txtr;
        } else {
            *orig_width = NO_GAME_IMAGE_X;
            *orig_height = NO_GAME_IMAGE_Y;
        }
    }
    int act_image_height = size.y;
    int act_image_width = (act_image_height *
                           ((game_image_texture != (GLuint) -1)
                            ? *orig_width
                            : NO_GAME_IMAGE_X) /
                           ((game_image_texture != (GLuint) -1)
                            ? *orig_height
                            : NO_GAME_IMAGE_Y));
    if (act_image_width > size.x) {
        act_image_width = size.x;
        act_image_height = (act_image_width *
                            ((game_image_texture != (GLuint) -1)
                             ? *orig_height
                             : NO_GAME_IMAGE_Y) /
                            ((game_image_texture != (GLuint) -1)
                             ? *orig_width
                             : NO_GAME_IMAGE_X));
    }
    ImVec2 pos = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(pos.x + (float) ((size.x - act_image_width) / 2), pos.y + (float) ((size.y - act_image_height) / 2)));

    if ((game_image_texture != (GLuint) -1)) {
        ImGui::Image((void *) (intptr_t) game_image_texture, ImVec2(act_image_width, act_image_height));
    } else if (empty_game_image_texture != 0) {
        ImGui::Image((void *) (intptr_t) empty_game_image_texture, ImVec2(act_image_width, act_image_height));
    } else {
        ImGui::Text("Image not available");
    }
}

void draw_background(ImVec2 vec) {
    if (background_image_texture != 0) {
        const ImVec4 col_v4(0.0f, 0.0f, 1.0f, 0.7f);
        ImGui::Image((void *) (intptr_t) background_image_texture, vec, ImVec2(0, 0), ImVec2(1, 1), col_v4);
    }
}

void draw_loading(ImVec2 vec) {
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoScrollbar;
    window_flags |= ImGuiWindowFlags_NoScrollWithMouse;

    int left_gap_pixels = 0;
    int right_gap_pixels = 0;
    int top_gap_pixels = 0;
    int bottom_gap_pixels = 0;
    get_gap_pixels(&left_gap_pixels, &right_gap_pixels, &top_gap_pixels, &bottom_gap_pixels);

    ImGui::SetNextWindowPos(ImVec2{static_cast<float>(left_gap_pixels), static_cast<float>(top_gap_pixels)}, ImGuiCond_Always);
    ImGui::SetNextWindowSize(vec, ImGuiCond_Always);
    ImGui::SetNextWindowFocus();
    ImGui::Begin("Loading", NULL, window_flags);

    ImVec2 curPos = ImGui::GetCursorPos();
    long act_millis = get_act_millis();
    int bt_index = (act_millis - loading_millis) / 100;
    while (bt_index > 9) {
        bt_index -= 10;
    }
    if (app_config_struct.mobile_mode && black_image_texture != 0) {
        const ImVec4 col_v4(0.0f, 0.0f, 1.0f, 0.7f);
        ImGui::Image((void *) (intptr_t) black_image_texture, vec);
    }

    if (loading_bt_image_texture[bt_index] != 0) {
        const ImVec4 col_v4(1.0f, 1.0f, 1.0f, 0.4f);
        curPos.y += (vec.y / 2.5);
        curPos.x += (vec.x / 4);
        vec.x /= 2;
        vec.y = vec.x / 7.05;
        ImGui::SetCursorPos(curPos);
        if (app_config_struct.mobile_mode) {
            ImGui::Image((void *) (intptr_t) loading_bt_image_texture[bt_index], vec);
        } else {
            ImGui::Image((void *) (intptr_t) loading_bt_image_texture[bt_index], vec, ImVec2(0, 0), ImVec2(1, 1), col_v4);
        }
    }

    ImGui::End();
}
