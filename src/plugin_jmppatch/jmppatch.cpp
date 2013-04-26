#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/SimpleLayout.hh>
#include <idp.hpp>
#include <funcs.hpp>
#include <bytes.hpp>
#include <auto.hpp>
#include <list>
#include <allins.hpp>

using namespace std;


/*!
Returns near, absolute indirect address if the instruction at head is a near absolute jmp.
*/
ea_t GetNearAbsJmpIndirectAddr(log4cpp::Category &log, ea_t head)
{
	// Fill up cmd structure.
	decode_insn(head);

	// Make sure it is an absolute indirect jmp.
	if(cmd.itype != NN_jmpni) return BADADDR;

	return cmd.Op1.addr;
}


/*!
Returns true if the function is a trampoline function.
*/
bool IsTrampolineFunc(
	log4cpp::Category &log,
	ea_t funcStartEA, ea_t funcEndEA)
{
	if (get_func(funcStartEA) == NULL)
	{	// Not a function
		return false;
	}


	// A trampoline function should only have 1 instruction.
	// So we try to get the next instruction and make
	// sure it is either a BADADDR or its address is after funcEndEA.
	ea_t next_instr_ea = next_head(funcStartEA, funcStartEA + 1000);
	if ( next_instr_ea != BADADDR && next_instr_ea < funcEndEA )
	{
		log.info( "\t%016llx is NOT a trampoline function. Possibly more than one instruction.", funcStartEA);
		return false;
	}


	char mnem[MAXSTR] = {0};
	ua_mnem(funcStartEA, mnem, sizeof(mnem)-1);
	//log.info( "\tHas only 1 instruction.: %016llx, %s, %lld",
	//	funcStartEA, mnem, funcEndEA - funcStartEA );

	decode_insn( funcStartEA );
	if ( cmd.itype < NN_ja || cmd.itype > NN_jmpshort )
	{
		log.info( "\t%016llx is NOT a trampoline function. Possibly not jxx instruction.", funcStartEA);
		return false;
	}

	log.info( "\t%016llx is a trampoline function.", funcStartEA );
	return true;
}

void JmpPatch(log4cpp::Category &log)
{
	int total_patched_call = 0;
	std::list<ea_t> trampolines;

	//
	// For each segment...
	//
	segment_t *seg = get_first_seg();
	while( seg != NULL )
	{

		char segName[MAXSTR] = { '\0' };
		get_segm_name(seg, segName, MAXSTR);

		log.info("Segment: \"%s\" %016llx : %016llx", segName, seg->startEA, seg->endEA);

		if (strncmp(segName, "_plt", 4) ==0)
		{
			log.info("Checking function in plt...");
			// For each function in the _plt segment ...
			func_t *func;
			ea_t funcStartEA, funcEndEA;
			func  = get_next_func(seg->startEA);

			while (func != NULL)
			{
				log.info("funcStartEA %016llx", funcStartEA);

				funcStartEA = func->startEA;
				funcEndEA = func->endEA;
				if (funcEndEA > seg->endEA) break;

				// funcName is only for Debugging. Can be deleted.
				char funcName[MAXSTR];
				get_func_name(funcStartEA, funcName, MAXSTR);
				log.info("\t%s (%016llx)", funcName, funcStartEA);

				if ( IsTrampolineFunc( log, funcStartEA, funcEndEA) == false )
					continue;

				log.info("\t\tIs Trampoline.");

				// Get the indrect address.
				ea_t jmpIndirectAddr = GetNearAbsJmpIndirectAddr(log, funcStartEA);
				if ( jmpIndirectAddr == BADADDR )
					continue;

				ea_t jmpAddr = get_first_dref_from(jmpIndirectAddr);
				log.info("\t\tJumping to %016llx", jmpAddr);


				// For all calls to this function...
				for ( ea_t xref_frm = get_first_cref_to(funcStartEA);
				xref_frm != BADADDR;
				xref_frm = get_next_cref_to(funcStartEA, xref_frm) )
				{
					decode_insn(xref_frm);
					if( cmd.itype < NN_call || cmd.itype > NN_callni ||
					cmd.Operands[0].type == o_reg ) continue;

					log.info("\t\tCalled from %016llx", xref_frm);

					// Get address offset which we'll use to patch
					segment_t *callSeg = getseg(xref_frm);
					ea_t instrAfterCall = next_head(xref_frm, callSeg->endEA);
					ea_t offset = jmpAddr - instrAfterCall;

					// Patch call instruction
					//unsigned char *patchAddr = (unsigned char*)&offset;
					log.info("\t\tPatching at %016llx with %016llx",
						xref_frm+cmd.Op1.offb,
						offset);

					unsigned char *patchAddr =  (unsigned char *) &offset;

					log.info("\t\t\tPatching byte %016llx with %02x", xref_frm+1, patchAddr[0]);
					log.info("\t\t\tPatching byte %016llx with %02x", xref_frm+2, patchAddr[1]);
					log.info("\t\t\tPatching byte %016llx with %02x", xref_frm+3, patchAddr[2]);
					log.info("\t\t\tPatching byte %016llx with %02x", xref_frm+4, patchAddr[3]);
	
					decode_insn(xref_frm);	
					//patch_long(xref_frm+1, offset);
					int max = 0;
					if( cmd.Op1.dtyp == dt_dword )
					{
						max = 4;
					}
					else if( cmd.Op1.dtyp == dt_qword )
					{
						max = 8;
					}

					for(int i=0; i < max; ++i)
						patch_byte(xref_frm+cmd.Op1.offb+i, patchAddr[i]);
				}

				//
				// Remove the trampoline function
				//

				// Patch with NOPs
				decode_insn(funcStartEA);
				for (int i = 0; i < cmd.size; ++i)
					patch_byte(funcStartEA+i, 0x90);

				//do_unknown(funcStartEA, DOUNK_EXPAND);
				auto_mark_range(funcStartEA, funcStartEA+6, AU_UNK);
				//noUsed(funcStartEA, funcStartEA+6);
				func = get_next_func(funcEndEA);
			}
		}

		analyze_area(seg->startEA, seg->endEA);
		seg = get_next_seg(seg->startEA);
	}

	bool result = autoWait();
	log.info("JmpPatch Done. autoWait() returned %d.", result);
}
