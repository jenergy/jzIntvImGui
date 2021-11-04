/************************************/
/* CP1600/CP1610 Disassembler       */
/* Version  0.2 by Frank Palazzolo  */
/*                 palazzol@tir.com */
/*                                  */
/* Modified slightly by J. Zbiciak  */
/* to fix a couple minor bugs.      */
/************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int get_num_words(int op, int dbd);

int diff_mode = 0;

#define R2        "R%c",'0'+i2
#define R3        "R%c",'0'+i3
#define RB        "R%c",'4'+((w2>>8)&3)
#define RR        "R%c",'0'+(i3&3)
#define IM        "#$%.4X",(dbd==1?(w2&0xFF)|(0xFF00&(w3<<8)):w2)
#define ADDR      "$%.4X",w2
#define DCL1      "$%.4X",w1
#define DCL2      "$%.4X",w2
#define BDCL      "$%.4X",(w1&0xFF)|(0xFF00&(w2<<8))
#define EXT       "$%.1X",(w1&0x0F)

#define DA        "$%.4X",((w2&0xfc)<<8)+(w3&0x3ff)
#define DAL       "$%.4X",gen_label(((w2&0xfc)<<8)+(w3&0x3ff))
#define DISPMINUS "$%.4X",addr+1-w2
#define DISPPLUS  "$%.4X",addr+2+w2

#define OPCODE(Q)      snprintf(display_buffer.opcode_field,sizeof(display_buffer.opcode_field),"%s",Q)
#define OPERAND1(Q)    snprintf(display_buffer.operand1_field,sizeof(display_buffer.operand1_field),Q)
#define OPERAND2(Q)    {snprintf(display_buffer.operand2_field,sizeof(display_buffer.operand2_field),Q); \
                       strcpy(display_buffer.operanddel_field,",");}

#define MAYBE2_1  if(i3&1)OPERAND1("2")
#define MAYBE2_2  if(i3&4)OPERAND2("2")
#define DISP_1     if(sign)OPERAND1(DISPMINUS);else OPERAND1(DISPPLUS)

const char *branch_code[16] = {
"B   ","BC  ","BOV ","BPL ","BEQ ","BLT ","BLE ","BUSC",
"NOPP","BNC ","BNOV","BMI ","BNEQ","BGE ","BGT ","BESC"
};

const char *commands[] = {
"org","dbdata","revbytes","skip","data","code"
};

int get_num_words(int op, int dbd)
{
    if (op == 0x4)
        return 3;

    if ((op & 0x3c0) == 0x200)
        return 2;

    if ((op & 0x3f8) == 0x240)
        return 2;

    if ((op & 0x3f8) == 0x278)
        return 2;

    if ((op & 0x238) == 0x200)
        return 2;

    if ((op & 0x238) == 0x238)
        return 2 + (dbd == 1);

    return 1;
}

int datatype[0x10000];
char *comments[0x10000];
int code[0x10000];
int label[0x10000];
int rlabel[1024];

struct tag_display_buffer
{
    char label_field[10];
    char address_field[10];
    char word1_field[10];
    char word2_field[10];
    char word3_field[10];
    char char_field[10];
    char opcode_field[10];
    char operand1_field[10];
    char operanddel_field[10];
    char operand2_field[10];
    char comment_field[80];
} display_buffer;


static int gen_label(int addr)
{
    int hash = 0;

    if (!diff_mode) return addr;
    if (label[addr]) return label[addr];

    hash = ((17*code[addr]) / 13) & 0x3FF;

    while (rlabel[hash] > 0x3F)
        hash = (hash + 1) & 0x3FF;

    label[addr] = (hash << 6) | rlabel[hash];

    return label[addr];
}

int main(int argc, char **argv)
{
    int addr;
    unsigned char c1, c2;
    int w1, w2=0, w3=0, s=0, i1, i2, i3, sign;
    int i, j, k;
    int done, found, temp, temp2, dbd=0;
    int org, endaddr;
    int pass, reverse_bytes, skip_lo = -1, skip_hi = -1;
    int romw = 0;

    unsigned short labeladdr;

    char command[256];
    char buffer[256];

    FILE *fpcode = NULL;
    FILE *fpsym = NULL;

    reverse_bytes = 0;

    if (argc > 1 && !strcmp(argv[1], "-d"))
    {
        argv++;
        argc--;
        diff_mode = 1;
    }

    if (argc == 1)
    {
        fprintf(stderr,"Usage: %s [-d] infile (symfile)\n",argv[0]);
        exit(0);
    }

    if (argc >= 2)
    {
        fpcode = fopen(argv[1],"rb");
        if (!fpcode)
        {
            fprintf(stderr,"Unable to open input file %s\n",argv[1]);
            exit(0);
        }
    }

    if (argc >= 3)
    {
        fpsym = fopen(argv[2],"r");
        if (!fpsym)
        {
            fprintf(stderr,"Unable to open symbol file %s\n",argv[2]);
            exit(0);
        }
    }

    addr = 0;

    for (i=0;i<0x10000;i++)
    {
        datatype[i] = 0;
        comments[i] = 0;
        code[i] = 0;
    }

    org = 0;

    if (argc >= 3)
    {

    done = 0;

    while(fgets(buffer, 256, fpsym) && !done)
    {
        int args;
        found = 0;
        args = sscanf(buffer,"%255s %x %x", command, &temp, &temp2);
        i = 0;
        while((i<6) && !found)
        {
            if (!strcmp(command,commands[i]))
            {
                switch (i)
                {
                    case 0:
                    {
                        org = temp;
                        break;
                    }
                    case 1:
                    case 4:
                    case 5:
                    {
                        if (i == 1) k = 17; else if (i==5) k = 32; else k = 1;
                        j = args - 1;
                        if (j == 0) break;
                        if (j == 1) temp2 = temp;
                        if (temp2 < temp)
                        {
                            fprintf(stderr,"'data' arg order\n");
                            exit(1);
                        }
                        for (addr = temp; addr <= temp2; addr++)
                            datatype[addr & 0xFFFF] |= k;
                        break;
                    }
                    case 2:
                    {
                        reverse_bytes = 1;
                        break;
                    }
                    case 3:
                    {
                        skip_lo = temp;
                        skip_hi = temp2;
                        for (addr = skip_lo; addr <= skip_hi; addr++)
                            datatype[addr & 0xFFFF] |= 9;
                        break;
                    }
                }
                found = 1;
            }
            i++;
        }
#if 0
        if (!found)
            done = 1;
#endif
    }

    fclose(fpsym);


    /* 'code' overrides 'data' and 'dbdata' */
    for (addr = 0; addr < 0x10000; addr++)
        if (datatype[addr] & 32) datatype[addr] &= ~17;

    fprintf(stderr,"Finished reading symbol file...\n");

    }

    addr = org;
    memset(code, 0xFF, sizeof(code));

    while(!feof(fpcode) && (addr < 0x10000))
    {
        while ((datatype[addr] & 8) && (addr < 0x10000)) addr++;
        if (addr == 0x10000) break;

        if (fread(&c1,1,1,fpcode) != 1 ||
            fread(&c2,1,1,fpcode) != 1)
        {
            if (feof(fpcode))
                break;

            perror("fread()");
            fprintf(stderr, "Error reading binary file.\n");
            exit(1);
        }

        if (reverse_bytes)
            code[addr] = (((unsigned short)c2)<<8)+c1;
        else
            code[addr] = (((unsigned short)c1)<<8)+c2;

        while (romw < 16 && code[addr] > (1 << romw)) romw++;
        addr++;
    }

    endaddr = addr;
    fprintf(stderr,"Finished reading code file... endaddr=%04X\n", endaddr);

    for(pass=1;pass<3;pass++)
    {

    for(addr=org;addr<endaddr;addr++)
    {
        display_buffer.label_field[0] = 0;
        display_buffer.address_field[0] = 0;
        display_buffer.word1_field[0] = 0;
        display_buffer.word2_field[0] = 0;
        display_buffer.word3_field[0] = 0;
        display_buffer.opcode_field[0] = 0;
        display_buffer.operand1_field[0] = 0;
        display_buffer.operanddel_field[0] = 0;
        display_buffer.operand2_field[0] = 0;
        display_buffer.comment_field[0] = 0;

        if (datatype[addr] & 8)
        {
            skip_lo = addr;
            while (addr < endaddr && (datatype[addr] & 8)) addr++;
            skip_hi = addr - 1;
            if (pass != 1)
                printf("\n\n"
                       "        RMB     $%.4X\n\n", skip_hi + 1 - skip_lo);
            dbd  = 0;
        }

        w1 = code[addr];

        snprintf(display_buffer.address_field,5,"%.4X",addr);

        if ((datatype[addr] & 1) == 0)
        {
            if (dbd) dbd--;
            s = get_num_words(w1,dbd);

            for (i = addr+1; i < addr + s; i++)
                if ((datatype[i] & 9) || i >= endaddr)
                    break;

            if (i < addr + s)
                while (i > addr)
                    datatype[--i] |= 1;
        }

        if ((addr+1 < endaddr) &&
            (datatype[addr] & 16) && (datatype[addr+1] & 16))
        {
            strcpy(display_buffer.char_field, "     ");
            w1 = code[addr];
            w2 = code[addr+1];
            dbd = 1;
            snprintf(display_buffer.word1_field,5,"%.4X",w1);
            snprintf(display_buffer.word2_field,5,"%.4X",w2);
            if (pass != 1 && ((w1 & 0xFF00) || (w2 & 0xFF00)))
            {
                printf("; Warning: MSBs would be truncated on double-byte data.\n");
                OPCODE("DECLE");
                OPERAND1(DCL1);
                OPERAND2(DCL2);
            } else
            {
                OPCODE("BIDECLE");
                OPERAND1(BDCL);
            }
            s = 2;
        } else if ((datatype[addr] & 1) || (w1 & 0xFC00))
        {
            strcpy(display_buffer.char_field, "[   ]");
            snprintf(display_buffer.word1_field,5,"%.4X",w1);
            display_buffer.char_field[1] = (w1>=32 && w1<=126) ? w1 : '.';
            OPCODE("DECLE");
            OPERAND1(DCL1);
            if (addr+1 < endaddr && (datatype[addr+1] & (16|8|1)) == 1)
            {
                w2 = code[addr+1];
                snprintf(display_buffer.word2_field,5,"%.4X",w2);
                OPERAND2(DCL2);
                s = 2;
                display_buffer.char_field[2] = (w2>=32 && w2<=126) ? w2 : '.';
            } else
                s = 1;
        } else
        {

            datatype[addr] |= 4;


            strcpy(display_buffer.char_field, "[   ]");

            switch (s)
            {
                case 1:
                {
                    snprintf(display_buffer.word1_field,5,"%.4X",w1);
                    display_buffer.char_field[1] = (w1>=32 && w1<=126) ? w1 : '.';
                    break;
                }
                case 2:
                {
                    w2 = code[addr+1];
                    snprintf(display_buffer.word1_field,5,"%.4X",w1);
                    snprintf(display_buffer.word2_field,5,"%.4X",w2);
                    display_buffer.char_field[1] = (w1>=32 && w1<=126) ? w1 : '.';
                    display_buffer.char_field[2] = (w2>=32 && w2<=126) ? w2 : '.';
                    break;
                }
                case 3:
                {
                    w2 = code[addr+1];
                    w3 = code[addr+2];
                    snprintf(display_buffer.word1_field,5,"%.4X",w1);
                    snprintf(display_buffer.word2_field,5,"%.4X",w2);
                    snprintf(display_buffer.word3_field,5,"%.4X",w3);
                    display_buffer.char_field[1] = (w1>=32 && w1<=126) ? w1 : '.';
                    display_buffer.char_field[2] = (w2>=32 && w2<=126) ? w2 : '.';
                    display_buffer.char_field[3] = (w3>=32 && w3<=126) ? w3 : '.';
                    break;
                }
            }

            i1 = (w1 & 0x03c0)>>6;
            i2 = (w1 & 0x0038)>>3;
            i3 = w1 & 0x0007;

            switch(i1)
            {
                case 0:
                {
                    switch (i2)
                    {
                        case 0:
                        {
                            switch (i3)
                            {
                                case 0:
                                {
                                    OPCODE("HLT");
                                    break;
                                }
                                case 1:
                                {
                                    OPCODE("SDBD");
                                    dbd = 2;
                                    break;
                                }
                                case 2:
                                {
                                    OPCODE("EIS");
                                    break;
                                }
                                case 3:
                                {
                                    OPCODE("DIS");
                                    break;
                                }
                                case 4:
                                {
                                    if ((w2 & 0x300) == 0x300)
                                    {
                                        if ((w2 & 3)==0)
                                        {
                                            OPCODE("J");
                                            labeladdr = ((w2&0xfc)<<8)+(w3&0x3ff);
                                            datatype[labeladdr] |= 2;
                                            if (pass == 2)
                                            {
                                                if ((labeladdr >= org) && (labeladdr < endaddr))
                                                {
                                                    OPERAND1(DAL);
                                                    display_buffer.operand1_field[0] = 'L';
                                                }
                                                else
                                                {
                                                    OPERAND1(DA);
                                                    display_buffer.operand1_field[0] = 'G';
                                                }
                                            }
                                        }
                                        else if ((w2 & 3)==1)
                                        {
                                            OPCODE("JE");
                                            labeladdr = ((w2&0xfc)<<8)+(w3&0x3ff);
                                            datatype[labeladdr] |= 2;
                                            if (pass == 2)
                                            {
                                                if ((labeladdr >= org) && (labeladdr < endaddr))
                                                {
                                                    OPERAND1(DAL);
                                                    display_buffer.operand1_field[0] = 'L';
                                                }
                                                else
                                                {
                                                    OPERAND1(DA);
                                                    display_buffer.operand1_field[0] = 'G';
                                                }
                                            }
                                        }
                                        else if ((w2 & 3)==2)
                                        {
                                            OPCODE("JD");
                                            labeladdr = ((w2&0xfc)<<8)+(w3&0x3ff);
                                            datatype[labeladdr] |= 2;
                                            if (pass == 2)
                                            {
                                                if ((labeladdr >= org) && (labeladdr < endaddr))
                                                {
                                                    OPERAND1(DAL);
                                                    display_buffer.operand1_field[0] = 'L';
                                                }
                                                else
                                                {
                                                    OPERAND1(DA);
                                                    display_buffer.operand1_field[0] = 'G';
                                                }
                                            }
                                        }
                                        else
                                            OPCODE(".illop");
                                    }
                                    else
                                    {
                                        if ((w2 & 3)==0)
                                        {
                                            OPCODE("JSR");
                                            OPERAND1(RB);
                                            labeladdr = ((w2&0xfc)<<8)+(w3&0x3ff);
                                            datatype[labeladdr] |= 2;
                                            if (pass == 2)
                                            {
                                                if ((labeladdr >= org) && (labeladdr < endaddr))
                                                {
                                                    OPERAND2(DAL);
                                                    display_buffer.operand2_field[0] = 'L';
                                                }
                                                else
                                                {
                                                    OPERAND2(DA);
                                                    display_buffer.operand2_field[0] = 'G';
                                                }
                                            }
                                        }
                                        else if ((w2 & 3)==1)
                                        {
                                            OPCODE("JSRE");
                                            OPERAND1(RB);
                                            labeladdr = ((w2&0xfc)<<8)+(w3&0x3ff);
                                            datatype[labeladdr] |= 2;
                                            if (pass == 2)
                                            {
                                                if ((labeladdr >= org) && (labeladdr < endaddr))
                                                {
                                                    OPERAND2(DAL);
                                                    display_buffer.operand2_field[0] = 'L';
                                                }
                                                else
                                                {
                                                    OPERAND2(DA);
                                                    display_buffer.operand2_field[0] = 'G';
                                                }
                                            }
                                        }
                                        else if ((w2 & 3)==2)
                                        {
                                            OPCODE("JSRD");
                                            OPERAND1(RB);
                                            labeladdr = ((w2&0xfc)<<8)+(w3&0x3ff);
                                            datatype[labeladdr] |= 2;
                                            if (pass == 2)
                                            {
                                                if ((labeladdr >= org) && (labeladdr < endaddr))
                                                {
                                                    OPERAND2(DAL);
                                                    display_buffer.operand2_field[0] = 'L';
                                                }
                                                else
                                                {
                                                    OPERAND2(DA);
                                                    display_buffer.operand2_field[0] = 'G';
                                                }
                                            }
                                        }
                                        else
                                            OPCODE(".illop");
                                    }

                                    break;
                                }
                                case 5:
                                {
                                    OPCODE("TCI");
                                    break;
                                }
                                case 6:
                                {
                                    OPCODE("CLRC");
                                    break;
                                }
                                case 7:
                                {
                                    OPCODE("SETC");
                                    break;
                                }
                            }
                            break;
                        }
                        case 1:
                        {
                            OPCODE("INCR");
                            OPERAND1(R3);
                            break;
                        }
                        case 2:
                        {
                            OPCODE("DECR");
                            OPERAND1(R3);
                            break;
                        }
                        case 3:
                        {
                            OPCODE("COMR");
                            OPERAND1(R3);
                            break;
                        }
                        case 4:
                        {
                            OPCODE("NEGR");
                            OPERAND1(R3);
                            break;
                        }
                        case 5:
                        {
                            OPCODE("ADCR");
                            OPERAND1(R3);
                            break;
                        }
                        case 6:
                        {
                            if (i3 < 4)
                            {
                                OPCODE("GSWD");
                                OPERAND1(RR);
                            }
                            else if (i3 < 6)
                            {
                                OPCODE(i3 & 1 ? "NOP2" : "NOP");
                            }
                            else
                            {
                                OPCODE(i3 & 1 ? "SIN2" : "SIN");
                            }
                            break;
                        }
                        case 7:
                        {
                            OPCODE("RSWD");
                            OPERAND1(R3);
                            break;
                        }
                    }
                    break;
                }

                case 1:
                {
                    switch (i2)
                    {
                        case 0:
                        {
                            OPCODE("SWAP");
                            OPERAND1(RR);
                            MAYBE2_2;
                            break;
                        }
                        case 1:
                        {
                            OPCODE("SLL");
                            OPERAND1(RR);
                            MAYBE2_2;
                            break;
                        }
                        case 2:
                        {
                            OPCODE("RLC");
                            OPERAND1(RR);
                            MAYBE2_2;
                            break;
                        }
                        case 3:
                        {
                            OPCODE("SLLC");
                            OPERAND1(RR);
                            MAYBE2_2;
                            break;
                        }
                        case 4:
                        {
                            OPCODE("SLR");
                            OPERAND1(RR);
                            MAYBE2_2;
                            break;
                        }
                        case 5:
                        {
                            OPCODE("SAR");
                            OPERAND1(RR);
                            MAYBE2_2;
                            break;
                        }
                        case 6:
                        {
                            OPCODE("RRC");
                            OPERAND1(RR);
                            MAYBE2_2;
                            break;
                        }
                        case 7:
                        {
                            OPCODE("SARC");
                            OPERAND1(RR);
                            MAYBE2_2;
                            break;
                        }
                    }
                    break;
                }
                case 2:
                {
                    if (((w1 & 0x38)>>3) == (w1 & 0x7))
                    {
                        OPCODE("TSTR");
                        OPERAND1(R2);
                    }
                    else
                    if ((w1 & 0x7) == 7)
                    {
                        OPCODE("JR");
                        OPERAND1(R2);
                    }
                    else
                    {
                        OPCODE("MOVR");
                        OPERAND1(R2);
                        OPERAND2(R3);
                    }
                    break;
                }
                case 3:
                {
                    OPCODE("ADDR");
                    OPERAND1(R2);
                    OPERAND2(R3);
                    break;
                }
                case 4:
                {
                    OPCODE("SUBR");
                    OPERAND1(R2);
                    OPERAND2(R3);
                    break;
                }
                case 5:
                {
                    OPCODE("CMPR");
                    OPERAND1(R2);
                    OPERAND2(R3);
                    break;
                }
                case 6:
                {
                    OPCODE("ANDR");
                    OPERAND1(R2);
                    OPERAND2(R3);
                    break;
                }
                case 7:
                {
                    if (((w1 & 0x38)>>3) == (w1 & 0x7))
                    {
                        OPCODE("CLRR");
                        OPERAND1(R3);
                    }
                    else
                    {
                        OPCODE("XORR");
                        OPERAND1(R2);
                        OPERAND2(R3);
                    }
                    break;
                }
                case 8:
                {
                    sign = (i2 & 4)>>2;
#if 0
                    if ((w1 & 0x1f) == 0x00)
                    {
                        OPCODE("B");
                        DISP_1;
                        if(sign)
                            labeladdr = addr+1-w2;
                        else
                            labeladdr = addr+2+w2;
                        datatype[labeladdr] |= 2;
                        if (pass == 2)
                        {
                            if ((labeladdr >= org) && (labeladdr < endaddr))
                                display_buffer.operand1_field[0] = 'L';
                            else
                                display_buffer.operand1_field[0] = 'G';
                        }
                    }
                    else
#endif
                    if ((w1 & 0x1f) == 0x8 && (w2 == 0x0000))
                    {
                        OPCODE("NOPP");
                    }
                    else
                    {
                        if ((w1 & 0x10) == 0x00)
                        {
                            OPCODE(branch_code[w1&0xf]);
                        } else
                        {
                            OPCODE("BEXT");
                            OPERAND2(EXT);
                        }
#if 0
                        DISP_1;
                        if(sign)
                            labeladdr = addr+1-w2;
                        else
                            labeladdr = addr+2+w2;
                        datatype[labeladdr] |= 2;
                        if (pass == 2)
                        {
                            if ((labeladdr >= org) && (labeladdr < endaddr))
                                display_buffer.operand1_field[0] = 'L';
                            else
                                display_buffer.operand1_field[0] = 'G';
                        }
#else
                        if(sign)
                            labeladdr = addr+1-w2;
                        else
                            labeladdr = addr+2+w2;
                        datatype[labeladdr] |= 2;
                        if (pass == 2)
                        {
                            snprintf(display_buffer.operand1_field,
                                    sizeof(display_buffer.operand1_field),
                                    "L%.4X", gen_label(labeladdr));
                        }
#endif
                    }

                    break;
                }
                case 9:
                {
                    switch(i2)
                    {
                        case 0:
                        {
                            OPCODE("MVO");
                            OPERAND1(R3);
                            OPERAND2(ADDR);
                            break;
                        }
                        case 6:
                        {
                            OPCODE("PSHR");
                            OPERAND1(R3);
                            break;
                        }
                        case 7:
                        {
                            OPCODE("MVOI");
                            OPERAND1(R3);
                            OPERAND2(IM);
                            break;
                        }
                        default:
                        {
                            OPCODE("MVO@");
                            OPERAND1(R3);
                            OPERAND2(R2);
                            break;
                        }
                    }
                    break;
                }
                case 0xa:
                {
                    switch(i2)
                    {
                        case 0:
                        {
                            OPCODE("MVI");
                            OPERAND1(ADDR);
                            OPERAND2(R3);
                            break;
                        }
                        case 6:
                        {
                            OPCODE("PULR");
                            OPERAND1(R3);
                            break;
                        }
                        case 7:
                        {
                            OPCODE("MVII");
                            OPERAND1(IM);
                            OPERAND2(R3);
                            break;
                        }
                        default:
                        {
                            OPCODE("MVI@");
                            OPERAND1(R2);
                            OPERAND2(R3);
                            break;
                        }
                    }
                    break;
                }
                case 0xb:
                {
                    switch(i2)
                    {
                        case 0:
                        {
                            OPCODE("ADD");
                            OPERAND1(ADDR);
                            OPERAND2(R3);
                            break;
                        }
                        case 7:
                        {
                            OPCODE("ADDI");
                            OPERAND1(IM);
                            OPERAND2(R3);
                            break;
                        }
                        default:
                        {
                            OPCODE("ADD@");
                            OPERAND1(R2);
                            OPERAND2(R3);
                            break;
                        }
                    }
                    break;
                }
                case 0xc:
                {
                    switch(i2)
                    {
                        case 0:
                        {
                            OPCODE("SUB");
                            OPERAND1(ADDR);
                            OPERAND2(R3);
                            break;
                        }
                        case 7:
                        {
                            OPCODE("SUBI");
                            OPERAND1(IM);
                            OPERAND2(R3);
                            break;
                        }
                        default:
                        {
                            OPCODE("SUB@");
                            OPERAND1(R2);
                            OPERAND2(R3);
                            break;
                        }
                    }
                    break;
                }
                case 0xd:
                {
                    switch(i2)
                    {
                        case 0:
                        {
                            OPCODE("CMP");
                            OPERAND1(ADDR);
                            OPERAND2(R3);
                            break;
                        }
                        case 7:
                        {
                            OPCODE("CMPI");
                            OPERAND1(IM);
                            OPERAND2(R3);
                            break;
                        }
                        default:
                        {
                            OPCODE("CMP@");
                            OPERAND1(R2);
                            OPERAND2(R3);
                            break;
                        }
                    }
                    break;
                }
                case 0xe:
                {
                    switch(i2)
                    {
                        case 0:
                        {
                            OPCODE("AND");
                            OPERAND1(ADDR);
                            OPERAND2(R3);
                            break;
                        }
                        case 7:
                        {
                            OPCODE("ANDI");
                            OPERAND1(IM);
                            OPERAND2(R3);
                            break;
                        }
                        default:
                        {
                            OPCODE("AND@");
                            OPERAND1(R2);
                            OPERAND2(R3);
                            break;
                        }
                    }
                    break;
                }
                case 0xf:
                {
                    switch(i2)
                    {
                        case 0:
                        {
                            OPCODE("XOR");
                            OPERAND1(ADDR);
                            OPERAND2(R3);
                            break;
                        }
                        case 7:
                        {
                            OPCODE("XORI");
                            OPERAND1(IM);
                            OPERAND2(R3);
                            break;
                        }
                        default:
                        {
                            OPCODE("XOR@");
                            OPERAND1(R2);
                            OPERAND2(R3);
                            break;
                        }
                    }
                    break;
                }
            }
        }

        if (pass == 2)
        {
            char buf[20];

            snprintf(buf, sizeof(buf), "%s%s",
                    display_buffer.operand1_field,
                    display_buffer.operanddel_field);

            if (datatype[addr] & 2)
                snprintf(display_buffer.label_field,
                        sizeof(display_buffer.label_field),
                        "L%.4X:",gen_label(addr));

            if (strstr(display_buffer.opcode_field, "illop") != NULL)
            {
                    printf("%-7s DECLE   $%s",
                            display_buffer.label_field,
                            display_buffer.word1_field);
                    if (display_buffer.word2_field[0])
                            printf(", $%s",
                                    display_buffer.word2_field);
                    if (display_buffer.word3_field[0])
                            printf(", $%s",
                                    display_buffer.word3_field);
                    printf("\n");
            } else
            {
                if (!diff_mode)
                    printf("%-7s %-8s%-8s%-7s %16s; $%4s  %4s %4s %4s  %5s\n",

                            display_buffer.label_field,
                            display_buffer.opcode_field,
                            buf,
                            display_buffer.operand2_field,
                            display_buffer.comment_field,
                            display_buffer.address_field,
                            display_buffer.word1_field,
                            display_buffer.word2_field,
                            display_buffer.word3_field,
                            display_buffer.char_field);
                else
                    printf("%-7s %-8s%-8s%-7s %16s\n",

                            display_buffer.label_field,
                            display_buffer.opcode_field,
                            buf,
                            display_buffer.operand2_field,
                            display_buffer.comment_field);
            }
        } else
            datatype[addr] |= 4;


        addr = (addr + s - 1) & 0xffff;
    }

    fprintf(stderr,"End of pass #%d\n",pass);

    if (pass != 1) continue;

    printf("\n; Properly Defined Local Symbols:\n\n");

    for(i=j=0;i<0x10000;i++)
    {
        if ((datatype[i] & 6) == 6)
        {
           if ((i >= org) && (i < endaddr))
           {
                if ((j++&7) == 0) printf("\n;");
                printf("   L%.4X", gen_label(i));
           }
        }
    }
    printf("\n\n; Improperly Defined Local Symbols:\n\n");

    for(i=0;i<0x10000;i++)
    {
        if ((datatype[i] & 6) == 2)
        {
           if ((i >= org) && (i < endaddr))
                printf("L%.4X   EQU     $%.4X\n", gen_label(i), i);
        }
    }

    printf("\n\n; Global Symbols:\n\n");

    for(i=j=0;i<0x10000;i++)
    {
        if (datatype[i] & 2)
        {
           if ((i < org) || (i >= endaddr))
                printf("G%.4X   EQU     $%.4X\n", i, i);
        }
    }

    printf("\n\n"
           "        ORG     $%.4X\n"
           "        ROMW    %d\n\n\n", org, romw);

    }

    return 0;
}

/* ======================================================================== */
/*  This program is free software; you can redistribute it and/or modify    */
/*  it under the terms of the GNU General Public License as published by    */
/*  the Free Software Foundation; either version 2 of the License, or       */
/*  (at your option) any later version.                                     */
/*                                                                          */
/*  This program is distributed in the hope that it will be useful,         */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       */
/*  General Public License for more details.                                */
/*                                                                          */
/*  You should have received a copy of the GNU General Public License along */
/*  with this program; if not, write to the Free Software Foundation, Inc., */
/*  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.             */
/* ======================================================================== */
/*                Copyright (c) 1998-2000, Frank Palazzolo                  */
/* ======================================================================== */
