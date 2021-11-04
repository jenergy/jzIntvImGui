#ifndef JZINTVIMGUI_GUI_EVENTS_H
#define JZINTVIMGUI_GUI_EVENTS_H

// Gui events
#define SCROLL_EVENT 1
#define CLOSE_POPUP_EVENT 2
#define PREPARE_FOR_LAUNCH_GAME_EVENT 3
#define START_GAME_EVENT 4

class GuiEvent {
public:
    GuiEvent();

    ~GuiEvent();

    int event;
    int param_int;
    float param_float;
};

#endif