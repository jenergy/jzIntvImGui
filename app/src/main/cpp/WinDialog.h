#pragma once

#ifdef WINDIALOG_EXPORTS
#define WINDIALOG_API __declspec(dllexport)
#else
#define WINDIALOG_API __declspec(dllimport)
#endif

extern "C" WINDIALOG_API int openDialog(const char* initial_folder, char* selected, const char* title, bool directories, char** descriptions = NULL, char** extensions = NULL, int numFilters = 0);
