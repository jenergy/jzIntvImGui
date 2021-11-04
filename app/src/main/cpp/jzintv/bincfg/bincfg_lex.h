#ifndef BINCFG_LEX_H_
#define BINCFG_LEX_H_

extern int      bc_line_no;
extern int      bc_dec;
extern uint32_t  bc_hex;
extern char     *bc_txt, *bc_text;

extern void     bc_restart(FILE *f);
extern int      bc_lex(void);

#endif
