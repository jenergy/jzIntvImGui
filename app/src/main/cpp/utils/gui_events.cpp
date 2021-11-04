#include "main.h"

GuiEvent::GuiEvent() {}

GuiEvent::~GuiEvent() {}

std::vector<GuiEvent *> gui_events;

void submit_gui_event(int event, int param_int, float param_float) {
    std::vector<GuiEvent *>::iterator events_it = gui_events.begin();
    while (events_it != gui_events.end()) {
        if ((*events_it)->event == event) {
            delete (*events_it);
            gui_events.erase(events_it);
            break;
        }
        events_it++;
    }

    GuiEvent *g = new GuiEvent();
    g->event = event;
    g->param_int = param_int;
    g->param_float = param_float;
    gui_events.push_back(g);
}