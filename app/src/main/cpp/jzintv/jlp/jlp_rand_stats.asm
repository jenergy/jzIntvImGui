;; ======================================================================== ;;
;;  Jean-Luc Project Feature Test                                           ;;
;; ======================================================================== ;;

;* ======================================================================== *;
;*  This program is free software; you can redistribute it and/or modify    *;
;*  it under the terms of the GNU General Public License as published by    *;
;*  the Free Software Foundation; either version 2 of the License, or       *;
;*  (at your option) any later version.                                     *;
;*                                                                          *;
;*  This program is distributed in the hope that it will be useful,         *;
;*  but WITHOUT ANY WARRANTY; without even the implied warranty of          *;
;*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *;
;*  General Public License for more details.                                *;
;*                                                                          *;
;*  You should have received a copy of the GNU General Public License along *;
;*  with this program; if not, write to the Free Software Foundation, Inc., *;
;*  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.             *;
;* ======================================================================== *;
;*                   Copyright (c) 2009, Joseph Zbiciak                     *;
;* ======================================================================== *;

            ROMW    16              ; Use 16-bit ROM width

            CFGVAR  "name" = "JLP Random Number Statistics"
            CFGVAR  "short_name" = "JLP Rand Stats"
            CFGVAR  "year" = 2011
            CFGVAR  "author" = "Joe Zbiciak"
            CFGVAR  "jlp" = 1
            CFGVAR  "license" = "GPLv2+"

;; ======================================================================== ;;
;;  Scratch Memory                                                          ;;
;; ======================================================================== ;;
            ORG     $100, $100, "-RWBN"
ISRVEC      RMB     2
BGC         RMB     1

;; ======================================================================== ;;
;;  System Memory                                                           ;;
;; ======================================================================== ;;
            ORG     $300, $300, "-RWBN"
SGCODE      RMB     32              ; space for save-game spinner
STACK       RMB     32
CURPOS      RMB     1
DELAY       RMB     1


;; ======================================================================== ;;
;;  JLP Memory                                                              ;;
;; ======================================================================== ;;
            ORG     $8040, $8040, "-RWBN"
HIST1       RMB     256
HIST0       RMB     256
HIST2       RMB     256


;; ======================================================================== ;;
;;  Macros and definitions                                                  ;;
;; ======================================================================== ;;
            INCLUDE "library/gimini.asm"
            INCLUDE "macro/util.mac"
            INCLUDE "macro/stic.mac"
            INCLUDE "macro/gfx.mac"
            INCLUDE "macro/print.mac"

            ORG     $5000
            REPEAT  $1000
            DECLE   $
            ENDR

            ORG     $5000           ; Use default memory map

;; ======================================================================== ;;
;;  EXEC-friendly ROM header.                                               ;;
;; ======================================================================== ;;
ROMHDR:     BIDECLE ZERO            ; MOB picture base   (points to NULL list)
            BIDECLE ZERO            ; Process table      (points to NULL list)
            BIDECLE MAIN            ; Program start address
            BIDECLE ZERO            ; Bkgnd picture base (points to NULL list)
            BIDECLE ONES            ; GRAM pictures      (points to NULL list)
            BIDECLE TITLE           ; Cartridge title/date
            DECLE   $03C0           ; No ECS title, run code after title,
                                    ; ... no clicks
ZERO:       DECLE   $0000           ; Screen border control
            DECLE   $0000           ; 0 = color stack, 1 = f/b mode
ONES:       DECLE   1, 1, 1, 1, 1   ; Initial color stack 0..3 and border: blue
;------------------------------------------------------------------------------

MACRO       SETBG   b,  r
            MVII    #C_%b%, %r%
            MVO     %r%,    BGC
ENDM


;; ======================================================================== ;;
;;  TITLE  -- Display our modified title screen & copyright date.           ;;
;; ======================================================================== ;;
TITLE:      PROC
            BYTE    111, 'JLP Rand Stats', 0
           
MAIN:
            MVII    #$100,  R4
            MVII    #$260,  R1
            CALL    FILLZERO

            SETISR  ISRINIT,R0

            MVII    #STACK, R6

            EIS

            MVII    #256 * 3, R1
            MVII    #$8040, R4
            CALL    FILLZERO

                                     ;01234567890123456789
            PRINT_CSTK  0, 0, White, "Collecting numbers."

            MVII    #128,   R0
@@o_loop:
            PSHR    R0
            CLRR    R5
@@loop:
            MVI     $9FFE,  R0
            MOVR    R0,     R1
            MOVR    R0,     R2
            SWAP    R2
            MOVR    R2,     R3
            XORR    R1,     R3
            ANDI    #$FF,   R1
            ANDI    #$FF,   R2
            ANDI    #$FF,   R3

            ADDI    #HIST0, R1
            MVI@    R1,     R0
            INCR    R0
            MVO@    R0,     R1

            ADDI    #HIST1, R2
            MVI@    R2,     R0
            INCR    R0
            MVO@    R0,     R2

            ADDI    #HIST2, R3
            MVI@    R3,     R0
            INCR    R0
            MVO@    R0,     R3

            MOVR    R5,     R0
            SLR     R0,     2
            SLR     R0,     1
            MVO@    R0,     R4
            DECR    R4

            DECR    R5
            BNEQ    @@loop

            MVII    #(('.' - $20) * 8) OR 7, R0
            MVO@    R0,     R4

            PULR    R0
            DECR    R0
            BNEQ    @@o_loop

            PRINT_CSTK  1, 15, White, "Done!"

            CALL    WAIT
            DECLE   60

MACRO   HPAG h,p,l,o
            CALL    CLRSCR

            PRINT_CSTK  0, 0, White, "Histo %h% Page %p%"

            CALL    SHOWHIST
            DECLE   %l% + %o%
            CALL    WAITKEY
ENDM
@@d_loop
            HPAG    0,0,HIST0,$00
            HPAG    0,1,HIST0,$20
            HPAG    0,2,HIST0,$40
            HPAG    0,3,HIST0,$60
            HPAG    0,4,HIST0,$80
            HPAG    0,5,HIST0,$A0
            HPAG    0,6,HIST0,$C0
            HPAG    0,7,HIST0,$E0

            HPAG    1,0,HIST1,$00
            HPAG    1,1,HIST1,$20
            HPAG    1,2,HIST1,$40
            HPAG    1,3,HIST1,$60
            HPAG    1,4,HIST1,$80
            HPAG    1,5,HIST1,$A0
            HPAG    1,6,HIST1,$C0
            HPAG    1,7,HIST1,$E0

            HPAG    2,0,HIST2,$00
            HPAG    2,1,HIST2,$20
            HPAG    2,2,HIST2,$40
            HPAG    2,3,HIST2,$60
            HPAG    2,4,HIST2,$80
            HPAG    2,5,HIST2,$A0
            HPAG    2,6,HIST2,$C0
            HPAG    2,7,HIST2,$E0

            B       @@d_loop

            ENDP


;; ======================================================================== ;;
;;  WAIT                                                                    ;;
;; ======================================================================== ;;
WAIT        PROC
            MVI@    R5,     R0
@@1         MVO     R0,     DELAY
            CLRR    R0
@@loop:     CMP     DELAY,  R0
            BNEQ    @@loop
            JR      R5
            ENDP


;; ======================================================================== ;;
;;  ISR                                                                     ;;
;; ======================================================================== ;;
ISR         PROC
            MVO     R0,     $20
            MVI     $21,    R0

            MVI     BGC,    R0
            MVII    #$28,   R4
            MVO@    R0,     R4
            MVO@    R0,     R4
            MVO@    R0,     R4
            MVO@    R0,     R4
            MVO@    R0,     R4

            MVI     DELAY,  R0
            DECR    R0
            BMI     @@nodelay
            MVO     R0,     DELAY
@@nodelay:
            JR      R5
            ENDP

;; ======================================================================== ;;
;;  ISRINIT                                                                 ;;
;; ======================================================================== ;;
ISRINIT     PROC

            DIS
            SETISR  ISR,    R0

            JE      $1014
            ENDP

;; ======================================================================== ;;
;;  LIBRARY INCLUDES                                                        ;;
;; ======================================================================== ;;
            INCLUDE "library/fillmem.asm"
            INCLUDE "library/hexdisp.asm"
            INCLUDE "library/print.asm"
            INCLUDE "library/wnk.asm"


SHOWHIST    PROC 
            MVI@    R5,     R3
            PSHR    R5
            MOVR    R3,     R5

            MVII    #32,    R3
            MVII    #disp_ptr(2,0), R4
@@loop
            MVII    #7,     R1

            MVI@    R5,     R0
            PSHR    R3
            PSHR    R5
            CALL    HEX16
            PULR    R5
            PULR    R3
            INCR    R4

            DECR    R3
            BNEQ    @@loop

            PULR    PC
            ENDP
