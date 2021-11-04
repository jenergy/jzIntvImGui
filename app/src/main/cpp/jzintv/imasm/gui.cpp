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
#include "llist.h"

char g_szLocalDir[MAX_PATH];
char g_szFrameClass[] = "IMASM_PREASSEMBLER_FRAME";
char g_szChildClass[] = "IMASM_PREASSEMBLER_CHILD";
char g_szAppName[] = "Imasm macro preassembler";

bool g_bAutoUpdate = false;

HWND g_hToolBar = NULL;
HWND g_hStatBar = NULL;
HINSTANCE g_hInst;
char g_szClass[50];

LList<SyntaxInfo> g_infoList;

char g_szCmdLineFile[MAX_PATH] = "";

extern HWND g_hCurChild;
extern HWND g_hMDIClient;

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point for the application.
//-----------------------------------------------------------------------------

INT APIENTRY WinMain(   HINSTANCE hInst,
                        HINSTANCE hPrevInst,
                        LPSTR pCmdLine,
                        INT nCmdShow)


{
    MSG msg;
    char *pC;
    INITCOMMONCONTROLSEX icex;
    HMODULE hRichEdLib = NULL;

    icex.dwSize = sizeof (INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;

    // Initialize the common controls we will use
    InitCommonControlsEx(&icex);

    // Determine which rich edit library to use
    hRichEdLib = LoadLibrary("riched20.dll");

    if (hRichEdLib != NULL)
    {
        strcpy(g_szClass, RICHEDIT_CLASS);
    }
    else
    {
        hRichEdLib = LoadLibrary("riched32.dll");

        if (hRichEdLib != NULL)
        {
            strcpy(g_szClass, "RichEdit");
        }
        else
        {
            DisplayLastError();
        }
    }

    g_hInst = hInst;

    // Figure out our local directory
    GetModuleFileName(NULL, g_szLocalDir, MAX_PATH);
    pC = strrchr(g_szLocalDir, '\\');
    if (pC != NULL)
    {
        *(pC + 1) = NULL;
    }

    LoadConfigInfo();

    if (DoCommandLine(pCmdLine))
    {
        // Register our unique class object
        MyRegisterClass(hInst);

        // Perform application initialization:
        HWND hMain = InitInstance(hInst, nCmdShow);

        if (hMain != NULL)
        {
            // Get accelerator list
            HACCEL hAccel = LoadAccelerators(hInst, MAKEINTRESOURCE(IDR_ACCEL));

            // Message Pump
            while (GetMessage(&msg, NULL, 0, 0) > 0)
            {
                if (!TranslateMDISysAccel(g_hMDIClient, &msg) &&
                    !TranslateAccelerator(hMain, hAccel, &msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
    }

    FreeLibrary(hRichEdLib);
    return 0;
}

//-----------------------------------------------------------------------------
// NAME: MyRegisterClass()
// DESCRIPTION: Registers the window class.
//-----------------------------------------------------------------------------
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)FrameProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICON));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDM_MAIN);
    wcex.lpszClassName  = g_szFrameClass;
    wcex.hIconSm        = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICON));

    if (!RegisterClassEx(&wcex))
    {
        return FALSE;
    }

    wcex.lpfnWndProc    = (WNDPROC)ChildProc;
    wcex.cbWndExtra     = sizeof (WndInfo) + 4;
    wcex.lpszMenuName   = NULL;
    //wcex.hIcon            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICON));
    wcex.lpszClassName  = g_szChildClass;
/*
    wcex.lpfnWndProc    = (WNDPROC)FilePaneProc;
    //wcex.cbWndExtra       = sizeof (WndInfo) + 4;
    wcex.lpszMenuName   = NULL;
    //wcex.hIcon            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICON));
    wcex.lpszClassName  = g_szFilePaneClass;
*/
    return RegisterClassEx(&wcex);
}

//-----------------------------------------------------------------------------
// NAME: InitInstance(HANDLE, int)
// DESCRIPTION: Saves instance handle and creates main window
//-----------------------------------------------------------------------------
HWND InitInstance(HINSTANCE hInstance, int iCmdShow)
{
    HWND hWnd;

    hWnd = CreateWindowEx(  0,
                            g_szFrameClass,
                            g_szAppName,
                            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            NULL,
                            NULL,
                            hInstance,
                            NULL);

    // If we could not create the window, stop exectution
    if (hWnd == NULL)
    {
        return FALSE;
    } // end if

    ShowWindow(hWnd, iCmdShow);
    UpdateWindow(hWnd);
    return hWnd;
}

//-----------------------------------------------------------------------------
// Name: DoExitProcessing()
// Desc:
//-----------------------------------------------------------------------------
void DoExitProcessing(HWND hWnd)
{
    KillTimer(hWnd, UPDATETIMER);
}

//-----------------------------------------------------------------------------
// Name: Highlight()
// Desc:
//-----------------------------------------------------------------------------
void Highlight(HWND hWnd, COLORREF col, DWORD dwStart, DWORD dwEnd)
{
    CHARFORMAT cf;

    // Select the text we are interested in
    SendMessage(hWnd, EM_SETSEL, (WPARAM)dwStart, (LPARAM)dwEnd);

    // Populate a CHARFORMAT structure accordingly
    cf.cbSize   = sizeof(CHARFORMAT);
    cf.dwMask   = CFM_COLOR;
    cf.dwEffects    = 0;
    cf.crTextColor  = col;

    // Colorize the text
    SendMessage(hWnd, EM_SETCHARFORMAT, (WPARAM)(SCF_SELECTION), (LPARAM)&cf);
}

//-----------------------------------------------------------------------------
// Name: LoadFile()
// Desc:
//-----------------------------------------------------------------------------
int LoadFile(HWND hRichEd, char *fName)
{
    HANDLE hFile = (HANDLE)CreateFile(  fName,
                                        GENERIC_READ,
                                        FILE_SHARE_READ,
                                        NULL,
                                        OPEN_EXISTING,
                                        0,
                                        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    EDITSTREAM es;
    es.dwCookie = (DWORD)hFile;
    es.dwError = 0;
    es.pfnCallback = LoadFileCallback;

    // Turn off EN_UPDATE and EN_CHANGE messages
    DWORD dwOldMask = SendMessage(hRichEd, EM_SETEVENTMASK, (WPARAM)0, 0);

    // Stream data in
    SendMessage(hRichEd, EM_STREAMIN, (WPARAM)SF_TEXT, (LPARAM)&es);

    // Turn messages back on
    SendMessage(hRichEd, EM_SETEVENTMASK, (WPARAM)0, (LPARAM)dwOldMask);

    CloseHandle(hFile);

    return 1;
}

//-----------------------------------------------------------------------------
// Name: SaveFile()
// Desc:
//-----------------------------------------------------------------------------
int SaveFile(HWND hRichEd, char *fName)
{
    OFSTRUCT of;
    EDITSTREAM es;
    HANDLE hFile = (HANDLE)OpenFile(fName, &of, OF_CREATE);

    if (!hFile)
    {
        return 0;
    }

    es.dwCookie = (DWORD)hFile;
    es.dwError = 0;
    es.pfnCallback = SaveFileCallback;

    SendMessage(hRichEd, EM_STREAMOUT, (WPARAM)SF_TEXT, (LPARAM)&es);

    CloseHandle(hFile);

    return 1;
}

//-----------------------------------------------------------------------------
// Name: GetWordColor()
// Desc:
//-----------------------------------------------------------------------------
COLORREF GetWordColor(char *szWord)
{
    SyntaxInfo *pSI;
    int i, iNum;

    if ((szWord == NULL) || (*szWord == NUL))
    {
        return RGB(0, 0, 0);
    }

    iNum = g_infoList.GetNumNodes();

    for (i = 0; i < iNum; i++)
    {
        pSI = g_infoList.GetNode(i);

        if (stricmp(szWord, pSI->sName.c_str()) == 0)
        {
            return pSI->crCol;
        }
    }

    return RGB(0, 0, 0);
}

//-----------------------------------------------------------------------------
// Name: LoadFileCallback()
// Desc:
//-----------------------------------------------------------------------------
DWORD CALLBACK LoadFileCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
    HFILE hFile = (HFILE)dwCookie;

    int iRet = ReadFile(    (HANDLE)hFile,
                            (LPVOID)pbBuff,
                            (DWORD)cb,
                            (LPDWORD)pcb,
                            NULL);

    if (iRet)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

//-----------------------------------------------------------------------------
// Name: SaveFileCallback()
// Desc:
//-----------------------------------------------------------------------------
DWORD CALLBACK SaveFileCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
    HFILE hFile = (HFILE)dwCookie;

    int iRet = WriteFile(   (HANDLE)hFile,
                            (LPCVOID)pbBuff,
                            (DWORD)cb,
                            (LPDWORD)pcb,
                            NULL);

    if (iRet)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

//-----------------------------------------------------------------------------
// Name: HighlightDoc()
// Desc:
//-----------------------------------------------------------------------------
void HighlightDoc(HWND hRichEd)
{
    DWORD dwPos, dwSize;
    TEXTRANGE tr;
    GETTEXTLENGTHEX tl;

    DWORD dwStart, dwEnd;

    char szBuff[MAX_PATH];
    int iNum;

    char szWord[MAX_PATH] = "";
    int iPos = 0;

    // Save the current cursor position
    SendMessage(hRichEd, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);

    // Turn off updates
    SendMessage(hRichEd, WM_SETREDRAW, (WPARAM)FALSE, 0);

    // Turn off EN_UPDATE and EN_CHANGE messages
    DWORD dwOldMask = SendMessage(hRichEd, EM_SETEVENTMASK, (WPARAM)0, 0);

    tl.flags = GTL_NUMCHARS;
    tl.codepage = CP_ACP;
    dwSize = SendMessage(hRichEd, EM_GETTEXTLENGTHEX, (WPARAM)&tl, 0);

    for (dwPos = 1; dwPos < dwSize; dwPos++)
    {
        // Get the character at the cursor
        tr.chrg.cpMin = dwPos-1;
        tr.chrg.cpMax = dwPos;
        tr.lpstrText = szBuff;
        iNum = SendMessage(hRichEd, EM_GETTEXTRANGE, (WPARAM)0, (LPARAM)&tr);

        if (iNum > 0)
        {
            // Test to see if the last char of our selection is a delimiter
            int iLen = lstrlen(szBuff);
            char c = szBuff[iLen - 1];

            if (isalnum(c))
            {
                szWord[iPos++] = c;
                if (iPos == MAX_PATH)
                {
                    // Buffer overflow
                    iPos--;
                }

                szWord[iPos] = NUL;
            }
            else
            {
                COLORREF crCol = GetWordColor(szWord);

                // Colorize the text
                Highlight(hRichEd, crCol, dwPos - (strlen(szWord)+1), dwPos);

                iPos = 0;
                *szWord = NUL;
            }
        }
    }

    // Restore cursor position
    SendMessage(hRichEd, EM_SETSEL, (WPARAM)dwStart, (LPARAM)dwEnd);

    // Turn updates back on
    SendMessage(hRichEd, WM_SETREDRAW, (WPARAM)TRUE, 0);
    RedrawWindow(hRichEd, NULL, NULL, TRUE);
    SendMessage(hRichEd, EM_SETEVENTMASK, (WPARAM)0, (LPARAM)dwOldMask);
}

//-----------------------------------------------------------------------------
// Name: ConfirmSave()
// Desc:
//-----------------------------------------------------------------------------
void ConfirmSave(HWND hWnd)
{
    int iRet;

    WndInfo *pWI = (WndInfo *)GetWindowLong(hWnd, WL_INFOPTR);

    if (pWI)
    {
        if (pWI->bIsDirty)
        {
            iRet = MessageBox(  hWnd,
                                "Text has changed, would you like to save?",
                                (*pWI->szFileName == NUL) ? "untitled" : pWI->szFileName,
                                MB_YESNO);
            if (iRet == IDYES)
            {
                HandleIDC_SAVE(hWnd);
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Name: LoadConfigInfo()
// Desc:
//-----------------------------------------------------------------------------
void LoadConfigInfo()
{
    ifstream inFile;
    string s, sFile = g_szLocalDir;
    char c, szBuff[MAX_PATH] = "";
    int i, iNum, iPos = 0;
    int iRed, iGreen, iBlue;
    bool bError = false;
    bool bDone = false;
    COLORREF crCol;
    SyntaxInfo si;

    sFile += "imasmsyntax.cfg";

    inFile.open(sFile.c_str());

    if (inFile.fail())
    {
        bDone = true;
    }
    else
    {
        while (!inFile.eof() && !bError)
        {
            // Find the color
            while (!inFile.eof() && !bError)
            {
                getline(inFile, s);
                iPos = s.find_first_of('[');

                if (iPos != string::npos)
                {
                    Token t("[] \t\r\n,");
                    t = s;
                    if (t.GetNumTokens() >= 3)
                    {
                        iRed = atoi(t[0]);
                        iGreen = atoi(t[1]);
                        iBlue = atoi(t[2]);

                        crCol = RGB(iRed, iGreen, iBlue);
                        break;
                    }
                    else
                    {
                        bError = true;
                        break;
                    }
                }
            } // while for color

            // Now read in the names
            // First find the opening brace
            do
            {
                c = inFile.get();
            } while (!inFile.eof() && (c != '{'));

            if (c == '{')
            {
                bDone = false;
            }

            // Now read in the names
            while (!inFile.eof() && !bError && !bDone)
            {
                getline(inFile, s);

                Token t(" \t\r\n,");

                iPos = s.find_first_of('}');

                if (iPos != string::npos)
                {
                    bDone = true;
                    s = s.substr(0, iPos);
                }

                t = s;

                iNum = t.GetNumTokens();

                for (i = 0; i < iNum; i++)
                {
                    si.crCol = crCol;
                    si.sName = t[i];
                    g_infoList.AddNode(si);
                }
            } // while for names
        }// while for main loop
    }

    inFile.close();

    if (bError)
    {
        OkMsgBoxPrintf(NULL, "Error", "Invalid syntax in config file: %s", s.c_str());
    }
    else if (!bDone)
    {
        OkMsgBoxPrintf(NULL, "Error", "Mismatched braces in config file");
    }


    if (1)
    {
        g_bAutoUpdate = true;
    }
}

//-----------------------------------------------------------------------------
// Name: void CreateToolbar()
// Desc:
//-----------------------------------------------------------------------------
void CreateToolbar(HWND hWnd)
{
    TBBUTTON btn[10];

    g_hToolBar = CreateWindowEx(0,
                                TOOLBARCLASSNAME,
                                NULL,
                                WS_BORDER | WS_VISIBLE | WS_CHILD | TBSTYLE_TOOLTIPS | CCS_TOP,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                hWnd,
                                (HMENU)IDC_TOOLBAR,
                                g_hInst,
                                0);

    SendMessage(g_hToolBar, TB_BUTTONSTRUCTSIZE, sizeof (TBBUTTON), 0);

    // Add stock bitmaps
    TBADDBITMAP tba;
    tba.hInst = HINST_COMMCTRL;
    tba.nID = IDB_STD_SMALL_COLOR;
    SendMessage(g_hToolBar, TB_ADDBITMAP, 15, (LPARAM)&tba);

    // Add user defined bitmaps
    tba.hInst = g_hInst;
    tba.nID = IDB_AUTO;
    SendMessage(g_hToolBar, TB_ADDBITMAP, 4, (LPARAM)&tba);


    btn[0].iBitmap = STD_FILENEW;
    btn[0].idCommand = IDC_NEW;
    btn[0].fsState = TBSTATE_ENABLED;
    btn[0].fsStyle = TBSTYLE_BUTTON;
    btn[0].dwData = 0;
    btn[0].iString = 0;

    btn[1].iBitmap = 0;
    btn[1].idCommand = 0;
    btn[1].fsState = TBSTATE_ENABLED;
    btn[1].fsStyle = TBSTYLE_SEP;
    btn[1].dwData = 0;
    btn[1].iString = 0;

    btn[2].iBitmap = STD_FILEOPEN;
    btn[2].idCommand = IDC_OPEN;
    btn[2].fsState = TBSTATE_ENABLED;
    btn[2].fsStyle = TBSTYLE_BUTTON;
    btn[2].dwData = 0;
    btn[2].iString = 0;

    btn[3].iBitmap = 18;
    btn[3].idCommand = IDC_OPENPROJECT;
    btn[3].fsState = TBSTATE_ENABLED;
    btn[3].fsStyle = TBSTYLE_BUTTON;
    btn[3].dwData = 0;
    btn[3].iString = 0;

    btn[4].iBitmap = STD_FILESAVE;
    btn[4].idCommand = IDC_SAVE;
    btn[4].fsState = TBSTATE_ENABLED;
    btn[4].fsStyle = TBSTYLE_BUTTON;
    btn[4].dwData = 0;
    btn[4].iString = 0;

    btn[5].iBitmap = 15;
    btn[5].idCommand = IDC_SAVEAS;
    btn[5].fsState = TBSTATE_ENABLED;
    btn[5].fsStyle = TBSTYLE_BUTTON;
    btn[5].dwData = 0;
    btn[5].iString = 0;

    btn[6].iBitmap = 0;
    btn[6].idCommand = 0;
    btn[6].fsState = TBSTATE_ENABLED;
    btn[6].fsStyle = TBSTYLE_SEP;
    btn[6].dwData = 0;
    btn[6].iString = 0;

    btn[7].iBitmap = 16;
    btn[7].idCommand = IDC_AUTO;
    btn[7].fsState = TBSTATE_ENABLED;
    btn[7].fsStyle = TBSTYLE_BUTTON | TBSTYLE_CHECK;
    btn[7].dwData = 0;
    btn[7].iString = 0;

    btn[8].iBitmap = 0;
    btn[8].idCommand = 0;
    btn[8].fsState = TBSTATE_ENABLED;
    btn[8].fsStyle = TBSTYLE_SEP;
    btn[8].dwData = 0;
    btn[8].iString = 0;

    btn[9].iBitmap = 17;
    btn[9].idCommand = IDC_PRECOMPILE;
    btn[9].fsState = TBSTATE_ENABLED;
    btn[9].fsStyle = TBSTYLE_BUTTON;
    btn[9].dwData = 0;
    btn[9].iString = 0;

    SendMessage(g_hToolBar, TB_ADDBUTTONS, 10, (LPARAM)btn);
}

//-----------------------------------------------------------------------------
// Name: DoCommandLine()
// Desc: Return nonzero to run GUI
//-----------------------------------------------------------------------------
int DoCommandLine(char *szBuff)
{
    Token t(" \t");

    t = szBuff;

    int iNum = t.GetNumTokens();

    if (iNum == 2)
    {
        ParseFiles(t[0], t[1], NULL);
        return 0;
    }
    else if (iNum == 1)
    {
        strcpy(g_szCmdLineFile, t[0]);
    }

    return 1;
}

//-----------------------------------------------------------------------------
// Name: ParseFiles()
// Desc:
//-----------------------------------------------------------------------------
bool ParseFiles(const char *szSrc, const char *szDst, char *szError)
{
    bool bSuccess = true;

    try
    {
        StringFIFO_fromFile iFIFO((char *)szSrc);
        StringFIFO_toFile   oFIFO((char *)szDst);
        Parser              p(&iFIFO, &oFIFO);

        p.ParseSourceFile();
    }
    catch(FileNotFound &f)
    {
        strcpy(szError, f.msg.c_str());
        return false;
    }
    catch(InternalError &ie)
    {
        strcpy(szError, ie.msg.c_str());
        return false;
    }
    catch(StringFIFO::BufferOverflow &bo)
    {
        strcpy(szError, "Buffer overflow");
        return false;
    }
    catch(Parser::ParseError &p)
    {
        strcpy(szError, p.msg.c_str());
        return false;
    }
    catch(...) // catch all
    {
        strcpy(szError, "Unhandled exception");
        return false;
    }

    return bSuccess;
}

//-----------------------------------------------------------------------------
// Name: UpdateStatusBar()
// Desc:
//-----------------------------------------------------------------------------
void UpdateStatusBar(HWND hWnd)
{
    WndInfo *pWI = (WndInfo *)GetWindowLong(hWnd, WL_INFOPTR);

    if (pWI)
    {
        int dwSt, dwEnd;
        char szBuff[MAX_PATH];

        SendMessage(pWI->hRichEd, EM_GETSEL, (WPARAM)&dwSt, (LPARAM)&dwEnd);

        int iLine = SendMessage(pWI->hRichEd, EM_LINEFROMCHAR, dwEnd, 0);
        int iLineIndx = SendMessage(pWI->hRichEd, EM_LINEINDEX, iLine, 0);

        wsprintf(szBuff, " Ln %d, Col %d", iLine, dwEnd - iLineIndx);
        SendMessage(g_hStatBar, SB_SETTEXT, 1 | 0, (LPARAM)szBuff);

        SHORT key = GetKeyState(VK_CAPITAL);
        if (key & 0x1)
        {
            SendMessage(g_hStatBar, SB_SETTEXT, 2 | 0, (LPARAM)"CAP");
        }
        else
        {
            SendMessage(g_hStatBar, SB_SETTEXT, 2 | 0, (LPARAM)"");
        }

        if (pWI->bInsert)
        {
            SendMessage(g_hStatBar, SB_SETTEXT, 3 | 0, (LPARAM)"INS");
        }
        else
        {
            SendMessage(g_hStatBar, SB_SETTEXT, 3 | 0, (LPARAM)"OVR");
        }

        key = GetKeyState(VK_NUMLOCK);
        if (key & 0x1)
        {
            SendMessage(g_hStatBar, SB_SETTEXT, 4 | 0, (LPARAM)"NUM");
        }
        else
        {
            SendMessage(g_hStatBar, SB_SETTEXT, 4 | 0, (LPARAM)"");
        }
    }
}

//-----------------------------------------------------------------------------
// Name: LoadFileIntoNewWindow()
// Desc:
//-----------------------------------------------------------------------------
int LoadFileIntoNewWindow(char *szFileName)
{
    int iRet = 0;

    g_hCurChild = NULL;

    HandleIDC_NEW(g_hMDIClient);

    while (!g_hCurChild);

    WndInfo *pWI = (WndInfo *)GetWindowLong(g_hCurChild, WL_INFOPTR);

    if (pWI)
    {
        if (LoadFile(pWI->hRichEd, szFileName))
        {
            lstrcpy(pWI->szFileName, szFileName);
            HighlightDoc(pWI->hRichEd);
            pWI->bIsDirty = false;
            SetWindowText(g_hCurChild, pWI->szFileName);
            SetFocus(pWI->hRichEd);
            iRet = 1;
        }
    }

    return iRet;
}

//-----------------------------------------------------------------------------
// Name: GetAsmFileName()
// Desc:
//-----------------------------------------------------------------------------
int GetAsmFileName(HWND hWnd, char *szFileTitle, char *szFileName)
{
    OPENFILENAME of;

    ZeroMemory(&of, sizeof (OPENFILENAME));

    of.lStructSize = sizeof (OPENFILENAME);
    of.hwndOwner = hWnd;
    of.hInstance = g_hInst;
    of.lpstrFilter = "ASM files (*.asm)\0*.asm\0"
                    "ALL files (*.*)\0*.*\0\0";
    of.lpstrFile = szFileName;
    of.nMaxFile = MAX_PATH;
    of.lpstrFileTitle = szFileTitle;
    of.nMaxFileTitle = MAX_PATH;
    of.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    return GetOpenFileName(&of);
}



//-----------------------------------------------------------------------------
// Name: ()
// Desc:
//-----------------------------------------------------------------------------


