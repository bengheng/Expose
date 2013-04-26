#ifdef __cplusplus
extern "C" {
#endif

#include <libvex.h>

#ifdef __cplusplus
}
#endif

#include <rextranslator.h>
#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <log4cpp/Category.hh>

#include <execinfo.h>
#include <signal.h>

using namespace std;


__attribute__((noreturn))
static void failure_exit(void)
{
	void *array[10];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	// print out all the frames to stderr
	fprintf(stderr, "Error: \n");
	backtrace_symbols_fd(array, size, 2);

	fprintf(stderr, "RexTranslator did failure exit. X_X\n");
	exit(1);
}

static void log_bytes( HChar *bytes, Int nbytes )
{
	fwrite( bytes, 1, nbytes, stdout );
}

static Bool chase_into_not_ok( void *opaque, Addr64 dst )
{
	return False;
}

int count1 = 0;
int count2 = 0;
static Bool chase_into_ok( void *opaque, Addr64 dst )
{
	return false;
}

static void *
dispatch (void)
{
	return NULL;
}

IRSB* Instrument(
    void *closureV,
    IRSB *bb_in,
    VexGuestLayout *layout,
    VexGuestExtents *vge,
    IRType gWordTy, IRType hWordTy)
{

	RexTranslator *rt = (RexTranslator*)closureV;

	assert(bb_in);
	rt->SetBBCur( rt->GetRexUtils()->RexDeepCopyIRSB(bb_in) );
	// printf("Instrument\tDEBUG0 bb_cur->stmts_used %d bb_cur->stmts_size %d\n",
	//	rt->GetBBCur()->stmts_used,
	//	rt->GetBBCur()->stmts_size );
	return bb_in;
}

RexTranslator::RexTranslator( log4cpp::Category *_log )
{
	log = _log;
}

RexTranslator::~RexTranslator()
{
}

void RexTranslator::InitVex()
{
	//
	// Initialize VexControl
	//
	vcon.iropt_verbosity		= 0;		// no output
	vcon.iropt_level		= 2;		// max optimisation
	vcon.iropt_precise_memory_exns	= False;	// only guest's stack ptr up to date
	// at potential mem exception points
	vcon.iropt_unroll_thresh	= 0;		// n/a
	vcon.guest_max_insns		= 80;		// max basic block length
	vcon.guest_chase_thresh	= 0;	// no chasing
	//vcon.guest_chase_thresh		= vcon.guest_max_insns - 1;
	//vcon.guest_chase_cond		= True;
	vcon.guest_chase_cond		= False;

	//
	// Initialize VEX
	//
	LibVEX_Init( &failure_exit,	// failure exit function
	             &log_bytes,	// logging output function
	             0,			// debug paranoia level
	             False,		// not supporting valgrind checking
	             &vcon );		// control

	//
	// Initialize VexArchInfo
	//
	LibVEX_default_VexArchInfo( &vai_x86 );
	vai_x86.hwcaps = 0;

	//
	// Set up args for LibVEX_Translate
	//
	/* IN: The instruction sets we are translating from and to.
	And guest/host misc info. */

	vta.arch_guest		= VexArchX86;
	vta.archinfo_guest	= vai_x86;
#ifdef AMD64
	vta.arch_host		= VexArchAMD64;
#else
	vta.arch_host		= VexArchX86;
#endif
	vta.archinfo_host	= vai_x86;
	vta.abiinfo_both	= vbi;

	// IN: an opaque value which is passed as the first arg to all
	// callback functions supplied in this struct.  Vex has no idea
	// what's at the other end of this pointer.
	vta.callback_opaque	= this;

	// IN: the block to translate, and its guest address.
	// where are the actual bytes in the host's address space?
	vta.guest_bytes		= NULL;

	// where do the bytes really come from in the guest's aspace?
	// This is the post-redirection guest address.  Not that Vex
	// understands anything about redirection; that is all done on
	// the Valgrind side.
	vta.guest_bytes_addr	= 0;

	// Is it OK to chase into this guest address?  May not be
	// NULL.
	vta.chase_into_ok	= /*chase_into_not_ok*/ chase_into_ok;

	// OUT: which bits of guest code actually got translated
	vta.guest_extents	= &vge;

	// IN: a place to put the resulting code, and its size
#ifdef AMD64
	vta.host_bytes		= NULL;
	vta.host_bytes_size	= 0;
#else	
	vta.host_bytes		= transbuf;
	vta.host_bytes_size	= N_TRANSBUF;
#endif
	// OUT: how much of the output area is used.
#ifdef AMD64
	vta.host_bytes_used	= NULL;
#else
	vta.host_bytes_used	= &trans_used;
#endif
	// IN: optionally, two instrumentation functions.  May be
	// NULL.
	vta.instrument1		= Instrument;
	vta.instrument2		= NULL;
	vta.finaltidy		= NULL;

	// IN: should this translation be self-checking?  default: False
	vta.do_self_check	= False;

	// IN: optionally, a callback which allows the caller to add its
	// own IR preamble following the self-check and any other
	// VEX-generated preamble, if any.  May be NULL.  If non-NULL,
	// the IRSB under construction is handed to this function, which
	// presumably adds IR statements to it.  The callback may
	// optionally complete the block and direct bb_to_IR not to
	// disassemble any instructions into it; this is indicated by
	// the callback returning True.
	vta.preamble_function	= NULL;

	// IN: debug: trace vex activity at various points
	//vta.traceflags		= (1 << 5) | (1 << 2);
	vta.traceflags		= 0;

	// IN: address of the dispatcher entry point.  Describes the
	// place where generated code should jump to at the end of each
	// bb.
	//
	// At the end of each translation, the next guest address is
	// placed in the host's standard return register (x86: %eax,
	// amd64: %rax, ppc32: %r3, ppc64: %r3).  Optionally, the guest
	// state pointer register (on host x86: %ebp; amd64: %rbp;
	// ppc32/64: r31) may be set to a VEX_TRC_ value to indicate any
	// special action required before the next block is run.
	//
	// Control is then passed back to the dispatcher (beyond Vex's
	// control; caller supplies this) in the following way:
	//
	// - On host archs which lack a link register (x86, amd64), by a
	// jump to the host address specified in 'dispatcher', which
	// must be non-NULL.
	//
	// - On host archs which have a link register (ppc32, ppc64), by
	// a branch to the link register (which is guaranteed to be
	// unchanged from whatever it was at entry to the
	// translation).  'dispatch' must be NULL.
	//
	// The aim is to get back and forth between translations and the
	// dispatcher without creating memory traffic to store return
	// addresses.
	vta.dispatch		= (void*) dispatch /*0x12345678*/;
}

void RexTranslator::AddChased( const Addr64 dst )
{
	chased.push_back(dst);
}

bool RexTranslator::FindChased( const Addr64 dst )
{
	for( int n = 0; n < chased.size(); ++n )
	{
		if( chased[n] == dst ) return true;
	}

	return false;
}

IRSB* RexTranslator::ToIR(
	unsigned char	*insns,
	const unsigned int	startEA,
	RexUtils *_rexutils )
{
	VexTranslateResult tres;

	vta.guest_bytes		= (UChar *) insns;
	vta.guest_bytes_addr	= startEA;

	rexutils = _rexutils;
	bb_cur = NULL;
	tres = LibVEX_Translate(&vta);
	if ( tres != VexTransOK )
		printf("\ntres = %d\n", (Int) tres );

	return bb_cur;
}

