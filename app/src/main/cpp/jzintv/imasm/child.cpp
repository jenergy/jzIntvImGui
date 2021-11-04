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
extern char g_szClass[];
extern HWND g_hCurChild;
extern RegHelp g_reg;

//-----------------------------------------------------------------------------
// Name: ChildProc()
// Desc:
//-----------------------------------------------------------------------------
LRESULT CALLBACK ChildProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WndInfo *pWI;

    switch (message)
    {
        case WM_CREATE: // This happens when window is first created
        {
            pWI = new WndInfo;
            SetWindowLong(hWnd, WL_INFOPTR, (LONG)pWI);

            pWI->kBytes = 200;
            pWI->bInsert = true;

            pWI->hRichEd = CreateWindowEx(  0,
                                            g_szClass,
                                            NULL,
                                            WS_BORDER | WS_VSCROLL | WS_HSCROLL | WS_VISIBLE | WS_CHILD |
                                            ES_MULTILINE | ES_WANTRETURN | ES_AUTOHSCROLL,
                                            CW_USEDEFAULT,
                                            CW_USEDEFAULT,
                                            CW_USEDEFAULT,
                                            CW_USEDEFAULT,
                                            hWnd,
                                            (HMENU)IDC_MAINRICHED,
                                            g_hInst,
                                            0);
            ShowWindow(pWI->hRichEd, SW_SHOWNORMAL);

            // Get and set the font, if required
            char szFont[MAX_PATH];
            if (g_reg.GetString("FontName", szFont, MAX_PATH))
            {
                CHARFORMAT cfmt;
                ZeroMemory(&cfmt, sizeof (CHARFORMAT));

                cfmt.cbSize = sizeof (CHARFORMAT);
                cfmt.dwMask = CFM_FACE;
                lstrcpy(cfmt.szFaceName, szFont);

                SendMessage(pWI->hRichEd, EM_SETCHARFORMAT, (WPARAM)SCF_ALL, (LPARAM)&cfmt);
            }

            if (g_bAutoUpdate)
            {
                SetTimer(hWnd, UPDATETIMER, 20000, NULL);
            }

            // Set the depth of the richedit
            SendMessage(pWI->hRichEd, EM_LIMITTEXT, pWI->kBytes*1024, 0);

            // Setup to get messages
            DragAcceptFiles(pWI->hRichEd, TRUE);
            SendMessage(    pWI->hRichEd,
                            EM_SETEVENTMASK,
                            (WPARAM)0,
                            (LPARAM)(   ENM_KEYEVENTS | ENM_MOUSEEVENTS |
                                        ENM_SCROLLEVENTS | ENM_UPDATE |
                                        ENM_CHANGE | ENM_DROPFILES));

            SetFocus(pWI->hRichEd);
            pWI->bIsDirty = false;

            break;
        }// WM_CREATE

        case WM_MDIACTIVATE:
            g_hCurChild = hWnd;
            break;

        case WM_MDIDESTROY:
            if (g_hCurChild == hWnd)
            {
                g_hCurChild = NULL;
            }
            break;

        case WM_TIMER:
            HighlightDoc(GetDlgItem(hWnd, IDC_MAINRICHED));
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_MAINRICHED:
                    switch (HIWORD(wParam))
                    {
                        case EN_UPDATE:
                            HandleEN_UPDATE((HWND)lParam);
                            break;

                        case EN_CHANGE:
                            pWI = (WndInfo *)GetWindowLong(hWnd, WL_INFOPTR);
                            if (pWI)
                            {
                                pWI->bIsDirty = true;
                                if (*pWI->szFileName != NUL)
                                {
                                    SetCaptionPrintf(hWnd, "%s*", pWI->szFileName);
                                }
                                else
                                {
                                    SetWindowText(hWnd, "untitled*");
                                }
                            }
                            break;
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
        {
            LPNMHDR pHdr = (LPNMHDR)lParam;
            pWI = (WndInfo *)GetWindowLong(hWnd, WL_INFOPTR);

            switch (pHdr->code)
            {
                case EN_DROPFILES:
                    pWI = (WndInfo *)GetWindowLong(hWnd, WL_INFOPTR);

                    if (pWI)
                    {
                        if (pHdr->hwndFrom == pWI->hRichEd)
                        {
                            ENDROPFILES *pED = (ENDROPFILES *)lParam;
                            char szBuff[MAX_PATH];

                            UINT uNum = DragQueryFile((HDROP)pED->hDrop, 0xFFFFFFFF, szBuff, MAX_PATH);

                            if (uNum > 1)
                            {
                                MessageBox(hWnd, "Please only drop one file at a time", "Error", MB_OK);
                            }
                            else
                            {
                                DragQueryFile((HDROP)pED->hDrop, 0, szBuff, MAX_PATH);

                                ConfirmSave(hWnd);
                                lstrcpy(pWI->szFileName, szBuff);
                                LoadFile(pWI->hRichEd, pWI->szFileName);
                                HighlightDoc(pWI->hRichEd);
                            }

                            DragFinish((HDROP)pED->hDrop);
                            return 1;
                        }
                    }
                    break;

                case EN_MSGFILTER:
                {
                    if (g_hCurChild == hWnd)
                    {
                        MSGFILTER *pMF = (MSGFILTER *)lParam;

                        if (pMF->msg == WM_KEYDOWN)
                        {
                            if (pMF->wParam == VK_INSERT)
                            {
                                pWI = (WndInfo *)GetWindowLong(hWnd, WL_INFOPTR);
                                if (pWI)
                                {
                                    pWI->bInsert = !pWI->bInsert;
                                }
                            }
                        }
                        UpdateStatusBar(hWnd);
                    }
                    break;
                }

                default:
                    break;
            }
            break;
        }

        case WM_CLOSE:
            ConfirmSave(hWnd);
            DestroyWindow(hWnd);
            break;

        case WM_SIZE:  // Get size of client
        {
            int iClientX = LOWORD(lParam);
            int iClientY = HIWORD(lParam);

            pWI = (WndInfo *)GetWindowLong(hWnd, WL_INFOPTR);

            if (pWI)
            {
                if (pWI->hRichEd)
                {
                    MoveWindow(pWI->hRichEd, 0, 0, iClientX, iClientY, TRUE);
                }
            }

            break; // WM_SIZE
        }

        case WM_DESTROY:
        {
            WndInfo *pWI = (WndInfo *)GetWindowLong(hWnd, WL_INFOPTR);
            if (pWI)
            {
                delete pWI;
            }
            DoExitProcessing(hWnd);
            break; // WM_DESTROY
        }

        default:    // Otherwise, let Windows process message
            break;
   }// end switch

   return DefMDIChildProc(hWnd, message, wParam, lParam);
}

