/*
Vine is Copyright (C) 2006-2009, BitBlaze Team.

You can redistribute and modify it under the terms of the GNU GPL,
version 2 or later, but it is made available WITHOUT ANY WARRANTY.
See the top-level README file for more details.

For more information about Vine and other BitBlaze software, see our
web site at: http://bitblaze.cs.berkeley.edu/
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <iostream>
#include "asm_program.h"


using namespace std;

void usage(char *prog)
{
  printf("Usage: %s filename\n", prog);
}

void print_prog_insns(asm_program_t *prog)
{
  for(map<address_t, asm_function_t *>::const_iterator it = 
	prog->functions.begin(); it != prog->functions.end();
      it++){
    asm_function_t *f = it->second;
    cout << f->name << " (" << hex << f->start_addr << "-"
	 << f->end_addr << ")" << endl;
    for(map<address_t, Instruction *>::const_iterator i2 = 
	  f->instmap.begin(); i2 != f->instmap.end(); i2++){
      Instruction *inst = i2->second;
      ostream_insn(prog, inst, cout);
      cout << endl;
    }
  }
}

int main(int argc, char *argv[])
{
  if(argc != 2){
    usage(argv[0]);
    return -1;
  }

  asm_program_t *prog = disassemble_program(argv[1]);
  assert(prog != NULL);
  print_prog_insns(prog);
}
