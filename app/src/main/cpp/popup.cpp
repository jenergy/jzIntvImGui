#include "main.h"

Popup::Popup(const char *title_param, const char *message_param) {
    title = strdup(title_param);
    messages.insert(string(message_param));
}

Popup::~Popup() {
    free(title);
    messages.clear();
}

Popup *popup = NULL;

void add_popup_by_stream(const char *title, std::basic_ostream<char, std::char_traits<char>> &ostream) {
    std::stringstream ss;
    ss << ostream.rdbuf();
    if (popup == NULL) {
        popup = new Popup(title, ss.str().c_str());
    } else {
        popup->messages.insert(ss.str().c_str());
    }
}
