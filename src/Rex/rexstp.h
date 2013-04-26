#ifndef __REXSTP_HEADER__
#define __REXSTP_HEADER__

#include <libvex.h>
#include <libvex_ir.h>
#include <stdint.h>
#include <stdarg.h>
#include <fstream>
#include <vector>
#include <rexfunc.h>
#include <rexio.h>
#include <rexutils.h>
#include <rextranslator.h>
#include <c_interface.h>
#include <map>
#include <MurmurHash3.h>
#include <c_interface.h>
#include <log4cpp/Category.hh>

using namespace std;

#define OBUFSIZE 512		

typedef list< IRStmt* >				REGDEF;
typedef REGDEF::iterator			REGDEFITR;
typedef list< pair<IRStmt*, REGDEFITR> >	REGPLUS;
typedef REGPLUS::iterator			REGPLUSITR;
typedef list< pair<IRStmt*, REGPLUSITR> >	REGLOAD;
typedef REGLOAD::iterator			REGLOADITR;
typedef list< pair<IRStmt*, REGLOADITR> >	REGPUT;
typedef REGPUT::iterator			REGPUTITR;



class RexSTP
{
public:
	RexSTP( log4cpp::Category *log, RexUtils *rexutils, RexTranslator *_rextrans );
	~RexSTP();

	int EmitFunc(
		#ifdef USE_STP_FILE
		fstream &fs,
		#endif
		VC &vc,
		RexFunc &func );

	void ResetStatesSnapshot();
	void SaveStatesSnapshot();
	void RevertStatesSnapshot();

	
private:
	char					out[OBUFSIZE];		// output buffer
	char *					outEnd;			// end of output buffer
	unsigned int				bbid;			// Basic block ID
	bool					useMachSt1;
	bool					useMemSt1;
	unsigned int				machSt;			// Machine state
	unsigned int				memSt;			// Memory state
	unsigned int				lastMachSt;		// Last machine state
	unsigned int				lastMemSt;		// last memory state
	log4cpp::Category			*log;			// logger
	RexUtils				*rexutils;
	RexTranslator				*rextrans;

	size_t GetTypeWidth( const IRType ty );

	int ExtractPathCond(
		RexFunc &func,
		IRSB *irsb,
		const unsigned int bbid );

	int EmitIRSB(
		#ifdef USE_STP_FILE
		fstream &fs,
		#endif
		VC &vc,
		RexFunc &func,
		IRSB *const irsb );

	int TranslateRexBB(
		RexTranslator *rextrans,
		RexFunc &func,
		#ifdef USE_STP_FILE
		fstream &fs,
		#endif
		VC &vc);

	const char * GetOp			( const IROp op );

	int ConstToUint32			( IRConst *const c, uint32_t *con );

	#ifdef USE_STP_FILE
	int EmitCstrPut_Const32			( fstream &fs, VC &vc, const Int putOffset, const uint32_t c, const IRType ty, RexOut *o_auxs );
	int EmitCstrPut_RdTmp32			( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const Int putOffset, const IRTemp tmp, const IRType ty, RexOut *o_auxs );
	int EmitCstrPut				( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, IRSB *const irsb, IRStmt *const stmt, RexOut *o_auxs );

	int EmitCstrWrTmp_Const32		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const uint32_t c );
	int EmitCstrWrTmp_Get32			( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRType wrty, const Int getOffset );
	int EmitCstrWrTmp_LoadRdTmp		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRType wrty, const IRTemp ldtmp );
	int EmitCstrWrTmp_RdTmp			( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRType wrty, const IRTemp rdtmp );
	int EmitCstrWrTmp_LoadConst		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const uint32_t addr );

	int EmitCstrWrTmp_Binop			( fstream &fs, VC &vc, IRSB *const irsb, const unsigned int fid, const unsigned int bid, const IRType wrty,  const IRTemp wrtmp, const IRExpr *const rhs );
	int EmitCstrWrTmp_BinopTmpTmp		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp arg1rdtmp, const IRTemp arg2rdtmp, const IROp op, const size_t wrtmpwidth, const size_t arg1width, const size_t arg2width );
	int EmitCstrWrTmp_BinopTmpConst		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp arg1rdtmp, const uint32_t arg2const, const IROp op, const size_t wrtmpwidth, const size_t arg1width, const size_t arg2width );
	int EmitCstrWrTmp_BinopConstTmp		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp arg1const, const uint32_t arg2rdtmp, const IROp op, const size_t wrtmpwidth, const size_t arg1width, const size_t arg2width);
	int EmitCstrWrTmp_BinopConstConst	( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp arg1const, const uint32_t arg2const, const IROp op, const size_t wrtmpwidth, const size_t arg1width, const size_t arg2width );
	int EmitCstrWrTmp_BinopXX		( fstream &ifs, char *s_wrtmp, char *s_arg1, char *s_arg2, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IROp op, const Expr e_wrtmp, const Expr e_arg1, const Expr e_arg2, const size_t wrtmpwidth, const size_t arg1width, const size_t arg2width, const int arg1const = -1, const int arg2const = -1 );
	int EmitCstrShiftByTmp			( fstream &fs, char *s_wrtmp, char *s_arg1, char *s_arg2, VC &vc, const IROp op, const Expr e_wrtmp, const Expr e_arg1, const Expr e_arg2, const size_t nbits );
	int EmitCstrArithShiftByTmp		( fstream &fs, char *s_wrtmp, char *s_arg1, char *s_arg2, VC &vc, const IROp op, const Expr e_wrtmp, const Expr e_arg1, const Expr e_arg2, const size_t nbits );

	int EmitCstrWrTmp_Unop			( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRExpr *const rhs );
	int EmitCstrWrTmp_UnopNot32		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_UnopNot1		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop1Uto32		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop32to1		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop8Sto32		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop8Uto32		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop16Uto32		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop32to8Tmp		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop32to8Const	( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const uint32_t argconst );
	int EmitCstrWrTmp_Unop64loto32		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop64HIto32		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop1Uto8		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop32to16Tmp		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop32to16Const	( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const uint32_t argconst );
	int EmitCstrWrTmp_Unsupported		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRType wrty );

	int EmitCstrWrTmp			( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, IRSB *const irsb, IRStmt *const stmt, /*IRStmt **ebpStmt, */RexParam *args, RexParam *vars );

	int EmitCstrStore_TmpTmpITE		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp lhstmp, const IRTemp rhstmp, RexOut *outputs, const int ebpoffset );
	int EmitCstrStore_TmpConstITE		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp lhstmp, const uint32_t rhsconst, RexOut *outputs, const int ebpoffset );
	int EmitCstrStore_ConstTmpITE		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const uint32_t lhsconst, const IRTemp rhstmp, RexOut *outputs );
	int EmitCstrStore_ConstConstITE		( fstream &fs, VC &vc, const uint32_t lhsconst, const uint32_t rhsconst, RexOut *outputs );
	int EmitCstrStore			( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, IRSB *const irsb, IRStmt *const stmt, RexOut *outputs, const int ebpoffset );

	Expr EmitWidenedOperand			( fstream &fs, VC &vc, Expr e_in, const char* newvarname, const char* oldvarname, const size_t oldnbits, const size_t newnbits, const bool signExtend );
	int EmitCstrWrTmp_BinopMull		( fstream &fs, VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, Expr e_wrtmp, const size_t nbits, const char* s_arg1, const char* s_arg2, const size_t arg1bits, const size_t arg2bits, const bool signExtend );

#else
	int EmitCstrPut_Const32			( VC &vc, const Int putOffset, const uint32_t c, const IRType ty, RexOut *o_auxs );
	int EmitCstrPut_RdTmp32			( VC &vc, const unsigned int fid, const unsigned int bid, const Int putOffset, const IRTemp tmp, const IRType ty, RexOut *o_auxs );
	int EmitCstrPut				( VC &vc, const unsigned int fid, const unsigned int bid, IRSB *const irsb, IRStmt *const stmt, RexOut *o_auxs );
	
	int EmitCstrWrTmp_Const32		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const uint32_t c );
	int EmitCstrWrTmp_Get32			( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRType wrty, const Int getOffset );
	int EmitCstrWrTmp_LoadRdTmp		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRType wrty, const IRTemp ldtmp );
	int EmitCstrWrTmp_RdTmp			( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRType wrty, const IRTemp rdtmp );
	int EmitCstrWrTmp_LoadConst		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const uint32_t addr );

	int EmitCstrWrTmp_Binop			( VC &vc, IRSB *const irsb, const unsigned int fid, const unsigned int bid, const IRType wrty, const IRTemp wrtmp, const IRExpr *const rhs );
	int EmitCstrWrTmp_BinopTmpTmp		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp arg1rdtmp, const IRTemp arg2rdtmp, const IROp op, const size_t wrtmpwidth, const size_t arg1width, const size_t arg2width );
	int EmitCstrWrTmp_BinopTmpConst		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp arg1rdtmp, const uint32_t arg2const, const IROp op, const size_t wrtmpwidth, const size_t arg1width, const size_t arg2width );
	int EmitCstrWrTmp_BinopConstTmp		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp arg1const, const uint32_t arg2rdtmp, const IROp op, const size_t wrtmpwidth, const size_t arg1width, const size_t arg2width );
	int EmitCstrWrTmp_BinopConstConst	( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp arg1const, const uint32_t arg2const, const IROp op, const size_t wrtmpwidth, const size_t arg1width, const size_t arg2width );
	int EmitCstrWrTmp_BinopXX		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IROp op, const Expr e_wrtmp, const Expr e_arg1, const Expr e_arg2, const size_t wrtmpwidth, const size_t arg1width, const size_t arg2width,  const int arg1const = -1, const int arg2const = -1 );
	int EmitCstrShiftByTmp			( VC &vc, const IROp op, const Expr e_wrtmp, const Expr e_arg1, const Expr e_arg2, const size_t nbits );
	int EmitCstrArithShiftByTmp		( VC &vc, const IROp op, const Expr e_wrtmp, const Expr e_arg1, const Expr e_arg2, const size_t nbits );

	int EmitCstrWrTmp_Unop			( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRExpr *const rhs );
	int EmitCstrWrTmp_UnopNot32		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_UnopNot1		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop1Uto32		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop32to1		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop8Sto32		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop8Uto32		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop16Uto32		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop32to8Tmp		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop32to8Const	( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const uint32_t argconst );
	int EmitCstrWrTmp_Unop64loto32		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop64HIto32		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop1Uto8		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop32to16Tmp		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRTemp rdtmp );
	int EmitCstrWrTmp_Unop32to16Const	( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const uint32_t argconst );
	int EmitCstrWrTmp_Unsupported		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, const IRType wrty );

	int EmitCstrWrTmp			( VC &vc, const unsigned int fid, const unsigned int bid, IRSB *const irsb, IRStmt *const stmt, /*IRStmt **ebpStmt, */RexParam *args, RexParam *vars );

	int EmitCstrStore_TmpTmpITE		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp lhstmp, const IRTemp rhstmp, RexOut *outputs, const int ebpoffset );
	int EmitCstrStore_TmpConstITE		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp lhstmp, const uint32_t rhsconst, RexOut *outputs, const int ebpoffset );
	int EmitCstrStore_ConstTmpITE		( VC &vc, const unsigned int fid, const unsigned int bid, const uint32_t lhsconst, const IRTemp rhstmp, RexOut *outputs );
	int EmitCstrStore_ConstConstITE		( VC &vc, const uint32_t lhsconst, const uint32_t rhsconst, RexOut *outputs );
	int EmitCstrStore			( VC &vc, const unsigned int fid, const unsigned int bid, IRSB *const irsb, IRStmt *const stmt, RexOut *outputs, const int ebpoffset );

	Expr EmitWidenedOperand			( VC &vc, Expr e_in, const char* newvarname, const size_t oldnbits, const size_t newnbits, const bool signExtend );
	int EmitCstrWrTmp_BinopMull		( VC &vc, const unsigned int fid, const unsigned int bid, const IRTemp wrtmp, Expr e_wrtmp, const size_t nbits, const size_t arg1bits, const size_t arg2bits, const bool signExtend );
#endif

	int EmitIRStmt (
		#ifdef USE_STP_FILE
		fstream &fs,
		#endif
		VC &vc,
		const unsigned int fid,
		const unsigned int bid,
		IRSB *const irsb,
		IRStmt *const stmt,
		//IRStmt **ebpStmt,
		RexParam *args,
		RexParam *vars,
		RexOut *outs,
		const int ebpoffset );

	int EmitDeclareTmp(
		#ifdef USE_STP_FILE
		fstream &fs,
		#endif
		VC &vc,
		const unsigned int fid,
		const unsigned int bid,
		const IRTemp tmp,
		const IRType ty );

	IRStmt* GetStoreRetnAddr( IRSB *const irsb, IRStmt *const ebpStmt );
	IRStmt* GetAdjustedESPStmt( IRSB *const irsb, IRStmt *const ebpStmt );

	int GetStoreTmpEBPoffset( IRSB *const irsb, const int i, IRStmt *const ebpStmt );
	IRStmt* GetEBPStmt( IRSB *const irsb );
	IRStmt* GetEAXStmt( IRSB *const irsb );

	void CheckRegRead( RexMach *rexMach, const unsigned int bbid, IRStmt *stmt );
	int CheckRegPlus( RexMach *rexMach, const unsigned int bbid, IRStmt *stmt );
	int CheckRegSubtract( RexMach *rexMach, const unsigned int bbid, IRStmt *stmt );
	void CheckRegLoad( RexMach *rexMach, const unsigned int bbid, IRStmt *stmt );
	void CheckRegStore( RexMach *rexMach, const unsigned int bbid, IRStmt *stmt, const unsigned int memst );
	void CheckRegWrite( RexMach *rexMach, const unsigned int bbid, IRStmt *stmt );

	void TransferStoreTmpToOutput( RexFunc &func );
	bool DerivedFromESP( RexTemp* rtmp );

};

#endif
