/* Auto generated, do not edit */

#include "cp1600/op_tables.h"
const uint8_t dec_format[] =
{
/*0000000000*/   fmt_impl_1op_a,    /* Implied 1-op arithmetic  (a)     */
/*0000000001*/   fmt_impl_1op_a,    /* Implied 1-op arithmetic  (a)     */
/*0000000010*/   fmt_impl_1op_a,    /* Implied 1-op arithmetic  (a)     */
/*0000000011*/   fmt_impl_1op_a,    /* Implied 1-op arithmetic  (a)     */
/*0000000100*/   fmt_jump,          /* Absolute jump instructions       */
/*0000000101*/   fmt_impl_1op_b,    /* Implied 1-op arithmetic  (b)     */
/*0000000110*/   fmt_impl_1op_b,    /* Implied 1-op arithmetic  (b)     */
/*0000000111*/   fmt_impl_1op_b,    /* Implied 1-op arithmetic  (b)     */
/*0000001000*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000001001*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000001010*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000001011*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000001100*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000001101*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000001110*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000001111*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000010000*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000010001*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000010010*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000010011*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000010100*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000010101*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000010110*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000010111*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000011000*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000011001*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000011010*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000011011*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000011100*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000011101*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000011110*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000011111*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000100000*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000100001*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000100010*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000100011*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000100100*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000100101*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000100110*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000100111*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000101000*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000101001*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000101010*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000101011*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000101100*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000101101*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000101110*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000101111*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000110000*/   fmt_gswd,          /* GSWD -- Get Status WorD insn.    */
/*0000110001*/   fmt_gswd,          /* GSWD -- Get Status WorD insn.    */
/*0000110010*/   fmt_gswd,          /* GSWD -- Get Status WorD insn.    */
/*0000110011*/   fmt_gswd,          /* GSWD -- Get Status WorD insn.    */
/*0000110100*/   fmt_nop_sin,       /* NOP, SIN instructions            */
/*0000110101*/   fmt_nop_sin,       /* NOP, SIN instructions            */
/*0000110110*/   fmt_nop_sin,       /* NOP, SIN instructions            */
/*0000110111*/   fmt_nop_sin,       /* NOP, SIN instructions            */
/*0000111000*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000111001*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000111010*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000111011*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000111100*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000111101*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000111110*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0000111111*/   fmt_reg_1op,       /* Combined Src/Dst Register 1-op   */
/*0001000000*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001000001*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001000010*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001000011*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001000100*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001000101*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001000110*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001000111*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001001000*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001001001*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001001010*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001001011*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001001100*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001001101*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001001110*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001001111*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001010000*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001010001*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001010010*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001010011*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001010100*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001010101*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001010110*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001010111*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001011000*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001011001*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001011010*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001011011*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001011100*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001011101*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001011110*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001011111*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001100000*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001100001*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001100010*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001100011*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001100100*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001100101*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001100110*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001100111*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001101000*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001101001*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001101010*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001101011*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001101100*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001101101*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001101110*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001101111*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001110000*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001110001*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001110010*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001110011*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001110100*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001110101*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001110110*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001110111*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001111000*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001111001*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001111010*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001111011*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001111100*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001111101*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001111110*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0001111111*/   fmt_rot_1op,       /* Rotate/Shift Register 1-op       */
/*0010000000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010000001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010000010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010000011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010000100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010000101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010000110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010000111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010001000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010001001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010001010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010001011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010001100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010001101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010001110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010001111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010010000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010010001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010010010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010010011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010010100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010010101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010010110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010010111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010011000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010011001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010011010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010011011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010011100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010011101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010011110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010011111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010100000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010100001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010100010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010100011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010100100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010100101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010100110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010100111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010101000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010101001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010101010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010101011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010101100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010101101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010101110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010101111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010110000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010110001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010110010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010110011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010110100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010110101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010110110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010110111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010111000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010111001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010111010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010111011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010111100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010111101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010111110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0010111111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011000000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011000001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011000010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011000011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011000100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011000101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011000110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011000111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011001000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011001001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011001010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011001011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011001100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011001101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011001110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011001111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011010000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011010001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011010010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011010011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011010100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011010101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011010110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011010111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011011000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011011001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011011010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011011011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011011100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011011101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011011110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011011111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011100000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011100001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011100010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011100011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011100100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011100101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011100110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011100111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011101000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011101001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011101010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011101011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011101100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011101101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011101110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011101111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011110000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011110001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011110010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011110011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011110100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011110101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011110110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011110111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011111000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011111001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011111010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011111011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011111100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011111101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011111110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0011111111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100000000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100000001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100000010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100000011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100000100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100000101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100000110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100000111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100001000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100001001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100001010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100001011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100001100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100001101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100001110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100001111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100010000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100010001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100010010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100010011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100010100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100010101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100010110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100010111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100011000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100011001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100011010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100011011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100011100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100011101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100011110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100011111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100100000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100100001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100100010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100100011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100100100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100100101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100100110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100100111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100101000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100101001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100101010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100101011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100101100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100101101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100101110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100101111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100110000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100110001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100110010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100110011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100110100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100110101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100110110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100110111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100111000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100111001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100111010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100111011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100111100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100111101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100111110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0100111111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101000000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101000001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101000010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101000011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101000100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101000101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101000110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101000111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101001000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101001001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101001010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101001011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101001100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101001101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101001110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101001111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101010000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101010001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101010010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101010011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101010100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101010101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101010110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101010111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101011000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101011001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101011010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101011011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101011100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101011101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101011110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101011111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101100000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101100001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101100010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101100011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101100100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101100101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101100110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101100111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101101000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101101001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101101010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101101011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101101100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101101101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101101110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101101111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101110000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101110001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101110010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101110011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101110100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101110101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101110110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101110111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101111000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101111001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101111010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101111011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101111100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101111101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101111110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0101111111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110000000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110000001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110000010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110000011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110000100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110000101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110000110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110000111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110001000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110001001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110001010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110001011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110001100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110001101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110001110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110001111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110010000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110010001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110010010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110010011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110010100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110010101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110010110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110010111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110011000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110011001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110011010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110011011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110011100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110011101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110011110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110011111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110100000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110100001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110100010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110100011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110100100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110100101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110100110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110100111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110101000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110101001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110101010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110101011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110101100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110101101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110101110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110101111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110110000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110110001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110110010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110110011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110110100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110110101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110110110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110110111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110111000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110111001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110111010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110111011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110111100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110111101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110111110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0110111111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111000000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111000001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111000010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111000011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111000100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111000101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111000110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111000111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111001000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111001001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111001010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111001011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111001100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111001101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111001110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111001111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111010000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111010001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111010010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111010011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111010100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111010101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111010110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111010111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111011000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111011001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111011010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111011011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111011100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111011101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111011110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111011111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111100000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111100001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111100010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111100011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111100100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111100101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111100110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111100111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111101000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111101001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111101010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111101011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111101100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111101101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111101110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111101111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111110000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111110001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111110010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111110011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111110100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111110101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111110110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111110111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111111000*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111111001*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111111010*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111111011*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111111100*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111111101*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111111110*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*0111111111*/   fmt_reg_2op,       /* Register  -> Register 2-op arith */
/*1000000000*/   fmt_cond_br,       /* Conditional branch.              */
/*1000000001*/   fmt_cond_br,       /* Conditional branch.              */
/*1000000010*/   fmt_cond_br,       /* Conditional branch.              */
/*1000000011*/   fmt_cond_br,       /* Conditional branch.              */
/*1000000100*/   fmt_cond_br,       /* Conditional branch.              */
/*1000000101*/   fmt_cond_br,       /* Conditional branch.              */
/*1000000110*/   fmt_cond_br,       /* Conditional branch.              */
/*1000000111*/   fmt_cond_br,       /* Conditional branch.              */
/*1000001000*/   fmt_cond_br,       /* Conditional branch.              */
/*1000001001*/   fmt_cond_br,       /* Conditional branch.              */
/*1000001010*/   fmt_cond_br,       /* Conditional branch.              */
/*1000001011*/   fmt_cond_br,       /* Conditional branch.              */
/*1000001100*/   fmt_cond_br,       /* Conditional branch.              */
/*1000001101*/   fmt_cond_br,       /* Conditional branch.              */
/*1000001110*/   fmt_cond_br,       /* Conditional branch.              */
/*1000001111*/   fmt_cond_br,       /* Conditional branch.              */
/*1000010000*/   fmt_cond_br,       /* Conditional branch.              */
/*1000010001*/   fmt_cond_br,       /* Conditional branch.              */
/*1000010010*/   fmt_cond_br,       /* Conditional branch.              */
/*1000010011*/   fmt_cond_br,       /* Conditional branch.              */
/*1000010100*/   fmt_cond_br,       /* Conditional branch.              */
/*1000010101*/   fmt_cond_br,       /* Conditional branch.              */
/*1000010110*/   fmt_cond_br,       /* Conditional branch.              */
/*1000010111*/   fmt_cond_br,       /* Conditional branch.              */
/*1000011000*/   fmt_cond_br,       /* Conditional branch.              */
/*1000011001*/   fmt_cond_br,       /* Conditional branch.              */
/*1000011010*/   fmt_cond_br,       /* Conditional branch.              */
/*1000011011*/   fmt_cond_br,       /* Conditional branch.              */
/*1000011100*/   fmt_cond_br,       /* Conditional branch.              */
/*1000011101*/   fmt_cond_br,       /* Conditional branch.              */
/*1000011110*/   fmt_cond_br,       /* Conditional branch.              */
/*1000011111*/   fmt_cond_br,       /* Conditional branch.              */
/*1000100000*/   fmt_cond_br,       /* Conditional branch.              */
/*1000100001*/   fmt_cond_br,       /* Conditional branch.              */
/*1000100010*/   fmt_cond_br,       /* Conditional branch.              */
/*1000100011*/   fmt_cond_br,       /* Conditional branch.              */
/*1000100100*/   fmt_cond_br,       /* Conditional branch.              */
/*1000100101*/   fmt_cond_br,       /* Conditional branch.              */
/*1000100110*/   fmt_cond_br,       /* Conditional branch.              */
/*1000100111*/   fmt_cond_br,       /* Conditional branch.              */
/*1000101000*/   fmt_cond_br,       /* Conditional branch.              */
/*1000101001*/   fmt_cond_br,       /* Conditional branch.              */
/*1000101010*/   fmt_cond_br,       /* Conditional branch.              */
/*1000101011*/   fmt_cond_br,       /* Conditional branch.              */
/*1000101100*/   fmt_cond_br,       /* Conditional branch.              */
/*1000101101*/   fmt_cond_br,       /* Conditional branch.              */
/*1000101110*/   fmt_cond_br,       /* Conditional branch.              */
/*1000101111*/   fmt_cond_br,       /* Conditional branch.              */
/*1000110000*/   fmt_cond_br,       /* Conditional branch.              */
/*1000110001*/   fmt_cond_br,       /* Conditional branch.              */
/*1000110010*/   fmt_cond_br,       /* Conditional branch.              */
/*1000110011*/   fmt_cond_br,       /* Conditional branch.              */
/*1000110100*/   fmt_cond_br,       /* Conditional branch.              */
/*1000110101*/   fmt_cond_br,       /* Conditional branch.              */
/*1000110110*/   fmt_cond_br,       /* Conditional branch.              */
/*1000110111*/   fmt_cond_br,       /* Conditional branch.              */
/*1000111000*/   fmt_cond_br,       /* Conditional branch.              */
/*1000111001*/   fmt_cond_br,       /* Conditional branch.              */
/*1000111010*/   fmt_cond_br,       /* Conditional branch.              */
/*1000111011*/   fmt_cond_br,       /* Conditional branch.              */
/*1000111100*/   fmt_cond_br,       /* Conditional branch.              */
/*1000111101*/   fmt_cond_br,       /* Conditional branch.              */
/*1000111110*/   fmt_cond_br,       /* Conditional branch.              */
/*1000111111*/   fmt_cond_br,       /* Conditional branch.              */
/*1001000000*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1001000001*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1001000010*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1001000011*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1001000100*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1001000101*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1001000110*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1001000111*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1001001000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001001001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001001010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001001011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001001100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001001101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001001110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001001111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001010000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001010001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001010010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001010011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001010100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001010101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001010110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001010111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001011000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001011001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001011010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001011011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001011100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001011101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001011110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001011111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001100000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001100001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001100010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001100011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001100100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001100101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001100110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001100111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001101000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001101001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001101010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001101011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001101100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001101101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001101110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001101111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001110000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001110001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001110010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001110011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001110100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001110101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001110110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001110111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1001111000*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1001111001*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1001111010*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1001111011*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1001111100*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1001111101*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1001111110*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1001111111*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1010000000*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1010000001*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1010000010*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1010000011*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1010000100*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1010000101*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1010000110*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1010000111*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1010001000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010001001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010001010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010001011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010001100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010001101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010001110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010001111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010010000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010010001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010010010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010010011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010010100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010010101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010010110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010010111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010011000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010011001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010011010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010011011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010011100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010011101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010011110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010011111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010100000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010100001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010100010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010100011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010100100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010100101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010100110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010100111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010101000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010101001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010101010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010101011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010101100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010101101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010101110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010101111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010110000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010110001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010110010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010110011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010110100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010110101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010110110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010110111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1010111000*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1010111001*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1010111010*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1010111011*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1010111100*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1010111101*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1010111110*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1010111111*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1011000000*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1011000001*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1011000010*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1011000011*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1011000100*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1011000101*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1011000110*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1011000111*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1011001000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011001001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011001010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011001011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011001100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011001101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011001110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011001111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011010000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011010001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011010010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011010011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011010100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011010101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011010110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011010111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011011000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011011001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011011010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011011011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011011100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011011101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011011110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011011111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011100000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011100001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011100010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011100011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011100100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011100101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011100110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011100111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011101000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011101001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011101010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011101011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011101100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011101101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011101110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011101111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011110000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011110001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011110010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011110011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011110100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011110101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011110110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011110111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1011111000*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1011111001*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1011111010*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1011111011*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1011111100*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1011111101*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1011111110*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1011111111*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1100000000*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1100000001*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1100000010*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1100000011*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1100000100*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1100000101*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1100000110*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1100000111*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1100001000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100001001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100001010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100001011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100001100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100001101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100001110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100001111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100010000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100010001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100010010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100010011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100010100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100010101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100010110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100010111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100011000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100011001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100011010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100011011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100011100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100011101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100011110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100011111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100100000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100100001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100100010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100100011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100100100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100100101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100100110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100100111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100101000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100101001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100101010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100101011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100101100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100101101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100101110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100101111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100110000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100110001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100110010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100110011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100110100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100110101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100110110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100110111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1100111000*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1100111001*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1100111010*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1100111011*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1100111100*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1100111101*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1100111110*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1100111111*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1101000000*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1101000001*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1101000010*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1101000011*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1101000100*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1101000101*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1101000110*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1101000111*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1101001000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101001001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101001010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101001011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101001100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101001101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101001110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101001111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101010000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101010001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101010010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101010011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101010100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101010101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101010110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101010111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101011000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101011001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101011010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101011011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101011100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101011101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101011110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101011111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101100000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101100001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101100010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101100011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101100100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101100101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101100110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101100111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101101000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101101001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101101010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101101011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101101100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101101101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101101110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101101111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101110000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101110001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101110010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101110011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101110100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101110101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101110110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101110111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1101111000*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1101111001*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1101111010*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1101111011*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1101111100*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1101111101*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1101111110*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1101111111*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1110000000*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1110000001*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1110000010*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1110000011*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1110000100*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1110000101*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1110000110*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1110000111*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1110001000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110001001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110001010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110001011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110001100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110001101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110001110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110001111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110010000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110010001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110010010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110010011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110010100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110010101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110010110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110010111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110011000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110011001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110011010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110011011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110011100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110011101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110011110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110011111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110100000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110100001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110100010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110100011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110100100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110100101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110100110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110100111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110101000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110101001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110101010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110101011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110101100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110101101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110101110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110101111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110110000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110110001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110110010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110110011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110110100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110110101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110110110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110110111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1110111000*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1110111001*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1110111010*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1110111011*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1110111100*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1110111101*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1110111110*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1110111111*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1111000000*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1111000001*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1111000010*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1111000011*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1111000100*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1111000101*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1111000110*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1111000111*/   fmt_dir_2op,       /* Direct    -> Register 2-op arith */
/*1111001000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111001001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111001010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111001011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111001100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111001101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111001110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111001111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111010000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111010001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111010010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111010011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111010100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111010101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111010110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111010111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111011000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111011001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111011010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111011011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111011100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111011101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111011110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111011111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111100000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111100001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111100010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111100011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111100100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111100101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111100110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111100111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111101000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111101001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111101010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111101011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111101100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111101101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111101110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111101111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111110000*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111110001*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111110010*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111110011*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111110100*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111110101*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111110110*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111110111*/   fmt_ind_2op,       /* Indirect  -> Register 2-op arith */
/*1111111000*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1111111001*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1111111010*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1111111011*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1111111100*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1111111101*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1111111110*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
/*1111111111*/   fmt_imm_2op,       /* Immediate -> Register 2-op arith */
};
