#ifndef __REXBB_HEADER__
#define __REXBB_HEADER__

#include <libvex.h>

#define BADADDR 0xffffffff

#define PASSLIMIT 3

class RexBB
{
public:
	RexBB( const unsigned char *const _insns,
		const size_t insnsLen,
		const unsigned int _startEA,
		const unsigned int _nextEA1,
		const unsigned int _nextEA2 );
	~RexBB();

	void		IncPass()	{ ++pass; 		}
	void		DecPass()	{ --pass;		}
	unsigned int	GetPass()	{ return pass;		}

	unsigned int	GetStartEA()	{ return startEA;	}
	unsigned char *	GetInsns()	{ return insns;		}
	unsigned int	GetInsnsLen()	{ return insnsLen;	}
	unsigned int	GetNextEA1()	{ return nextEA1;	}
	unsigned int	GetNextEA2()	{ return nextEA2;	}

private:
	unsigned int	pass;		// number of times this bb is passed
	unsigned char *	insns;		// instructions
	size_t		insnsLen;	// num of bytes for instructions
	unsigned int	startEA;	// start address of bb
	unsigned int	nextEA1;	// address to go to if branch condition is true
	unsigned int	nextEA2;	// address to go to if branch condition is false
};

#endif

