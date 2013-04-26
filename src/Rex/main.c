#ifdef __cplusplus
extern "C" {
#endif
#include "libvex.h"
#ifdef __cplusplus
}
#endif
#include <stdlib.h>
//#include <rex.h>
#include "vexmem.h"
#include "irtoir.h"

#include <stdio.h>

int main()
{
	unsigned char insn[] = { 0xf7, 0xd0 };

	/*
	Rex r;
	r.ToIR( insn, (unsigned int)0x8000 );
	*/

	
	translate_init();
	IRSB *bb = translate_insn( VexArchX86, insn, (unsigned int)0x8000000);
	
	printf("stmts_size: %d\n", bb->stmts_size);
	printf("stmts_used: %d\n", bb->stmts_used);

	IRStmt **stmts = bb->stmts;
	printf("stmts->tag: %08x\n", (*stmts)->tag );
	printf("stmts->IMark->addr: %08x\n", (*stmts)->Ist.IMark.addr);
	printf("stmts->IMark->len: %d\n", (*stmts)->Ist.IMark.len);
}
