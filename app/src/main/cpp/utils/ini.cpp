#include "main.h"

char properties_file_name[FILENAME_MAX];

static string format_string(string res) {
    trim(res);
    if (!res.empty()) {
        res.append("\n");
    }
    return res;
}

// char*
static bool condition_char_pointer(void **val) {
    char **casted = (char **) (*val);
    char *string_val = *casted;
    return !is_memory_empty(string_val);
}

static string print_char_pointer(const char *key, void **val) {
    string key_ = key;
    char **casted = (char **) (*val);
    char *val_ = *casted;
    ostringstream s;
    s << key_ << " = " << val_;
    return s.str();
}

static string print_char_pointer_replace_new_line(const char *key, void **val) {
    string key_ = key;
    char **casted = (char **) (*val);
    char *val_ = *casted;
    ostringstream s;
    s << key_ << " = " << val_;
    string res = s.str();
    char *str_tmp = strdup(s.str().c_str());
    const vector<std::string> &vec = split(str_tmp, "\n", true);
    free(str_tmp);
    string final_res;
    for (int i = 0; i < vec.size(); i++) {
        final_res.append(vec[i]);
        if (i < vec.size() - 1) {
            final_res.append("\\n");
        }
    }

    return final_res;
}

// uint64_t
static bool condition_uint64(void **val) {
    uint64_t *casted = reinterpret_cast<uint64_t *>((uint64_t **) (*val));
    uint64_t val_ = reinterpret_cast<uint64_t>(*casted);
    return val_ > 0;
}

static string print_uint64_hex(const char *key, void **val) {
    string key_ = key;
    uint64_t *casted = reinterpret_cast<uint64_t *>((uint64_t **) (*val));
    uint64_t val_ = reinterpret_cast<uint64_t>(*casted);
    ostringstream s;
    s << key_ << " = 0x" << std::hex << std::uppercase << val_;
    return s.str();
}

static string print_uint64(const char *key, void **val) {
    string key_ = key;
    uint64_t *casted = reinterpret_cast<uint64_t *>((uint64_t **) (*val));
    uint64_t val_ = reinterpret_cast<uint64_t>(*casted);
    ostringstream s;
    s << key_ << " = " << val_;
    return s.str();
}

// bool
static bool condition_bool(void **val) {
    return true;
}

static bool condition_bool_dummy(void **val) {
    return false;
}

static string print_bool(const char *key, void **val) {
    string key_ = key;
    int *casted = reinterpret_cast<int *>((int **) (*val));
    int val_ = reinterpret_cast<int>(*casted);
    int bool_val = val_ & 1;
    if (bool_val != 0) {
        bool_val = 1;
    }
    ostringstream s;
    s << key_ << " = " << bool_val;
    return s.str();
}

// int64_t
static bool condition_int64(void **val) {
    int64_t *casted = reinterpret_cast<int64_t *>((int64_t **) (*val));
    int64_t val_ = reinterpret_cast<int64_t>(*casted);
    return val_ >= 0;
}

static string print_int64(const char *key, void **val) {
    string key_ = key;
    int64_t *casted = reinterpret_cast<int64_t *>((int64_t **) (*val));
    int64_t val_ = reinterpret_cast<int64_t>(*casted);
    ostringstream s;
    s << key_ << " = " << val_;
    return s.str();
}

// double
static bool condition_double(void **val) {
    double *casted = reinterpret_cast<double *>((double **) (*val));
    double val_ = (*casted);
    return val_ >= 0;
}

static string print_double(const char *key, void **val) {
    string key_ = key;
    double *casted = reinterpret_cast<double *>((double **) (*val));
    double val_ = (*casted);
    ostringstream s;
    s << key_ << " = " << val_;
    return s.str();
}

// SDL_FRect
static bool condition_sdl_frect(void **val) {
    return true;
}

static string print_sdl_frect(const char *key, void **val) {
    string res;
    string key_ = key;
    SDL_FRect *casted = reinterpret_cast<SDL_FRect *>((SDL_FRect **) (*val));
    SDL_FRect val_ = (*casted);
    float *f = reinterpret_cast<float *>(&val_);
    const char *params[] = {"_x_", "_y_", "_w_", "_h_"};
    int siz = sizeof(params) / sizeof(char *);
    for (int i = 0; i < siz; i++) {
        ostringstream s;
        char *act_key = replaceWord(key, "_s_", params[i]);
        float _val = *(f + i);
        if (_val != -1) {
            s << act_key << " = " << *(f + i) << "\n";
            res.append(s.str());
        }
        free(act_key);
    }
    trim(res);
    res.append("\n");
    return res;
}

// string
static bool condition_string(void **val) {
    string *casted = reinterpret_cast<string *>((string **) (*val));
    string val_ = (*casted);
    trim(val_);
    return !val_.empty();
}

static string print_string(const char *key, void **val) {
    string key_ = key;
    string *casted = reinterpret_cast<string *>((string **) (*val));
    string val_ = (*casted);
    trim(val_);
    ostringstream s;
    s << key_ << " = " << val_;
    return s.str();
}

// vector*
static bool condition_vector_pointer(void **val) {
    vector<void *> **casted = (vector<void *> **) (*val);
    vector<void *> *vec = *casted;
    int size = vec->size();
    return size > 0;
}

static string print_vector_pointer(const char *key, void **val) {
    string key_ = key;
    vector<void *> **casted = (vector<void *> **) (*val);
    vector<void *> *vec = *casted;
    return "to implement";
}

static bool condition_vector_control_pointer(void **val) {
    vector<Control *> *casted2 = reinterpret_cast<vector<Control *> * >((vector<Control *> **) (*val));
    vector<Control *> lef = casted2[0];
    vector<Control *> right = casted2[1];
    return lef.size() > 0 || right.size() > 0;
}

static string save_controls_section(vector<Control *> *controls_ptr, bool override);

static string print_vector_control_pointer(const char *key, void **val) {
    string key_ = key;
    vector<Control *> *casted2 = reinterpret_cast<vector<Control *> * >((vector<Control *> **) (*val));
    return save_controls_section(reinterpret_cast<vector<struct Control *> *>(casted2), true);
}

static string print_vector_string_pointer(const char *key, void **val) {
    string key_ = key;
    vector<string> **casted = (vector<string> **) (*val);
    vector<string> *vec_ptr = *casted;
    vector<string> vec = *vec_ptr;
    string res;
    for (int i = 0; i < vec.size(); i++) {
        if (!(vec[i].empty())) {
            char buf[10];
            sprintf(buf, "%d", (i + 1));
            res.append(key).append("_").append(buf).append(" = ").append(vec[i]).append("\n");
        }
    }
    return res;
}

static int get_size_of_member(int type) {
    switch (type) {
        case CHAR_POINTER_T:
            return sizeof(char *);
        case UINT_64_T:
            return sizeof(uint64_t);
        case BOOL_T:
            return sizeof(bool);
        case INT_64_T:
            return sizeof(int64_t);
        case DOUBLE_T:
            return sizeof(double);
        case SDL_FRECT_T:
            return sizeof(SDL_FRect);
        case STRING_T:
            return sizeof(string);
        case VECTOR_POINTER_T:
            return sizeof(vector<void *> *);
    }
    return -1;
}

static bool (*get_condition_func_default(int type))(void **) {
    switch (type) {
        case CHAR_POINTER_T:
            return condition_char_pointer;
        case UINT_64_T:
            return condition_uint64;
        case BOOL_T:
            return condition_bool;
        case INT_64_T:
            return condition_int64;
        case DOUBLE_T:
            return condition_double;
        case SDL_FRECT_T:
            return condition_sdl_frect;
        case STRING_T:
            return condition_string;
        case VECTOR_POINTER_T:
            return condition_vector_pointer;
    }
    return NULL;
}

static string (*get_print_func_default(int type))(const char *, void **) {
    switch (type) {
        case CHAR_POINTER_T:
            return print_char_pointer;
        case UINT_64_T:
            return print_uint64;
        case BOOL_T:
            return print_bool;
        case INT_64_T:
            return print_int64;
        case DOUBLE_T:
            return print_double;
        case SDL_FRECT_T:
            return print_sdl_frect;
        case STRING_T:
            return print_string;
        case VECTOR_POINTER_T:
            return print_vector_pointer;
    }
    return NULL;
}

static ini_option_t ini_options_general[] =
        {
                {EXEC_BIN_CRC_32_OPTION,                        UINT_64_T,        NULL, print_uint64_hex},
                {GROM_BIN_CRC_32_OPTION,                        UINT_64_T,        NULL, print_uint64_hex},
                {TUTORVISION_EXEC_BIN_CRC_32_OPTION,            UINT_64_T,        NULL, print_uint64_hex},
                {TUTORVISION_GROM_BIN_CRC_32_OPTION,            UINT_64_T,        NULL, print_uint64_hex},
                {ECS_BIN_CRC_32_OPTION,                         UINT_64_T,        NULL, print_uint64_hex},
                {WINDOW_WIDTH_OPTION,                           UINT_64_T,        NULL, NULL},
                {WINDOW_HEIGHT_OPTION,                          UINT_64_T,        NULL, NULL},
                {BUTTONS_SIZE_OPTION,                           UINT_64_T,        NULL, NULL},
                {SCROLLBAR_SIZE_OPTION,                         UINT_64_T,        NULL, NULL},
                {LAST_CRC_32_OPTION,                            UINT_64_T,        NULL, print_uint64_hex},
                {NUM_ROMS_JUMP_OPTION,                          UINT_64_T,        NULL, NULL},
                {FONT_SIZE_OPTION,                              UINT_64_T,        NULL, NULL},
                {STYLE_INDEX_OPTION,                            UINT_64_T,        NULL, NULL},
                {JZINTV_RESOLUTION_INDEX_OPTION,                UINT_64_T,        NULL, NULL},
                {MOBILE_DEFAULT_PORTRAIT_CONTROLS_SIZE_OPTION,  UINT_64_T,        NULL, NULL},
                {MOBILE_DEFAULT_LANDSCAPE_CONTROLS_SIZE_OPTION, UINT_64_T,        NULL, NULL},
                {MOBILE_ECS_PORTRAIT_ALPHA_OPTION,              UINT_64_T,        NULL, NULL},
                {MOBILE_ECS_LANDSCAPE_ALPHA_OPTION,             UINT_64_T,        NULL, NULL},
                {HIDE_UNAVAILABLE_ROMS_OPTION,                  BOOL_T,           NULL, NULL},
                {WINDOW_MAXIMIZED_OPTION,                       BOOL_T,           NULL, NULL},
                {JZINTV_FULLSCREEN_OPTION,                      BOOL_T,           NULL, NULL},
                {MOBILE_SHOW_CONTROLS_OPTION,                   BOOL_T,           NULL, NULL},
                {MOBILE_SHOW_CONFIGURATION_CONTROLS_OPTION,     BOOL_T,           NULL, NULL},
                {MOBILE_USE_INVERTED_CONTROLS_OPTION,           BOOL_T,           NULL, NULL},
                {"DUMMY",                                       BOOL_T,           condition_bool_dummy, NULL},
                {"DUMMY",                                       BOOL_T,           condition_bool_dummy, NULL},
                {ROMS_LIST_WIDTH_PERCENTAGE_OPTION,             DOUBLE_T,         NULL, NULL},
                {IMAGE_HEIGHT_PERCENTAGE_OPTION,                DOUBLE_T,         NULL, NULL},
                {MOBILE_PORTRAIT_TOP_GAP_PERCENTAGE_OPTION,     DOUBLE_T,         NULL, NULL},
                {MOBILE_PORTRAIT_BOTTOM_GAP_PERCENTAGE_OPTION,  DOUBLE_T,         NULL, NULL},
                {MOBILE_LANDSCAPE_LEFT_GAP_PERCENTAGE_OPTION,   DOUBLE_T,         NULL, NULL},
                {MOBILE_LANDSCAPE_RIGHT_GAP_PERCENTAGE_OPTION,  DOUBLE_T,         NULL, NULL},
                {CUSTOM_COMMAND_OPTION,                         VECTOR_POINTER_T, NULL, print_vector_string_pointer},
                {ROMS_FOLDER_OPTION,                            CHAR_POINTER_T,   NULL, NULL},
                {KEYBOARD_HACK_FILE_OPTION,                     CHAR_POINTER_T,   NULL, NULL},
                {PALETTE_FILE_OPTION,                           CHAR_POINTER_T,   NULL, NULL},
                {FONT_FILENAME_OPTION,                          CHAR_POINTER_T,   NULL, NULL},
                {MOBILE_SCREEN_PORTRAIT_S_PERC_OPTION,          SDL_FRECT_T,      NULL, NULL},
                {MOBILE_SCREEN_LANDSCAPE_S_PERC_OPTION,         SDL_FRECT_T,      NULL, NULL},
        };

static ini_option_t ini_options_controls[] =
        {
                {CONTROL_PORTRAIT_S_PERC_OPTION,    SDL_FRECT_T, NULL, NULL},
                {CONTROL_LANDSCAPE_S_PERC_OPTION,   SDL_FRECT_T, NULL, NULL},
                {CONTROL_VISIBLE_OPTION,            INT_64_T,    NULL, NULL},
                {CONTROL_ALPHA_PORTRAIT_OPTION,     INT_64_T,    NULL, NULL},
                {CONTROL_ALPHA_LANDSCAPE_OPTION,    INT_64_T,    NULL, NULL},
                {CONTROL_FILE_NAME_RELEASED_OPTION, STRING_T,    NULL, NULL},
                {CONTROL_FILE_NAME_PRESSED_OPTION,  STRING_T,    NULL, NULL},
                {CONTROL_OVERRIDE_EVENT_OPTION,     STRING_T,    NULL, NULL},
        };

static ini_option_t ini_options_games[] =
        {
                {GAME_NAME_OPTION,                      CHAR_POINTER_T,   NULL,                 NULL},
                {DESCRIPTION_OPTION,                    CHAR_POINTER_T,   NULL,                  print_char_pointer_replace_new_line},
                {IMAGE_FILE_NAME_OPTION,                CHAR_POINTER_T,   NULL,                 NULL},
                {BOX_FILE_NAME_OPTION,                  CHAR_POINTER_T,   NULL,                 NULL},
                {KEYBOARD_HACK_FILE_OPTION,             CHAR_POINTER_T,   NULL,                 NULL},
                {PALETTE_FILE_OPTION,                   CHAR_POINTER_T,   NULL,                 NULL},
                {USE_TUTORVISION_EXEC_OPTION,           BOOL_T,           NULL,                 NULL},
                {USE_TUTORVISION_GROM_OPTION,           BOOL_T,           NULL,                 NULL},
                {USE_TUTORVISION_GRAM_OPTION,           BOOL_T,           NULL,                 NULL},
                {ECS_TAPE_NAME_AUTO_OPTION,             BOOL_T,           NULL,                 NULL},
                {JLP_SAVE_FILE_AUTO_OPTION,             BOOL_T,           NULL,                 NULL},
                {"DUMMY",                               BOOL_T,           condition_bool_dummy, NULL},
                {"DUMMY",                               BOOL_T,           condition_bool_dummy, NULL},
                {"DUMMY",                               BOOL_T,           condition_bool_dummy, NULL},
                {CUSTOM_COMMAND_OPTION,                 VECTOR_POINTER_T, NULL,                             print_vector_string_pointer},
                {MOBILE_SCREEN_PORTRAIT_S_PERC_OPTION,  SDL_FRECT_T,      NULL,                 NULL},
                {MOBILE_SCREEN_LANDSCAPE_S_PERC_OPTION, SDL_FRECT_T,      NULL,                 NULL},
                {GAME_CONTROLS_OPTION,                  VECTOR_POINTER_T, condition_vector_control_pointer, print_vector_control_pointer},
        };

static void save_section(void *p, int size, ini_option_t *options, ostringstream &resul, string *suffix) {
    int last_type = CHAR_POINTER_T;
    for (int i = 0; i < size; i++) {
        ini_option_t opt = options[i];
        if (last_type == BOOL_T && opt.type != BOOL_T) {
            char tmp[100];
            sprintf(tmp, "%d", p);
            int val_of_pointer = atoi(tmp);
            while (val_of_pointer % 4 != 0) {
                p = static_cast<char *>(p) + 1;
                sprintf(tmp, "%d", p);
                val_of_pointer = atoi(tmp);
            }
        }
        bool (*condition_func)(void **) = opt.condition_func;
        if (condition_func == NULL) {
            condition_func = get_condition_func_default(opt.type);
        }
        if (condition_func(&p)) {
            string (*print_func)(const char *, void **) = opt.print_func;
            if (print_func == NULL) {
                print_func = get_print_func_default(opt.type);
            }

            string real_key = opt.key;

            if (suffix != NULL && (!(*suffix).empty())) {
                real_key.append(*suffix);
            }
            bool is_for_mobile = real_key.rfind("mobile", 0) == 0;
            if (!is_for_mobile || app_config_struct.mobile_mode) {
                string res = print_func(real_key.c_str(), &p);
                resul << format_string(res);
            }
        }

        p = static_cast<char *>(p) + get_size_of_member(opt.type);
        last_type = opt.type;
    }
}

static string save_general_section() {
    ostringstream resul;
    resul << "[General]\n";
    void *p = &app_config_struct;

    int size = sizeof(ini_options_general) / sizeof(ini_option_t);
    save_section(p, size, ini_options_general, resul, NULL);
    return format_string(resul.str());
}

static string save_controls_section(vector<Control *> *controlz, bool override) {
    ostringstream resul_main;
    if (!override) {
        resul_main << "[Controls]";
    }
    for (int hand_index = 0; hand_index < 2; hand_index++) {
        vector<Control *> *container = &(controlz[hand_index]);
        for (int k = 0; k < container->size(); k++) {
            void *p = container->at(k);

            string control_key = ((Control *) p)->original_event;
            control_key = control_key.append((hand_index == 0 ? "__L" : "__R"));

            if (!override) {
                resul_main << "\n[" << control_key << "]\n";
            }
            int size = sizeof(ini_options_controls) / sizeof(ini_option_t);
            string key_suffix = "_";
            key_suffix.append(control_key);
            ostringstream resul2;
            save_section(p, size, ini_options_controls, resul2, override ? &key_suffix : NULL);
            resul2 << "\n";

            string genen = resul2.str();
            const char *ggg = genen.c_str();
            resul_main << format_string(genen);
        }
    }
    return format_string(resul_main.str());
}

static string save_games_section() {
    ostringstream resul;
    resul << "[Games]\n";
    for (int k = 0; k < app_config_struct.num_valid_crc32s; k++) {
        rom_config_struct_t &roms_config = roms_configuration[k];
        if (k > 0) {
            resul << "\n";
        }
        resul << "[0x" << std::hex << std::uppercase << roms_config.crc32 << "]\n";
        void *p = &(roms_configuration[k]);
        int size = sizeof(ini_options_games) / sizeof(ini_option_t);
        save_section(p, size, ini_options_games, resul, NULL);
    }
    return format_string(resul.str());
}

static void write_file_ini(std::string ini_content);

void save_config_file() {
    string cnt = "";
//    if (app_config_struct.mobile_mode) {
        normalize_screen_to_delta();
        vector<Control *> new_delta_default_controls[2];
        normalize_controls_to_delta(new_delta_default_controls, LEFT_HAND_INDEX);
        normalize_controls_to_delta(new_delta_default_controls, RIGHT_HAND_INDEX);
        cnt = save_controls_section(new_delta_default_controls, false);
        clear_controls(&new_delta_default_controls[LEFT_HAND_INDEX]);
        clear_controls(&new_delta_default_controls[RIGHT_HAND_INDEX]);

        // This will force the call to init_jzintv_rendering_rect to reconstruct_from_delta
        init_jzintv_screen_references();
//    }
    string gen = save_general_section();
    string games = save_games_section();
    string ini_cont = gen.append("\n").append(cnt).append("\n").append(games);
    write_file_ini(ini_cont);
}

static bool is_section_ini(char *line) {
    if (line == NULL) {
        THROW_BY_STREAM("Empty line");
    }
    std::string s(line);
    trim(s);
    return s[0] == '[' && s[strlen(s.c_str()) - 1] == ']';
}

static void get_section_from_ini(char *line, char *buf) {
    if (line == NULL) {
        THROW_BY_STREAM("Empty line");
    }
    char *p = findWord(line, "[");
    if (p != NULL) {
        char *q = findWord(p, "]");
        if (q != NULL) {
            int siz = q - p - 1;
            if (siz > 0) {
                char *tmp = static_cast<char *>(malloc((siz + 1) * sizeof(char)));
                memcpy(tmp, p + 1, siz);
                tmp[siz] = 0;
                std::string s(tmp);
                trim(s);
                if (strlen(s.c_str()) == 0) {
                    buf[0] = 0;
                } else {
                    strcpy(buf, s.c_str());
                }
                free(tmp);
            } else {
                THROW_BY_STREAM("Parenthesis error!");
            }
        }
    } else {
        THROW_BY_STREAM("Wrong section!");
    }
}

static void get_key_from_ini(char *line, char *buf) {
    if (line == NULL) {
        THROW_BY_STREAM("Empty line");
    }
    char *p = findWord(line, "=");
    if (p != NULL) {
        int siz = p - line;
        char *tmp = static_cast<char *>(malloc((siz + 1) * sizeof(char)));
        memcpy(tmp, line, siz);
        tmp[siz] = 0;
        std::string s(tmp);
        trim(s);
        if (strlen(s.c_str()) == 0) {
            buf[0] = 0;
        } else {
            strcpy(buf, s.c_str());
        }
        free(tmp);
    } else {
        THROW_BY_STREAM("Wrong key!");
    }
}

static void get_value_from_ini(char *line, char *buf) {
    if (line == NULL) {
        THROW_BY_STREAM("Empty line");
    }
    char *p = findWord(line, "=");
    if (p != NULL) {
        int siz = strlen(line) - (p - line) - 1;
        char *tmp = static_cast<char *>(malloc((siz + 1) * sizeof(char)));
        memcpy(tmp, p + 1, siz);
        tmp[siz] = 0;
        std::string s(tmp);
        trim(s);
        if (strlen(s.c_str()) == 0) {
            buf[0] = 0;
        } else {
            strcpy(buf, s.c_str());
        }
        free(tmp);
    } else {
        THROW_BY_STREAM("Wrong value!");
    }
}

static char *read_file_ini(char *ini_file) {
    std::string output;
    std::string myText;

    std::ifstream MyReadFile(ini_file);

    while (getline(MyReadFile, myText)) {
        output.append(myText).append("\n");
    }

    MyReadFile.close();
    return strdup(output.c_str());
}

static int get_custom_command_index(char *key) {
    char subbuff[5];
    int start_offs = strlen(CUSTOM_COMMAND_OPTION) + 1;
    int num_chars = strlen(key) - start_offs;
    memcpy(subbuff, &key[start_offs], num_chars);
    subbuff[num_chars] = 0;
    int res = -1;
    bool error = false;
    try {
        res = string_to_int(subbuff) - 1;
        if (res >= MAX_CUSTOM_COMMANDS) {
            error = true;
        }
    }
    catch (...) {
        error = true;
    }
    if (error) {
        THROW_BY_STREAM("Error, custom command index must be between 1 and " << MAX_CUSTOM_COMMANDS << ": " << key);
        res = -1;
    }
    return res;
}

static void add_custom_command(vector<string> *cc_vec_ptr, char *key, char *command) {
    int index = get_custom_command_index(key);
    vector<string> &cc_vec = *cc_vec_ptr;
    while (cc_vec.size() < index + 1) {
        cc_vec.push_back("");
    }
    if (strlen(command) > FILENAME_MAX) {
        command[FILENAME_MAX] = 0;
    }
    cc_vec[index] = command;
}

static bool set_general_value(char *key, char *value, struct app_config_struct_t *app_conf) {
    bool res = true;
    if (!strcmp(key, ROMS_FOLDER_OPTION)) {
        free(app_conf->roms_folder_ini);
        app_conf->roms_folder_ini = strdup(value);
        if (strlen(app_conf->roms_folder_ini) > FILENAME_MAX) {
            app_conf->roms_folder_ini[FILENAME_MAX] = 0;
        }
    } else if (!strcmp(key, KEYBOARD_HACK_FILE_OPTION)) {
        free(app_conf->keyboard_hack_file);
        app_conf->keyboard_hack_file = normalize_path(value, false);
        if (strlen(app_conf->keyboard_hack_file) > FILENAME_MAX) {
            app_conf->keyboard_hack_file[FILENAME_MAX] = 0;
        }
    } else if (!strcmp(key, PALETTE_FILE_OPTION)) {
        free(app_conf->palette_file);
        app_conf->palette_file = normalize_path(value, false);
        if (strlen(app_conf->palette_file) > FILENAME_MAX) {
            app_conf->palette_file[FILENAME_MAX] = 0;
        }
    } else if (startsWith(key, CUSTOM_COMMAND_OPTION)) {
        add_custom_command(app_conf->custom_commands, key, value);
    } else if (!strcmp(key, EXEC_BIN_CRC_32_OPTION)) {
        app_conf->execBinCrc32 = hex_to_uint32(value);
    } else if (!strcmp(key, GROM_BIN_CRC_32_OPTION)) {
        app_conf->gromBinCrc32 = hex_to_uint32(value);
    } else if (!strcmp(key, TUTORVISION_EXEC_BIN_CRC_32_OPTION)) {
        app_conf->tutorvisionExecBinCrc32 = hex_to_uint32(value);
    } else if (!strcmp(key, TUTORVISION_GROM_BIN_CRC_32_OPTION)) {
        app_conf->tutorvisionGromBinCrc32 = hex_to_uint32(value);
    } else if (!strcmp(key, ECS_BIN_CRC_32_OPTION)) {
        app_conf->ecsBinCrc32 = hex_to_uint32(value);
    } else if (!strcmp(key, WINDOW_WIDTH_OPTION)) {
        app_conf->window_width = string_to_int(value);
    } else if (!strcmp(key, WINDOW_HEIGHT_OPTION)) {
        app_conf->window_height = string_to_int(value);
    } else if (!strcmp(key, BUTTONS_SIZE_OPTION)) {
        app_conf->buttons_size = string_to_int(value);
    } else if (!strcmp(key, SCROLLBAR_SIZE_OPTION)) {
        app_conf->scrollbar_size = string_to_int(value);
    } else if (!strcmp(key, ROMS_LIST_WIDTH_PERCENTAGE_OPTION)) {
        app_conf->roms_list_width_percentage = string_to_float(value);
    } else if (!strcmp(key, IMAGE_HEIGHT_PERCENTAGE_OPTION)) {
        app_conf->image_height_percentage = string_to_float(value);
    } else if (!strcmp(key, NUM_ROMS_JUMP_OPTION)) {
        app_conf->num_roms_jump = string_to_int(value);
    } else if (!strcmp(key, HIDE_UNAVAILABLE_ROMS_OPTION)) {
        app_conf->hide_unavailable_roms = string_to_bool(value);
    } else if (!strcmp(key, LAST_CRC_32_OPTION)) {
        app_conf->last_crc32 = hex_to_uint32(value);
    } else if (!strcmp(key, FONT_FILENAME_OPTION)) {
        free(app_conf->font_filename);
        app_conf->font_filename = normalize_path(value, false);
    } else if (!strcmp(key, FONT_SIZE_OPTION)) {
        app_conf->font_size = string_to_int(value);
        font_size = app_conf->font_size;
    } else if (!strcmp(key, STYLE_INDEX_OPTION)) {
        app_conf->style_index = string_to_int(value);
    } else if (!strcmp(key, JZINTV_RESOLUTION_INDEX_OPTION)) {
        app_conf->jzintv_resolution_index = string_to_int(value);
    } else if (!strcmp(key, WINDOW_MAXIMIZED_OPTION)) {
        app_conf->window_maximized = string_to_bool(value);
    } else if (!strcmp(key, MOBILE_PORTRAIT_TOP_GAP_PERCENTAGE_OPTION)) {
        app_conf->mobile_portrait_top_gap_percentage = string_to_float(value);
    } else if (!strcmp(key, MOBILE_PORTRAIT_BOTTOM_GAP_PERCENTAGE_OPTION)) {
        app_conf->mobile_portrait_bottom_gap_percentage = string_to_float(value);
    } else if (!strcmp(key, MOBILE_LANDSCAPE_LEFT_GAP_PERCENTAGE_OPTION)) {
        app_conf->mobile_landscape_left_gap_percentage = string_to_float(value);
    } else if (!strcmp(key, MOBILE_LANDSCAPE_RIGHT_GAP_PERCENTAGE_OPTION)) {
        app_conf->mobile_landscape_right_gap_percentage = string_to_float(value);
    } else if (!strcmp(key, MOBILE_ECS_LANDSCAPE_ALPHA_OPTION)) {
        app_conf->mobile_ecs_landscape_alpha = string_to_int(value);
    } else if (!strcmp(key, MOBILE_ECS_PORTRAIT_ALPHA_OPTION)) {
        app_conf->mobile_ecs_portrait_alpha = string_to_int(value);
    } else if (!strcmp(key, MOBILE_SHOW_CONTROLS_OPTION)) {
        app_conf->mobile_show_controls = string_to_bool(value);
    } else if (!strcmp(key, MOBILE_SCREEN_LANDSCAPE_X_PERC_OPTION)) {
        app_conf->mobile_landscape_rect.x = string_to_float(value);
    } else if (!strcmp(key, MOBILE_SCREEN_LANDSCAPE_Y_PERC_OPTION)) {
        app_conf->mobile_landscape_rect.y = string_to_float(value);
    } else if (!strcmp(key, MOBILE_SCREEN_LANDSCAPE_W_PERC_OPTION)) {
        app_conf->mobile_landscape_rect.w = string_to_float(value);
    } else if (!strcmp(key, MOBILE_SCREEN_LANDSCAPE_H_PERC_OPTION)) {
        app_conf->mobile_landscape_rect.h = string_to_float(value);
    } else if (!strcmp(key, MOBILE_SCREEN_PORTRAIT_X_PERC_OPTION)) {
        app_conf->mobile_portrait_rect.x = string_to_float(value);
    } else if (!strcmp(key, MOBILE_SCREEN_PORTRAIT_Y_PERC_OPTION)) {
        app_conf->mobile_portrait_rect.y = string_to_float(value);
    } else if (!strcmp(key, MOBILE_SCREEN_PORTRAIT_W_PERC_OPTION)) {
        app_conf->mobile_portrait_rect.w = string_to_float(value);
    } else if (!strcmp(key, MOBILE_SCREEN_PORTRAIT_H_PERC_OPTION)) {
        app_conf->mobile_portrait_rect.h = string_to_float(value);
    } else if (!strcmp(key, MOBILE_SHOW_CONFIGURATION_CONTROLS_OPTION)) {
        app_conf->mobile_show_configuration_controls = string_to_bool(value);
    } else if (!strcmp(key, JZINTV_FULLSCREEN_OPTION)) {
        app_conf->jzintv_fullscreen = string_to_bool(value);
    } else if (!strcmp(key, MOBILE_USE_INVERTED_CONTROLS_OPTION)) {
        app_conf->mobile_use_inverted_controls = string_to_bool(value);
    } else if (!strcmp(key, MOBILE_DEFAULT_PORTRAIT_CONTROLS_SIZE_OPTION)) {
        app_conf->mobile_default_portrait_controls_size = string_to_int(value);
    } else if (!strcmp(key, MOBILE_DEFAULT_LANDSCAPE_CONTROLS_SIZE_OPTION)) {
        app_conf->mobile_default_landscape_controls_size = string_to_int(value);
    } else {
        res = false;
    }
    return res;
}

static bool set_game_value(char *key, char *value, struct rom_config_struct_t *act_rom_config) {
    bool res = true;
    if (!strcmp(key, GAME_NAME_OPTION)) {
        free(act_rom_config->game_name);
        act_rom_config->game_name = strdup(value);
        if (strlen(act_rom_config->game_name) > FILENAME_MAX) {
            act_rom_config->game_name[FILENAME_MAX] = 0;
        }
    } else if (!strcmp(key, DESCRIPTION_OPTION)) {
        char *tmp = replaceWord(value, "\\n", "\n");
        act_rom_config->description = tmp;
        if (strlen(act_rom_config->description) > DESCRIPTION_MAX_LENGTH) {
            act_rom_config->description[DESCRIPTION_MAX_LENGTH] = 0;
        }
    } else if (!strcmp(key, USE_TUTORVISION_EXEC_OPTION)) {
        act_rom_config->use_tutorvision_exec = string_to_bool(value);
    } else if (!strcmp(key, USE_TUTORVISION_GROM_OPTION)) {
        act_rom_config->use_tutorvision_grom = string_to_bool(value);
    } else if (!strcmp(key, USE_TUTORVISION_GRAM_OPTION)) {
        act_rom_config->use_tutorvision_gram = string_to_bool(value);
    } else if (!strcmp(key, ECS_TAPE_NAME_AUTO_OPTION)) {
        act_rom_config->ecs_tape_name_auto = string_to_bool(value);
    } else if (!strcmp(key, JLP_SAVE_FILE_AUTO_OPTION)) {
        act_rom_config->jlp_save_file_auto = string_to_bool(value);
    } else if (!strcmp(key, IMAGE_FILE_NAME_OPTION)) {
        act_rom_config->image_file_name = strdup(value);
        if (strlen(act_rom_config->image_file_name) > FILENAME_MAX) {
            act_rom_config->image_file_name[FILENAME_MAX] = 0;
        }
    } else if (!strcmp(key, BOX_FILE_NAME_OPTION)) {
        act_rom_config->box_file_name = strdup(value);
        if (strlen(act_rom_config->box_file_name) > FILENAME_MAX) {
            act_rom_config->box_file_name[FILENAME_MAX] = 0;
        }
    } else if (!strcmp(key, KEYBOARD_HACK_FILE_OPTION)) {
        free(act_rom_config->keyboard_hack_file);
        act_rom_config->keyboard_hack_file = strdup(value);
        if (strlen(act_rom_config->keyboard_hack_file) > FILENAME_MAX) {
            act_rom_config->keyboard_hack_file[FILENAME_MAX] = 0;
        }
    } else if (!strcmp(key, PALETTE_FILE_OPTION)) {
        free(act_rom_config->palette_file);
        act_rom_config->palette_file = strdup(value);
        if (strlen(act_rom_config->palette_file) > FILENAME_MAX) {
            act_rom_config->palette_file[FILENAME_MAX] = 0;
        }
    } else if (startsWith(key, CUSTOM_COMMAND_OPTION)) {
        if (act_rom_config->custom_commands == NULL) {
            act_rom_config->custom_commands = new vector<string>();
        }
        add_custom_command(act_rom_config->custom_commands, key, value);
    } else if (!strcmp(key, MOBILE_SCREEN_LANDSCAPE_X_PERC_OPTION)) {
        act_rom_config->mobile_landscape_rect.x = string_to_float(value);
    } else if (!strcmp(key, MOBILE_SCREEN_LANDSCAPE_Y_PERC_OPTION)) {
        act_rom_config->mobile_landscape_rect.y = string_to_float(value);
    } else if (!strcmp(key, MOBILE_SCREEN_LANDSCAPE_W_PERC_OPTION)) {
        act_rom_config->mobile_landscape_rect.w = string_to_float(value);
    } else if (!strcmp(key, MOBILE_SCREEN_LANDSCAPE_H_PERC_OPTION)) {
        act_rom_config->mobile_landscape_rect.h = string_to_float(value);
    } else if (!strcmp(key, MOBILE_SCREEN_PORTRAIT_X_PERC_OPTION)) {
        act_rom_config->mobile_portrait_rect.x = string_to_float(value);
    } else if (!strcmp(key, MOBILE_SCREEN_PORTRAIT_Y_PERC_OPTION)) {
        act_rom_config->mobile_portrait_rect.y = string_to_float(value);
    } else if (!strcmp(key, MOBILE_SCREEN_PORTRAIT_W_PERC_OPTION)) {
        act_rom_config->mobile_portrait_rect.w = string_to_float(value);
    } else if (!strcmp(key, MOBILE_SCREEN_PORTRAIT_H_PERC_OPTION)) {
        act_rom_config->mobile_portrait_rect.h = string_to_float(value);
    } else {
        res = false;
    }
    return res;
}

bool set_control_value(char *key, char *value, Control *act_control) {
    bool res = true;
    if (!strcmp(key, CONTROL_PORTRAIT_X_PERC_OPTION)) {
        act_control->portrait_frect.x = string_to_float(value);
    } else if (!strcmp(key, CONTROL_PORTRAIT_Y_PERC_OPTION)) {
        act_control->portrait_frect.y = string_to_float(value);
    } else if (!strcmp(key, CONTROL_PORTRAIT_W_PERC_OPTION)) {
        act_control->portrait_frect.w = string_to_float(value);
    } else if (!strcmp(key, CONTROL_PORTRAIT_H_PERC_OPTION)) {
        act_control->portrait_frect.h = string_to_float(value);
    } else if (!strcmp(key, CONTROL_LANDSCAPE_X_PERC_OPTION)) {
        act_control->landscape_frect.x = string_to_float(value);
    } else if (!strcmp(key, CONTROL_LANDSCAPE_Y_PERC_OPTION)) {
        act_control->landscape_frect.y = string_to_float(value);
    } else if (!strcmp(key, CONTROL_LANDSCAPE_W_PERC_OPTION)) {
        act_control->landscape_frect.w = string_to_float(value);
    } else if (!strcmp(key, CONTROL_LANDSCAPE_H_PERC_OPTION)) {
        act_control->landscape_frect.h = string_to_float(value);
    } else if (!strcmp(key, CONTROL_VISIBLE_OPTION)) {
        act_control->is_visible = string_to_int(value);
        if (act_control->is_visible != 1 && act_control->is_visible != 0 && act_control->is_visible != -1) {
            THROW_BY_STREAM("Error converting visible value:" << act_control->is_visible << " Forcing to -1");
            act_control->is_visible = -1;
        }
    } else if (!strcmp(key, CONTROL_ALPHA_PORTRAIT_OPTION)) {
        act_control->alpha_portrait = string_to_int(value);
    } else if (!strcmp(key, CONTROL_ALPHA_LANDSCAPE_OPTION)) {
        act_control->alpha_landscape = string_to_int(value);
    } else if (!strcmp(key, CONTROL_FILE_NAME_PRESSED_OPTION)) {
        act_control->file_name_pressed = value;
    } else if (!strcmp(key, CONTROL_FILE_NAME_RELEASED_OPTION)) {
        act_control->file_name_released = value;
    } else if (!strcmp(key, CONTROL_OVERRIDE_EVENT_OPTION)) {
        if (strlen(value) > 10) {
            return false;
        }
        act_control->override_event = value;
        add_event_to_monitor(value);
    } else {
        res = false;
    }
    return res;
}

rom_config_struct_t *add_new_game_by_crc32(uint32_t crc32) {
    rom_config_struct_t x;
    reset_rom_config(&x, crc32);
    roms_configuration.push_back(x);
    app_config_struct.num_total_crc32s++;
    rom_config_struct_t *res = &(roms_configuration[roms_configuration.size() - 1]);
    res->custom_commands = new vector<string>;
    return res;
}

static void fix_mobile_config() {
    if (!app_config_struct.mobile_mode) {
        app_config_struct.mobile_show_controls = false;
        app_config_struct.mobile_show_configuration_controls = false;
        app_config_struct.mobile_landscape_left_gap_percentage = 0;
        app_config_struct.mobile_landscape_right_gap_percentage = 0;
        app_config_struct.mobile_portrait_bottom_gap_percentage = 0;
        app_config_struct.mobile_portrait_top_gap_percentage = 0;
        app_config_struct.mobile_ecs_portrait_alpha = 0;
        app_config_struct.mobile_ecs_landscape_alpha = 0;
        app_config_struct.mobile_landscape_rect.x = -1;
        app_config_struct.mobile_landscape_rect.y = -1;
        app_config_struct.mobile_landscape_rect.w = -1;
        app_config_struct.mobile_landscape_rect.h = -1;
        app_config_struct.mobile_portrait_rect.x = -1;
        app_config_struct.mobile_portrait_rect.y = -1;
        app_config_struct.mobile_portrait_rect.w = -1;
        app_config_struct.mobile_portrait_rect.h = -1;
    } else {
        if (app_config_struct.font_size < 30) {
            app_config_struct.font_size = 30;
        }
        if (app_config_struct.scrollbar_size < 40) {
            app_config_struct.scrollbar_size = 40;
        }

        if (app_config_struct.buttons_size < 30) {
            app_config_struct.buttons_size = 30;
        }

        if (app_config_struct.mobile_default_portrait_controls_size > 4 || app_config_struct.mobile_default_portrait_controls_size < 0) {
            app_config_struct.mobile_default_portrait_controls_size = 4;
        }
        if (app_config_struct.mobile_default_landscape_controls_size > 4 || app_config_struct.mobile_default_landscape_controls_size < 0) {
            app_config_struct.mobile_default_landscape_controls_size = 2;
        }
    }
}

static void fix_incompatible_options() {
    if (app_config_struct.mobile_show_configuration_controls && !app_config_struct.mobile_show_controls) {
        app_config_struct.mobile_show_configuration_controls = false;
        ADD_POPUP("Wrong configuration", "Fixed wrong configuration for flags 'show controls' and 'show configuration controls'");
    }
    if (app_config_struct.roms_list_width_percentage > 80 || app_config_struct.roms_list_width_percentage < 20) {
        app_config_struct.roms_list_width_percentage = 20;
        ADD_POPUP("Wrong configuration", "Fixed wrong configuration for flag 'roms_list_width_percentage'");
    }

    if (app_config_struct.image_height_percentage > 80 || app_config_struct.image_height_percentage < 20) {
        app_config_struct.image_height_percentage = 20;
        ADD_POPUP("Wrong configuration", "Fixed wrong configuration for flag 'image_height_percentage'");
    }

    if (app_config_struct.mobile_portrait_top_gap_percentage > 30 || app_config_struct.mobile_portrait_top_gap_percentage < 0) {
        app_config_struct.mobile_portrait_top_gap_percentage = 0;
        ADD_POPUP("Wrong configuration", "Fixed wrong configuration for flag 'mobile_portrait_top_gap_perc' (must be < 30)");
    }

    if (app_config_struct.mobile_portrait_bottom_gap_percentage > 30 || app_config_struct.mobile_portrait_bottom_gap_percentage < 0) {
        app_config_struct.mobile_portrait_bottom_gap_percentage = 0;
        ADD_POPUP("Wrong configuration", "Fixed wrong configuration for flag 'mobile_portrait_bottom_gap_perc' (must be < 30)");
    }

    if (app_config_struct.mobile_landscape_left_gap_percentage > 30 || app_config_struct.mobile_landscape_left_gap_percentage < 0) {
        app_config_struct.mobile_landscape_left_gap_percentage = 0;
        ADD_POPUP("Wrong configuration", "Fixed wrong configuration for flag 'mobile_landscape_left_gap_perc' (must be < 30)");
    }

    if (app_config_struct.mobile_landscape_right_gap_percentage > 30 || app_config_struct.mobile_landscape_right_gap_percentage < 0) {
        app_config_struct.mobile_landscape_right_gap_percentage = 0;
        ADD_POPUP("Wrong configuration", "Fixed wrong configuration for flag 'mobile_landscape_right_gap_perc' (must be < 30)");
    }

    if (app_config_struct.mobile_ecs_portrait_alpha > 255 || app_config_struct.mobile_ecs_portrait_alpha < 0) {
        app_config_struct.mobile_ecs_portrait_alpha = 0;
        ADD_POPUP("Wrong configuration", "Fixed wrong configuration for flag 'mobile_ecs_portrait_alpha' (must be < 255)");
    }

    if (app_config_struct.mobile_ecs_landscape_alpha > 255 || app_config_struct.mobile_ecs_landscape_alpha < 0) {
        app_config_struct.mobile_ecs_landscape_alpha = 0;
        ADD_POPUP("Wrong configuration", "Fixed wrong configuration for flag 'mobile_ecs_landscape_alpha' (must be < 255)");
    }

    int max_size = app_config_struct.mobile_mode ? 90 : 48;
    int min_font_size = app_config_struct.mobile_mode ? 30 : 15;
    int min_scrollbar_size = app_config_struct.mobile_mode ? 40 : 15;
    int min_buttons_size = app_config_struct.mobile_mode ? 30 : 15;

    if (app_config_struct.font_size > max_size) {
        ADD_POPUP("Wrong configuration", "Fixed wrong configuration for flag 'font_size' (must be <= " << max_size << ")");
        app_config_struct.font_size = max_size;
    }
    if (app_config_struct.scrollbar_size > max_size) {
        ADD_POPUP("Wrong configuration", "Fixed wrong configuration for flag 'scrollbar_size' (must be <= " << max_size << ")");
        app_config_struct.scrollbar_size = max_size;
    }
    if (app_config_struct.buttons_size > max_size) {
        ADD_POPUP("Wrong configuration", "Fixed wrong configuration for flag 'buttons_size' (must be <= " << max_size << ")");
        app_config_struct.buttons_size = max_size;
    }

    if (app_config_struct.font_size < min_font_size) {
        ADD_POPUP("Wrong configuration", "Fixed wrong configuration for flag 'font_size' (must be >= " << min_font_size << ")");
        app_config_struct.font_size = min_font_size;
    }

    if (app_config_struct.scrollbar_size < min_scrollbar_size) {
        ADD_POPUP("Wrong configuration", "Fixed wrong configuration for flag 'scrollbar_size' (must be >= " << min_scrollbar_size << ")");
        app_config_struct.scrollbar_size = min_scrollbar_size;
    }

    if (app_config_struct.buttons_size < min_buttons_size) {
        ADD_POPUP("Wrong configuration", "Fixed wrong configuration for flag 'buttons_size' (must be >= " << min_buttons_size << ")");
        app_config_struct.buttons_size = min_buttons_size;
    }

    if (!app_config_struct.jzintv_fullscreen && get_force_fullscreen()) {
//        ADD_POPUP("Wrong configuration", "Fixed wrong configuration for flag 'jzintv_fullscreen' ");
        app_config_struct.jzintv_fullscreen = true;
    }
}

bool shown_message_config_duplicate = false;
bool shown_message_file_duplicate = false;

int remove_duplicates(vector<rom_config_struct_t> *vec, bool is_for_config) {
    vector<rom_config_struct_t>::iterator it = vec->begin();
    std::set<uint32_t> st;
    string previous_ref;
    uint32_t previous_crc32 = 0;
    ostringstream oss;
    oss << (is_for_config ? "\n\nMultiple configuration for the same crc32:\n" : "\n\nMultiple files for the same crc32:\n");
    bool found = false;
    while (it != vec->end()) {
        bool ident = false;
        rom_config_struct_t *act = &(*it);
        if (previous_crc32 != 0 && act->crc32 == previous_crc32) {
            ident = true;
            found = true;
            if (is_for_config) {
                oss << "\n'" << (act->game_name == NULL ? "<empty game name>" : act->game_name) << "' duplicate of '" << previous_ref << "' (crc32:0x" << std::hex << act->crc32 << ")\n";
            } else {
                oss << "\n'" << act->file_name << "' duplicate of '" << previous_ref << "' (crc32:0x" << std::hex << act->crc32 << ")\n";
            }
        }
        previous_crc32 = act->crc32;
        previous_ref = (is_for_config ? (act->game_name == NULL ? "<empty game name>" : act->game_name) : (act->file_name == NULL ? "<empty file name>" : act->file_name));
        if (ident) {
            free_rom_config_struct(act);
            it = vec->erase(it);
        } else {
            ++it;
        }
    }

    if (found) {
        string trimmed = oss.str().c_str();
        trim(trimmed);
        bool show_popup = (is_for_config ? !shown_message_config_duplicate : !shown_message_file_duplicate);
        if (show_popup) {
            ADD_POPUP("Multiple CRC32", trimmed);
        }
        if (is_for_config) {
            shown_message_config_duplicate = true;
        } else {
            shown_message_file_duplicate = true;
        }
    }
    return vec->size();
}

void LoadIniSettingsFromMemory(const char *ini_data, size_t ini_size) {
    struct app_config_struct_t *app_conf = &app_config_struct;
    char *actual_section = NULL;

    if (ini_size == 0) {
        ini_size = strlen(ini_data);
    }
    string buffer = ini_data;
    buffer[ini_size] = 0;
    char *buf = strdup(buffer.c_str());
    char *const buf_end = buf + ini_size;

    uint32_t act_crc_32 = -1;
    int control_position = -1;
    char section[100];
    char key[100];
    char value[5000]; // Enough ??
    char *line_end = NULL;
    struct rom_config_struct_t *act_rom_config = NULL;
    Control *act_control = NULL;
    int min_malloc_size = MIN_MALLOC_SIZE;
    app_conf->num_total_crc32s = 0;
    bool skip_section = false;
    try {
        for (char *line = buf; line < buf_end; line = line_end + 1) {
            try {
                // Skip new lines markers, then find end of the line
                while (*line == '\n' || *line == '\r') {
                    line++;
                }
                line_end = line;
                while (line_end < buf_end && *line_end != '\n' && *line_end != '\r') {
                    line_end++;
                }
                line_end[0] = 0;
                if (line[0] == ';') {
                    continue;
                }
                int len = strlen_trim(line);
                if (len == 0) {
                    continue;
                }
                // Parse starts here
                if (is_section_ini(line)) {
                    skip_section = false;
                    // Switch to new section (General, game, controls..)
                    get_section_from_ini(line, section);
                    bool is_empty_section = strlen(section) == 0;
                    if (is_empty_section) {
                        THROW_BY_STREAM("Empty section in property file");
                    }
                    if (!strcmp(section, GENERAL_SECTION) || !strcmp(section, GAMES_SECTION) ||
                        !strcmp(section, CONTROLS_SECTION)) {
                        // Main Sections
                        if (actual_section != NULL) {
                            free(actual_section);
                        }
                        actual_section = strdup(section);
                    } else {
                        // Sub Section of actual section
                        if (!strcmp(actual_section, GAMES_SECTION)) {
                            act_crc_32 = hex_to_uint32(section);
                            act_rom_config = add_new_game_by_crc32(act_crc_32);
                        } else if (!strcmp(actual_section, CONTROLS_SECTION)) {
                            skip_section = true;
                            act_control = add_new_delta_default_control_by_undecoded_event(section);
                            skip_section = false;
                        }
                    }
                } else if (!skip_section) {
                    // Data definition for actual section
                    get_key_from_ini(line, key);
                    get_value_from_ini(line, value);
                    bool is_empty_key = strlen(key) == 0;
                    if (is_empty_key) {
                        THROW_BY_STREAM("Empty key in property file");
                    }
                    bool is_empty_value = strlen(value) == 0;
                    if (is_empty_value) {
                        THROW_BY_STREAM("Empty value in property file");
                    }
                    if (!strcmp(actual_section, GENERAL_SECTION)) {
                        if (!set_general_value(key, value, app_conf)) {
                            THROW_BY_STREAM("Invalid key");
                        }
                    } else if (!strcmp(actual_section, GAMES_SECTION)) {
                        bool is_game_key = set_game_value(key, value, act_rom_config);
                        if (!is_game_key) {
                            // Override dei controls per i singoli giochi
                            if (!manage_override_control_for_game(key, value, act_rom_config)) {
                                THROW_BY_STREAM("Invalid key or value");
                            }
                        }
                    } else if (!strcmp(actual_section, CONTROLS_SECTION)) {
                        if (!set_control_value(key, value, act_control)) {
                            THROW_BY_STREAM("Invalid key or value");
                        }
                    }
                }
            }
            catch (const std::stringstream &ex) {
                char buf[1000];
                ADD_CONFIG_WARNING(ex.str().c_str() << " at line '" << line << "'");
            }
        }
    }
    catch (const std::exception &ex) {
        free(buf);
        if (actual_section != NULL) {
            free(actual_section);
        }
        std::stringstream new_ex(ex.what());
        throw new_ex;
    }
    catch (...) {
        free(buf);
        if (actual_section != NULL) {
            free(actual_section);
        }
        std::stringstream new_ex("Unknown error");
        throw new_ex;
    }
    free(buf);
    sort_config_by_crc_32(&roms_configuration);
    app_conf->num_valid_crc32s = remove_duplicates(&roms_configuration, true);
    if (actual_section != NULL) {
        free(actual_section);
    }

    fix_mobile_config();
    fix_incompatible_options();
}

static void write_file_ini(std::string ini_content) {
    std::ofstream fout(properties_file_name);
    fout << ini_content;
    fout.close();
}
