/*
 Owned and copyright BitBlaze, 2007. All rights reserved.
 Do not copy, disclose, or distribute without explicit written
 permission.
*/

#ifndef __IRTOIR_H
#define __IRTOIR_H

#include "vexmem.h"
#include "libvex.h"

//======================================================================
// 
// Translation functions. These should not be used directly unless you
// understand exactly how they work and have a specific need for them.
// If you just wanna translate a program, see generate_vex_ir and 
// generate_bap_ir at the bottom. They wrap these functions and
// provide an easier interface for translation. 
//
//======================================================================

//
// Initializes VEX. This function must be called before translate_insn
// can be used. 
// vexir.c
void translate_init();

//
// Translates 1 asm instruction (in byte form) into a block of VEX IR
//
// \param insn_start Pointer to bytes of instruction
// \param insn_addr Address of the instruction in its own address space
// \return An IRSB containing the VEX IR translation of the given instruction
// vexir.c
IRSB *translate_insn( VexArch guest, unsigned char *insn_start, unsigned int insn_addr );

#endif

