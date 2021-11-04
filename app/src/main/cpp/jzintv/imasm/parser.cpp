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
#include <cstdarg>
#include <cstdlib>

using namespace std;

static const IgnoreFlags IgnoreFully  = { true,  true  };
static const IgnoreFlags IgnoreNone   = { false, false };

#define NUL '\0'

#if defined(WIN32X) || !defined(WIN32)

int stricmp_(const char *str1, const char *str2)
{
    while (tolower(*str1) == tolower(*str2))
    {
        if (*str1 == NUL)
        {
            return 0;
        }

        str1++;
        str2++;
    }

    if (tolower(*str1) > tolower(*str2))
    {
        return 1;
    }

    return -1;
}

#elif defined(USE_STRCASECMP)

int stricmp_(const char *str1, const char *str2)
{
    return strcasecmp(str1, str2);
}

#else

int stricmp_(const char *str1, const char *str2)
{
    return stricmp(str1, str2);
}

#endif

Parser::~Parser()
{
    map<const char *, macro *, Parser_ltstr>::iterator itr;

    for (itr = m_macroMap.begin(); itr != m_macroMap.end(); itr++)
    {
        free(static_cast<void *>(const_cast<char *>(itr->first)));
        delete(itr->second);
    }

    m_macroMap.clear();
}


int Parser::ParseSourceFile()
{
    string s;
    IgnoreFlags Ignore;

    while (inputFIFO->getLine(s, Ignore))
    {
        StripReturn(s);

        if (Ignore.skip_macro)
        {
            PutString(s, true, IgnoreNone);
            continue;
        }

        if (ParseLine(s) == -1)
            return 0;
    }

    return 1;
}

int Parser::ParseUntilOutput()
{
    string s;
    IgnoreFlags Ignore;

    while (outputFIFO->isEmpty() &&
           inputFIFO->getLine(s, Ignore))
    {
        StripReturn(s);

        if (Ignore.skip_macro)
        {
            PutString(s, true, IgnoreNone);
            continue;
        }

        if (ParseLine(s) == -1)
            return 0;
    }

    return 1;
}

void Parser::PutString(string &s, bool addNL, IgnoreFlags Ignore)
{
    string sTmp = s;
    string sSub;
    string::size_type crPos;
    string::size_type nlPos;

    if (addNL) sTmp += "\n";

    // Nuke any CRs we find (from straggling CR+LF pairs)
    while ((crPos = sTmp.find('\r')) != string::npos)
        sTmp.erase(crPos, 1);

    // Now chop into newline-terminated substrings
    while ((nlPos = sTmp.find('\n')) != string::npos)
    {
        sSub = sTmp.substr(0, nlPos + 1);
        outputFIFO->putLine(sSub.c_str(), Ignore);

        sTmp.erase(0, nlPos + 1);
    }

    if (sTmp.length() > 0)
        outputFIFO->putLine(sTmp.c_str(), Ignore);
}

void Parser::PutStringAsCmt(string &s, bool addNL, IgnoreFlags Ignore)
{
    string sTmp = s;
    string sSub;
    string::size_type crPos;
    string::size_type nlPos;

    if (addNL) sTmp += "\n";

    // Nuke any CRs we find (from straggling CR+LF pairs)
    while ((crPos = sTmp.find('\r')) != string::npos)
        sTmp.erase(crPos, 1);

    // Now chop into newline-terminated substrings
    while ((nlPos = sTmp.find('\n')) != string::npos)
    {
        sSub = sTmp.substr(0, nlPos + 1);

        // Convert original line into a comment.
        // If the leading character is a space, replace it.  Otherwise, if
        // it's anything else (including a TAB), insert in front of it.
        if (sSub.length() > 0 && sSub[0] == ' ')
            sSub.replace(0,1, ";");
        else
            sSub.insert(0, ";");

        outputFIFO->putLine(sSub.c_str(), Ignore);

        sTmp.erase(0, nlPos + 1);
    }

    if (sTmp.length() > 0)
        outputFIFO->putLine(sTmp.c_str(), Ignore);
}

int Parser::ParseLine(string &sLine)
{
    Token lineToks(", \t()\r\n");
    bool bHandled = false;
    string sOrig = sLine;
    int iRet;

    // Massacre any comments to keep them from mucking up the works
    string::size_type iPos = sLine.find(";");
    if (iPos != string::npos)
        sLine.substr(0, iPos);

    // Now, through the magic of operator overloading, tokenize the line.
    lineToks = sLine;

    // If we found any tokens, see if any of them are meaningful.
    if (lineToks.GetNumTokens() > 0)
    {
        if (stricmp_(lineToks[0], "MACRO") == 0) // macro definition
        {
            macro *m = new macro;
            bool redef_err = false;

            // Output original line as a comment
            PutString(sOrig, true, IgnoreFully);

            redef_err = (m_macroMap.find(lineToks[1]) != m_macroMap.end());

            m->sName = lineToks[1];

            if (ReadMacro(m, sLine) == -1)
                return -1;

            if (!redef_err)
                m_macroMap[strdup(lineToks[1])] = m;
            else
                ThrowError("MACRO '%s' redefined!", lineToks[1]);

            bHandled = true;
        }
        else // Test to see if we have a macro match
        {
            string sMacro;

            iRet = FindMacros(sLine, sMacro);

            if (iRet == -1)
            {
                return -1;
            }
            else if (iRet == 1)
            {
                // Found a macro, so output the original line as a comment
                // followed by the expanded line.
                PutStringAsCmt(sOrig, true, IgnoreNone);
                if (sMacro.length() > 0)
                    PutString(sMacro);

                bHandled = true;
            }
            // else if iRet == 0, pass the line through.
        }

    }

    if (!bHandled)
    {
        PutString(sOrig);
    }

    return 1;
}

int Parser::ReadMacro(macro *pMacro, string &sLine)
{
    string s, sTemp;
    Token t(" \t\r\n");
    bool ok;
    IgnoreFlags Ignore;

    pMacro->iType = mUNKNOWN;

    s = sLine;

    if (GetMacroArgs(pMacro, s) == -1)
    {
        return -1;
    }

    s.erase();

    // Read the first line of the macro
    do {
        ok = inputFIFO->getLine(s, Ignore);
        if (Ignore.skip_macro)
            PutString(s, false, IgnoreNone);  // XXX: Why IgnoreNone?
    } while (ok && Ignore.skip_macro);
    if (!ok && inputFIFO->isEOF())
    {
        ThrowError("End of file encountered while reading macro %s",
                   pMacro->sName.c_str());
        return -1;
    }
    PutString(s, false, IgnoreFully);

    // Strip leading whitespace and trailing return character.
    string::size_type iPos = 0, len = s.length();
    while (iPos < len && isspace(s[iPos]))
        iPos++;
    s.erase(0, iPos);

    StripReturn(s);

    t = s;

    if (t.GetNumTokens() == 0 || stricmp_(t[0], "ENDM") != 0)
    {
        pMacro->codeVector.push_back(s);
    }
    else
    {
        ThrowWarning("Empty macro %s defined.", pMacro->sName.c_str());
        return 1;
    }


    // Read the rest of the macro
    while (1)
    {
        do {
            ok = inputFIFO->getLine(s, Ignore);
            if (Ignore.skip_macro)
                PutString(s, false, IgnoreNone);
        } while (ok && Ignore.skip_macro);

        if (!ok && inputFIFO->isEOF())
        {
            ThrowError("End of file encountered while reading macro %s",
                       pMacro->sName.c_str());
            return -1;
        }

        PutString(s, false, IgnoreFully);
        StripReturn(s);
        t = s;

        if (t.GetNumTokens() > 0)
        {
            if (stricmp_(t[0], "ENDM") != 0)
            {
                pMacro->codeVector.push_back(s);
            }
            else
            {
                break;
            }
        }
    }

    return 1;
}

int Parser::ThrowError(const char *format, ...)
{
    char msg_buf[1024], loc_buf[16];
    va_list args;
    va_start(args, format);
    vsprintf(msg_buf, format, args);

    const StringFIFO::loc *inLoc = inputFIFO->getLocation();
    sprintf(loc_buf, ":%d: ", inLoc->lineNo);

    string msg = string(inLoc->fname) + loc_buf + "ERROR - " + msg_buf;

    throw ParseError(msg);
}

int Parser::ThrowWarning(const char *format, ...)
{
    char msg_buf[256], loc_buf[16];
    va_list args;
    va_start(args, format);
    vsprintf(msg_buf, format, args);

    const StringFIFO::loc *inLoc = inputFIFO->getLocation();
    sprintf(loc_buf, ":%d: ", inLoc->lineNo);
    cerr << inLoc->fname << loc_buf << "WARNING - " << msg_buf << endl;

    return 1;
}

int Parser::GetMacroString(macro *pMacro, string &sLine, string &sOut)
{
    int iRet = 0;
    string sMacName;
    sMacName = pMacro->sName;

    string::size_type iPos = sLine.find(sMacName);

    if (iPos != string::npos)
    {
        int iParen = 0;

        char c;

        while (iPos < sLine.length())
        {
            c = sLine[iPos];
            sOut += c;

            if (pMacro->iType == mPAREN)
            {
                if (c == '(')
                {
                    iParen++;
                }
                else if (c == ')')
                {
                    iParen--;

                    if (iParen == 0)
                    {
                        break;
                    }
                }
            }

            iPos++;
        }

        if (iParen == 0)
        {
            iRet = 1;
        }
        else
        {
            ThrowError("Mismatched parenthesis in macro: %s", sMacName.c_str());
        }
    }

    // Eat trailing whitespace
    iPos = sOut.length();

    if (iPos > 0)
    {
        while (iPos > 0 && isspace(sOut[--iPos]))
            ;

        sOut = sOut.substr(0, iPos+1);
    }

    return iRet;
}

// Macros should be found and parsed in a left to right order
int Parser::FindMacros(string &sLine, string &sOut)
{
    int iRet = 0, i, iNumToks;
    Token t(" \t,()\r\n#$");
    macro *pMac = nullptr;
    string sSub;
    string::size_type iPos, iEnd;

    string sOrig = sLine, sCurStr = sLine, sTmpStr, sCmtStr;

    bool bChanged;

    sOut.erase();

    // Take only the first line if the input is multiple lines,
    // and save the rest in sTmpStr.
    if ((iPos = sCurStr.find('\n')) != string::npos)
    {
        sTmpStr = sCurStr.substr(iPos);
        sCurStr.erase(0, iPos);
    } else
        sTmpStr = "";


    // Hide comments from the rest of the parser
    iPos = sCurStr.find(';');
    if (iPos != string::npos)
    {
        sCmtStr = sCurStr.substr(iPos);
        sCurStr.erase(iPos);
    } else
    {
        sCmtStr.erase();
    }

    // Tokenize it and expand all macros in the line.
    // A macro may expand to multiple lines.
    bChanged = false;
    pMac     = nullptr;
    t        = sCurStr;
    iNumToks = t.GetNumTokens();

    for (i = 0; i < iNumToks; i++)
    {
        pMac = GetMacroPtr(t[i]);

        if (pMac != nullptr)
        {
            iRet = 1;

            // Always expand the leftmost macro
            iPos = sCurStr.find(pMac->sName, 0);

            string sMacro;

            sSub = sCurStr.substr(iPos);

            if (GetMacroString(pMac, sSub, sMacro))
            {
                string sExpanded;

                if (ExpandMacro(pMac, sMacro, sExpanded) == -1)
                {
                    return -1;
                }

                sCurStr.replace(iPos, sMacro.length(), sExpanded);

                if (sCurStr.length() >= MAX_LINE - 1)
                {
                    PutStringAsCmt(sOrig, true, IgnoreNone);
                    ThrowError("Maximum line length of %d exceeded "
                               "during expansion of '%s'",
                               MAX_LINE, pMac->sName.c_str());
                }

                bChanged = true;

                break;
            }
        }
    }

    // Put back any comments.
    sCurStr += sCmtStr;


    // If we expanded a macro, then put this guy back onto our
    // string of things to process.  Otherwise, push it to the output
    // and continue.  By doing this, we correctly handle macros that
    // expand to more than one line.
    if (bChanged)
    {
        // Put new stuff in front of existing unprocessed stuff
        sTmpStr = sCurStr + sTmpStr;
        sCurStr.erase();

    } else
    {
        sOut += sCurStr;
    }

    // find unprocessed segments delimited by newlines.
    // push back into input queue.
    if (sTmpStr.length() > 0)
    {
        vector<string::size_type> newlines;

        iPos = sTmpStr.find('\n');

        while (iPos != string::npos)
        {
            newlines.push_back(iPos);
            iPos = sTmpStr.find('\n', iPos + 1);
        }

        iEnd = sTmpStr.length();

        while (!newlines.empty())
        {
            iPos = newlines.back();
            newlines.pop_back();

            inputFIFO->ungetLine(
                sTmpStr.substr(iPos + 1, iEnd - iPos).c_str(),
                IgnoreNone);

            iEnd = iPos;
        }

        if (iEnd != 0)
            inputFIFO->ungetLine(
                sTmpStr.substr(0, iEnd).c_str(),
                IgnoreNone);
    }

    return iRet;
}

macro *Parser::GetMacroPtr(const char *macName)
{
    map<const char *, macro *, Parser_ltstr>::iterator itr;

    itr = m_macroMap.find(macName);
    return itr == m_macroMap.end() ? nullptr : itr->second;
}

int Parser::ExpandMacro(macro *pMacro, string &sMacro, string &sTargetString)
{
    // Expand any symbols in the line
    string::size_type iPos, iSkip;
    string sSearch, sMutableLine;

    vector<string> args;

    int iNumArgs = GetMacroArgs(pMacro->sName, sMacro, args, pMacro->iType);

    if (iNumArgs == -1)
    {
        return -1;
    }

    int iNumSyms = pMacro->argVector.size();

    if (iNumArgs != iNumSyms)
    {
        ThrowError("MACRO '%s' expected %d arguments, got %d",
                    pMacro->sName.c_str(), iNumSyms, iNumArgs);
        return -1;
    }

    string exp_string = std::to_string(num_expansions++);

    vector<string>::iterator pCurLine;
    for
    (
        pCurLine  = pMacro->codeVector.begin();
        pCurLine != pMacro->codeVector.end();
        ++pCurLine
    )
    {
        // Add newlines between lines (ie. before every line but the first)
        if (pCurLine != pMacro->codeVector.begin())
            sTargetString += '\n';

        // Make a mutable local copy of current line
        sMutableLine = *pCurLine;

        // Now expand any symbols in the line of code
        vector<string>::iterator pSym;
        vector<string>::iterator pArg;

        for
        (
            pSym  = pMacro->argVector.begin(), pArg = args.begin();
            pSym != pMacro->argVector.end();
            ++pSym, ++pArg
        )
        {
            // Set the symbol we will search for
            sSearch = "%";
            sSearch += *pSym;
            sSearch += '%';

            // Now do a search and replace
            iPos  = sMutableLine.find(sSearch);
            iSkip = 0;

            // If we found it, replace all occurances
            while (iPos != string::npos)
            {
                iSkip = iPos + pArg->length();
                sMutableLine.replace(iPos, sSearch.length(), *pArg);

                // Find again
                iPos = sMutableLine.find(sSearch, iSkip);
            }
        }

        // Expand any %% that remain to a unique string representing the
        // number of macro expansions we've had..
        {
            sSearch = "%%";

            // Now do a search and replace
            iPos  = sMutableLine.find(sSearch);
            iSkip = 0;

            // If we found it, replace all occurances
            while (iPos != string::npos)
            {
                iSkip = iPos + exp_string.length();
                sMutableLine.replace(iPos, sSearch.length(), exp_string);

                // Find again
                iPos = sMutableLine.find(sSearch, iSkip);
            }
        }

        sTargetString += sMutableLine;
    }

    return 1;
}

int Parser::GetMacroArgs(macro *pMacro, string &sMacro)
{
    int iNum = -1;

    if (pMacro && pMacro->iType == mUNKNOWN)
    {
        string::size_type iPos = 0;

        // Ugly: Find MACRO case-insensitively so we can skip it.  :-P
        while (1)
        {
            iPos = sMacro.find_first_of("mM", iPos);

            if (iPos == string::npos)
                break;

            if (stricmp_("MACRO", sMacro.substr(iPos,5).c_str()) == 0 &&
                sMacro.length() > iPos + 5 && isspace(sMacro[iPos+5]))
            {
                iPos += 6;
                break;
            }
            iPos++;
        }

        if (iPos == string::npos)
        {
            ThrowError("Unable to find MACRO keyword in macro string %s",
                        sMacro.c_str());
            return -1;
        }

        // Get to the first char of the macro name
        iPos = sMacro.find(pMacro->sName, iPos);

        string s = sMacro.substr(iPos);

        iPos = s.find_first_of('(');
        if (iPos == string::npos)
        {
            pMacro->iType = mNOPAREN;
        }
        else
        {
            pMacro->iType = mPAREN;
        }

        iNum = GetMacroArgs(pMacro->sName, s,
                            pMacro->argVector, pMacro->iType);
    }

    return iNum;
}

int Parser::GetMacroArgs(string &sMacName, string &sMacro, vector<string> &argVector, int iType)
{
    int iParen, iBracket, iNum = 0, iTempPos;
    char c;
    bool bDone = false;
    bool bAdd;
    string sTemp;
    string::size_type iPos = 0;

    // Ugly: Find MACRO case-insensitively so we can skip it.  :-P
    while (1)
    {
        iPos = sMacro.find_first_of("mM", iPos);

        if (iPos == string::npos)
            break;

        if (stricmp_("MACRO", sMacro.substr(iPos,5).c_str()) == 0 &&
            sMacro.length() > iPos + 5 && isspace(sMacro[iPos+5]))
        {
            iPos += 6;
            break;
        }
        iPos++;
    }

    // MACRO keyword may have already been stripped... ugh
    if (iPos == string::npos)
        iPos = 0;

    iPos = sMacro.find(sMacName, iPos);

    if (iPos == string::npos)
    {
        ThrowError("Unable to find macro name %s in macro string %s",
                   sMacName.c_str(), sMacro.c_str());
        return -1;
    }
    else
    {
        iPos += sMacName.length();

        if (iType == mNOPAREN)
        {
            if (iPos < sMacro.length() && !isspace(sMacro[iPos]))
            {
                ThrowError("Macro %s does not match macro name %s",
                           sMacro.c_str(), sMacName.c_str());
                return -1;
            }
        }
        else if (iType == mPAREN)
        {
            // Eat whitespace
            for (; (iPos < sMacro.length() && isspace(sMacro[iPos])); iPos++)
                ;

            if (sMacro[iPos] != '(')
            {
                ThrowError("Opening parenthesis not found for macro \"%s\"",
                           sMacro.c_str());
                return -1;
            }
            else
            {
                iPos++;
            }
        }
    }

    if (iPos != string::npos)
    {
        iParen = 1;
        iBracket = 0;

        // Grab each of the params
        while (!bDone)
        {
            sTemp.erase();

            // Eat whitespace
            for (; iPos < sMacro.length() && isspace(sMacro[iPos]) ; iPos++)
                ;

            bAdd = false;

            do
            {
                c = iPos < sMacro.length() ? sMacro[iPos++] : NUL;

                if (c == '(')
                {
                    sTemp += c;

                    if (iBracket == 0)
                    {
                        iParen++;
                    }
                }
                else if (c == ')')
                {
                    if (iBracket == 0)
                    {
                        iParen--;
                    }

                    // If iParen is 0, we have reached the end of the macro
                    if (iParen == 0)
                    {
                        // If iBracket != 0, there was a bracketry problem
                        if (iBracket != 0)
                        {
                            ThrowError("Mismatched brackets in macro %s", sMacro.c_str());
                            bDone = true;
                            return -1;
                        }
                        else if (iType == mNOPAREN)
                        {
                            ThrowError("Mismatched parenthesis in macro %s", sMacro.c_str());
                            bDone = true;
                            return -1;
                        }
                        else
                        {
                            bAdd = true;
                            bDone = true;
                        }
                    }
                    else
                    {
                        sTemp += c;
                    }
                }
                else if (c == '[')
                {
                    if (iParen == 1)
                    {
                        iBracket++;
                    }
                    else
                    {
                        sTemp += c;
                    }
                }
                else if (c == ']')
                {
                    if (iParen == 1)
                    {
                        iBracket--;
                    }
                    else
                    {
                        sTemp += c;
                    }
                }
                else if (c == ',')
                {
                    if ((iParen == 1) && (iBracket == 0))
                    {
                        bAdd = true;
                    }
                    else
                    {
                        sTemp += c;
                    }
                }
                else if (c == NUL)
                {
                    if (iType == mNOPAREN)
                    {
                        if (iBracket != 0)
                        {
                            ThrowError("Mismatched brackets while reading args for macro %s", sMacro.c_str());
                            bDone = true;
                            return -1;
                        }
                        else
                        {
                            bAdd = true;
                        }
                    }
                    else
                    {
                        ThrowError("End of line encountered while reading args for macro %s", sMacro.c_str());
                        bDone = true;
                        return -1;
                    }

                    bDone = true;
                }
                else
                {
                    sTemp += c;
                }

                if (bAdd) // Add the param into the structure
                {
                    // Eat trailing whitespace
                    iTempPos = sTemp.length() - 1;

                    if (iTempPos >= 0)
                    {
                        for (; ((iTempPos >= 0) && isspace(sTemp[iTempPos])); iTempPos--)
                            /* empty */;

                        sTemp = sTemp.substr(0, iTempPos+1);
                    }

                    // See if this was a valid argument
                    if  (
                        ((iType == mPAREN) && (sTemp.length() == 0)) ||
                        ((iType == mNOPAREN) && (iNum > 0) && (sTemp.length() == 0))
                        )
                    {
                        ThrowError("No argument specified in macro %s", sMacro.c_str());
                        bDone = true;
                    }
                    else
                    {
                        if  (
                            (iType == mNOPAREN && sTemp.length() != 0) ||
                            (iType == mPAREN)
                            )
                        {
                            argVector.push_back(sTemp);
                            iNum++;
                        }
                    }// else
                } // if
            } while (!bAdd); // Loop until we have added a param
        }// while (sMacro
    }

    return iNum;
}

int Parser::StripReturn(string &s)
{
    string::size_type iPos = s.find('\r');

    if (iPos != string::npos)
        s = s.substr(0, iPos);

    iPos = s.find('\n');

    if (iPos != string::npos)
        s = s.substr(0, iPos);

    return 1;
}

int Parser::StripReturn(char *s)
{
    char *pC = strchr(s, '\r');

    if (pC != nullptr)
        *pC = NUL;

    pC = strchr(s, '\n');

    if (pC != nullptr)
        *pC = NUL;

    return 1;
}

