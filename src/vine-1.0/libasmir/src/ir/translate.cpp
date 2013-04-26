/*
Vine is Copyright (C) 2006-2009, BitBlaze Team.

You can redistribute and modify it under the terms of the GNU GPL,
version 2 or later, but it is made available WITHOUT ANY WARRANTY.
See the top-level README file for more details.

For more information about Vine and other BitBlaze software, see our
web site at: http://bitblaze.cs.berkeley.edu/
*/

#include "translate.h"
#include <bfd.h>

extern bool use_eflags_thunks;


// from asm_program.cpp
void init_disasm_info(asm_program_t *prog);

asm_program_t *instmap_to_asm_program(instmap_t *map)
{
  return map->prog;
}

// Returns a fake asm_program_t for use when disassembling bits out of memory
asm_program_t* fake_prog_for_arch(enum bfd_architecture arch)
{
  int machine = 0; // TODO: pick based on arch
  asm_program_t *prog = new asm_program_t;
  
  prog->abfd = bfd_openw("/dev/null", NULL);
  assert(prog->abfd);
  bfd_set_arch_info(prog->abfd, bfd_lookup_arch(arch, machine));

  //not in .h bfd_default_set_arch_mach(prog->abfd, arch, machine);
  bfd_set_arch_info(prog->abfd, bfd_lookup_arch(arch, machine));
  init_disasm_info(prog);
  return prog;
}


int instmap_has_address(instmap_t *insts,
			address_t addr)
{
  return (insts->imap.find(addr) != insts->imap.end());
}

/*
cflow_t *instmap_control_flow_type(instmap_t *insts,
		      address_t addr)
{
  cflow_t * ret = (cflow_t *) malloc(sizeof(cflow_t));

  assert(insts->imap.find(addr) != insts->imap.end());
  Instruction *inst = insts->imap.find(addr)->second;
  address_t target;
  ret->ctype = get_control_transfer_type(inst, &target);
  ret->target = target;
  return ret;
}
*/

vine_block_t *instmap_translate_address(instmap_t *insts,
					address_t addr)
{
  translate_init();
  vx_FreeAll();
  vector<Instruction *> foo;

  assert(insts->imap.find(addr) != insts->imap.end());

  foo.push_back(insts->imap.find(addr)->second);
  vector<vine_block_t *> blocks = generate_vex_ir(insts->prog, foo);
  blocks = generate_vine_ir(insts->prog, blocks);
  assert(blocks.size() == 1);
  vine_block_t *block = *(blocks.begin());
  block->vex_ir = NULL;
  vx_FreeAll();
  return block;
}


vine_blocks_t *instmap_translate_address_range(instmap_t *insts,
					   address_t start,
					   address_t end)
{
  translate_init();
  vx_FreeAll();
  vector<Instruction *> foo;
  vine_blocks_t *ret = new vine_blocks_t;

  for(address_t i = start; i <= end; i++){
    if(insts->imap.find(i) == insts->imap.end()) continue;
    foo.push_back(insts->imap.find(i)->second);
   }
  vector<vine_block_t *> blocks = generate_vex_ir(insts->prog, foo);
  blocks = generate_vine_ir(insts->prog, blocks);

  for(unsigned i = 0; i < blocks.size(); i++){
    vine_block_t *block = blocks.at(i);
    assert(block);
    
    if(block->inst == NULL)
      continue;
    
    /* This is broken as it removes the jumps from the end of
       repz instructions. Just because control flow eventually goes on
       to the following instruction doesn't mean that jump is at the end.
       It should simply be checking that the jump is to the following
       instruction since we know where that is.
    // If the statement isn't control flow, and not the last
    // remove the dummy jumps to the next block that vex inserts
    bfd_vma target;
    int ctype = get_control_transfer_type(block->inst, &target);
    Stmt *s;
    switch(ctype){
    case INST_CONT:
      s = block->vine_ir->back();
      if(s->stmt_type == JMP){
	block->vine_ir->pop_back();
      }
      break;
    default:
      break;
    }
    */
  }
  
  ret->insert(ret->end(), blocks.begin(), blocks.end());
  
  for(vector<vine_block_t *>::iterator it = ret->begin();
      it != ret->end(); it++){
    vine_block_t *block = *it;
    block->vex_ir = NULL; // this isn't available. 
  }

  return ret;
}



address_t get_last_segment_address(const char *filename, 
				   address_t addr)
{
  bfd *abfd = initialize_bfd(filename);
  unsigned int opb = bfd_octets_per_byte(abfd);

  address_t ret = addr;
  for(asection *section = abfd->sections; 
      section != (asection *)  NULL; section = section->next){

    bfd_vma datasize = bfd_get_section_size_before_reloc(section);
    if(datasize == 0) continue;

    address_t start = section->vma;
    address_t end = section->vma + datasize/opb;
    if(addr >= start && addr <= end)
      return end;
  }
  return ret;
}

instmap_t *
filename_to_instmap(const char *filename)
{
  instmap_t *ret = new instmap_t;
  asm_program_t *prog = disassemble_program(filename);
  ret->prog = prog;

  // iterate over functions
  for ( map<address_t, asm_function_t *>::const_iterator i = prog->functions.begin();
	i != prog->functions.end(); i++ )
    {
      asm_function_t *f = i->second;
      ret->imap.insert(f->instmap.begin(), f->instmap.end());
      //ret->insert(pair<address_t, Instruction *>(inst->address, inst));
    }
 
  return ret;
}


// from asm_program.cpp
void init_disasm_info(bfd *abfd, struct disassemble_info *disasm_info);


// Quick helper function for ocaml, since we don't have a proper libopcodes
// interface yet.
// This isn't really the right place for it...
extern "C" {
void print_disasm_rawbytes(enum bfd_architecture arch,
			   bfd_vma addr,
			   const char *buf, int size)
{
  asm_program_t *prog = byte_insn_to_asmp(arch, addr, (unsigned char*)buf, size);
  
  if (!prog)
    return;

  disassembler_ftype disas = disassembler(prog->abfd);
  disas(addr, &prog->disasm_info);
  free_asm_program(prog);
  fflush(stdout);
}

}
