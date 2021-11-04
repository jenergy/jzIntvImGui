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
            CFGVAR  "name" = "JLP Feature Test"
            CFGVAR  "year" = 2009
            CFGVAR  "author" = "Joe Zbiciak"
            CFGVAR  "jlp" = 3
            CFGVAR  "jlp_flash" = 100
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
            BYTE    109, 'JLP Feature Test', 0
           
MAIN:
            MVII    #$100,  R4
            MVII    #$260,  R1
            CALL    FILLZERO

            SETISR  ISRINIT,R0

            MVII    #STACK, R6

            EIS

            PRINT_CSTK 0, 0, White, "Memory fill..."
            ; Make sure all RAM is addressable
            MVII    #$8040, R4
            MVII    #$9F80 - $8040, R0
@@fill:
            MVO@    R0,     R4
            DECR    R0
            BNEQ    @@fill

            MVII    #$8040, R4
            MVII    #$9F80 - $8040, R0
@@comp:
            CMP@    R4,     R0
            BNEQ    @@fill_fail
            DECR    R0
            BNEQ    @@comp

            PRINT_CSTK 1, 0, White, "SDBD ROM check..."

            ; Check SDBD works for all of ROM
            MVII    #ROMHDR, R4
            MVII    #(END_OF_ROM - ROMHDR) / 4, R1
            CLRR    R0
@@sdbd_rom_loop:
            SDBD
            ADD@    R4,     R0
            SDBD
            ADD@    R4,     R0
            DECR    R1
            BNEQ    @@sdbd_rom_loop

            PSHR    R0


            MVII    #ROMHDR, R4
            MVII    #(END_OF_ROM - ROMHDR) / 4, R1
            CLRR    R0
            MVII    #$FF,   R5
                
@@sdbd_rom_loop2:       ; do it again w/out SDBD
            MVI@    R4,     R2
            ANDR    R5,     R2
            ADDR    R2,     R0

            MVI@    R4,     R2
            ANDR    R5,     R2
            SWAP    R2
            ADDR    R2,     R0

            MVI@    R4,     R2
            ANDR    R5,     R2
            ADDR    R2,     R0

            MVI@    R4,     R2
            ANDR    R5,     R2
            SWAP    R2
            ADDR    R2,     R0

            DECR    R1
            BNEQ    @@sdbd_rom_loop2 

            PULR    R1

            CMPR    R1,     R0
            BEQ     @@sdbd_rom_ok

            PSHR    R1
            PSHR    R0

            PRINT_CSTK 1, 0, Red, "SDBD ROM CHECK FAIL"

            PULR    R0
            MVII    #2,     R1
            MVII    #disp_ptr(2, 0), R4
            CALL    HEX16

            PULR    R1
            MVII    #2,     R1
            MVII    #disp_ptr(2, 5), R4
            CALL    HEX16

            DECR    PC


@@sdbd_rom_ok:



            PRINT_CSTK 2, 0, White, "SDBD RAM check..."

            ; Check SDBD works for all of RAM
            MVII    #$8040, R4
            MVII    #($9F7F - $8040)/4, R1
            CLRR    R0
@@sdbd_ram_loop:
            SDBD
            ADD@    R4,     R0
            SDBD
            ADD@    R4,     R0
            DECR    R1
            BNEQ    @@sdbd_ram_loop

            PSHR    R0


            MVII    #$8040, R4
            MVII    #($9F7F - $8040)/4, R1
            CLRR    R0
            MVII    #$FF,   R5
                
@@sdbd_ram_loop2:       ; do it again w/out SDBD
            MVI@    R4,     R2
            ANDR    R5,     R2
            ADDR    R2,     R0

            MVI@    R4,     R2
            ANDR    R5,     R2
            SWAP    R2
            ADDR    R2,     R0

            MVI@    R4,     R2
            ANDR    R5,     R2
            ADDR    R2,     R0

            MVI@    R4,     R2
            ANDR    R5,     R2
            SWAP    R2
            ADDR    R2,     R0

            DECR    R1
            BNEQ    @@sdbd_ram_loop2 

            PULR    R1

            CMPR    R1,     R0
            BEQ     @@sdbd_ram_ok

            PSHR    R1
            PSHR    R0

            PRINT_CSTK 2, 0, Red, "SDBD RAM CHECK FAIL"

            PULR    R0
            MVII    #2,     R1
            MVII    #disp_ptr(3, 0), R4
            CALL    HEX16

            PULR    R1
            MVII    #2,     R1
            MVII    #disp_ptr(3, 5), R4
            CALL    HEX16

            DECR    PC


@@sdbd_ram_ok:


            CALL    CLRSCR

            ; Check multiply/divide
            CALL    MKDEAD

            MVII    #$1234, R0
            MVII    #$ABCD, R1

            ;   (s16)$1234 x (s16)$ABCD
            MVO     R0,     $9F80
            MVO     R1,     $9F81
            MVII    #$200,  R4
            CALL    PR_RSLT
            DECLE   $FA03, $4FA4

            ;   (s16)$ABCD x (s16)$1234
            MVO     R0,     $9F81
            MVO     R1,     $9F80
            MVII    #$20A,  R4
            CALL    PR_RSLT
            DECLE   $FA03, $4FA4

            ;   (s16)$1234 x (u16)$ABCD
            MVO     R0,     $9F82
            MVO     R1,     $9F83
            MVII    #$214,  R4
            CALL    PR_RSLT
            DECLE   $0C37, $4FA4

            ;   (s16)$ABCD x (u16)$1234
            MVO     R0,     $9F83
            MVO     R1,     $9F82
            MVII    #$21E,  R4
            CALL    PR_RSLT
            DECLE   $FA03, $4FA4

            ;   (u16)$1234 x (s16)$ABCD
            MVO     R0,     $9F84
            MVO     R1,     $9F85
            MVII    #$228,  R4
            CALL    PR_RSLT
            DECLE   $FA03, $4FA4

            ;   (u16)$ABCD x (s16)$1234
            MVO     R0,     $9F85
            MVO     R1,     $9F84
            MVII    #$232,  R4
            CALL    PR_RSLT
            DECLE   $0C37, $4FA4

            ;   (u16)$1234 x (u16)$ABCD
            MVO     R0,     $9F86
            MVO     R1,     $9F87
            MVII    #$23C,  R4
            CALL    PR_RSLT
            DECLE   $0C37, $4FA4

            ;   (u16)$ABCD x (u16)$1234
            MVO     R0,     $9F87
            MVO     R1,     $9F86
            MVII    #$246,  R4
            CALL    PR_RSLT
            DECLE   $0C37, $4FA4

            ;   (s16)$1234 / (s16)$ABCD
            MVO     R0,     $9F88
            MVO     R1,     $9F89
            MVII    #$250,  R4
            CALL    PR_RSLT
            DECLE   $1234, $0000

            ;   (s16)$ABCD / (s16)$1234
            MVO     R0,     $9F89
            MVO     R1,     $9F88
            MVII    #$25A,  R4
            CALL    PR_RSLT
            DECLE   $F49D, $FFFC

            ;   (u16)$1234 / (u16)$ABCD
            MVO     R0,     $9F8A
            MVO     R1,     $9F8B
            MVII    #$264,  R4
            CALL    PR_RSLT
            DECLE   $1234, $0000

            ;   (u16)$ABCD / (u16)$1234
            MVO     R0,     $9F8B
            MVO     R1,     $9F8A
            MVII    #$26E,  R4
            CALL    PR_RSLT
            DECLE   $07F9, $0009

            ;   Make sure 'rand' is at least a little random
            MVI     $9FFE,  R0
            CMP     $9FFE,  R0
            BNEQ    @@rand_ok
            CMP     $9FFE,  R0
            BNEQ    @@rand_ok

            PSHR    R0
            PRINT_CSTK 6, 0, Red,   "Rand broken: "
            PULR    R0
            MVII    #2,     R1
            CALL    HEX16
            B       @@done_rand

@@rand_ok:  PRINT_CSTK 6, 0, White, "Rand appears OK"

@@done_rand:
            CALL    WAIT
            DECLE   1 * 60

            ; Test the CRC-16 acceleration
            MVII    #$C0ED, R0
            MVO     R0,     $9FFD       ; initialize CRC-16 poly

            MVI     $9FFD,  R1
            CMPR    R1,     R0
            BNEQ    @@crc16_bad

            MVII    #$BABE, R0
            MVO     R0,     $9FFC       ; Update CRC-16
            
            MVII    #$D216, R0
            MVI     $9FFD,  R1
            CMPR    R1,     R0
            BNEQ    @@crc16_bad

            MVII    #$5000, R4
            MVII    #END_OF_ROM - $5000, R1
            CALL    CRC16
            PSHR    R0

            MVII    #$5000, R4
            MVII    #END_OF_ROM - $5000, R1
            CALL    CRC16
            PULR    R1
            CMPR    R1,     R0

            BNEQ    @@crc16_bad
                                    ;012345678901234567
            PRINT_CSTK 7, 0, White, "CRC-16 appears OK"
            B       @@crc16_done
@@crc16_bad:
            PSHR    R0
            PSHR    R1
            
            PRINT_CSTK 7, 0, Red,   "CRC-16: "

            PULR    R0
            MVII    #2,     R1
            CALL    HEX16

            INCR    R4
            PULR    R0
            MVII    #2,     R1
            CALL    HEX16

@@crc16_done:
            CALL    WAIT
            DECLE   1 * 60

            ; Test zero handshake at $9FFF
            MVI     $9FFF,  R0
            TSTR    R0
            BEQ     @@zero_ok

            PSHR    R0
            PRINT_CSTK 8, 0, Red, "Bad zero: "
            PULR    R0
            MVII    #2,     R1
            CALL    HEX16
            B       @@zero_done

@@zero_ok:
            PRINT_CSTK 8, 0, White, "Zero handshake ok"
@@zero_done:
            CALL    WAIT
            DECLE   1 * 60

            ;   Try to fill all the SG slots
            PRINT_CSTK 9, 0, White, "Erasing flash:  "
            PSHR    R4
            MVI     $8023,  R0
            SUBI    #8,     R0
@@erase_flash:
            PULR    R4
            PSHR    R4
            ADDI    #8,     R0
            PSHR    R0
            MVII    #C_YEL, R1
            CALL    HEX16

            PULR    R0
            PSHR    R0
            MVII    #2,     R2
            CALL    SG_UPD
            PULR    R0
            CMP     $8024,  R0
            BNC     @@erase_flash


            CALL    WAIT
            DECLE   1 * 60

            PULR    R4

            PRINT_CSTK 10,0, White, "Filling flash:  "
            PSHR    R4
            MVI     $8023,  R0
            DECR    R0
@@fill_flash:
            PULR    R4
            PSHR    R4
            INCR    R0
            PSHR    R0
            MVII    #C_YEL, R1
            CALL    HEX16

            PULR    R0
            PSHR    R0

            MVII    #$9000, R5
            MVII    #96,    R2
@@fff       MVO@    R0,     R5

            SARC    R0
            BNC     @@fff_nop
            XORI    #$EDD1, R0
@@fff_nop
            DECR    R2
            BNEQ    @@fff

            PULR    R0
            PSHR    R0
            MVII    #$9000, R1
            MVII    #0,     R2
            CALL    SG_UPD
            PULR    R0
            CMP     $8024,  R0
            BNC     @@fill_flash

            CALL    WAIT
            DECLE   1 * 60

            PULR    R4

            PRINT_CSTK 11, 0, White, "Reading flash:  "

            PSHR    R4
            MVI     $8023,  R0
            DECR    R0
@@read_flash:
            PULR    R4
            PSHR    R4
            INCR    R0
            PSHR    R0
            MVII    #C_YEL, R1
            CALL    HEX16

            PULR    R0
            PSHR    R0

            MVII    #$9000, R5
            MVII    #96,    R2
            MVII    #$DEAD, R1
@@rfcx      MVO@    R1,     R5      ; set memory to DEAD before flash read
            DECR    R2
            BNEQ    @@rfcx

            MVII    #$9000, R1
            MVII    #1,     R2
            CALL    SG_UPD
            PULR    R0
            PSHR    R0

            MVII    #$9000, R5
            MVII    #96,    R2
@@rfc       CMP@    R5,     R0
            BNEQ    @@fail

            SARC    R0
            BNC     @@rfc_nop
            XORI    #$EDD1, R0
@@rfc_nop
            DECR    R2
            BNEQ    @@rfc

            PULR    R0

            CMP     $8024,  R0
            BNC     @@read_flash
            PULR    R4

            MVII    #($50-$20) *8 + C_GRN, R0
            MVO     R0,     $200 + 19

            DECR    PC

@@fail
            DECR    R5
            PSHR    R5
            MOVR    R5,         R0
            MVII    #$2EF - 5,  R4
            MVII    #C_RED,     R1
            CALL    HEX16

            MVII    #($46-$20) *8 + C_RED, R0
            MVO     R0,     $2EF

            MVII    #$200,  R4
            MVII    #16,    R3

            PULR    R5
@@floop:
            PSHR    R3

            MVI@    R5,     R0
            PSHR    R5
            MVII    #7,     R1
            CALL    HEX16
            PULR    R5

            CLRR    R0
            MVO@    R0,     R4

            MVI@    R5,     R0
            PSHR    R5
            MVII    #6,     R1
            CALL    HEX16
            PULR    R5

            CLRR    R0
            MVO@    R0,     R4

            PULR    R3
            DECR    R3
            BNEQ    @@floop

            DECR    PC

@@fill_fail:
            PSHR    R0
            DECR    R4
            MOVR    R4,     R0
            MVI@    R4,     R1
            PSHR    R1

            MVII    #disp_ptr(5,1), R4
            MVII    #C_RED, R1
            CALL    HEX16               ; show address

            PULR    R0                  ; show value read
            INCR    R4
            CALL    HEX16

            PULR    R0                  ; show expected value
            INCR    R4
            CALL    HEX16

                                 ;0123456789012345678
            PRINT_CSTK 0, 1, Red, "MEMORY FILL FAILED"

            DECR    PC

            ENDP

PR_RSLT     PROC
            PSHR    R0
            PSHR    R1

            MVI     $9F8F,  R0
            MVII    #7,     R1

            CMP@    R5,     R0
            BEQ     @@ok1
            MVII    #2,     R1
            
@@ok1:      PSHR    R5
            CALL    HEX16
            PULR    R5

            MVI     $9F8E,  R0
            MVII    #7,     R1

            CMP@    R5,     R0
            BEQ     @@ok2
            MVII    #2,     R1

@@ok2:      PSHR    R5
            CALL    HEX16

            CALL    MKDEAD

            PULR    R5

            PULR    R1
            PULR    R0
            JR      R5
            ENDP
           

MKDEAD      PROC
            PSHR    R5
            MVII    #$9F80, R5
            MVII    #16,    R1
            MVII    #$DEAD, R0
@@f         MVO@    R0,     R5
            DECR    R1
            BNEQ    @@f
            PULR    PC
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


SG_SYSRAM   SET     SGCODE

;; ======================================================================== ;;
;;  SG_UPD:  Update the save-game slots based on bitmap in R0.              ;;
;;                                                                          ;;
;;  INPUT                                                                   ;;
;;      R0  Slot number to operate on                                       ;;
;;      R1  Address to copy to/from in JLP RAM                              ;;
;;      R2  Command to invoke:                                              ;;
;;                                                                          ;;
;;            0 -- Copy JLP RAM to Flash                                    ;;
;;            1 -- Copy Flash to JLP RAM                                    ;;
;;            2 -- Erase flash sector                                       ;;
;;                                                                          ;;
;; ======================================================================== ;;
SG_UPD      PROC
            PSHR    R5
            JSRD    R5,         @@copy

            ;; === start of code that will run from RAM
            MVO@    R0,         R1      ; SG_SYSRAM + 0: initiate command
@@loop:     ADD@    R1,         PC      ; SG_SYSRAM + 1: Wait for JLP to return
            JR      R5                  ; SG_SYSRAM + 2:
            MVII    #$20,       R1      ; SG_SYSRAM + 3: \
            MVO@    R1,         R1      ; SG_SYSRAM + 5:  |- simple ISR
            JR      R5                  ; SG_SYSRAM + 6: /
            ;; === end of code that will run from RAM

@@cmdtbl:   DECLE   $802D,      $C0DE   ; Copy JLP RAM to flash row  
            DECLE   $802E,      $DEC0   ; Copy flash row to JLP RAM  
            DECLE   $802F,      $BEEF   ; Erase flash sector 

@@copy:     MVO     R1,         $8025   ; \_ Save SG arguments in JLP
            MVO     R0,         $8026   ; /
           
            MVII    #SG_SYSRAM, R4
            REPEAT  7       
            MVI@    R5,         R0      ; \_ Copy code fragment to System RAM
            MVO@    R0,         R4      ; /
            ENDR

            SLL     R2,         1       ; \_ Index into command table
            ADDR    R2,         R5      ; /
            MVI@    R5,         R1      ; Get command address
            MVI@    R5,         R0      ; Get unlock word
           
            MVO@    R6,         R4      ; save old stack pointer
            MOVR    R4,         R6      ; new stack pointer
           
            MVII    #$100,      R4      ; \
            SDBD                        ;  |_ Save old ISR on new stack
            MVI@    R4,         R2      ;  |
            PSHR    R2                  ; /
           
            MVII    #SG_SYSRAM + 3, R2  ; \
            MVO     R2,         $100    ;  |_ Set up new ISR in RAM
            SWAP    R2                  ;  |
            MVO     R2,         $101    ; /
           
            CLRR    R2                  ; Look for 0 in command reg afterward
            JSRE    R5,    SG_SYSRAM    ; Invoke the command
           
            PULR    R2                  ; \ 
            MVO     R2,         $100    ;  |_ Restore old ISR from new stack
            SWAP    R2                  ;  |
            MVO     R2,         $101    ; /
           
            PULR    R6                  ; Restore old SP
            PULR    PC                  ; Return
            ENDP


;; ======================================================================== ;;
;;  CRC16                                                                   ;;
;;                                                                          ;;
;;  Compute a CRC-16 on a block of memory.  Implements a right-shifting     ;;
;;  CRC with the polynomial 0xAD52.                                         ;;
;;                                                                          ;;
;;  The CRC16T table has the original datum XORed in with the precomputed   ;;
;;  poly value so that we don't need to mask it out of our CRC.  We can     ;;
;;  just SWAP and XOR and it'll cancel out.                                 ;;
;;                                                                          ;;
;;  INPUTS: CRC16                                                           ;;
;;      R1  Number of words to checksum                                     ;;
;;      R4  Base address to start checksum                                  ;;
;;                                                                          ;;
;;  INPUTS: CRC16.1                                                         ;;
;;      R0  Initial checksum                                                ;;
;;      R1  Number of words to checksum                                     ;;
;;      R4  Base address to start checksum                                  ;;
;;                                                                          ;;
;;  OUTPUTS                                                                 ;;
;;      R0  Final checksum                                                  ;;
;;      R4  Points past end of 4K segment                                   ;;
;;      R1..R3 unmodified                                                   ;;
;; ======================================================================== ;;

CRC16       PROC
            CLRR    R0
@@1:       
            PSHR    R5
            PSHR    R3
            PSHR    R2
            PSHR    R1
           
            MVII    #$FF,   R3
            MVII    #CRC16T,R5
@@loop:
            XOR@    R4,     R0      ;    8 Merge in next data word
           
            MOVR    R0,     R2      ;    6 \
            ANDR    R3,     R2      ;    6  |- Index into crc16t[].
            ADDR    R5,     R2      ;    6 /
            SWAP    R0              ;    6 Shift left by 8
            XOR@    R2,     R0      ;    8 XOR in CRC update for lower 8
           
            MOVR    R0,     R2      ;    6 \
            ANDR    R3,     R2      ;    6  |- Index into crc16t[].
            ADDR    R5,     R2      ;    6 /
            SWAP    R0              ;    6 Shift left by 8
            XOR@    R2,     R0      ;    8 XOR in CRC update for lower 8
           
            DECR    R1
            BNEQ    @@loop
           
            PULR    R1
            PULR    R2
            PULR    R3
            PULR    PC
            ENDP
           
CRC16T      PROC
            DECLE   $0000, $C035, $DACF, $1AFA, $EF3B, $2F0E, $35F4, $F5C1
            DECLE   $84D3, $44E6, $5E1C, $9E29, $6BE8, $ABDD, $B127, $7112
            DECLE   $5303, $9336, $89CC, $49F9, $BC38, $7C0D, $66F7, $A6C2
            DECLE   $D7D0, $17E5, $0D1F, $CD2A, $38EB, $F8DE, $E224, $2211
            DECLE   $A606, $6633, $7CC9, $BCFC, $493D, $8908, $93F2, $53C7
            DECLE   $22D5, $E2E0, $F81A, $382F, $CDEE, $0DDB, $1721, $D714
            DECLE   $F505, $3530, $2FCA, $EFFF, $1A3E, $DA0B, $C0F1, $00C4
            DECLE   $71D6, $B1E3, $AB19, $6B2C, $9EED, $5ED8, $4422, $8417
            DECLE   $16A9, $D69C, $CC66, $0C53, $F992, $39A7, $235D, $E368
            DECLE   $927A, $524F, $48B5, $8880, $7D41, $BD74, $A78E, $67BB
            DECLE   $45AA, $859F, $9F65, $5F50, $AA91, $6AA4, $705E, $B06B
            DECLE   $C179, $014C, $1BB6, $DB83, $2E42, $EE77, $F48D, $34B8
            DECLE   $B0AF, $709A, $6A60, $AA55, $5F94, $9FA1, $855B, $456E
            DECLE   $347C, $F449, $EEB3, $2E86, $DB47, $1B72, $0188, $C1BD
            DECLE   $E3AC, $2399, $3963, $F956, $0C97, $CCA2, $D658, $166D
            DECLE   $677F, $A74A, $BDB0, $7D85, $8844, $4871, $528B, $92BE
            DECLE   $2D52, $ED67, $F79D, $37A8, $C269, $025C, $18A6, $D893
            DECLE   $A981, $69B4, $734E, $B37B, $46BA, $868F, $9C75, $5C40
            DECLE   $7E51, $BE64, $A49E, $64AB, $916A, $515F, $4BA5, $8B90
            DECLE   $FA82, $3AB7, $204D, $E078, $15B9, $D58C, $CF76, $0F43
            DECLE   $8B54, $4B61, $519B, $91AE, $646F, $A45A, $BEA0, $7E95
            DECLE   $0F87, $CFB2, $D548, $157D, $E0BC, $2089, $3A73, $FA46
            DECLE   $D857, $1862, $0298, $C2AD, $376C, $F759, $EDA3, $2D96
            DECLE   $5C84, $9CB1, $864B, $467E, $B3BF, $738A, $6970, $A945
            DECLE   $3BFB, $FBCE, $E134, $2101, $D4C0, $14F5, $0E0F, $CE3A
            DECLE   $BF28, $7F1D, $65E7, $A5D2, $5013, $9026, $8ADC, $4AE9
            DECLE   $68F8, $A8CD, $B237, $7202, $87C3, $47F6, $5D0C, $9D39
            DECLE   $EC2B, $2C1E, $36E4, $F6D1, $0310, $C325, $D9DF, $19EA
            DECLE   $9DFD, $5DC8, $4732, $8707, $72C6, $B2F3, $A809, $683C
            DECLE   $192E, $D91B, $C3E1, $03D4, $F615, $3620, $2CDA, $ECEF
            DECLE   $CEFE, $0ECB, $1431, $D404, $21C5, $E1F0, $FB0A, $3B3F
            DECLE   $4A2D, $8A18, $90E2, $50D7, $A516, $6523, $7FD9, $BFEC
            ENDP



;; ======================================================================== ;;
;;  JLP_CRC16                                                               ;;
;;                                                                          ;;
;;  Compute a CRC-16 on a block of memory.  Implements a right-shifting     ;;
;;  CRC with the polynomial 0xAD52.  This version uses JLP acceleration.    ;;
;;                                                                          ;;
;;  INPUTS: CRC16                                                           ;;
;;      R1  Number of words to checksum                                     ;;
;;      R4  Base address to start checksum                                  ;;
;;                                                                          ;;
;;  INPUTS: CRC16.1                                                         ;;
;;      R0  Initial checksum                                                ;;
;;      R1  Number of words to checksum                                     ;;
;;      R4  Base address to start checksum                                  ;;
;;                                                                          ;;
;;  OUTPUTS                                                                 ;;
;;      R0  Final checksum                                                  ;;
;;      R4  Points past end of 4K segment                                   ;;
;;      R1..R3 unmodified                                                   ;;
;; ======================================================================== ;;
JLP_CRC16   PROC
            CLRR    R0
@@1:        MVO     R0,     $9FFD
            PSHR    R1
            PSHR    R2

            MVII    #$9FFC, R2
            SLRC    R1,     1
            BNC     @@loop
            MVI@    R4,     R0
            MVO@    R0,     R2
@@even:     BEQ     @@done

@@loop:
            MVI@    R4,     R0
            MVO@    R0,     R2
            MVI@    R4,     R0
            MVO@    R0,     R2
            DECR    R1
            BNEQ    @@loop

@@done:     MVI     $9FFD,  R0
            PULR    R2
            PULR    R1
            JR      R5
            ENDP

            REPEAT  $6000 - $
            DECLE   0
            ENDR

            IF      $ <> $6000
                ERROR "Not $6000"
            ENDI

;; Fill a ROM segment with 12-bit data to test JLP's 12 ROM support

            REPEAT  $FFF
            DECLE   $ - $6000
            ENDR
END_OF_ROM  DECLE   $FFF
