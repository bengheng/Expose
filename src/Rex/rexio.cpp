#include <rexio.h>
#include <libvex.h>
#include <assert.h>
#include <list>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <log4cpp/Category.hh>

using namespace std;

RexIO::RexIO( log4cpp::Category *_log )
{
	log = _log;
}

RexIO::~RexIO()
{
	Clear();
}

void RexIO::Clear()
{
	size_t numio;
	IO *io;

	numio = iolist.size();
	while( numio > 0 )
	{
		io = iolist.back();
		delete io;
		iolist.pop_back();
		--numio;
	}

	// Free permutations
	while( permutations.begin() != permutations.end() )
	{
		delete permutations.back();
		permutations.pop_back();
	}

}

/*!
Adds io to iolist. doFree specifies if io should be freed by
the destructor.
*/
void RexIO::AddIO( IO* io )
{
	assert( io != NULL);
	iolist.push_back(io);
}

/*!
Transfers specified "io" from this class to the specified rexio.
*/
void RexIO::TransferIO( IO* in, RexIO &dst )
{
	list<IO*>::iterator b, e;

	// Find the io and do the transfer
	for( b = iolist.begin(), e = iolist.end();
	b != e; ++b )
	{
		if( *b == in )
		{
			dst.AddIO( *b  );
			iolist.erase(b);
			break;
		}
	}
}


/*!
Gets the first IO.
*/
IO* RexIO::GetFirstIO()
{
	ioitr = iolist.begin();
	return GetNextIO();
}

/*!
Gets the next IO.
*/
IO* RexIO::GetNextIO()
{
	if( ioitr == iolist.end() ) return NULL;
	return *(ioitr++);
}

/*!
Pads IO with dummies to specified size.
*/
void RexIO::PadDummies( const unsigned int toSize )
{
	IO* io;

	while( iolist.size() < toSize )
	{
		io = new IO;
		assert(io != NULL);

		io->tag = DUMMY;
		iolist.push_back(io);
	}
}

/*!
Remove all dummies.
*/
void RexIO::ZapDummies()
{
	list<IO*>::iterator itr;

	itr = iolist.begin();
	while( itr != iolist.end() )
	{
		if( (*itr)->tag == DUMMY )
		{
			delete (*itr);
			itr = iolist.erase(itr);	
		}
		else
			++itr;
	}
}

bool RexIO::IsSameIO( IO *io1, IO *io2 )
{
	if( io1->tag != io2->tag )
		return false;

	switch( io1->tag )
	{
	case DUMMY:
		return true;
	case PARAM:
		return (io1->io.param.ebpOffset == io2->io.param.ebpOffset);
	case IN:
		return (io1->io.in.fid == io2->io.in.fid &&
			io1->io.in.bid == io2->io.in.bid &&
			io1->io.in.tmp == io2->io.in.tmp );
	case PUT:
		return (io1->io.put.offset == io2->io.put.offset);
	case ST_TMP:
		return (io1->io.store_tmp.curMemSt == io2->io.store_tmp.curMemSt &&
			io1->io.store_tmp.fid == io2->io.store_tmp.fid &&
			io1->io.store_tmp.bid == io2->io.store_tmp.bid &&
			io1->io.store_tmp.tmp == io2->io.store_tmp.tmp);
	case ST_CONST:
		return (io1->io.store_const.curMemSt == io2->io.store_const.curMemSt &&
			io1->io.store_const.con == io2->io.store_const.con &&
			io1->io.store_const.arr_rest == io2->io.store_const.arr_rest );
	case RETN:
		return (io1->io.retn.fid == io2->io.retn.fid &&
			io1->io.retn.bid == io2->io.retn.bid &&
			io1->io.retn.tmp == io2->io.retn.tmp );
	case PATHCOND:
			return (io1->io.pathcond.fid == io2->io.pathcond.fid &&
			io1->io.pathcond.bid == io2->io.pathcond.bid &&
			io1->io.pathcond.tmp == io2->io.pathcond.tmp );
	default:
		log->fatal("FATAL %s %d: Unrecognized io tag %d!\n", __FILE__, __LINE__, io1->tag );
		exit(1);
	}

	assert( false ); // should never reach here!
	return false;
}

/*!
Returns true if the current IO in list1 matches an IO
in list2, and subsequent IOs in list1 matches some IO
in list2.
*/
bool RexIO::IsSubsetHelper(
	list<IO*> *list1,
	list<IO*>::iterator itr1,
	list<IO*> *list2 )
{
	if( itr1 == list1->end() )
		return true;

	bool found = false;
	list<IO*>::iterator b, e;
	for( b = list2->begin(), e = list2->end();
	b != e; ++b )
	{
		if( IsSameIO( *itr1, *b ) == true )
		{
			found = true;
			break;
		}
	}

	return found && IsSubsetHelper( list1, ++itr1, list2 );
}

bool RexIO::IsSubset( list<IO*> *list1, list<IO*> *list2 )
{
	assert( list1 != NULL );
	assert( list2 != NULL );
	if( list1->size() > list2->size() ) return false;

	return IsSubsetHelper( list1, list1->begin(), list2 );
}

/*!
Remove lists containing elements that are subsets of other lists.
*/
void RexIO::RemoveSubsetPmtns()
{
	vector< list<IO*>* >::iterator b1, b2;

	b1 = permutations.begin();
	while( b1 != permutations.end() )
	{
		b2 = b1;
		++b2;
		if( b2 == permutations.end() ) break;
		
		if( IsSubset(*b1, *b2) )
		{
			delete *b1;
			b1 = permutations.erase( b1 );
			continue;
		}
	
		b1++;
	}
}

/*!
Prints IO's. Mostly for debugging.
*/
void RexIO::Print()
{
	PrintIOList( &iolist );
}

void RexIO::PrintIOList( list<IO*> *lst )
{
	list<IO*>::iterator b, e;

	for( b = lst->begin(), e = lst->end();
	b != e; ++b )
	{
		IO* io = (*b);
		switch( io->tag )
		{
		case PARAM:
			log->debug("PARAM(%d); ",	
				io->io.param.ebpOffset);
			break;
		case IN:
			log->debug("IN(%u,%u,%u); ",
				io->io.in.fid,
				io->io.in.bid,
				io->io.in.tmp);
			break;
		case PUT:
			log->debug("PUT(%03x); ", io->io.put.offset);
			break;
		case ST_TMP:
			log->debug("ST_TMP(m%u,f%ub%ut%u); ",
				io->io.store_tmp.curMemSt,
				io->io.store_tmp.fid,
				io->io.store_tmp.bid,
				io->io.store_tmp.tmp);
			break;
		case ST_CONST:
			log->debug("ST_CONST(%u,%u); ",
				io->io.store_const.con,
				io->io.store_const.arr_rest );
			break;
		case RETN:
			log->debug("RETN(%u,%u,%u); ",
				io->io.retn.fid,
				io->io.retn.bid,
				io->io.retn.tmp);
			break;
		case PATHCOND:
			log->debug("PC(%u,%u,%u,%c); ",
				io->io.pathcond.fid,
				io->io.pathcond.bid,
				io->io.pathcond.tmp,
				io->io.pathcond.taken?'T':'F');
			break;
		case DUMMY:
			log->debug("DUMMY; ");
			break;
		default:
			log->fatal("FATAL %s %d: UNKNOWN OUT TAG %d!\n", __FILE__, __LINE__, io->tag );
			exit(1);
		}

	}
	log->debug("\n");
}

void RexIO::PrintPermutation()
{
	vector< list<IO*>* >::iterator b, e;
	for( b = permutations.begin(), e = permutations.end();
	b != e; ++b )
	{
		PrintIOList( *b );
	}
}

/*!
Returns the IO by the specified index.
*/
IO* RexIO::GetIOByIdx( const unsigned int idx )
{
	list< IO* >::iterator b, e;
	unsigned int n;

	for( b = iolist.begin(), e = iolist.end(), n = 0;
	b != e && n < idx; ++b, ++n );

	return ( b == e ? NULL : *b );

}

/*!
Returns the last IO.
*/
IO* RexIO::GetLastIO()
{
	list< IO* >::iterator itr;
	itr = iolist.end();
	return *(--itr);
}

/*!
Returns the index for the last pair of io's in this class
and lst such that both are not DUMMYs.
*/
int RexIO::FindLastNonDummy( list<IO*> *lst )
{
	list< IO* >::iterator b1, e1, b2, e2;
	unsigned int n;

	assert( iolist.size() == lst->size() );

	n = iolist.size();

	b1 = iolist.begin();
	e1 = iolist.end(),
	b2 = lst->begin();
	e2 = lst->end();

	do
	{
		--e1;
		--e2;
		--n;

		if( (*e1)->tag != DUMMY && (*e2)->tag != DUMMY )
			return n;

	} while( b1 != e1 && b2 != e2 );

	return -1;
}

void RexIO::Permute()
{
	if( iolist.size() == 0 ) return;

	assert( permutations.size() == 0 );
	PermuteHelper( permutations, iolist.size(), --iolist.end() );

	// Remove subset permutations
	//RemoveSubsetPmtns();
}

/*!
Recursive helper function to permutation iolist.
*/
void RexIO::PermuteHelper(
	vector< list<IO*>* > &pmtns,
	const size_t pmtnSize,
	list<IO*>::iterator weaveitr )
{
	list<IO*>* newSubPmtn;
	list<IO*>* subPmtn;
	IO* weaveio;


	// Base case.
	if( pmtnSize == 1 )
	{
		newSubPmtn = new list<IO*>();
		newSubPmtn->push_back( GetIOByIdx(0) );
		pmtns.push_back( newSubPmtn );
		return;
	}

	// IO to be "weaved" into sub-permutation.
	weaveio = *weaveitr;

	// Perform permutation of sub-elements.
	vector< list<IO*>* > subPmtns;
	PermuteHelper( subPmtns, pmtnSize - 1, --weaveitr );

	bool rightToLeft = true;

	vector< list<IO*>* >::iterator b1, e1;
	// For each sub-permutation...
	for( b1 = subPmtns.begin(), e1 = subPmtns.end();
	b1 != e1; ++b1 )
	{
		subPmtn = *b1;

		for( int j = 0; j < pmtnSize; ++j )
		{
			newSubPmtn = new list<IO*>();

			int pushPos = rightToLeft ? pmtnSize - 1 - j : j;

			// For each element in sub-permutation,
			// push element into new permutation unless
			// element is at same position as j.
			list<IO*>::iterator ele;
			ele = subPmtn->begin();
			int k = 0;
			while( k < pmtnSize )
			{
				if( pushPos == k )
				{
					newSubPmtn->push_back( weaveio );
				}
				else
				{
					assert( *ele != NULL );
					newSubPmtn->push_back( *ele );
					++ele;
				}
				++k;
			}

			// Add new permutation
			pmtns.push_back( newSubPmtn );
		}

		// Switch direction
		rightToLeft != rightToLeft;
	}

	// Free sub-permutations
	while( subPmtns.begin() != subPmtns.end() )
	{
		subPmtn = subPmtns.back();
		delete subPmtn;
		subPmtns.pop_back();
	}
}

list<IO*>* RexIO::GetFirstPermutation()
{
	pmtnitr = permutations.begin();
	return this->GetNextPermutation();
}

list<IO*>* RexIO::GetNextPermutation()
{

	if( pmtnitr == permutations.end() ) return NULL;
	return *(pmtnitr++);
}

//====================================================================
// RexParam
//====================================================================
/*!
Finds the IO for a parameter.
*/
IO* RexParam::FindParam( const int ebpOffset )
{
	list<IO *>::iterator b, e;

	for( b = iolist.begin(), e = iolist.end(); b != e; ++b )
	{
		IO* param = *b;
		if( param->tag != PARAM ) continue;

		if( param->io.param.ebpOffset == ebpOffset )
			return param;
	}

	return NULL;
}

void RexParam::AddParamOrdered( const int ebpOffset )
{
	IO* param;
	list<IO*>::iterator b, e;

	b = iolist.begin(), e = iolist.end();
	while( b != e )
	{
		param = *b;

		// No duplicates
		if( ebpOffset == param->io.param.ebpOffset )
			return;

		if( ebpOffset < param->io.param.ebpOffset )
			break;

		++b;
	}

	param = new IO;
	assert(param != NULL);
	param->tag			= PARAM;
	param->io.param.ebpOffset	= ebpOffset;
	iolist.insert( b, param );
}

/*!
Adds an IO defined by EBP offset.
*/
void RexParam::AddParam( const int ebpOffset )
{
	IO* param;

	param = FindParam( ebpOffset );
	if( param == NULL )
	{
		param = new IO;
		assert(param != NULL);

		param->tag			= PARAM;
		param->io.param.ebpOffset	= ebpOffset;

		iolist.push_back( param );
	}
}

//====================================================================
// RexPathCond
//====================================================================
/*!
Finds the IO for a path condition.
*/
IO* RexPathCond::FindPC(
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp tmp,
	const bool taken )
{
	list<IO *>::iterator b, e;

	for( b = iolist.begin(), e = iolist.end(); b != e; ++b )
	{
		IO* pc = *b;
		if( pc->tag != PATHCOND ) continue;

		if( pc->io.pathcond.fid == fid &&
		pc->io.pathcond.bid == bid &&
		pc->io.pathcond.tmp == tmp &&
		pc->io.pathcond.taken == taken )
			return pc;
	}

	return NULL;
}

/*!
Adds an IO defined by a path condition.
*/
void RexPathCond::AddPC(
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp tmp,
	const bool taken )
{
	IO* pc;

	pc = FindPC( fid, bid, tmp, taken );
	if( pc == NULL )
	{
		pc = new IO;
		assert(pc != NULL);

		pc->tag = PATHCOND;
		pc->io.pathcond.fid	= fid;
		pc->io.pathcond.bid	= bid;
		pc->io.pathcond.tmp	= tmp;
		pc->io.pathcond.taken	= taken;
		iolist.push_back( pc );
	}
}


//====================================================================
// RexIn
//====================================================================
/*!
Finds the IO for a load or get.
*/
IO* RexIn::FindIn(
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp tmp )
{
	list<IO *>::iterator b, e;

	for( b = iolist.begin(), e = iolist.end(); b != e; ++b )
	{
		IO* in = *b;
		if( in->tag != IN ) continue;

		if( in->io.in.fid == fid &&
		in->io.in.bid == bid &&
		in->io.in.tmp == tmp )
			return in;
	}

	return NULL;
}

/*!
Adds an IO defined by a load or get.
*/
void RexIn::AddIn(
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp tmp )
{
	IO* in;

	in = FindIn( fid, bid, tmp );
	if( in == NULL )
	{
		in = new IO;
		assert(in != NULL);

		in->tag = IN;
		in->io.in.fid = fid;
		in->io.in.bid = bid;
		in->io.in.tmp = tmp;

		iolist.push_back( in );
	}
}

//====================================================================
// RexOut
//====================================================================

/*!
Finds the IO for a put.
*/
IO* RexOut::FindPutOffset( const int putos )
{
	list<IO *>::iterator b, e;

	for( b = iolist.begin(), e = iolist.end(); b != e; ++b )
	{
		IO* out = *b;
		if( out->tag != PUT ) continue;

		if( out->io.put.offset == putos )
			return out;
	}

	return NULL;
}

/*!
Adds an IO defined by a put.
*/
void RexOut::AddOut_PutOffset( const int putos )
{
	IO* out;

	out = FindPutOffset( putos );
	if( out == NULL )
	{
		out = new IO;
		assert(out != NULL);

		out->tag = PUT;
		out->io.put.offset = putos;

		iolist.push_back( out );
	}
}

/*!
Finds the IO for a store to the tmp.
*/
IO* RexOut::FindStoreTmp(
	const unsigned int curMemSt,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp tmp )
{
	list<IO *>::iterator b, e;

	for( b = iolist.begin(), e = iolist.end(); b != e; ++b )
	{
		IO* out = *b;
		if( out->tag != ST_TMP ) continue;

		if( out->io.store_tmp.curMemSt == curMemSt &&
		out->io.store_tmp.fid == fid &&
		out->io.store_tmp.bid == bid &&
		out->io.store_tmp.tmp == tmp )
		//if( out->io.store_tmp.ebpoffset == ebpoffset )
			return out;
	}

	return NULL;
}

/*!
Finds the IO for a store to the constant.
*/
IO* RexOut::FindStoreConst( const unsigned int curMemSt, const unsigned int c )
{
	list<IO *>::iterator b, e;

	for( b = iolist.begin(), e = iolist.end(); b != e; ++b )
	{
		IO* out = *b;
		if( out->tag != ST_CONST ) continue;

		if( out->io.store_const.curMemSt == curMemSt &&
		out->io.store_const.con == c )
			return out;
	}

	return NULL;
}

/*!
Finds the IO for retn.
*/
IO* RexOut::FindRetn( const unsigned int fid, const unsigned int bid, const IRTemp tmp )
{
	list<IO *>::iterator b, e;

	for( b = iolist.begin(), e = iolist.end(); b != e; ++b )
	{
		IO* out = *b;
		if( out->tag != RETN ) continue;

		if( out->io.retn.fid == fid &&
		out->io.retn.bid == bid &&
		out->io.retn.tmp == tmp )
			return out;
	}

	return NULL;
}


/*!
Adds an IO defined by a store to a tmp.
*/
void RexOut::AddOut_StoreTmp(
	const unsigned int curMemSt,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp tmp )
{
	IO* out = NULL;

	if( ( out = FindStoreTmp( curMemSt, fid, bid, tmp ) ) == NULL )
	{
		out = new IO;
		assert(out != NULL);

		out->tag			= ST_TMP;
		iolist.push_back(out);
	}
	
	out->io.store_tmp.curMemSt	= curMemSt;
	out->io.store_tmp.fid		= fid;
	out->io.store_tmp.bid		= bid;
	out->io.store_tmp.tmp		= tmp;
}

/*!
Adds an output defined by a store to a constant.
*/
void RexOut::AddOut_StoreConst(
	const unsigned int curMemSt,
	const unsigned int c,
	const unsigned int arr_rest )
{
	IO* out;

	out = FindStoreConst( curMemSt, c );
	if( out == NULL )
	{
		out = new IO;
		assert(out != NULL);

		out->tag			= ST_CONST;
		out->io.store_const.curMemSt	= curMemSt;
		out->io.store_const.con		= c;
		out->io.store_const.arr_rest	= arr_rest;
		iolist.push_back( out );
	}
	else
	{
		out->io.store_const.arr_rest = arr_rest;
	}
}

/*!
Adds an output defined by retn.
*/
void RexOut::AddOut_Retn(
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp tmp )
{
	IO* out;

	out = FindRetn( fid, bid, tmp );
	if( out == NULL )
	{
		out = new IO;
		assert(out != NULL);

		out->tag		= RETN;
		out->io.retn.fid	= fid;
		out->io.retn.bid	= bid;
		out->io.retn.tmp	= tmp;
		iolist.push_back( out );
	}
}


