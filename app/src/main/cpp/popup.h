#include <string>
#include <set>

#define ADD_POPUP(x,y) {std::stringstream popup_variable; add_popup_by_stream(x, popup_variable << y);}

using namespace std;
class Popup {
public:
    Popup(const char* title, const char* message);

    ~Popup();
    char* title;
    set<string> messages;
};
