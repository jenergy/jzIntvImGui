#include "main.h"

bool is_memory_empty(const char *ptr) {
    return ptr == NULL || !strcmp(ptr, "");
}

bool is_memory_blank(const char *ptr) {
    string str = ptr == NULL ? "" : ptr;
    trim(str);
    return str.empty();
}
