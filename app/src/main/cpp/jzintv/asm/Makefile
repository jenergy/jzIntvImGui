##############################################################################
## Source-level Makefile for as1600  (Linux/Unix Native Build)
##############################################################################

##############################################################################
## Project directory structure
##############################################################################
B=../../bin

##############################################################################
## Project-wide build flags
##############################################################################
P          = 

#WARN    = -Wall -W -Wshadow -Wpointer-arith 
#WARN   += -Wbad-function-cast -Wcast-qual 
WARN =

#CC = $(P) egcs
 CC = $(P) gcc


#OPT_FLAGS = -O
#OPT_FLAGS = -O2 -ggdb $(WARN)
 OPT_FLAGS = -O6 -fomit-frame-pointer $(WARN)
#OPT_FLAGS = -O2 -pg -ggdb $(WARN)

CFLAGS = $(OPT_FLAGS) -I. -I.. $(DEF_FLAGS)
LFLAGS = -L../lib


TOCLEAN = $(ASMOBJ) as1600.c
PROG    = $(B)/as1600

##############################################################################
## Generic build-rules
##############################################################################

all: $(PROG)

clean:
	rm -f $(OBJS) 
	rm -f $(TOCLEAN)

%.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<

ASMSRC = as1600.c fraosub.c frapsub.c frasmain.c fryylex.c
ASMOBJ = as1600.o fraosub.o frapsub.o frasmain.o fryylex.o


$(B)/as1600: $(ASMOBJ)
	$(CC) -s -o $(B)/as1600 $(CFLAGS) $(ASMOBJ) -static

as1600.o:   fraebin.h fraeuni.h fragcon.h frasmdat.h
fraosub.o:  fraebin.h fraeuni.h fragcon.h frasmdat.h
frapsub.o:  fraebin.h fraeuni.h fragcon.h frasmdat.h
frasmain.o: fraebin.h fraeuni.h fragcon.h frasmdat.h
fryylex.o:  fraebin.h fraeuni.h fragcon.h frasmdat.h

as1600.o:   as1600.y as1600.c as1600.h
fraosub.o:  as1600.y as1600.c as1600.h
frapsub.o:  as1600.y as1600.c as1600.h
frasmain.o: as1600.y as1600.c as1600.h
fryylex.o:  as1600.y as1600.c as1600.h

as1600.c: as1600.y
	bison -o as1600.c --defines as1600.y


all: $(PROG)
