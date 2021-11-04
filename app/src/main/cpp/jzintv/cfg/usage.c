#include "config.h"

#define USAGE_C_
#include "cfg.h"

LOCAL const char *jzintv_copyright =
"Copyright 2018, Joseph Zbiciak"                                            "\n"
""
"Portions    Tim Lindner, John Tanner, Rick Reynolds, Pedro Giffuni,"       "\n"
"copyright:  Joe Fisher, Frank Palazzolo, Kjell Breding, Daniele Moglia,"   "\n"
"            Marco Turconi, Patrick Nadeau, Andrea Mazzoleni (scale2x/3x/4x),\n"
"            Markus Oberhumer (minilzo), Oscar Toledo (AVI),"               "\n"
"            Jean-loup Gailly (zlib), Mark Adler (zlib)"                    "\n"
;


/* ======================================================================== */
/*  USAGE            -- Just give usage info and exit.                      */
/* ======================================================================== */
void usage(void)
{
    jzp_init(0,stdout,NULL,NULL);
    jzp_printf(
                                                                            "\n"
"jzIntv v%d.%d"                                                             "\n"
"%s%c"
                                                                            "\n"
"%s"
                                                                            "\n"
"Usage:"                                                                    "\n"
"    jzintv [flags] gamefile"                                               "\n"
                                                                            "\n"
"Specifying a game image:"                                                  "\n"
"    jzIntv supports most, if not all, known formats for Intellivision"     "\n"
"    game ROM images.  When invoking jzIntv, provide the full path to"      "\n"
"    the game-image file.  If you omit the file extension, jzIntv will"     "\n"
"    search for an appropriate file by trying the following extensions:"    "\n"
"    .rom, .bin, .int, and .itv.  The overall ROM search process is"        "\n"
"    somewhat involved.  See the jzIntv documentation for details."         "\n"
                                                                            "\n"
"    If the filename ends in .bin, .int, or .itv, or if jzIntv does not"    "\n"
"    recognize the file's extension, it will assume the file is in BIN+CFG" "\n"
"    format.  In that case, jzIntv will look for a matching .cfg file."     "\n"
                                                                            "\n"
"    Systems with case-sensitive file names (Linux, UNIX, etc.) need to"    "\n"
"    ensure the filename extensions are in lower-case.  On all systems,"    "\n"
"    jzIntv expects to receive file names with lower-case extensions."      "\n"
                                                                            "\n"
    ,JZINTV_VERSION_MAJOR
    ,JZINTV_VERSION_MINOR
    ,svn_revision ? svn_revision : ""
    ,svn_revision ? '\n' : ' '
    ,jzintv_copyright
    );
    jzp_printf(
"ROM Image Flags:"                                                          "\n"
"    -e /path/to/exec.bin          Specifies path to the EXEC ROM image."   "\n"
"    --execimg=/path/to/exec.bin   By default, jzIntv looks in current dir.  \n"
                                                                            "\n"
"    -g /path/to/grom.bin          Specifies path to the GROM ROM image."   "\n"
"    --gromimg=/path/to/grom.bin   By default, jzIntv looks in current dir.  \n"
                                                                            "\n"
"    -E /path/to/ecs.bin           Specifies path to the ECS ROM image"     "\n"
"    --ecsimg=/path/to/ecs.bin     By default, jzIntv looks in current dir.  \n"
"                                  Only needed when ECS is enabled."        "\n"
                                                                            "\n"
"Intellivision Hardware Flags:"                                             "\n"
"    -s#     --ecs=#               ECS.           0: Disable, 1: Enable"    "\n"
"    -v#     --voice=#             Intellivoice.  0: Disable, 1: Enable"    "\n"
"    -W#     --voicewindow=#       Sets averaging window for voice filter." "\n"
"    -Vname  --voicefiles=name     Saves voice WAV files to name####.wav."  "\n"
"    -P      --pal                 Start jzIntv in PAL (50Hz) mode."        "\n"
                                                                            "\n"
"Video and Sound Flags:"                                                    "\n"
    );
#ifndef GP2X
    jzp_printf(
"    -z<res> --displaysize=<res>   Desired active display size, *before*"   "\n"
"                                  adding border area."                     "\n"
"                                  <res> can be a string of the form"       "\n"
"                                  XDIMxYDIM,DEPTH such as \"320x200,8\""   "\n"
"                                  or a single digit specifying a built-in" "\n"
"                                  resolution from the following set."      "\n"
"                                      0:  320x200,8bpp"                    "\n"
"                                      1:  640x480,8bpp"                    "\n"
"                                      2:  320x240,16bpp"                   "\n"
"                                      3:  1024x768,8bpp"                   "\n"
"                                      4:  1680x1050,8bpp"                  "\n"
"                                      5:  800x400,16bpp"                   "\n"
"                                      6:  1600x1200,32bpp"                 "\n"
"                                      7:  3280x1200,32bpp"                 "\n"
                                                                            "\n"
"    -b#     --gfx-border-pct=#    Increase display size by # percent,"     "\n"
"                                  filling the add'l space w/ border color.\n"
"                                  e.g. -z0 -b10 produces a 384x240 display\n"
"                                  with 320x200 active area centered within.\n"
                                                                            "\n"
"            --gfx-border-x=#      Directly set horizontal border padding." "\n"
"            --gfx-border-y=#      Directly set vertical border padding."   "\n"
                                                                            "\n"
"            --resolution=<res>    Synonym for --displaysize"               "\n"
"    -f# -x# --fullscreen=#        Full screen display:"                    "\n"
"                                      0:  Windowed"                        "\n"
"                                      1:  Full screen"                     "\n"
"            --prescale=<ps>       Enable prescaler <ps>.  Use the flag"    "\n"
"                                  \"--prescale=-1\" to print a list of the\n"
"                                  supported prescalers."                   "\n"
    );
#endif
    jzp_printf(
"            --gfx-palette=<file>  Load alternate palette from <file>"      "\n"
"    -G#     --gramsize=#          Change number of GRAM tiles"             "\n"
"                                      0:  64 tiles (standard)"             "\n"
"                                      1:  128 tiles"                       "\n"
"                                      2:  256 tiles (INTV88)"              "\n"
                                                                            "\n"
"    -a#     --audiorate=#         Audio sampling rate.  0 disables audio." "\n"
"            --audio=#             Synonym for --audiorate."                "\n"
"    -Fname  --audiofile=name      Records all audio to specified file."    "\n"
"    -w#     --audiowindow=#       Sets averaging window for audio filter." "\n"
"    -B#     --audiobufsize=#      Internal audio buffer size."             "\n"
"    -C#     --audiobufcnt=#       Internal audio buffer count."            "\n"
"    -M#     --audiomintick=#      Minimum Intellivision cycles between"    "\n"
"                                  explicit calls to snd_tick()."           "\n"
                                                                            "\n"
"Input Configuration Flags:"                                                "\n"
"    Currently, jzIntv does not offer a flexible method to re-bind keys."   "\n"
"    The kbdhackfile does allow you to crudely specify key bindings."       "\n"
                                                                            "\n"
"    --kbdhackfile=/path/to/file   Configure key bindings."                 "\n"
                                                                            "\n"
"    -m#     --kbdmap=#            Specify initial keyboard map (0-3)"      "\n"

                                                                            "\n"
"    Analog and USB joysticks have a rich set of configuration parameters." "\n"
"    Run jzIntv with an empty configuration string to find out defaults"    "\n"
"    are for your joystick and operating system.  For more information on"  "\n"
"    configuring joysticks, see the file jzintv/doc/jzintv/joystick.txt"    "\n"
                                                                            "\n"
"    --js0=\"config string\"         Configures Joystick #0"                "\n"
"    --js1=\"config string\"         Configures Joystick #1"                "\n"
"    --js2=\"config string\"         Configures Joystick #2"                "\n"
"    --js3=\"config string\"         Configures Joystick #3"                "\n"
                                                                            "\n"
    );
#ifdef DIRECT_INTV2PC
    jzp_printf(
"    The INTV2PC can drive either the Master Component or the ECS' controller\n"
"    inputs.  The following flags associate INTV2PCs with controllers:"     "\n"
                                                                            "\n"
"    -i#     --i2pc0=#             Port # for INTV2PC for Master Comp inputs.\n"
"    -I#     --i2pc1=#             Port # for INTV2PC for ECS inputs."      "\n"
"            --intv2pc0=#          Synonym for --i2pc0"                     "\n"
"            --intv2pc1=#          Synonym for --i2pc1"                     "\n"
                                                                            "\n"
    );
#endif
#ifdef CGC_DLL
    jzp_printf(
"    The Classic Game Controller can drive either the Master Component or"  "\n"
"    the ECS' controller inputs.  The following flags associate CGCs with"  "\n"
"    controllers:"                                                          "\n"
                                                                            "\n"
"    --cgc0[=#]                    CGC for Master Component inputs."        "\n"
"    --cgc1[=#]                    CGC for ECS inputs."                     "\n"
                                                                            "\n"
    );
#endif
#ifdef CGC_THREAD
    jzp_printf(
"    The Classic Game Controller can drive either the Master Component or"  "\n"
"    the ECS' controller inputs.  The following flags associate CGCs with"  "\n"
"    controllers.  They also specify the path to the CGC's device node:"    "\n"
                                                                            "\n"
"    --cgc0=/path/to/cgc           CGC for Master Component inputs."        "\n"
"    --cgc1=/path/to/cgc           CGC for ECS inputs."                     "\n"
                                                                            "\n"
    );
#endif
    jzp_printf(
"Intellicart .ROM emulation specific flags:"                                "\n"
"    -c#     --icartcache=#        Change caching policy for Intellicart"   "\n"
"                                  .ROM programs:"                          "\n"
"                                  0:  Cache bankswitched memory (default)" "\n"
"                                  1:  Don't cache bankswitched memory"     "\n"
"                                  2:  Only cache read-only, non-banksw."   "\n"
"                                  3:  Do not cache anything."              "\n"
                                                                            "\n"
"Debugger flags:"                                                           "\n"
"    -d      --debugger            Enable jzIntv's debugger."               "\n"
"            --sym-file=path       Load symbol table from 'path'."          "\n"
"            --src-map=path        Load source/listing map from 'path'."    "\n"
"            --script=path         Execute debug commands from 'path'."     "\n"
"            --rand-mem            Randomize memories on startup"           "\n"
                                                                            "\n"
    );
    jzp_printf(
"Misc Flags:"                                                               "\n"
"    -r#     --ratecontrol=#       \\_ Speed up by factor #.  Setting #"    "\n"
"            --macho=#             /  to 0 disables rate control."          "\n"
                                                                            "\n"
"    -p path --rom-path=path       Append path to the ROM search path."     "\n"
                                                                            "\n"
"    -q      --quiet               Hide jzIntv's non-error output."         "\n"
                                                                            "\n"
"            --gui-mode            Tells jzIntv to listen for commands from""\n"
"                                  a GUI over stdin."                       "\n"
                                                                            "\n"
"    -J<#>                         Explicitly set JLP accelerator enable."  "\n"
"                                  0:  Off"                                 "\n"
"                                  1:  Accel+RAM On, no flash"              "\n"
"                                  2:  Accel+RAM Off"                       "\n"
"                                  3:  Accel+RAM On, with flash"            "\n"
                                                                            "\n"
"            --jlp-savegame=path   Enable JLP-style save-game support with" "\n"
"                                  'path' as the save-game file."           "\n"
                                                                            "\n"
"            --file-io             Enable Emu-Link File-IO support"         "\n"
"                                  (See examples/fileio/fileio.asm)"        "\n"
                                                                            "\n"
"            --start-delay=#       Delay jzIntv startup by # seconds."      "\n"
"                                  # can be floating point (e.g. 1.5)."     "\n"
                                                                            "\n"
"            --avirate=#           Scales time by # when recording AVI files.\n"
"                                  # can be floating point (e.g. 1.5)."     "\n"
                                                                            "\n"
"            --ecs-tape=path       Template for ECS tape file names."       "\n"
"                                  An '#' in the name expands to the 4 char""\n"
"                                  CSAV/CLOD name preceded by an '_', if"   "\n"
"                                  provided."                               "\n"
                                                                            "\n"
"            --ecs-printer=path    File to append ECS printer output to."   "\n"
                                                                            "\n"
"            --cheat='<cheat>'     Adds a cheat code.  Up to 8 cheats can"  "\n"
"                                  be added.  See doc/jzintv/cheat.txt."    "\n"
                                                                            "\n"
"    -l      --license             License information"                     "\n"
" -h -?      --help                This usage info"                         "\n"
                                                                            "\n"
    );
    jzp_printf(
"Environment:"                                                              "\n"
"    JZINTV_ROM_PATH               Controls ROM search path.  Components of\n"
"                                  the ROM search path should be separated" "\n"
"                                  by \"" PATH_COMPONENT_SEP
                                       "\" characters.  jzIntv examines"    "\n"
"                                  dirs specified by --rom-path before"     "\n"
"                                  dirs specified by JZINTV_ROM_PATH."      "\n"
                                                                            "\n"
#ifdef GP2X
"GP2X specific flags"                                                       "\n"
"            --gp2xclock=#         Set clock rate in MHz.  Default:  200MHz.\n"
"                                  0 means \"do not change current rate.\"" "\n"
                                                                            "\n"
"            --gp2x-pad-bias=#     Set the biasing mode for the GP2X's"     "\n"
"                                  directional control pad:"                "\n"
"                                  1: 16-dir, no bias (default)"            "\n"
"                                  2: 8-dir, UD/LR bias"                    "\n"
"                                  3: 8-dir, diagonal bias"                 "\n"
"                                  4: 8-dir with dead zones"                "\n"
"                                  5: 4-dir, UD/LR bias"                    "\n"
"                                  6: 4-dir, diagonal bias"                 "\n"
                                                                            "\n"
#endif
"Legal note:"                                                               "\n"
"    Intellivision(TM) is a trademark of Intellivision Entertainment.  Neither"
                                                                            "\n"
"    Joe Zbiciak nor jzIntv are affiliated with Intellivision Entertainment.\n"
                                                                            "\n"
    );

    jzp_flush();
}

/* ======================================================================== */
/*  LICENSE          -- Just give license/authorship info and exit.         */
/* ======================================================================== */
void license(void)
{
    jzp_init(0,stdout,NULL,NULL);
    jzp_printf(
                                                                            "\n"
"jzIntv v%d.%d"                                                             "\n"
"%s%c"
                                                                            "\n"
"%s"
                                                                            "\n"
"This program is free software; you can redistribute it and/or modify it"   "\n"
"under the terms of the GNU General Public License as published by the Free\n"
"Software Foundation; either version 2 of the License, or (at your option)" "\n"
"any later version."                                                        "\n"
                                                                            "\n"
"This program is distributed in the hope that it will be useful, but WITHOUT\n"
"ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or"     "\n"
"FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for" "\n"
"more details."                                                             "\n"
                                                                            "\n"
"You should have received a copy of the GNU General Public License along"   "\n"
"with this program; if not, write to the Free Software Foundation, Inc.,"   "\n"
"51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA."               "\n"
                                                                            "\n"
"Run \"jzintv --help\" for usage information."                              "\n"
                                                                            "\n"
"Legal note:"                                                               "\n"
"    Intellivision(TM) is a trademark of Intellivision Entertainment.  Joe" "\n"
"    Zbiciak and jzIntv are not affiliated with Intellivision Entertainment.\n"
                                                                            "\n"
    ,JZINTV_VERSION_MAJOR
    ,JZINTV_VERSION_MINOR
    ,svn_revision ? svn_revision : ""
    ,svn_revision ? '\n' : ' '
    ,jzintv_copyright
    );

    jzp_flush();
}
