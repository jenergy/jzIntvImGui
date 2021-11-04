/*

IMASM Macro Precompiler

Copyright (C) 2003  Joe Fisher, Shiny Technologies, LLC
http://www.shinytechnologies.com
Portions Copyright (C) 2003  Joseph Zbiciak.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "includes.h"

extern char g_szChildClass[];
extern HINSTANCE g_hInst;
extern char g_szLocalDir[MAX_PATH];
extern HWND g_hCurChild;
extern HWND g_hMDIClient;
extern RegHelp g_reg;

//-----------------------------------------------------------------------------
// Name: HandleIDC_NEW()
// Desc:
//-----------------------------------------------------------------------------
void HandleIDC_NEW(HWND hMDIWnd)
{
    HWND hChild = CreateMDIWindow(  g_szChildClass,
                                    (LPSTR)"untitled",
                                    WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    hMDIWnd,
                                    g_hInst,
                                    0);
    ShowWindow(hChild, SW_SHOW);
}

//-----------------------------------------------------------------------------
// Name: HandleIDC_SAVEAS()
// Desc:
//-----------------------------------------------------------------------------
int HandleIDC_SAVEAS(HWND hWnd)
{
    OPENFILENAME of;
    int iRet = 0;

    WndInfo *pWI = (WndInfo *)GetWindowLong(hWnd, WL_INFOPTR);

    if (pWI)
    {
        ZeroMemory(&of, sizeof (OPENFILENAME));

        of.lStructSize = sizeof (OPENFILENAME);
        of.hwndOwner = hWnd;
        of.hInstance = g_hInst;
        of.lpstrFilter = "ASM files (*.asm)\0*.asm\0"
                        "ALL files (*.*)\0*.*\0\0";
        of.lpstrFile = pWI->szFileName;
        of.nMaxFile = MAX_PATH;
        of.Flags = OFN_HIDEREADONLY;

        iRet = GetSaveFileName(&of);
    }

    return iRet;
}

//-----------------------------------------------------------------------------
// Name: HandleIDC_SAVE()
// Desc:
//-----------------------------------------------------------------------------
void HandleIDC_SAVE(HWND hWnd)
{
    WndInfo *pWI = (WndInfo *)GetWindowLong(hWnd, WL_INFOPTR);

    if (pWI)
    {
        if (*pWI->szFileName == NUL)
        {
            if (HandleIDC_SAVEAS(hWnd) == 0)
            {
                return;
            }
        }

        if (SaveFile(pWI->hRichEd, pWI->szFileName))
        {
            pWI->bIsDirty = false;
            SetCaptionPrintf(hWnd, "%s", pWI->szFileName);
        }
    }
}

//-----------------------------------------------------------------------------
// Name: HandleIDC_OPEN()
// Desc:
//-----------------------------------------------------------------------------
void HandleIDC_OPEN(HWND hMDIWnd)
{
    OPENFILENAME of;
    char szFileName[MAX_PATH] = "";

    ZeroMemory(&of, sizeof (OPENFILENAME));

    of.lStructSize = sizeof (OPENFILENAME);
    of.hwndOwner = hMDIWnd;
    of.hInstance = g_hInst;
    of.lpstrFilter = "ASM files (*.asm)\0*.asm\0"
                    "ALL files (*.*)\0*.*\0\0";
    of.lpstrFile = szFileName;
    of.nMaxFile = MAX_PATH;
    of.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    int iRet = GetOpenFileName(&of);

    if (iRet)
    {
        LoadFileIntoNewWindow(szFileName);
    }
}

//-----------------------------------------------------------------------------
// Name: HandleIDC_OPENPROJECT()
// Desc:
//-----------------------------------------------------------------------------
void HandleIDC_OPENPROJECT(HWND hMDIWnd, FilePane *pFP)
{
    OPENFILENAME of;
    char szFileName[MAX_PATH] = "";

    ZeroMemory(&of, sizeof (OPENFILENAME));

    of.lStructSize = sizeof (OPENFILENAME);
    of.hwndOwner = hMDIWnd;
    of.hInstance = g_hInst;
    of.lpstrFilter = "IMASM Project files (*.imp)\0*.imp\0"
                    "ALL files (*.*)\0*.*\0\0";
    of.lpstrFile = szFileName;
    of.nMaxFile = MAX_PATH;
    of.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    int iRet = GetOpenFileName(&of);

    if (iRet)
    {
        if (!pFP->LoadProject(szFileName))
        {
            MessageBox(hMDIWnd, "Unable to open project", szFileName, MB_ICONSTOP);
        }
    }
}

//-----------------------------------------------------------------------------
// Name: HandleEN_UPDATE()
// Desc:
//-----------------------------------------------------------------------------
void HandleEN_UPDATE(HWND hWnd)
{
    DWORD dwStart, dwEnd;
    TEXTRANGE tr;
    char szBuff[MAX_PATH];
    int iNum;

    static char szLastWord[MAX_PATH] = "";
    static int iPos = 0;

    // Save the  selection
    SendMessage(hWnd, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);

    // Get the character at the cursor
    tr.chrg.cpMin = dwEnd-1;
    tr.chrg.cpMax = dwEnd;
    tr.lpstrText = szBuff;
    iNum = SendMessage(hWnd, EM_GETTEXTRANGE, (WPARAM)0, (LPARAM)&tr);

    if (iNum > 0)
    {
        // Test to see if the last char of our selection is a delimiter
        int iLen = lstrlen(szBuff);
        char c = szBuff[iLen - 1];

        if (isalnum(c))
        {
            szLastWord[iPos++] = c;
            if (iPos == MAX_PATH)
            {
                // Buffer overflow
                iPos--;
            }

            szLastWord[iPos] = NUL;
        }
        else
        {
            iPos = 0;

            DWORD dwCol = GetWordColor(szLastWord);

            if (dwCol != RGB(0, 0, 0))
            {
                // Colorize the text
                Highlight(hWnd, dwCol, dwEnd - (strlen(szLastWord)+1), dwEnd-1);

                // Reset the  selection
                SendMessage(hWnd, EM_SETSEL, (WPARAM)dwStart, (LPARAM)dwEnd);
                Highlight(hWnd, RGB(0, 0, 0), dwStart, dwEnd);
            }

            *szLastWord = NUL;
        }
    }

    UpdateStatusBar(GetParent(hWnd));
}

//-----------------------------------------------------------------------------
// Name: HandleIDC_PRECOMPILE()
// Desc:
//-----------------------------------------------------------------------------
void HandleIDC_PRECOMPILE(HWND hWnd)
{
    bool bSuccess = true;
    char szSrc[MAX_PATH], szDst[MAX_PATH], szErr[MAX_PATH];

    WndInfo *pWI = (WndInfo *)GetWindowLong(hWnd, WL_INFOPTR);

    if (pWI)
    {
        // First, write to a temporary file
        GetTempFileName(g_szLocalDir, "src", 0, szSrc);
        GetTempFileName(g_szLocalDir, "dst", 0, szDst);

        SaveFile(pWI->hRichEd, szSrc);

        if (ParseFiles(szSrc, szDst, szErr))
        {
            LoadFileIntoNewWindow(szDst);
        }
        else
        {
            MessageBox(hWnd, szErr, "Error", MB_ICONSTOP);
        }

        DeleteFile(szSrc);
        DeleteFile(szDst);
    }
}

//-----------------------------------------------------------------------------
// Name: HandleIDC_FONT()
// Desc:
//-----------------------------------------------------------------------------
void HandleIDC_FONT(HWND hClient)
{
    CHOOSEFONT cf;
    LOGFONT lf;
    CHARFORMAT cfmt;
    HWND hWnd = g_hCurChild;
    HWND hSaveChild = g_hCurChild;

    int iRet;

    ZeroMemory(&cf, sizeof (CHOOSEFONT));
    ZeroMemory(&lf, sizeof (LOGFONT));
    ZeroMemory(&cfmt, sizeof (CHARFORMAT));

    cfmt.cbSize = sizeof (CHARFORMAT);
    cfmt.dwMask = CFM_FACE;// | CFM_SIZE;

    WndInfo *pWI = (WndInfo *)GetWindowLong(hWnd, WL_INFOPTR);

    if (pWI)
    {
        SendMessage(pWI->hRichEd, EM_GETCHARFORMAT, (WPARAM)SCF_DEFAULT, (LPARAM)&cfmt);

        cf.lStructSize = sizeof (CHOOSEFONT);
        cf.hwndOwner = hWnd;
        cf.lpLogFont = &lf;
        cf.Flags = CF_FORCEFONTEXIST | CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;

        strcpy(lf.lfFaceName, cfmt.szFaceName);

        iRet = ChooseFont(&cf);

        if (iRet != 0)
        {
            ZeroMemory(&cfmt, sizeof (CHARFORMAT));

            cfmt.cbSize = sizeof (CHARFORMAT);
            cfmt.dwMask = CFM_FACE;// | CFM_SIZE;
            //cfmt.yHeight = lf.lfHeight;
            strcpy(cfmt.szFaceName, lf.lfFaceName);

            // Save the user's choice in the registry
            g_reg.AddString("FontName", lf.lfFaceName);

            // Set all the windows to this font
            do
            {
                WndInfo *pWI = (WndInfo *)GetWindowLong(hWnd, WL_INFOPTR);

                if (pWI)
                {
                    // Turn off EN_UPDATE and EN_CHANGE messages
                    DWORD dwOldMask = SendMessage(pWI->hRichEd, EM_SETEVENTMASK, (WPARAM)0, 0);

                    // Set the new font
                    SendMessage(pWI->hRichEd, EM_SETCHARFORMAT, (WPARAM)SCF_ALL, (LPARAM)&cfmt);

                    // Turn messages back on
                    SendMessage(pWI->hRichEd, EM_SETEVENTMASK, (WPARAM)0, (LPARAM)dwOldMask);
                }

                SendMessage(hClient, WM_MDINEXT, (WPARAM)hWnd, (LPARAM)0);
                hWnd = (HWND)SendMessage(hClient, WM_MDIGETACTIVE, 0, 0);
            } while ((hWnd != NULL) && (hWnd != hSaveChild));
        }
    }

    SendMessage(hClient, WM_MDIACTIVATE, (WPARAM)hSaveChild, 0);
}

//-----------------------------------------------------------------------------
// Name: HandleIDC_PROJECTNEW()
// Desc:
//-----------------------------------------------------------------------------
void HandleIDC_PROJECTNEW(HWND hWnd, FilePane *pFP)
{
    ItemParam ip;

    int iRet = DialogBoxParam(  g_hInst,
                                MAKEINTRESOURCE(IDD_NEWPROJECT),
                                hWnd,
                                (DLGPROC)NewProjectProc,
                                (LPARAM)&ip);

    if (iRet)
    {
        if (!pFP->NewProject(ip.sName.c_str(), ip.sPath.c_str()))
        {
            MessageBox(hWnd, "Unable to create new project", ip.sName.c_str(), MB_ICONSTOP);
        }
    }
}

//-----------------------------------------------------------------------------
// Name: HandleIDC_PROJECTADDSOURCE()
// Desc:
//-----------------------------------------------------------------------------
void HandleIDC_PROJECTADDSOURCE(HWND hWnd, FilePane *pFP)
{
    char szFileName[MAX_PATH] = "";
    char szFileTitle[MAX_PATH] = "";

    if (pFP->IsOpen())
    {
        if (GetAsmFileName(hWnd, szFileTitle, szFileName))
        {
            pFP->AddSourceFile(szFileTitle, szFileName);
        }
    }
}

//-----------------------------------------------------------------------------
// Name: HandleIDC_PROJECTADDINCLUDE()
// Desc:
//-----------------------------------------------------------------------------
void HandleIDC_PROJECTADDINCLUDE(HWND hWnd, FilePane *pFP)
{
    char szFileName[MAX_PATH] = "";
    char szFileTitle[MAX_PATH] = "";

    if (pFP->IsOpen())
    {
        if (GetAsmFileName(hWnd, szFileTitle, szFileName))
        {
            pFP->AddIncludeFile(szFileTitle, szFileName);
        }
    }
}

//-----------------------------------------------------------------------------
// Name: HandleIDC_PROJECTADDOTHER()
// Desc:
//-----------------------------------------------------------------------------
void HandleIDC_PROJECTADDOTHER(HWND hWnd, FilePane *pFP)
{
    char szFileName[MAX_PATH] = "";
    char szFileTitle[MAX_PATH] = "";

    if (pFP->IsOpen())
    {
        if (GetAsmFileName(hWnd, szFileTitle, szFileName))
        {
            pFP->AddOtherFile(szFileTitle, szFileName);
        }
    }
}

//-----------------------------------------------------------------------------
// Name: HandleIDC_PROJECTREMOVE()
// Desc:
//-----------------------------------------------------------------------------
void HandleIDC_PROJECTREMOVE(HWND hWnd, FilePane *pFP)
{
    pFP->RemoveSelected();
}


//-----------------------------------------------------------------------------
// Name: ()
// Desc:
//-----------------------------------------------------------------------------

