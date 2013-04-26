#ifndef __REXIR_HEADER__
#define __REXIR_HEADER__

// Adapted from libvex_ir.h

#ifdef __cplusplus
extern "C" {
#endif
#include <libvex.h>
#ifdef __cplusplus
}
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <log4cpp/Category.hh>
//#include <log4cpp/FileAppender.hh>
//#include <log4cpp/PatternLayout.hh>
//#include <log4cpp/SimpleLayout.hh>

class RexUtils
{
public:
	RexUtils( log4cpp::Category *_log );
	~RexUtils();

	int		AllocMemPool( const size_t nbytes );
	void		FreeMemPool();
	void*		Alloc( const size_t nbytes );

	void		RexPrint( const char *_msg, ... );

	void		RexPPIRType ( const IRType ty );
	void		RexPPIRConst ( const IRConst *const con );
	void		RexPPIRCallee ( const IRCallee *const ce );
	void		RexPPIRRegArray ( const IRRegArray *const arr );
	void		RexPPIRTemp ( const IRTemp tmp );
	void		RexPPIROp ( const IROp op );
	void		RexPPIRExpr ( const IRExpr *const e );
	void		RexPPIREffect ( const IREffect fx );
	void		RexPPIRDirty ( const IRDirty *const d );
	void		RexPPIRCAS ( const IRCAS *const cas );
	void		RexPPIRJumpKind ( const IRJumpKind kind );
	void		RexPPIRMBusEvent ( const IRMBusEvent event );
	void		RexPPIRStmt ( const IRStmt *const s );
	void		RexPPIRTypeEnv ( const IRTypeEnv *const env );
	void		RexPPIRSB ( const IRSB *const bb );


	IRConst*	RexIRConst_U1 ( Bool bit );
	IRConst*	RexIRConst_U8 ( UChar u8 );
	IRConst*	RexIRConst_U16 ( UShort u16 );
	IRConst*	RexIRConst_U32 ( UInt u32 );
	IRConst*	RexIRConst_U64 ( ULong u64 );
	IRConst*	RexIRConst_F64 ( Double f64 );
	IRConst*	RexIRConst_F64i ( ULong f64i );
	IRConst*	RexIRConst_V128 ( UShort con );

	IRCallee*	RexMkIRCallee ( Int regparms, HChar* name, void* addr );
	IRRegArray*	RexMkIRRegArray ( Int base, IRType elemTy, Int nElems );
	IRCAS*		RexMkIRCAS ( IRTemp oldHi, IRTemp oldLo, IREndness end, IRExpr* addr, IRExpr* expdHi, IRExpr* expdLo, IRExpr* dataHi, IRExpr* dataLo );

	IRExpr*		RexIRExpr_Binder ( Int binder );
	IRExpr*		RexIRExpr_Get ( Int off, IRType ty );
	IRExpr*		RexIRExpr_GetI ( IRRegArray* descr, IRExpr* ix, Int bias );
	IRExpr*		RexIRExpr_RdTmp ( IRTemp tmp );
	IRExpr*		RexIRExpr_Qop ( IROp op, IRExpr* arg1, IRExpr* arg2, IRExpr* arg3, IRExpr* arg4 );
	IRExpr*		RexIRExpr_Triop  ( IROp op, IRExpr* arg1, IRExpr* arg2, IRExpr* arg3 );
	IRExpr*		RexIRExpr_Binop ( IROp op, IRExpr* arg1, IRExpr* arg2 );
	IRExpr*		RexIRExpr_Unop ( IROp op, IRExpr* arg );
	IRExpr*		RexIRExpr_Load ( IREndness end, IRType ty, IRExpr* addr );
	IRExpr*		RexIRExpr_Const ( IRConst* con );
	IRExpr*		RexIRExpr_CCall ( IRCallee* cee, IRType retty, IRExpr** args );
	IRExpr*		RexIRExpr_Mux0X ( IRExpr* cond, IRExpr* expr0, IRExpr* exprX );

	IRExpr**	RexMkIRExprVec_0 ( void );
	IRExpr**	RexMkIRExprVec_1 ( IRExpr* arg1 );
	IRExpr**	RexMkIRExprVec_2 ( IRExpr* arg1, IRExpr* arg2 );
	IRExpr**	RexMkIRExprVec_3 ( IRExpr* arg1, IRExpr* arg2, IRExpr* arg3 );
	IRExpr**	RexMkIRExprVec_4 ( IRExpr* arg1, IRExpr* arg2, IRExpr* arg3, IRExpr* arg4 );
	IRExpr**	RexMkIRExprVec_5 ( IRExpr* arg1, IRExpr* arg2, IRExpr* arg3, IRExpr* arg4, IRExpr* arg5 );
	IRExpr**	RexMkIRExprVec_6 ( IRExpr* arg1, IRExpr* arg2, IRExpr* arg3, IRExpr* arg4, IRExpr* arg5, IRExpr* arg6 );
	IRExpr**	RexMkIRExprVec_7 ( IRExpr* arg1, IRExpr* arg2, IRExpr* arg3, IRExpr* arg4, IRExpr* arg5, IRExpr* arg6, IRExpr* arg7 );
	IRExpr**	RexMkIRExprVec_8 ( IRExpr* arg1, IRExpr* arg2, IRExpr* arg3, IRExpr* arg4, IRExpr* arg5, IRExpr* arg6, IRExpr* arg7, IRExpr* arg8 );

	IRStmt*		RexIRStmt_NoOp ( void );
	IRStmt*		RexIRStmt_IMark ( Addr64 addr, Int len );
	IRStmt*		RexIRStmt_AbiHint ( IRExpr* base, Int len, IRExpr* nia );
	IRStmt*		RexIRStmt_Put ( Int off, IRExpr* data );
	IRStmt*		RexIRStmt_PutI ( IRRegArray* descr, IRExpr* ix, Int bias, IRExpr* data );
	IRStmt*		RexIRStmt_WrTmp ( IRTemp tmp, IRExpr* data );
	IRStmt*		RexIRStmt_Store ( IREndness end, IRExpr* addr, IRExpr* data );
	IRStmt*		RexIRStmt_CAS ( IRCAS* cas );
	IRStmt*		RexIRStmt_LLSC ( IREndness end, IRTemp result, IRExpr* addr, IRExpr* storedata );
	IRStmt*		RexIRStmt_Dirty ( IRDirty* d );
	IRStmt*		RexIRStmt_MBE ( IRMBusEvent event );
	//IRStmt*		RexIRStmt_MBE( void );
	IRStmt*		RexIRStmt_Exit ( IRExpr* guard, IRJumpKind jk, IRConst* dst );

	IRTypeEnv*	RexEmptyIRTypeEnv ( void );
	IRDirty*	RexEmptyIRDirty ( void );
	IRSB*		RexEmptyIRSB ( void );

	IRExpr**	RexShallowCopyIRExprVec ( IRExpr** vec );
	IRExpr**	RexDeepCopyIRExprVec ( IRExpr** vec );
	IRConst*	RexDeepCopyIRConst ( IRConst* c );
	IRCallee*	RexDeepCopyIRCallee ( IRCallee* ce );
	IRRegArray*	RexDeepCopyIRRegArray( IRRegArray* d );
	IRExpr*		RexDeepCopyIRExpr( IRExpr* e );
	IRDirty*	RexDeepCopyIRDirty ( IRDirty* d );
	IRCAS*		RexDeepCopyIRCAS ( IRCAS* cas );
	IRStmt*		RexDeepCopyIRStmt( IRStmt* s );
	IRTypeEnv*	RexDeepCopyIRTypeEnv( IRTypeEnv* src );
	IRSB*		RexDeepCopyIRSB( IRSB* bb );
	IRSB*		RexDeepCopyIRSBExceptStmts( IRSB* bb );
private:
	//log4cpp::FileAppender	*appender;
	//log4cpp::SimpleLayout	*layout;
	log4cpp::Category	*log;
	size_t			memfree;
	size_t			memsize;
	unsigned char*		mempool;
	unsigned char*		nextmem;

	};

#endif
