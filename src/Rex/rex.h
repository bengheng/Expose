#ifndef __REX_HEADER__
#define __REX_HEADER__

#include <libvex.h>
#include <libvex_ir.h>
#include <stdint.h>
#include <stdarg.h>
#include <fstream>
#include <vector>
#include <rexfunc.h>
#include <rexio.h>
#include <rexutils.h>
#include <rexstp.h>
#include <rexmachine.h>
#include <c_interface.h>
#include <map>
#include <log4cpp/Category.hh>
#include <pthread.h>

using namespace std;

typedef enum
{
	REX_SE_FALSE,
	REX_SE_TRUE,
	REX_SE_MISMATCH,
	REX_SE_TIMEOUT,
	REX_SE_ERR,
	REX_TOO_MANY,
	REX_ERR_IR,
	REX_ERR_UNK
} REXSTATUS;

typedef struct _thdcontext
{
	pthread_cond_t		*cond;
	pthread_mutex_t		*mutex;
	VC			*vc;
	Expr			*e_queries;
	unsigned int		num_queries;
	#ifdef USE_STP_FILE
	char			queryfname[256];
	#endif
	//log4cpp::Category	*log;
	REXSTATUS		retval;
} QUERYCONTEXT, *PQUERYCONTEXT;

#define PATH_LIMIT		500
#define PERMUTATION_LIMIT	8

class Rex
{
public:
	Rex( log4cpp::Category *_log,
		RexUtils *_rexutils1,
		RexUtils *_rexutils2,
		RexTranslator *_rextrans );
	~Rex();

	
	REXSTATUS SemanticEquiv(
		#ifdef USE_STP_FILE
		const char *const stpfile_prefix,
		#endif
		RexFunc &func1, RexFunc &func2,
		const int timeout_sec );

private:
	RexSTP			*rexstp;
	log4cpp::Category	*log;		// logger
	RexUtils		*rexutils1;	// RexUtils for func1
	RexUtils		*rexutils2;	// RexUtils for func2
	int			queryTimeoutMicrosec;
	void EmitPathCond(
		VC &vc,
		#ifdef USE_STP_FILE
		fstream &queryfs,
		#endif
		RexFunc &func );

	Expr GetRetnExpr(
		VC &vc,
		#ifdef USE_STP_FILE
		char query[512],
		#endif
		RexFunc &func1,
		RexFunc &func2 );

	Expr GetOutputExpr(
		VC &vc,
		#ifdef USE_STP_FILE
		char query[512],
		fstream &fs,
		#endif
		const unsigned int memSt,
		const unsigned int machSt,
		IO *const out );

	void EmitParams(
		VC &vc,
		#ifdef USE_STP_FILE
		fstream &queryfs,
		#endif
		RexFunc &func1,
		RexFunc &func2,
		list<IO*> *params1,
		list<IO*> *params2 );

	bool IsValidConjOutputs (
		list<IO*> *func1_outs,
		list<IO*> *func2_outs );

	unsigned int EmitConjOutputs (
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
		const unsigned int e_queries_offset  );

	REXSTATUS EmitOutputs(
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
		list<IO*> *o_varpmtn );

	REXSTATUS EmitAssertedQuery(
		VC &vc,
		#ifdef USE_STP_FILE
		fstream &template_fs,
		const char stpqpref[256],
		#endif
		RexFunc &func1,
		RexFunc &func2 );

	bool EmitMemConstraints(
		VC &vc,
		#ifdef USE_STP_FILE
		fstream &queryfs,
		#endif
		RexFunc &func1,
		RexFunc &func2 );

	void EmitSingleMemConstraint(
		VC &vc,
		Expr *expr,
		#ifdef USE_STP_FILE
		char query[1024],
		const size_t querylen,
		#endif
		RexFunc &func,
		RexTemp *rextmp );

	bool FilteredMemConstraint( RexTemp * rextmp );

	pid_t DoFork( PQUERYCONTEXT ctx );
	void ForkQuery( PQUERYCONTEXT ctx );
	void ThreadQuery( PQUERYCONTEXT ctx );

};

#endif
