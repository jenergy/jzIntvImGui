// Thanks to https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring

#include "main.h"

// trim from start (in place)
void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s) {
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s) {
    rtrim(s);
    return s;
}

char *replaceWord(const char *s, const char *oldW, const char *newW) {
    char *result;
    int i, cnt = 0;
    int newWlen = strlen(newW);
    int oldWlen = strlen(oldW);

    // Counting the number of times old word
    // occur in the string
    for (i = 0; s[i] != '\0'; i++) {
        if (strstr(&s[i], oldW) == &s[i]) {
            cnt++;

            // Jumping to index after the old word.
            i += oldWlen - 1;
        }
    }

    // Making new string of enough length
    result = (char *) malloc(i + cnt * (newWlen - oldWlen) + 1);

    i = 0;
    while (*s) {
        // compare the substring with the result
        if (strstr(s, oldW) == s) {
            strcpy(&result[i], newW);
            i += newWlen;
            s += oldWlen;
        } else
            result[i++] = *s++;
    }

    result[i] = '\0';
    return result;
}

char *findWord(const char *s, const char *oldW) {
    int i = 0;
    for (i = 0; s[i] != '\0'; i++) {
        if (strstr(&s[i], oldW) == &s[i]) {
            return const_cast<char *>(&s[i]);
        }
    }
    return NULL;
}

bool startsWith(const char *a, const char *b) {
    if (strncmp(a, b, strlen(b)) == 0) return 1;
    return 0;
}

static int imgui_size(const char *str) {
    const ImVec2 label_size = ImGui::CalcTextSize(str, NULL, true);
    return label_size.x;
}

std::vector<std::string> split(const char *str, const char *delimiter, bool accept_empty) {
    using namespace std;
    vector<string> res;
    size_t pos = 0;
    string token;
    string s = str;
    if (strcmp("", delimiter)) {
        while ((pos = s.find(delimiter)) != std::string::npos) {
            token = s.substr(0, pos);
            trim(token);
            if (!token.empty() || accept_empty) {
                res.push_back(token);
            }
            s.erase(0, pos + strlen(delimiter));
        }
        if (!s.empty() || accept_empty) {
            res.push_back(s);
        }
    } else {
        pos = 1;
        while (!s.empty()) {
            token = s.substr(0, pos);
            if (!token.empty()) {
                if ((strcmp(" ", token.c_str()) || (res.size() > 0 && strcmp(" ", res.at(res.size() - 1).c_str())))) {
                    res.push_back(token);
                }
            }
            s.erase(0, pos);
        }
    }

    return res;
}

vector<std::string> get_vector_string(string str, int size) {
    std::string qq = std::string(str);
    trim(qq);

    vector<std::string> res;

    if (imgui_size(qq.c_str()) < size) {
        res.push_back(qq);
    } else {
        vector<std::string> delimiters;
        delimiters.push_back(" ");
        delimiters.push_back("");

        bool try_no_space;
        while (!qq.empty()) {
            for (int i = 0; i < 2; i++) {
                const char *delimiter = delimiters[i].c_str();
                char *dup = strdup(qq.c_str());
                vector<std::string> vec = split(dup, delimiter, false);
                free(dup);
                string new_string = "";
                int max = vec.size();
                while (max > 0) {
                    new_string.clear();
                    for (int i = 0; i < max; i++) {
                        new_string.append(vec.at(i)).append(delimiter);
                    }
                    trim(new_string);
                    bool sizeok = imgui_size(new_string.c_str()) <= size;
                    if (sizeok || max == 1) {
                        try_no_space = !sizeok && i == 0;
                        if (!try_no_space) {
                            res.push_back(new_string);
                            string next_string = "";
                            for (int i = max; i < vec.size(); i++) {
                                next_string.append(vec.at(i)).append(delimiter);
                            }
                            trim(next_string);
                            qq = next_string;
                            dup = strdup(qq.c_str());
                            vec = split(dup, delimiter, false);
                            free(dup);
                            max = vec.size();
                        } else {
                            break;
                        }
                    } else {
                        max--;
                    }
                }
                if (!try_no_space) {
                    break;
                }
            }
        }
    }
    return res;
}

char *wrap_string_by_size(char *str, int size) {
    std::string qq = std::string(str);
    trim(qq);

    if (imgui_size(qq.c_str()) < size) {
        return strdup(qq.c_str());
    }
    using namespace std;
    const char *delimit1 = " ";
    const char *delimit2 = "";

    for (int j = 0; j < 2; j++) {
        const vector<std::string> &vec = split(str, j == 0 ? delimit1 : delimit2, false);
        string new_string = "";
        int max = vec.size();
        while (max > 0) {
            new_string.clear();
            for (int i = 0; i < max; i++) {
                new_string.append(vec.at(i)).append(j == 0 ? delimit1 : delimit2);
            }
            trim(new_string);
            if (imgui_size(new_string.c_str()) <= size) {
                string next_string = "";
                for (int i = max; i < vec.size(); i++) {
                    next_string.append(vec.at(i)).append(j == 0 ? delimit1 : delimit2);
                }
                trim(next_string);
                const char *resu = new_string.append("\n").append(next_string).c_str();
                return strdup(resu);
            } else {
                max--;
            }
        }
    }
    return NULL;
}

int strlen_trim(char *str) {
    if (str == NULL) {
        return 0;
    }
    std::string s(str);
    trim(s);
    int res = strlen(s.c_str());
    return res;
}

float string_to_float(char *val) {
    char *p = NULL;
    char *q = val;

    if (val == NULL) {
        THROW_BY_STREAM("Error converting to float (string_to_float): empty value");
    }

    errno = 0;
    float res = strtof(q, &p);
    if (errno != 0 || *p != '\0') {
        THROW_BY_STREAM("Error converting to float (string_to_float): '" << val << "'");
    }

    int len = strlen_trim(val);
    if (len == 0) {
        THROW_BY_STREAM("Error converting to float (string_to_float): empty value");
    }

    if (res < 0) {
        THROW_BY_STREAM("Error converting to float (string_to_float): negative value");
    }
    return res;
}

uint32_t string_to_int(char *val) {
    char *p = NULL;
    char *q = val;

    if (val == NULL) {
        THROW_BY_STREAM("Error converting to int (string_to_int): empty value");
    }

    if (*q == '-') {
        THROW_BY_STREAM("Error converting to int (string_to_int): negative value");
    }
    uint32_t res = strtoul(q, &p, 10);
    if (*p != 0) {
        THROW_BY_STREAM("Error converting to int (string_to_int): '" << val << "'");
    }
    int len = strlen_trim(val);
    if (len == 0) {
        THROW_BY_STREAM("Error converting to int (string_to_int): empty value");
    }
    return res;
}

bool string_to_bool(char *val) {
    char *p = NULL;
    char *q = val;

    if (val == NULL) {
        THROW_BY_STREAM("Error converting to bool (string_to_bool): empty value");
    }
    uint32_t res = strtoul(q, &p, 10);
    if (*p != 0) {
        THROW_BY_STREAM("Error converting to bool (string_to_bool): '" << val << "'");
    }
    int len = strlen_trim(val);
    if (len == 0) {
        THROW_BY_STREAM("Error converting to bool (string_to_bool): empty value");
    }

    if (res != 0 && res != 1) {
        THROW_BY_STREAM("Error converting to bool (string_to_bool): only 1 and 0 are accepted");
    }
    return res == 1;
}

uint32_t hex_to_uint32(char *val) {
    int len = strlen_trim(val);
    if (len == 0) {
        THROW_BY_STREAM("Error converting to int (hex_to_uint32): empty value");
    }

    if (strncasecmp(val, "0x", 2)) {
        THROW_BY_STREAM("Error converting to uint_32: '" << val << "'");
    }

    char *p = NULL;
    char *q = val + 2;
    strtoul(q, &p, 16);
    if (val == NULL) {
        THROW_BY_STREAM("Error converting to int (hex_to_uint32): empty value");
    }
    if (*p != 0) {
        THROW_BY_STREAM("Error converting to int (hex_to_uint32): '" << val << "'");
    }

    uint32_t res;
    std::stringstream ssh;
    ssh << std::hex << val;
    ssh >> res;
    return res;
}

