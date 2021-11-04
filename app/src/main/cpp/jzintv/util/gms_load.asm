;;==========================================================================;;
;; GMS Loader Routine.                                                      ;;
;; Copyright 2001, Joe Zbiciak, intvnut AT gmail.com.                       ;;
;; http://spatula-city.org/~im14u2c/intv/                                   ;;
;;==========================================================================;;

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
;*                   Copyright (c) 2000, Joseph Zbiciak                     *;
;* ======================================================================== *;

            ROMW    16

CPUREGS     EQU $0040
DISPMODE    EQU $004E
PROGRAM     EQU $C800
SCRATCH     EQU $0500
WINDOW      EQU $0800

WBANK       EQU WINDOW SHR 8
CBANK       EQU CPUREGS SHR 8

STACK       EQU SCRATCH + $100

REGS        STRUCT  CPUREGS + WINDOW
@@0         EQU     $ + 0
@@1         EQU     $ + 1
@@2         EQU     $ + 2
@@3         EQU     $ + 3
@@4         EQU     $ + 4
@@5         EQU     $ + 5
@@6         EQU     $ + 6
@@7         EQU     $ + 7
@@stat      EQU     $ + 8
@@intr      EQU     $ + 9
@@b0        EQU     $ + 10
@@b1        EQU     $ + 11
@@b2        EQU     $ + 12

@@isrv      EQU     $ + 13
@@mode      EQU     $ + 14
@@visb      EQU     $ + 15
            ENDS

            ORG     PROGRAM

;; ======================================================================== ;;
;;  GMSLOADER -- This is where we start.                                    ;;
;; ======================================================================== ;;
GMSLOADER   PROC

            MVII    #GMSMAIN,   R0  ; \
            MVO     R0,     $100    ;  |__ Initialize via an interrupt handler
            SWAP    R0              ;  |
            MVO     R0,     $101    ; /
            EIS                     ;
            DECR    PC              ; Spin till interrupt happens.

            ENDP

STUB        JR      R5

;; ======================================================================== ;;
;;  GMSMAIN   -- This is where the actual action begins.                    ;;
;; ======================================================================== ;;
GMSMAIN     PROC

            DIS

            MVII    #STACK, R6      ; Put our stack in scratch memory.

            MVII    #STUB,  R0      ; \
            MVO     R0,     $100    ;  |   Stub out interrupt handler for now.
            SWAP    R0              ;  |-- We need to allow interrupts to 
            MVO     R0,     $101    ;  |   happen otherwise STIC gets unhappy.
            EIS                     ; /

            ;; Copy over most of the important ranges:
            CALL    COPYRAM
            DECLE   $0800,  $4000   ; $4000 - $47FF
            DECLE   $0020,  $0000   ; $0000 - $001F
            DECLE   $000B,  $0028   ; $0028 - $0032
            DECLE   $0010,  $00F0   ; $00F0 - $00FF
            DECLE   $025E,  $0102   ; $0102 - $035F
            DECLE   $0200,  $3800   ; $3800 - $39FF
            DECLE   0

            ;; Copy back the 3 words that go at $7000:
            MVII    #WBANK, R1      ; \
            MVII    #CBANK, R2      ;  |-- Point window at save area
            CALL    IC_SETBANK      ; /

            MVI     WINDOW+0, R0    ; \
            MVI     WINDOW+1, R3    ;  |-- Read three bytes in.
            MVI     WINDOW+2, R4    ; / 

            MVII    #WBANK, R1      ; \
            MVII    #$70,   R2      ;  |-- Point window at $7000
            CALL    IC_SETBANK      ; /

            MVO     R0, WINDOW+0    ; \
            MVO     R3, WINDOW+1    ;  |-- Write three bytes out.
            MVO     R4, WINDOW+2    ; /

            ;; Point the window at our register save area.
            MVII    #WBANK, R1      ; \
            MVII    #CBANK, R2      ;  |-- Point window at CPUREGS
            CALL    IC_SETBANK      ; /

            ;; Prepare for final launch.  We do that from an ISR.
            MVII    #GMSLAUNCH, R1
            MVO     R1,     $100
            SWAP    R1
            MVO     R1,     $101
            DECR    PC              ; spin

            ENDP

;; ======================================================================== ;;
;;  GMSLAUNCH -- Actually launch the game.                                  ;;
;; ======================================================================== ;;
GMSLAUNCH   PROC
            DIS

            ;; Is display visible?
            MVI     REGS.visb,  R0
            TSTR    R0
            BEQ     @@notvis
            MVO     R0,     $20
@@notvis:
            
            ;; Color stack or FG/BG
            MVI     $21,        R0  ; Assume color-stack
            MVI     REGS.mode,  R0
            TSTR    R0
            BEQ     @@colstk
            MVO     R0,         $21 ; FG/BG
@@colstk:

            ;; Restore ISR vector
            MVI     REGS.isrv,  R0
            MVO     R0,         $100
            SWAP    R0
            MVO     R0,         $101

            ;; Restore registers R1 - R6
            MVII    #REGS.1,    R5
            MVI@    R5,         R1
            MVI@    R5,         R2
            MVI@    R5,         R3
            MVI@    R5,         R4
            INCR    R5
            MVI@    R5,         R6
            MVI     REGS.5,     R5

            ;; Separate return paths based on EIS or DIS:
            MVI     REGS.intr,  R0
            ANDI    #1,         R0
            BEQ     @@do_eis

            ;; Launch w/out EIS.
            MVI     REGS.stat,  R0
            RSWD    R0
            MVI     REGS.0,     R0
            MVI     REGS.7,     R7

            ;; Launch with EIS.
@@do_eis:   MVI     REGS.stat,  R0
            RSWD    R0
            MVI     REGS.0,     R0
            EIS
            MVI     REGS.7,     R7

            ENDP


;;==========================================================================;;
;;  COPYRAM  -- Copies data from Intellicart space to CPU space.            ;;
;;                                                                          ;;
;;  This performs a copy from the Intellicart's private memory space to     ;;
;;  the Intellivision's memory space.  The copy occurs via the bankswitch   ;;
;;  window in chunks no larger than 2048 words.                             ;;
;;                                                                          ;;
;;  Inputs:                                                                 ;;
;;      R5 -- List of ranges to copy, stored as "length, addr" pairs.       ;;
;;            Length == 0 terminates.  Function will return immediately     ;;
;;            after the 0 word.                                             ;;
;;                                                                          ;;
;;  Outputs:                                                                ;;
;;      R0 -- Untouched.                                                    ;;
;;      R1, R2, R3, R4, R5 -- Trashed.                                      ;;
;;==========================================================================;;
COPYRAM     PROC

            B       @@first 

@@o_loop:   MVI@    R5,     R4      ; Get address.
            PSHR    R5              ; Save our table ptr.

@@m_loop:
            MVII    #WBANK, R1      ; \
            MOVR    R4,     R2      ;  |   Point window to start of range
            SWAP    R2              ;  |-- (within 256 word granularity).
            ANDI    #$FF,   R2      ;  |
            CALL    IC_SETBANK      ; /

            MOVR    R4,     R5      ; \
            ANDI    #$FF,   R5      ;  |-- Point source pointer into window.
            ADDI    #WINDOW,R5      ; /

            MOVR    R5,     R2          ; \
            ADDR    R3,     R2          ;  |   Make sure copy loop stays 
            CMPI    #WINDOW + 2048, R2  ;  |__ inside the window.  If 
            BNC     @@cnt_ok            ;  |   "start+count > end_of_window",
            MVII    #WINDOW+2048-1, R2  ;  |   count = end_of_window - start.
@@cnt_ok:   SUBR    R5,     R2          ; /

            SUBR    R2,     R3      ; Subtract this pass' count from total

@@i_loop:   MVI@    R5,     R1      ; \
            MVO@    R1,     R4      ;  |__ Copy as much data as we can.
            DECR    R2              ;  |
            BNEQ    @@i_loop        ; /

            TSTR    R3              ;
            BNEQ    @@m_loop        ; Keep going until it's all copied.
            
            PULR    R5              ; Restore table ptr.
@@first:    MVI@    R5,     R3      ; Get length byte
            TSTR    R3              ; Is it 0?
            BNEQ    @@o_loop        ; No:   Lets do this range.
            JR      R5              ; Yes:  Return

            ENDP


;;==========================================================================;;
;; IC_SETBANK    -- Sets a bank for a given 2K page of memory.              ;;
;;                                                                          ;;
;;  Inputs:                                                                 ;;
;;    R1 -- 5 MSBs of bank address in bits 7..3                             ;;
;;    R2 -- New bank address to point to in 8 LSBs.                         ;;
;;    R5 -- Return address                                                  ;;
;;                                                                          ;;
;;  Outputs:                                                                ;;
;;    R1 -- Address of Intellicart base control register.                   ;;
;;    R2 -- Trashed.                                                        ;;
;;    R5 -- Trashed.                                                        ;;
;;                                                                          ;;
;;  Example:                                                                ;;
;;    To remap 0x7800-0x7FFF in the Inty's address space to point to        ;;
;;    0x4000-0x47FF in the cart's address space, pass in the following      ;;
;;    parameters:  R1 = 0x0078, R2 = 0x0040.                                ;;
;;                                                                          ;;
;;==========================================================================;;
IC_SETBANK  PROC
            PSHR    R5                  ; Save return address.
            MOVR    R1,     R5          ; Get Hi/Lo 2K bit from address in R5
            ADDR    R5,     R5
            ANDI    #$10,   R5

            SLR     R1,     2           ; Put 4 MSBs of page in 4 LSBs of R0
            SLR     R1,     2
            ANDI    #$0F,   R1          ; Keep only the four LSBs.
            ADDR    R5,     R1          ; Merge bits: 76543210 -> xxxx37654
            ADDI    #$40,   R1          ; Point it at Intellicart ctrl regs.
            MVO@    R2,     R1          ; Write to control register.
            PULR    PC                  ; Return
            ENDP

