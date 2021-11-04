// FileDialog c++ dll https://docs.microsoft.com/it-it/samples/microsoft/windows-classic-samples/open-dialog-box-sample/
// FileDialog c++ dll https://www.daniweb.com/programming/software-development/threads/446920/setting-a-hook-for-getopenfilename

#include "pch.h" // use stdafx.h in Visual Studio 2017 and earlier
#include <utility>
#include <limits.h>
#include "WinDialog.h"
#include <windows.h>
#include <shobjidl.h> 
#include <vector>

#define PATH_MAX 512

int openDialog(const char *initial_folder, char *selected, const char *title, bool directories, char **descriptions, char **extensions, int numFilters) {
    int rc = 1;
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr)) {
        IFileOpenDialog *pFileOpen;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void **>(&pFileOpen));

        if (SUCCEEDED(hr)) {
            if (title != NULL) {
                wchar_t wtext[200];
                size_t size = strlen(title) + 1;
                size_t outSize;
                mbstowcs_s(&outSize, wtext, size, title, size - 1);
                LPWSTR ptr = wtext;
                hr = pFileOpen->SetTitle(ptr);
            }
            if (directories) {
                DWORD dwOptions;
                hr = pFileOpen->GetOptions(&dwOptions);
                hr = pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS);
            }

            if (initial_folder != NULL) {
                hr = pFileOpen->ClearClientData();
                IShellItem *psiFolder;
                int len = strlen(initial_folder);
                WCHAR szFilePath[PATH_MAX + 1];
                MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, initial_folder, len, szFilePath, PATH_MAX);
                szFilePath[len] = 0;
                hr = SHCreateItemFromParsingName(szFilePath, NULL, IID_PPV_ARGS(&psiFolder));
                if (SUCCEEDED(hr)) {
                    hr = pFileOpen->SetDefaultFolder(psiFolder);
                }
            }

            COMDLG_FILTERSPEC *filter_spec = NULL;
            std::vector<wchar_t *> ptrs;
            if (numFilters > 0) {
                filter_spec = (COMDLG_FILTERSPEC *) calloc(numFilters, sizeof(COMDLG_FILTERSPEC));
                for (int i = 0; i < numFilters; i++) {
                    {
                        wchar_t *wtext = (wchar_t *) malloc(sizeof(wchar_t) * 200);
                        size_t size = strlen(descriptions[i]) + 1;
                        size_t outSize;
                        mbstowcs_s(&outSize, wtext, size, descriptions[i], size - 1);
                        ptrs.push_back(wtext);
                        LPWSTR ptr = wtext;
                        filter_spec[i].pszName = ptr;
                    }
                    {
                        wchar_t *wtext = (wchar_t *) malloc(sizeof(wchar_t) * 200);
                        size_t size = strlen(extensions[i]) + 1;
                        size_t outSize;
                        mbstowcs_s(&outSize, wtext, size, extensions[i], size - 1);
                        LPWSTR ptr = wtext;
                        ptrs.push_back(wtext);
                        filter_spec[i].pszSpec = ptr;
                    }
                }

                pFileOpen->SetFileTypes(numFilters, filter_spec);
            }

            // Show the Open dialog box.
            hr = pFileOpen->Show(NULL);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr)) {
                IShellItem *pItem;
                hr = pFileOpen->GetResult(&pItem);
                if (SUCCEEDED(hr)) {
                    PWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    // Display the file name to the user.
                    if (SUCCEEDED(hr)) {
                        int len;
                        if (0 != (len = WideCharToMultiByte(CP_ACP, 0, pszFilePath, -1, selected, PATH_MAX, NULL, NULL))) {
                            rc = 0;
                            selected[len] = '\0';
                        } else {
                            rc = 1;
                        }

                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
            if (numFilters > 0) {
                free(filter_spec);
                for (int i = 0; i < ptrs.size(); i++) {
                    free(ptrs[i]);
                }
            }
        }
        CoUninitialize();
    }
    if (FAILED(hr)) {
        if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
            rc = 1;
        } else {
            rc = -1;
        }
    }
    return rc;
}
