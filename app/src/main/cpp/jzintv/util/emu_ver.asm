;; ======================================================================== ;;
;;  Use EMU_DETECT interface to query emulator version.                     ;;
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
;*                   Copyright (c) 2006, Joseph Zbiciak                     *;
;* ======================================================================== *;


            ROMW    16              ; Use 16-bit ROM width
            CFGVAR  "name" = "Emulator Detect"
            CFGVAR  "year" = 2006
            CFGVAR  "author" = "Joe Zbiciak"
            CFGVAR  "license" = "GPLv2+"

;; ======================================================================== ;;
;;  Macros and definitions                                                  ;;
;; ======================================================================== ;;
            INCLUDE "library/gimini.asm"
            INCLUDE "macro/util.mac"
            INCLUDE "macro/stic.mac"
            INCLUDE "macro/gfx.mac"
            INCLUDE "macro/print.mac"

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


MACRO ps16(a)
    ((ASC(%a%,0) SHL 8) OR (ASC(%a%,1)))
ENDM


;; ======================================================================== ;;
;;  Known emulators:                                                        ;;
;; ======================================================================== ;;

EMUTBL      PROC
            DECLE   ps16("JZ"), @@jzintv
            DECLE   ps16("KI"), @@kinty
            DECLE   ps16("BL"), @@bliss
            DECLE   ps16("NO"), @@nost
            DECLE   ps16("IW"), @@intvwin
            DECLE   ps16("ID"), @@intvdos
            DECLE   ps16("IP"), @@intvpc
@@last 
@@jzintv    STRING  'jzIntv',0
@@kinty     STRING  'KInty', 0
@@bliss     STRING  'Bliss', 0
@@nost      STRING  'Nostalgia', 0
@@intvwin   STRING  'IntvWin', 0
@@intvdos   STRING  'IntvDOS', 0
@@intvpc    STRING  'IntvPC', 0
            ENDP

;; ======================================================================== ;;
;;  TITLE  -- Display our modified title screen & copyright date.           ;;
;; ======================================================================== ;;
TITLE:      PROC
            BYTE    106, 'Emu Detect', 0
           
MAIN:
            CALL    CLRSCR  

            SETISR  ISR,    R0

            EIS
           
            ;; ------------------------------------------------------------ ;;
            ;;  Check for EMU_LINK support.                                 ;;
            ;; ------------------------------------------------------------ ;;
            MVII    #$656D, R0      ; \_ "em"
            MVII    #$753F, R1      ; /  "u?"
            SETC
            SIN
            BNC     @@ok
           
            ;; ------------------------------------------------------------ ;;
            ;;  Print failure message if we don't detect an emulator        ;;
            ;; ------------------------------------------------------------ ;;
                                  ;0123456789012345678901234567890123456789
            PRINT_CSTK 6, 5, YEL,      "No emulator          detected"
            DECR    PC

@@ok:
            ;; ------------------------------------------------------------ ;;
            ;;  See what emulator it is.                                    ;;
            ;; ------------------------------------------------------------ ;;
            MVII    #EMUTBL,R4
            PSHR    R1

@@loop:     CMP@    R4,     R0
            BEQ     @@found_it
            INCR    R4
            CMPI    #EMUTBL.last, R4
            BNC     @@loop

                                  ;0123456789012345678901234567890123456789
            PRINT_CSTK 4, 5, YEL, "    Unrecognized     emulator detected:"

            MVII    #C_WHT, R1
            MVII    #disp_ptr(7, 5), R4
            CALL    HEX16

            PULR    R0
            MVII    #C_WHT, R1
            MVII    #disp_ptr(7, 11), R4
            CALL    HEX16

            DECR    PC

@@found_it:
            MVI@    R4,     R5      ; Pointer to name
            MOVR    R5,     R0
            MVII    #disp_ptr(6, 10)*2+1,R3

            CLRR    R2
@@center_lp:
            DECR    R3
            CMP@    R5,     R2
            BNEQ    @@center_lp

            SLR     R3,     1
            MOVR    R3,     R4

            MVII    #C_YEL, R1
            CALL    PRINT.R

            PULR    R0
            MVII    #C_YEL, R1
            MVII    #disp_ptr(7, 8),    R4
            CALL    HEX16

            DECR    PC

            ENDP
           
;; ======================================================================== ;;
;;  ISR                                                                     ;;
;; ======================================================================== ;;
ISR         PROC
            MVO     R0,     $20
            MVII    #__CSTK.GROM_BLU, R0
            MVII    #$28,   R4
            MVO@    R0,     R4
            MVO@    R0,     R4
            MVO@    R0,     R4
            MVO@    R0,     R4
            MVO@    R0,     R4
            JR      R5
            ENDP

;; ======================================================================== ;;
;;  LIBRARY INCLUDES                                                        ;;
;; ======================================================================== ;;
            INCLUDE "library/print.asm"
            INCLUDE "library/fillmem.asm"
            INCLUDE "library/prnum16.asm"
            INCLUDE "library/hexdisp.asm"
