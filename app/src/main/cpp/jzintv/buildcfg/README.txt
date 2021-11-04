The Makefiles look in this directory for files ending in .mak to include
to configure the build process.  You can override the C and C++ compilers
and other such things by adding files here.  The files will be included in
the order determined by a shell 'glob'.

The filenames that end in '.mak.txt' are example files derived from my own
local environment.  Use one of these when creating your own.

There are also multiple scripts in this directory that some build environments
use.  These filenames end in .sh.  Most scripts assume they will be invoked
from the jzintv/src directory, as that's where 'make' runs.

