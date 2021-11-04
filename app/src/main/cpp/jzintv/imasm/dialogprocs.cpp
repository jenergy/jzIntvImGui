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

//-----------------------------------------------------------------------------
// Name: AboutProc()
// Desc: Handles messages for the dialog
//-----------------------------------------------------------------------------
INT_PTR CALLBACK AboutProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_INITDIALOG:
        {
            CenterWindow(hDlg);
            SetCaptionPrintf(GetDlgItem(hDlg, IDS_VERSION), "Version: %s", IMASMVERSION);
            return TRUE;
        }

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    return TRUE;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }

    return FALSE; // Didn't handle message
}

//-----------------------------------------------------------------------------
// Name: NewProjectProc()
// Desc: Handles messages for the dialog
//-----------------------------------------------------------------------------
INT_PTR CALLBACK NewProjectProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static ItemParam *pIP;

    char szBuff[MAX_PATH];
    int iLen;

    switch(msg)
    {
        case WM_INITDIALOG:
            pIP = (ItemParam *)lParam;

            CenterWindow(hDlg);
            GetCurrentDirectory(MAX_PATH-1, szBuff);
            iLen = lstrlen(szBuff);
            if (szBuff[iLen-1] != '\\')
            {
                szBuff[iLen] = '\\';
                szBuff[iLen+1] = NUL;
            }
            SetWindowText(GetDlgItem(hDlg, IDC_PROJECTPATH), szBuff);
            SetFocus(GetDlgItem(hDlg, IDC_PROJECTNAME));
            return TRUE;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_PROJECTNAME:
                    switch (HIWORD(wParam))
                    {
                        case EN_CHANGE:
                            break;
                    }
                    break;

                case IDC_PROJECTPATH:
                    switch (HIWORD(wParam))
                    {
                        case EN_CHANGE:
                            break;
                    }
                    break;

                case IDC_BROWSE:
                    break;

                case IDOK:
                    GetDlgItemText(hDlg, IDC_PROJECTNAME, szBuff, MAX_PATH);
                    if (lstrlen(szBuff) == 0)
                    {
                        MessageBox(hDlg, "Invalid project name specified", "Error", MB_ICONSTOP);
                        break;
                    }
                    pIP->sName = szBuff;

                    GetDlgItemText(hDlg, IDC_PROJECTPATH, szBuff, MAX_PATH);
                    if (lstrlen(szBuff) == 0)
                    {
                        MessageBox(hDlg, "Invalid project path specified", "Error", MB_ICONSTOP);
                        break;
                    }
                    pIP->sPath = szBuff;

                    EndDialog(hDlg, 1);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    return TRUE;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }

    return FALSE; // Didn't handle message
}

