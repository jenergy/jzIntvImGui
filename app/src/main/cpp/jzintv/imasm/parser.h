/*

IMASM Macro Precompiler

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

#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include <string>
#include <list>
#include <map>

#include "strfifo.h"

int stricmp_(const char *str1, const char *str2);

enum
{
    mPAREN,
    mNOPAREN,
    mUNKNOWN
};

typedef struct _tag_symbol
{
    std::string sName;
    int iVal;
    int iSize;
} symbol;

typedef struct _tag_macro
{
    std::string sName;
    int iType;
    std::vector<std::string> argVector;
    std::vector<std::string> codeVector;
} macro;

struct Parser_ltstr
{
    bool operator()(const char* s1, const char* s2) const
    {
        return strcmp(s1, s2) < 0;
    }
};

class Parser
{
    public:
        Parser(StringFIFO *iFIFO, StringFIFO *oFIFO) :
            inputFIFO (iFIFO),
            outputFIFO(oFIFO), num_expansions(0) { }

        ~Parser();

        int ParseSourceFile();
        int ParseUntilOutput();
        char *GetErrorStr() {return m_szError;}

        struct ParseError
        {
            std::string msg;
            ParseError(std::string &m) : msg(m) { }
        };


    private:

        StringFIFO *inputFIFO, *outputFIFO;
        int num_expansions;

        void PutString(std::string &s, bool addNl = true,
                       IgnoreFlags Ignore = {false,false});
        void PutStringAsCmt(std::string &s, bool addNl = true,
                            IgnoreFlags Ignore = {false,false});

        int ParseLine(std::string &s);
        int ReadMacro(macro *pMacro, std::string &szLine);
        int ExpandMacro(macro *pMacro, std::string &sLine,
                        std::string &sTargetString);
        int ThrowError(const char *format, ...);
        int ThrowWarning(const char *format, ...);
        int FindMacros(std::string &sLine, std::string &sOut);
        int StripReturn(std::string &s);
        int StripReturn(char *s);

        macro *GetMacroPtr(const char *macName);
        int GetMacroString(macro *pMacro, std::string &sLine,
                           std::string &sOut);
        int GetMacroArgs(macro *pMacro, std::string &sMacro);
        int GetMacroArgs(std::string &sMacName, std::string &sMacro,
                         std::vector<std::string> &argVector, int iType);

        std::map<const char *, macro *, Parser_ltstr> m_macroMap;
        char m_szError[MAX_PATH];
};

#endif

