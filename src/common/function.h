#ifndef __FUNCTION_HEADER__
#define __FUNCTION_HEADER__

#include <log4cpp/Category.hh>
#ifdef __IDP__
#include <ida.hpp>
#include <funcs.hpp>
#include <name.hpp>
#include <map>
#include <list>
#include <utility>

#endif
#include <set>
#include <vector>

using namespace std;

// Stuff we need to define ourself
// if not compiled with IDA definitions.
#ifndef __IDP__
#include <stdint.h>
#ifdef __X64__
typedef uint64_t ea_t;	// effective address
#else
typedef uint32_t ea_t;	// effective address
#endif

#define MAXNAMELEN 512

#endif

#ifdef __IDP__
typedef struct
{
	unsigned char*	insns;
	ea_t		startEA;
	size_t		size;
	vector<ea_t>	endEAs;
} InsnChunk;

#endif

typedef struct
{
	union
	{
		char*		str;
		unsigned char*	bin;
	} insns;
	size_t		size;
	ea_t		startEA;
	ea_t		endEA;
	ea_t		nextbb1;
	ea_t		nextbb2;
} BasicBlock;

class Function
{
public:
	#ifdef __IDP__
	Function( const unsigned int file_id, func_t *const pFunc, log4cpp::Category *pLog );
	void	ParseFuncInstrs( vector< Function * > *const pVecFunctions );
	void	IncrementD(void)					{ ++D; };
	void	IncrementX(void)					{ ++X; };

	void	InsertRefStr( const ea_t value, const size_t len )
	{
		this->refStr.insert( make_pair(value, len) );
	};

	void	InsertXtrnCall( char *const calleeName )
	{
		string fname( calleeName );
		this->xtrnCall.insert( fname );
	};

	#else
	Function(
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
		log4cpp::Category	*pLog );
	#endif

	~Function();


	void	SetFuncId( const unsigned int funcId )		{ function_id = funcId;	}
	void	SetNumNgrams( const unsigned short uNgrams )	{ uNumNgrams = uNgrams; }
	void	SetLevel( const unsigned short uLvl )		{ uLevel = uLvl; }
	void	SetTaint( const unsigned short uTnt )		{ uTaint = uTnt; }
	void	SetCompoNum( const unsigned short uCmp )	{ uComponentNum = uCmp; }
	void	SetRank( const unsigned short uRnk )		{ uRank = uRnk; }
	void	AddChild( Function *const pChild );
	void	AddParent(Function *const pParent );

	char *					GetName	(void)		{ return function_name;	}
	ea_t					GetSize	(void)		{ return size;		}
	ea_t					GetStartEA(void)	{ return startEA;	}
	ea_t					GetEndEA(void)		{ return endEA;	}
	unsigned int				GetFileId(void)		{ return file_id;	}
	unsigned int				GetFuncId(void)		{ return function_id;	}
	unsigned int				GetCycComp(void)	{ return cyccomp;	}
	unsigned short				GetNumVars(void)	{ return uNumVars;	}
	unsigned short				GetNumParams(void)	{ return uNumParams;	}
	unsigned short				GetNumNgrams(void)	{ return uNumNgrams;	}
	unsigned short				GetLevel(void)		{ return uLevel;	}
	unsigned short				GetTaint(void)		{ return uTaint;	}
	unsigned short				GetCompoNum(void)	{ return uComponentNum;	}
	unsigned short				GetRank(void)		{ return uRank;		}
	set< pair<ea_t, size_t> > *		GetRefStr(void)		{ return &refStr;	}
	set< string > *				GetXtrnCall(void)	{ return &xtrnCall;	}
	#ifdef __IDP__
	func_t *				GetFuncType(void)	{ return get_func(startEA); }
	InsnChunk*				AllocInsnChunk( const size_t size );
	InsnChunk*				GetCurInsnChunk(void)	{ return curchunk;	}
	BasicBlock*				FindBB( const ea_t addr );
	BasicBlock*				CreateBB( const ea_t addr );
	void					MarkDstBB( const ea_t src, const ea_t dst );
	int					MarkBB( const ea_t addr );
	void					PrintBBs();
	void					PrintEdges();
	void					PrintChunks();

	InsnChunk*				FindHoldingChunk( const ea_t addr );
	int					GetFirstBB(BasicBlock *const bb);
	int					GetNextBB(BasicBlock *const bb);
	
	bool					HasCDECLPrologue( const ea_t ea1, const ea_t ea2 );
	#endif
	Function *				GetFirstChild(void);
	Function *				GetNextChild(void);
	Function *				GetFirstParent(void);
	Function *				GetNextParent(void);

private:
	ea_t				startEA;
	ea_t				endEA;
	ea_t				size;				// ending address - starting address.
	char				function_name[MAXNAMELEN];	// function name.
	unsigned int			function_id;			// function id.
	unsigned int			file_id;			// file id.
	unsigned int			cyccomp;			// cyclomatic complexity.
	unsigned short			uNumVars;			// number of local variables.
	unsigned short			uNumParams;			// number of parameters.
	unsigned short			uNumNgrams;			// number of ngrams.
	unsigned short			uLevel;				// level in function call graph.
	unsigned short			uComponentNum;			// component number.
	unsigned short			uTaint;				// taint to prevent loops.
	unsigned short			uRank;				// rank: 0, 1, 2
									// 0 - uninit
									// 1 - descendant
									// 2 - ancestor

	
	set< pair<ea_t, size_t> >	refStr;				// referenced strings.
	set< string >			xtrnCall;			// external calls.
	set< Function * >		setChildren;			// function's immediate children.
	set< Function * >::iterator	childItr;			// child iterator.
	set< Function * >		setParents;			// function's immediate parents.
	set< Function * >::iterator	parentItr;			// parent iterator.
	log4cpp::Category		*pLog;				// log.

	#ifdef __IDP__
	// For computing cyclomatic complexity
	unsigned int			D;				// number of decision points
	unsigned int			X;				// number of exit points

	void ParseStackFrame( func_t *const pFn );

	vector<InsnChunk*>		insnchunks;			// instruction chunks
	InsnChunk*			curchunk;			// current chunk of instructions
	map< ea_t, BasicBlock* >	basicblks;			// basic blocks

	vector< pair<ea_t, ea_t> >	edges;				// edges
	list<ea_t>			basicblocks;			// basic block start addresses
	list<ea_t>::iterator		bbitr;

	bool				has_cdecl_prologue;
	#endif
};


#endif
