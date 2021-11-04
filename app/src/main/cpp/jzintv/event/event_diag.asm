;; ======================================================================== ;;
;;  Event Diagnostics Program                                               ;;
;;  Uses EMU_LINK interface to query jzIntv for raw event queue data.       ;;
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
            CFGVAR  "name" = "Event Diagnostics v1"
            CFGVAR  "short_name" = "Event Diags v1"
            CFGVAR  "year" = 2006
            CFGVAR  "author" = "Joe Zbiciak"
            CFGVAR  "license" = "GPLv2+"

;; ======================================================================== ;;
;;  Scratch Memory                                                          ;;
;; ======================================================================== ;;
            ORG     $100, $100, "-RWBN"
ISRVEC      RMB     2
BGC         RMB     1
EVTNAM      RMB     20

;; ======================================================================== ;;
;;  System Memory                                                           ;;
;; ======================================================================== ;;
            ORG     $300, $300, "-RWBN"
STACK       RMB     32
CURPOS      RMB     1



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

MACRO       SETBG   b,  r
            MVII    #C_%b%, %r%
            MVO     %r%,    BGC
ENDM


;; ======================================================================== ;;
;;  TITLE  -- Display our modified title screen & copyright date.           ;;
;; ======================================================================== ;;
TITLE:      PROC
            BYTE    106, 'Event Diagnostics v1', 0
           
MAIN:
            MVII    #$100,  R4
            MVII    #$260,  R1
            CALL    FILLZERO

            SETISR  ISRINIT,R0

            MVII    #STACK, R6

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
            SETBG   RED, R0      ;0123456789012345678901234567890123456789
            PRINT_CSTK 5, 3, YEL,   "Requires jzIntv    EMU-LINK support."
            DECR    PC

@@ok:
            SETBG   LBL, R0       ;01234567890123456789
            PRINT_CSTK 0, 0, BLK, "Event Diagnostics v1"

            MVII    #60,    R0
            MVII    #$800,  R1
            MVII    #disp_ptr(2,0), R4
@@msglp:    MVO@    R1,     R4
            ADDI    #8,     R1
            DECR    R0
            BNEQ    @@msglp

            MVII    #disp_ptr(6,0), R0
            MVO     R0,     CURPOS

@@main_loop:
            ;; ------------------------------------------------------------ ;;
            ;;  Spin until an event arrives.                                ;;
            ;; ------------------------------------------------------------ ;;
@@wait_evt: CALL    DO_EL
            DECLE   EVTNAM

            INCR    R0
            BEQ     @@wait_evt

            PSHR    R1

            ;; ------------------------------------------------------------ ;;
            ;;  Scroll up the screen for new event if needed.               ;;
            ;; ------------------------------------------------------------ ;;
            MVI     CURPOS, R4
            CMPI    #$2F0,  R4
            BNC     @@no_scroll

            CALL    SCROLL

@@no_scroll

            ;; ------------------------------------------------------------ ;;
            ;;  Print Up/Down indicator.                                    ;;
            ;; ------------------------------------------------------------ ;;
            MVII    #gen_cstk_card(ASC("U",0)-$20, GROM, Black, NoAdv), R0
            PULR    R1
            TSTR    R1
            BEQ     @@up
            MVII    #gen_cstk_card(ASC("D",0)-$20, GROM, Black, NoAdv), R0
@@up
            MVO@    R0,     R4
            INCR    R4

            ;; ------------------------------------------------------------ ;;
            ;;  *Grumble*  Fix underscores.                                 ;;
            ;; ------------------------------------------------------------ ;;
            MVII    #EVTNAM,    R5
            MVII    #$EF,       R1

@@us_fix:   MVI@    R5,         R0
            CMPI    #$5F,       R0
            BNEQ    @@not_us

            DECR    R5
            MVO@    R1,         R5

@@not_us:   TSTR    R0
            BNEQ    @@us_fix

            ;; ------------------------------------------------------------ ;;
            ;;  Print the (corrected) name.                                 ;;
            ;; ------------------------------------------------------------ ;;
            MVII    #EVTNAM,            R0
            MVII    #__CSTK.GROM_BLK,   R1
            CALL    PRINT.R

            MVI     CURPOS, R1
            ADDI    #20,    R1
            MVO     R1,     CURPOS

            CLRR    R0
            INCR    PC
@@eol       MVO@    R0,     R4
            CMPR    R4,     R1
            BNEQ    @@eol

            B       @@main_loop
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
            JR      R5
            ENDP

;; ======================================================================== ;;
;;  ISRINIT                                                                 ;;
;; ======================================================================== ;;
ISRINIT     PROC

            DIS

            MVII    #TTXT,  R4
            MVII    #$3800, R5
            MVII    #TTXT.end-TTXT, R0

@@loop:     MVI@    R4,     R1
            MVO@    R1,     R5
            SWAP    R1
            MVO@    R1,     R5
            DECR    R0
            BNEQ    @@loop

            SETISR  ISR,    R0

            JE      $1014
            ENDP

;; ======================================================================== ;;
;;  SCROLL                                                                  ;;
;; ======================================================================== ;;
SCROLL      PROC
            PSHR    R5

            MVII    #$200,  R4
            MVII    #$214,  R5
            MVII    #220/10,R1

@@loop
            REPEAT  10
            MVI@    R5,     R0
            MVO@    R0,     R4
            ENDR

            DECR    R1
            BNEQ    @@loop

            MVII    #$2F0 - 20, R4
            MVO     R4,     CURPOS

            PULR    PC
            ENDP

;; ------------------------------------------------------------------------ ;;
;;  Event Emu-Link on Major API #9.                                         ;;
;;                                                                          ;;
;;  The event Emu-Link API is very simple:                                  ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;      R2 == 0x0000:  Just return event number and up/down                 ;;
;;      R2 != 0x0000:  Try to write ASCII name of event @R1.                ;;
;;                     ASCII names are bounded to 18 chars + NUL.           ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;      R0:   0xFFFF = No events, otherwise event #                         ;;
;;      R1:   0 = UP, 1 = DOWN                                              ;;
;; ------------------------------------------------------------------------ ;;


;; ======================================================================== ;;
;;  DO_EL   Make an event emu_link call.  API follows call.                 ;;
;;  DO_EL.1 Make an event emu_link call.  API in R2.                        ;;
;; ======================================================================== ;;
DO_EL       PROC
            MVI@    R5,     R2
@@1         MVII    #$4A5A, R0
            PSHR    R5
            PSHR    R2
            MVII    #9,     R1  ; event subsystem on major API #9
            SETC
            SIN
            BC      @@failed
            DECR    R6          ; dump saved EL call #
            PULR    PC          ; return

@@failed:   
            CALL    CLRSCR
            SETBG   RED, R0

            MVII    #disp_ptr(7, 3), R4
            MVII    #__CSTK.GROM_YEL, R1
            CALL    HEX16
                                   ;0123456789012345678901234567890123456789
            PRINT_CSTK  5, 0, YEL, "EMU-LINK call failed"
            PULR    R0
            MVII    #disp_ptr(7, 8), R4
            MVII    #__CSTK.GROM_YEL, R1
            CALL    HEX16

            PULR    R0
            MVII    #disp_ptr(7,13), R4
            MVII    #__CSTK.GROM_YEL, R1
            CALL    HEX16

            B       $
            ENDP

;; ======================================================================== ;;
;;  LIBRARY INCLUDES                                                        ;;
;; ======================================================================== ;;
            INCLUDE "library/print.asm"
            INCLUDE "library/fillmem.asm"
            INCLUDE "library/prnum16.asm"
            INCLUDE "library/hexdisp.asm"

;; ======================================================================== ;;
;;  Title screen text rendered as a graphic.                                ;;
;; ======================================================================== ;;
TTXT        PROC
            gfx_start
            gfx_row "......#."
            gfx_row ".######."
            gfx_row "...#..##"
            gfx_row "...#..#."
            gfx_row "...#..#."
            gfx_row "...#..#."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "..#....."
            gfx_row "........"
            gfx_row "#.#.###."
            gfx_row "#.#.##.."
            gfx_row "#.#...#."
            gfx_row "#.#.###."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "..##.###"
            gfx_row "..#..###"
            gfx_row "..#..#.."
            gfx_row "..#..###"
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row ".###.###"
            gfx_row ".#.#.#.#"
            gfx_row ".#.#.#.#"
            gfx_row ".###.###"
            gfx_row ".#......"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "....#..."
            gfx_row ".##.##.#"
            gfx_row ".#..#..#"
            gfx_row ".#..#..."
            gfx_row ".#..##.#"
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row ".....#.."
            gfx_row "##...##."
            gfx_row "#....#.."
            gfx_row ".#...#.."
            gfx_row "##...##."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "#......."
            gfx_row "#......."
            gfx_row "###.###."
            gfx_row "#.#.###."
            gfx_row "#.#.#..."
            gfx_row "#.#.###."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "..###.#."
            gfx_row "..###.#."
            gfx_row "..#...#."
            gfx_row "..###..#"
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "#.###.##"
            gfx_row "#.###.#."
            gfx_row "#.#...#."
            gfx_row "..###.#."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "..#....."
            gfx_row "#.##.###"
            gfx_row "#.#..##."
            gfx_row "#.#....#"
            gfx_row "#.##.###"
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "...#...."
            gfx_row "...#.###"
            gfx_row "...#...#"
            gfx_row "...#..#."
            gfx_row "...#.###"
            gfx_row "..##...."
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row ".#......"
            gfx_row ".#.....#"
            gfx_row ".#.###.#"
            gfx_row ".#.#.#.#"
            gfx_row ".#.#.#.#"
            gfx_row ".#.#.#.#"
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "#.#.#..."
            gfx_row "..#.#..."
            gfx_row "..#.#..."
            gfx_row "#..#...."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "###.###."
            gfx_row "##..###."
            gfx_row "..#.#..."
            gfx_row "###.###."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "###.###."
            gfx_row "###.##.."
            gfx_row "#.....#."
            gfx_row "###.###."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "......#."
            gfx_row "......#."
            gfx_row "........"
            gfx_row "........"
            gfx_row "#......."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "#..#.#.."
            gfx_row "#..#.#.."
            gfx_row "#..#...."
            gfx_row "#..#...."
            gfx_row ".##....."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "..##.###"
            gfx_row "...#.#.#"
            gfx_row ".###.#.#"
            gfx_row ".###.#.#"
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "...#...."
            gfx_row "...#...#"
            gfx_row ".###...#"
            gfx_row ".#.#...."
            gfx_row ".#.#...."
            gfx_row ".###...."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row ".###..#."
            gfx_row ".#..#.#."
            gfx_row ".#..#..."
            gfx_row ".#..#..."
            gfx_row ".###...."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "..#....."
            gfx_row "..#....."
            gfx_row "..###.##"
            gfx_row "..#.#.##"
            gfx_row "..#.#.#."
            gfx_row "..###.##"
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "..##...."
            gfx_row "..#....."
            gfx_row "#######."
            gfx_row "#.#.#.#."
            gfx_row "..#.#.#."
            gfx_row "#.#.###."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "##.###.."
            gfx_row "#..###.."
            gfx_row "#..#...."
            gfx_row "#..###.."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row ".###..##"
            gfx_row ".###...#"
            gfx_row ".#...###"
            gfx_row ".###.###"
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row ".....#.."
            gfx_row ".....#.."
            gfx_row "..##.###"
            gfx_row ".#...#.#"
            gfx_row ".#...#.#"
            gfx_row "..##.#.#"
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "...###.#"
            gfx_row "...###.#"
            gfx_row "...#...#"
            gfx_row "...###.."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row ".#.###.#"
            gfx_row ".#.###.#"
            gfx_row ".#.#...#"
            gfx_row "#..###.#"
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "...#...."
            gfx_row "##.##..."
            gfx_row ".#.#...."
            gfx_row ".#.#...."
            gfx_row ".#.##..."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "###..##."
            gfx_row "##....#."
            gfx_row "..#.###."
            gfx_row "###.###."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "#.#.###."
            gfx_row "#.#.##.."
            gfx_row "#.#...#."
            gfx_row ".##.###."
            gfx_row "..#....."
            gfx_row "###....."
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "..#.#.#."
            gfx_row "..#.#.#."
            gfx_row "..##.##."
            gfx_row "...#.#.."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "#......."
            gfx_row "#......."
            gfx_row "###.###."
            gfx_row "#.#.###."
            gfx_row "#.#.#..."
            gfx_row "#.#.###."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "...#...."
            gfx_row "#..#...."
            gfx_row "##.###.#"
            gfx_row "#..#.#.#"
            gfx_row "#..#.#.#"
            gfx_row "##.#.#.#"
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "##.##..."
            gfx_row "##.#...."
            gfx_row "...#...."
            gfx_row "##.#...."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "#......."
            gfx_row "..#..#.."
            gfx_row "#.##.#.#"
            gfx_row "#.#....#"
            gfx_row "#.#....."
            gfx_row "#.##...#"
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row ".......#"
            gfx_row ".....#.#"
            gfx_row "##...#.#"
            gfx_row "#......#"
            gfx_row ".#.....#"
            gfx_row "##.....#"
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row ".#.###.#"
            gfx_row "#..###.#"
            gfx_row ".#.#...#"
            gfx_row ".#.###.."
            gfx_row "........"
            gfx_row ".......#"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row ".#....#."
            gfx_row ".#.##.#."
            gfx_row ".#....#."
            gfx_row "##....##"
            gfx_row ".#......"
            gfx_row "##......"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "......#."
            gfx_row "#.###.#."
            gfx_row "#.#.#..."
            gfx_row "#.#.#..."
            gfx_row "#.###..."
            gfx_row "..#....."
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row ".###.##."
            gfx_row ".#.#.#.."
            gfx_row ".#.#.#.."
            gfx_row ".###.#.."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "....#..."
            gfx_row "..#.#..."
            gfx_row "..#.#.#."
            gfx_row "....##.."
            gfx_row "....#.#."
            gfx_row "....#.#."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "###.#.#."
            gfx_row "###.#.#."
            gfx_row "#...#.#."
            gfx_row "###..##."
            gfx_row "......#."
            gfx_row "....###."
            gfx_flush
            gfx_start
            gfx_row "....#..."
            gfx_row "....#..."
            gfx_row "..###.##"
            gfx_row "..#.#.#."
            gfx_row "..#.#.#."
            gfx_row "..###.##"
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "#.#.#.#."
            gfx_row "#.#.#.#."
            gfx_row "#.##.##."
            gfx_row "#..#.#.."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "......#."
            gfx_row "###...#."
            gfx_row "#.#....."
            gfx_row "#.#....."
            gfx_row "#.#.#..."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "....#..#"
            gfx_row "....#..#"
            gfx_row "....#..#"
            gfx_row "....#..#"
            gfx_row ".....##."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row ".###.###"
            gfx_row ".##..###"
            gfx_row "...#.#.."
            gfx_row ".###.###"
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "......#."
            gfx_row "...#..#."
            gfx_row "...##.##"
            gfx_row "...#..#."
            gfx_row "...#..#."
            gfx_row "...##.#."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "#.###.##"
            gfx_row "#.###.##"
            gfx_row "#.#....."
            gfx_row "#.###.##"
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "#.###..."
            gfx_row "..###..."
            gfx_row "#.#....."
            gfx_row "#.###..."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "###..##."
            gfx_row "#.#...#."
            gfx_row "#.#.###."
            gfx_row "#.#.###."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "#####..#"
            gfx_row "#.#.#..#"
            gfx_row "#.#.#..#"
            gfx_row "#.#.#..#"
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "##.###.."
            gfx_row "##.##..."
            gfx_row ".....#.."
            gfx_row "##.###.."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row ".#......"
            gfx_row ".##.###."
            gfx_row ".#..#.#."
            gfx_row ".#..#.#."
            gfx_row ".##.###."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "..##.###"
            gfx_row "..#..###"
            gfx_row "..#..#.."
            gfx_row "..#..###"
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row ".#...#.."
            gfx_row ".#......"
            gfx_row ".###.#.#"
            gfx_row ".#.#.#.#"
            gfx_row ".#.#.#.#"
            gfx_row ".###.#.#"
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row ".....#.."
            gfx_row ".....#.."
            gfx_row "##.###.."
            gfx_row ".#.#.#.."
            gfx_row ".#.#.#.."
            gfx_row ".#.###.."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row ".#......"
            gfx_row ".#.###.#"
            gfx_row ".#.#.#.#"
            gfx_row ".#.#.#.#"
            gfx_row ".#.#.#.#"
            gfx_row ".......#"
            gfx_row ".......#"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row ".......#"
            gfx_row "##.#.#.#"
            gfx_row ".#.#.#.#"
            gfx_row ".#.#.#.#"
            gfx_row "##.###.#"
            gfx_row "........"
            gfx_row "........"
            gfx_flush
            gfx_start
            gfx_row "........"
            gfx_row "........"
            gfx_row "#.###..."
            gfx_row "..##...."
            gfx_row "....#..."
            gfx_row "#.###.#."
            gfx_row "........"
            gfx_row "........"
            gfx_flush
@@end:      
            ENDP
