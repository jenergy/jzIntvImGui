#ifndef HANDLERS_H_INCLUDED
#define HANDLERS_H_INCLUDED

#include "includes.h"

void HandleIDC_NEW(HWND hWnd);
void HandleIDC_OPEN(HWND hWnd);
void HandleIDC_OPENPROJECT(HWND hMDIWnd, class FilePane *pFP);
void HandleEN_UPDATE(HWND hWnd);
int HandleIDC_SAVEAS(HWND hWnd);
void HandleIDC_SAVE(HWND hWnd);
void HandleIDC_FONT(HWND hWnd);
void HandleIDC_PRECOMPILE(HWND hWnd);
void HandleIDC_PROJECTNEW(HWND hWnd, FilePane *pFP);
void HandleIDC_PROJECTADDSOURCE(HWND hWnd, FilePane *pFP);
void HandleIDC_PROJECTADDINCLUDE(HWND hWnd, FilePane *pFP);
void HandleIDC_PROJECTADDOTHER(HWND hWnd, FilePane *pFP);
void HandleIDC_PROJECTREMOVE(HWND hWnd, FilePane *pFP);

#endif
