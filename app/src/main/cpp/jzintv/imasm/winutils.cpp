// Pixels to dialog units
//TabStopList[n] = 4 * DesiredPixelPosition /
//                     LOWORD(GetDialogBaseUnits());


//-----------------------------------------------------------------------------
//
// WinUtils.cpp
//
// Commonly used Windows API shortcuts and convenience functions
//
// by Joe Fisher
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdarg.h>

#include "winutils.h"

void SetUDRange (HWND hwnd, UINT id, int min, int max, int pos)
{
    SendDlgItemMessage (hwnd, id, UDM_SETRANGE, 0, MAKELONG (max, min));
    SendDlgItemMessage (hwnd, id, UDM_SETPOS, 0, MAKELONG (pos, 0));
}

void DlgItemPrintf (HWND hwnd, int ID, char *format, ...)
{
    char buffer [256];
    char *args;
    args = (char *)&format + sizeof (format);
    vsprintf (buffer, format, args);
    SetDlgItemText (hwnd, ID, buffer);
}

void SetCaptionPrintf (HWND hwnd, char *format, ...)
{
    char buffer [256];
    char *args;
    args = (char *)&format + sizeof (format);
    vsprintf (buffer, format, args);
    SetWindowText (hwnd, buffer);
}

void OkMsgBoxPrintf (HWND hwnd, char *title, char *format, ...)
{
    char buffer [256];
    char *args;
    args = (char *)&format + sizeof (format);
    vsprintf (buffer, format, args);
    MessageBox (hwnd, buffer, title, MB_OK);
}

void CenterWindow (HWND hwnd)
{
    int xWindow = GetSystemMetrics (SM_CXSCREEN) / 2;
    int yWindow = GetSystemMetrics (SM_CYSCREEN) / 2;
    RECT rc;
    GetWindowRect (hwnd, &rc);
    xWindow -= (rc.right - rc.left) / 2;
    yWindow -= (rc.bottom - rc.top) / 2;
    MoveWindow (hwnd, xWindow, yWindow, rc.right - rc.left,
                rc.bottom - rc.top, true);
}

bool SetIniFileName (char *iniFileBuff, const char *iniFileName)
{
    int pathLen = GetCurrentDirectory (MAX_PATH - 1, iniFileBuff) - 1;
    if (pathLen <= 0)
    {
        return 0;
    }

    int fileLen = strlen (iniFileName);
    if (iniFileBuff [strlen (iniFileBuff) - 1] != '\\')
    {
        strcat (iniFileBuff, "\\");
    }
    if ((pathLen + fileLen) < MAX_PATH)
    {
        strcat (iniFileBuff, iniFileName);
        return true;
    }
    else
    {
        return false;
    }
}

void DisplayCommDlgError(HWND hWnd, int iError)
{
    switch (iError)
    {
        case CDERR_DIALOGFAILURE:
            MessageBox( hWnd,
                        "CDERR_DIALOGFAILURE",
                        "Common Dialog Error",
                        MB_ICONINFORMATION);
            break;

        case CDERR_FINDRESFAILURE:
            MessageBox( hWnd,
                        "CDERR_FINDRESFAILURE",
                        "Common Dialog Error",
                        MB_ICONINFORMATION);
            break;

        case CDERR_NOHINSTANCE:
            MessageBox( hWnd,
                        "CDERR_NOHINSTANCE",
                        "Common Dialog Error",
                        MB_ICONINFORMATION);
            break;

        case CDERR_INITIALIZATION:
            MessageBox( hWnd,
                        "CDERR_INITIALIZATION",
                        "Common Dialog Error",
                        MB_ICONINFORMATION);
            break;

        case CDERR_NOHOOK:
            MessageBox( hWnd,
                        "CDERR_NOHOOK",
                        "Common Dialog Error",
                        MB_ICONINFORMATION);
            break;

        case CDERR_LOCKRESFAILURE:
            MessageBox( hWnd,
                        "CDERR_LOCKRESFAILURE",
                        "Common Dialog Error",
                        MB_ICONINFORMATION);
            break;

        case CDERR_NOTEMPLATE:
            MessageBox( hWnd,
                        "CDERR_NOTEMPLATE",
                        "Common Dialog Error",
                        MB_ICONINFORMATION);
            break;

        case CDERR_LOADRESFAILURE:
            MessageBox( hWnd,
                        "CDERR_LOADRESFAILURE",
                        "Common Dialog Error",
                        MB_ICONINFORMATION);
            break;

        case CDERR_STRUCTSIZE:
            MessageBox( hWnd,
                        "CDERR_STRUCTSIZE",
                        "Common Dialog Error",
                        MB_ICONINFORMATION);
            break;

        case CDERR_LOADSTRFAILURE:
            MessageBox( hWnd,
                        "CDERR_LOADSTRFAILURE",
                        "Common Dialog Error",
                        MB_ICONINFORMATION);
            break;

        case FNERR_BUFFERTOOSMALL:
            MessageBox( hWnd,
                        "FNERR_BUFFERTOOSMALL",
                        "Common Dialog Error",
                        MB_ICONINFORMATION);
            break;

        case CDERR_MEMALLOCFAILURE:
            MessageBox( hWnd,
                        "CDERR_MEMALLOCFAILURE",
                        "Common Dialog Error",
                        MB_ICONINFORMATION);
            break;

        case FNERR_INVALIDFILENAME:
            MessageBox( hWnd,
                        "FNERR_INVALIDFILENAME",
                        "Common Dialog Error",
                        MB_ICONINFORMATION);
            break;

        case CDERR_MEMLOCKFAILURE:
            MessageBox( hWnd,
                        "CDERR_MEMLOCKFAILURE",
                        "Common Dialog Error",
                        MB_ICONINFORMATION);
            break;

        case FNERR_SUBCLASSFAILURE:
            MessageBox( hWnd,
                        "FNERR_SUBCLASSFAILURE",
                        "Common Dialog Error",
                        MB_ICONINFORMATION);
            break;
    }
}

//-----------------------------------------------------------------------------
// Name: DisplayLastError()
// Desc: Displays a message box with the last system error
//-----------------------------------------------------------------------------
void DisplayLastError()
{
    LPVOID lpMsgBuf;
    FormatMessage(  FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    GetLastError(),
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                    (LPTSTR) &lpMsgBuf,
                    0,
                    NULL);

    // Display the string.
    MessageBox(NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION);

    // Free the buffer.
    LocalFree(lpMsgBuf);
}



/*
        case WM_NCACTIVATE:
            if ((BOOL)wParam == FALSE)
            {
                DefWindowProc(hWnd, msg, wParam, lParam);
                RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_FRAME);
                break;
            }
        // Fall through if wParam == TRUE, i.e., window is active.

        case WM_NCPAINT:
        {
            DefWindowProc(hWnd, msg, wParam, lParam);

            HDC hdc = GetWindowDC(hWnd);
            HDC memDC = CreateCompatibleDC(hdc);

            HINSTANCE hInst = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
            HBITMAP hBit = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_PINOUT), IMAGE_BITMAP, 16, 16, LR_DEFAULTSIZE);

            SelectObject(memDC, hBit);
            BitBlt(hdc, 130, 5, 16, 16, memDC, 0, 0, SRCCOPY);

            ReleaseDC(hWnd, hdc);

            DeleteObject(hBit);
            DeleteDC(memDC);

            return 0;
        }

*/
