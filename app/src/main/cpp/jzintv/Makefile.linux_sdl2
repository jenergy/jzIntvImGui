##############################################################################
## Source-level Makefile for jzintv
##
## Before editing this file, consider overriding details with .mak files
## in the buildcfg/ directory.  See buildcfg/README.txt for details.
##############################################################################

##############################################################################
## Project directory structure
##############################################################################
B = ../bin
L = ../lib
R = ../rom

##############################################################################
## Include local customizations, if any.
##############################################################################
-include buildcfg/*.mak

##############################################################################
## Project-wide build flags
##############################################################################

# Set "X" to be the executable extension, and "O" to be the object extension.
X =        
O = o

# Set "RM" to be the remove/delete command, if not overridden.
RM ?= rm -f 

# Set to 1 to enable the INTV2PC parallel port controller interface.
ENABLE_INTV2PC ?= 0

# Set to 1 to enable the Classic Game Controller (CGC) USB serial interface.
ENABLE_CGC ?= 1

# Set to 0 or comment out to disable GNU Readline support.
# Set to 1 to enable it.
GNU_READLINE ?= 1

# Get SDL2 related build flags.
SDL2_CFLAGS := $(shell sdl2-config --cflags) -DUSE_SDL2
SDL2_LFLAGS := $(shell sdl2-config --static-libs || sdl2-config --libs)

# Override these in a .mak file. See buidlcfg/00-compiler*.txt for examples.
#
# Default to GCC (unspecified version) for the compiler.  We need a _minimum_
# of C99 for C, and C++14 for C++.
LTO ?= -flto=auto -ffat-lto-objects
ifeq ($(CC),cc)
 CC  = gcc -std=gnu99
endif
ifeq ($(CC),gcc)
 CC  = gcc -std=gnu99
endif
ifeq ($(CXX),c++)
 CXX = g++ -std=c++14
endif
ifeq ($(CXX),g++)
 CXX = g++ -std=c++14
endif

# Default optimization flags, if none specified ina buildcfg/*.mak file.
 OPT_FLAGS ?= -O3

#OPT_FLAGS += -DBENCHMARK_STIC

# If enabling the sanitizer, don't sanitize for alignment.
ifneq ($(SANI),)
SANI += -fno-sanitize=alignment
endif

CFLAGS   += $(OPT_FLAGS) $(SANI) $(LTO) $(WARN)   -I. -I.. $(DEF_FLAGS) $(EXTRA)
CXXFLAGS += $(OPT_FLAGS) $(SANI) $(LTO) $(WARNXX) -I. -I.. $(DEF_FLAGS) $(EXTRA)

CFLAGS   += $(SDL2_CFLAGS) 

# Allow user to specify some extra flags on the command line.
CFLAGS   += $(EXTRA)
CXXFLAGS += $(EXTRA)

ifndef $(NO_LIB_RT)
# Most Linux builds use the high-resolution clock, and so we need -lrt.
LFLAGS   += -lrt
endif

OBJS      = jzintv.$(O)
PROG_SDL2 = $(B)/jzintv
PROG_NULL = $(B)/jzintv_batch
TOCLEAN  += $(PROG_SDL2)
TOCLEAN  += $(PROG_NULL)
TOCLEAN  += core

ifeq ($(GNU_READLINE),1)
RL_CFLAGS = -DUSE_GNU_READLINE
RL_LFLAGS = -lreadline -ltermcap
GNU_READLINE_STATIC_HACK ?= 1
endif

ifeq ($(GNU_READLINE_STATIC_HACK),1)
# An attempt at some dark magic to avoid pulling in things we don't actually
# need, but cause an unnecessary dynamic library dependency.  Disable this
# at the first sign of trouble.
RL_LFLAGS += -Wl,--defsym=setpwent=0 -Wl,--defsym=getpwent=0
RL_LFLAGS += -Wl,--defsym=endpwent=0
RL_LFLAGS += -Wl,--defsym=getpwuid=0 -Wl,--defsym=getpwnam=0
endif

##############################################################################
## Generic build-rules
##############################################################################

all: build

$(PROG_SDL2): $(OBJS) $(OBJS_SDL1)
	$(CXX) -o $(PROG_SDL2) $(OBJS) $(OBJS_SDL2) $(CFLAGS) $(SDL2_LFLAGS) $(LFLAGS) $(RL_LFLAGS)

$(PROG_NULL): $(OBJS) $(OBJS_NULL)
	$(CXX) -o $(PROG_NULL) $(OBJS) $(OBJS_NULL) $(CFLAGS) $(SLFLAGS) $(RL_LFLAGS)

clean:
	$(RM) $(OBJS) 
	$(RM) $(OBJS_SDL2) 
	$(RM) $(OBJS_NULL) 
	$(RM) $(TOCLEAN)

%.$(O): %.c
	$(CC) -o $@ $(CFLAGS) -c $<

%.s: %.c
	$(CC) -fverbose-asm -S -o $@ $(CFLAGS) -c $<

##############################################################################
## Linux-specific stuff
##############################################################################

ifeq ($(ENABLE_INTV2PC),1)
CFLAGS    += -DDIRECT_INTV2PC
OBJS_SDL2 += pads/pads_cgc_linux.$(O)
endif

ifeq ($(ENABLE_CGC),1)
OBJS += pads/pads_intv2pc.$(O)

pads/pads_intv2pc.$(O):
	$(CC) -O3 -o pads/pads_intv2pc.$(O) $(CFLAGS) -c pads/pads_intv2pc.c
endif

##############################################################################
## Makefile.common includes all the subMakefiles and such
##############################################################################
include Makefile.common
build: jzIntv SDK-1600
