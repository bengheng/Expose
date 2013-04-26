#ifndef __REXMACHINE_H__
#define __REXMACHINE_H__

#include <libvex.h>
#include <vector>
#include <log4cpp/Category.hh>

using namespace std;

typedef enum{ ECX=0, EDX, ESP, EBP, REGNONE } REG;
typedef enum{ REGREAD=0, PLUS, SUBTRACT, USELOAD, USESTORE, ACTNONE } ACTIONTYPE;
typedef struct _RexTemp
{
	REG			reg;
	unsigned int		bbid;
	IRTemp			tmp;
	int			memst;
	ACTIONTYPE		action;
	int			offset;
	struct _RexTemp *	base;
} RexTemp;

class RexMach
{
public:
	RexMach( log4cpp::Category *_log );
	~RexMach();

	void EraseTemps();
	void Print();

	void Read(
		const REG r,
		const unsigned int bbid,
		const IRTemp tmp );

	void AddSubtract(
		const unsigned int nbbid,
		const IRTemp ntmp,
		const unsigned int obbid,
		const IRTemp otmp,
		const int subtractOffset );

	void AddPlus(
		const unsigned int nbbid,
		const IRTemp ntmp,
		const unsigned int obbid,
		const IRTemp otmp,
		const int plusOffset );

	void SetLoaded(
		const unsigned int nbbid,
		const IRTemp ntmp,
		const unsigned int obbid,
		const IRTemp otmp );

	void SetStored(
		const unsigned int memst,
		const unsigned int obbid,
		const IRTemp otmp );

	void Write(
		const REG r,
		const unsigned int bbid,
		IRTemp tmp );


	RexTemp* GetFirstSortedLoadRexTemp();
	RexTemp* GetNextSortedLoadRexTemp();

	RexTemp* GetFirstLoadedRexTemp();
	RexTemp* GetNextLoadedRexTemp();
	RexTemp* GetFirstStoredRexTemp();
	RexTemp* GetNextStoredRexTemp();

	void SortLoads();

	void PrintRexTemp( RexTemp *rextmp );

	void PrintLoad();
	void PrintStore();
	void Clear();
private:
	log4cpp::Category		*log;
	vector< RexTemp* >		rexTemps;
	vector< RexTemp* >		sortedLoads;
	vector< RexTemp* >::iterator	storeitr;
	vector< RexTemp* >::iterator	loaditr;
	vector< RexTemp* >::iterator	sortedloaditr;
	RexTemp*			registers[4];	// Currently ECX, EDX, ESP and EBP

	//void PrintRegHelper( RexTemp *rtmp );

	RexTemp* FindRexTemp( const unsigned int bbid, const IRTemp tmp );
	void PrintReg( REG r );


	void SetUse(
		const unsigned int nbbid,
		const IRTemp ntmp,
		const unsigned int obbid,
		const IRTemp otmp,
		const ACTIONTYPE action,
		const int memst = -1 );

};

#endif
