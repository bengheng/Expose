#ifndef __REXTRANSLATOR_HEADER__
#define __REXTRANSLATOR_HEADER__

#ifdef __cplusplus
extern "C" {
#endif
#include <libvex.h>
#ifdef __cplusplus
}
#endif

#include <vector>
#include <rexutils.h>
#include <log4cpp/Category.hh>


using namespace std;

#define N_TRANSBUF 5000

class RexTranslator
{
public:
	RexTranslator( log4cpp::Category *_log );
	~RexTranslator();

	IRSB*			ToIR( unsigned char *insns,
					const unsigned int startEA,
					RexUtils *_rexutils );
	VexGuestExtents*	GetGuestExtents()	{ return &vge; }
	RexUtils*		GetRexUtils()		{ return rexutils; }
	IRSB*			GetBBCur()		{ return bb_cur; }
	void			SetBBCur(IRSB* bb)	{ bb_cur = bb; }
	bool			FindChased( const Addr64 dst );
	void			AddChased( const Addr64 dst );
	void			InitVex();

private:
	log4cpp::Category		*log;
	VexControl			vcon;
	VexArchInfo			vai_x86;
	VexTranslateArgs		vta;
	VexGuestExtents			vge;
	VexAbiInfo			vbi;
	UChar transbuf[N_TRANSBUF];
	Int				trans_used;
	RexUtils			*rexutils;
	vector< Addr64 >		chased;

	IRSB*				bb_cur;
};

#endif
