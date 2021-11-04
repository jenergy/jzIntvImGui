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

#ifndef TOKEN_H_INCLUDED
#define TOKEN_H_INCLUDED

#include <string>
#include <vector>

class Token
{
public:
    Token(const char *);
    ~Token();

    Token &operator =(Token &tok);
    Token &operator =(const char *szBuff);
    Token &operator =(std::string &sBuff);

    const char *operator[](size_t i);

    int SetNewSeparators(const char *);
    int Tokenize(const char *);

    int GetNumTokens() {return m_tokVector.size();}

private:
    void Tokenize();

    std::string m_str;
    std::vector<std::string> m_tokVector;
    char m_szSeps[255];
};

#endif

