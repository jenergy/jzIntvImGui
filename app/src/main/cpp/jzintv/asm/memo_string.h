/* ======================================================================== */
/*  String Memoization                                                      */
/*                                                                          */
/*  Rather than strdup'ing strings all over the place because we don't      */
/*  know where they'll be needed or when they'll be freed.  We can free     */
/*  the whole lot at the end if it matters.                                 */
/*                                                                          */
/*  Naturally, it only works for read-only strings.  It does allocate its   */
/*  own private copy.                                                       */
/* ======================================================================== */
#ifndef MEMO_STRING_H_
#define MEMO_STRING_H_ 1

char       *arena_dup(const char *string);
const char *memoize_string(const char *string);
void        reset_memoize_string(void);

#endif
