#include <imgui_internal.h>

class ImguiScrollable {
public:
    ImguiScrollable();

    ~ImguiScrollable();

    bool scroll(bool includeScrollbar = false);

    void stop_scroll();

    void start_autoscroll();

    ImGuiWindow* window;
    bool is_dragging;
    bool is_auto_scrolling;
    bool hover_unneeded;
private:
    void force_scroll(float index);
    bool is_position_in_window(float x, float y, bool includeScrollbar);
    bool is_drag_started_in_window();
    float start_scroll_position;
    long start_scroll_millis;
    float start_autoscroll_position;
    float end_scroll_position;
    long end_scroll_millis;
    bool can_auto_scroll;
    long diff_position;
    long diff_millis;
    float mouseX;
    float mouseY;
    float start_scroll_mouseY;
    float initial_autoscroll_velocity;
    float new_autoscroll_position;
    float initial_scrollbar_position;
    float forced_scrollbar_posY;
    float last_scrollbar_posY;
};
