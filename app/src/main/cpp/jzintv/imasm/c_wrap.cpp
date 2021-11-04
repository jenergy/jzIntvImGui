// No jokes 'bout the file name, mmm-kay?


/*

IMASM Macro Precompiler

Copyright (C) 2003  Joe Fisher, Shiny Technologies, LLC
Portions Copyright (C) 2003  Joseph Zbiciak.

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
/*  This file provides a C interface to the IMASM parser.                   */
/*  Input comes into the parser by way of StringFIFO_fromCallback.          */
/*  Parser output comes by way of a plane-jane StringFIFO.                  */
/* ======================================================================== */

#include <cstdio>
#include <iostream>
#include <cstdlib>
#include "c_wrap.h"
#include "strfifo.h"
#include "includes.h"

static StringFIFO_fromCallback *iFIFO = nullptr;
static StringFIFO              *oFIFO = nullptr;
static Parser                  *p     = nullptr;
static void (*report_error)(const char*,void*) = nullptr;
static void *re_opaque = nullptr;

using namespace std;

extern "C" int init_parser(struct parser_callbacks *pc)
{
    try
    {
        iFIFO = new StringFIFO_fromCallback(*pc);
        report_error = pc->report_error;
        re_opaque    = pc->re_opaque;

        oFIFO = new StringFIFO();
        p     = new Parser(iFIFO, oFIFO);
    } catch(InternalError &ie)
    {
        cerr << ie.msg << endl;
        exit(1);
    } catch(StringFIFO::BufferOverflow &bo)
    {
        cerr << "ERROR - Line too large. " << endl
             << "        Maximum line length:   " << bo.buffer_size << endl
             << "        Attempted line length: " << bo.string_length << endl;

        // todo: output StringFIFO_fromFile location.

        exit(1);
    } catch(StringFIFO::LoopingExceeded &le)
    {
        cerr << "ERROR - Possible infinite feedback loop " << endl
             << "        Reached maximum loop count: " << le.max_loops
                                                       << endl
             << "        Recursion in macro expansion can cause "
                         "infinite loops." << endl;

        // todo: output StringFIFO_fromFile location.

        exit(1);
    } catch(StringFIFO::QueueDepthExceeded &qe)
    {
        cerr << "ERROR - Possible infinite feedback loop " << endl
             << "        Reached maximum queue depth: " << qe.max_queue << endl
             << "        Recursion in macro expansion can cause "
                         "infinite loops." << endl;

        // todo: output StringFIFO_fromFile location.

        exit(1);
    } catch(Parser::ParseError &pe)
    {
        cerr << "ERROR - " << endl  << pe.msg << endl;
        exit(1);
    } catch(...) // catch all
    {
        cerr << "INTERNAL ERROR: UNHANDLED EXCEPTION! "
             << __FILE__ ":" << __LINE__ << endl;
    exit(1);
    }
    return 0;
}

extern "C" void close_parser()
{
    try
    {
        if (p)     delete p;
        if (oFIFO) delete oFIFO;
        if (iFIFO) delete iFIFO;
    } catch(...) // catch all
    {
        cerr << "INTERNAL ERROR: UNHANDLED EXCEPTION! "
             << __FILE__ ":" << __LINE__ << endl;
    exit(1);
    }
}

static char errbuf[MAX_LINE];

extern "C" char *get_parsed_line(char **buf, int *maxlen,
                                 ignore_flags_t *c_ignore)
{
    (*buf)[0] = 0;
    try
    {
        IgnoreFlags Ignore;

        if (oFIFO->isEmpty())
            p->ParseUntilOutput();

        if (oFIFO->getLine(buf, maxlen, Ignore) == false)
            return nullptr;

        if (c_ignore)
        {
            c_ignore->skip_parse = Ignore.skip_parse;
            c_ignore->skip_macro = Ignore.skip_macro;
        }
    } catch(InternalError &ie)
    {
        cerr << ie.msg << endl;
        exit(1);
    } catch(StringFIFO::BufferOverflow &bo)
    {
        const StringFIFO::loc *loc = iFIFO->getLocation();

        snprintf(errbuf, sizeof(errbuf),
                "%s:%d: ERROR - Line too large. \n"
                "%s:%d:         Maximum line length:    %d\n"
                "%s:%d:         Attempted line length:  %d\n",
                loc->fname ? loc->fname : "<unknown>", loc->lineNo,
                loc->fname ? loc->fname : "<unknown>", loc->lineNo,
                bo.buffer_size,
                loc->fname ? loc->fname : "<unknown>", loc->lineNo,
                bo.string_length);

        if (report_error)
        {
            report_error(errbuf, re_opaque);
            (*buf)[0] = '\n';
            (*buf)[1] = 0;
        } else
        {
            cerr << errbuf << endl;
            exit(1);
        }
    } catch(StringFIFO::LoopingExceeded &le)
    {
        const StringFIFO::loc *loc = iFIFO->getLocation();

        snprintf(errbuf, sizeof(errbuf),
                "%s:%d: ERROR - Possible infinite feedback loop. \n"
                "%s:%d:         Reached maximum loop count:  %d\n"
                "%s:%d:         Recursion in macro expansion can cause "
                                "infinite loops.\n",
                loc->fname ? loc->fname : "<unknown>", loc->lineNo,
                loc->fname ? loc->fname : "<unknown>", loc->lineNo,
                le.max_loops,
                loc->fname ? loc->fname : "<unknown>", loc->lineNo);

        if (report_error)
        {
            report_error(errbuf, re_opaque);
            (*buf)[0] = '\n';
            (*buf)[1] = 0;
        } else
        {
            cerr << errbuf << endl;
            exit(1);
        }
    } catch(StringFIFO::QueueDepthExceeded &qe)
    {
        const StringFIFO::loc *loc = iFIFO->getLocation();

        snprintf(errbuf, sizeof(errbuf),
                "%s:%d: ERROR - Possible infinite feedback loop. \n"
                "%s:%d:         Reached maximum queue depth:  %d\n"
                "%s:%d:         Recursion in macro expansion can cause "
                                "infinite loops.\n",
                loc->fname ? loc->fname : "<unknown>", loc->lineNo,
                loc->fname ? loc->fname : "<unknown>", loc->lineNo,
                qe.max_queue,
                loc->fname ? loc->fname : "<unknown>", loc->lineNo);

        if (report_error)
        {
            report_error(errbuf, re_opaque);
            (*buf)[0] = '\n';
            (*buf)[1] = 0;
        } else
        {
            cerr << errbuf << endl;
            exit(1);
        }
    } catch(Parser::ParseError &pe)
    {
        if (report_error)
        {
            report_error(pe.msg.c_str(), re_opaque);
            (*buf)[0] = '\n';
            (*buf)[1] = 0;
        } else
        {
            cerr << pe.msg << endl;
            exit(1);
        }
    } catch(...) // catch all
    {
        cerr << "INTERNAL ERROR: UNHANDLED EXCEPTION! "
             << __FILE__ ":" << __LINE__ << endl;
        exit(1);
    }

    return *buf;
}
