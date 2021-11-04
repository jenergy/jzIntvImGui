#ifndef EMITTERS_H_
#define EMITTERS_H_ 1

void emit_listing_mode(const listing_mode m);
void emit_comment(const int is_user, const char *const format, ...);
void emit_set_equ(const unsigned int value);
void emit_location(const int seg, const int pag, const int loc,
                   const int type, const char *const mode);
void emit_mark_with_mode(const int lo, const int hi, const char *const mode);
void emit_reserve(const int endaddr);
void emit_entering_file(const char *const name);
void emit_exiting_file (const char *const name);
void emit_warnerr(const char *const file, const int line, const warnerr type,
                  const char *const format, ...);
void emit_raw_error(const char *const raw_error);
void emit_listed_line(const char *const buf);
void emit_unlisted_line(void);
void emit_generated_instr(const char *const buf);
void emit_cfgvar_int(const char *const var, const int value);
void emit_cfgvar_str(const char *const var, const char *const value);
void emit_srcfile_override(const char *const var, const int value);
void emit_as1600_cfg_int(const as1600_cfg_item item, const int value);
void emit_listing_column(const int hex_source, const int hex_no_src,
                         const int source_col);
void emit_err_if_overwritten(const int enable_warning);
void emit_force_overwrite(const int force_overwrite);

void intermed_start_pass_1(void);
void intermed_start_pass_2(void);
void intermed_finish(const int debugmode);


irec_union *pass2_next_rec(void);
void        pass2_release_rec(irec_union *const irec);

#endif
