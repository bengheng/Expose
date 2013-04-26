#include <rexmachine.h>
#include <libvex.h>
#include <vector>
#include <log4cpp/Category.hh>

using namespace std;

RexMach::RexMach( log4cpp::Category *_log )
{
	registers[ECX] = NULL;
	registers[EDX] = NULL;
	registers[ESP] = NULL;
	registers[EBP] = NULL;
	log = _log;
}

RexMach::~RexMach()
{
	EraseTemps();
}

void RexMach::Read(
	const REG r,
	const unsigned int bbid,
	const IRTemp tmp )
{
	if( r == REGNONE ) return;

	// Return if we already have it.
	RexTemp *newtmp = FindRexTemp( bbid, tmp );
	if( newtmp != NULL) return;

	newtmp		=  new RexTemp();
	newtmp->reg	= r;
	newtmp->bbid 	= bbid;
	newtmp->tmp	= tmp;
	newtmp->memst	= -1;
	newtmp->action	= REGREAD;
	newtmp->base	= registers[r];

	//log->debug( "RexMach::Read\n" );
	//PrintRexTemp( newtmp );
	rexTemps.push_back( newtmp );
}

void RexMach::AddSubtract(
	const unsigned int nbbid,
	const IRTemp ntmp,
	const unsigned int obbid,
	const IRTemp otmp,
	const int subtractOffset )
{
	//log->debug("RexMach::AddSubtract obbid %u otmp %d\n", obbid, otmp);
	RexTemp *oldtmp = FindRexTemp( obbid, otmp );
	if( oldtmp == NULL ) return;

	RexTemp *newtmp = new RexTemp();
	newtmp->reg	= oldtmp->reg;
	newtmp->bbid 	= nbbid;
	newtmp->tmp	= ntmp;
	newtmp->memst	= -1;
	newtmp->action	= SUBTRACT;
	newtmp->base	= oldtmp;
	newtmp->offset	= subtractOffset;

	//log->debug( "RexMach::AddSubtract %08x b%ut%u\n",
	//	newtmp, newtmp->bbid, newtmp->tmp );
	//PrintRexTemp( newtmp );
	rexTemps.push_back( newtmp );
}

void RexMach::AddPlus(
	const unsigned int nbbid,
	const IRTemp ntmp,
	const unsigned int obbid,
	const IRTemp otmp,
	const int plusOffset )
{
	RexTemp *oldtmp = FindRexTemp( obbid, otmp );
	if( oldtmp == NULL ) return;

	RexTemp *newtmp = new RexTemp();
	newtmp->reg	= oldtmp->reg;
	newtmp->bbid 	= nbbid;
	newtmp->tmp	= ntmp;
	newtmp->memst	= -1;
	newtmp->action	= PLUS;
	newtmp->base	= oldtmp;
	newtmp->offset	= plusOffset;

	//log->debug( "RexMach::AddPlus %08x\n", newtmp);
	//PrintRexTemp( newtmp );
	rexTemps.push_back( newtmp );
}

void RexMach::SetStored(
	const unsigned int memst,
	const unsigned int obbid,
	const IRTemp otmp )
{
	SetUse( obbid, otmp, obbid, otmp, USESTORE, memst );
}

void RexMach::SetLoaded(
	const unsigned int nbbid,
	const IRTemp ntmp,
	const unsigned int obbid,
	const IRTemp otmp )
{
	SetUse( nbbid, ntmp, obbid, otmp, USELOAD );
}

void RexMach::SetUse(
	const unsigned int nbbid,
	const IRTemp ntmp,
	const unsigned int obbid,
	const IRTemp otmp,
	const ACTIONTYPE action,
	const int memst )
{
	RexTemp *oldtmp = FindRexTemp( obbid, otmp );
	if( oldtmp == NULL ) return;

	RexTemp *newtmp = new RexTemp();
	newtmp->reg		= oldtmp->reg;
	newtmp->bbid 		= nbbid;
	newtmp->tmp		= ntmp;
	newtmp->memst		= memst;
	newtmp->action		= action;
	newtmp->offset		= 0;
	newtmp->base		= oldtmp;

	//log->debug( "RexMach::SetLoaded\n");
	//PrintRexTemp( newtmp );
	rexTemps.push_back( newtmp );

}

void RexMach::Write(
	const REG r,
	const unsigned int bbid,
	IRTemp tmp )
{
	if( r == REGNONE ) return;

	RexTemp *rextmp = FindRexTemp( bbid, tmp );

	// Note: It is possible for rextmp to be NULL.
	registers[r] = rextmp;

	//log->debug( "RexMach::Write b%dt%u registers[%d] = %08x\n", bbid, tmp, r, rextmp);
}

RexTemp* RexMach::FindRexTemp(
	const unsigned int bbid,
	const IRTemp tmp )
{
	vector< RexTemp* >::iterator b, e;
	for( b = rexTemps.begin(), e = rexTemps.end();
	b != e; ++b )
	{
		if( (*b)->bbid == bbid &&
		(*b)->tmp == tmp )
			return *b;
	}

	return NULL;
}
void RexMach::EraseTemps()
{
	while( rexTemps.size() > 0 )
	{
		delete rexTemps.back();
		rexTemps.pop_back();
	}

	sortedLoads.clear();

	registers[EDX] = NULL;
	registers[ECX] = NULL;
	registers[ESP] = NULL;
	registers[EBP] = NULL;
}

void RexMach::PrintRexTemp( RexTemp *rtmp )
{
	if( rtmp == NULL ) return;

	char act = '?';
	switch( rtmp->action )
	{
	case REGREAD:	act = 'R'; break;
	case PLUS:	act = '+'; break;
	case SUBTRACT:	act = '-'; break;
	case USELOAD:	act = 'L'; break;
	case USESTORE:	act = 'S'; break;
	case ACTNONE:
	default:
		act = '?';
	}

	const char *regstr;
	switch( rtmp->reg )
	{
	case ECX: regstr = "ECX"; break;
	case EDX: regstr = "EDX"; break;
	case ESP: regstr = "ESP"; break;
	case EBP: regstr = "EBP"; break;
	case REGNONE:
	default: regstr = "UNK";
	}

	log->debug( "%08x\t%s\tb%ut%u\t%d\t%c\t%03x\t%08x\n",
		rtmp,
		regstr,
		rtmp->bbid,
		rtmp->tmp,
		rtmp->memst,
		act,
		rtmp->offset,
		rtmp->base );

	RexTemp *base = rtmp->base;
	if( base != NULL )
		PrintRexTemp( base );
}

void RexMach::PrintReg( REG r )
{
	vector< RexTemp* >::iterator b, e;

	for( b = rexTemps.begin(), e = rexTemps.end();
	b != e; ++b )
	{
		RexTemp *rtmp = *b;
		if( rtmp != NULL )
		{
			const char *regstr;
			switch( rtmp->reg )
			{
			case ECX: regstr = "ECX"; break;
			case EDX: regstr = "EDX"; break;
			case ESP: regstr = "ESP"; break;
			case EBP: regstr = "EBP"; break;
			case REGNONE:
			default: regstr = "UNK";
			}
			log->debug("---------- %s ----------\n", regstr);
			PrintRexTemp( rtmp );	
		}
	} 
		
}

/*
void RexMach::PrintRexTemp( RexTemp *rextmp, int lvl )
{

	for( int i = 0; i < lvl; ++i ) log->debug("\t");
	log->debug( "addr\t: %08x\n", rextmp );
	for( int i = 0; i < lvl; ++i ) log->debug("\t");
	log->debug( "reg\t: %d\n", rextmp->reg );
	for( int i = 0; i < lvl; ++i ) log->debug("\t");
	log->debug( "bt\t: b%ut%u\n", rextmp->bbid, rextmp->tmp );
	for( int i = 0; i < lvl; ++i ) log->debug("\t");
	log->debug( "acc\t: %d\n", rextmp->acc );
	for( int i = 0; i < lvl; ++i ) log->debug("\t");
	log->debug( "base\t: %08x\n", rextmp->base );
	for( int i = 0; i < lvl; ++i ) log->debug("\t");
	log->debug( "offsets\t: " );

	vector< pair<OFFSETTYPE, unsigned int> >::iterator b1, e1;
	for( b1 = rextmp->offset.begin(), e1 = rextmp->offset.end();
	b1 != e1; ++b1 )
	{
		switch( b1->first )
		{
		case PLUS:	log->debug("(+, %03x) ", b1->second ); break;
		case SUBTRACT:	log->debug("(-, %03x) ", b1->second ); break;
		case OSNONE:
		default:
			log->debug("(?, %03x) ", b1->second );
		}
	}
	log->debug("\n");

	if( rextmp->base != 0x0 )
	{
		PrintRexTemp( rextmp->base, lvl+1 );
	}
}
*/

void RexMach::PrintStore()
{
	int i = 0;
	RexTemp *tmp = GetFirstStoredRexTemp();
	while( tmp != NULL )
	{
		if( tmp->action == USESTORE )
		{
			log->debug( "+++ Store Temp %d +++\n", i );
			PrintRexTemp( tmp );
		}

		++i;
		tmp = GetNextStoredRexTemp();
	}
}

void RexMach::PrintLoad()
{
	int i = 0;
	RexTemp *tmp = GetFirstLoadedRexTemp();
	while( tmp != NULL )
	{
		if( tmp->action == USELOAD )
		{
			log->debug( "+++ Load Temp %d +++\n", i );
			PrintRexTemp( tmp );
		}

		++i;
		tmp = GetNextLoadedRexTemp();
	}
}

void RexMach::Print()
{
	log->debug("=== Printing offsets for ECX ===\n");
	PrintReg( ECX );
	log->debug("=== Printing offsets for EDX ===\n");
	PrintReg( EDX );
	log->debug("=== Printing offsets for ESP ===\n");
	PrintReg( ESP );
	log->debug("=== Printing offsets for EBP ===\n");
	PrintReg( EBP );

}

RexTemp* RexMach::GetFirstLoadedRexTemp()
{
	loaditr = rexTemps.begin();
	return GetNextLoadedRexTemp();
}

RexTemp* RexMach::GetNextLoadedRexTemp()
{
	if( loaditr == rexTemps.end() ) return NULL;

	while( loaditr != rexTemps.end() &&
	(*loaditr)->action != USELOAD )
		++loaditr;

	if( loaditr == rexTemps.end() )
		return NULL;

	return *(loaditr++);
}

// Sort loads first by base, then by offset.
void RexMach::SortLoads()
{
	sortedLoads.clear();

	// For each Load, look for entry with same
	// base. If not found, just insert at end.
	// If base is found, look for entry having
	// just smaller offset.

	RexTemp *loadtmp = GetFirstLoadedRexTemp();
	while( loadtmp != NULL )
	{
		vector< RexTemp* >::iterator b, e;
		for( b = sortedLoads.begin(), e = sortedLoads.end();
		b != e; ++b )
		{
			if( (*b)->base->base == loadtmp->base->base ) break;
		}
		
		for( ; (b != e) && ((*b)->base->base == loadtmp->base->base );
		++b )
		{
			if( (*b)->base != NULL && loadtmp->base != NULL &&
			(*b)->base->action == loadtmp->base->action &&
			(*b)->base->offset > loadtmp->base->offset )
				break;
		}

		sortedLoads.insert( b, loadtmp );	
		loadtmp = GetNextLoadedRexTemp();
	}

	/*
	log->debug( "Sorted loads are:\n" );
	vector<RexTemp*>::iterator b1, e1;
	for( b1 = sortedLoads.begin(), e1 = sortedLoads.end();
	b1 != e1; ++b1 )
	{
		PrintRexTemp( *b1, 0 );
		log->debug("-----------------\n");
	}
	*/
}

RexTemp* RexMach::GetFirstSortedLoadRexTemp()
{
	if( sortedLoads.size() == 0 )
		SortLoads();
	sortedloaditr = sortedLoads.begin();
	return GetNextSortedLoadRexTemp();
}

RexTemp* RexMach::GetNextSortedLoadRexTemp()
{
	if( sortedloaditr == sortedLoads.end() ) return NULL;
	return *(sortedloaditr++);
}

RexTemp* RexMach::GetFirstStoredRexTemp()
{
	storeitr = rexTemps.begin();
	return GetNextStoredRexTemp();
}

RexTemp* RexMach::GetNextStoredRexTemp()
{
	if( storeitr == rexTemps.end() ) return NULL;

	while( storeitr != rexTemps.end() &&
	(*storeitr)->action != USESTORE )
		++storeitr;

	if( storeitr == rexTemps.end() )
		return NULL;

	return *(storeitr++);
}

