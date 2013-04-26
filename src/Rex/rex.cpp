#include <libvex.h>
#include <libvex_ir.h>
#include <rexutils.h>
#include <rexstp.h>
#include <rex.h>
#include <rexfunc.h>
#include <rexio.h>
#include <rexmachine.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fstream>
#include <vector>
#include <iomanip>
#include <c_interface.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <errno.h>
#include <log4cpp/Category.hh>
#include <math.h>
#include <signal.h>

using namespace std;

Rex::Rex( log4cpp::Category *_log,
	RexUtils *_rexutils1,
	RexUtils *_rexutils2,
	RexTranslator *_rextrans )
{
	log		= _log;
	rexutils1	= _rexutils1;
	rexutils2	= _rexutils2;
	rexstp		= new RexSTP( _log, _rexutils1, _rextrans );
	queryTimeoutMicrosec	= 0;
}

Rex::~Rex()
{
	if( rexstp != NULL )
		delete rexstp;
}

/*!
Emits the path conditions and returns the number of path conditions.
*/
void Rex::EmitPathCond(
	VC &vc,
	#ifdef USE_STP_FILE
	fstream &queryfs,
	#endif
	RexFunc &func )
{
	IO *pc;
	RexPathCond *pathcond;

	pathcond = func.GetPathCond();
	pc = pathcond->GetFirstIO();
	while( pc != NULL )
	{
		if( pc->io.pathcond.taken == true )
		{
			#ifdef USE_STP_FILE
			queryfs << "ASSERT( "
				<< "f" << pc->io.pathcond.fid
				<< "b" << pc->io.pathcond.bid
				<< "t" << pc->io.pathcond.tmp
				<< " <=> TRUE );\n"; 
			#endif
			vc_assertFormula( vc, vc_iffExpr( vc,
				vc_lookupVarByName( "f%ub%ut%u",
					pc->io.pathcond.fid,
					pc->io.pathcond.bid,
					pc->io.pathcond.tmp ),
				vc_trueExpr( vc ) ) );
		}
		else
		{
			#ifdef USE_STP_FILE
			queryfs << "ASSERT( "
				<< "f" << pc->io.pathcond.fid
				<< "b" << pc->io.pathcond.bid
				<< "t" << pc->io.pathcond.tmp
				<< " <=> FALSE );\n"; 
			#endif
			vc_assertFormula( vc, vc_iffExpr( vc,
				vc_lookupVarByName( "f%ub%ut%u",
					pc->io.pathcond.fid,
					pc->io.pathcond.bid,
					pc->io.pathcond.tmp ),
				vc_falseExpr( vc ) ) );
		}

		pc = pathcond->GetNextIO();
	}
}

/*!
Returns an Expr for the return value. We only
expect at most 1 return value per function.
*/
Expr Rex::GetRetnExpr(
	VC &vc,
	#ifdef USE_STP_FILE
	char query[512],
	#endif
	RexFunc &func1,
	RexFunc &func2 )
{
	if( func1.GetNumRetn() != 1 ||
	func2.GetNumRetn() != 1 )
		return NULL;

	IO *out1 = func1.GetRetn()->GetFirstIO();
	IO *out2 = func2.GetRetn()->GetFirstIO();

	#ifdef USE_STP_FILE
	if( strlen( query ) > 0 )
		strncat( query, " AND ", 512 );

	char retns[64];
	snprintf( retns, 64, "(f%ub%ut%u = f%ub%ut%u)",
		out1->io.retn.fid,
		out1->io.retn.bid,
		out1->io.retn.tmp,
		out2->io.retn.fid,
		out2->io.retn.bid,
		out2->io.retn.tmp);

	strncat( query, retns, 512 );
	#endif

	return vc_eqExpr( vc,
		vc_lookupVarByName( "f%ub%ut%u",
			out1->io.retn.fid,
			out1->io.retn.bid,
			out1->io.retn.tmp ),
		vc_lookupVarByName( "f%ub%ut%u",
			out2->io.retn.fid,
			out2->io.retn.bid,
			out2->io.retn.tmp ) );
}

Expr Rex::GetOutputExpr(
	VC &vc,
	#ifdef USE_STP_FILE
	char query[512],
	fstream &fs,
	#endif
	const unsigned int memSt,
	const unsigned int machSt,
	IO *const out )
{
	Expr			e_var;
	Expr			e_retvar;
	Expr			e_retvaridx;
	Expr			e_tmp;
	#ifdef USE_STP_FILE
	char			s[128];
	ios_base::fmtflags	ff;
	#endif

	assert( out != NULL );
	assert( out->tag != DUMMY );
	assert( out->tag != IN );

	#ifdef USE_STP_FILE
	s[0] = 0x0;
	#endif

	switch( out->tag )
	{
	case PUT:
		e_var = vc_lookupVarByName( "MachSt%u", machSt );
		e_retvaridx = vc_bvConstExprFromInt( vc, 12, out->io.put.offset );
		e_retvar = vc_readExpr( vc, e_var, e_retvaridx );
		#ifdef USE_STP_FILE
		snprintf( s, 128, "MachSt%u[0hex%03x]", machSt, out->io.put.offset);
		#endif
		break;

	case ST_TMP:
		e_retvaridx = vc_lookupVarByName( "f%ub%ut%u",
				out->io.store_tmp.fid,
				out->io.store_tmp.bid,
				out->io.store_tmp.tmp );
		e_var = vc_lookupVarByName( "MemSt%u", out->io.store_tmp.curMemSt );
		e_retvar = vc_readExpr( vc, e_var, e_retvaridx );

		#ifdef USE_STP_FILE
		snprintf( s, 128, "MemSt%u[f%ub%ut%u]",
			out->io.store_tmp.curMemSt,
			out->io.store_tmp.fid,
			out->io.store_tmp.bid,
			out->io.store_tmp.tmp );
		#endif

		/*
		e_retvaridx = vc_lookupVarByName( "ARRAYREST%x", out->io.store_tmp.arr_rest );
		e_tmp = vc_lookupVarByName( "f%ub%ut%u",
				out->io.store_tmp.fid,
				out->io.store_tmp.bid,
				out->io.store_tmp.tmp );
		vc_assertFormula( vc, vc_eqExpr( vc, e_retvaridx, e_tmp) );
		
		e_var = vc_lookupVarByName( "MemSt%u", out->io.store_tmp.curMemSt );
		e_retvar = vc_readExpr( vc, e_var, e_retvaridx );

		#ifdef USE_STP_FILE
		snprintf( s, 128, "MemSt%u[ARRAYREST%x]",
			out->io.store_tmp.curMemSt,
			out->io.store_tmp.arr_rest );
		ff = fs.flags();
		fs << "ASSERT( ARRAYREST"
			<< hex << out->io.store_tmp.arr_rest
			<< " = "
			<< dec << "f" << out->io.store_tmp.fid
			<< "b" << out->io.store_tmp.bid
			<< "t" << out->io.store_tmp.tmp << " );\n" ;
		fs.setf(ff);
		#endif
		*/
		break;

	case ST_CONST:
		e_retvaridx = vc_bv32ConstExprFromInt( vc, out->io.store_const.con );
		e_var = vc_lookupVarByName( "MemSt%u", out->io.store_const.curMemSt );
		e_retvar = vc_readExpr( vc, e_var, e_retvaridx );
	
		#ifdef USE_STP_FILE
		snprintf( s, 128, "MemSt%u[0hex%08x]",
			out->io.store_const.curMemSt,
			out->io.store_const.con );
		#endif
		/*
		e_retvaridx = vc_lookupVarByName( "ARRAYREST%x", out->io.store_const.arr_rest );
		e_tmp = vc_bv32ConstExprFromInt( vc, out->io.store_const.con );
		vc_assertFormula( vc, vc_eqExpr( vc, e_retvaridx, e_tmp) );
		
		e_var = vc_lookupVarByName( "MemSt%u", out->io.store_const.curMemSt );
		e_retvar = vc_readExpr( vc, e_var, e_retvaridx );

		#ifdef USE_STP_FILE
		snprintf( s, 128, "MemSt%u[ARRAYREST%x]",
			out->io.store_const.curMemSt,
			out->io.store_const.arr_rest );
		ff = fs.flags();
		fs << "ASSERT( ARRAYREST"
			<< hex << out->io.store_const.arr_rest
			<< " = 0hex"
			<< setw(8) << setfill('0') << hex << out->io.store_const.con << " );\n";
		fs.setf(ff);
		#endif
		*/
		break;
	case RETN:
		e_retvar = vc_lookupVarByName( "f%ub%ut%u",
			out->io.retn.fid,
			out->io.retn.bid,
			out->io.retn.tmp );
		#ifdef USE_STP_FILE
		snprintf( s, 128, "f%ub%ut%u",
			out->io.retn.fid,
			out->io.retn.bid,
			out->io.retn.tmp );
		#endif
		break;
	default:
		log->fatal("FATAL %s %d: Unknown tag when emitting output!\n",
			__FILE__, __LINE__ );
		return NULL;
	}

	#ifdef USE_STP_FILE
	strncat( query, s, 512 );
	#endif
	return e_retvar;
}

void Rex::EmitParams(
	VC &vc,
	#ifdef USE_STP_FILE
	fstream &queryfs,
	#endif
	RexFunc &func1,
	RexFunc &func2,
	list<IO*> *params1,
	list<IO*> *params2 )
{
	list<IO*>::iterator	b1, e1;
	list<IO*>::iterator	b2, e2;
	IO*			p1;
	IO*			p2;
	Expr			e_memSt1;
	Expr			e_machSt1;
	Expr			e_memSt2;
	Expr			e_machSt2;
	Expr			e_esp;
	Expr			e_lhs;
	Expr			e_rhs;
	Expr			e_ebpOffset;
	ios_base::fmtflags	ff;

	e_memSt1 = vc_lookupVarByName( "MemSt%u", func1.GetFirstMemSt() );
	e_machSt1 = vc_lookupVarByName( "MachSt%u", func1.GetFirstMachSt() );
	e_memSt2 = vc_lookupVarByName( "MemSt%u", func2.GetFirstMemSt() );
	e_machSt2 = vc_lookupVarByName( "MachSt%u", func2.GetFirstMachSt() );
	e_esp = vc_bvConstExprFromInt( vc, 12, 0x10 );

	// For each func1 input and corresponding func2 input...
	for( b1 = params1->begin(), e1 = params1->end(),
	b2 = params2->begin(), e2 = params2->end(); 
	b1 != e1 && b2 != e2;
	++b1, ++b2 )
	{
		p1 = *b1;
		p2 = *b2;

		if( p1->tag == DUMMY ) continue;
		if( p2->tag == DUMMY ) continue;

		assert( p1->tag == PARAM );
		assert( p2->tag == PARAM );

		e_ebpOffset = vc_bv32ConstExprFromInt( vc, p1->io.param.ebpOffset );
		e_lhs = vc_readExpr( vc, e_memSt1,
				vc_bv32PlusExpr( vc,
					vc_bv32MinusExpr( vc,
						vc_readExpr( vc, e_machSt1, e_esp ),
						vc_bv32ConstExprFromInt( vc, 0x00000004 ) ),
					e_ebpOffset ) ); 

		e_ebpOffset = vc_bv32ConstExprFromInt( vc, p2->io.param.ebpOffset );
		e_rhs = vc_readExpr( vc, e_memSt2,
				vc_bv32PlusExpr( vc,
					vc_bv32MinusExpr( vc,
						vc_readExpr( vc, e_machSt2, e_esp ),
						vc_bv32ConstExprFromInt( vc, 0x00000004 ) ),
					e_ebpOffset ) ); 

		vc_assertFormula( vc, vc_eqExpr( vc, e_lhs, e_rhs ) );

		/*
		vc_assertFormula( vc, vc_eqExpr( vc,
			vc_lookupVarByName( "f%ub%ut%u",
				p1->io.in.fid,
				p1->io.in.bid,
				p1->io.in.tmp ),
			vc_lookupVarByName( "f%ub%ut%u",
				p2->io.in.fid,
				p2->io.in.bid,
				p2->io.in.tmp ) ) );
		*/
		#ifdef USE_STP_FILE
		ff = queryfs.flags();
		queryfs << "ASSERT( "
			<< "MemSt" << func1.GetFirstMemSt() << "[ "
			<< "BVPLUS(32, BVSUB(32, MachSt" << func1.GetFirstMachSt() << "[0hex010], "
			<< "0hex00000004), 0hex" << setw(8) << setfill('0') << hex << p1->io.param.ebpOffset << ") ] = " << dec
			<< "MemSt" << func2.GetFirstMemSt() << "[ "
			<< "BVPLUS(32, BVSUB(32, MachSt" << func2.GetFirstMachSt() << "[0hex010], "
			<< "0hex00000004), 0hex" << setw(8) << setfill('0') << hex << p2->io.param.ebpOffset << ") ] );\n" << dec;

		queryfs.setf(ff);

		/*
		queryfs << "ASSERT( "
			<< "f" << p1->io.in.fid
			<< "b" << p1->io.in.bid
			<< "t" << p1->io.in.tmp
			<< " = " 
			<< "f" << p2->io.in.fid
			<< "b" << p2->io.in.bid
			<< "t" << p2->io.in.tmp
			<< " );\n";
		*/
		#endif
	}

}

/*!
Returns true if outputs are valid when conjugated.
*/
bool Rex::IsValidConjOutputs (
	list<IO*> *func1_outs,
	list<IO*> *func2_outs )
{
	IO*			fn1_out;
	IO*			fn2_out;
	list<IO*>::iterator	b1, e1;
	list<IO*>::iterator	b2, e2;

	// For each func1 output and corresponding func2 output...
	for( b1 = func1_outs->begin(), e1 = func1_outs->end(),
	b2 = func2_outs->begin(), e2 = func2_outs->end(); 
	b1 != e1 && b2 != e2;
	++b1, ++b2 )
	{
		fn1_out = *b1;
		fn2_out = *b2;

		// Try to narrow down the search space
		// by enforcing strict matches
		if( ( fn1_out->tag == PUT && fn2_out->tag != PUT ) ||
		( fn1_out->tag != PUT && fn2_out->tag == PUT ) ||
		( fn1_out->tag == RETN && fn2_out->tag != RETN ) ||
		( fn1_out->tag != RETN && fn2_out->tag == RETN ) )
			return false;
	}

	return true;
}


/*!
Emit conjugated outputs.
*/
unsigned int Rex::EmitConjOutputs (
	VC &vc,
	#ifdef USE_STP_FILE
	fstream &queryfs,
	char query[512],
	#endif
	RexFunc &func1,
	RexFunc &func2,
	list<IO*> *func1_outs,
	list<IO*> *func2_outs,
	Expr *e_queries,
	const unsigned int e_queries_offset  )
{
	IO*			fn1_out;
	IO*			fn2_out;
	list<IO*>::iterator	b1, e1;
	list<IO*>::iterator	b2, e2;
	Expr			e_lhs;
	Expr			e_rhs;
	unsigned int		num_queries;

	// For each func1 output and corresponding func2 output...
	num_queries = 0;
	for( b1 = func1_outs->begin(), e1 = func1_outs->end(),
	b2 = func2_outs->begin(), e2 = func2_outs->end(); 
	b1 != e1 && b2 != e2;
	++b1, ++b2 )
	{
		fn1_out = *b1;
		fn2_out = *b2;

		if( fn1_out->tag == DUMMY ) continue;
		if( fn2_out->tag == DUMMY ) continue;

		// Try to narrow down the search space
		// by enforcing strict matches
		if( ( fn1_out->tag == PUT && fn2_out->tag != PUT ) ||
		( fn1_out->tag != PUT && fn2_out->tag == PUT ) ||
		( fn1_out->tag == RETN && fn2_out->tag != RETN ) ||
		( fn1_out->tag != RETN && fn2_out->tag == RETN ) )
			continue;

		#ifdef USE_STP_FILE
		if( strlen(query) != 0 )
			strncat( query, "\nAND ", 512 );
		#endif
		#ifdef USE_STP_FILE
		strncat( query, "(", 512 );
		#endif

		e_lhs = GetOutputExpr(
			vc,
			#ifdef USE_STP_FILE
			query,
			queryfs,
			#endif
			func1.GetFinalMemSt(),
			func1.GetFinalMachSt(),
			fn1_out );
		if( e_lhs == NULL ) return 0;

		#ifdef USE_STP_FILE
		strncat( query, " = ", 512 );
		#endif

		e_rhs = GetOutputExpr(
			vc,
			#ifdef USE_STP_FILE
			query,
			queryfs,
			#endif
			func2.GetFinalMemSt(),
			func2.GetFinalMachSt(),
			fn2_out );
		if( e_rhs == NULL ) return 0;

		#ifdef USE_STP_FILE
		strncat( query, ")", 512 );
		#endif

		e_queries[e_queries_offset + num_queries++] = vc_eqExpr( vc, e_lhs, e_rhs );
	}

	return num_queries;
}

void child( PQUERYCONTEXT ctx )
{
	int query_res;
	#ifdef USE_STP_FILE	
	char			callbuf[256];
	FILE			*f;
	char			resbuf[1024];

	// Calls ../stp/bin/stp <queryfname>
	snprintf( callbuf, sizeof(callbuf), "%s/libmatcher/src/stp/bin/stp %s",
		getenv("HOME"), ctx->queryfname);
	f = popen(callbuf, "r");
	if( f == 0 )
	{
		fprintf( stderr, "Could not execute \"%s\"!\n", callbuf);
		ctx->retval = REX_SE_ERR;
		pthread_exit( NULL );
	}
	resbuf[0] = 0x0;
	fgets(resbuf, 1024, f);
	pclose(f);
	#endif

	query_res = ctx->num_queries == 1 ?
		vc_query( *(ctx->vc), ctx->e_queries[0] ) :
		vc_query( *(ctx->vc), vc_andExprN( *(ctx->vc), ctx->e_queries, ctx->num_queries ) );
		
	switch( query_res )
	{
	case 0: // INVALID
		#ifdef USE_STP_FILE
		if( strcmp("Invalid.\n", resbuf) != 0 )
		{
			vc_printVarDecls( *(ctx->vc) );
			vc_printAsserts( *(ctx->vc) );
			vc_printQuery( *(ctx->vc) );
			vc_printCounterExample( *(ctx->vc) );
			//ctx->log->debug("%s: libstp says invalid, "
			//	"but ../stp/bin/stp says \"%s\"!\n",
			//	ctx->queryfname, resbuf );
			ctx->retval = REX_SE_MISMATCH;
			break;
		}
		#endif
		ctx->retval = REX_SE_FALSE;
		break;

	case 1: // VALID
		#ifdef USE_STP_FILE
		if( strcmp("Valid.\n", resbuf) != 0 )
		{
			vc_printVarDecls( *(ctx->vc) );
			vc_printAsserts( *(ctx->vc) );
			vc_printQuery( *(ctx->vc) );
			vc_printCounterExample( *(ctx->vc) );
			//ctx->log->debug("%s: libstp says valid, "
			//	"but ../stp/bin/stp says \"%s\"!\n",
			//	ctx->queryfname, resbuf );
			ctx->retval = REX_SE_MISMATCH;
			break;
		}
		//ctx->log->debug("%s ", ctx->queryfname);
		#endif

		//ctx->log->debug( "VALID\n" );
		ctx->retval = REX_SE_TRUE;
		break;

	case 2: // ERROR
		//ctx->log->debug( "\tERROR\n" );
		ctx->retval = REX_SE_ERR;
		break;
	default:
		//ctx->log->fatal( "FATAL %s %d: Unknown query result!\n",
		//	__FILE__, __LINE__ );
		ctx->retval = REX_SE_ERR;
	}
}

// The thread that calls vc_query.
// We use a thread so that we can timeout.
void* thread( void* context )
{
	PQUERYCONTEXT		ctx = (PQUERYCONTEXT) context;
	int			query_res;

	//pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, NULL );
	//ctx->log->debug("Inside thread\n");

	if( ctx->num_queries == 0 )
	{
		pthread_exit( NULL );
	}

	child( ctx );

	// Signals to the calling thread
	// that we're done.
	pthread_mutex_lock( ctx->mutex );
	pthread_cond_signal( ctx->cond );
	pthread_mutex_unlock( ctx->mutex );
	pthread_exit( NULL );
}

pid_t Rex::DoFork( PQUERYCONTEXT ctx )
{
	int p = fork ();
 
	if (p == -1) {
		ctx->retval = REX_ERR_UNK;
		exit (REX_ERR_UNK);
	}
 
	if (p == 0) {
		child( ctx );
		exit( ctx->retval );
	}
 
	return p;
	
}

void Rex::ForkQuery( PQUERYCONTEXT ctx )
{
	sigset_t mask;
	sigset_t orig_mask;
	struct timespec timeout;
	pid_t pid;
 
	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);
 
	if( sigprocmask(SIG_BLOCK, &mask, &orig_mask) < 0 )
	{
		ctx->retval = REX_ERR_UNK;
		return;
	}
 
	pid = DoFork( ctx );
 
	timeout.tv_sec = 5;
	timeout.tv_nsec = 0;
 
	do
	{
		if(sigtimedwait(&mask, NULL, &timeout) < 0)
		{
			if (errno == EINTR)
			{
				// Interrupted by a signal other than SIGCHLD.
				continue;
			}
			else if (errno == EAGAIN)
			{
				//printf ("Timeout, killing child\n");
				kill (pid, SIGKILL);
			}
			else
			{
				//perror ("sigtimedwait");
				ctx->retval = REX_ERR_UNK;
				return;
			}
		}
 
		break;
	} while (1);
 
	if (waitpid(pid, NULL, 0) < 0) {
		ctx->retval = REX_ERR_UNK;
		return;
	}
}

void Rex::ThreadQuery( PQUERYCONTEXT ctx )
{
	// Set up the timeout stuff
	pthread_cond_t	cond = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t	mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock( &mutex );

	ctx->cond	= &cond;
	ctx->mutex	= &mutex;

	struct timespec	ts;
	struct timeval	tp, tpbegin;
	gettimeofday( &tp, NULL );
	memcpy( &tpbegin, &tp, sizeof(struct timeval) );

	// Record beginning time
	ts.tv_sec = tp.tv_sec;
	ts.tv_nsec = tp.tv_usec * 1000;
	ts.tv_sec += (int)ceil( (float)queryTimeoutMicrosec / (float)1000000 );

	pthread_t threadId;
	pthread_create( &threadId, NULL, &thread, (void*) ctx );

	// Wait...
	if( pthread_cond_timedwait( &cond, &mutex, &ts ) == ETIMEDOUT )
	{
		// Cancel (instead of kill) thread.
		// ref: http://devinfee.com/blog/2010/12/01/pthread-kill-and-cancel/
		//pthread_cancel( threadId );
		
		ctx->retval = REX_SE_TIMEOUT;
	}
	else
	{
		//retval = ctx->retval;
	}

	
	// Subtract the time elapsed from queryTimeoutMicrosec
	struct timeval	tpend;
	gettimeofday( &tpend, NULL );
	// Convert from timeval to timespec

	log->debug( "ELAPSED %d\n",  ( tpend.tv_sec*1000000 + tpend.tv_usec ) -
		( tpbegin.tv_sec*1000000 + tpbegin.tv_usec ) );
	queryTimeoutMicrosec -=
		( tpend.tv_sec*1000000 + tpend.tv_usec ) -
		( tpbegin.tv_sec*1000000 + tpbegin.tv_usec );
	if( queryTimeoutMicrosec < 0 )
		queryTimeoutMicrosec = 0;
	pthread_join( threadId, NULL );
	pthread_mutex_unlock( &mutex );
	pthread_cond_destroy( &cond );
	pthread_mutex_destroy( &mutex );
	pthread_detach( threadId );
}

REXSTATUS Rex::EmitOutputs(
	VC &vc,
	#ifdef USE_STP_FILE
	fstream &queryfs,
	char query[512],
	const char *queryfname,
	#endif
	RexFunc &func1,
	RexFunc &func2,
	list<IO*> *o_auxpmtn,
	list<IO*> *o_argpmtn,
	list<IO*> *o_varpmtn )
{
	unsigned int		num_queries = 0;
	Expr			*e_queries;
	int			query_res;
	REXSTATUS		retval;
	#ifdef USE_STP_FILE
	query[0] = 0x0;
	#endif

	log->debug("TIME LEFT: %d\n", queryTimeoutMicrosec );
	// No more time available.
	if( queryTimeoutMicrosec == 0 )
		return REX_SE_TIMEOUT;

	retval = REX_SE_TIMEOUT;

	size_t num_entries =
		func1.GetPathCond()->GetNumIO() +
		func2.GetPathCond()->GetNumIO() + 1;
	if( o_auxpmtn != NULL ) num_entries += o_auxpmtn->size();
	if( o_argpmtn != NULL ) num_entries += o_argpmtn->size();
	if( o_varpmtn != NULL ) num_entries += o_varpmtn->size();

	e_queries = new Expr[num_entries];

	// Emit conjugated outputs
	#ifdef USE_STP_FILE
	if( o_auxpmtn != NULL )
		num_queries += EmitConjOutputs ( vc, queryfs, query, func1, func2,
			func1.GetOutAuxs()->GetList(), o_auxpmtn, e_queries, num_queries  );
	if( o_argpmtn != NULL )
		num_queries += EmitConjOutputs ( vc, queryfs, query, func1, func2,
			func1.GetOutArgs()->GetList(), o_argpmtn, e_queries, num_queries  );
	if( o_varpmtn != NULL )
		num_queries += EmitConjOutputs ( vc, queryfs, query, func1, func2,
			func1.GetOutVars()->GetList(), o_varpmtn, e_queries, num_queries  );
	#else
	if( o_auxpmtn != NULL )
		num_queries += EmitConjOutputs ( vc, func1, func2,
			func1.GetOutAuxs()->GetList(), o_auxpmtn, e_queries, num_queries  );
	if( o_argpmtn != NULL )
		num_queries += EmitConjOutputs ( vc, func1, func2,
			func1.GetOutArgs()->GetList(), o_argpmtn, e_queries, num_queries  );
	if( o_varpmtn != NULL )
		num_queries += EmitConjOutputs ( vc, func1, func2,
			func1.GetOutVars()->GetList(), o_varpmtn, e_queries, num_queries  );
	#endif

	// Path conditions and retn
	Expr retnExpr = NULL;
	#ifdef USE_STP_FILE
	EmitPathCond( vc, queryfs, func1 );
	EmitPathCond( vc, queryfs, func2 );
	retnExpr = GetRetnExpr( vc, query, func1, func2 );

	queryfs << "QUERY( " << query << " );\n";
	queryfs.flush();
	queryfs.close();
	#else
	EmitPathCond( vc, func1 );
	EmitPathCond( vc, func2 );
	retnExpr = GetRetnExpr( vc, func1, func2 );
	#endif

	if( retnExpr != NULL )
		e_queries[ num_queries++ ] = retnExpr;

	// Create thread to make queries
	QUERYCONTEXT ctx;
	ctx.vc		= &vc;
	ctx.e_queries	= e_queries;
	ctx.num_queries	= num_queries;
	#ifdef USE_STP_FILE
	snprintf( ctx.queryfname, 256, "%s", queryfname );
	#endif
	//ctx.log		= log;

	// TODO
	//ThreadQuery( &ctx );
	//ForkQuery( &ctx );
	child( &ctx );
	

	log->debug("Exiting EmitOutputs\n");

	delete [] e_queries;
	return ctx.retval;
}

/*!
Recursive function to emit nested constraints for memory.
Function returns false if the innermost memory load is not yet
generated. Otherwise, it returns true.
*/
void Rex::EmitSingleMemConstraint(
	VC &vc,
	Expr *expr,
	#ifdef USE_STP_FILE
	char query[1024],
	const size_t querylen,
	#endif
	RexFunc &func,
	RexTemp *rextmp )
{
	// Base case
	if( rextmp == NULL )
	{
		#ifdef USE_STP_FILE
		query[0] = 0x0;
		#endif
		*expr = NULL;
		return;
	}


	//log->debug("DEBUG rextmp %08x\n", rextmp);
	EmitSingleMemConstraint(
		vc,
		expr,
		#ifdef USE_STP_FILE
		query,
		querylen,
		#endif
		func,
		rextmp->base );


	#ifdef USE_STP_FILE
	char subquery[1024];
	#endif
	Expr subExpr = NULL;




	switch( rextmp->action )
	{
	case REGREAD:
		if( rextmp->base == 0 )
		{
			unsigned int regoffset;
			switch( rextmp->reg )
			{
			case ECX: regoffset = 4; break;
			case EDX: regoffset = 8; break;
			case ESP: regoffset = 16; break;
			case EBP: regoffset = 20; break;
			case REGNONE:
			default: regoffset = 0xff;
			}

			#ifdef USE_STP_FILE
			snprintf( subquery, sizeof(subquery), "MachSt%u[0hex%03x]",
				func.GetFirstMachSt(), regoffset );
			snprintf( query, querylen, "%s", subquery );
			#endif

			subExpr = ( rextmp->base == 0 ?
				vc_bvConstExprFromInt(vc, 12, regoffset) :
				*expr );

			*expr = vc_readExpr( vc,
				vc_lookupVarByName( "MachSt%u", func.GetFirstMachSt() ),
				subExpr );
		}
		
		break;

	case PLUS:
		assert( rextmp->base != 0 );

		#ifdef USE_STP_FILE
		snprintf( subquery, sizeof(subquery), "BVPLUS( 32, %s, 0hex%08x )",
			query, rextmp->offset );
		snprintf( query, querylen, "%s", subquery );
		#endif

		subExpr = *expr;
		*expr = vc_bv32PlusExpr( vc, subExpr,
				vc_bv32ConstExprFromInt( vc, rextmp->offset ) );
		break;
		
	case SUBTRACT:
		assert( rextmp->base != 0 );

		#ifdef USE_STP_FILE
		snprintf( subquery, sizeof(subquery), "BVSUB( 32, %s, 0hex%08x )",
			query, rextmp->offset );
		snprintf( query, querylen, "%s", subquery );
		#endif

		subExpr = *expr;
		*expr = vc_bv32MinusExpr( vc, subExpr,
				vc_bv32ConstExprFromInt( vc, rextmp->offset ) );
		break;

	case USELOAD:
		assert( rextmp->base != 0 );

		#ifdef USE_STP_FILE
		snprintf( subquery, sizeof(subquery), "MemSt%u[ %s ]",
			func.GetFirstMemSt(), query );
		snprintf( query, querylen, "%s", subquery );
		#endif

		subExpr = *expr;
		*expr = vc_readExpr( vc,
			vc_lookupVarByName( "MemSt%u", func.GetFirstMemSt() ),
			subExpr );
		break;
	default:
		// Unexpected
		assert( false );
		return;
	}

	// log->debug( "DEBUG query is \"%s\"\n", query );
}

bool Rex::FilteredMemConstraint( RexTemp * rextmp )
{
	if( rextmp->action == USELOAD &&
	rextmp->base != NULL && rextmp->base->action == SUBTRACT && rextmp->base->offset == 4 &&
	rextmp->base->base != NULL && rextmp->base->base->action == REGREAD )
		return true;

	if( rextmp->action == USELOAD &&
	rextmp->base != NULL && rextmp->base->action == PLUS && rextmp->base->offset == 4 &&
	rextmp->base->base != NULL && rextmp->base->base->action == SUBTRACT && rextmp->base->base->offset == 4 &&
	rextmp->base->base->base != NULL && rextmp->base->base->base->action == REGREAD )
		return true;


	return false;
}

/*!
Asserts memory constraints. We assume that the order in which
the memory is loaded is the same. Otherwise the permutation is
too huge!
*/
bool Rex::EmitMemConstraints(
	VC &vc,
	#ifdef USE_STP_FILE
	fstream &queryfs,
	#endif
	RexFunc &func1,
	RexFunc &func2 )
{
	RexMach *rexmach1 = func1.GetRexMach();
	RexMach *rexmach2 = func2.GetRexMach();

	RexTemp *rextemp1 = rexmach1->GetFirstSortedLoadRexTemp();
	RexTemp *rextemp2 = rexmach2->GetFirstSortedLoadRexTemp();

	#ifdef USE_STP_FILE
	char qleft[1024];
	char qright[1024];
	#endif

	Expr lhsExpr;
	Expr rhsExpr;
	bool emitted = false;

	while( rextemp1 != NULL &&
	rextemp2 != NULL )
	{
		if( FilteredMemConstraint(rextemp2) == true ||
		FilteredMemConstraint(rextemp1) == true )
		{
			rextemp1 = rexmach1->GetNextSortedLoadRexTemp();
			rextemp2 = rexmach2->GetNextSortedLoadRexTemp();
			continue;
		}

		EmitSingleMemConstraint(
			vc,
			&lhsExpr,
			#ifdef USE_STP_FILE
			qleft,
			sizeof(qleft),
			#endif
			func1,
			rextemp1 );
		if( lhsExpr == NULL )
		{
			rextemp1 = rexmach1->GetNextSortedLoadRexTemp();
			rextemp2 = rexmach2->GetNextSortedLoadRexTemp();
			continue;
		}

		EmitSingleMemConstraint(
			vc,
			&rhsExpr,
			#ifdef USE_STP_FILE
			qright,
			sizeof(qright),
			#endif
			func2,
			rextemp2 );
		if( rhsExpr == NULL )
		{
			rextemp1 = rexmach1->GetNextSortedLoadRexTemp();
			rextemp2 = rexmach2->GetNextSortedLoadRexTemp();
			continue;
		}

		#ifdef USE_STP_FILE
		queryfs << "ASSERT( " << qleft << " = " << qright << " );\n";
		#endif

		vc_assertFormula( vc, vc_eqExpr( vc, lhsExpr,rhsExpr ) );

		emitted = true;
		rextemp1 = rexmach1->GetNextSortedLoadRexTemp();
		rextemp2 = rexmach2->GetNextSortedLoadRexTemp();
	}
	return emitted;
}

/*!
Emits asserts and query for the different permutations of function 1 and 2
inputs and outputs.
*/
REXSTATUS Rex::EmitAssertedQuery(
	VC &vc,
	#ifdef USE_STP_FILE
	fstream &template_fs,
	const char stpqpref[256],
	#endif
	RexFunc &func1,
	RexFunc &func2 )
{
	REXSTATUS	status = REX_SE_FALSE;
	//list<IO*>* 	iarg_pmtn;
	list<IO*>*	ivar_pmtn;
	list<IO*>*	oaux_pmtn;
	list<IO*>*	oarg_pmtn;
	list<IO*>*	ovar_pmtn;

	int		m, n, o, p, q;
	RexParam	*func1_iargs;
	RexParam	*func1_ivars;
	RexParam	*func2_iargs;
	RexParam	*func2_ivars;
	RexOut		*func2_oauxs;
	RexOut		*func2_oargs;
	RexOut		*func2_ovars;

	func1_iargs = func1.GetArgs();
	func1_ivars = func1.GetVars();
	func2_iargs = func2.GetArgs();
	func2_ivars = func2.GetVars();
	func2_oauxs = func2.GetOutAuxs();
	func2_oargs = func2.GetOutArgs();
	func2_ovars = func2.GetOutVars();

	#ifdef USE_STP_FILE
	char		query[512];
	fstream		queryfs;
	char		queryfname[256];

	log->debug( "%s:\n\t%d(%d!) oauxs\n\t%d(%d!) oargs\n\t%d(%d!) ovars\n\t"
		"%d(%d!) args\n\t%d(%d!) vars\n",
		stpqpref,
		func2_oauxs->GetNumPermutation(), func2_oauxs->GetNumIO(),
		func2_oargs->GetNumPermutation(), func2_oargs->GetNumIO(),
		func2_ovars->GetNumPermutation(), func2_ovars->GetNumIO(),
		func2_iargs->GetNumPermutation(), func2_iargs->GetNumIO(),
		func2_ivars->GetNumPermutation(), func2_ivars->GetNumIO() );
	#endif



	// We don't need to check for m == 0 in the outermost loop
	// because the function should have some outputs to non-var
	// or non-arg locations. Otherwise, it isn't really doing
	// any work since no results are seen outside the function.
	m = 0;
	oaux_pmtn = func2_oauxs->GetFirstPermutation();
	while( status == REX_SE_FALSE && oaux_pmtn != NULL ) // For each func2 aux output pmtn
	{
		if( IsValidConjOutputs( func1.GetOutAuxs()->GetList(), oaux_pmtn ) == false )
		{
			m++;
			oaux_pmtn = func2_oauxs->GetNextPermutation();
			continue;
		}

		n = 0;
		oarg_pmtn = func2_oargs->GetFirstPermutation();
		while( status == REX_SE_FALSE &&
		( oarg_pmtn != NULL || n == 0 ) ) // For each func2 arg output pmtn
		{
			if( oarg_pmtn != NULL &&
			IsValidConjOutputs( func1.GetOutArgs()->GetList(), oarg_pmtn ) == false )
			{
				n++;
				oarg_pmtn = func2_oargs->GetNextPermutation();
				continue;
			}
	
			o = 0;
			ovar_pmtn = func2_ovars->GetFirstPermutation();
			while( status == REX_SE_FALSE &&
			( ovar_pmtn != NULL || o == 0 ) ) // For each func2 var output pmtn
			{
				if( ovar_pmtn != NULL &&
				IsValidConjOutputs( func1.GetOutVars()->GetList(), ovar_pmtn ) == false )
				{
					o++;
					ovar_pmtn = func2_ovars->GetNextPermutation();
					continue;
				}

				q = 0;
				ivar_pmtn = func2_ivars->GetFirstPermutation();
				while( status == REX_SE_FALSE &&
				( ivar_pmtn != NULL || q == 0 ) ) // For each func2 var in pmtn
				{
					vc_push( vc );

					#ifdef USE_STP_FILE
					// Create query file and copy from template file
					snprintf( queryfname, sizeof(queryfname), "%s_%d_%d_%d_%d.stp",
						stpqpref, m, n, o, q );
					queryfs.open( queryfname, fstream::out );
					template_fs.seekg( fstream::beg );
					queryfs << template_fs.rdbuf();
					#endif

					// This replaces EmitParams for input
					// arguments.
					bool emitted = EmitMemConstraints( vc,
						#ifdef USE_STP_FILE
						queryfs,
						#endif
						func1, func2 );
	
					if( emitted == true )
					{
						/*
						if( ivar_pmtn != NULL )
						{
							EmitParams( vc,
								#ifdef USE_STP_FILE
								queryfs,
								#endif
								func1,
								func2,
								func1_ivars->GetList(),
								ivar_pmtn );
						}
						*/

						status = EmitOutputs( vc,
							#ifdef USE_STP_FILE
							queryfs, query, queryfname,
							#endif
							func1, func2,
							oaux_pmtn,
							oarg_pmtn,
							ovar_pmtn );

						#ifdef USE_STP_FILE
						//log->debug( "Removing \"%s\"\n", queryfname );
						//remove( queryfname );
						#endif
					}
					vc_pop( vc );
					q++;
					ivar_pmtn = func2_ivars->GetNextPermutation();
				}
				o++;
				ovar_pmtn = func2_ovars->GetNextPermutation();
			}
			n++;
			oarg_pmtn = func2_oargs->GetNextPermutation();
		}
		m++;
		oaux_pmtn = func2_oauxs->GetNextPermutation();
	}

	return status;
}

REXSTATUS Rex::SemanticEquiv(
	#ifdef USE_STP_FILE
	const char *const stpfile_prefix,
	#endif
	RexFunc &func1,
	RexFunc &func2,
	const int timeout_sec )
{
	size_t		func1_numpaths;
	size_t		func2_numpaths;
	size_t		m, n;
	VC		vc;
	int		retval;
	REXSTATUS	status = REX_SE_ERR;

	func1.SetRexUtils( rexutils1 );
	func2.SetRexUtils( rexutils2 );
	vc = vc_createValidityChecker();

	func1_numpaths = func1.GetNumPaths();
	if( func1_numpaths > PATH_LIMIT ) return REX_TOO_MANY;
	func2_numpaths = func2.GetNumPaths();
	if( func2_numpaths > PATH_LIMIT ) return REX_TOO_MANY;

	log->debug( "Func %d has %d paths.\nFunc %d has %d paths.\n",
		func1.GetId(), func1_numpaths, func2.GetId(), func2_numpaths );

	// Set query timeout
	queryTimeoutMicrosec = timeout_sec * 1000000;

	m = 0;
	// For every path condition in func1
	while( m < func1_numpaths )
	{
		#ifdef USE_STP_FILE
		// Create a file to store the CVC for func1, which will
		// be concatenated with the CVC for func2.
		fstream func1fs;
		char	func1fname[256];
		snprintf( func1fname, sizeof(func1fname), "%s_%d.stp", stpfile_prefix, m );
		assert( func1fs.is_open() == false );
		func1fs.open( func1fname, fstream::in|fstream::out|fstream::trunc );
		#endif

		rexstp->ResetStatesSnapshot();
		vc_push( vc );

		// Emit func1
		rexutils1->AllocMemPool( 1 << 28 );
		func1.SetUsePath(m);
		func1.ClearIRSBs();
		func1.ClearIOs();
		retval = rexstp->EmitFunc(
			#ifdef USE_STP_FILE
			func1fs,
			#endif
			vc,
			func1 );
		if( retval != 0 )
		{
			status = REX_SE_ERR;
			vc_pop( vc ); // for func1
			vc_Destroy(vc);
			rexutils1->FreeMemPool();
			return status;
		}

		rexstp->SaveStatesSnapshot();
		status = REX_SE_FALSE;

		// For every path condition in func2
		n = 0;
		while( status == REX_SE_FALSE &&
		n < func2_numpaths )
		{
			func1.ResetUsePathItr();
			func1.ZapArgsDummies();
			func1.ZapVarsDummies();
			func1.ZapOutAuxsDummies();
			func1.ZapOutArgsDummies();
			func1.ZapOutVarsDummies();

			#ifdef USE_STP_FILE
			char	stpfname[256];
			char	stpqpref[256];

			// Create a file to store the CVC for func2, which will
			// be prefixed with the CVC for func1.
			fstream func2fs;
			snprintf( stpqpref, sizeof(stpqpref), "%s_%d_%d", stpfile_prefix, m, n );
			snprintf( stpfname, sizeof(stpfname), "%s.stp", stpqpref );
			assert( func2fs.is_open() == false );
			func2fs.open( stpfname, fstream::in|fstream::out|fstream::trunc );

			// Copy func1 CVC into this file
			func1fs.seekg( fstream::beg );
			func2fs << func1fs.rdbuf();
			#endif

			vc_push( vc );

			// Emit func2
			rexstp->RevertStatesSnapshot();
			rexutils2->AllocMemPool( 1 << 28 );
			func2.SetUsePath(n);
			func2.ClearIRSBs();
			func2.ClearIOs();
			log->debug( "Emitting func2\n" );
			retval = rexstp->EmitFunc(
				#ifdef USE_STP_FILE
				func2fs,
				#endif
				vc,
				func2 );
			if( retval != 0 )
			{
				status = REX_SE_ERR;
				vc_pop( vc ); // for func2
				vc_pop( vc ); // for func1
				vc_Destroy(vc);
				rexutils1->FreeMemPool();
				rexutils2->FreeMemPool();
				return status;
			}
			//log->debug("func1 Path %d: ", m); func1.PrintUsePath();
			//log->debug("func2 Path %d: ", n); func2.PrintUsePath();

			if( func1.GetNumRetn() != func2.GetNumRetn() )
			{
				log->debug("Skipping because different number of retns (%d vs %d )\n",
					func1.GetNumRetn(), func2.GetNumRetn());
				#ifdef USE_STP_FILE
				// Close STP file
				func2fs.close();
				assert( func2fs.is_open() == false );
				#endif
				vc_pop( vc );
				rexutils2->FreeMemPool();

				n++;
				continue;
			}
			/*
			// Since we're using RexMachine to extract the input arguments
			// now, we don't need this anymore.

			if( func1.GetNumArgs() != func2.GetNumArgs() )
			{
				log->debug("Skip diff num args (%d vs %d)\n",
					func1.GetNumArgs(),
					func2.GetNumArgs() );
				log->debug("func1 IArgs: "); func1.PrintArgs();
				log->debug("func2 IArgs: "); func2.PrintArgs();
				#ifdef USE_STP_FILE
				// Close STP file
				func2fs.close();
				assert( func2fs.is_open() == false );
				#endif
				vc_pop( vc );
				rexutils2->FreeMemPool();

				n++;
				continue;
			}
			*/
			// Don't permute input arguments.
			//func2.PermuteArgs();
	

			if( func1.GetNumVars() != func2.GetNumVars() )
			{
				log->debug("Skip diff num vars (%d vs %d)\n",
					func1.GetNumVars(),
					func2.GetNumVars() );
				#ifdef USE_STP_FILE
				// Close STP file
				func2fs.close();
				assert( func2fs.is_open() == false );
				#endif
				vc_pop( vc );
				rexutils2->FreeMemPool();

				n++;
				continue;
			}
			func2.PermuteVars();

			// Make sure outputs from both func1 and func2 are
			// of the same size.
			size_t func1_noutauxs = func1.GetNumOutAuxs();
			size_t func2_noutauxs = func2.GetNumOutAuxs();
			size_t func1_noutargs = func1.GetNumOutArgs();
			size_t func2_noutargs = func2.GetNumOutArgs();
			size_t func1_noutvars = func1.GetNumOutVars();
			size_t func2_noutvars = func2.GetNumOutVars();
			if( func1_noutauxs != func2_noutauxs ||
			func1_noutargs != func2_noutargs ||
			func1_noutvars != func2_noutvars )
			//if( false )
			{
				
				log->debug("Skip diff num outputs "
					"AUX: %d,%d "
					"ARG: %d,%d "
					"VAR: %d,%d\n",
					func1_noutauxs,
					func2_noutauxs,
					func1_noutargs,
					func2_noutargs,
					func1_noutvars,
					func2_noutvars );
				vc_pop( vc );
				rexutils2->FreeMemPool();

				/*
				func1.GetRexMach()->PrintStore();
				func2.GetRexMach()->PrintStore();
				log->debug("func1 OAuxs: "); func1.PrintOutAuxs();
				log->debug("func2 OAuxs: "); func2.PrintOutAuxs();
				log->debug("func1 OArgs: "); func1.PrintOutArgs();
				log->debug("func2 OArgs: "); func2.PrintOutArgs();
				log->debug("func1 OVars: "); func1.PrintOutVars();
				log->debug("func2 OVars: "); func2.PrintOutVars();
				log->debug( "######### FUNC1 IRSB ##########\n" );
				func1.PrintIRSBs();
				log->debug( "######### FUNC2 IRSB ##########\n" );
				func2.PrintIRSBs();
				*/

				#ifdef USE_STP_FILE
				// Close STP file
				func2fs.close();
				assert( func2fs.is_open() == false );
				#endif
				n++;
				continue;
			}
			log->debug( "func1 #out_auxs %zu func2 #out_auxs %zu\n",
				func1.GetNumOutAuxs(),
				func2.GetNumOutAuxs() );

			if( func1_noutauxs > PERMUTATION_LIMIT ||
			func2_noutauxs > PERMUTATION_LIMIT ||
			func1_noutargs > PERMUTATION_LIMIT ||
			func2_noutargs > PERMUTATION_LIMIT ||
			func1_noutvars > PERMUTATION_LIMIT ||
			func2_noutvars > PERMUTATION_LIMIT )
			{
				status = REX_TOO_MANY;
				#ifdef USE_STP_FILE
				// Close STP file
				func2fs.close();
				assert( func2fs.is_open() == false );
				#endif
				vc_pop( vc );
				rexutils2->FreeMemPool();

				n++;
				continue;
			}

			//func1.GetRexMach()->PrintStore();
			//func1.PrintIRSBs();
			func2.PermuteOutAuxs();
			func2.PermuteOutArgs();
			func2.PermuteOutVars();

			if( func1.GetNumPathConds() != func2.GetNumPathConds() )
			{
				log->debug("Skip diff num pathconds %d vs %d\n",
					func1.GetNumPathConds(),
					func2.GetNumPathConds() );
				vc_pop( vc );
				rexutils2->FreeMemPool();

				#ifdef USE_STP_FILE
				// Close STP file
				func2fs.close();
				assert( func2fs.is_open() == false );
				#endif
				n++;
				continue;

			}

			/*
			log->debug("func1 IArgs: "); func1.PrintArgs();
			log->debug("func2 IArgs: "); func2.PrintArgs();
			log->debug("func1 IVars: "); func1.PrintVars();
			log->debug("func2 IVars: "); func2.PrintVars();
			log->debug("func1 OAuxs: "); func1.PrintOutAuxs();
			log->debug("func2 OAuxs: "); func2.PrintOutAuxs();
			log->debug("func1 OArgs: "); func1.PrintOutArgs();
			log->debug("func2 OArgs: "); func2.PrintOutArgs();
			log->debug("func1 OVars: "); func1.PrintOutVars();
			log->debug("func2 OVars: "); func2.PrintOutVars();
			log->debug("func1 Retn: "); func1.PrintRetn();
			log->debug("func2 Retn: "); func2.PrintRetn();
			log->debug("func1 PC: "); func1.PrintPathConds();
			log->debug("func2 PC: "); func2.PrintPathConds();
			*/

			status = EmitAssertedQuery(
				vc,
				#ifdef USE_STP_FILE
				func2fs,
				stpqpref,
				#endif
				func1,
				func2 );

			log->debug("Path %d,%d %s valid permutations.\n",
				m, n, status == REX_SE_TRUE ? "has" : "does NOT have" );

			
			#ifdef USE_STP_FILE
			// Close STP file
			func2fs.close();
			assert( func2fs.is_open() == false );
			#endif
			vc_pop( vc );
			rexutils2->FreeMemPool();
			n++;
		}

		#ifdef USE_STP_FILE
		func1fs.close();
		assert( func1fs.is_open() == false );
		#endif
		vc_pop( vc );
		rexutils1->FreeMemPool();

		// If we didn't find any path in func2 that
		// matches this path in func1, then the 2
		// functions don't match.
		if( status != REX_SE_TRUE ) break;

		m++;
	}

	log->debug( "Rex done.\n" );

	vc_Destroy(vc);

	return status;
}


