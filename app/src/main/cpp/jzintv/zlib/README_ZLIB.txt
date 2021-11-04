This directory contains a very lightly modified version of zlib 1.2.8 by 
Mark Adler and Jean-loup Gailly.  They retain the copyright to the source.

The only modifications made to this source are to allow the deflate algorithm
to compile outside the full ZLib library.  While minor, the modifications do
make this version diverge from the official version.

All modifications--and any errors arising from them--made by Joe Zbiciak.

Download the original, unmodified ZLib at http://zlib.net/.

Modifications:

 -- Rename local to LOCAL to match jzIntv's definition
 -- Use ANSI C function definitions to match jzIntv's compile flags
 -- #define NO_GZIP forced on
 -- #define Z_SOLO forced on
 -- #define Z_PREFIX forced on, to prevent interference w/ outside zlib
 -- Modify #include lines to point to zlib directory explicitly
 -- Tweaked some (voidpf) casts to (Bytef*) to hush up some compiler warnings
