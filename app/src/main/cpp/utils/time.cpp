#include <main.h>

long get_act_millis() {
//    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
//    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
//    auto epoch = now_ms.time_since_epoch();
//    auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
//    long long duration = value.count();
//    return duration;
    return SDL_GetTicks();
}