                             /*   GMSPROGRAM  PROC                         */
    0x02B8, 0xC80A,          /*               MVII    #GMSMAIN,   R0       */
    0x0240, 0x0100,          /*               MVO     R0,     $100         */
    0x0040,                  /*               SWAP    R0                   */
    0x0240, 0x0101,          /*               MVO     R0,     $101         */
    0x0002,                  /*               EIS                          */
    0x0017,                  /*               DECR    PC                   */
                             /*               ENDP                         */
                             /*                                            */
    0x00af,                  /*   STUB        JR      R5                   */
                             /*                                            */
                             /*   GMSMAIN     PROC                         */
    0x0003,                  /*               DIS                          */
    0x02BE, 0x0600,          /*               MVII    #STACK, R6           */
    0x02B8, 0xC809,          /*               MVII    #STUB,  R0           */
    0x0240, 0x0100,          /*               MVO     R0,     $100         */
    0x0040,                  /*               SWAP    R0                   */
    0x0240, 0x0101,          /*               MVO     R0,     $101         */
    0x0002,                  /*               EIS                          */
                             /*                                            */
    0x0004, 0x01C8, 0x0085,  /*               CALL    COPYRAM              */
    0x0020, 0x0000,          /*               DECLE   $0020,  $0000        */
    0x000B, 0x0028,          /*               DECLE   $000B,  $0028        */
    0x0010, 0x00F0,          /*               DECLE   $0010,  $00F0        */
    0x025E, 0x0102,          /*               DECLE   $025E,  $0102        */
    0x0200, 0x3800,          /*               DECLE   $0200,  $3800        */
    0x0800, 0x4000,          /*               DECLE   $0800,  $4000        */
    0x0000,                  /*               DECLE   0                    */
                             /*                                            */
    0x02B9, 0x0008,          /*               MVII    #WBANK, R1           */
    0x02BA, 0x0010,          /*               MVII    #BBANK, R2           */
    0x0004, 0x01C8, 0x00AF,  /*               CALL    IC_SETBANK           */
                             /*                                            */
    0x0280, 0x0800,          /*               MVI     WINDOW+0, R0         */
    0x0283, 0x0801,          /*               MVI     WINDOW+1, R3         */
    0x0284, 0x0802,          /*               MVI     WINDOW+2, R4         */
                             /*                                            */
    0x02B9, 0x0008,          /*               MVII    #WBANK, R1           */
    0x02BA, 0x0070,          /*               MVII    #$70,   R2           */
    0x0004, 0x01C8, 0x00AF,  /*               CALL    IC_SETBANK           */
                             /*                                            */
    0x0240, 0x0800,          /*               MVO     R0, WINDOW+0         */
    0x0243, 0x0801,          /*               MVO     R3, WINDOW+1         */
    0x0244, 0x0802,          /*               MVO     R4, WINDOW+2         */
                             /*                                            */
                             /*                                            */
    0x02B9, 0x0008,          /*               MVII    #WBANK, R1           */
    0x02BA, 0x0000,          /*               MVII    #CBANK, R2           */
    0x0004, 0x01C8, 0x00AF,  /*               CALL    IC_SETBANK           */
                             /*                                            */
                             /*                                            */
    0x02B9, 0xC84E,          /*               MVII    #GMSLAUNCH, R1       */
    0x0241, 0x0100,          /*               MVO     R1,     $100         */
    0x0041,                  /*               SWAP    R1                   */
    0x0241, 0x0101,          /*               MVO     R1,     $101         */
    0x0017,                  /*               DECR    PC                   */
                             /*               ENDP                         */
                             /*                                            */
                             /*   GMSLAUNCH   PROC                         */
    0x0003,                  /*               DIS                          */
    0x0280, 0x084F,          /*               MVI     REGS.visb,  R0       */
    0x0080,                  /*               TSTR    R0                   */
    0x0204, 0x0002,          /*               BEQ     @@notvis             */
    0x0240, 0x0020,          /*               MVO     R0,     $20          */
                             /*                                            */
    0x0280, 0x0021,          /*               MVI     $21,        R0       */
    0x0280, 0x084E,          /*               MVI     REGS.mode,  R0       */
    0x0080,                  /*               TSTR    R0                   */
    0x0204, 0x0002,          /*               BEQ     @@colstk             */
    0x0240, 0x0021,          /*               MVO     R0,         $21      */
                             /*                                            */
    0x0280, 0x084D,          /*               MVI     REGS.isrv,  R0       */
    0x0240, 0x0100,          /*               MVO     R0,         $100     */
    0x0040,                  /*               SWAP    R0                   */
    0x0240, 0x0101,          /*               MVO     R0,         $101     */
                             /*                                            */
    0x02BD, 0x0841,          /*               MVII    #REGS.1,    R5       */
    0x02A9,                  /*               MVI@    R5,         R1       */
    0x02AA,                  /*               MVI@    R5,         R2       */
    0x02AB,                  /*               MVI@    R5,         R3       */
    0x02AC,                  /*               MVI@    R5,         R4       */
    0x000D,                  /*               INCR    R5                   */
    0x02AE,                  /*               MVI@    R5,         R6       */
    0x0285, 0x0845,          /*               MVI     REGS.5,     R5       */
                             /*                                            */
    0x0280, 0x0849,          /*               MVI     REGS.intr,  R0       */
    0x03B8, 0x0001,          /*               ANDI    #1,         R0       */
    0x0204, 0x0007,          /*               BEQ     @@do_eis             */
                             /*                                            */
    0x0280, 0x0848,          /*               MVI     REGS.stat,  R0       */
    0x0038,                  /*               RSWD    R0                   */
    0x0280, 0x0840,          /*               MVI     REGS.0,     R0       */
    0x0287, 0x0847,          /*               MVI     REGS.7,     R7       */
                             /*                                            */
    0x0280, 0x0848,          /*     @@do_eis: MVI     REGS.stat,  R0       */
    0x0038,                  /*               RSWD    R0                   */
    0x0280, 0x0840,          /*               MVI     REGS.0,     R0       */
    0x0002,                  /*               EIS                          */
    0x0287, 0x0847,          /*               MVI     REGS.7,     R7       */
                             /*               ENDP                         */
                             /*                                            */
                             /*   COPYRAM     PROC                         */
    0x0200, 0x0023,          /*               B       @@first              */
                             /*                                            */
    0x02AC,                  /*     @@o_loop: MVI@    R5,     R4           */
    0x0275,                  /*               PSHR    R5                   */
                             /*                                            */
    0x02B9, 0x0008,          /*     @@m_loop: MVII    #WBANK, R1           */
    0x00A2,                  /*               MOVR    R4,     R2           */
    0x0042,                  /*               SWAP    R2                   */
    0x03BA, 0x00FF,          /*               ANDI    #$FF,   R2           */
    0x0004, 0x01C8, 0x00AF,  /*               CALL    IC_SETBANK           */
                             /*                                            */
    0x00A5,                  /*               MOVR    R4,     R5           */
    0x03BD, 0x00FF,          /*               ANDI    #$FF,   R5           */
    0x02FD, 0x0800,          /*               ADDI    #WINDOW,R5           */
                             /*                                            */
    0x00AA,                  /*               MOVR    R5,     R2           */
    0x00DA,                  /*               ADDR    R3,     R2           */
    0x037A, 0x1000,          /*               CMPI    #WINDOW + 2048, R2   */
    0x0209, 0x0002,          /*               BNC     @@cnt_ok             */
    0x02BA, 0x0FFF,          /*               MVII    #WINDOW+2048-1, R2   */
    0x012A,                  /*     @@cnt_ok: SUBR    R5,     R2           */
                             /*                                            */
    0x0113,                  /*               SUBR    R2,     R3           */
                             /*                                            */
    0x02A9,                  /*     @@i_loop: MVI@    R5,     R1           */
    0x0261,                  /*               MVO@    R1,     R4           */
    0x0012,                  /*               DECR    R2                   */
    0x022C, 0x0004,          /*               BNEQ    @@i_loop             */
                             /*                                            */
    0x009B,                  /*               TSTR    R3                   */
    0x022C, 0x001F,          /*               BNEQ    @@m_loop             */
                             /*                                            */
    0x02B5,                  /*               PULR    R5                   */
    0x02AB,                  /*      @@first: MVI@    R5,     R3           */
    0x009B,                  /*               TSTR    R3                   */
    0x022C, 0x0026,          /*               BNEQ    @@o_loop             */
    0x00AF,                  /*               JR      R5                   */
                             /*               ENDP                         */
                             /*                                            */
                             /*   IC_SETBANK  PROC                         */
    0x0275,                  /*               PSHR    R5                   */
    0x008D,                  /*               MOVR    R1,     R5           */
    0x00ED,                  /*               ADDR    R5,     R5           */
    0x03BD, 0x0010,          /*               ANDI    #$10,   R5           */
    0x0065,                  /*               SLR     R1,     2            */
    0x0065,                  /*               SLR     R1,     2            */
    0x03B9, 0x000F,          /*               ANDI    #$0F,   R1           */
    0x00E9,                  /*               ADDR    R5,     R1           */
    0x02F9, 0x0040,          /*               ADDI    #$40,   R1           */
    0x024A,                  /*               MVO@    R2,     R1           */
    0x02B7,                  /*               PULR    PC                   */
                             /*               ENDP                         */
