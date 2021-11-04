;;==========================================================================;;
;; Joe Zbiciak's HELLO WORLD, Version 2. ;;
;; PUBLIC DOMAIN ;;
;; http://spatula-city.org/~im14u2c/intv/ ;;
;;==========================================================================;;

;* ======================================================================== *;
;* TO BUILD IN BIN+CFG FORMAT: *;
;* as1600 -o hello.bin -l hello.lst hello.asm *;
;* *;
;* TO BUILD IN ROM FORMAT: *;
;* as1600 -o hello.rom -l hello.lst hello.asm *;
;* ======================================================================== *;

;* ======================================================================== *;
;* This demo (specifically, the contents of hello2.asm) is hereby placed *;
;* into the public domain. The library routines it includes are NOT *;
;* placed in the public domain, however. You may distribute and/or *;
;* modify this demo as you see fit. Enjoy! *;
;* ======================================================================== *;

ROMW 16 ; Use standard GI 10-bit ROM width

;------------------------------------------------------------------------------
; Include system information
;------------------------------------------------------------------------------
INCLUDE "../library/gimini.asm"

;------------------------------------------------------------------------------
; macros
;------------------------------------------------------------------------------

MACRO gen_cs_card(f, c, g, a)
(GEN_CS_CARD.%f% + %c%*8 + GEN_CS_CARD.%g% + GEN_CS_CARD.%a%)
ENDM

MACRO disp_ptr(r, c)
($200 + %r%*20 + %c%)
ENDM

MACRO disp_ofs(r, c)
(%r%*20 + %c%)
ENDM

;; MVOD: Write a 16-bit value as Double-Byte-Data. Leaves its first 
;; operand byte-swapped.
MACRO MVOD r, a
MVO %r%, %a%
SWAP %r%
MVO %r%, %a% + 1
ENDM

;; LOOP: Decrements %r%, and branches to %l% if it's non-zero.
MACRO LOOP r, l
DECR %r%
BNEQ %l%
ENDM

GEN_CS_CARD PROC
@@Black EQU 00000000000000b ; foreground == 0
@@Blue EQU 00000000000001b ; foreground == 1
@@Red EQU 00000000000010b ; foreground == 2
@@Tan EQU 00000000000011b ; foreground == 3
@@DarkGreen EQU 00000000000100b ; foreground == 4
@@Green EQU 00000000000101b ; foreground == 5
@@Yellow EQU 00000000000110b ; foreground == 6
@@White EQU 00000000000111b ; foreground == 7
@@Grey EQU 01000000000000b ; foreground == 8
@@Cyan EQU 01000000000001b ; foreground == 9
@@Orange EQU 01000000000010b ; foreground == 10
@@Brown EQU 01000000000011b ; foreground == 11
@@Pink EQU 01000000000100b ; foreground == 12
@@LightBlue EQU 01000000000101b ; foreground == 13
@@YellowGreen EQU 01000000000110b ; foreground == 14
@@Purple EQU 01000000000111b ; foreground == 15
@@GRAM EQU 00100000000000b ; Select card from GRAM
@@GROM EQU 00000000000000b ; Select card from GROM
@@Advance EQU 10000000000000b ; Advances color stack.
@@NoAdvance EQU 00000000000000b ; Does not advance color stack
ENDP


;------------------------------------------------------------------------------
; EXEC-friendly ROM header.
;------------------------------------------------------------------------------
ORG $5000 ; Use default memory map
ROMHDR: BIDECLE ZERO ; MOB picture base (points to NULL list)
BIDECLE ZERO ; Process table (points to NULL list)
BIDECLE MAIN ; Program start address
BIDECLE ZERO ; Bkgnd picture base (points to NULL list)
BIDECLE ONES ; GRAM pictures (points to NULL list)
BIDECLE TITLE ; Cartridge title/date
DECLE $03C0 ; No ECS title, run code after title,
; ... no clicks
ZERO: DECLE $0000 ; Screen border control
DECLE $0000 ; 0 = color stack, 1 = f/b mode
ONES: DECLE C_BLU, C_BLU ; Initial color stack 0 and 1: Blue
DECLE C_BLU, C_BLU ; Initial color stack 2 and 3: Blue
DECLE C_BLU ; Initial border color: Blue
;------------------------------------------------------------------------------


;; ======================================================================== ;;
;; TITLE -- Display our modified title screen & copyright date. ;;
;; ======================================================================== ;;
TITLE: PROC
BYTE 103, 'Hello World', 0
BEGIN

; Patch the title string to say '=JRMZ=' instead of Mattel.
PRINT 3, 1, White, "=JRMZ= Productions"
PRINT 10, 8, White, "2003 =JRMZ="

; Done.
RETURN ; Return to EXEC for title screen display
ENDP

;; ======================================================================== ;;
;; MAIN: Here's our main program code. ;;
;; ======================================================================== ;;
MAIN: PROC
BEGIN

CALL CLRSCR ; Clear the screen

PRINT 5, 4, Yellow, "Hello World!"
MVII #disp_ptr(6, 3), R4
MVII #14, R0
MVII #gen_cs_card(Yellow, 13, GROM, NoAdvance), R1 ; 13 is 'dash'
@@dash_loop:
MVO@ R1, R4
LOOP R0, @@dash_loop

RETURN ; Return to the EXEC and sit doing nothing.
ENDP

;; ======================================================================== ;;
;; LIBRARY INCLUDES ;;
;; ======================================================================== ;;
INCLUDE "../library/print.asm" ; PRINT.xxx routines
INCLUDE "../library/fillmem.asm" ; CLRSCR/FILLZERO/FILLMEM


;- 
;-----------------------------------------------------------------------------
;Joseph Zbiciak http://spatula-city.org/~im14u2c/ Not your average "Joe"
; R$+@$=W <-- sendmail.cf {$/{{.+ <-- modem noise
; !@#!@@! <-- Mr. Dithers swearing Zbiciak <-- Joe's last name
;-------- Program Intellivision! http://spatula-city.org/SDK-1600/ ----------
