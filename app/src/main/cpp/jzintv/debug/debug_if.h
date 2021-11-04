/*
 * ============================================================================
 *  Title:    Simple Debugger
 *  Author:   J. Zbiciak
 * ============================================================================
 *
 * ============================================================================
 *
 * ============================================================================
 */

#ifndef DEBUG_IF_H_
#define DEBUG_IF_H_

/* ======================================================================== */
/*  Some asynchronous fault controls                                        */
/* ======================================================================== */
#define DEBUG_HLT_INSTR  ( 2)
#define DEBUG_CRASHING   ( 1)
#define DEBUG_NO_FAULT   ( 0)
#define DEBUG_ASYNC_HALT (-1)
extern        int  debug_fault_detected;
extern const char *debug_halt_reason;

#endif
