;; ======================================================================== ;;
;;  Joystick Diagnostics Program                                            ;;
;;  Uses EMU_LINK interface to query jzIntv for raw joystick parameters     ;;
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
;*                   Copyright (c) 2005, Joseph Zbiciak                     *;
;* ======================================================================== *;

            ROMW    16              ; Use 16-bit ROM width
            CFGVAR  "name" = "Joystick Diagnostics"
            CFGVAR  "short_name" = "Joystick Diags"
            CFGVAR  "year" = 2005
            CFGVAR  "author" = "Joe Zbiciak"
            CFGVAR  "license" = "GPLv2+"

            ORG     $300, $300, "-RWBN"
NUM_JOY     RMB     1
CUR_JOY     RMB     1
RAW_POS     RMB     2
RAW_CTR     RMB     2
RAW_MIN     RMB     2
RAW_MAX     RMB     2
NRM_POS     RMB     2
DISC        RMB     1
NUM_BUT     RMB     1
BUTTONS     RMB     2
NUM_HAT     RMB     1
HATS        RMB     1
STICSH      RMB     24


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

;; ======================================================================== ;;
;;  EMUlink API subfunctions for joysticks:                                 ;;
;;                                                                          ;;
;;    00: Number of joysticks.  Result in R0.  Ignores R3.                  ;;
;;    01: Get geometry: Returns # of axes, balls, hats, buttons in R0..R3   ;;
;;    02: Get X/Y raw pos:  Returns 16-bit X/Y pos in R1, R2.               ;;
;;    03: Get X/Y raw min:  Returns 16-bit X/Y min in R1, R2.               ;;
;;    04: Get X/Y raw max:  Returns 16-bit X/Y max in R1, R2.               ;;
;;    05: Get X/Y raw ctr:  Returns 16-bit X/Y max in R1, R2.               ;;
;;    06: Get X/Y cooked:   Norm'd 8-bit X/Y in R1, R2. Disc Dir in R0.     ;;
;;    07: Get buttons.  Returns 32-bit bitmap in R1, R2.                    ;;
;;    08: Get hats.  Returns hats 0..3 in 4 x 4-bit fields in R0.           ;;
;; ======================================================================== ;;

EL_NUM_JOY  EQU 0 
EL_GET_GEO  EQU 1
EL_RAW_POS  EQU 2
EL_RAW_MIN  EQU 3
EL_RAW_MAX  EQU 4
EL_RAW_CTR  EQU 5
EL_GET_XY   EQU 6
EL_GET_BTN  EQU 7
EL_GET_HAT  EQU 8

;; ======================================================================== ;;
;;  TITLE  -- Display our modified title screen & copyright date.           ;;
;; ======================================================================== ;;
TITLE:      PROC
            BYTE    105, 'Joystick Diagnostics', 0
           
MAIN:
            MVII    #$100,  R4
            MVII    #$260,  R1
            CALL    FILLZERO

            MVII    #ISR,   R0
            MVO     R0,     $100
            SWAP    R0
            MVO     R0,     $101
            MVII    #$2F0,  R6

            EIS
           
            ;; ------------------------------------------------------------ ;;
            ;;  Check for EMU_LINK support.                                 ;;
            ;; ------------------------------------------------------------ ;;
            MVII    #$4A5A, R0
            SETC
            SIN
            BNC     @@ok
            TSTR    R0
            BEQ     @@ok
           
            ;; ------------------------------------------------------------ ;;
            ;;  Print failure message if we don't detect EMU_LINK.          ;;
            ;; ------------------------------------------------------------ ;;
                                  ;0123456789012345678901234567890123456789
            PRINT_CSTK 6, 3, RED,    "Requires jzIntv    EMU-LINK support."
            B       $

@@ok:
            ;; ------------------------------------------------------------ ;;
            ;;  See if any joysticks are hooked up.                         ;;
            ;; ------------------------------------------------------------ ;;
            CALL    DO_EL
            DECLE   EL_NUM_JOY

            TSTR    R0
            BNEQ    @@got_joy
                                  ;0123456789012345678901234567890123456789
            PRINT_CSTK 6, 1, RED, "No Joystics Detected"
            B       $

@@got_joy   MVO     R0, NUM_JOY

            ;; ------------------------------------------------------------ ;;
            ;;  Main processing loop.  Read info about current controller   ;;
            ;;  and display it.                                             ;;
            ;; ------------------------------------------------------------ ;;
@@loop:
            CALL    GET_RAW
            DECLE   RAW_POS, EL_RAW_POS

            CALL    GET_RAW
            DECLE   RAW_CTR, EL_RAW_CTR

            CALL    GET_RAW
            DECLE   RAW_MIN, EL_RAW_MIN

            CALL    GET_RAW
            DECLE   RAW_MAX, EL_RAW_MAX

            CALL    GET_RAW
            DECLE   NRM_POS, EL_GET_XY
            MVO     R0, DISC

            CALL    GET_RAW
            DECLE   BUTTONS, EL_GET_BTN

;           CALL    DO_EL
;           DECLE   EL_GET_HAT
;           MVO     R0, HATS
    
            CALL    DISP_PAIR
            DECLE   RAW_POS, disp_ptr(1, 0)
    
            CALL    DISP_PAIR
            DECLE   RAW_CTR, disp_ptr(2, 0)
    
            CALL    DISP_PAIR
            DECLE   RAW_MIN, disp_ptr(3, 0)
    
            CALL    DISP_PAIR
            DECLE   RAW_MAX, disp_ptr(4, 0)

            CALL    DISP_PAIR
            DECLE   NRM_POS, disp_ptr(5, 0)

            CALL    DISP_PAIR
            DECLE   BUTTONS, disp_ptr(6, 0)

            B       @@loop

            ; Done.
            ENDP
           
;; ======================================================================== ;;
;;  ISR                                                                     ;;
;; ======================================================================== ;;
ISR         PROC
            MVO     R0,     $20
            MVII    #__CSTK.GROM_TAN, R0
            MVII    #$28,   R4
            MVO@    R0,     R4
            MVO@    R0,     R4
            MVO@    R0,     R4
            MVO@    R0,     R4
            MVO@    R0,     R4
            JR      R5
            ENDP

;; ======================================================================== ;;
;;  DISP_PAIR                                                               ;;
;; ======================================================================== ;;
DISP_PAIR   PROC
            MVI@    R5,     R3
            MVI@    R5,     R4
            PSHR    R5

            MVI@    R3,     R0
            INCR    R3
            PSHR    R3
            MVII    #__CSTK.GROM_WHT, R1
            CALL    HEX16

            INCR    R4
            PULR    R3
            MVI@    R3,     R0
            CALL    HEX16

            PULR    PC
            ENDP

;; ======================================================================== ;;
;;  DO_EL   Make an joystick emu_link call.  API follows call.              ;;
;;  DO_EL.1 Make an joystick emu_link call.  API in R2.                     ;;
;; ======================================================================== ;;
DO_EL       PROC
            MVI@    R5,     R2
@@1         MVII    #$4A5A, R0
            PSHR    R5
            PSHR    R2
            MVII    #8,     R1  ; joystick subsystem on major API #8
            SETC
            SIN
            BC      @@failed
            DECR    R6          ; dump saved EL call #
            PULR    PC          ; return

@@failed:   CALL    CLRSCR
            MVII    #disp_ptr(7, 3), R4
            MVII    #__CSTK.GROM_RED, R1
            CALL    HEX16
                                   ;0123456789012345678901234567890123456789
            PRINT_CSTK  5, 0, RED, "EMU-LINK call failed"
            PULR    R0
            MVII    #disp_ptr(7, 8), R4
            MVII    #__CSTK.GROM_RED, R1
            CALL    HEX16

            PULR    R0
            MVII    #disp_ptr(7,13), R4
            MVII    #__CSTK.GROM_RED, R1
            CALL    HEX16

            B       $
            ENDP

;; ======================================================================== ;;
;;  GET_RAW:  EL calls RAW_POS thru RAW_CTR                                 ;;
;; ======================================================================== ;;
GET_RAW     PROC
            MVI@    R5,     R4
            MVI@    R5,     R2      ; Sub-API number
            PSHR    R5
            MVI     CUR_JOY,R3      ; Current joystick number
            CALL    DO_EL.1
            MVO@    R1,     R4
            MVO@    R2,     R4
            PULR    PC
            ENDP

;; ======================================================================== ;;
;;  LIBRARY INCLUDES                                                        ;;
;; ======================================================================== ;;
            INCLUDE "library/print.asm"
            INCLUDE "library/fillmem.asm"
            INCLUDE "library/prnum16.asm"
            INCLUDE "library/hexdisp.asm"
