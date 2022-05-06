#include "main.h"
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>

bool exist_file(const char *fileName) {
    std::ifstream infile(fileName);
    return infile.good();
}

bool exist_folder(string pathname) {
    char *realPathname = normalize_path(pathname.c_str(), false); // stat fails if there's a final slash
    bool res = false;
    struct stat info;

    int tmp = stat(realPathname, &info);
    if (tmp == 0 && info.st_mode & S_IFDIR) {
        res = true;
    }
    free(realPathname);
    return res;
}

bool copy_file(string source, string destination) {
    std::ifstream srce(source.c_str(), std::ios::binary);
    std::ofstream dest(destination.c_str(), std::ios::binary);
    dest << srce.rdbuf();
    return exist_file(destination.c_str());
}

char *get_file_name(string pathname) {
    char *tmp = normalize_path(pathname.c_str(), false);
    const std::vector<std::string> &vec_append_path = split(tmp, "/", false);
    free(tmp);
    int siz = vec_append_path.size();
    string res = vec_append_path[siz - 1];
    return strdup(res.c_str());
}

char *get_absolute_path(const char *original_root_path, const char *append_path, bool is_path_free, int *result) {
    char *tmp = strdup(original_root_path);
    char *root_path = normalize_path(tmp, true);
    free(tmp);
    *result = 0;
    bool error_occurred = false;
    char *normalized_append_path = normalize_path(append_path, true);
    std::string final_path;

    if (!is_path_free) {
        if (normalized_append_path[0] == '/') {
            char tmp[FILENAME_MAX];
            memcpy(tmp, &normalized_append_path[1], strlen(normalized_append_path));
            free(normalized_append_path);
            normalized_append_path = strdup(tmp);
        }

        if (strlen(normalized_append_path) >= 2 && normalized_append_path[0] == '.' && normalized_append_path[1] == '.') {
            error_occurred = true;
        }
    }

    if (!error_occurred) {
        char *prefix = strdup(root_path);
        const std::vector<std::string> &vec_append_path = split(normalized_append_path, "/", false);
        const char *drive_append_path = vec_append_path[0].c_str();
        int len = strlen(drive_append_path);
        bool absolute_path_provided = normalized_append_path[0] == '/' || (len > 0 && drive_append_path[len - 1] == ':');

        if (absolute_path_provided) {
            free(prefix);
            prefix = strdup("");
            if (normalized_append_path[0] == '/') {
                char *tmp = strdup(root_path);
                const std::vector<std::string> &vec_root_path = split(tmp, "/", false);
                free(tmp);
                const char *drive_root_path = vec_root_path[0].c_str();
                int len = strlen(drive_root_path);
                if (len > 0 && drive_root_path[len - 1] == ':') {
                    free(prefix);
                    prefix = strdup(drive_root_path);
                }
            }
        }
        char complete_path[FILENAME_MAX];
        sprintf(complete_path, "%s%s", prefix, normalized_append_path);
        free(prefix);

        const std::vector<std::string> &vec = split(complete_path, "/", false);
        int max = vec.size();

        std::vector<int> vecInt;

        for (int i = 0; i < max; i++) {
            vecInt.push_back(1);
        }

        for (int i = 0; i < max; i++) {
            if (!vec.at(i).compare("..")) {
                for (int j = i - 1; j >= -1; j--) {
                    if (j == -1) {
                        error_occurred = true;
                        break;
                    }
                    std::string tmp(vec.at(j).c_str());
                    trim(tmp);
                    if (tmp.compare("..") && tmp.compare(".")) {
                        if (vecInt.at(j) == 1) {
                            vecInt[j] = 0;
                            break;
                        }
                    }
                }
            }
        }

        if (!error_occurred) {
            if (complete_path[0] == '/') {
                final_path.append("/");
            }
            for (int i = 0; i < max; i++) {
                std::string tmp(vec.at(i).c_str());
                ltrim(tmp);
                const char *tmpCharp = tmp.c_str();
                if (strlen(tmpCharp) > 0 && strcmp(tmpCharp, ".") && strcmp(tmpCharp, "..") && vecInt.at(i) == 1) {
                    final_path.append(vec.at(i)).append("/");
                }
            }

            if (!is_path_free) {
                // Verificare che il path finale inizi con rootPath
                int compare_res = strncmp(root_path, final_path.c_str(), strlen(root_path));
                if (compare_res != 0) {
                    error_occurred = true;
                }
            }
        }
    }
    free(normalized_append_path);
    free(root_path);
    if (error_occurred) {
        *result = -1;
        return strdup(root_path);
    } else {
        return strdup(final_path.c_str());
    }
}

char *normalize_path(const char *path, bool is_directory) {
    char *replaceDoubleBack = replaceWord(path, "\\", "/");
    char *val2 = replaceWord(replaceDoubleBack, "//", "/");
    char *val = replaceWord(val2, "//", "/");
    free(replaceDoubleBack);
    free(val2);

    if (!is_directory) {
        if (val[strlen(val) - 1] == '/') {
            val[strlen(val) - 1] = 0;
        }
    } else {
        if (val[strlen(val) - 1] != '/') {
            char tmp[FILENAME_MAX];
            sprintf(tmp, "%s/", val);
            free(val);
            val = strdup(tmp);
        }
    }
    return val;
}

#ifdef WIN32

#include <windows.h>
#include <shellapi.h>

// Uncomment if DLL is not available
#include <shlobj.h>
//static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {
//    if (uMsg == BFFM_INITIALIZED) {
//        std::string tmp = (const char *) lpData;
//        std::cout << "path: " << tmp << std::endl;
//        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
//    }
//
//    return 0;
//}
//
//int browse_folder(const char *initial_folder, char *selected) {
//    TCHAR path[MAX_PATH];
//
//    BROWSEINFO bi = {0};
//    bi.lpszTitle = ("Browse for folder...");
//    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
//    bi.lpfn = BrowseCallbackProc;
//    bi.lParam = (LPARAM) initial_folder;
//
//    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
//
//    if (pidl != 0) {
//        //get the name of the folder and put it in path
//        SHGetPathFromIDList(pidl, path);
//
//        //free memory used
//        IMalloc *imalloc = 0;
//        if (SUCCEEDED(SHGetMalloc(&imalloc))) {
//            imalloc->Free(pidl);
//            imalloc->Release();
//        }
//
//        strcpy(selected, path);
//        return 0;
//    }
//
//    return 1;
//}
//
//// Returns an empty string if dialog is canceled
//int openfilename(const char *initial_folder, char *selected, char **filter_descriptions, char **filter_extensions, int numFilters) {
//
//    OPENFILENAME ofn;
//    char fileName[MAX_PATH] = "";
//    ZeroMemory(&ofn, sizeof(ofn));
//
//    ofn.lStructSize = sizeof(OPENFILENAME);
//    ofn.hwndOwner = NULL;
//    ofn.lpstrInitialDir = initial_folder;
//
//    // Filters management
//    string oss;
//    for (int i = 0; i < numFilters; i++) {
//        oss.append(filter_descriptions[i]).append(" (").append(filter_extensions[i]).append(")");
//        oss.push_back('\0');
//        oss.append(filter_extensions[i]);
//        oss.push_back('\0');
//    }
//
//    if (numFilters > 0) {
//        ofn.lpstrFilter = oss.c_str();
//    }
//
//    ofn.lpstrFile = fileName;
//    ofn.nMaxFile = MAX_PATH;
//    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
//    ofn.lpstrDefExt = "";
//
//    int res = 1;
//    if (GetOpenFileName(&ofn)) {
//        res = 0;
//        strcpy(selected, fileName);
//    }
//
//    return res;
//}

// Gestione dialog con Dll
#include "WinDialog.h"

int browse_folder(const char *initial_folder, char *selected) {
    openDialog(initial_folder, selected, NULL, true);
}

int openfilename(const char *initial_folder, char *selected, char **filter_descriptions, char **filter_extensions, int numFilters) {
    openDialog(initial_folder, selected, NULL, false, filter_descriptions, filter_extensions, numFilters);
}

#else
#include <ImGuiFileDialog.h>
int browse_folder(const char *initial_folder, char *selected) {
    int res = 1;
    string init_folder = initial_folder;
    init_folder.append("/");
    ImGuiFileDialog::Instance()->OpenModal("ChooseDlgKey", "Choose Folder", NULL, init_folder);
    return res;
}

int openfilename(const char *initial_folder, char *selected, char **filter_descriptions, char **filter_extensions, int numFilters) {
    int res = 1;

    string init_folder = initial_folder;
    init_folder.append("/");
    string filters = "";
    if (numFilters > 0) {
        for (int i = 0; i < numFilters; i++) {
            char *tmp = replaceWord(filter_extensions[i], ";", ",");
            char *tmp3 = replaceWord(filter_extensions[i], ";", " ");
            char *tmp2 = replaceWord(tmp, "*.", ".");
            filters.append(filter_descriptions[i]).append(" (").append(tmp3).append(")").append("{").append(tmp2).append("}");
            free(tmp);
            free(tmp2);
            free(tmp3);
        }
    }
    ImGuiFileDialog::Instance()->OpenModal("ChooseDlgKey", "Choose File", filters.empty() ? "All files (*.*){.*}" : filters.c_str(), init_folder);
    return res;
}

#endif

std::string browse_item(std::string saved_path, const char *restore, bool directory, char **filter_descriptions, char **filter_extensions, int numFilters) {
    char selected[PATH_MAX];
    int res;
    res = directory ? browse_folder(saved_path.c_str(), selected) : openfilename(saved_path.c_str(), selected, filter_descriptions, filter_extensions, numFilters);
    if (!res) {
        return selected;
    } else {
        return restore;
    }
}

void create_folder(string folder) {
#ifdef WIN32
    mkdir(folder.c_str());
#else
    mkdir(folder.c_str(), 0777);
#endif
}

std::string get_formatted_path(const char* path, bool keep_final_slash) {
#ifdef WIN32
    string dos_command = "for %f in (\"";
    dos_command.append(path);
    dos_command.append("\") do @echo %~sf");

    string res = exec(dos_command.c_str());

    int len = res.length();
    const char *resc = res.c_str();
    while (resc[len - 1] == '\n' || resc[len - 1] == '\r') {
        res.pop_back();
        len = res.length();
        resc = res.c_str();
    }

    if (!keep_final_slash) {
        len = res.length();
        resc = res.c_str();
        while (resc[len - 1] == '/' || resc[len - 1] == '\\') {
            res.pop_back();
            len = res.length();
            resc = res.c_str();
        }
    }

    std::replace(res.begin(), res.end(), '/', '\\');
    return res;
#else
    return path;
#endif
}

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}
