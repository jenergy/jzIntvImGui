/*

Token parser class

Copyright (C) 2003  Joe Fisher, Shiny Technologies, LLC
http://www.shinytechnologies.com

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

#include "token.h"
#include "excep.h"

using namespace std;

Token::Token(const char *szSeps)
{
    strcpy(m_szSeps, szSeps);
}

Token::~Token()
{
}

Token &Token::operator =(Token &tok)
{
    m_str = tok.m_str;
    m_tokVector = tok.m_tokVector;
    strcpy(m_szSeps, tok.m_szSeps);

    return *this;
}

Token &Token::operator =(const char *szBuff)
{
    m_str = szBuff;
    Tokenize();

    return *this;
}

Token &Token::operator =(string &sBuff)
{
    m_str = sBuff;
    Tokenize();

    return *this;
}

const char *Token::operator[](size_t i)
{
    return i > m_tokVector.size() ? nullptr : m_tokVector[i].c_str();
}

void Token::Tokenize()
{
    size_t len = m_str.length();
    m_tokVector.clear();

    if (len == 0)
        return;

    char *pStart, *pSep, *szBuff, *ws1, *ws2, *last_nws;
    bool prev_ws, curr_ws;
    szBuff = new char[len + 1];

    // make local copy
    strcpy(szBuff, m_str.c_str());

    // skip leading whitespace
    ws1 = szBuff;
    while (*ws1 && isspace(*ws1))
        ws1++;

    if (!*ws1)  // leave if empty
    {
        delete[] szBuff;
        return;
    }

    // compress whitespace
    for (ws2 = szBuff, last_nws = ws1, prev_ws = false; *ws1; ws1++)
        if ((curr_ws = isspace(*ws1)) == false || !prev_ws)
        {
            if (!curr_ws) last_nws = ws2;
            *ws2++ = *ws1;
            prev_ws = curr_ws;
        }

    last_nws[1] = 0;

    // Now separate into tokens
    pStart = szBuff;
    while ((pSep = strpbrk(pStart, m_szSeps)) != nullptr && *pSep != 0)
    {
        *pSep = 0;                              // Zero-out separator
        m_tokVector.push_back(string(pStart));  // Save up to separator
        pStart = pSep + 1;                      // Start at char after sep.
    }

    // Get trailing token, if any, since end-of-string is an implicit
    // terminator.
    if (*pStart)
        m_tokVector.push_back(string(pStart));  // Save up to separator

    // Nuke our temp string.
    delete[] szBuff;
}

int Token::SetNewSeparators(const char *szSeps)
{
    strcpy(m_szSeps, szSeps);
    Tokenize();

    return 1;
}

int Token::Tokenize(const char *szSeps)
{
    char szTemp[255];

    if (strlen(szSeps) > 254)
        throw InternalError("Token::Tokenize", "szSeps too long");

    strcpy(szTemp, m_szSeps);
    strcpy(m_szSeps, szSeps);
    Tokenize();
    strcpy(m_szSeps, szTemp);

    return 1;

}

