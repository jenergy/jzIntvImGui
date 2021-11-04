/* Quick and Dirty header for interfacing to Frank Palazzolo's Disassembler */

#ifndef DASM1600_H_
#define DASM1600_H_

int dasm1600(char *outbuf, int addr, int dbd, int w1, int w2, int w3);

int set_symb_addr_format(int);

#endif

