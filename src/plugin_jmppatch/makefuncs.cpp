#include <ida.hpp>
#include <idp.hpp>
#include <segment.hpp>
#include <funcs.hpp>
#include <auto.hpp>

int makefuncs( )
{
	// Loop through all segments
	for( int i = 0; i < get_segm_qty(); ++i )
	{
		segment_t *seg = getnseg(i);

		// Skip non-code segments
		if( seg->type != SEG_CODE ) continue;

		// For every address in segment
		ea_t bytes = 1;
		ea_t currEA = seg->startEA;
		ea_t lastEA = seg->endEA;
		//msg("DEBUG lastEA %a\n", lastEA);
		for(; currEA < lastEA; currEA += bytes )
		{
			// Get the flags for this address
			flags_t flags = get_flags_novalue(currEA);

			// If it is unknown and looks like prologue,
			// try to make code.
			if( isUnknown(flags) == true &&
			get_byte(currEA) == 0x55 && get_byte(currEA+1) == 0x89 )
			{
				autoWait();
				autoMark( currEA, AU_CODE );
				autoWait();
				flags = get_flags_novalue(currEA);
			}

			// Skip if not head byte.
			if( isHead(flags) == false )
			{
				bytes = 1;
				continue;
			}

			func_t *pFuncChunk = get_fchunk(currEA);
			if( pFuncChunk != NULL )
			{
				// Already a function.
				// Skip to end of function.
				currEA = pFuncChunk->startEA;
				bytes = pFuncChunk->size();
				continue;
			}

			if( get_byte(currEA) == 0x55 &&
			get_byte(currEA+1) == 0x89 )
			{
				autoWait();
				ea_t nextfchunk = get_next_fchunk(currEA)->startEA;
				if( nextfchunk != BADADDR)
				{
					// Mark region till next function chunk as unknown,
					// then add as function.
					do_unknown_range(currEA, nextfchunk - currEA, DOUNK_SIMPLE);
					auto_mark_range(currEA, nextfchunk, AU_UNK);
					autoWait();
					bool res = add_func(currEA, BADADDR);
					autoWait();	
					if( res == 1 )
					{
						// Created function successfully.
						pFuncChunk = get_func(currEA);
						currEA = pFuncChunk->startEA;
						bytes = pFuncChunk->size();
						continue;
					}
				}	
			}

			bytes = 1;
		}

		analyze_area( seg->startEA, seg->endEA );
	}
}
