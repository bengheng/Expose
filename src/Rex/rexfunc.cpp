#include <assert.h>
#include <string.h>
#include <rexfunc.h>
#include <rexbb.h>
#include <rexutils.h>
#include <rex.h>
#include <libvex_ir.h>
#include <stdio.h>
#include <vector>
#include <map>
#include <list>
#include <time.h>
#include <log4cpp/Category.hh>

RexFunc::RexFunc( log4cpp::Category *_log,
	const unsigned int _fid,
	const unsigned int _startEA )
{
	fid		= _fid;
	startEA		= _startEA;
	funcSize	= 0;
	insns		= NULL;
	finalized	= false;
	usepath		= NULL;
	rexutils	= NULL;
	log		= _log;

	retn		= new RexOut( _log );
	outauxs		= new RexOut( _log );
	outargs		= new RexOut( _log );
	outvars		= new RexOut( _log );
	inargs		= new RexParam( _log );
	invars		= new RexParam( _log );
	pathcond	= new RexPathCond( _log );
	rexMach		= new RexMach( _log );
}

RexFunc::~RexFunc()
{
	size_t npaths;
	size_t nbbs;
	vector<RexBB*>* path;
	RexBB* bb;

	// Free all paths
	npaths = paths.size();
	while( npaths > 0 )
	{
		path = paths.back();
		paths.pop_back();
		delete path;
		--npaths;
	}

	// Free all bbs
	nbbs = bbs.size();
	while( nbbs > 0 )
	{
		bb = bbs.back();
		bbs.pop_back();
		delete bb;
		--nbbs;
	}

	// Free instructions
	if( insns != NULL ) delete [] insns;

	if( retn != NULL ) delete retn;
	if( outauxs != NULL ) delete outauxs;
	if( outargs != NULL ) delete outargs;
	if( outvars != NULL ) delete outvars;
	if( inargs != NULL ) delete inargs;
	if( invars != NULL ) delete invars;
	if( pathcond != NULL ) delete pathcond;
	if( rexMach != NULL ) delete rexMach;
}

void RexFunc::SetRexUtils( RexUtils *const _rexutils )
{
	assert( rexutils == NULL || rexutils == _rexutils );
	if( rexutils == NULL )
		rexutils = _rexutils;
}

void RexFunc::AddBB(
	const unsigned char *const  _insns,
	const size_t _insnsLen,
	const unsigned int _startEA,
	const unsigned int _nextEA1,
	const unsigned int _nextEA2 )
{
	RexBB *rexbb;

	assert( finalized == false );
	assert( _insns != NULL );
	assert( _insnsLen != 0 );

	// DEBUG
	//if( GetId() == 556 )
	//{
	//	log->debug( "DEBUG Func %d: Adding BB %08x, len %zu, next1 = %08x, next2 = %08x\n",
	//		GetId(), _insnsLen, _startEA, _nextEA1, _nextEA2 );
	//}
	// GUBED

	rexbb = new RexBB( _insns, _insnsLen, _startEA, _nextEA1, _nextEA2 );
	assert( rexbb != NULL );

	bbs.push_back(rexbb);

	funcSize += _insnsLen;
}

/*!
BBs cannot be added once this function is called.
*/
int RexFunc::FinalizeBBs()
{
	if( MakePaths() != 0 )
		return -1;

	// Allocate space for instructions
	//assert( insns == NULL );
	//insns = new unsigned char[funcSize];
	//assert( insns != NULL );

	finalized = true;

	return 0;
}

/*!
Returns instructions on usepath.
*/
unsigned char* RexFunc::GetInsnsOnUsePath( list<unsigned int> &work )
{
	vector<RexBB*>::iterator b, e;
	size_t num_bytes;

	assert( finalized == true );
	assert( usepath != NULL );
	assert( usepathitr == usepath->begin() );
	
	if( finalized == false )
	{
		log->warn("Basic blocks not finalized. Call FinalizeBBs() first.\n");
		return NULL;
	}

	if( insns != NULL )
	{
		delete [] insns;
		insnsOffsetToEA.clear();
	}


	// Get the total size needed
	num_bytes = 0;
	for( b = usepath->begin(), e = usepath->end(); b != e; ++b )
	{
		RexBB * rexbb = *b;
		num_bytes += rexbb->GetInsnsLen();
	}

	insnsSize = num_bytes;
	num_bytes += 8; // Add 8 0x0 bytes at end of insn block to
			// force VEX to fail and get Ijk_NoDecode,
			// so that it will not overflow to the next
			// function.
	
	insns = new unsigned char[num_bytes];
	assert( insns != NULL );

	// Set all bytes to no-ops
	//for( int i = 0; i < num_bytes; ++i )
	//	insns[i] = 0x90;

	// Copy instructions for each bb on this path into insns.
	num_bytes = 0; // re-use
	for( b = usepath->begin(), e = usepath->end(); b != e; ++b )
	{
		RexBB * rexbb = *b;
		memcpy( &insns[num_bytes],
			rexbb->GetInsns(),
			rexbb->GetInsnsLen() );
		insnsOffsetToEA[num_bytes] = rexbb->GetStartEA();
		//log->info( "Mapping %d bytes at insn offset %d to %08x\n",
		//	rexbb->GetInsnsLen(), num_bytes, rexbb->GetStartEA() );
		work.push_back( num_bytes );
		num_bytes += rexbb->GetInsnsLen();
	}

	memset( &insns[num_bytes], 0xff, 8 );

	// DEBUG
	//if( GetId() == 556 )
	//{
	//	log->debug( "Printing insns for func %d\n", GetId() );
	//	for( size_t i = 0; i < num_bytes; ++i )
	//	{
	//		log->debug( "%02x ", insns[i] );
	//	}
	//	log->debug( "\n" );
	//}
	// GUBED

	return insns;
}

unsigned int RexFunc::GetEAAtInsnOffset(
	const unsigned char* insns,
	const unsigned int offset )
{
	assert( insns != NULL );

	map< unsigned int, unsigned int >::iterator b, e;
	b = insnsOffsetToEA.begin();
	e = insnsOffsetToEA.end();
	do
	{
		--e;

		if( e->first <= offset )
			break;
	} while( b != e );

	//log->debug( "Getting EA at offset: %08x + (%d - %d) = %08x\n",
	//	b->second, offset, b->first,
	//	(b->second + (offset - b->first) ));
	return e->first == offset ? e->second : e->second + (offset - e->first);
}

RexBB* RexFunc::GetFirstUsePathRexBB()
{
	rexbbitr = usepath->begin();
	return GetNextUsePathRexBB();
}

RexBB* RexFunc::GetNextUsePathRexBB()
{
	if( rexbbitr == usepath->end() ) return NULL;
	return *(rexbbitr++);
}

/*!
Duplicates the path and saves it.
*/
void RexFunc::SavePath( vector< RexBB* > &path )
{
	vector<RexBB*>::iterator b, e;
	vector<RexBB*> *newpath;

	newpath = new vector<RexBB*>();
	assert( newpath != NULL );

	// Copies all bbs into new path.
	for( b = path.begin(), e = path.end();
	b != e; ++b )
	{
		newpath->push_back( *b );
	}

	paths.push_back( newpath );
}


/*!
Finds basic block based on addr.
*/
RexBB * RexFunc::FindBB( const unsigned int addr )
{
	RexBB* rexbb;

	vector< RexBB* >::iterator b, e;
	for( b = bbs.begin(), e = bbs.end(); b != e; ++b )
	{
		rexbb = *b;
		if( rexbb->GetStartEA() == addr ) return rexbb;
	}

	return NULL;
}

bool RexFunc::HasUnexploredChild( vector<RexBB *> &path, const unsigned int addr, int depth )
{
	RexBB* rexbb;
	unsigned int child1;
	unsigned int child2;

	if( addr == BADADDR ) return false;

	// Weird loops do occur, we can't go on
	// forever, so stop when we've reached
	// a "depth" of 50.
	if( depth > 50 ) return false;

	rexbb = NULL;
	child1 = BADADDR;
	child2 = BADADDR;

	// Get next basic block.
	rexbb = FindBB( addr );
	assert( rexbb != NULL );

	// Found unexplored child.
	if( rexbb->GetPass() == 0 ) return true;

	child1 = rexbb->GetNextEA1();
	child2 = rexbb->GetNextEA2();

	// Randomly select child to test.
	if( rand() % 2 )
	{
		if( HasUnexploredChild( path, child1, depth+1 ) ||
		HasUnexploredChild( path, child2, depth+1 ) )
			return true;
	}
	else
	{
		if( HasUnexploredChild( path, child2, depth+1 ) ||
		HasUnexploredChild( path, child1, depth+1 ) )
			return true;
	}

	return false;
}

int RexFunc::MakePathHelper( vector<RexBB *> &path, const unsigned int addr )
{
	RexBB* rexbb;
	unsigned int nextEA1;
	unsigned int nextEA2;
	unsigned int firstEA;
	unsigned int secondEA;

	if( addr == BADADDR ) return 0;

	rexbb = NULL;
	nextEA1 = BADADDR;
	nextEA2 = BADADDR;

	// Get next basic block.
	rexbb = FindBB( addr );
	if( rexbb == NULL )
	{
		log->debug("Cannot find bb at %08x\n", addr);
		return 0;
	}

	// If pass exceeds limit, return.
	if( rexbb->GetPass() > PASSLIMIT )
	{
		//log->debug("bb %08x Exceed PASSLIMIT %d\n", rexbb->GetStartEA(), PASSLIMIT );
		return 0;
	}

	// Push bb into path and increment pass.
	rexbb->IncPass();
	path.push_back(rexbb);

	nextEA1 = rexbb->GetNextEA1();
	nextEA2 = rexbb->GetNextEA2();

	
	//log->debug( "bb %08x nextEA1 %08x nextEA2 %08x\n", rexbb->GetStartEA(), nextEA1, nextEA2 );
	// If nextEA1 AND nextEA2 are BADADDR, this path is completed.
	// Copy the path and return.
	if( nextEA1 == BADADDR && nextEA2 == BADADDR )
	{
		SavePath( path );
		path.pop_back();
		rexbb->DecPass();
		return 0;
	}

	// Go to first nextEA if there's unexplored child.
	// We give priority to the child with 0 pass.
	// TODO: Can we just go to child1, then child2?
	RexBB *nextBB = FindBB(nextEA1);
	if( nextBB == NULL ) return -1;
	if( nextEA1 != BADADDR &&
	nextBB->GetPass() == 0 )
	{
		firstEA = nextEA1;
		secondEA = nextEA2;
	}
	else
	{
		firstEA = nextEA2;
		secondEA = nextEA1;
	}

	if( firstEA != BADADDR && HasUnexploredChild( path, firstEA, 0 ) )
	{
		//log->debug("1 Exploring %08x\n", firstEA);
		if( MakePathHelper( path, firstEA ) != 0 )
		{
			path.pop_back();
			rexbb->DecPass();
			return -1;
		}
	}
	if( firstEA != BADADDR && HasUnexploredChild( path, secondEA, 0 ) )
	{
		//log->debug("2 Exploring %08x\n", firstEA);
		if( MakePathHelper( path, secondEA ) )
		{
			path.pop_back();
			rexbb->DecPass();
			return -1;
		}
	}	

	path.pop_back();
	rexbb->DecPass();

	return 0;
}

int RexFunc::MakePaths()
{
	vector< RexBB* > path;

	if( finalized == true ) return 0;

	return MakePathHelper( path, startEA );

	// DEBUG
	//size_t npaths = paths.size();
	//for( size_t n = 0; n < npaths; ++n )
	//{
	//	log->debug("func%u Path %d: ", GetId(), n);

	//	vector< RexBB* > *p = paths[n];
	//	vector<RexBB*>::iterator b, e;
	//	for( b = p->begin(), e = p->end(); b != e; ++b )
	//	{
	//		log->debug("%08x, ", (*b)->GetStartEA() );
	//	}
	//	log->debug("\n");
	//}
	// GUBED
}

void RexFunc::PrintUsePath()
{
	vector<RexBB*>::iterator b, e;

	for( b = usepath->begin(), e = usepath->end();
	b != e; ++b )
	{
		if( b != usepath->begin() )
			log->debug( ", " );
		log->debug( "%08x", (*b)->GetStartEA() );
	}
	log->debug("\n");
}

/*!
Sets the id of the path to use.
*/
void RexFunc::SetUsePath( const unsigned int pathid )
{
	assert( pathid < paths.size() );

	usepath		= paths[pathid];
	usepathid	= pathid;
	usepathitr	= usepath->begin();
}


bool RexFunc::IsNextBBTaken(
	const unsigned int addrTrue,
	const unsigned int addrFalse )
{
	bool bbtaken = false;

	//log->debug( "Checking next bb taken: %08x, %08x\n", addrTrue, addrFalse );

	assert( usepath != NULL );
	assert( usepathitr != usepath->end() );

	
	for(; usepathitr != usepath->end(); ++usepathitr )
	{
		//log->debug( "Trying %08x\n", (*usepathitr)->GetStartEA() );
		if( (*usepathitr)->GetStartEA() == addrTrue )
		{
			//log->debug( "\tMatch TRUE\n" );
			bbtaken = true;
			break;
		}
		else if( (*usepathitr)->GetStartEA() == addrFalse )
		{
			//log->debug( "\tMatch FALSE\n ");
			bbtaken = false;
			break;
		}
	}

	if( usepathitr == usepath->end() )
	{
		PrintIRSBs();
		log->debug("At func %d, usepathitr == usepath->end()\n", GetId() );
		assert( usepathitr != usepath->end() );
	}
	++usepathitr;

	return bbtaken;
}

void RexFunc::PrintIRSBs()
{
	vector<IRSB*>::iterator b, e;

	assert( rexutils != NULL );
	if( rexutils == NULL )
		return;
	
	for( b = irsbs.begin(), e = irsbs.end();
	b != e; ++b )
	{
		rexutils->RexPPIRSB( *b );
	}
}

void RexFunc::AddIRSB( IRSB *const irsb )
{
	irsbs.push_back( irsb );
}

IRSB *RexFunc::GetFirstIRSB()
{
	irsbitr = irsbs.begin();
	return GetNextIRSB();
}

IRSB *RexFunc::GetNextIRSB()
{
	IRSB *irsb;
	if( irsbitr == irsbs.end() ) return NULL;
	return *(irsbitr++);
}

/*!
If the last IRSB is a jump or if there's a function call
anywhere, we don't support it.
*/
bool RexFunc::HasUnsupportedIRSB()
{
	IRSB *irsb = GetFirstIRSB();
	IRJumpKind lastjump;	
	while( irsb != NULL )
	{
		lastjump = irsb->jumpkind;

		if( lastjump == Ijk_Call )
			return true;

		irsb = GetNextIRSB();
	}

	if( irsbs.size() > 0 &&
	lastjump == Ijk_Ret )
		return true;

	return false;

}

void RexFunc::ClearIRSBs()
{
	irsbs.clear();
}

void RexFunc::ClearIOs()
{
	outauxs->Clear();
	outargs->Clear();
	outvars->Clear();
	inargs->Clear();
	invars->Clear();
	pathcond->Clear();
	retn->Clear();
	rexMach->EraseTemps();

}


/*!
Adds a GET from register.
*/
/*
void RexSTP::AddRegGet( IRStmt *stmt )
{
	// Checks stmt is a get for a register

	// If there was a previous PUT to
	// the register, 
	// If there is a previous get 
	regGetStmts.push_back( stmt );
}
*/
/*!
Adds a PLUS statement.
*/
/*
void RexSTP::AddRegPlus( IRStmt *stmt )
{
}
*/
/*
void RexFunc::AddRegPut( REG reg, unsigned int offset, REG baseReg )
{
	vector< pair<REG, unsigned int> > newReg;

	if( reg == NONE )
		return;

	// Just clear the register offsets
	if( offset == 0xffffffff )
	{
		regs[ reg ].clear();
		return;
	}

	// Copy the offsets from baseReg
	if( baseReg != NONE )
	{
		vector< pair<REG, unsigned int> >::iterator b1, e1;
		for( b1 = regs[baseReg].begin(), e1 = regs[baseReg].end();
		b1 != e1; ++b1 )
		{
			newReg.push_back( *b1 );
		}
	}

	newReg.push_back( make_pair<REG, unsigned int>(reg, offset) );

	// Assign new offsets
	regs[reg] = newReg;
}
*/

void RexFunc::PrintRexMach()
{
	rexMach->Print();
}
