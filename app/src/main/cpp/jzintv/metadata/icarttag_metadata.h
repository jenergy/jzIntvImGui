#ifndef ICARTTAG_METADATA_H_
#define ICARTTAG_METADATA_H_

/* ======================================================================== */
/*  This is a little messy, owing to the ICARTTAG visitor interface.        */
/*                                                                          */
/*  GET_GAME_METADATA_ICARTTAG_VISITOR   -- Gets a visitor object           */
/*  PUT_GAME_METADATA_ICARTTAG_VISITOR   -- Destroys visitor; returns the   */
/*                                          game_metadata_t it constructed  */
/* ======================================================================== */

/* Forward Declarations */
struct icarttag_visitor_t;
struct game_metadata_t;

/* ======================================================================== */
/*  GET_GAME_METADATA_ICARTTAG_VISITOR                                      */
/*                                                                          */
/*  Returns an icarttag_visitor_t that has an empty game_metadata_t object  */
/*  attached.  This object is suitable for passing to icarttag_decode.      */
/* ======================================================================== */
struct icarttag_visitor_t *get_game_metadata_icarttag_visitor(void);

/* ======================================================================== */
/*  PUT_GAME_METADATA_ICARTTAG_VISITOR                                      */
/*                                                                          */
/*  Frees the icarttag_visitor_t from get_game_metadata_icarttag_visitor.   */
/*  Returns the game_metadata_t that was constructed, if any.               */
/* ======================================================================== */
struct game_metadata_t *
    put_game_metadata_icarttag_visitor(struct icarttag_visitor_t *);

#endif
