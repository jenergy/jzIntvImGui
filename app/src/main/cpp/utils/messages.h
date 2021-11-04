#ifndef JZINTVIMGUI_MESSAGES_H
#define JZINTVIMGUI_MESSAGES_H
#define INFOS_INDEX 1
#define WARNINGS_INDEX 2
#define ERRORS_INDEX 3
#define CONFIG_WARNINGS_INDEX 4

#define ADD_CONFIG_WARNING(y) {std::stringstream message_variable; add_message_by_stream(message_variable << y, CONFIG_WARNINGS_INDEX);}
#define ADD_INFO(y)  {std::stringstream message_variable; add_message_by_stream(message_variable << y, INFOS_INDEX);}
#define ADD_WARNING(y)  {std::stringstream message_variable; add_message_by_stream(message_variable << y, WARNINGS_INDEX);}
#define ADD_ERROR(y)  {std::stringstream message_variable; add_message_by_stream(message_variable << y, ERRORS_INDEX);}

#endif //JZINTVIMGUI_MESSAGES_H
