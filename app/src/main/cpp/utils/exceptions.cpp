#include "main.h"

void throw_by_stream(std::basic_ostream<char, std::char_traits<char>> &ostream) {
    std::stringstream ss;
    ss << ostream.rdbuf();
    throw (ss);
}