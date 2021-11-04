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


#ifndef STRFIFO_H_
#define STRFIFO_H_ 1

#include <fstream>
#include <list>
#include <string>
#include <iostream>
#include "c_wrap.h"

struct IgnoreFlags
{
    bool skip_parse;    // If set, do not parse this line.
    bool skip_macro;    // If set, do not expand macros.
};

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
class StringFIFO
{
    private:
        static const int max_looping = 400000;
        static const int max_queue   = 400000;

    public:
        StringFIFO() : looping(0), q_depth(0),
                       lineNo(0), fname("<internal>") { }
        virtual ~StringFIFO() { }

        virtual bool getLine(char *buf,  int  maxlen, IgnoreFlags &Ignore);
        virtual bool getLine(char **buf, int *buflen, IgnoreFlags &Ignore);

        virtual bool getLine(std::string &str, IgnoreFlags &Ignore)
        {
            if (queue.empty())
                return false;

            // Base class never increments this
            if (looping > max_looping)
                throw LoopingExceeded(max_looping);

            str    = queue.front();
            Ignore = Ignore_queue.front();

            queue.pop_front();
            Ignore_queue.pop_front();
            q_depth--;

            return true;
        }

        virtual void putLine(const char *buf, IgnoreFlags Ignore)
        {
            q_depth++;
            queue.push_back(std::string(buf));
            Ignore_queue.push_back(Ignore);

            if (q_depth > max_queue)
                throw QueueDepthExceeded(max_queue);
        }

        virtual void putLine(const std::string &str, IgnoreFlags Ignore)
        {
            q_depth++;
            queue.push_back(str);
            Ignore_queue.push_back(Ignore);

            if (q_depth > max_queue)
                throw QueueDepthExceeded(max_queue);
        }

        virtual void ungetLine(const char *buf, IgnoreFlags Ignore)
        {
            q_depth++;
            queue.push_front(std::string(buf));
            Ignore_queue.push_front(Ignore);

            if (q_depth > max_queue)
                throw QueueDepthExceeded(max_queue);
        }

        virtual void ungetLine(const std::string &str, IgnoreFlags Ignore)
        {
            q_depth++;
            queue.push_front(str);
            Ignore_queue.push_front(Ignore);

            if (q_depth > max_queue)
                throw QueueDepthExceeded(max_queue);
        }


        virtual bool isEmpty(void) { return queue.empty(); }
        virtual bool isEOF(void)   { return queue.empty(); }

        virtual void didLoop(void)
        {
            looping++;

            if (looping > max_looping)
            {
                queue.clear();          // dump remaining queue
                Ignore_queue.clear();   // dump remaining queue
                q_depth = 0;
                throw LoopingExceeded(max_looping);
            }
        }
        virtual void endLoop(void) { looping = 0;          }

        struct BufferOverflow
        {
            int buffer_size, string_length;
            BufferOverflow(int bs, int sl) :
                buffer_size(bs), string_length(sl) { }
        };

        struct LoopingExceeded
        {
            int max_loops;
            LoopingExceeded(int ml) : max_loops(ml) { }
        };

        struct QueueDepthExceeded
        {
            int max_queue;
            QueueDepthExceeded(int mq) : max_queue(mq) { }
        };

        struct loc
        {
            const char *fname;
            int         lineNo;
        };

        virtual const loc *getLocation(void)
        {
            location.fname  = fname;
            location.lineNo = lineNo;
            return &location;
        }


    protected:
        int looping;
        int q_depth;

        std::list<std::string>  queue;
        std::list<IgnoreFlags>  Ignore_queue;

        loc location;
        int lineNo;
        const char *fname;
};


class StringFIFO_fromFile : public StringFIFO
{
    public:
        StringFIFO_fromFile(const char *f_name);
        ~StringFIFO_fromFile();

        bool getLine(char *buf,  int  maxlen, IgnoreFlags &Ignore);
        bool getLine(char **buf, int *buflen, IgnoreFlags &Ignore);
        bool getLine(std::string &str, IgnoreFlags &Ignore);
        bool isEOF(void);
    private:
        std::ifstream inFile;
};


class StringFIFO_toFile : public StringFIFO
{
    public:
        StringFIFO_toFile(const char *f_name);
        ~StringFIFO_toFile();

        bool getLine(char  *buf, int  maxlen, IgnoreFlags &Ignore);
        bool getLine(char **buf, int *buflen, IgnoreFlags &Ignore);
        bool getLine(std::string &str, IgnoreFlags &Ignore);
        void putLine(const char *buf, IgnoreFlags Ignore);
        void putLine(const std::string &str, IgnoreFlags Ignore);
        void ungetLine(const char *buf, IgnoreFlags Ignore);
        void ungetLine(const std::string &str, IgnoreFlags Ignore);
        bool isEmpty(void) { return true;  }
        bool isEOF(void)   { return false; }

    private:
        std::ofstream outFile;
};

static const int hbSize = 1 << 24;

class StringFIFO_fromCallback : public StringFIFO, parser_callbacks
{
    public:
        StringFIFO_fromCallback
        (
            const parser_callbacks &pc
        ) : parser_callbacks(pc)
        {
            is_eof = false;
            lineNo = 0;
            fname  = "<internal>";
            location.fname = fname;
        };
        ~StringFIFO_fromCallback()
        {
            if (location.fname && location.fname != fname)
                delete[] location.fname;
        };

        bool getLine(char  *buf, int  maxlen, IgnoreFlags &Ignore);
        bool getLine(char **buf, int *buflen, IgnoreFlags &Ignore);
        bool getLine(std::string &str, IgnoreFlags &Ignore);
        void putLine(const char *buf, IgnoreFlags Ignore);
        void putLine(const std::string &str, IgnoreFlags Ignore);
        void ungetLine(const char *buf, IgnoreFlags Ignore);
        void ungetLine(const std::string &str, IgnoreFlags Ignore);

        bool isEmpty(void) { return queue.empty();  }
        bool isEOF(void);
        const loc *getLocation(void);


    private:
        bool is_eof;

        //static const int hbSize = 4096;
        static char hugeBuf[hbSize];
};


#endif

