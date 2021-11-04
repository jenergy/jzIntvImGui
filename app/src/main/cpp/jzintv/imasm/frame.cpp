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

extern bool g_bAutoUpdate;
extern HINSTANCE g_hInst;
extern HWND g_hToolBar;
extern HWND g_hStatBar;
extern char g_szCmdLineFile[MAX_PATH];

HWND g_hCurChild = NULL;
HWND g_hMDIClient = NULL;

const int iNumMenus = 7;
PopupStruct g_aPopup[iNumMenus];

FilePane *g_pFP = NULL;

RegHelp g_reg(HKEY_CURRENT_USER);

//-----------------------------------------------------------------------------
// NAME: FrameProc(HWND, unsigned, WORD, LONG)
// DESCRIPTION:  Processes messages for the main window.
//-----------------------------------------------------------------------------
LRESULT CALLBACK FrameProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE: // This happens when window is first created
        {
            CLIENTCREATESTRUCT ccs;

            // Open the registry key we will be using
            g_reg.OpenKey(IMASM_REGISTRY_KEY);

            // Retrieve the handle to the window menu and assign the
            // first child window identifier.
            ccs.hWindowMenu = GetSubMenu(GetMenu(hWnd), 3);
            ccs.idFirstChild = IDC_CHILDBASE;

            // Create the MDI client window.
            g_hMDIClient = CreateWindowEx(0,
                                        "MDICLIENT",
                                        (LPCTSTR)NULL,
                                        WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL,
                                        CW_USEDEFAULT,
                                        CW_USEDEFAULT,
                                        CW_USEDEFAULT,
                                        CW_USEDEFAULT,
                                        hWnd,
                                        (HMENU)IDC_CLIENT,
                                        g_hInst,
                                        (LPVOID)&ccs);
            ShowWindow(g_hMDIClient, SW_SHOW);

            // Create the file pane window
            g_pFP = new FilePane(hWnd);

            // Create the status bar
            g_hStatBar = CreateWindowEx(    0,
                                            STATUSCLASSNAME,
                                            NULL,
                                            WS_VISIBLE | WS_CHILD | SBARS_SIZEGRIP,
                                            CW_USEDEFAULT,
                                            CW_USEDEFAULT,
                                            CW_USEDEFAULT,
                                            CW_USEDEFAULT,
                                            hWnd,
                                            (HMENU)IDC_STATUS,
                                            g_hInst,
                                            0);

            CreateToolbar(hWnd);

            if (g_bAutoUpdate)
            {
                SendMessage(g_hToolBar, TB_SETSTATE, (WPARAM)IDC_AUTO, (LPARAM)(TBSTATE_ENABLED | TBSTATE_CHECKED));
            }

            // Fill up the popup structure
            HMENU hMenu = GetMenu(hWnd);
            HMENU hFile = GetSubMenu(hMenu, 0);
            HMENU hFormat = GetSubMenu(hMenu, 1);
            HMENU hProject = GetSubMenu(hMenu, 2);
            HMENU hWindow = GetSubMenu(hMenu, 3);
            HMENU hHelp = GetSubMenu(hMenu, 4);

            g_aPopup[0].hMenu = hMenu;
            g_aPopup[0].uiString = IDS_FILE_MENU;
            g_aPopup[1].hMenu = hFile;
            g_aPopup[1].uiString = IDS_FILE_MENU;
            g_aPopup[2].hMenu = hFormat;
            g_aPopup[2].uiString = IDS_FORMAT_MENU;
            g_aPopup[3].hMenu = hProject;
            g_aPopup[3].uiString = IDS_PROJECT_MENU;
            g_aPopup[4].hMenu = hWindow;
            g_aPopup[4].uiString = IDS_WINDOW_MENU;
            g_aPopup[5].hMenu = hHelp;
            g_aPopup[5].uiString = IDS_HELP_MENU;
            g_aPopup[6].hMenu = 0;
            g_aPopup[6].uiString = 0;

            return 0;
        }// WM_CREATE

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_NEW:
                    HandleIDC_NEW(g_hMDIClient);
                    break;

                case IDC_OPEN:
                    HandleIDC_OPEN(g_hMDIClient);
                    break;

                case IDC_OPENPROJECT:
                    HandleIDC_OPENPROJECT(g_hMDIClient, g_pFP);
                    break;

                case IDC_SAVEAS:
                    if (HandleIDC_SAVEAS(g_hCurChild) == 0)
                    {
                        break;
                    }
                    // FALLTHROUGH:

                case IDC_SAVE:
                    HandleIDC_SAVE(g_hCurChild);
                    break;

                case IDC_FONT:
                    HandleIDC_FONT(g_hMDIClient);
                    break;

                case IDC_EXIT:
                    PostMessage(hWnd, WM_CLOSE, 0, 0);
                    break;

                case IDC_ABOUT:
                    DialogBoxParam( g_hInst,
                                    MAKEINTRESOURCE(IDD_ABOUT),
                                    hWnd,
                                    (DLGPROC)AboutProc,
                                    0);
                    break;

                case IDC_AUTO:
                {
                    DWORD dwState = SendMessage(g_hToolBar, TB_GETSTATE, (WPARAM)IDC_AUTO, 0);

                    if ((dwState & TBSTATE_CHECKED) != 0)
                    {
                        g_bAutoUpdate = true;
                        HighlightDoc(GetDlgItem(g_hCurChild, IDC_MAINRICHED));
                        SetTimer(hWnd, UPDATETIMER, 20000, NULL);
                    }
                    else
                    {
                        g_bAutoUpdate = false;
                        KillTimer(hWnd, UPDATETIMER);
                    }
                    break;
                }

                case IDC_PRECOMPILE:
                    HandleIDC_PRECOMPILE(g_hCurChild);
                    break;

                case IDC_CASCADE:
                    SendMessage(g_hMDIClient, WM_MDICASCADE, MDITILE_SKIPDISABLED, 0);
                    break;

                case IDC_TILEHORIZ:
                    SendMessage(g_hMDIClient, WM_MDITILE, MDITILE_HORIZONTAL | MDITILE_SKIPDISABLED, 0);
                    break;

                case IDC_TILEVERT:
                    SendMessage(g_hMDIClient, WM_MDITILE, MDITILE_VERTICAL | MDITILE_SKIPDISABLED, 0);
                    break;

                case IDC_PROJECTNEW:
                    HandleIDC_PROJECTNEW(hWnd, g_pFP);
                    break;

                case IDC_PROJECTADDSOURCE:
                    HandleIDC_PROJECTADDSOURCE(hWnd, g_pFP);
                    break;

                case IDC_PROJECTADDINCLUDE:
                    HandleIDC_PROJECTADDINCLUDE(hWnd, g_pFP);
                    break;

                case IDC_PROJECTADDOTHER:
                    HandleIDC_PROJECTADDOTHER(hWnd, g_pFP);
                    break;

                case IDC_PROJECTREMOVE:
                    HandleIDC_PROJECTREMOVE(hWnd, g_pFP);
                    break;
            }
            break;

        case WM_NOTIFY:
        {
            LPNMHDR pHdr = (LPNMHDR)lParam;

            switch (pHdr->code)
            {
                case TTN_GETDISPINFO:
                {
                    LPTOOLTIPTEXT lpttt;

                    lpttt = (LPTOOLTIPTEXT)lParam;
                    lpttt->hinst = g_hInst;

                    // Specify the resource identifier of the descriptive
                    // text for the given button.
                    switch (lpttt->hdr.idFrom)
                    {
                        case IDC_NEW:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_NEW);
                            break;
                        case IDC_OPEN:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_OPEN);
                            break;
                        case IDC_OPENPROJECT:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_OPENPROJECT);
                            break;
                        case IDC_SAVE:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_SAVE);
                            break;
                        case IDC_SAVEAS:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_SAVEAS);
                            break;
                        case IDC_AUTO:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_AUTO);
                            break;
                        case IDC_PRECOMPILE:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_PRECOMPILE);
                            break;
                    }
                    break;
                }

                case NM_DBLCLK:
                case NM_RETURN:
                {
                    char szPath[MAX_PATH];

                    if (g_pFP->GetCurSelPath(szPath, MAX_PATH))
                    {
                        LoadFileIntoNewWindow(szPath);
                    }
                    break;
                }

                case TVN_KEYDOWN:
                {
                    NMTVKEYDOWN *pKD;

                    pKD = (NMTVKEYDOWN *)lParam;

                    if (pKD->wVKey == VK_DELETE)
                    {
                        g_pFP->RemoveSelected();
                    }
                    break;
                }

                default:
                    break;
            }
            break;
        }

        case WM_MENUSELECT:
        {
            UINT uFlags = (UINT)HIWORD(wParam);
            HMENU hMain = NULL;
            UINT i = 0;

            if (uFlags & MF_SYSMENU) // Handle non-system popup menus
            {
                MenuHelp(WM_MENUSELECT, wParam, lParam, NULL, g_hInst, g_hStatBar, &i);
            }
            else if (uFlags & MF_POPUP)
            {
                MenuHelp(WM_MENUSELECT, wParam, lParam, GetMenu(hWnd), g_hInst, g_hStatBar, (UINT *)g_aPopup);
            }
            else
            {
                for (i = 1; i < iNumMenus; i++)
                {
                    if ((HMENU)lParam == g_aPopup[i].hMenu)
                    {
                        MenuHelp(WM_MENUSELECT, wParam, lParam, GetMenu(hWnd), g_hInst, g_hStatBar, (LPUINT)&g_aPopup[i].uiString);
                        break;
                    }
                }
            }



            return 0;
        }

        case WM_CLOSE:
            DestroyChildren(g_hMDIClient);
            DestroyWindow(hWnd);
            break;

        case WM_SIZE:  // Get size of client
        {
            int iClientX = LOWORD(lParam);
            int iClientY = HIWORD(lParam);

            SendMessage(g_hToolBar, TB_AUTOSIZE, 0, 0);

            int aiParts[] = {iClientX - 210, iClientX - 110, iClientX - 80, iClientX - 50, iClientX - 20};
            SendMessage(g_hStatBar, SB_SETPARTS, (WPARAM)5, (LPARAM)aiParts);
            SendMessage(g_hStatBar, WM_SIZE, 0, 0);
            UpdateStatusBar(g_hCurChild);

            RECT rc;

            GetWindowRect(g_hToolBar, &rc);

            int cyTool = rc.bottom - rc.top;

            GetWindowRect(g_hStatBar, &rc);

            int cyStat = rc.bottom - rc.top;

            // Now resize the file pane
            int cxFilePane = iClientX / 5;
            g_pFP->Resize(0, cyTool, cxFilePane, iClientY - cyTool - cyStat);

            // And resize the MDI Client window
            MoveWindow(g_hMDIClient, cxFilePane, cyTool, iClientX - cxFilePane, iClientY - cyTool - cyStat, TRUE);

            // Do this on the first WM_SIZE message so the window is sized properly
            if (*g_szCmdLineFile != NUL)
            {
                LoadFileIntoNewWindow(g_szCmdLineFile);
                *g_szCmdLineFile = NUL;
            }

            return 0; // WM_SIZE
        }

        case WM_DESTROY:
            if (g_pFP)
            {
                delete g_pFP;
            }

            DoExitProcessing(hWnd);
            PostQuitMessage(0);
            break; // WM_DESTROY

        default:    // Otherwise, let Windows process message
            break;
   }// end switch

   return DefFrameProc(hWnd, g_hMDIClient, message, wParam, lParam);
}

//-----------------------------------------------------------------------------
// Name: void DestroyChildren(HWND hWnd)()
// Desc:
//-----------------------------------------------------------------------------
void DestroyChildren(HWND hClient)
{
    HWND hWnd = (HWND)SendMessage(hClient, WM_MDIGETACTIVE, 0, 0);
    HWND hLast = NULL;

    while (hWnd != hLast)
    {
        hLast = hWnd;
        ConfirmSave(hWnd);
        SendMessage(hClient, WM_MDIDESTROY, (WPARAM)hWnd, 0);
        hWnd = (HWND)SendMessage(hClient, WM_MDIGETACTIVE, 0, 0);
    }
}

//-----------------------------------------------------------------------------
// Name: ()
// Desc:
//-----------------------------------------------------------------------------
