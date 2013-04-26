#include <log4cpp/Category.hh>
#ifdef __IDP__
#include <ida.hpp>
#include <idp.hpp>
#include <funcs.hpp>
//#include <allins.hpp>
#include <struct.hpp>
#include <frame.hpp>
#include <auto.hpp>
#include <intel.hpp>

#include <blacklist.h>
#include <map>
#include <list>
#include <utility>
#endif
#include <function.h>
#include <set>
#include <vector>
#include <string.h>

using namespace std;

#ifdef __IDP__
#define _strncpy_	qstrncpy
#else
#define _strncpy_	strncpy
#endif

#ifdef __IDP__
void idaapi CalcSize( ea_t ea1, ea_t ea2, void *ud )
{
	ea_t *pSize = (ea_t*) ud;
	*pSize += (ea2 - ea1);
}


/*!
Extracts number of local variables and parameters passed
to the function.
Adapted from "The IDA Pro Book".
*/
void Function::ParseStackFrame( func_t *const pFn )
{
	struc_t *frame = get_frame(pFn);

	// Reset counters
	this->uNumVars = 0;
	this->uNumParams = 0;

	if( frame != NULL )
	{
		size_t ret_addr = pFn->frsize + pFn->frregs;	// offset to return address
		for( size_t m = 0; m < frame->memqty; ++m )	// loop through members
		{
			if( frame->members[m].soff < pFn->frsize )
			{	// Local variable
				++(this->uNumVars);
			}
			else if( frame->members[m].soff > ret_addr )
			{	// Parameter
				++(this->uNumParams);
			}
		}
	}
}

/*!
 * Parses the function's instructions for the following information.
 *
 * 1. Cyclomatic complexity (deferred until all chunks processed).
 *        Computes the cyclomatic complexity for the function,
 *        where M = D - X + 2
 *        M Cyclomatic complexity
 *        D Number of decision points in function.
 *        X Number of exit points in function.
 *
 *        We choose this over other methods as this can handle
 *        multiple exit points.
 *
 * 2. Referenced ASCII strings
 *
 * 3. Called (imported) functions.
 * */

typedef struct
{
	Function *pFunction;
	vector<Function *> *pVecFunctions;
	log4cpp::Category *pLog;
} FUNCPARSER, *PFUNCPARSER;

void idaapi ParseFuncInstrsHelper( ea_t ea1, ea_t ea2, void *ud )
{
	ea_t		bytes = 1;
	log4cpp::Category *pLog;
	PFUNCPARSER pFuncParser = (PFUNCPARSER) ud;
	Function *pParent = pFuncParser->pFunction;
	vector<Function *> *pVecFuncs = pFuncParser->pVecFunctions;
	pLog = pFuncParser->pLog;


	// This flag will determine if we need to do the additional
	// work to save the instructions, mark out basic blocks, etc.
	bool doSaveInsns = pParent->HasCDECLPrologue( ea1, ea2 );
	if( IsBlacklistedSegEA( ea1 ) == true )
	{
		doSaveInsns = false;
	}

	pLog->info( "DEBUG Parsing %s doSaveInsns %d", pParent->GetName(), doSaveInsns );

	InsnChunk* curchunk = NULL;
	// Alloc instruction chunks
	if( doSaveInsns )
	{
		curchunk = pParent->AllocInsnChunk( ea2 - ea1 );
		if( curchunk == NULL )
		{
			pLog->fatal( "curchunk == NULL!!" );
			assert( curchunk != NULL );
			return;
		}
		curchunk->startEA = ea1;
		curchunk->size = ea2 - ea1;

		// The beginning of a chunk is always
		// the beginning of a bb.
		pParent->MarkBB( ea1 );
	}

	// For all the instructions in the function,
	// count the number of decision points D and
	// exit points X.
	//for( ea_t addr = this->startEA; addr <= this->endEA; addr+=bytes )
	for( ea_t addr = ea1; addr < ea2; addr += bytes )
	{
		pLog->info("At insn %08x bytes %d", addr, bytes);
		// Get the flags for this address
		flags_t flags = get_flags_novalue(addr);

		// Only look at the address if it's a head byte,
		// i.e. the start of an instruction and is code.
		if( isHead(flags) == false || isCode(flags) == false )
		{
			bytes = 1;
			// Save 'bytes'-number of bytes  at addr
			if( doSaveInsns )
			{
				//curchunk->insns[ addr - ea1 ] = get_byte( addr );
				curchunk->insns[ addr - ea1 ] = 0x90;
			}
			continue;
		}

		insn_t cmdcpy;

		// Fills up cmd structure
		int retlen = decode_insn( addr );
		pLog->info("DEBUG 0: decoding insn at %08x, cmd.size = %zu sizeof(cmdcpy) %zu retlen = %d",
			addr,
			cmd.size,
			sizeof(cmdcpy),
			retlen );
		memcpy( &cmdcpy, &cmd, sizeof(cmdcpy) );

		// Set offset to next instruction
		bytes = cmdcpy.size;

		pLog->info("DEBUG 1: insn %08x bytes %zu", addr, bytes);
		if( doSaveInsns )
		{
			for(int n = 0; n < bytes; ++n )
			{
				curchunk->insns[addr-ea1+n] = get_byte(addr+n);
			}
		}

		func_t *f;
		if( cmdcpy.itype == NN_retn ||
		cmdcpy.itype == NN_retf ||
		cmdcpy.itype == NN_hlt )
		{
			pParent->IncrementX();
			if( doSaveInsns )
			{
				curchunk->endEAs.push_back( addr + bytes );
			}
		}
		else if( ( cmdcpy.itype >= NN_call &&
		           cmdcpy.itype <= NN_callni) ||
			 ( cmdcpy.itype >= NN_int &&		// interrupts
			   cmdcpy.itype <= NN_int3 ) ||
		         ( cmdcpy.Op1.addr != 0x0 &&		// A jmp that is like a function call
		           cmdcpy.itype == NN_jmp &&
			( f = get_func(cmdcpy.Op1.addr) ) != NULL &&
			cmdcpy.Op1.addr == f->startEA  ) ||
			( cmdcpy.itype == NN_cmps ) ||
			( cmdcpy.itype == NN_scas ) )
		{
			// Call instructions. If a near call is made, check if
			// it is a function in the PLT segment. This implies
			// that the called function is imported.

			if( doSaveInsns )
			{
				ea_t addr2 = addr+bytes;
				curchunk->endEAs.push_back( addr2 );

				// A function call or a "repe cmpsb"/"repe scasb" instruction
				// "separates" basic blocks.
				// This is a requirement to make VEX happy.
				if( (cmdcpy.itype >= NN_call && cmdcpy.itype <= NN_callni) ||
				( cmdcpy.itype >= NN_int && cmdcpy.itype <= NN_int3 ) ||
				cmdcpy.itype == NN_cmps ||
				cmdcpy.itype == NN_scas )
				{

					// Keep going until next valid instruction...
					while( addr2 < ea2 )
					{
						flags_t f = get_flags_novalue(addr2);
						if( isHead(f) == true && isCode(f) == true )
							break;
						++addr2;
					}

					// Mark next instruction for new basic block.
					if( addr2 != 0x0 && addr2 < ea2 )
					{

						// We demand that the next valid instruction
						// is not a NOP. This is kinda strict, but
						// there's currently no other way to handle
						// the frustrating case where the function
						// makes a call to a terminating function
						// and is not expected to return from that
						// function. This throws IDA off and cannot
						// decode the instructions after the call
						// properly.
						insn_t cmdcpy2;
						decode_insn( addr2 );
						memcpy( &cmdcpy2, &cmd, sizeof(cmdcpy2) );
						if( cmdcpy2.itype != NN_nop )
							pParent->MarkDstBB( addr, addr2 );
					}
				}
			}

			// We consider a jmp to the beginning address of a function
			// to be a function call as well.

			if( cmdcpy.Op1.type != o_near ||
			cmdcpy.Op1.addr == BADADDR ||
			cmdcpy.Op1.addr == 0x0)
				continue;

			ea_t calleeAddr = cmdcpy.Op1.addr;

			char calleeName[MAXSTR];
			get_func_name(calleeAddr, calleeName, sizeof(calleeName));
			segment_t *seg = getseg(calleeAddr);

			// Skip blacklisted function
			if( IsBlacklistedFunc(calleeName) == true ||
			IsBlacklistedSegEA(calleeAddr) == true )
			{
				continue;
			}

			if( seg->type == SEG_XTRN )
			{
				// string fname(calleeName);
				// xtrnCall.insert( fname );
				pParent->InsertXtrnCall( calleeName );
			}
			else
			{
				//
				// Add child if we can find it
				//
				vector< Function * >::iterator fnb, fne;
				for( fnb = pVecFuncs->begin(), fne = pVecFuncs->end();
				fnb != fne; ++fnb )
				{
					Function *pChild = *fnb;
					if( pChild->GetStartEA() != calleeAddr ) continue;

		
					pParent->AddChild( pChild );
					if( pParent->GetRank() != 1 )
					{
						pParent->SetRank(2);
						pChild->SetRank(1);
					}
					else if( pChild->GetRank() == 0 )
					{
						pChild->SetRank(1);
					}

					// Add current function as parent to child
					pChild->AddParent( pParent );
				}
			}
		}
		else if( cmdcpy.itype >= NN_ja &&
		cmdcpy.itype <= NN_jmpshort )
		{
			pParent->IncrementD();

			if( doSaveInsns )
			{
				ea_t addr2 = addr+bytes;
				curchunk->endEAs.push_back( addr2 );

				// Mark an edge from the current instruction
				// to the address operand of the jump instruction.
				if( cmdcpy.Op1.addr != 0x0 )
				{
					pParent->MarkDstBB( addr, cmdcpy.Op1.addr );
				}
			
				//ea_t nextbb1 = cmdcpy.Op1.addr;
				//ea_t nextbb2 = BADADDR;

				// For a conditional jump, we'll need to get the address of the
				// next instruction when the condition fails.
				if( cmdcpy.itype < NN_jmp )
				{
					// Keep going until next valid instruction...
					while( addr2 < ea2 )
					{
						flags_t f = get_flags_novalue(addr2);
						if( isHead(f) == true && isCode(f) == true )
							break;
						++addr2;
					}

					// Found next instruction for normal flow.
					if( addr2 != 0x0 && addr2 < ea2 )
					{
						pParent->MarkDstBB( addr, addr2 );
					}
				}
				
			}
		}
		else if( cmdcpy.itype >= NN_mov &&
		cmdcpy.itype <= NN_movzx )
		{	// Mov instructions. For now, extract address of referenced strings.
			// TODO: Extract immediate values too.

			if( cmdcpy.Op2.type == o_imm )
			{
				size_t strlength = get_max_ascii_length(cmdcpy.Op2.value, ASCSTR_C);
				if( strlength != 0 )
				{
					//char str[MAXSTR];
					//get_ascii_contents( cmdcpy.Op2.value, strlength, ASCSTR_C, str, sizeof(str));
					
					// Remove control characters
					//RemoveControlChars( str, sizeof(str) );
					// refStr.insert( make_pair(cmdcpy.Op2.value, strlength) );
					pParent->InsertRefStr( cmdcpy.Op2.value, strlength );
				}
			}
		}
		pLog->info("DEBUG 2: insn %08x", addr);

	}

	// DEBUG PRINT
	//pParent->PrintChunks();
	//pParent->PrintBBs();
	//pParent->PrintEdges();
}

void Function::ParseFuncInstrs( vector< Function * > *const pVecFuncs  )
{
	// iterator all function chunks, parsing the instructions
	this->D = 0;
	this->X = 0;

	FUNCPARSER funcParser;
	funcParser.pFunction = this;
	funcParser.pVecFunctions = pVecFuncs;
	funcParser.pLog = pLog;
	iterate_func_chunks(
		get_func( this->GetStartEA() ), 
		ParseFuncInstrsHelper, 
		&funcParser, 
		false );

	// Computes cyclomatic complexity
	this->cyccomp  = this->D - this->X + 2;
}

Function::Function(
	const unsigned int file_id,
	func_t *const pFn,
	log4cpp::Category *pLog )
{
	this->file_id = file_id;
	this->function_id = 0;	// Must be updated.
	this->startEA = pFn->startEA;
	this->endEA = pFn->endEA;

	// save the function name.
	get_func_name( this->startEA, function_name, sizeof(function_name) - 1 );

	// get function size
	size = 0;
	iterate_func_chunks( pFn, CalcSize, &size, false );

	// get number of local variables and parameters
	this->ParseStackFrame( pFn );

	// initialize child and parent iterator
	childItr = setChildren.begin();
	parentItr = setParents.begin();

	uNumNgrams = 0;		// clear number of ngrams
	uLevel = 0;		// initialize function call graph level
	uTaint = 0;		// clear taint
	uComponentNum = 0;	// clear component number

	// Used for marking "root". In a loop, the first
	// function found would be marked as "root".
	uRank = 0;

	this->curchunk		= NULL;
	this->pLog		= pLog;
}

InsnChunk* Function::AllocInsnChunk( const size_t size )
{
	InsnChunk* newChunk;
	newChunk = new InsnChunk();
	if( newChunk == NULL )
	{
		pLog->fatal("Cannot allocate new chunk!");
		return NULL;
	}

	newChunk->insns = new unsigned char[size];
	if( newChunk->insns == NULL )
	{
		pLog->fatal("Cannot allocate new insns!");
		delete newChunk;
		return NULL;
	}
	
	curchunk = newChunk;
	insnchunks.push_back( newChunk );
	return newChunk; //success

}

BasicBlock* Function::FindBB( const ea_t addr )
{
	map<ea_t, BasicBlock*>::iterator itr;
	itr = basicblks.find( addr );
	if( itr != basicblks.end() )
		return itr->second; // found

	return NULL; // can't find
}

BasicBlock* Function::CreateBB( const ea_t addr )
{
	BasicBlock* newbb = new BasicBlock;
	newbb->startEA = addr;
	newbb->nextbb1 = BADADDR;
	newbb->nextbb2 = BADADDR;

	basicblks[addr] = newbb;
	return newbb;
}

#define EA_IN_CHUNK( ea, chunk )	\
	( (ea >= chunk->startEA) &&		\
	(ea < (chunk->startEA + chunk->size)) )

/*!
Returns the instruction chunk that contains addr.
*/
InsnChunk* Function::FindHoldingChunk( const ea_t addr )
{
	vector<InsnChunk*>::iterator b, e;
	for( b = insnchunks.begin(), e = insnchunks.end();
	b != e; ++b )
	{
		InsnChunk *chunk = *b;

		if( EA_IN_CHUNK(addr, chunk) )
			return *b;
	}	
	return NULL;
}

/*!
Populates the BasicBlock structure pointed to by bb.
*/
int Function::GetFirstBB( BasicBlock *const bb )
{
	basicblocks.sort();
	bbitr = basicblocks.begin();

	return GetNextBB( bb );
}

int Function::GetNextBB( BasicBlock *const bb )
{
	if( bbitr == basicblocks.end() )
	{
		
		return -1;
	}

	ea_t bb_startea = *bbitr;
	InsnChunk *chunk = FindHoldingChunk( bb_startea );
	if( chunk == NULL )
	{
		pLog->fatal( "Cannot find chunk containing %08x!", bb_startea );
	
		/*
		bb->startEA = bb_startea;
		bb->endEA = bb->startEA;
		bb->nextbb1 = BADADDR;
		bb->nextbb2 = BADADDR;
		++bbitr;
		*/
		return -1;
	}
	bb->startEA = bb_startea;
	bb->nextbb1 = BADADDR;
	bb->nextbb2 = BADADDR;

	list<ea_t>::iterator nextbbitr = bbitr;
	++nextbbitr;
	/*
	bb->endEA = *nextbbitr;

	// If bb->endEA is not in the same chunk,
	// then we're at the last bb in the chunk.
	if( !EA_IN_CHUNK( bb->endEA, chunk ) )
	{
		bb->endEA = chunk->startEA + chunk->size;
	}
	*/
	bb->endEA = ( nextbbitr == basicblocks.end() ?
		chunk->startEA + chunk->size :
		*nextbbitr );

	// If we can find an endEA between the current bb->startEA
	// and bb->endEA, then we "shorten" the bb by setting
	// bb->endEA to the found endEA.
	vector<ea_t>::iterator b1, e1;
	for( b1 = chunk->endEAs.begin(), e1 = chunk->endEAs.end();
	b1 != e1; ++b1 )
	{
		// We don't include the bb's startEA in the search
		// because this bb's startEA can very easily be the
		// endEA of another bb.
		if( *b1 > bb->startEA &&
		*b1 < bb->endEA )
		{
			bb->endEA = *b1;
			break;
		}
	}

	size_t bbsize = bb->endEA - bb->startEA;

	// Make sure there's enough space.
	// Each byte corresponds to 2 characters.
	if( bb->size < (bbsize << 1) )
	{
		pLog->info("GetNextBB returning -1 because size too small %zu vs %zu",
			bb->size, (bbsize << 1));
		return -1;
	}

	// Copy instructions from chunk into bb
	// as string.
	bb->insns.str[0] = 0x0;
	for( size_t i = 0; i < bbsize; ++i )
	{
		size_t len = strlen( bb->insns.str );
		qsnprintf( &bb->insns.str[ i << 1 ], bb->size - len,
			"%02x", chunk->insns[bb->startEA - chunk->startEA + i] );
	}

	// Go through all the edges and get the ones in the bb
	vector< pair<ea_t, ea_t> >::iterator b, e;
	for( b = edges.begin(), e = edges.end(); b != e; ++b )
	{
		pair<ea_t, ea_t> edge = *b;

		// Check each endEAs
		if( edge.first >= bb->startEA && edge.first < bb->endEA )
		{
			if( bb->nextbb1 == BADADDR )
			{
				bb->nextbb1 = edge.second;
			}
			else if( bb->nextbb2 == BADADDR )
			{
				bb->nextbb2 = edge.second;
			}
			else
			{
				assert( bb->nextbb1 != BADADDR && bb->nextbb2 != BADADDR );
			}
		}
	}

	// The only situation where both nextbb1 and nextbb2 are BADADDR
	// is when we're returning from the basic block. If we can't
	// find a return, then we should "flow" to the next basic block.
	// The chunk's endEAs contains both addresses for returns and jumps.
	// But since we did not distinguish them, we'll just search all
	// endEAs for the one that lies within this bb.
	if( bb->nextbb1 == BADADDR && bb->nextbb2 == BADADDR )
	{
		vector<ea_t>::iterator b1, e1;
		for( b1 = chunk->endEAs.begin(), e1 = chunk->endEAs.end();
		b1 != e1; ++b1 )
		{
			if( *b1 > bb->startEA &&
			*b1 <= bb->endEA )
			{
				break;
			}
		}

		if( b1 == e1 )
		{
			// Didn't find a valid return. So there
			// should be a normal flow to the next bb.
			if( nextbbitr != basicblocks.end() )
			{
				bb->nextbb1 = *nextbbitr;
			}
		}
	}

	++bbitr;

	return 0;
}

/*!
Marks dst as bb and create an edge from src to dst.
*/
void Function::MarkDstBB( const ea_t src, const ea_t dst )
{

	if( MarkBB( dst ) != 0 ) return;

	// Add edge if we can't find it.
	if( find(edges.begin(), edges.end(), make_pair<ea_t, ea_t>(src, dst)) == edges.end() )
	{
		edges.push_back( make_pair<ea_t,ea_t>( src, dst ) );
	}
}

/*!
Mark addr as new bb if it doesn't exist.
*/
int Function::MarkBB( const ea_t addr )
{
	if( IsBlacklistedSegEA(addr) == true )
	{
		pLog->info( "%s: Cannot mark BB at %08x. Blacklisted.",
			function_name, addr );
		return -1;
	}

	pLog->info( "%s: Mark BB at %08x",
		function_name, addr );

	// Add bb if we can't find it.
	if( find(basicblocks.begin(), basicblocks.end(), addr) == basicblocks.end() )
	{
		basicblocks.push_back( addr );
	}

	return 0;
}

/*!
Returns true if the function has a cdecl prologue defined by
the following first two instructions of the function.

push ebp
mov ebp, esp

- OR -

push ebp
mov ecx, 0FFFFFFFFh
mov ebp, esp

- OR -

push ebp
mov eax, ????????h
mov ebp, esp
*/
bool Function::HasCDECLPrologue( const ea_t ea1, const ea_t ea2 )
{
	ea_t bytes = 1;

	// We should have already identified if the function
	// has a cdecl prologue if we're not at the first chunk.
	if( insnchunks.size() > 0 )
		return has_cdecl_prologue;


	has_cdecl_prologue = false;

	int idx = 0;
	for( ea_t addr = ea1;
	addr < ea2 && has_cdecl_prologue == false;
	addr += bytes  )
	{
		// Get the flags for this address
		flags_t flags = get_flags_novalue(addr);

		// Only look at the address if it's a head byte,
		// i.e. the start of an instruction and is code.
		if( isHead(flags) == false || isCode(flags) == false )
		{
			bytes = 1;
			continue;
		}

		// Fills up cmd structure
		decode_insn( addr );
		insn_t cmdcpy;
		memcpy( &cmdcpy, &cmd, sizeof(cmdcpy) );

		// Set offset to next instruction
		bytes = cmdcpy.size;

		switch( idx )
		{
		case 0:
			// Check for "push ebp"
			if( cmdcpy.itype != NN_push ||
			cmdcpy.Op1.type != o_reg ||
			cmdcpy.Op1.reg != R_bp )
			{
				return false;
			}
			break;
		case 1:
			// Check for "mov ebp, esp"
			// or "mov ecx, 0FFFFFFFFh"
			// or "mov eax, ?????????h"
			if( cmdcpy.itype == NN_mov &&
			cmdcpy.Op1.type == o_reg &&
			cmdcpy.Op1.reg == R_bp &&
			cmdcpy.Op2.type == o_reg &&
			cmdcpy.Op2.reg == R_sp )
			{
				has_cdecl_prologue = true;
			}
			else if( ( cmdcpy.itype != NN_mov ||
			cmdcpy.Op1.type != o_reg ||
			cmdcpy.Op1.reg != R_cx ||
			cmdcpy.Op2.type != o_imm ||
			cmdcpy.Op2.value != 0xffffffff )
			&&
			( cmdcpy.itype != NN_mov ||
			cmdcpy.Op1.type != o_reg ||
			cmdcpy.Op1.reg != R_ax ||
			cmdcpy.Op2.type != o_imm ) )
			{
				return false;
			}

			break;
		case 2:
			// Check for "mov ebp, esp"
			// or "mov ecx, 0FFFFFFFFh"
			if( cmdcpy.itype != NN_mov ||
			cmdcpy.Op1.type != o_reg ||
			cmdcpy.Op1.reg != R_bp ||
			cmdcpy.Op2.type != o_reg ||
			cmdcpy.Op2.reg != R_sp )
			{
				return false;
			}

			has_cdecl_prologue = true;

		default:
			break;
		}

		++idx;
	}

	return has_cdecl_prologue;
}

void Function::PrintChunks()
{
	pLog->info("Printing chunks:");
	vector<InsnChunk*>::iterator b, e;
	for( b = insnchunks.begin(), e = insnchunks.end();
	b != e; ++b )
	{
		pLog->info( "%08x, %zu, %08x", (*b)->startEA, (*b)->size, (*b)->startEA+(*b)->size );
	}
}

void Function::PrintBBs()
{
	pLog->info("Printing BBs:");
	list<ea_t>::iterator b2, e2;
	for( b2 = basicblocks.begin(), e2 = basicblocks.end(); b2 != e2; ++b2 )
	{
		pLog->info( "%08x", *b2 );
	}
}

void Function::PrintEdges()
{
	// DEBUG PRINT
	pLog->info("Printing Edges:");
	vector< pair<ea_t,ea_t> >::iterator b1, e1;
	for( b1 = edges.begin(), e1 = edges.end(); b1 != e1; ++b1 )
	{
		pLog->info( "%08x -> %08x", b1->first, b1->second );
	}

}

#else

Function::Function(
	const unsigned int	_file_id,
	const unsigned int	_function_id,
	const char *const	_function_name,
	const ea_t		_startEA,
	const ea_t		_endEA,
	const unsigned int	_cyccomp,
	const ea_t		_size,
	const unsigned short	_uNumVars,
	const unsigned short	_uNumParams,
	const unsigned short	_uNumNgrams,
	const unsigned short	_uLevel,
	const unsigned short	_uComponentNum,
	log4cpp::Category 	*pLog )
{
	this->file_id		= _file_id;
	this->function_id	= _function_id;
	_strncpy_( this->function_name, _function_name, sizeof(this->function_name) );
	this->startEA		= _startEA;
	this->endEA		= _endEA;
	this->cyccomp		= _cyccomp;
	this->size		= _size;
	this->uNumVars		= _uNumVars;
	this->uNumParams	= _uNumParams;
	this->uNumNgrams	= _uNumNgrams;
	this->uLevel		= _uLevel;
	this->uComponentNum	= _uComponentNum;
	this->uTaint = 0;
	this->uRank = 0;

}

#endif // __IDP__

Function::~Function()
{
	#ifdef __IDP__
	// Free instruction chunks
	while( insnchunks.size() != 0 )
	{
		InsnChunk *insnchunk = insnchunks.back();
		insnchunks.pop_back();
		delete insnchunk->insns;
		delete insnchunk;
	}

	// Free all basic blocks
	map< ea_t, BasicBlock* >::iterator b, e;
	for( b = basicblks.begin(), e = basicblks.end();
	b != e; ++b )
	{
		delete b->second;
	}
	#endif
}

// Adds pointer to parent if not already existing.
void Function::AddParent( Function *const pParent )
{
	// Sanity check
	if( pParent == NULL ) return;

	//vector< Function * >::iterator b, e;
	//for( b = vecParents.begin(), e = vecParents.end();
	//b != e; ++ b)
	//{
	//	// If child already exists, return.
	//	if( *b == pChild ) return;
	//}
	//pLog->info("DEBUG Adding parent %s for %s", pParent->GetName(), this->GetName());
	setParents.insert( pParent );

}

// Returns the first parent. Use with GetNextParent().
Function *Function::GetFirstParent(void)
{
	parentItr = setParents.begin();
	if( parentItr == setParents.end() ) return NULL;
	return *(parentItr++);
}

// Returns the next parent. Use with GetFirstParent().
Function *Function::GetNextParent(void)
{
	// Return NULL if no more parent
	if( parentItr == setParents.end() ) return NULL;
	return *(parentItr++);
}

// Adds pointer to child if not already existing.
void Function::AddChild( Function *const pChild )
{
	// Sanity check
	if( pChild == NULL ) return;

	/*
	vector< Function * >::iterator b, e;
	for( b = setChildren.begin(), e = setChildren.end();
	b != e; ++ b)
	{
		// If child already exists, return.
		if( *b == pChild ) return;
	}
	*/

	setChildren.insert( pChild );
}

// Returns the first child. Use with GetNextChild().
Function *Function::GetFirstChild(void)
{
	childItr = setChildren.begin();
	if( childItr == setChildren.end() ) return NULL;
	return *(childItr++);
}

// Returns the next child. Use with GetFirstChild().
Function *Function::GetNextChild(void)
{
	// Return NULL if no more child
	if( childItr == setChildren.end() ) return NULL;
	return *(childItr++);
}


