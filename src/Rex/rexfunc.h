#ifndef __REXFUNC_HEADER__
#define __REXFUNC_HEADER__

#include <libvex.h>
#include <vector>
#include <set>
#include <map>
#include <list>
#include <rexbb.h>
#include <rexio.h>
#include <rexutils.h>
#include <rexmachine.h>
#include <log4cpp/Category.hh>

using namespace std;

class RexFunc
{
public:
	RexFunc(
		log4cpp::Category *_log,
		const unsigned int _fid,
		const unsigned int _startEA );
	~RexFunc();

	
	void SetRexUtils( RexUtils *const _rexutils );
	RexUtils* GetRexUtils() { return rexutils; }

	void AddBB(
		const unsigned char *const  _insns,
		const size_t _insnsLen,
		const unsigned int _startEA,
		const unsigned int _nextEA1 = BADADDR,
		const unsigned int _nextEA2 = BADADDR );
	int FinalizeBBs();

	unsigned int		GetId()			{ return fid;				}
	unsigned int		GetStartEA()		{ return startEA;			}
	size_t			GetSize()		{ return funcSize;			}

	unsigned char*		GetInsnsOnUsePath( list<unsigned int> &work );
	size_t			GetInsnsSize()		{ return insnsSize;			}
	unsigned int		GetEAAtInsnOffset( const unsigned char *insns, const unsigned int offset );
	RexBB*			GetFirstUsePathRexBB();
	RexBB*			GetNextUsePathRexBB();
	size_t			GetNumPaths()		{ return paths.size();			}
	void			ResetUsePathItr()	{ usepathitr = usepath->begin();	}
	void			SetUsePath( const unsigned int pathid );
	void			PrintUsePath();
	bool			IsNextBBTaken( const unsigned int addrTrue, const unsigned int addrFalse );

	bool			HasUnsupportedIRSB();
	void			AddIRSB( IRSB *const irsb );
	IRSB*			GetFirstIRSB();
	IRSB*			GetNextIRSB();
	size_t			GetNumIRSBs()		{ return irsbs.size();		}
	void			PrintIRSBs();
	void			ClearIRSBs();
	void			ClearIOs();
	void			SetFirstMemSt( const unsigned int _firstMemSt )		{ firstMemSt = _firstMemSt;	}
	void			SetFirstMachSt( const unsigned int _firstMachSt )	{ firstMachSt = _firstMachSt;	}
	void			SetFinalMemSt( const unsigned int _finalMemSt )		{ finalMemSt = _finalMemSt;	}
	void			SetFinalMachSt( const unsigned int _finalMachSt )	{ finalMachSt = _finalMachSt;	}
	unsigned int		GetFirstMemSt()		{ return firstMemSt;		}
	unsigned int		GetFirstMachSt()	{ return firstMachSt;		}
	unsigned int		GetFinalMemSt()		{ return finalMemSt;		}
	unsigned int		GetFinalMachSt()	{ return finalMachSt;		}
	
	RexOut *		GetOutAuxs()		{ return outauxs;		}
	size_t			GetNumOutAuxs()		{ return outauxs->GetNumIO();	}
	void			PrintOutAuxs()		{ outauxs->Print();		}
	IO *			GetFirstOutAux()	{ return outauxs->GetFirstIO();	}
	IO *			GetNextOutAux()		{ return outauxs->GetNextIO();	}
	void			ZapOutAuxsDummies()	{ outauxs->ZapDummies();		}
	void			PermuteOutAuxs()	{ outauxs->Permute();		}

	RexOut *		GetOutArgs()		{ return outargs;		}
	size_t			GetNumOutArgs()		{ return outargs->GetNumIO();	}
	void			PrintOutArgs()		{ outargs->Print();		}
	IO *			GetFirstOutArg()	{ return outargs->GetFirstIO();	}
	IO *			GetNextOutArg()		{ return outargs->GetNextIO();	}
	void			ZapOutArgsDummies()	{ outargs->ZapDummies();		}
	void			PermuteOutArgs()	{ outargs->Permute();		}

	RexOut *		GetOutVars()		{ return outvars;		}
	size_t			GetNumOutVars()		{ return outvars->GetNumIO();	}
	void			PrintOutVars()		{ outvars->Print();		}
	IO *			GetFirstOutVar()	{ return outvars->GetFirstIO();	}
	IO *			GetNextOutVar()		{ return outvars->GetNextIO();	}
	void			ZapOutVarsDummies()	{ outvars->ZapDummies();		}
	void			PermuteOutVars()	{ outvars->Permute();		}

	RexParam *		GetArgs()		{ return inargs;			}
	size_t			GetNumArgs()		{ return inargs->GetNumIO();	}
	void			PrintArgs()		{ inargs->Print();			}
	void			ZapArgsDummies()	{ inargs->ZapDummies();		}
	void			PermuteArgs()		{ inargs->Permute();		}

	RexParam *		GetVars()		{ return invars;			}
	size_t			GetNumVars()		{ return invars->GetNumIO();	}
	void			PrintVars()		{ invars->Print();			}
	void			ZapVarsDummies()	{ invars->ZapDummies();		}
	void			PermuteVars()		{ invars->Permute();		}

	RexPathCond *		GetPathCond()		{ return pathcond;			}
	size_t			GetNumPathConds()	{ return pathcond->GetNumIO();	}
	void			PrintPathConds()	{ pathcond->Print();			}

	RexOut *		GetRetn()		{ return retn;			}
	size_t			GetNumRetn()		{ return retn->GetNumIO();	}
	void			PrintRetn()		{ retn->Print();			}

	RexMach *		GetRexMach()		{ return rexMach;		}
	void			PrintRexMach();
private:
	log4cpp::Category			*log;
	RexUtils				*rexutils;
	unsigned int				startEA;	/// start address of function
	size_t					funcSize;	/// function size
	vector<IRSB*>				irsbs;		/// IR Super Blocks
	vector<IRSB*>::iterator			irsbitr;	/// IRSB iterator
	vector<RexBB*>				bbs;		/// basic blocks
	vector< vector< RexBB* >* >		paths;		/// all paths
	unsigned char *				insns;		/// instructions
	size_t					insnsSize;	/// size of instructions
	map< unsigned int, unsigned int >	insnsOffsetToEA;/// map instruction offsets to EA
	bool					finalized;	/// set to true after adding bbs
	unsigned int				fid;		/// function id
	vector<RexBB*>*				usepath;	/// path being used
	unsigned int				usepathid;	/// id of path being used
	vector<RexBB*>::iterator		usepathitr;	/// path iterator for path being used
	vector<RexBB*>::iterator		rexbbitr;	/// also path iterator, but used for getting RexBB
	//list< regUse* >*			regUses[2];	/// Register uses, EBP and ECX for now
	//vector< list<reguse*>* >		freeRegUses;	/// Vector of reg uses to free
	//vector< pair<REG, unsigned int> >	regs[2];
	RexMach					*rexMach;

	RexOut				*retn;		/// return value
	RexOut				*outauxs;	/// outputs to memory that is not var or arg
	RexOut				*outargs;	/// outputs to arguments
	RexOut				*outvars;	/// outputs to variables
	RexParam			*inargs;	/// arguments
	RexParam			*invars;	/// local variables
	RexPathCond			*pathcond;	/// path conditions
	unsigned int			firstMemSt;	/// first memory state
	unsigned int			firstMachSt;	/// first machine state
	unsigned int			finalMemSt;	/// final memory state
	unsigned int			finalMachSt;	/// final machine state

	int MakePathHelper( vector<RexBB *> &path, const unsigned int addr );
	bool HasUnexploredChild( vector<RexBB *> &path, const unsigned int addr, int depth );

	//size_t GetIRSBLen( const IRSB *const irsb );
	RexBB* FindBB( const unsigned int addr );
	void SavePath( vector<RexBB*> &path );
	int MakePaths();

};

#endif

