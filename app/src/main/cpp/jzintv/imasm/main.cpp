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

const char *const license_string =
    "IMASM Macro Precompiler\n"
    "imasm -help for help\n"
    "\n"
    "Copyright (C) 2003  Joe Fisher, Shiny Technologies, LLC\n"
    "Portions Copyright (C) 2003  Joseph Zbiciak\n"
    "\n"
    "http://www.shinytechnologies.com\n"
    "\n"
    "This program is free software; you can redistribute it and/or\n"
    "modify it under the terms of the GNU General Public License\n"
    "as published by the Free Software Foundation; either version 2\n"
    "of the License, or (at your option) any later version.\n"
    "\n"
    "This program is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "GNU General Public License for more details.\n"
    "\n"
    "You should have received a copy of the GNU General Public License\n"
    "along with this program; if not, write to the Free Software\n"
    "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, "
    "USA.\n";

int main(int argc, char *argv[])
{
    if (argc <= 1)
    {
        OutputLicense();
        exit(0);
    } else if (argc != 3)
    {
        // For now, always output help if the user doesn't specify
        // correct argument set.
        OutputHelp();
        exit(0);
    }

    try
    {
        StringFIFO_fromFile iFIFO(argv[1]);
        StringFIFO_toFile   oFIFO(argv[2]);
        Parser              p(&iFIFO, &oFIFO);

        p.ParseSourceFile();
    } catch(FileNotFound &f)
    {
        cerr << "ERROR: " << f.msg << endl;
        exit(1);
    } catch(InternalError &ie)
    {
        cerr << ie.msg << endl;
        exit(1);
    } catch(StringFIFO::BufferOverflow &bo)
    {
        cerr << "ERROR:  Line too large. " << endl
             << "    Maximum line length: " << bo.buffer_size << endl
             << "  Attempted line length: " << bo.string_length << endl;

        // todo: output StringFIFO_fromFile location.

        exit(1);
    } catch(Parser::ParseError &p)
    {
        cerr << "ERROR: " << endl  << p.msg << endl;
        exit(1);
    } catch(...) // catch all
    {
        cerr << "INTERNAL ERROR: UNHANDLED EXCEPTION! "
             << __FILE__ ":" << __LINE__ << endl;
    }



    return 0;
}

void OutputLicense()
{
    cout << license_string << endl;
}

void OutputHelp()
{
    cout << "IMASM Macro Precompiler"                        << endl
         << "Copyright (C) 2003  Joe Fisher, Joseph Zbiciak" << endl << endl
         << "IMASM usage:"                                   << endl << endl
         << "  Preprocess file:  imasm inputfile outputfile" << endl
         << "  This help:        imasm -help"                << endl
         << "  Show license:     imasm"                      << endl;
}

