
/*

IMASM Macro Precompiler

Copyright (C) 2003  Joe Fisher, Shiny Technologies, LLC
Portions Copyright (C) 2003 Joseph Zbiciak.
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

/* ======================================================================== */
/*  StringFIFO is a generic FIFOing mechanism for providing I/O between     */
/*  line-oriented entities.  StringFIFOs can be "push" or "pull".           */
/*                                                                          */
/*  Strings (more specifically, char*) are pushed onto the FIFO with the    */
/*  putLine() member function.  Strings are popped off the FIFO with the    */
/*  getLine() member function.                                              */
/*                                                                          */
/*  The class supports both "push" and "pull" uses.  Input streams tend to  */
/*  be "pull", and output streams tend to be "push".  More complex hookups  */
/*  tend to be a little of both.                                            */
/*                                                                          */
/*  Two subclasses of StringFIFO specialize FIFOs that are fed by files or  */
/*  which feed files:  StringFIFO_fromFile and StringFIFO_toFile.           */
/*                                                                          */
/*  Another subclass of StringFIFO provides a specialized FIFO that is      */
/*  fed via a callback:  StringFIFO_fromCallback.                           */
/* ======================================================================== */


#include "strfifo.h"
#include "excep.h"
#include <algorithm>
#include <cstring>

using namespace std;

/* ======================================================================== */
/*  StringFIFO::getLine:  Return a line if one available, or false.         */
/* ======================================================================== */
bool
StringFIFO::getLine(char *buf, int maxlen, IgnoreFlags &Ignore)
{
    string *head;
    int len;

    if (queue.empty())
        return false;

    Ignore = Ignore_queue.front();
    head   = &*queue.begin();
    len    = head->length();

    // todo: Convert to ParseError?
    // ..... No. Have Parser catch BufferOverflow and rethrow as ParseError.
    if (len >= maxlen)
        throw BufferOverflow(len, maxlen);

    strcpy(buf, head->c_str());

    q_depth--;
    queue.pop_front();
    Ignore_queue.pop_front();

    lineNo++;
    return true;
}

bool
StringFIFO::getLine(char **buf, int *buflen, IgnoreFlags &Ignore)
{
    string *head;
    int len;

    if (queue.empty())
        return false;

    Ignore = Ignore_queue.front();
    head   = &*queue.begin();
    len    = head->length();

    if (len >= *buflen)
    {
        char *new_buf = static_cast<char *>(malloc(len + 128));
        if (!new_buf)
            throw BufferOverflow(len, *buflen);

        free(*buf);
        *buflen = len + 128;
        *buf    = new_buf;
    }

    strcpy(*buf, head->c_str());

    q_depth--;
    queue.pop_front();
    Ignore_queue.pop_front();

    lineNo++;
    return true;
}



/* ======================================================================== */
/*  StringFIFO_fromFILE:  Constructor has job of opening the file.          */
/* ======================================================================== */
StringFIFO_fromFile::StringFIFO_fromFile(const char *file_name)
{
    char *tmp;
    inFile.open(file_name);

    if (inFile.fail())
        throw FileNotFound("input", file_name);

    lineNo = 0;
    tmp    = new char[strlen(file_name) + 1];
    fname  = tmp;
    strcpy(tmp, file_name);
}

StringFIFO_fromFile::~StringFIFO_fromFile()
{
    inFile.close();
    delete[] fname;
}


/* ======================================================================== */
/*  StringFIFO_fromFile::getLine actually reads the file.                   */
/* ======================================================================== */
bool
StringFIFO_fromFile::getLine(string &str, IgnoreFlags &Ignore)
{
    // We allow "putLine" on a StringFIFO_fromFile.  The effect is to insert
    // new lines ahead of lines that we would be reading from the file.
    if (!queue.empty())
    {
        bool gotLine;

        didLoop();
        gotLine = StringFIFO::getLine(str, Ignore);
        if (gotLine)
            lineNo--; // counteract parent's lineNo++;

        return gotLine;
    }
    endLoop();

    // Nothing in the queue, so go out to the file.
    if (inFile.eof())
    {
        str = "";  // at least zero out the string.
        return false;
    }

    getline(inFile, str);

    if (inFile.fail())
    {
        str = "";  // at least zero out the string.
        return false;
    }

    lineNo++;
    Ignore.skip_macro = false;
    Ignore.skip_parse = false;

    return true;
}

bool
StringFIFO_fromFile::getLine(char *buf, int maxlen, IgnoreFlags &Ignore)
{
    string tmp;
    bool gotLine;

    if ((gotLine = getLine(tmp, Ignore)) == true)
    {
        int len = tmp.length();
        if (len >= maxlen)
            throw BufferOverflow(len, maxlen);

        strcpy(buf, tmp.c_str());
    } else
        buf[0] = 0;
    return gotLine;
}

bool
StringFIFO_fromFile::getLine(char **buf, int *buflen, IgnoreFlags &Ignore)
{
    string tmp;
    bool gotLine;

    if ((gotLine = getLine(tmp, Ignore)) == true)
    {
        int len = tmp.length();
        if (len >= *buflen)
        {
            char *new_buf = static_cast<char *>(malloc(len + 128));
            if (!new_buf)
                throw BufferOverflow(len, *buflen);

            free(*buf);
            *buflen = len + 128;
            *buf    = new_buf;
        }

        strcpy(*buf, tmp.c_str());
    } else
        buf[0] = nullptr;
    return gotLine;
}


/* ======================================================================== */
/*  StringFIFO_fromFile::isEOF returns true when both the queue and file    */
/*  are done.  StringFIFO_fromFile::isEmpty only checks the queue -- there  */
/*  may be more available if somebody might "putLine".                      */
/* ======================================================================== */
bool
StringFIFO_fromFile::isEOF(void)
{
    return queue.empty() && inFile.eof();
}


/* ======================================================================== */
/*  StringFIFO_toFILE:  Constructor has job of opening the file.            */
/*                      Destructor has job of closing it.                   */
/* ======================================================================== */
StringFIFO_toFile::StringFIFO_toFile(const char *file_name)
{
    char *tmp;
    outFile.open(file_name);

    if (outFile.fail())
        throw FileNotFound("output", file_name);

    lineNo = 0;
    tmp    = new char[strlen(file_name) + 1];
    fname  = tmp;
    strcpy(tmp, file_name);
}

StringFIFO_toFile::~StringFIFO_toFile()
{
    outFile.close();
    delete[] fname;
}


/* ======================================================================== */
/*  StringFIFO_toFile::getLine doesn't make sense, so error it out.         */
/*  StringFIFO_toFile::ungetLine also.                                      */
/* ======================================================================== */
bool
StringFIFO_toFile::getLine(char *, int, IgnoreFlags&)
{
    throw InternalError("StringFIFO", "getLine on StringFIFO_toFile attempted",
                        __FILE__, __LINE__);
}
bool
StringFIFO_toFile::getLine(char **, int *, IgnoreFlags&)
{
    throw InternalError("StringFIFO", "getLine on StringFIFO_toFile attempted",
                        __FILE__, __LINE__);
}
bool
StringFIFO_toFile::getLine(string&, IgnoreFlags&)
{
    throw InternalError("StringFIFO", "getLine on StringFIFO_toFile attempted",
                        __FILE__, __LINE__);
}
void
StringFIFO_toFile::ungetLine(const char *, IgnoreFlags)
{
    throw InternalError("StringFIFO", "ungetLine on StringFIFO_toFile attempted",
                        __FILE__, __LINE__);
}
void
StringFIFO_toFile::ungetLine(const string &, IgnoreFlags)
{
    throw InternalError("StringFIFO", "ungetLine on StringFIFO_toFile attempted",
                        __FILE__, __LINE__);
}

/* ======================================================================== */
/*  StringFIFO_toFile::putLine just writes the line out immediately.        */
/*  It does NOT add an 'end of line' character.                             */
/* ======================================================================== */
void
StringFIFO_toFile::putLine(const char *buf, IgnoreFlags Ignore)
{
    if (!Ignore.skip_parse)
        outFile << buf;

    // oddball: we increment the line count here because "toFile" FIFOs
    // automagically "get" each line out to the file every time its put.
    lineNo++;
}
void
StringFIFO_toFile::putLine(const string &str, IgnoreFlags Ignore)
{
    if (!Ignore.skip_parse)
        outFile << str;

    // oddball: we increment the line count here because "toFile" FIFOs
    // automagically "get" each line out to the file every time its put.
    lineNo++;
}


char StringFIFO_fromCallback::hugeBuf[hbSize];

/* ======================================================================== */
/*  StringFIFO_fromCallback::getLine first consults the queue (since we     */
/*  allow putLine to stuff things in there ahead of us).  If queue is       */
/*  empty, then we go ahead and call the callback function.                 */
/* ======================================================================== */
bool
StringFIFO_fromCallback::getLine(char *buf, int maxlen, IgnoreFlags &Ignore)
{
    // We allow "putLine" on a StringFIFO_fromFile.  The effect is to insert
    // new lines ahead of lines that we would be reading from the file.
    // The "reexam" callback allows the other side to adjust this input
    if (!queue.empty())
    {
        didLoop();
        is_eof = false;

        bool ok = StringFIFO::getLine(buf, maxlen, Ignore);
        ignore_flags_t c_ignore;
        c_ignore.skip_parse = Ignore.skip_parse;
        c_ignore.skip_macro = Ignore.skip_macro;

        if (ok && reexam)
            reexam(buf, maxlen, &c_ignore, rx_opaque);

        Ignore.skip_parse = c_ignore.skip_parse;
        Ignore.skip_macro = c_ignore.skip_macro;
        return ok;
    }
    endLoop();

    // Nothing in the queue.  Call the getline callback to see if we
    // can get some more text.
    ignore_flags_t c_ignore = { false, false };
    int gotLine = getline(buf, maxlen, &c_ignore, gl_opaque);

    Ignore.skip_parse = c_ignore.skip_parse;
    Ignore.skip_macro = c_ignore.skip_macro;

    if (gotLine == 0 && get_eof != nullptr)
        is_eof = get_eof(ge_opaque) != 0;
    else
        is_eof = false;

    if (gotLine != 0)
        lineNo++;

    return gotLine ? true : false;
}
bool
StringFIFO_fromCallback::getLine(string &str, IgnoreFlags &Ignore)
{
    bool gotLine = getLine(hugeBuf, hbSize, Ignore);

    if (gotLine)
        str = hugeBuf;
    else
        str = "";

    return gotLine ? true : false;
}

bool
StringFIFO_fromCallback::getLine(char **buf, int *buflen, IgnoreFlags &Ignore)
{
    // For now, limit to the larger of hbSize and *buflen.  *sigh*

    if (hbSize > *buflen)
    {
        bool ok = getLine(hugeBuf, hbSize, Ignore);

        if (ok)
        {
            int len = strlen(hugeBuf) + 1;
            if (len >= *buflen)
            {
                char *new_buf = static_cast<char *>(malloc(len + 128));
                if (!new_buf)
                    throw BufferOverflow(len, *buflen);

                free(*buf);
                *buflen = len + 128;
                *buf    = new_buf;
            }
            strcpy(*buf, hugeBuf);
        } else
            (*buf)[0] = 0;

        return ok;
    } else
        return getLine(*buf, *buflen, Ignore);
}


/* ======================================================================== */
/*  StringFIFO_fromCallback::isEOF only returns EOF if we have a callback   */
/*  that can tell us unequivocally that it's EOF.  EOF does not prevent     */
/*  calls to the getline callback.  Receiving new data either by getline    */
/*  or calls to the putLine method will negage the EOF status.              */
/* ======================================================================== */
bool
StringFIFO_fromCallback::isEOF()
{
    if (is_eof)
        return true;

    if (get_eof)
        is_eof = get_eof(ge_opaque) ? true : false;

    return is_eof;
}

/* ======================================================================== */
/*  StringFIFO_fromCallback::putLine just ensures is_eof is cleared before  */
/*  stuffing stuff into the queue.  Ditto for ::ungetLine                   */
/* ======================================================================== */
void
StringFIFO_fromCallback::putLine(const char *buf, IgnoreFlags Ignore)
{
    is_eof = false;

    StringFIFO::putLine(buf, Ignore);
}
void
StringFIFO_fromCallback::putLine(const string &str, IgnoreFlags Ignore)
{
    is_eof = false;

    StringFIFO::putLine(str, Ignore);
}
void
StringFIFO_fromCallback::ungetLine(const char *buf, IgnoreFlags Ignore)
{
    is_eof = false;

    StringFIFO::ungetLine(buf, Ignore);
}
void
StringFIFO_fromCallback::ungetLine(const string &str, IgnoreFlags Ignore)
{
    is_eof = false;

    StringFIFO::ungetLine(str, Ignore);
}

/* ======================================================================== */
/*  StringFIFO_fromCallback::getLocation relies on a callback to report     */
/*  our logical location within a stream.  If no callback is registered,    */
/*  then we fall back on our built-in line counter, reporting the filename  */
/*  as "<internal>".                                                        */
/* ======================================================================== */
const StringFIFO::loc *
StringFIFO_fromCallback::getLocation()
{
    if (get_pos)
    {
        const char *f = get_pos(&location.lineNo, gp_opaque);
        if (f != nullptr)
        {
            if (location.fname != nullptr && location.fname != fname)
                delete[] location.fname;

            char *tmp = new char[strlen(f) + 1];
            location.fname = tmp;
            strcpy(tmp, f);
        } else
        {
            location.fname = fname;
        }
    } else
    {
        location.fname  = fname;
        location.lineNo = lineNo;
    }

    return &location;
}
