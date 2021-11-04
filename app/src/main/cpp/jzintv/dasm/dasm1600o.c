
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

#define R2        "R%c",'0'+i2
#define R3        "R%c",'0'+i3
#define RB        "R%c",'4'+((w2>>8)&3)
#define RR        "R%c",'0'+(i3&3)
#define IM        "#%04hx",(dbd==1?(w2&0xFF)|(w3<<8):w2)
#define ADDR      "$%04hx",w2

#define DA        "$%04hx",((w2&0xfc)<<8)+(w3&0x3ff)
#define DISPMINUS "$%04hx",addr+1-w2
#define DISPPLUS  "$%04hx",addr+2+w2

#define OPCODE(Q)      snprintf(display_buffer.opcode_field,sizeof(display_buffer.opcode_field),Q)
#define OPERAND1(Q)    snprintf(display_buffer.operand1_field,sizeof(display_buffer.operand1_field),Q)
#define OPERAND2(Q)    {snprintf(display_buffer.operand2_field,sizeof(display_buffer.operand2_field),Q); \
                        strcpy(display_buffer.operanddel_field,",");}

#define MAYBE2_1  if(i3&1)OPERAND1("2")
#define MAYBE2_2  if(i3&4)OPERAND2("2")
#define DISP_1     if(sign)OPERAND1(DISPMINUS);else OPERAND1(DISPPLUS)

const char *branch_code[16] = {
"B   ","BC  ","BOV ","BPL ","BEQ ","BLT ","BLE ","BUSC",
"B   ","BNC ","BNOV","BMI ","BNEQ","BGE ","BGT ","BESC"
};

const char *commands[3] = {
"org","data","revbytes"
};

static int get_num_words(int op, int dbd)
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

int main(int argc, char **argv)
{
    int addr;
    unsigned char c1, c2;
    int w1=0, w2=0, w3=0, s, i1, i2, i3, sign;
    int i;
    int done, found, temp, dbd=0;
    int org, endaddr;
    int pass, reverse_bytes;

    unsigned short labeladdr;

    char command[256];

    FILE *fpcode = 0;
    FILE *fpsym = 0;

    reverse_bytes = 0;

    if (argc == 1)
    {
        fprintf(stderr,"Usage: %s infile (symfile)\n",argv[0]);
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

    while(!feof(fpsym) && !done)
    {
        found = 0;
        fscanf(fpsym,"%255s",command);
        i = 0;
        while((i<3) && !found)
        {
            if (!strcmp(command,commands[i]))
            {
                switch (i)
                {
                    case 0:
                    {
                        fscanf(fpsym,"%x",&org);
                        break;
                    }
                    case 1:
                    {
                        fscanf(fpsym,"%x",&temp);
                        datatype[temp] |= 1;
                        break;
                    }
                    case 2:
                    {
                        reverse_bytes = 1;
                        break;
                    }
                }
                found = 1;
            }
            i++;
        }
        if (!found)
            done = 1;
    }

    fclose(fpsym);

    fprintf(stderr,"Finished reading symbol file...\n");

    }

    addr = org;

    while(!feof(fpcode) && (addr < 0x10000))
    {
        fread(&c1,1,1,fpcode);
        fread(&c2,1,1,fpcode);
        if (reverse_bytes)
            code[addr] = (((unsigned short)c2)<<8)+c1;
        else
            code[addr] = (((unsigned short)c1)<<8)+c2;
        addr++;
    }

    fprintf(stderr,"Finished reading code file...\n");

    endaddr = addr-1;

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

        w1 = code[addr];

        snprintf(display_buffer.address_field,sizeof(display_buffer.address_field),"%04hx",addr);

        if (datatype[addr] & 1)
        {
            snprintf(display_buffer.word1_field,sizeof(display_buffer.word1_field),"%04hx",w1);
            snprintf(display_buffer.opcode_field,sizeof(display_buffer.opcode_field),".word");
            snprintf(display_buffer.operand1_field,sizeof(display_buffer.operand1_field),"$%04hx",w1);
            s = 1;
        }
        else
        {

        datatype[addr] |= 4;

    if (dbd) dbd--;

        s = get_num_words(w1,dbd);

        strcpy(display_buffer.char_field, "[   ]");

        switch (s)
        {
            case 1:
            {
                snprintf(display_buffer.word1_field,sizeof(display_buffer.word1_field),"%04hx",w1);
                display_buffer.char_field[1] = (w1>=32 && w1<=126) ? w1 : '.';
                break;
            }
            case 2:
            {
                w2 = code[addr+1];
                snprintf(display_buffer.word1_field,sizeof(display_buffer.word1_field),"%04hx",w1);
                snprintf(display_buffer.word2_field,sizeof(display_buffer.word2_field),"%04hx",w2);
                display_buffer.char_field[1] = (w1>=32 && w1<=126) ? w1 : '.';
                display_buffer.char_field[2] = (w2>=32 && w2<=126) ? w2 : '.';
                break;
            }
            case 3:
            {
                w2 = code[addr+1];
                w3 = code[addr+2];
                snprintf(display_buffer.word1_field,sizeof(display_buffer.word1_field),"%04hx",w1);
                snprintf(display_buffer.word2_field,sizeof(display_buffer.word2_field),"%04hx",w2);
                snprintf(display_buffer.word3_field,sizeof(display_buffer.word3_field),"%04hx",w3);
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
                                        OPERAND1(DA);
                                        labeladdr = ((w2&0xfc)<<8)+(w3&0x3ff);
                                        datatype[labeladdr] |= 2;
                                        if (pass == 2)
                                        {
                                            if ((labeladdr >= org) && (labeladdr < endaddr))
                                                display_buffer.operand1_field[0] = 'L';
                                            else
                                                display_buffer.operand1_field[0] = 'G';
                                        }
                                    }
                                    else if ((w2 & 3)==1)
                                    {
                                        OPCODE("JE");
                                        OPERAND1(DA);
                                        labeladdr = ((w2&0xfc)<<8)+(w3&0x3ff);
                                        datatype[labeladdr] |= 2;
                                        if (pass == 2)
                                        {
                                            if ((labeladdr >= org) && (labeladdr < endaddr))
                                                display_buffer.operand1_field[0] = 'L';
                                            else
                                                display_buffer.operand1_field[0] = 'G';
                                        }
                                    }
                                    else if ((w2 & 3)==2)
                                    {
                                        OPCODE("JD");
                                        OPERAND1(DA);
                                        labeladdr = ((w2&0xfc)<<8)+(w3&0x3ff);
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
                                        OPCODE(".illop");
                                }
                                else
                                {
                                    if ((w2 & 3)==0)
                                    {
                                        OPCODE("JSR");
                                        OPERAND1(RB);
                                        OPERAND2(DA);
                                        labeladdr = ((w2&0xfc)<<8)+(w3&0x3ff);
                                        datatype[labeladdr] |= 2;
                                        if (pass == 2)
                                        {
                                            if ((labeladdr >= org) && (labeladdr < endaddr))
                                                display_buffer.operand2_field[0] = 'L';
                                            else
                                                display_buffer.operand2_field[0] = 'G';
                                        }
                                    }
                                    else if ((w2 & 3)==1)
                                    {
                                        OPCODE("JSRE");
                                        OPERAND1(RB);
                                        OPERAND2(DA);
                                        labeladdr = ((w2&0xfc)<<8)+(w3&0x3ff);
                                        datatype[labeladdr] |= 2;
                                        if (pass == 2)
                                        {
                                            if ((labeladdr >= org) && (labeladdr < endaddr))
                                                display_buffer.operand2_field[0] = 'L';
                                            else
                                                display_buffer.operand2_field[0] = 'G';
                                        }
                                    }
                                    else if ((w2 & 3)==2)
                                    {
                                        OPCODE("JSRD");
                                        OPERAND1(RB);
                                        OPERAND2(DA);
                                        labeladdr = ((w2&0xfc)<<8)+(w3&0x3ff);
                                        datatype[labeladdr] |= 2;
                                        if (pass == 2)
                                        {
                                            if ((labeladdr >= org) && (labeladdr < endaddr))
                                                display_buffer.operand2_field[0] = 'L';
                                            else
                                                display_buffer.operand2_field[0] = 'G';
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
                            OPCODE("NOP");
                            MAYBE2_1;
                        }
                        else
                        {
                            OPCODE("SIN");
                            MAYBE2_1;
                        }
                        break;
                    }
                    case 7:
                    {
                        OPCODE("RSWD");
                        OPERAND1(R3);
                        break;
                    }

                    break;
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
                    break;
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
                if ((w1 & 0x3) == 7)
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
                else if ((w1 & 0x1f) == 0x8)
                {
                    OPCODE("NOPP");
                }
                else if ((w1 & 0x10) == 0x00)
                {
                    OPCODE(branch_code[w1&0xf]);
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
                {
                    OPCODE("BEXT");
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
                    OPERAND2("E");
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
            break;
        }
        }

    if (pass == 2)
    {

        if (datatype[addr] & 2)
            snprintf(display_buffer.label_field,
                     sizeof(display_buffer.label_field),"L%04hx:",addr);

        printf("%-10s $%4s  %4s %4s %4s  %5s  %-8s %s%1s%s    %s\n",
                display_buffer.label_field,
                display_buffer.address_field,
                display_buffer.word1_field,
                display_buffer.word2_field,
                display_buffer.word3_field,
                display_buffer.char_field,
                display_buffer.opcode_field,
                display_buffer.operand1_field,
                display_buffer.operanddel_field,
                display_buffer.operand2_field,
                display_buffer.comment_field);

    }

    addr = (addr + s - 1) & 0xffff;
    }

    fprintf(stderr,"End of pass #%d\n",pass);

    }

    printf("\nSymbol Table:\n\n");

    for(i=0;i<0x10000;i++)
    {
        if (datatype[i] & 2)
        {
           if ((i >= org) && (i < endaddr))
                printf("L%04hx\n",i);
           else
                printf("G%04hx\n",i);
        }
    }

    return 0;
}
