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


#define NUL '\0'

#if defined(WIN32) && !defined(WIN32_X)
#define IMASM_GUI (1)
    #include <windows.h>
    #include <richedit.h>
    #include <commctrl.h>
#endif

#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>

#include "excep.h"
#include "main.h"
#include "token.h"
#include "parser.h"

#ifdef IMASM_GUI
    #include "dialogprocs.h"
    #include "gui.h"
    #include "child.h"
    #include "frame.h"
    #include "filepane.h"

    #include "handlers.h"
    #include "registry.h"
    #include "resource.h"
    #include "winutils.h"
#endif

#ifndef MAX_LINE
    #define MAX_LINE (1 << 24)
#endif

#ifndef MAX_PATH
    #ifdef PATH_MAX
        #define MAX_PATH PATH_MAX
    #else
        #define MAX_PATH 256
    #endif
#endif




