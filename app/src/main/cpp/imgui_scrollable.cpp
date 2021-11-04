#include <imgui.h>
#include <imgui_internal.h>
#include "imgui_scrollable.h"

extern long get_act_millis();

ImguiScrollable::ImguiScrollable() : window(NULL),
                                     start_scroll_position(-1),
                                     start_scroll_millis(0),
                                     start_autoscroll_position(-1),
                                     end_scroll_position(-1),
                                     end_scroll_millis(0),
                                     can_auto_scroll(false),
                                     is_auto_scrolling(false),
                                     diff_position(0),
                                     diff_millis(0),
                                     is_dragging(0),
                                     mouseX(0.0f),
                                     mouseY(0.0f),
                                     start_scroll_mouseY(0.0f),
                                     initial_autoscroll_velocity(0.0f),
                                     new_autoscroll_position(0.0f),
                                     initial_scrollbar_position(0.0f),
                                     forced_scrollbar_posY(0.0f),
                                     last_scrollbar_posY(-10.0f),
                                     hover_unneeded(false) {}

ImguiScrollable::~ImguiScrollable() {}

static int compute_end_drag_position(ImGuiWindow *window, float last_mouse_pos, int first, int last) {
    int tot_elements_viewed = last - first;
    int px_for_element = window->Size.y / tot_elements_viewed;
    int relative_position_mouse = last_mouse_pos - window->Pos.y;
    int relative_position_element = relative_position_mouse / px_for_element;
    return relative_position_element;
}

static int diff(int num1, int num2) {
    int res = num1 - num2;
    if (res < 0) {
        res = -res;
    }
    return res;
}

static int diff_float(float num1, float num2) {
    float res = num1 - num2;
    if (res < 0) {
        res = -res;
    }
    return res;
}

static float get_new_autoscroll_position(float initial_velocity, long millis_passed) {
//    s = V0t - 1/2 at2
    float decel = 0.003;
    bool direction_thumb_up = initial_velocity < 0;
    float abs_initial_velocity = direction_thumb_up ? -initial_velocity : initial_velocity;
    // V0t
    float initial_space = abs_initial_velocity * (float) millis_passed;

    // 1/2 at2
    float final_space = 0.5 * decel * (float) (millis_passed * millis_passed);

    float res = initial_space - final_space;
    return direction_thumb_up ? -res : res;
}

void ImguiScrollable::force_scroll(float index) {
    window->ScrollTargetCenterRatio.y = 0.0f;
    window->ScrollTarget.y = index;
}

bool ImguiScrollable::is_position_in_window(float x, float y, bool includeScrollbar) {
    bool is_mouse_in_window = (x > window->Pos.x &&
                               x < window->Size.x + window->Pos.x - (includeScrollbar ? 0 : (window->ScrollbarSizes.x + 5)) &&
                               y > window->Pos.y &&
                               y < window->Size.y + window->Pos.y);
    return is_mouse_in_window;
}

bool ImguiScrollable::is_drag_started_in_window() {
    ImGuiContext &g = *GImGui;
    ImVec2 *vec = &g.IO.MouseClickedPos[0];
    return is_position_in_window(vec->x, vec->y, false);
}

void ImguiScrollable::stop_scroll() {
    can_auto_scroll = false;
    is_auto_scrolling = false;
    is_dragging = false;
}

void ImguiScrollable::start_autoscroll() {
    if (is_dragging) {
        // Start autoScroll
        diff_position = mouseY - start_autoscroll_position;
        if (can_auto_scroll) {
            is_auto_scrolling = true;
            initial_autoscroll_velocity =
                    (float) diff_position / (float) diff_millis;
            start_scroll_millis = get_act_millis();
            initial_scrollbar_position = window->Scroll.y;
            last_scrollbar_posY = -10.0f;
        } else {
            is_auto_scrolling = false;
        }
    }
    is_dragging = false;
}

bool ImguiScrollable::scroll(bool includeScrollbar) {
    mouseX = ImGui::GetIO().MousePos.x;
    mouseY = ImGui::GetIO().MousePos.y;
    bool is_mouse_in_window = is_position_in_window(mouseX, mouseY, includeScrollbar);

    ImGuiContext &g = *GImGui;
    ImGuiID id = g.HoveredWindow != NULL ? g.HoveredWindow->ID : 0;
    ImGuiID act_id = window->ID;
    bool is_window_hovered = (id == act_id && id > 0);

    if (is_mouse_in_window) {
        if (ImGui::IsMouseClicked(0) && (is_window_hovered || hover_unneeded)) {
            can_auto_scroll = false;
            is_auto_scrolling = false;
            start_scroll_mouseY = mouseY;
        } else if (ImGui::IsMouseDragging(0, 5) && !ImGui::IsMouseReleased(0) && is_drag_started_in_window()) {
            if (!is_dragging) {
                // Start dragging for scrolling
                start_scroll_position = start_scroll_mouseY;
                start_scroll_millis = get_act_millis();
            } else {
                // Continuing dragging for scrolling
                end_scroll_position = mouseY;
                end_scroll_millis = get_act_millis();
                diff_position = end_scroll_position - start_scroll_position;
                long diff_position_abs = diff_position > 0 ? diff_position : -diff_position;
                diff_millis = end_scroll_millis - start_scroll_millis;
                if (diff_position != 0) {
                    forced_scrollbar_posY = window->Scroll.y - diff_position;
                    force_scroll(forced_scrollbar_posY);
                }

                if (diff_position_abs > 5) {
                    if (!can_auto_scroll) {
                        can_auto_scroll = true;
                        start_scroll_millis = get_act_millis();
                        start_autoscroll_position = mouseY;
                    }
                } else {
                    if (diff_millis > 250) {
                        can_auto_scroll = false;
                    }
                }
                start_scroll_position = end_scroll_position;
            }
            is_dragging = true;
        } else {
            start_autoscroll();
        }
    } else {
        start_autoscroll();
    }
    bool mouse_clicked = ImGui::IsMouseClicked(0);
    if (mouse_clicked && is_auto_scrolling) {
        is_auto_scrolling = false;
    }
    if (is_auto_scrolling) {
        end_scroll_millis = get_act_millis();
        diff_millis = end_scroll_millis - start_scroll_millis;
        new_autoscroll_position = get_new_autoscroll_position(initial_autoscroll_velocity,
                                                              diff_millis);
        forced_scrollbar_posY = initial_scrollbar_position - new_autoscroll_position;
        if (diff_float(forced_scrollbar_posY, last_scrollbar_posY) < 3) {
            is_auto_scrolling = false;
        } else {
            force_scroll(forced_scrollbar_posY);
            last_scrollbar_posY = forced_scrollbar_posY;
        }

        if (forced_scrollbar_posY <= 0 || forced_scrollbar_posY >= window->ScrollMax.y) {
            stop_scroll();
        }
    }
    return is_mouse_in_window;
}