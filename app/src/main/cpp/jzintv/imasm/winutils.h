#ifndef WINUTILS_H_INCLUDED
#define WINUTILS_H_INCLUDED

#include <windows.h>
#include <commctrl.h>

#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete [] (p);   (p)=NULL; } }

void SetUDRange (HWND, UINT, int, int, int);
void DlgItemPrintf (HWND, int , char *, ...);
void OkMsgBoxPrintf (HWND, char *, char *, ...);
void SetCaptionPrintf (HWND, char *, ...);
void CenterWindow (HWND);
bool SetIniFileName (char *, const char *);
void DisplayCommDlgError(HWND, int);
void DisplayLastError();
#endif
