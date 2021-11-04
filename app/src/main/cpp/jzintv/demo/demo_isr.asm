;; ======================================================================== ;;
;;  DEMO_ISR:  This drives most of the demo playback process.               ;;
;; ======================================================================== ;;

DEMO_ISR    PROC
            MVII    #DEMO_Q,    R3
@@nextq     MVI@    R3,         PC      ; jump to first thing in work list

            ;; ------------------------------------------------------------ ;;
            ;;  Update MOB registers from shadow if needed.                 ;;
            ;; ------------------------------------------------------------ ;;
@@mob_upd:  INCR    R3
            MVII    #DEMO_MOBS, R4      ; point to STIC MOB shadow
            CLRR    R5

            REPEAT  24                  ; copy over X, Y, A from shadow
            MVI@    R4,         R0
            MVO@    R0,         R5
            ENDR

            MVI@    R3,         PC      ; jump to next item on work list

            ;; ------------------------------------------------------------ ;;
            ;;  Update STIC general control registers from shadow.          ;;
            ;; ------------------------------------------------------------ ;;
@@oth_stic: INCR    R3
            MVII    #DEMO_STIC, R4      ; point to STIC ctrl shadow
            MVII    #$28,       R5

            ; First word has color stack in it
            MVI@    R4,         R0      ; get CS0..CS3
            MVO@    R0,         R5      ; write CS0 from bits 3:0
            SWAP    R0
            MVO@    R0,         R5      ; write CS1 from bits 11:8
            RRC     R0,         2
            RRC     R0,         2
            MVO@    R0,         R5      ; write CS2 from bits 15:12
            SWAP    R0
            MVO@    R0,         R5      ; write CS3 from bits 7:4  

            ; Next word has border color, h_dly, v_dly, border extension in it
            MVI@    R4,         R0
            MVO@    R0,         R5      ; write border color from bits 3:0
            SWAP    R0
            MVII    #$30,       R5      ; point to h_dly
            MVO@    R0,         R5      ; write h_dly from bits 10:8
            RRC     R0,         1
            RRC     R0,         2
            MVO@    R0,         R5      ; write v_dly from bits 13:11
            RRC     R0,         1
            RRC     R0,         2
            MVO@    R0,         R5      ; write border ext from bits 15:14

            MVI@    R3,         PC      ; jump to next item in work list

            ;; ------------------------------------------------------------ ;;
            ;;  Set FGBG mode if requested.                                 ;;
            ;; ------------------------------------------------------------ ;;
@@fgbg:     MVO     R0,         $21     ; set FGBG mode
            INCR    R3
            MVI@    R3,         PC

            ;; ------------------------------------------------------------ ;;
            ;;  Set CSTK mode if requested.                                 ;;
            ;; ------------------------------------------------------------ ;;
@@cstk:     MVI     $21,        R0      ; set CSTK mode
            INCR    R3
            MVI@    R3,         PC

            ;; ------------------------------------------------------------ ;;
            ;;  Enable the display if requested.                            ;;
            ;; ------------------------------------------------------------ ;;
@@viden:    MVO     R0,         $20
            INCR    R3
            MVI@    R3,         PC


            ;; ------------------------------------------------------------ ;;
            ;;  GRAM update routines.                                       ;;
            ;; ------------------------------------------------------------ ;;
@@gr_cp8:   MVI@    R3,         R0      ; 8
            MOVR    R0,         R1      ; 6

            ; Unpack GRAM index from bits 7:2
            SLL     R1,         1       ; 6
            ANDI    #$1F8,      R1      ; 8
            ADDI    #$3800,     R1      ; 8
            MOVR    R1,         R4      ; 6

            ; Unpack Tile DB index from remaining bits
            SWAP    R0                  ; 8
            ANDI    #$3FF,      R0      ; 8
            SLL     R0,         2       ; 8
            MVII    #TILE_DB,   R5      ; 8
            ADDR    R0,         R5      ; 6
                                        ;-- 
                                        ;80
                                        
            REPEAT  4
            MVI@    R4,         R0      ; 8
            MVO@    R0,         R5      ; 9
            SWAP    R0                  ; 6
            MVO@    R0,         R5      ; 9
            ENDR

