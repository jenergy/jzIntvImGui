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

extern HINSTANCE g_hInst;

//-----------------------------------------------------------------------------
// Name: FilePane()
// Desc:
//-----------------------------------------------------------------------------
FilePane::FilePane(HWND hWnd)
{
    m_hParent = hWnd;
    CreateTreeView();
    CreateImageList();
    LoadDefaultPane();
}

//-----------------------------------------------------------------------------
// Name: ~FilePane()
// Desc:
//-----------------------------------------------------------------------------
FilePane::~FilePane()
{
    DeleteNodes(m_hTreeRoot);
}

//-----------------------------------------------------------------------------
// Name: DeleteNodes()
// Desc: Delete memory allocated for each item
//-----------------------------------------------------------------------------
void FilePane::DeleteNodes(HTREEITEM hRoot)
{
    HTREEITEM hItem;

    hItem = TreeView_GetChild(m_hTree, hRoot);

    while (hItem != NULL)
    {
        DeleteNodes(hItem);

        hItem = TreeView_GetChild(m_hTree, hRoot);
    }

    TVITEM tv;

    ZeroMemory(&tv, sizeof (TVITEM));
    tv.mask = TVIF_PARAM;
    tv.hItem = hRoot;
    TreeView_GetItem(m_hTree, &tv);

    ItemParam *pItem = (ItemParam *)tv.lParam;

    if (pItem != NULL)
    {
        delete pItem;
    }

    TreeView_DeleteItem(m_hTree, hRoot);
}

//-----------------------------------------------------------------------------
// Name: LoadDefaultPane()
// Desc:
//-----------------------------------------------------------------------------
int FilePane::LoadDefaultPane()
{
    m_bHaveProject = false;

    DeleteNodes(m_hTreeRoot);

    m_hTreeRoot = AddItem(TVI_ROOT, "no project", FP_PROJECT, NULL, false);
    m_hTreeSource = AddItem(m_hTreeRoot, "Source Files", FP_FOLDER, NULL, false);
    m_hTreeInclude = AddItem(m_hTreeRoot, "Include Files", FP_FOLDER, NULL, false);
    m_hTreeOther = AddItem(m_hTreeRoot, "Other Files", FP_FOLDER, NULL, false);
    return 1;
}

//-----------------------------------------------------------------------------
// Name: LoadProject()
// Desc:
//-----------------------------------------------------------------------------
int FilePane::LoadProject(const char *szPath)
{
    Token t(";");
    string s;
    int iRet = 0;
    ItemParam ip;

    if (m_bHaveProject)
    {
        LoadDefaultPane();
    }

    ifstream inFile(szPath);

    if (!inFile.fail())
    {
        while (!inFile.eof())
        {
            getline(inFile, s);
            t = s;
            if (t.GetNumTokens() > 2)
            {
                if (stricmp_(t[0], "source") == 0)
                {
                    ip.sName = t[1];
                    ip.sPath = t[2];
                    AddItem(m_hTreeSource, t[1], FP_SOURCE, &ip);
                }
                else if (stricmp_(t[0], "include") == 0)
                {
                    ip.sName = t[1];
                    ip.sPath = t[2];
                    AddItem(m_hTreeInclude, t[1], FP_INCLUDE, &ip);
                }
                else if (stricmp_(t[0], "other") == 0)
                {
                    ip.sName = t[1];
                    ip.sPath = t[2];
                    AddItem(m_hTreeOther, t[1], FP_OTHER, &ip);
                }
                else if (stricmp_(t[0], "name") == 0)
                {
                    ip.sName = t[1];
                    ip.sPath = t[2];
                    UpdateItem(m_hTreeRoot, t[1], &ip);
                }
            }
        }

        RedrawWindow(m_hTree, NULL, NULL, RDW_INVALIDATE);
        m_bHaveProject = true;
        iRet = 1;
    }

    return iRet;
}

//-----------------------------------------------------------------------------
// Name: LoadProject()
// Desc:
//-----------------------------------------------------------------------------
int FilePane::LoadProject(string &sPath)
{
    return LoadProject(sPath.c_str());
}

//-----------------------------------------------------------------------------
// Name: AddSourceFile()
// Desc:
//-----------------------------------------------------------------------------
int FilePane::AddSourceFile(string &sName, string &sPath)
{
    ItemParam ip;

    ip.sName = sName;
    ip.sPath = sPath;
    AddItem(m_hTreeSource, sName, FP_SOURCE, &ip);

    WriteProject();

    return 1;
}

//-----------------------------------------------------------------------------
// Name: AddSourceFile()
// Desc:
//-----------------------------------------------------------------------------
int FilePane::AddSourceFile(const char *szName, const char *szPath)
{
    ItemParam ip;

    ip.sName = szName;
    ip.sPath = szPath;
    AddItem(m_hTreeSource, szName, FP_SOURCE, &ip);

    WriteProject();

    return 1;
}

//-----------------------------------------------------------------------------
// Name: AddIncludeFile()
// Desc:
//-----------------------------------------------------------------------------
int FilePane::AddIncludeFile(string &sName, string &sPath)
{
    ItemParam ip;

    ip.sName = sName;
    ip.sPath = sPath;
    AddItem(m_hTreeInclude, sName, FP_INCLUDE, &ip);

    WriteProject();

    return 1;
}

//-----------------------------------------------------------------------------
// Name: AddIncludeFile()
// Desc:
//-----------------------------------------------------------------------------
int FilePane::AddIncludeFile(const char *szName, const char *szPath)
{
    ItemParam ip;

    ip.sName = szName;
    ip.sPath = szPath;
    AddItem(m_hTreeInclude, szName, FP_INCLUDE, &ip);

    WriteProject();

    return 1;
}

//-----------------------------------------------------------------------------
// Name: AddOtherFile()
// Desc:
//-----------------------------------------------------------------------------
int FilePane::AddOtherFile(string &sName, string &sPath)
{
    ItemParam ip;

    ip.sName = sName;
    ip.sPath = sPath;
    AddItem(m_hTreeOther, sName, FP_OTHER, &ip);

    WriteProject();

    return 1;
}

//-----------------------------------------------------------------------------
// Name: AddOtherFile()
// Desc:
//-----------------------------------------------------------------------------
int FilePane::AddOtherFile(const char *szName, const char *szPath)
{
    ItemParam ip;

    ip.sName = szName;
    ip.sPath = szPath;
    AddItem(m_hTreeOther, szName, FP_OTHER, &ip);

    WriteProject();

    return 1;
}

//-----------------------------------------------------------------------------
// Name: AddItem()
// Desc:
//-----------------------------------------------------------------------------
HTREEITEM FilePane::AddItem(HTREEITEM hParent, const char *szItem, int iType, ItemParam *pIP, bool bSort)
{
    TVINSERTSTRUCT tvi;

    char szBuff[MAX_PATH];

    lstrcpy(szBuff, szItem);

    ZeroMemory(&tvi, sizeof (TVINSERTSTRUCT));
    tvi.hParent = hParent;
    if (bSort)
    {
        tvi.hInsertAfter = TVI_SORT;
    }
    else
    {
        tvi.hInsertAfter = TVI_LAST;
    }
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvi.item.pszText = szBuff;
    tvi.item.cchTextMax = MAX_PATH;

    if (pIP)
    {
        tvi.item.lParam = (LPARAM)(new ItemParam);
        ((ItemParam *)(tvi.item.lParam))->sName = pIP->sName;
        ((ItemParam *)(tvi.item.lParam))->sPath = pIP->sPath;
    }

    if ((iType >= 0) && (iType < FP_SIZE))
    {
        tvi.item.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvi.item.iImage = m_anImages[iType];
        tvi.item.iSelectedImage = m_anImages[iType];
    }

    HTREEITEM hItem = TreeView_InsertItem(m_hTree, &tvi);

    RedrawWindow(m_hTree, NULL, NULL, RDW_INVALIDATE);

    return hItem;
}

//-----------------------------------------------------------------------------
// Name: AddItem()
// Desc:
//-----------------------------------------------------------------------------
HTREEITEM FilePane::AddItem(HTREEITEM hParent, string &sItem, int iType, ItemParam *pIP, bool bSort)
{
    return AddItem(hParent, sItem.c_str(), iType, pIP, bSort);
}

//-----------------------------------------------------------------------------
// Name: UpdateItem()
// Desc:
//-----------------------------------------------------------------------------
int FilePane::UpdateItem(HTREEITEM hItem, const char *szItem, ItemParam *pIP)
{
    TVITEM tvi;
    ItemParam *pTemp;

    char szBuff[MAX_PATH];
    lstrcpy(szBuff, szItem);

    ZeroMemory(&tvi, sizeof (TVITEM));

    tvi.mask = TVIF_HANDLE | TVIF_TEXT;
    tvi.hItem = hItem;
    tvi.pszText = szBuff;
    tvi.cchTextMax = MAX_PATH;

    if (pIP)
    {
        // Check to see if memory has already been allocated
        TVITEM tv;
        ZeroMemory(&tv, sizeof (TVITEM));
        tv.mask = TVIF_PARAM;

        tv.hItem = hItem;
        TreeView_GetItem(m_hTree, &tv);
        pTemp = (ItemParam *)tv.lParam;

        // If not, allocate it
        if (!pTemp)
        {
            tvi.lParam = (LPARAM)(new ItemParam);
        }

        tvi.mask |= TVIF_PARAM;
        ((ItemParam *)(tvi.lParam))->sName = pIP->sName;
        ((ItemParam *)(tvi.lParam))->sPath = pIP->sPath;
    }

    return TreeView_SetItem(m_hTree, &tvi);
}

//-----------------------------------------------------------------------------
// Name: CreateImageList()
// Desc:
//-----------------------------------------------------------------------------
void FilePane::CreateImageList()
{
    HBITMAP hBit;
    HBITMAP hBitMask;

    m_hImgList = ImageList_Create(16, 16, ILC_MASK, FP_SIZE, 0);

    hBit = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_PROJECT));
    hBitMask = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_PROJECT1));
    m_anImages[FP_PROJECT] = ImageList_Add(m_hImgList, hBit, hBitMask);
    DeleteObject(hBit);
    DeleteObject(hBitMask);

    hBit = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_SOURCE));
    hBitMask = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_SOURCE1));
    m_anImages[FP_SOURCE] = ImageList_Add(m_hImgList, hBit, hBitMask);
    DeleteObject(hBit);
    DeleteObject(hBitMask);

    hBit = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_INCLUDE));
    hBitMask = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_INCLUDE1));
    m_anImages[FP_INCLUDE] = ImageList_Add(m_hImgList, hBit, hBitMask);
    DeleteObject(hBit);
    DeleteObject(hBitMask);

    hBit = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_FOLDER));
    hBitMask = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_FOLDER1));
    m_anImages[FP_FOLDER] = ImageList_Add(m_hImgList, hBit, hBitMask);
    DeleteObject(hBit);
    DeleteObject(hBitMask);

    hBit = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_OTHER));
    hBitMask = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_OTHER1));
    m_anImages[FP_OTHER] = ImageList_Add(m_hImgList, hBit, hBitMask);
    DeleteObject(hBit);
    DeleteObject(hBitMask);

    TreeView_SetImageList(m_hTree, m_hImgList, TVSIL_NORMAL);
}

//-----------------------------------------------------------------------------
// Name: CreateTreeView()
// Desc:
//-----------------------------------------------------------------------------
int FilePane::CreateTreeView()
{
    m_hTree = CreateWindowEx(   WS_EX_WINDOWEDGE,
                                WC_TREEVIEW,
                                NULL,
                                WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER |
                                TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                m_hParent,
                                (HMENU)NULL,
                                GetModuleHandle(NULL),
                                0);

    if (m_hTree)
    {
        ShowWindow(m_hTree, SW_SHOWNORMAL);
        return 1;
    }
    return 0;
}

//-----------------------------------------------------------------------------
// Name: Resize()
// Desc:
//-----------------------------------------------------------------------------
int FilePane::Resize(RECT &rc)
{
    return Resize(rc.left, rc.top, rc.right, rc.bottom);
}

//-----------------------------------------------------------------------------
// Name: Resize()
// Desc:
//-----------------------------------------------------------------------------
int FilePane::Resize(int x, int y, int cx, int cy)
{
    return MoveWindow(m_hTree, x, y, cx, cy, TRUE);
}

//-----------------------------------------------------------------------------
// Name: GetCurSelPath()
// Desc:
//-----------------------------------------------------------------------------
int FilePane::GetCurSelPath(char *szBuff, int iMax)
{
    int iRet = 0;

    HTREEITEM hCur = TreeView_GetSelection(m_hTree);

    if (hCur &&
        (hCur != m_hTreeRoot) &&
        (hCur != m_hTreeSource) &&
        (hCur != m_hTreeInclude))
    {
        // Get the info structure
        TVITEM tv;

        ZeroMemory(&tv, sizeof (TVITEM));
        tv.mask = TVIF_PARAM;
        tv.hItem = hCur;
        TreeView_GetItem(m_hTree, &tv);

        ItemParam *pItem = (ItemParam *)tv.lParam;

        if (pItem != NULL)
        {
            if (iMax > pItem->sPath.length())
            {
                lstrcpy(szBuff, pItem->sPath.c_str());
                iRet = 1;
            }
        }
    }

    return iRet;
}

//-----------------------------------------------------------------------------
// Name: NewProject()
// Desc:
//-----------------------------------------------------------------------------
int FilePane::NewProject(string &sName, string &sPath)
{
    int iRet = 0;

    if (sPath[sPath.length() - 1] != '\\')
    {
        sPath += '\\';
    }

    string sFile = sPath;
    sFile += sName;
    sFile += ".imp";

    CreateDirectory(sPath.c_str(), NULL);

    ofstream outFile(sFile.c_str());

    if (!outFile.eof())
    {
        outFile << "name;" << sName << ";" << sFile << endl;
        outFile.close();

        if (LoadProject(sFile))
        {
            iRet = 1;
        }
    }

    return iRet;
}

//-----------------------------------------------------------------------------
// Name: NewProject()
// Desc:
//-----------------------------------------------------------------------------
int FilePane::NewProject(const char *szName, const char *szPath)
{
    string sName = szName;
    string sPath = szPath;
    return NewProject(sName, sPath);
}

//-----------------------------------------------------------------------------
// Name: RemoveSelected()
// Desc:
//-----------------------------------------------------------------------------
int FilePane::RemoveSelected()
{
    int iRet = 0;

    HTREEITEM hCur = TreeView_GetSelection(m_hTree);

    if (hCur &&
        (hCur != m_hTreeRoot) &&
        (hCur != m_hTreeSource) &&
        (hCur != m_hTreeInclude))
    {
        // Get the info structure
        TVITEM tv;

        ZeroMemory(&tv, sizeof (TVITEM));
        tv.mask = TVIF_PARAM;
        tv.hItem = hCur;
        TreeView_GetItem(m_hTree, &tv);

        ItemParam *pItem = (ItemParam *)tv.lParam;

        if (pItem)
        {
            delete pItem;
        }

        TreeView_DeleteItem(m_hTree, hCur);
        WriteProject();
        RedrawWindow(m_hTree, NULL, NULL, RDW_INVALIDATE);
    }

    return iRet;
}


//-----------------------------------------------------------------------------
// Name: WriteProject()
// Desc:
//-----------------------------------------------------------------------------
int FilePane::WriteProject()
{
    ItemParam *pIP;
    TVITEM tv;
    HTREEITEM hItem;

    ZeroMemory(&tv, sizeof (TVITEM));
    tv.mask = TVIF_PARAM;

    // First get the root item
    tv.hItem = m_hTreeRoot;
    TreeView_GetItem(m_hTree, &tv);
    pIP = (ItemParam *)tv.lParam;

    if (pIP)
    {
        ofstream outFile(pIP->sPath.c_str());

        // Output Root item data
        if (!outFile.fail())
        {
            outFile << "name;" << pIP->sName << ";" << pIP->sPath << endl;

            // Get Source items
            hItem = TreeView_GetChild(m_hTree, m_hTreeSource);
            while (hItem)
            {
                tv.hItem = hItem;
                TreeView_GetItem(m_hTree, &tv);
                pIP = (ItemParam *)tv.lParam;

                // Output item data
                if (pIP)
                {
                    outFile << "source;" << pIP->sName << ";" << pIP->sPath << endl;
                }

                hItem = TreeView_GetNextSibling(m_hTree, hItem);
            }

            // Get Include items
            hItem = TreeView_GetChild(m_hTree, m_hTreeInclude);
            while (hItem)
            {
                tv.hItem = hItem;
                TreeView_GetItem(m_hTree, &tv);
                pIP = (ItemParam *)tv.lParam;

                // Output item data
                if (pIP)
                {
                    outFile << "include;" << pIP->sName << ";" << pIP->sPath << endl;
                }

                hItem = TreeView_GetNextSibling(m_hTree, hItem);
            }

            // Get Other items
            hItem = TreeView_GetChild(m_hTree, m_hTreeOther);
            while (hItem)
            {
                tv.hItem = hItem;
                TreeView_GetItem(m_hTree, &tv);
                pIP = (ItemParam *)tv.lParam;

                // Output item data
                if (pIP)
                {
                    outFile << "other;" << pIP->sName << ";" << pIP->sPath << endl;
                }

                hItem = TreeView_GetNextSibling(m_hTree, hItem);
            }
        }
    }

    return 1;
}




//-----------------------------------------------------------------------------
// Name: ()
// Desc:
//-----------------------------------------------------------------------------
