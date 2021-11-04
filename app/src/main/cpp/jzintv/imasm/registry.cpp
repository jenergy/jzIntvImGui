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

RegHelp::RegHelp(HKEY hRoot)
{
    m_hRoot = hRoot;
}

RegHelp::~RegHelp()
{
    CloseKey();
}

RegHelp::OpenKey(char *szSection)
{
    if (RegCreateKeyEx( m_hRoot,
                        szSection,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &m_hKey,
                        NULL)
        != ERROR_SUCCESS)
    {
        return 0;
    }

    return 1;
}

RegHelp::CloseKey()
{
    if (m_hKey != NULL)
    {
        RegCloseKey(m_hKey);
    }

    return 1;
}

int RegHelp::AddString(char *szSection, char *szBuff)
{
    if (!m_hKey)
    {
        return 0;
    }

    if (RegSetValueEx(  m_hKey,
                        szSection,
                        0,
                        REG_SZ,
                        (const BYTE *)szBuff,
                        lstrlen(szBuff) + 1)
        != ERROR_SUCCESS)
    {
        return 0;
    }

    return 1;
}

int RegHelp::AddDword(char *szSection, DWORD dwVal)
{
    if (!m_hKey)
    {
        return 0;
    }

    if (RegSetValueEx(  m_hKey,
                        szSection,
                        0,
                        REG_DWORD,
                        (const BYTE *)&dwVal,
                        sizeof (DWORD))
        != ERROR_SUCCESS)
    {
        return 0;
    }

    return 1;
}

int RegHelp::GetString(char *szSection, char *szBuff, unsigned long ulSize)
{
    if (!m_hKey)
    {
        return 0;
    }

    unsigned long len = ulSize;

    if (RegQueryValueEx(m_hKey,
                        szSection,
                        NULL,
                        NULL,
                        (LPBYTE)szBuff,
                        &len)
        != ERROR_SUCCESS)
    {
        return 0;
    }

    return 1;
}

int RegHelp::GetDword(char *szSection, DWORD *dwVal)
{
    if (!m_hKey)
    {
        return 0;
    }

    unsigned long len = sizeof (DWORD);

    if (RegQueryValueEx(m_hKey,
                        szSection,
                        NULL,
                        NULL,
                        (LPBYTE)&dwVal,
                        &len)
        != ERROR_SUCCESS)
    {
        return 0;
    }

    return 1;
}
