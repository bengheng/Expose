#include <assert.h>
#include <libvex.h>
#include <libvex_ir.h>
#include <rexutils.h>
#include <rexstp.h>
#include <rextranslator.h>
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

#include <log4cpp/Category.hh>

using namespace std;

#define VARNAMESIZE	20

#ifdef USE_STP_FILE
#define PRINTOBUF( ob, oe, fmt, ... )			\
	snprintf( ob, oe - ob, fmt, __VA_ARGS__ );	\
	ob += strlen(ob);				\
	assert( ob < oe )

#define STPPRINT( ofs, s )				\
	assert( ofs.is_open() == true );		\
	ofs << s;
#endif

#define TMP(t) t

#define IS_GET32_R( stmt, r )				\
	( ( stmt->tag == Ist_WrTmp ) &&			\
	( stmt->Ist.WrTmp.data->tag == Iex_Get ) &&	\
	( stmt->Ist.WrTmp.data->Iex.Get.offset == r ) )

#define IS_T_GET32_R( t, stmt, r )			\
	( IS_GET32_R( stmt, r ) &&			\
	( stmt->Ist.WrTmp.tmp == t ) )

#define IS_PUT32_R( stmt, r )				\
	( ( stmt->tag == Ist_Put ) &&			\
	( stmt->Ist.Put.offset == r ) )

#define IS_PUT32_RT( stmt, r, t )			\
	( IS_PUT32_R( stmt, r ) &&			\
	( stmt->Ist.Put.data->tag == Iex_RdTmp ) &&	\
	( stmt->Ist.Put.data->Iex.RdTmp.tmp == t ) )

#define IS_PUT32_RC( stmt, r )				\
	( IS_PUT32_R( stmt, r ) &&			\
	( stmt->Ist.Put.data->tag == Iex_Const ) )


#define IS_SUB32( stmt )						\
	(( stmt->tag == Ist_WrTmp ) &&					\
	( stmt->Ist.WrTmp.data->tag == Iex_Binop ) &&			\
	( stmt->Ist.WrTmp.data->Iex.Binop.op == Iop_Sub32 ) )

#define IS_SUB32_T( stmt, t )						\
	( IS_SUB32(stmt) &&						\
	( stmt->Ist.WrTmp.data->Iex.Binop.arg1->tag == Iex_RdTmp ) &&	\
	( stmt->Ist.WrTmp.data->Iex.Binop.arg1->Iex.RdTmp.tmp == t ) )
	
#define IS_SUB32_TC( stmt, t, c_in, c_out )								\
	( IS_SUB32_T( stmt, t ) &&									\
	( stmt->Ist.WrTmp.data->Iex.Binop.arg2->tag == Iex_Const ) &&					\
	( ConstToUint32( stmt->Ist.WrTmp.data->Iex.Binop.arg2->Iex.Const.con, &c_out ) == 0 ) &&	\
	( c_out == c_in ) )

#define IS_ADD32( stmt )						\
	(( stmt->tag == Ist_WrTmp ) &&					\
	( stmt->Ist.WrTmp.data->tag == Iex_Binop ) &&			\
	( stmt->Ist.WrTmp.data->Iex.Binop.op == Iop_Add32 ) )

#define IS_ADD32_TX( stmt, t )						\
	( IS_ADD32( stmt ) &&						\
	( stmt->Ist.WrTmp.data->Iex.Binop.arg1->tag == Iex_RdTmp ) &&	\
	( stmt->Ist.WrTmp.data->Iex.Binop.arg1->Iex.RdTmp.tmp == t ) )

#define IS_ADD32_XC( stmt, c_in, c_out )								\
	( IS_ADD32( stmt ) &&										\
	( stmt->Ist.WrTmp.data->Iex.Binop.arg2->tag == Iex_Const ) &&					\
	( ConstToUint32( stmt->Ist.WrTmp.data->Iex.Binop.arg2->Iex.Const.con, &c_out ) == 0 ) &&	\
	( c_in == c_out ) )

#define IS_ADD32_TC( stmt, t, c_in, c_out )					\
	( IS_ADD32_TX( stmt, t ) &&					\
	IS_ADD32_XC( stmt, c_in, c_out ) )

#define IS_T_ADD32( t, stmt )						\
	( IS_ADD32( stmt ) &&						\
	( stmt->Ist.WrTmp.tmp == t ) &&					\
	( stmt->Ist.WrTmp.data->Iex.Binop.arg1->tag == Iex_RdTmp ) &&	\
	( stmt->Ist.WrTmp.data->Iex.Binop.arg2->tag == Iex_Const ) )

#define IS_STORE32( stmt )					\
	( stmt->tag == Ist_Store )

#define IS_STORE32_T( stmt, t )					\
	( IS_STORE32(stmt) &&					\
	( stmt->Ist.Store.addr->tag == Iex_RdTmp ) &&		\
	( stmt->Ist.Store.addr->Iex.RdTmp.tmp == t ) )

#define IS_STORE32_TT( stmt, t1, t2 )				\
	( IS_STORE32_T( stmt, t1 ) &&				\
	( stmt->Ist.Store.data->tag == Iex_RdTmp ) &&		\
	( stmt->Ist.Store.data->Iex.RdTmp.tmp == t2) )

#define IS_STORE32_TC( stmt, t, c_in, c_out )						\
	( IS_STORE32_T( stmt, t ) &&							\
	( stmt->Ist.Store.data->tag == Iex_Const ) &&					\
	( ConstToUint32( stmt->Ist.Store.data->Iex.Const.con, &c_out ) == 0 ) &&	\
	( c_in == c_out ) )

#define IS_LOAD32( stmt )						\
	( ( stmt->tag == Ist_WrTmp ) &&					\
	( stmt->Ist.WrTmp.data->tag == Iex_Load ) )

#define IS_LOAD32_T( stmt, t )						\
	( IS_LOAD32( stmt ) &&						\
	( stmt->Ist.WrTmp.data->Iex.Load.addr->tag == Iex_RdTmp ) &&	\
	( stmt->Ist.WrTmp.data->Iex.Load.addr->Iex.RdTmp.tmp == t ) )

#define IS_LOAD32_C( stmt, c_in, c_out )							\
	( IS_LOAD32( stmt ) &&									\
	( stmt->Ist.WrTmp.data->Iex.Load.addr->tag == Iex_Const ) &&				\
	( ConstToUint32(stmt->Ist.WrTmp.data->Iex.Load.addr->Iex.Const.con, &c_out) == 0 ) &&	\
	( c_in == c_out ) )

#define IS_T_LOAD32_C( t, stmt, c )						\
	( IS_LOAD32_C( stmt, c ) &&						\
	( stmt->Ist.WrTmp.tmp == t  ) )

#define IS_IMARK( stmt )						\
	( stmt->tag == Ist_IMark )

RexSTP::RexSTP( log4cpp::Category *_log, RexUtils *_rexutils, RexTranslator *_rextrans )
{
	ResetStatesSnapshot();

	out[0]	= 0x0;
	outEnd	= out + OBUFSIZE;

	log		= _log;
	rexutils	= _rexutils;
	rextrans	= _rextrans;
}

RexSTP::~RexSTP()
{
}

/*!
Converts a constant value to a 32 bits unsigned integer.
*/
int RexSTP::ConstToUint32( IRConst *const c, uint32_t *out )
{
	
	assert( c != NULL );

	switch( c->tag )
	{
	case Ico_U1:	*out = c->Ico.U1;	return 0;
	case Ico_U8:	*out = c->Ico.U8;	return 0;
	case Ico_U16:	*out = c->Ico.U16;	return 0;
	case Ico_U32:	*out = c->Ico.U32;	return 0;
	case Ico_U64:
	case Ico_F64:
	case Ico_F64i:
	case Ico_V128:
	default:
		log->fatal( "%s,%d:\tInvalid const tag %d!", __FILE__, __LINE__, c->tag );
		rexutils->RexPPIRConst( c );
		log->fatal( "\n" );
		//exit(1);
		return -1;
	}

	return -1;	
}

#ifdef USE_STP_FILE
const char * OPLUT[] = { "SBVLT", "SBVLE", "BVLT", "BVLE", "BVSUB", "BVPLUS", "BVMULT", "BVXOR",
	"=", "|", "&", "<<", ">>" };

const char * RexSTP::GetOp( const IROp op )
{
	switch( op )
	{
	case Iop_CmpLT64S:
	case Iop_CmpLT32S:	return OPLUT[0];
	case Iop_CmpLT64U:
	case Iop_CmpLT32U:	return OPLUT[2];
	case Iop_CmpLE64S:
	case Iop_CmpLE32S:	return OPLUT[1];
	case Iop_CmpLE64U:
	case Iop_CmpLE32U:	return OPLUT[3];
	case Iop_Sub8:
	case Iop_Sub16:
	case Iop_Sub32:		return OPLUT[4];
	case Iop_Add8:
	case Iop_Add16:
	case Iop_Add32:		return OPLUT[5];
	case Iop_Mul32:
	case Iop_MullS8:
	case Iop_MullS16:
	case Iop_MullS32:
	case Iop_MullU8:
	case Iop_MullU16:	
	case Iop_MullU32:	return OPLUT[6];
	case Iop_Xor8:
	case Iop_Xor16:
	case Iop_Xor32:		return OPLUT[7];
	case Iop_CmpEQ64:
	case Iop_CmpEQ32:
	case Iop_CmpEQ16:
	case Iop_CmpEQ8:
	case Iop_CmpNE64:
	case Iop_CmpNE32:
	case Iop_CmpNE16:
	case Iop_CmpNE8:	return OPLUT[8];
	case Iop_Or32:
	case Iop_Or16:
	case Iop_Or8:		return OPLUT[9];
	case Iop_And32:
	case Iop_And16:
	case Iop_And8:		return OPLUT[10];
	case Iop_Shl8:
	case Iop_Shl16:
	case Iop_Shl32:		return OPLUT[11];
	case Iop_Shr8:
	case Iop_Shr16:
	case Iop_Shr32:		return OPLUT[12];
	default:
		log->fatal( "%s,%d:\tInvalid op type %u!", __FILE__, __LINE__, op );
		rexutils->RexPPIROp( op );
		log->debug("\n");
		exit(1);	
	}

	return "ERROR";
}
#endif

//=======================================
// Put
//=======================================

/*!

Case 1: (common)
	MachSt<n+1> : ARRAY BITVECTOR(12) OF BITVECTOR(32) = MachSt<n> WITH [0hex<ar_base>] := 0hex<c>;

Case 2:
	c<n> : BITVECTOR(32);
	ASSERT( c<n>[<hi>:<lo>] = 0hex<c> );
	ASSERT( c<n>[31:<hi+1>] = MachSt<n>[0hex<ar_base>][31:<hi+1>] );
	ASSERT( c<n>[<lo-1>:0] = MachSt<n>[0hex<ar_base>][<lo-1>:0] );
	MachSt<n+1> : ARRAY BITVECTOR(12) OF BITVECTOR(32) = MachSt<n> WITH [0hex<ar_base>] := c<n>;

*/
int RexSTP::EmitCstrPut_Const32(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const Int putOffset,
	const uint32_t c,
	const IRType ty,
	RexOut *o_auxs )
{
	char		varname[VARNAMESIZE];
	Expr		e_oldMachSt;
	Expr		e_machStIdx;
	Expr		e_machSt;
	Expr		e_con;
	unsigned int	oldMachSt;
	unsigned char	numBytes;
	unsigned char	arrayBase;
	unsigned char	arrayOffset;
	unsigned char	assignHi;
	unsigned char	assignLo;
	#ifdef USE_STP_FILE
	char *		obuf;
	#endif

	arrayOffset = putOffset % 4;
	arrayBase = putOffset - arrayOffset;

	// Only checks certain registers
	if( o_auxs != NULL &&
	( putOffset == 0 ) )	// EAX
	//putOffset == 4 ||	// ECX
	//putOffset == 8 ||	// EDX
	//putOffset == 12 ||	// EBX
	//putOffset == 24 ||	// ESI
	//putOffset == 28 ) )	// EDI
	{
		o_auxs->AddOut_PutOffset( arrayBase );
	}

	switch(ty)
	{
	case Ity_I32:	numBytes = 4;	break;
	case Ity_I16:	numBytes = 2;	break;
	case Ity_I8:	numBytes = 1;	break;
	default:
		numBytes = 4;
		log->fatal( "FATAL %s %d: Invalid type %d!", __FILE__, __LINE__, ty );
		//exit(1); // maybe its ok not to exit...
		return -1;
	}

	#ifdef USE_STP_FILE
	obuf = out;
	obuf[0] = 0x0;
	#endif
	oldMachSt = machSt++;

	e_oldMachSt = vc_lookupVarByName( "MachSt%u", useMachSt1 == true ? 1 : oldMachSt );
	e_machStIdx = vc_bvConstExprFromInt( vc, 12, arrayBase );



	if( arrayOffset != 0 || numBytes != 4 )
	{
		// The annoying case - we are setting a subreg
		// to a constant value. Need to declare new tmp,
		// stitch together the appropriate value
		// and finally update with the new tmp.

		// As a result will update with a tmp instead of
		// a constant value. This could be fixed with
		// some work, but it's not clear whether that
		// would be worth it. So I'll do it this way
		// first and see what the performance results are.


		// Compute range to assign putData
		assignLo = 8*arrayOffset;
		assignHi = assignLo + (8*numBytes) - 1;
		//assignHi = 31 - (8 * arrayOffset);
		//assignLo = assignHi - (8 * numBytes) + 1;

		// Clip assignHi if it overshoots
		if( assignHi > 31 ) assignHi = 31;

		if( assignHi > 31 || assignHi < 7 ||
		assignLo < 0 || assignLo > 24 )
		{
			log->fatal("FATAL %s %d: assignHi %d assignLo %d\n",
				__FILE__, __LINE__, assignHi, assignLo);
			rexutils->RexPPIRType( ty );
			// Sanity check assertions
			assert(assignHi <= 31);
			assert(assignHi >= 7);
			assert(assignLo >= 0);
			assert(assignLo <= 24);
		}


		// Declare new tmp
		snprintf( varname, VARNAMESIZE, "c%x", oldMachSt );
		e_con = vc_varExpr( vc, varname, vc_bv32Type( vc ) );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "c%x : BITVECTOR(32);\n", oldMachSt );
		#endif

		// Set appropriate bytes equal to the data
		vc_assertFormula( vc,
			vc_eqExpr( vc,
				vc_bvExtract( vc, e_con, assignHi, assignLo ),
				vc_bvExtract( vc,
					vc_bv32ConstExprFromInt( vc, c ),
					assignHi, assignLo ) ) );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "ASSERT( c%x[%u:%u] = 0hex%08x[%u:%u] );\n",
			oldMachSt, assignHi, assignLo, c, assignHi, assignLo );
		#endif

		// Set remaining pieces equal to MaSt[arrayBase]
		if( assignHi < 31 )
		{
			vc_assertFormula( vc,
				vc_eqExpr( vc,
					vc_bvExtract( vc, e_con, 31, assignHi+1 ),
					vc_bvExtract( vc,
						vc_readExpr( vc, e_oldMachSt, e_machStIdx),
						31, assignHi+1 ) ) );
			#ifdef USE_STP_FILE
			PRINTOBUF( obuf, outEnd, "ASSERT( c%x[31:%u] = MachSt%u[0hex%03x][31:%u] );\n",
				oldMachSt, assignHi + 1,
				useMachSt1 == true ? 1 : oldMachSt, arrayBase, assignHi + 1 );
			#endif
		}

		if( assignLo > 0 )
		{
			vc_assertFormula( vc,
				vc_eqExpr( vc,
					vc_bvExtract( vc, e_con, assignLo-1, 0 ),
					vc_bvExtract( vc,
						vc_readExpr( vc, e_oldMachSt, e_machStIdx),
						assignLo-1, 0 ) ) );
			#ifdef USE_STP_FILE
			PRINTOBUF( obuf, outEnd, "ASSERT( c%x[%u:0] = MachSt%u[0hex%03x][%u:0] );\n",
				oldMachSt, assignLo - 1,
				useMachSt1 == true ? 1 : oldMachSt, arrayBase, assignLo - 1 );
			#endif
		}
	}

	snprintf( varname, VARNAMESIZE, "MachSt%u", machSt );
	vc_writeExpr( vc, varname,
		e_oldMachSt, e_machStIdx,
		arrayOffset == 0 && numBytes == 4 ? vc_bv32ConstExprFromInt( vc, c ) : e_con );
	#ifdef USE_STP_FILE
	PRINTOBUF( obuf, outEnd, "MachSt%u : ARRAY BITVECTOR(12) OF BITVECTOR(32) "
		"= MachSt%u WITH [0hex%03x] := ",
		machSt, useMachSt1 == true ? 1 : oldMachSt, arrayBase );
	if( arrayOffset == 0 && numBytes == 4 )
	{
		// The common case. Emit a direct update with constant.
		PRINTOBUF( obuf, outEnd, "0hex%08x", c );
	}
	else
	{
		PRINTOBUF( obuf, outEnd, "c%x", oldMachSt );
	}
	PRINTOBUF( obuf, outEnd, "%s", ";\n" );
	STPPRINT( fs, out );
	#endif

	useMachSt1 = false;
	return 0;
}

/*!
Case 1:
	bb<b>t<tmp>_<n> : BITVECTOR(32);
	ASSERT( bb<b>t<tmp>_<n>[<hi>:<lo>] = bb<b>t<tmp>[<hi>:<lo>] );
	ASSERT( bb<b>t<tmp>_<n>[31:<hi+1>] = MachSt<n>[31:<hi+1>] );
	ASSERT( bb<b>t<tmp>_<n>[<lo-1>:0] = MachSt<n>[<lo-1>:0] );
	MachSt<n+1> : ARRAY BITVECTOR(12) OF BITVECTOR(32) = MachSt<n> WITH [0hex<ar_base>] := bb<b>t<tmp>_<n>
Case 2:
	MachSt<n+1> : ARRAY BITVECTOR(12) OF BITVECTOR(32) = MachSt<n> WITH [0hex<ar_base>] := bb<b>t<tmp>

*/
int RexSTP::EmitCstrPut_RdTmp32(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const Int putOffset,
	const IRTemp tmp,
	const IRType ty,
	RexOut *o_auxs )
{
	char		varname[VARNAMESIZE];
	Expr		e_oldMachSt;
	Expr		e_machSt;
	Expr		e_machStIdx;
	Expr		e_subtmp;
	Expr		e_tmp;
	unsigned int	oldMachSt;
	unsigned char	numBytes;
	unsigned char	arrayBase;
	unsigned char	arrayOffset;
	unsigned char	assignHi;
	unsigned char	assignLo;
	#ifdef USE_STP_FILE
	char *		obuf;
	#endif

	arrayOffset = putOffset % 4;
	arrayBase = putOffset - arrayOffset;

	// Only checks certain registers
	if( o_auxs != NULL &&
	( putOffset == 0 ) )	// EAX
	//putOffset == 4 ||	// ECX
	//putOffset == 8 ||	// EDX
	//putOffset == 12 ||	// EBX
	//putOffset == 24 ||	// ESI
	//putOffset == 28 ) )	// EDI
	{
		o_auxs->AddOut_PutOffset( arrayBase );
	}

	switch(ty)
	{
	case Ity_I32:	numBytes = 4;	break;
	case Ity_I16:	numBytes = 2;	break;
	case Ity_I8:	numBytes = 1;	break;
	default:
		numBytes = 4;
		log->fatal( "FATAL %s %d: Invalid type %d!", __FILE__, __LINE__, ty );
		//exit(1); // maybe its ok not to exit...
		return -1;
	}

	#ifdef USE_STP_FILE
	obuf = out;
	obuf[0] = 0x0;
	#endif
	oldMachSt = machSt++;

	assignLo = 8*arrayOffset;
	assignHi = assignLo + (8*numBytes) - 1;
	//assignHi = 31 - (8*arrayOffset);
	//assignLo = assignHi - (8*numBytes) + 1;

	e_oldMachSt = vc_lookupVarByName( "MachSt%u", useMachSt1 == true ? 1 : oldMachSt );
	e_machStIdx = vc_bvConstExprFromInt( vc, 12, arrayBase );
	e_tmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(tmp) );

	// If PUT-ing less than 32 bits, need to declare
	// a new 32 bits temp and stitch together the
	// src and current MaSt value.
	if( numBytes < 4 )
	{
		// Sanity check
		assert( assignHi <= 31 );
		assert( assignHi >= 7 );
		assert( assignLo >= 0 );
		assert( assignLo <= 24 );

		// Declare new tmp
		snprintf( varname, VARNAMESIZE, "f%ub%ut%u_%u", fid, bid, TMP(tmp), oldMachSt );
		e_subtmp  = vc_varExpr( vc, varname, vc_bv32Type( vc ) );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "f%ub%ut%u_%u : BITVECTOR(32);\n",
			fid, bid, TMP(tmp), oldMachSt );
		#endif

		// Set appropriate range equal to src tmp
		vc_assertFormula( vc, vc_eqExpr( vc,
			vc_bvExtract( vc, e_subtmp, assignHi, assignLo ),
			vc_bvExtract( vc, e_tmp, assignHi, assignLo ) ) );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u_%u[%u:%u] = f%ub%ut%u[%u:%u] );\n",
			fid, bid, TMP(tmp), oldMachSt, assignHi, assignLo,
			fid, bid, TMP(tmp), assignHi, assignLo );
		#endif

		// Set remaining pieces equal to MaSt[arrayBase]
		if( assignHi < 31 )
		{
			vc_assertFormula( vc, vc_eqExpr( vc,
				vc_bvExtract( vc, e_subtmp, 31, assignHi+1 ),
				vc_bvExtract( vc, vc_readExpr( vc, e_oldMachSt, e_machStIdx ), 31, assignHi+1 ) ) );
			#ifdef USE_STP_FILE
			PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u_%u[31:%u] = MachSt%u[0hex%03x][31:%u] );\n",
				fid, bid, TMP(tmp), oldMachSt, assignHi + 1,
				useMachSt1 == true ? 1 : oldMachSt, arrayBase, assignHi + 1 );
			#endif
		}

		if( assignLo > 0 )
		{
			vc_assertFormula( vc, vc_eqExpr( vc,
				vc_bvExtract( vc, e_subtmp, assignLo-1, 0 ),
				vc_bvExtract( vc, vc_readExpr( vc, e_oldMachSt, e_machStIdx ), assignLo-1, 0 ) ) );
			#ifdef USE_STP_FILE
			PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u_%u[%u:0] = MachSt%u[0hex%03x][%u:0] );\n",
				fid, bid, TMP(tmp), oldMachSt, assignLo - 1,
				useMachSt1 == true ? 1 : oldMachSt, arrayBase, assignLo - 1 );
			#endif
		}
	}

	snprintf( varname, VARNAMESIZE, "MachSt%u", machSt );
	vc_writeExpr( vc, varname,
		e_oldMachSt, e_machStIdx, numBytes < 4 ? e_subtmp : e_tmp );
	#ifdef USE_STP_FILE
	PRINTOBUF( obuf, outEnd, "MachSt%u : ARRAY BITVECTOR(12) OF BITVECTOR(32) "
		"= MachSt%u WITH [0hex%03x] := f%ub%ut%u",
		machSt, useMachSt1 == true ? 1 : oldMachSt, arrayBase, fid, bid, TMP(tmp) );
	if( numBytes < 4 )
		PRINTOBUF( obuf, outEnd, "_%u", oldMachSt );
	PRINTOBUF( obuf, outEnd, "%s", ";\n" );
	STPPRINT( fs, out );
	#endif
	useMachSt1 = false;

	return 0;
}

/*!
Emit constraint for Ist_Put.
*/
int RexSTP::EmitCstrPut(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	IRSB *const irsb,
	IRStmt *const stmt,
	RexOut *o_auxs )
{
	Int		putOffset;
	IRExpr *	rhs;
	IRTemp		rdtmp;
	IRType		ty;
	uint32_t	con;

	assert( irsb != NULL );
	assert( stmt != NULL );
	assert( stmt->tag == Ist_Put );

	putOffset = stmt->Ist.Put.offset;
	rhs = stmt->Ist.Put.data;

	switch( rhs->tag )
	{
	case Iex_RdTmp:
		rdtmp = rhs->Iex.RdTmp.tmp;
		ty = irsb->tyenv->types[rdtmp];
		EmitCstrPut_RdTmp32(
			#ifdef USE_STP_FILE
			fs,
			#endif
			vc, fid, bid, putOffset, rdtmp, ty,
			o_auxs );
		break;

	case Iex_Const:
		if( ConstToUint32(rhs->Iex.Const.con, &con) != 0 )
			return -1;
		EmitCstrPut_Const32(
			#ifdef USE_STP_FILE
			fs,
			#endif
			vc, putOffset,
			con, Ity_I32,
			o_auxs );
		break;
	default:
		log->fatal("FATAL %s %d: Invalid Put data tag %d!", __FILE__, __LINE__, rhs->tag );
		//exit(1);
		return -1;
	}

	return 0;
}

//=======================================
// WrTmp
//=======================================

/*!
ASSERT( f<fid>b<bid>t<tmp> = 0hex<c> );
*/
int RexSTP::EmitCstrWrTmp_Const32(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const uint32_t c )
{
	Expr	e_wrtmp;
	#ifdef USE_STP_FILE
	char *	obuf;
	obuf = out;
	obuf[0]	= 0x0;
	#endif

	e_wrtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	vc_assertFormula( vc, vc_eqExpr( vc,
		e_wrtmp,
		vc_bv32ConstExprFromInt( vc, c) ) );

	#ifdef USE_STP_FILE
	PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u = 0hex%08x );\n", fid, bid, TMP(wrtmp), c );
	STPPRINT( fs, out );
	#endif

	return 0;
}

/*!
ASSERT( f<fid>b<bid>t<wrtmp>[<hi>:<lo>] = MachSt<n>[0hex<arr_base>][<hi>:<lo>] );
*/
int RexSTP::EmitCstrWrTmp_Get32(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRType wrty,
	const Int getOffset )
{
	char		varname[VARNAMESIZE];
	Expr		e_wrtmp;
	Expr		e_machSt;
	Expr		e_machStAr;
	Expr		e_machStIdx;
	Expr		e_lhs;
	Expr		e_rhs;
	unsigned char	numBytes;
	unsigned char	arrayBase;
	unsigned char	arrayOffset;
	unsigned char	assignHi;
	unsigned char	assignLo;
	#ifdef USE_STP_FILE
	char *		obuf;
	#endif
	
	/*
	if( getOffset == 0 ||	// EAX
	getOffset == 4 ||	// ECX
	getOffset == 8 ||	// EDX
	getOffset == 12 ||	// EBX
	getOffset == 24 ||	// ESI
	getOffset == 28 )	// EDI
	{
		if( args != NULL )
		{
			args->AddIn( fid, bid, TMP(wrtmp) );
		}
		else if( vars != NULL )
		{
			vars->AddIn( fid, bid, TMP(wrtmp) );
		}
	}
	*/

	arrayOffset = getOffset % 4;
	arrayBase = getOffset - arrayOffset;

	switch( wrty )
	{
		case Ity_I32:	numBytes = 4;	break;
		case Ity_I16:	numBytes = 2;	break;
		case Ity_I8:	numBytes = 1;	break;
		default:
			numBytes = 4;
			log->fatal( "FATAL %s %d: Invalid type %d!", __FILE__, __LINE__, wrty );
			//exit(1); // maybe its ok not to exit...
			return -1;
	}

	#ifdef USE_STP_FILE
	obuf = out;
	obuf[0] = 0x0;
	#endif

	assignHi = 31 - ( 8 * arrayOffset );
	assignLo = assignHi - ( 8 * numBytes ) + 1;

	e_wrtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	e_machSt = vc_lookupVarByName( "MachSt%u", useMachSt1 == true ? 1 : machSt );
	e_machStIdx = vc_bvConstExprFromInt( vc, 12, arrayBase );
	e_machStAr = vc_readExpr( vc, e_machSt, e_machStIdx );

	if( numBytes < 4 )
	{
		e_lhs = vc_bvExtract( vc, e_wrtmp, assignHi, assignLo );
		e_rhs = vc_bvExtract( vc, e_machStAr, assignHi, assignLo );

		snprintf( varname, VARNAMESIZE, "f%ub%ut%ua", fid, bid, TMP(wrtmp) );
		Expr e_zero = vc_varExpr( vc, varname, vc_bv32Type( vc ) );
		vc_assertFormula( vc, vc_eqExpr( vc,
			e_zero,
			vc_bv32ConstExprFromInt( vc, 0 ) ) );

		if( assignHi < 31 )
		{
			vc_assertFormula( vc, vc_eqExpr( vc,
				vc_bvExtract( vc, e_wrtmp, 31, assignHi+1 ),
				vc_bvExtract( vc, e_zero, 31, assignHi+1 ) ) );
		}

		if( assignLo > 0 )
		{
			vc_assertFormula( vc, vc_eqExpr( vc,
				vc_bvExtract( vc, e_wrtmp, assignLo-1, 0 ),
				vc_bvExtract( vc, e_zero, assignLo-1, 0 ) ) );
		}
	}
	else
	{
		e_lhs = e_wrtmp;
		e_rhs = e_machStAr;
	}
	vc_assertFormula( vc, vc_eqExpr( vc, e_lhs, e_rhs) );

	#ifdef USE_STP_FILE
	if( numBytes < 4 )
	{
		PRINTOBUF( obuf, outEnd, "f%ub%ut%ua : BITVECTOR(32);\n", fid, bid, TMP(wrtmp) );
		PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%ua = 0hex00000000 );\n",
			fid, bid, TMP(wrtmp) );

		if( assignHi < 31 )
		{
			PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u[31:%u] = f%ub%ut%ua[31:%u] );\n",
				fid, bid, TMP(wrtmp), assignHi+1,
				fid, bid, TMP(wrtmp), assignHi+1 );
		}

		if( assignLo > 0 )
		{
			PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u[%u:0] = f%ub%ut%ua[%u:0] );\n",
				fid, bid, TMP(wrtmp), assignLo-1,
				fid, bid, TMP(wrtmp), assignLo-1 );
		}
	}

	PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u", fid, bid, TMP(wrtmp) );
	if( numBytes < 4 )
	{
		assert( assignHi <= 31 );
		assert( assignHi >= 7 );
		assert( assignLo >= 0 );
		assert( assignLo <= 24 );
		PRINTOBUF( obuf, outEnd, "[%u:%u]", assignHi, assignLo );
	}
	PRINTOBUF( obuf, outEnd, " = MachSt%u[0hex%03x]", useMachSt1 == true ? 1 : machSt, arrayBase );
	if( numBytes < 4 )
	{
		assert( assignHi <= 31 );
		assert( assignHi >= 7 );
		assert( assignLo >= 0 );
		assert( assignLo <= 24 );
		PRINTOBUF( obuf, outEnd, "[%u:%u]", assignHi, assignLo );
	}
	PRINTOBUF( obuf, outEnd, "%s", " );\n" );

	STPPRINT( fs, out );
	#endif

	return 0;
}

/*!
ASSERT( f<fid>b<bid>t<wrtmp>[31:0] = MemSt<memSt>[f<fid>b<bid>t<rtmp>] );
*/
int RexSTP::EmitCstrWrTmp_LoadRdTmp(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRType wrty,
	const IRTemp ldtmp )
{
	Expr	e_lhs;
	Expr	e_wrtmp;
	Expr	e_ldtmp;
	Expr	e_memSt;
	#ifdef USE_STP_FILE
	char *	obuf;
	obuf = out;
	obuf[0] = 0x0;
	#endif

	e_wrtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	e_ldtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(ldtmp) );
	e_memSt = vc_lookupVarByName( "MemSt%u", useMemSt1 == true ? 1 : memSt );

	#ifdef USE_STP_FILE
	PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u", fid, bid, TMP(wrtmp) );
	#endif

	e_lhs = e_wrtmp;
	if( wrty == Ity_I64 )
	{
		e_lhs = vc_bvExtract( vc, e_wrtmp, 31, 0);
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "%s", "[31:0]" );
		#endif
	}

	vc_assertFormula( vc, vc_eqExpr( vc, e_lhs, vc_readExpr( vc, e_memSt, e_ldtmp) ) );

	#ifdef USE_STP_FILE
	PRINTOBUF( obuf, outEnd, " = MemSt%u[f%ub%ut%u] );\n",
		useMemSt1 == true ? 1 : memSt, fid, bid, TMP(ldtmp) );
	STPPRINT( fs, out );
	#endif

	return 0;
}


/*!
ASSERT( f<fid>b<bid>t<wrtmp> = MemSt<memSt>[0hex<addr>] );
*/
int RexSTP::EmitCstrWrTmp_LoadConst(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const uint32_t addr )
{
	Expr	e_wrtmp;
	Expr	e_memSt;
	Expr	e_memStIdx;
	#ifdef USE_STP_FILE
	char *	obuf;
	#endif

	e_wrtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	e_memStIdx = vc_bv32ConstExprFromInt( vc, addr );
	e_memSt = vc_lookupVarByName( "MemSt%u", useMemSt1 == true ? 1 : memSt );

	vc_assertFormula( vc, vc_eqExpr( vc, e_wrtmp, vc_readExpr( vc, e_memSt, e_memStIdx) ) );

	#ifdef USE_STP_FILE
	obuf = out;
	obuf[0] = 0x0;
	PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u = MemSt%u[0hex%08x] );\n",
		fid, bid, TMP(wrtmp), useMemSt1 == true ? 1 : memSt, addr );

	STPPRINT( fs, out );
	#endif

	return 0;
}
/*!
case 1:	ASSERT( f<fid>b<bid>t<wrtmp> = f<fid>b<bid>t<rdtmp> );
case 2: ASSERT( f<fid>b<bid>t<wrtmp> <=> f<fid>b<bid>t<rdtmp> );
case 3: ASSERT( f<fid>b<bid>t<wrtmp>[31:0] = f<fid>b<bid><rdtmp>[31:0] );
*/
int RexSTP::EmitCstrWrTmp_RdTmp(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRType wrty,
	const IRTemp rdtmp )
{
	Expr	e_wrtmp;
	Expr	e_lhs;
	Expr	e_rhs;
	Expr	e_rdtmp;
	#ifdef USE_STP_FILE
	char *	obuf;
	obuf = out;
	obuf[0] = 0x0;
	#endif

	e_wrtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	e_lhs = e_wrtmp;

	#ifdef USE_STP_FILE
	PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u", fid, bid, TMP(wrtmp) );
	#endif
	switch( wrty )
	{
	case Ity_I1:
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "%s", " <=> " );
		#endif
		break;
	case Ity_I64:
		e_lhs = vc_bvExtract( vc, e_wrtmp, 31, 0 );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "%s", "[31:0]" );
		#endif
	default:
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "%s", " = " );
		#endif
		break;
	}

	e_rdtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(rdtmp) );
	e_rhs = e_rdtmp;

	#ifdef USE_STP_FILE
	PRINTOBUF( obuf, outEnd, "f%ub%ut%u", fid, bid, TMP(rdtmp) );
	#endif
	if( wrty == Ity_I64 )
	{
		e_rhs = vc_bvExtract( vc, e_rdtmp, 31, 0 );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "%s", "[31:0]" );
		#endif
	}

	vc_assertFormula( vc, wrty == Ity_I1 ?
		vc_iffExpr( vc, e_lhs, e_rhs ) :
		vc_eqExpr( vc, e_lhs, e_rhs ) );

	#ifdef USE_STP_FILE
	PRINTOBUF( obuf, outEnd, "%s", " );\n");
	STPPRINT( fs, out );
	#endif
	return 0;
}

/*!
Takes an input variable and creates a new
variable with the specified width.
*/
Expr RexSTP::EmitWidenedOperand(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	Expr e_in,
	const char* newvarname,
	#ifdef USE_STP_FILE
	const char* oldvarname,
	#endif
	const size_t oldnbits,
	const size_t newnbits,
	const bool signExtend
)
{
	assert( newnbits > oldnbits );
	Expr e_widen;


	#ifdef USE_STP_FILE
	char	*obuf;
	obuf = out;
	obuf[0] = 0x0;

	PRINTOBUF( obuf, outEnd, "%s : BITVECTOR(%d);\n", newvarname, newnbits );
	#endif

	// Declare the widened register.
	e_widen = vc_varExpr( vc, newvarname, vc_bvType( vc, newnbits ) );

	if( signExtend == true )
	{	// sign-extended
		e_widen = vc_bvSignExtend( vc, e_in, newnbits );

		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "ASSERT( %s = BVSX( %s, %d );\n",
			newvarname, oldnbits-1, oldnbits-1 );
		#endif

	}
	else
	{	// zero-extended

		// Copy the original bits over.
		vc_assertFormula( vc, vc_eqExpr( vc,
			vc_bvExtract( vc, e_widen, oldnbits-1, 0 ),
			vc_bvExtract( vc, e_in, oldnbits-1, 0 ) ) );

		// Set the other bits to 0
		vc_assertFormula( vc, vc_eqExpr( vc,
			vc_bvExtract( vc, e_widen, newnbits-1, oldnbits ),
			vc_bvExtract( vc,
				vc_bvConstExprFromInt( vc, newnbits, 0 ),
				newnbits-1, oldnbits ) ) );

		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "ASSERT( %s[%d:0] = %s[%d:0] );\n",
			newvarname, oldnbits-1, oldnbits-1 );

		PRINTOBUF( obuf, outEnd, "ASSERT( %s[%d:%d] = 0hex%0*x[%d:%d] );\n",
			newvarname,
			newnbits-1, oldnbits,
			(int)(newnbits/4), 0, newnbits-1, oldnbits );
		#endif
	}

	#ifdef USE_STP_FILE
	STPPRINT( fs, out );
	#endif

	return e_widen;
}


/*!
Extend operands to the width of the result before
multiplyinging. It is this extension of the operands
that gives the same effect as a "widening" multiply.
And what determines whether the effect of that
"widening" multiply acts like a signed or unsigned
multiply is whether the operands are sign-extended
or zero-extended.

Reference:
http://objectmix.com/verilog/189077-verilog2001-$signed-question.html
*/
int RexSTP::EmitCstrWrTmp_BinopMull(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	Expr e_wrtmp,
	const size_t nbits,
	#ifdef USE_STP_FILE
	const char* s_arg1,
	const char* s_arg2,
	#endif
	const size_t arg1bits,
	const size_t arg2bits,
	const bool signExtend )
{
	char	varname1[VARNAMESIZE];
	char	varname2[VARNAMESIZE];

	// Create two temporary registers of the same width
	// as the result to widen the operands.

	snprintf( varname1, VARNAMESIZE, "f%ub%ut%ua1", fid, bid, TMP(wrtmp) );
	Expr e_widen1 = EmitWidenedOperand(
		#ifdef USE_STP_FILE
		fs,
		#endif
		vc,
		e_wrtmp,
		varname1,
		#ifdef USE_STP_FILE
		s_arg1,
		#endif
		arg1bits,
		nbits,
		signExtend );

	snprintf( varname2, VARNAMESIZE, "f%ub%ut%ua2", fid, bid, TMP(wrtmp) );
	Expr e_widen2 = EmitWidenedOperand(
		#ifdef USE_STP_FILE
		fs,
		#endif
		vc,
		e_wrtmp,
		varname2,
		#ifdef USE_STP_FILE
		s_arg2,
		#endif
		arg2bits,
		nbits,
		signExtend );

	vc_assertFormula( vc, vc_eqExpr( vc,
		e_wrtmp,
		vc_bvMultExpr( vc, nbits, e_widen1, e_widen2 ) ) );

	#ifdef USE_STP_FILE
	char	*obuf;
	obuf = out;
	obuf[0] = 0x0;

	PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u = BVMUL(%d, %s, %s) );\n",
		fid, bid, TMP(wrtmp),
		nbits, varname1, varname2 );
	STPPRINT( fs, out );
	#endif

	return 0;
}


typedef Expr (*ShiftFunction) ( VC, int, Expr );

/*!
If the number of bits to shift is a temporary register, things get
very complicated. We need to guess the actual number of bits to shift
by comparing the temporary register with each possible value.
*/
int RexSTP::EmitCstrShiftByTmp(
	#ifdef USE_STP_FILE
	fstream &fs,
	char *s_wrtmp,
	char *s_arg1,
	char *s_arg2,
	#endif
	VC &vc,
	const IROp op,
	const Expr e_wrtmp,
	const Expr e_arg1,
	const Expr e_arg2,
	const size_t nbits )
{
	int		nshift;
	int 		nnibble;
	ShiftFunction	doshift = NULL;
	#ifdef USE_STP_FILE
	char	*obuf;
	obuf = out;
	obuf[0] = 0x0;
	#endif

	nnibble = (int)(nbits/4);

	vc_assertFormula( vc, vc_iteExpr( vc,
		vc_eqExpr( vc, e_arg2, vc_bvConstExprFromInt( vc, nbits, 0 ) ),
			vc_eqExpr( vc, e_wrtmp, e_arg1 ),
			vc_trueExpr( vc ) ) );
	#ifdef USE_STP_FILE
	PRINTOBUF( obuf, outEnd, "ASSERT(IF (%s = 0hex%0*x) "
		"THEN %s = %s "
		"ELSE TRUE ENDIF);\n",
		s_arg2, nnibble, 0,  s_wrtmp, s_arg1 );
	#endif

	// Select the shift function to use.
	switch( op )
	{
	case Iop_Shr8:
	case Iop_Shr16:
	case Iop_Shr32:
	case Iop_Shr64: doshift = vc_bvRightShiftExpr; break;
	case Iop_Shl8:
	case Iop_Shl16:
	case Iop_Shl32:
	case Iop_Shl64: doshift = vc_bvLeftShiftExpr; break;
	default: doshift = NULL;
	}
	assert( doshift != NULL );

	// Test temp register value
	for( nshift = 1; nshift < nbits; nshift++)
	{
		vc_assertFormula( vc, vc_iteExpr( vc,
			vc_eqExpr( vc, e_arg2, vc_bvConstExprFromInt( vc, nbits, nshift ) ),
			vc_eqExpr( vc,
				e_wrtmp,
				vc_bvExtract( vc,
					doshift( vc, nshift,  e_arg1 ),
					nbits-1, 0 ) ),
			vc_trueExpr( vc ) ) );
		#ifdef USE_STP_FILE
		STPPRINT( fs, out );
		obuf = out;
		obuf[0] = 0x0;
		PRINTOBUF( obuf, outEnd, "ASSERT(IF (%s = 0hex%0*x) "
			"THEN %s = (%s %s %u)[%d:0] "
			"ELSE TRUE ENDIF);\n",
			s_arg2, nnibble, nshift,
			s_wrtmp, s_arg1, GetOp(op), nshift, nbits-1 );
		#endif
	}

	#ifdef USE_STP_FILE
	STPPRINT( fs, out );
	#endif
	return 0;
}

/*!
Arithmetic shift version for case where the shift value is
a temporary register.
*/
int RexSTP::EmitCstrArithShiftByTmp(
	#ifdef USE_STP_FILE
	fstream &fs,
	char *s_wrtmp,
	char *s_arg1,
	char *s_arg2,
	#endif
	VC &vc,
	const IROp op,
	const Expr e_wrtmp,
	const Expr e_arg1,
	const Expr e_arg2,
	const size_t nbits )
{
	int		nshift;
	int 		nnibble;
	ShiftFunction	doshift = NULL;
	#ifdef USE_STP_FILE
	char	*obuf;
	obuf = out;
	obuf[0] = 0x0;
	#endif

	nnibble = (int)(nbits/4);

	vc_assertFormula( vc, vc_iteExpr( vc,
		vc_eqExpr( vc, e_arg2, vc_bvConstExprFromInt( vc, nbits, 0 ) ),
			vc_eqExpr( vc, e_wrtmp, e_arg1 ),
			vc_trueExpr( vc ) ) );
	#ifdef USE_STP_FILE
	PRINTOBUF( obuf, outEnd, "ASSERT(IF (%s = 0hex%0*x) "
		"THEN %s = %s "
		"ELSE TRUE ENDIF);\n",
		s_arg2, nnibble, 0,  s_wrtmp, s_arg1 );
	#endif

	// Select the shift function to use.
	// There's only right shift.
	switch( op )
	{
	case Iop_Sar8:
	case Iop_Sar16:
	case Iop_Sar32:
	case Iop_Sar64: doshift = vc_bvRightShiftExpr; break;
	default: doshift = NULL;
	}
	assert( doshift != NULL );

	// Test temp register value
	for( nshift = 1; nshift < nbits; nshift++)
	{
		vc_assertFormula( vc, vc_iteExpr( vc,
			vc_eqExpr( vc, e_arg2, vc_bvConstExprFromInt( vc, nbits, nshift ) ),
			vc_eqExpr( vc,
				e_wrtmp,
				vc_bvSignExtend( vc,
					doshift( vc, nshift,  e_arg1 ), nbits ) ),
			vc_trueExpr( vc ) ) );
		#ifdef USE_STP_FILE
		STPPRINT( fs, out );
		obuf = out;
		obuf[0] = 0x0;
		PRINTOBUF( obuf, outEnd, "ASSERT(IF (%s = 0hex%0*x) "
			"THEN %s = BVSX( (%s %s %u)[%d:0], %d ) "
			"ELSE TRUE ENDIF);\n",
			s_arg2, nnibble, nshift,
			s_wrtmp, s_arg1, GetOp(op), nshift, nbits-1, nbits );
		#endif
	}

	#ifdef USE_STP_FILE
	STPPRINT( fs, out );
	#endif
}
int RexSTP::EmitCstrWrTmp_BinopXX(
	#ifdef USE_STP_FILE
	fstream &fs,
	char *s_wrtmp,
	char *s_arg1,
	char *s_arg2,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IROp op,
	const Expr e_wrtmp,
	const Expr e_arg1,
	const Expr e_arg2,
	const size_t wrtmpwidth,
	const size_t arg1width,
	const size_t arg2width,
	const int arg1const,
	const int arg2const )
{
	Expr	e_lhs;
	Expr	e_rhs;
	int	lo_zero_bit = 0;
	int	hi_zero_bit = 64;
	bool	useiff;
	bool	doAssert = true;
	#ifdef USE_STP_FILE
	char	*obuf;
	obuf = out;
	obuf[0] = 0x0;
	#endif

	e_lhs = e_wrtmp;

	// Get the hi_zero_bit.
	switch( wrtmpwidth )
	{
	case 1: hi_zero_bit = 0; break;
	case 8:
	case 16:
	case 32: hi_zero_bit = 31; break;
	case 64: hi_zero_bit = 63; break;
	default: return -1;
	}

	// Now get lo_zero_bit. Let's hope we don't go
	// to more than 64-bits anytime soon.
	if( op == Iop_Add8 || op == Iop_Sub8 ||
	op == Iop_And8 || op == Iop_Or8 || op == Iop_Xor8 ||
	op == Iop_Shr8 || op == Iop_Sar8 || op == Iop_Shl8 )
	{ lo_zero_bit = 8; }
	else if( op == Iop_Add16 || op == Iop_Sub16 ||
	op == Iop_And16 || op == Iop_Or16 || op == Iop_Xor16 ||
	op == Iop_Shr16 || op == Iop_Sar16 || op == Iop_Shl16 )
	{ lo_zero_bit = 16; }
	else if( op == Iop_Add32 || op == Iop_Sub32 ||
	op == Iop_And32 || op == Iop_Or32 || op == Iop_Xor32 ||
	op == Iop_Shr32 || op == Iop_Sar32 || op == Iop_Shl32 )
	{ lo_zero_bit = 32; }
	else if( op == Iop_Add64 || op == Iop_Sub64 ||
	op == Iop_And64 || op == Iop_Or64 || op == Iop_Xor64 ||
	op == Iop_Shr64 || op == Iop_Sar64 || op == Iop_Shl64 )
	{ lo_zero_bit = 64; }

	if( hi_zero_bit > lo_zero_bit )
	{
		vc_assertFormula( vc, vc_eqExpr(vc,
					vc_bvExtract( vc, e_lhs, hi_zero_bit, lo_zero_bit ),
					vc_bvExtract( vc, vc_bvConstExprFromInt( vc, hi_zero_bit+1, 0 ) ,
						hi_zero_bit, lo_zero_bit ) ) );

#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "ASSERT( %s[%d:%d] = ( (0x%0*x)[%d:%d]) );\n",
				s_wrtmp, hi_zero_bit, lo_zero_bit,
				(int)((hi_zero_bit+1) / 4), 0, hi_zero_bit, lo_zero_bit );
#endif
	}

#ifdef USE_STP_FILE
	PRINTOBUF( obuf, outEnd, "ASSERT( %s", s_wrtmp );
#endif
	
	useiff = false;
	switch( op )
	{
		case Iop_Add8:
			e_lhs = vc_bvExtract( vc, e_wrtmp, 7, 0 );
			e_rhs = vc_bvPlusExpr( vc, 8,
					vc_bvExtract( vc, e_arg1, 7, 0 ),
				vc_bvExtract( vc, e_arg2, 7, 0 ) );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "[7:0] = %s(8,%s[7:0],%s[7:0]) );\n",
			GetOp(op), s_arg1, s_arg2 );
		#endif
		break;

	case Iop_Add16:
		e_lhs = vc_bvExtract( vc, e_wrtmp, 15, 0 );
		e_rhs = vc_bvPlusExpr( vc, 16,
				vc_bvExtract( vc, e_arg1, 15, 0 ),
				vc_bvExtract( vc, e_arg2, 15, 0 ) );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "[15:0] = %s(16,%s[15:0],%s[15:0]) );\n",
			GetOp(op), s_arg1, s_arg2 );
		#endif
		break;

	case Iop_Add32:
		e_rhs = vc_bv32PlusExpr( vc, e_arg1, e_arg2 );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, " = %s(32,%s,%s) );\n",
			GetOp(op), s_arg1, s_arg2 );
		#endif
		break;

	case Iop_Sub8:
		e_lhs = vc_bvExtract( vc, e_wrtmp, 7, 0 );
		e_rhs = vc_bvMinusExpr( vc, 8,
				vc_bvExtract( vc, e_arg1, 7, 0 ),
				vc_bvExtract( vc, e_arg2, 7, 0 ) );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "[7:0] = %s(8,%s[7:0],%s[7:0]) );\n",
			GetOp(op), s_arg1, s_arg2 );
		#endif
		break;

	case Iop_Sub16:
		e_lhs = vc_bvExtract( vc, e_wrtmp, 15, 0 );
		e_rhs = vc_bvMinusExpr( vc, 16,
				vc_bvExtract( vc, e_arg1, 15, 0 ),
				vc_bvExtract( vc, e_arg2, 15, 0 ) );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "[15:0] = %s(16,%s[15:0],%s[15:0]) );\n",
			GetOp(op), s_arg1, s_arg2 );
		#endif
		break;


	case Iop_Sub32:
		e_rhs = vc_bv32MinusExpr( vc, e_arg1, e_arg2 );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, " = %s(32,%s,%s) );\n",
			GetOp(op), s_arg1, s_arg2 );
		#endif
		break;

	case Iop_Mul8:
	case Iop_Mul16:
	case Iop_Mul32:
		e_rhs = vc_bv32MultExpr( vc, e_arg1, e_arg2 );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, " = %s(32,%s,%s) );\n",
			GetOp(op), s_arg1, s_arg2 );
		#endif
		break;

	case Iop_Mul64:
		e_rhs = vc_bvMultExpr( vc, 64, e_arg1, e_arg2 );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, " = %s(64,%s,%s) );\n",
			GetOp(op), s_arg1, s_arg2 );
		#endif
		break;

	// NOTE: Signed and unsigned multiplication are only different if you consider
	// "widening" multiplies, where the result is wider than the input operands.
	// There is no actual difference between signed and unsigned multiplication
	// if the multiplication is "non-widening", i.e. if the result is the same as
	// the two input operands.
	case Iop_MullS8:
	case Iop_MullS16:
	case Iop_MullS32:
		EmitCstrWrTmp_BinopMull(
			#ifdef USE_STP_FILE
			fs,
			#endif
			vc,
			fid, bid, wrtmp,
			e_wrtmp, wrtmpwidth,
			#ifdef USE_STP_FILE
			s_arg1, s_arg2,
			#endif
			arg1width, arg2width,
			true );
		doAssert = false;
		break;

	case Iop_MullU8:
	case Iop_MullU16:
	case Iop_MullU32:
		EmitCstrWrTmp_BinopMull(
			#ifdef USE_STP_FILE
			fs,
			#endif
			vc,
			fid, bid, wrtmp,
			e_wrtmp, wrtmpwidth,
			#ifdef USE_STP_FILE
			s_arg1, s_arg2,
			#endif
			arg1width, arg2width,
			false );
		doAssert = false;
		break;

	case Iop_Xor8:
		e_lhs = vc_bvExtract( vc, e_wrtmp, 7, 0 );
		e_rhs = vc_bvXorExpr( vc,
				vc_bvExtract( vc, e_arg1, 7, 0 ),
				vc_bvExtract( vc, e_arg2, 7, 0 ) );

		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "[7:0] = (%s[7:0] %s %s[7:0]) );\n",
			GetOp(op), s_arg1, s_arg2 );
		#endif
		break;

	case Iop_Xor16:
		e_lhs = vc_bvExtract( vc, e_wrtmp, 15, 0 );
		e_rhs = vc_bvXorExpr( vc,
				vc_bvExtract( vc, e_arg1, 15, 0 ),
				vc_bvExtract( vc, e_arg2, 15, 0 ) );

		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "[15:0] = (%s[15:0] %s %s[15:0]) );\n",
			GetOp(op), s_arg1, s_arg2 );
		#endif

	case Iop_Xor32:
	case Iop_Xor64:
		e_rhs = vc_bvXorExpr( vc, e_arg1, e_arg2 );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, " = %s(%s,%s) );\n",
			GetOp(op), s_arg1, s_arg2 );
		#endif
		break;

	case Iop_And8:
		e_lhs = vc_bvExtract( vc, e_wrtmp, 7, 0 );
		e_rhs = vc_bvAndExpr( vc,
				vc_bvExtract( vc, e_arg1, 7, 0),
				vc_bvExtract( vc, e_arg2, 7, 0) );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "[7:0] = (%s[7:0] %s %s[7:0]) );\n",
			s_arg1, GetOp(op), s_arg2 );
		#endif
		break;

	case Iop_And16:
		e_lhs = vc_bvExtract( vc, e_wrtmp, 15, 0 );
		e_rhs = vc_bvAndExpr( vc,
				vc_bvExtract( vc, e_arg1, 15, 0),
				vc_bvExtract( vc, e_arg2, 15, 0) );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "[15:0] = (%s[15:0] %s %s[15:0]) );\n",
			s_arg1, GetOp(op), s_arg2 );
		#endif
		break;

	case Iop_And32:
	case Iop_And64:
		e_rhs = vc_bvAndExpr( vc, e_arg1, e_arg2 );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, " = (%s %s %s) );\n",
			s_arg1, GetOp(op), s_arg2 );
		#endif
		break;

	case Iop_Or8:
		e_lhs = vc_bvExtract( vc, e_wrtmp, 7, 0 );
		e_rhs = vc_bvOrExpr( vc,
				vc_bvExtract( vc, e_arg1, 7, 0),
				vc_bvExtract( vc, e_arg2, 7, 0) );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "[7:0] = (%s[7:0] %s %s[7:0]) );\n",
			s_arg1, GetOp(op), s_arg2 );
		#endif
		break;

	case Iop_Or16:
		e_lhs = vc_bvExtract( vc, e_wrtmp, 15, 0 );
		e_rhs = vc_bvOrExpr( vc,
				vc_bvExtract( vc, e_arg1, 15, 0),
				vc_bvExtract( vc, e_arg2, 15, 0) );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "[15:0] = (%s[15:0] %s %s[15:0]) );\n",
			s_arg1, GetOp(op), s_arg2 );
		#endif
		break;

	case Iop_Or32:
	case Iop_Or64:
		e_rhs = vc_bvOrExpr( vc, e_arg1, e_arg2 );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, " = (%s %s %s) );\n",
			s_arg1, GetOp(op), s_arg2 );
		#endif
		break;

	case Iop_Shl8:
	case Iop_Shl16:
	case Iop_Shl32:
		if( arg2const != -1 )
		{
			e_rhs = vc_bvExtract( vc,
					vc_bvLeftShiftExpr( vc,
						arg2const,
						e_arg1 ),
					31, 0 );
			#ifdef USE_STP_FILE
			PRINTOBUF( obuf, outEnd, " = (%s %s %u)[31:0] );\n",
				s_arg1, GetOp(op), arg2const );
			#endif
		}
		else
		{
			EmitCstrShiftByTmp(
				#ifdef USE_STP_FILE
				fs, s_wrtmp, s_arg1, s_arg2,
				#endif
				vc, op, e_wrtmp, e_arg1, e_arg2, 32 );
			doAssert = false;
		}
		break;

	case Iop_Shl64:
		if( arg2const != - 1)
		{
			e_rhs = vc_bvExtract( vc,
					vc_bvLeftShiftExpr( vc,
						arg2const,
						e_arg1 ),
					63, 0 );
			#ifdef USE_STP_FILE
			PRINTOBUF( obuf, outEnd, " = (%s %s %u)[63:0] );\n",
				s_arg1, GetOp(op), arg2const );
			#endif
		}
		else
		{
			EmitCstrShiftByTmp(
				#ifdef USE_STP_FILE
				fs, s_wrtmp, s_arg1, s_arg2,
				#endif
				vc, op, e_wrtmp, e_arg1, e_arg2, 64 );
			doAssert = false;
		}
		break;

	case Iop_Sar8:
	case Iop_Sar16:
	case Iop_Sar32:
		if( arg2const != -1 )
		{
			e_rhs = vc_bvSignExtend( vc,
					vc_bvRightShiftExpr( vc,
						arg2const,
						e_arg1 ),
					32 );
			#ifdef USE_STP_FILE
			PRINTOBUF( obuf, outEnd, " = BVSX( (%s %s %u [31:0]), 32);\n",
				s_arg1, GetOp(op), arg2const );
			#endif
		}
		else
		{
			EmitCstrArithShiftByTmp(
				#ifdef USE_STP_FILE
				fs, s_wrtmp, s_arg1, s_arg2,
				#endif
				vc, op, e_wrtmp, e_arg1, e_arg2, 32 );
			doAssert = false;
		}

	case Iop_Sar64:
		if( arg2const != -1 )
		{
			e_rhs = vc_bvSignExtend( vc,
					vc_bvRightShiftExpr( vc,
						arg2const,
						e_arg1 ),
					64 );
			#ifdef USE_STP_FILE
			PRINTOBUF( obuf, outEnd, " = BVSX( (%s %s %u [63:0]), 64);\n",
				s_arg1, GetOp(op), arg2const );
			#endif
		}
		else
		{
			EmitCstrArithShiftByTmp(
				#ifdef USE_STP_FILE
				fs, s_wrtmp, s_arg1, s_arg2,
				#endif
				vc, op, e_wrtmp, e_arg1, e_arg2, 64 );
			doAssert = false;
		}

	case Iop_Shr8:
	case Iop_Shr16:
	case Iop_Shr32:
		if( arg2const != -1 )
		{	// Number of bits to shift is known (not a temporary register).
			e_rhs = vc_bvExtract( vc,
					vc_bvRightShiftExpr( vc,
						arg2const,
						e_arg1 ),
					31, 0 );
			#ifdef USE_STP_FILE
			PRINTOBUF( obuf, outEnd, " = (%s %s %u)[31:0] );\n",
				s_arg1, GetOp(op), arg2const );
			#endif
		}
		else
		{	// Don't know number of bits to shift.
			EmitCstrShiftByTmp(
				#ifdef USE_STP_FILE
				fs, s_wrtmp, s_arg1, s_arg2,
				#endif
				vc, op, e_wrtmp, e_arg1, e_arg2, 32 );
			doAssert = false;
		}
		break;

	case Iop_Shr64:
		if( arg2const != -1 )
		{	// Number of bits to shift is known (not a temporary register).

			e_rhs = vc_bvExtract( vc,
					vc_bvRightShiftExpr( vc,
						arg2const,
						e_arg1 ),
						63, 0 );
			#ifdef USE_STP_FILE
			PRINTOBUF( obuf, outEnd, " = (%s %s %u)[63:0] );\n",
				s_arg1, GetOp(op), arg2const );
			#endif
		}
		else
		{	// Don't know number of bits to shift.
			EmitCstrShiftByTmp(
				#ifdef USE_STP_FILE
				fs, s_wrtmp, s_arg1, s_arg2,
				#endif
				vc, op, e_wrtmp, e_arg1, e_arg2, 64 );
			doAssert = false;
		}
		break;

	case Iop_CmpLT32S:
	case Iop_CmpLT64S:
		e_rhs = vc_sbvLtExpr( vc, e_arg1, e_arg2 );
		useiff = true;
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, " <=> %s(%s,%s) );\n",
			GetOp(op), s_arg1, s_arg2 );
		#endif
		break;

	case Iop_CmpLT32U:
	case Iop_CmpLT64U:
		e_rhs = vc_bvLtExpr( vc, e_arg1, e_arg2 );
		useiff = true;
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, " <=> %s(%s,%s) );\n",
			GetOp(op), s_arg1, s_arg2 );
		#endif
		break;

	case Iop_CmpLE32S:
	case Iop_CmpLE64S:
		e_rhs = vc_sbvLeExpr( vc, e_arg1, e_arg2 );
		useiff = true;
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, " <=> %s(%s,%s) );\n",
			GetOp(op), s_arg1, s_arg2 );
		#endif
		break;

	case Iop_CmpLE32U:
	case Iop_CmpLE64U:
		e_rhs = vc_bvLeExpr( vc, e_arg1, e_arg2 );
		useiff = true;
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, " <=> %s(%s,%s) );\n",
			GetOp(op), s_arg1, s_arg2 );
		#endif
		break;

	case Iop_CmpEQ8:
		e_rhs = vc_eqExpr( vc,
				vc_bvExtract( vc, e_arg1, 7, 0 ),
				vc_bvExtract( vc, e_arg2, 7, 0 ) );
		useiff = true;
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, " <=> (%s[7:0] %s %s[7:0]) );\n",
			s_arg1, GetOp(op), s_arg2 );
		#endif
		break;

	case Iop_CmpEQ16:
		e_rhs = vc_eqExpr( vc,
				vc_bvExtract( vc, e_arg1, 15, 0 ),
				vc_bvExtract( vc, e_arg2, 15, 0 ) );
		useiff = true;
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, " <=> (%s[15:0] %s %s[15:0]) );\n",
			s_arg1, GetOp(op), s_arg2 );
		#endif
		break;

	case Iop_CmpEQ32:
	case Iop_CmpEQ64:
		e_rhs = vc_eqExpr( vc, e_arg1, e_arg2 );
		useiff = true;
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, " <=> (%s %s %s) );\n",
			s_arg1, GetOp(op), s_arg2 );
		#endif
		break;

	case Iop_CmpNE8:
		e_rhs = vc_notExpr( vc, vc_eqExpr( vc,
				vc_bvExtract( vc, e_arg1, 7, 0 ),
				vc_bvExtract( vc, e_arg2, 7, 0 ) ) );
		useiff = true;
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, " <=> (NOT (%s[7:0] %s %s) ) );\n",
			s_arg1, GetOp(op), s_arg2 );
		#endif
		break;

	case Iop_CmpNE16:
		e_rhs = vc_notExpr( vc, vc_eqExpr( vc,
				vc_bvExtract( vc, e_arg1, 15, 0 ),
				vc_bvExtract( vc, e_arg2, 15, 0 ) ) );
		useiff = true;
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, " <=> (NOT (%s[15:0] %s %s) ) );\n",
			s_arg1, GetOp(op), s_arg2 );
		#endif
		break;

	case Iop_CmpNE32:
	case Iop_CmpNE64:
		e_rhs = vc_notExpr( vc, vc_eqExpr( vc, e_arg1, e_arg2 ) );
		useiff = true;
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, " <=> (NOT (%s %s %s) ) );\n",
			s_arg1, GetOp(op), s_arg2 );
		#endif
		break;

	default:
		log->fatal( "FATAL %s %d: Unsupported op %u!\n", __FILE__, __LINE__, op );
		rexutils->RexPPIROp( op );
		//exit(1);
		return -1;
	}

	if( doAssert == true )
	{
		vc_assertFormula( vc, useiff == true ?
			vc_iffExpr( vc, e_lhs, e_rhs ) :
			vc_eqExpr( vc, e_lhs, e_rhs ) );
		#ifdef USE_STP_FILE
		STPPRINT( fs, out );
		#endif
	
	}

	return 0;
}

int RexSTP::EmitCstrWrTmp_BinopTmpTmp(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRTemp arg1rdtmp,
	const IRTemp arg2rdtmp,
	const IROp op,
	const size_t wrtmpwidth,
	const size_t arg1width,
	const size_t arg2width )
{
	Expr	e_wrtmp;
	Expr	e_arg1rdtmp;
	Expr	e_arg2rdtmp;
	char	s_wrtmp[64];
	char	s_arg1rdtmp[64];
	char	s_arg2rdtmp[64];

	snprintf( s_wrtmp, 64, "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	snprintf( s_arg1rdtmp, 64, "f%ub%ut%u", fid, bid, TMP(arg1rdtmp) );
	snprintf( s_arg2rdtmp, 64, "f%ub%ut%u", fid, bid, TMP(arg2rdtmp) );
	e_wrtmp = vc_lookupVarByName( s_wrtmp );
	e_arg1rdtmp = vc_lookupVarByName( s_arg1rdtmp );
	e_arg2rdtmp = vc_lookupVarByName( s_arg2rdtmp );

	return EmitCstrWrTmp_BinopXX(
		#ifdef USE_STP_FILE
		fs,
		s_wrtmp, s_arg1rdtmp, s_arg2rdtmp,
		#endif
		vc,
		fid, bid, wrtmp,
		op,
		e_wrtmp, e_arg1rdtmp, e_arg2rdtmp,
		wrtmpwidth, arg1width, arg2width );
}

int RexSTP::EmitCstrWrTmp_BinopTmpConst(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRTemp arg1rdtmp,
	const uint32_t arg2const,
	const IROp op,
	const size_t wrtmpwidth,
	const size_t arg1width,
	const size_t arg2width )
{
	Expr	e_wrtmp;
	Expr	e_arg1rdtmp;
	Expr	e_arg2const;
	char	s_wrtmp[64];
	char	s_arg1rdtmp[64];
	#ifdef USE_STP_FILE
	char	s_arg2const[64];
	#endif

	snprintf( s_wrtmp, 64, "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	snprintf( s_arg1rdtmp, 64, "f%ub%ut%u", fid, bid, TMP(arg1rdtmp) );
	#ifdef USE_STP_FILE
	snprintf( s_arg2const, 64, "0hex%08x", arg2const );
	#endif
	e_wrtmp = vc_lookupVarByName( s_wrtmp );
	e_arg1rdtmp = vc_lookupVarByName( s_arg1rdtmp );
	e_arg2const = vc_bv32ConstExprFromInt( vc, arg2const );

	return EmitCstrWrTmp_BinopXX(
		#ifdef USE_STP_FILE
		fs,
		s_wrtmp, s_arg1rdtmp, s_arg2const,
		#endif
		vc,
		fid, bid, wrtmp,
		op,
		e_wrtmp, e_arg1rdtmp, e_arg2const,
		wrtmpwidth, arg1width, arg2width,
		-1, (int) arg2const );
}

int RexSTP::EmitCstrWrTmp_BinopConstTmp(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRTemp arg1const,
	const uint32_t arg2rdtmp,
	const IROp op,
	const size_t wrtmpwidth,
	const size_t arg1width,
	const size_t arg2width )
{
	Expr	e_wrtmp;
	Expr	e_arg1const;
	Expr	e_arg2rdtmp;
	char	s_wrtmp[64];
	#ifdef USE_STP_FILE
	char	s_arg1const[64];
	#endif
	char	s_arg2rdtmp[64];

	snprintf( s_wrtmp, 64, "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	#ifdef USE_STP_FILE
	snprintf( s_arg1const, 64, "0hex%08x", arg1const );
	#endif
	snprintf( s_arg2rdtmp, 64, "f%ub%ut%u", fid, bid, TMP(arg2rdtmp) );
	e_wrtmp = vc_lookupVarByName( s_wrtmp );
	e_arg1const = vc_bv32ConstExprFromInt( vc, arg1const );
	e_arg2rdtmp = vc_lookupVarByName( s_arg2rdtmp );

	return EmitCstrWrTmp_BinopXX(
		#ifdef USE_STP_FILE
		fs,
		s_wrtmp, s_arg1const, s_arg2rdtmp,
		#endif
		vc,
		fid, bid, wrtmp,
		op,
		e_wrtmp, e_arg1const, e_arg2rdtmp,
		wrtmpwidth, arg1width, arg2width,
		(int) arg1const, -1 );
}

int RexSTP::EmitCstrWrTmp_BinopConstConst(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRTemp arg1const,
	const uint32_t arg2const,
	const IROp op,
	const size_t wrtmpwidth,
	const size_t arg1width,
	const size_t arg2width )
{
	Expr	e_wrtmp;
	Expr	e_arg1const;
	Expr	e_arg2const;
	char	s_wrtmp[64];
	#ifdef USE_STP_FILE
	char	s_arg1const[64];
	char	s_arg2const[64];
	#endif

	snprintf( s_wrtmp, 64, "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	#ifdef USE_STP_FILE
	snprintf( s_arg1const, 64, "0hex%08x", arg1const );
	snprintf( s_arg2const, 64, "0hex%08x", arg2const );
	#endif
	e_wrtmp = vc_lookupVarByName( s_wrtmp );
	e_arg1const = vc_bv32ConstExprFromInt( vc, arg1const );
	e_arg2const = vc_bv32ConstExprFromInt( vc, arg2const );

	return EmitCstrWrTmp_BinopXX(
		#ifdef USE_STP_FILE
		fs,
		s_wrtmp, s_arg1const, s_arg2const,
		#endif
		vc,
		fid, bid, wrtmp,
		op,
		e_wrtmp, e_arg1const, e_arg2const,
		wrtmpwidth, arg1width, arg2width,
		(int) arg1const, (int) arg2const);
}

int RexSTP::EmitCstrWrTmp_Binop(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	IRSB *const irsb,
	const unsigned int fid,
	const unsigned int bid,
	const IRType wrty,
	const IRTemp wrtmp,
	const IRExpr *const rhs )
{
	IRExpr *	arg1;
	IRExpr *	arg2;
	IROp		op;
	IRTemp		arg1rdtmp;
	IRTemp		arg2rdtmp;
	uint32_t	arg1const;
	uint32_t	arg2const;

	assert( rhs != NULL );
	assert( rhs->tag == Iex_Binop );

	arg1 = rhs->Iex.Binop.arg1;
	arg2 = rhs->Iex.Binop.arg2;
	op = rhs->Iex.Binop.op;

	switch( arg1->tag )
	{
	case Iex_RdTmp:
		arg1rdtmp = arg1->Iex.RdTmp.tmp;
		switch( arg2->tag )
		{
		case Iex_RdTmp:
			arg2rdtmp = arg2->Iex.RdTmp.tmp;
			EmitCstrWrTmp_BinopTmpTmp(
				#ifdef USE_STP_FILE
				fs,
				#endif
				vc, fid, bid, wrtmp,
				arg1rdtmp, arg2rdtmp, op,
				GetTypeWidth(wrty),
				GetTypeWidth(irsb->tyenv->types[arg1rdtmp]),
				GetTypeWidth(irsb->tyenv->types[arg2rdtmp]) );
			break;
		case Iex_Const:
			if( ConstToUint32( arg2->Iex.Const.con, &arg2const ) != 0 )
				return -1;
			EmitCstrWrTmp_BinopTmpConst(
				#ifdef USE_STP_FILE
				fs,
				#endif
				vc, fid, bid, wrtmp,
				arg1rdtmp, arg2const, op,
				GetTypeWidth(wrty),
				GetTypeWidth(irsb->tyenv->types[arg1rdtmp]),
				32 ); // Support only 32-bit constants
			break;
		default:
			log->fatal( "FATAL %s %d: Invalid arg2 tag %u!\n", __FILE__, __LINE__, arg2->tag );
			//exit(1);
			return -1;
		}
		
		break;
	case Iex_Const:
		if( ConstToUint32( arg1->Iex.Const.con, &arg1const ) != 0 )
			return -1;
		switch( arg2->tag )
		{
		case Iex_RdTmp:
			arg2rdtmp = arg2->Iex.RdTmp.tmp;
			EmitCstrWrTmp_BinopConstTmp(
				#ifdef USE_STP_FILE
				fs,
				#endif
				vc, fid, bid, wrtmp,
				arg1const, arg2rdtmp, op,
				GetTypeWidth(wrty),
				32,
				GetTypeWidth(irsb->tyenv->types[arg2rdtmp]) );
			break;
		case Iex_Const:
			if( ConstToUint32( arg2->Iex.Const.con, &arg2const ) != 0 )
				return -1;
			EmitCstrWrTmp_BinopConstConst(
				#ifdef USE_STP_FILE
				fs,
				#endif
				vc, fid, bid, wrtmp,
				arg1const, arg2const, op,
				GetTypeWidth(wrty), 32, 32 );
			break;
		default:
			log->fatal( "FATAL %s %d: Invalid arg2 tag %u!\n", __FILE__, __LINE__, arg2->tag );
			//exit(1);
			return -1;
		}

		break;
	default:
		log->fatal( "FATAL %s %d: Invalid arg1 tag %u!\n", __FILE__, __LINE__, arg1->tag );
		//exit(1);
		return -1;
	}

	return 0;
}	

int RexSTP::EmitCstrWrTmp_UnopNot32(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRTemp rdtmp )
{
	Expr	e_lhs;
	Expr	e_rhs;
	#ifdef USE_STP_FILE
	char	*obuf;
	#endif

	e_lhs = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	e_rhs = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(rdtmp) );
	vc_assertFormula( vc, vc_eqExpr( vc, e_lhs, e_rhs ) );

	#ifdef USE_STP_FILE
	obuf = out;
	obuf[0] = 0x0;

	PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u = ~ f%ub%ut%u );\n",
		fid, bid, TMP(wrtmp), fid, bid, TMP(rdtmp) );

	STPPRINT( fs, out );
	#endif
	return 0;
}

int RexSTP::EmitCstrWrTmp_UnopNot1(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRTemp rdtmp )
{
	Expr	e_lhs;
	Expr	e_rhs;
	#ifdef USE_STP_FILE
	char	*obuf;
	#endif

	e_lhs = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	assert( getType(e_lhs) == BOOLEAN_TYPE );
	e_rhs = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(rdtmp) );
	assert( getType(e_rhs) == BOOLEAN_TYPE );
	vc_assertFormula( vc, vc_iffExpr( vc, e_lhs, vc_notExpr( vc, e_rhs ) ) );

	#ifdef USE_STP_FILE
	obuf = out;
	obuf[0] = 0x0;

	PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u <=> NOT f%ub%ut%u );\n",
		fid, bid, TMP(wrtmp), fid, bid, TMP(rdtmp) );

	STPPRINT( fs, out );
	#endif
	return 0;
}

int RexSTP::EmitCstrWrTmp_Unop1Uto32(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRTemp rdtmp )
{
	Expr	e_wrtmp;
	Expr	e_rdtmp;
	#ifdef USE_STP_FILE
	char	*obuf;
	#endif

	e_wrtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	e_rdtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(rdtmp) );
	vc_assertFormula( vc, vc_iteExpr( vc,
		e_rdtmp,
		vc_eqExpr( vc, e_wrtmp, vc_bv32ConstExprFromInt( vc, 1 ) ),
		vc_eqExpr( vc, e_wrtmp, vc_bv32ConstExprFromInt( vc, 0 ) ) ) );

	#ifdef USE_STP_FILE
	obuf = out;
	obuf[0] = 0x0;

	PRINTOBUF( obuf, outEnd, "ASSERT( IF f%ub%ut%u THEN f%ub%ut%u = 0hex00000001 "
		"ELSE f%ub%ut%u = 0hex00000000 ENDIF );\n",
		fid, bid, TMP(rdtmp), fid, bid, TMP(wrtmp), fid, bid, TMP(wrtmp) );

	STPPRINT( fs, out );
	#endif
	return 0;
}

int RexSTP::EmitCstrWrTmp_Unop32to1(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRTemp rdtmp )
{
	Expr	e_wrtmp;
	Expr	e_rdtmp;
	#ifdef USE_STP_FILE
	char	*obuf;
	#endif

	e_wrtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	assert( getType( e_wrtmp ) == BOOLEAN_TYPE );
	e_rdtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(rdtmp) );

	// vc_assertFormula( vc, vc_iffExpr( vc, e_wrtmp, vc_trueExpr( vc ) ) );
	/*
	vc_assertFormula( vc, vc_iteExpr( vc,
		vc_eqExpr( vc, vc_bvExtract( vc, e_rdtmp, 0, 0), vc_bvConstExprFromInt( vc, 1, 1 ) ),
		vc_iffExpr( vc, e_wrtmp, vc_trueExpr( vc ) ),
		vc_iffExpr( vc, e_wrtmp, vc_falseExpr( vc ) ) ) );
	*/
	vc_assertFormula( vc, vc_iteExpr( vc,
		vc_eqExpr( vc, vc_bvExtract( vc, e_rdtmp, 0, 0), vc_bvConstExprFromInt( vc, 1, 1 ) ),
		e_wrtmp,
		vc_notExpr( vc, e_wrtmp ) ) );

	#ifdef USE_STP_FILE
	obuf = out;
	obuf[0] = 0x0;

	PRINTOBUF( obuf, outEnd, "ASSERT( IF f%ub%ut%u[0:0] = 0bin1 "
		"THEN f%ub%ut%u "
		"ELSE NOT f%ub%ut%u ENDIF );\n",
		fid, bid, TMP(rdtmp), fid, bid, TMP(wrtmp), fid, bid, TMP(wrtmp) );

	STPPRINT( fs, out );
	#endif
	return 0;
}

int RexSTP::EmitCstrWrTmp_Unop8Sto32(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRTemp rdtmp )
{
	Expr	e_wrtmp;
	Expr	e_rdtmp;
	Expr	e_wrtmp_upper;
	Expr	e_wrtmp_lower;
	#ifdef USE_STP_FILE
	char	*obuf;
	#endif

	e_wrtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	//assert( getType( e_wrtmp ) == BOOLEAN_TYPE );
	e_rdtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(rdtmp) );

	e_wrtmp_upper = vc_bvExtract( vc, e_wrtmp, 31, 8 ); 
	e_wrtmp_lower = vc_bvExtract( vc, e_wrtmp, 7, 0 );

	vc_assertFormula( vc, vc_iteExpr( vc,
		vc_eqExpr( vc, vc_bvExtract( vc, e_rdtmp, 7, 7 ), vc_bvConstExprFromInt( vc, 1, 1) ),
		vc_eqExpr( vc, e_wrtmp_upper, vc_bvConstExprFromInt( vc, 24, 0xffffff) ),
		vc_eqExpr( vc, e_wrtmp_upper, vc_bvConstExprFromInt( vc, 24, 0x000000) ) ) );

	vc_assertFormula( vc, vc_eqExpr( vc,
		e_wrtmp_lower,
		vc_bvExtract( vc, e_rdtmp, 7, 0 ) ) );

	#ifdef USE_STP_FILE
	obuf = out;
	obuf[0] = 0x0;

	PRINTOBUF( obuf, outEnd, "ASSERT( IF f%ub%ut%u[7:7] = 0bin1 "
		"THEN f%ub%ut%u[31:8] = 0hexFFFFFF "
		"ELSE f%ub%ut%u[31:8] = 0hex000000 ENDIF );\n",
		fid, bid, TMP(rdtmp), fid, bid, TMP(wrtmp), fid, bid, TMP(wrtmp) );

	PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u[7:0] = f%ub%ut%u[7:0] );\n",
		fid, bid, TMP(wrtmp), fid, bid, TMP(rdtmp) );

	STPPRINT( fs, out );
	#endif
	return 0;
}

int RexSTP::EmitCstrWrTmp_Unop8Uto32(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRTemp rdtmp )
{
	Expr	e_wrtmp;
	Expr	e_rdtmp;
	#ifdef USE_STP_FILE
	char	*obuf;
	#endif

	e_wrtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	e_rdtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(rdtmp) );
	vc_assertFormula( vc, vc_eqExpr( vc, e_wrtmp, e_rdtmp ) );

	#ifdef USE_STP_FILE
	obuf = out;
	obuf[0] = 0x0;

	PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u = f%ub%ut%u );\n",
		fid, bid, TMP(wrtmp), fid, bid, TMP(rdtmp) );

	STPPRINT( fs, out );
	#endif
	return 0;
}

int RexSTP::EmitCstrWrTmp_Unop16Uto32(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRTemp rdtmp )
{
	Expr	e_wrtmp;
	Expr	e_rdtmp;
	#ifdef USE_STP_FILE
	char * obuf;
	#endif

	e_wrtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	e_rdtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(rdtmp) );
	vc_assertFormula( vc, vc_eqExpr( vc, e_wrtmp, e_rdtmp ) );

	#ifdef USE_STP_FILE
	obuf = out;
	obuf[0] = 0x0;

	PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u = f%ub%ut%u );\n",
		fid, bid, TMP(wrtmp), fid, bid, TMP(rdtmp) );

	STPPRINT( fs, out );
	#endif
	return 0;
}

int RexSTP::EmitCstrWrTmp_Unop32to8Tmp(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRTemp rdtmp )
{
	Expr	e_wrtmp;
	Expr	e_rdtmp;
	Expr	e_lhs;
	Expr	e_rhs;
	#ifdef USE_STP_FILE
	char	*obuf;
	#endif

	e_wrtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	e_rdtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(rdtmp) );
	e_lhs = vc_bvExtract( vc, e_wrtmp, 7, 0 );
	e_rhs = vc_bvExtract( vc, e_rdtmp, 7, 0 );
	vc_assertFormula( vc, vc_eqExpr( vc, e_lhs, e_rhs ) );

	#ifdef USE_STP_FILE
	obuf = out;
	obuf[0] = 0x0;

	PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u[7:0] = f%ub%ut%u[7:0] );\n",
		fid, bid, TMP(wrtmp), fid, bid, TMP(rdtmp) );

	STPPRINT( fs, out );
	#endif
	return 0;
}

int RexSTP::EmitCstrWrTmp_Unop32to8Const(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const uint32_t  argconst )
{
	Expr	e_wrtmp;
	#ifdef USE_STP_FILE
	char	*obuf;
	#endif

	e_wrtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	vc_assertFormula( vc, vc_eqExpr( vc,
		vc_bvExtract( vc, e_wrtmp, 7, 0 ),
		vc_bvConstExprFromInt( vc, 8, argconst ) ) );

	#ifdef USE_STP_FILE
	obuf = out;
	obuf[0] = 0x0;

	PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u[7:0] = 0hex%02x );\n",
		fid, bid, TMP(wrtmp), argconst );

	STPPRINT( fs, out );
	#endif
	return 0;
}

int RexSTP::EmitCstrWrTmp_Unop64loto32(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRTemp rdtmp )
{
	Expr	e_wrtmp;
	Expr	e_rdtmp;
	#ifdef USE_STP_FILE
	char	*obuf;
	#endif

	e_wrtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	e_rdtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(rdtmp) );
	vc_assertFormula( vc, vc_eqExpr( vc,
		e_wrtmp,
		vc_bvExtract( vc, e_rdtmp, 31, 0 ) ) );

	#ifdef USE_STP_FILE
	obuf = out;
	obuf[0] = 0x0;

	PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u = f%ub%ut%u[31:0] );\n",
		fid, bid, TMP(wrtmp), fid, bid, TMP(rdtmp) );

	STPPRINT( fs, out );
	#endif
	return 0;
}

int RexSTP::EmitCstrWrTmp_Unop64HIto32(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRTemp rdtmp )
{
	Expr	e_wrtmp;
	Expr	e_rdtmp;
	#ifdef USE_STP_FILE
	char	*obuf;
	#endif

	e_wrtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	e_rdtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(rdtmp) );
	vc_assertFormula( vc, vc_eqExpr( vc,
		e_wrtmp,
		vc_bvExtract( vc, e_rdtmp, 63, 32 ) ) );

	#ifdef USE_STP_FILE
	obuf = out;
	obuf[0] = 0x0;

	PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u = f%ub%ut%u[63:32] );\n",
		fid, bid, TMP(wrtmp), fid, bid, TMP(rdtmp) );

	STPPRINT( fs, out );
	#endif
	return 0;
}

int RexSTP::EmitCstrWrTmp_Unop1Uto8(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRTemp rdtmp )
{
	// ASSERT ( IF CVxtY THEN CVxtZ = 0hex00000001 ELSE CVxtZ = 0hex00000000
	//              ENDIF);

	Expr	e_wrtmp;
	Expr	e_rdtmp;
	Expr	e_wrtmp_lower;
	Expr	e_wrtmp_upper;
	#ifdef USE_STP_FILE
	char * obuf;
	#endif

	e_wrtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	e_rdtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(rdtmp) );
	assert( getType(e_rdtmp) == BOOLEAN_TYPE );

	e_wrtmp_lower = vc_bvExtract( vc, e_wrtmp, 7, 0 );
	e_wrtmp_upper = vc_bvExtract( vc, e_wrtmp, 31, 8 );

	vc_assertFormula( vc, vc_eqExpr( vc,
			e_wrtmp_upper,
			vc_bvConstExprFromInt(vc, 24, 0x000000 ) ) );
	vc_assertFormula( vc, vc_iteExpr( vc,
			e_rdtmp,
			vc_eqExpr( vc, e_wrtmp_lower, vc_bvConstExprFromInt( vc, 8, 0x01 ) ),
			vc_eqExpr( vc, e_wrtmp_lower, vc_bvConstExprFromInt( vc, 8, 0x00 ) ) ) );

	#ifdef USE_STP_FILE
	obuf = out;
	obuf[0] = 0x0;

	PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u[31:8] = (0hex00000000)[31:8] );\n",
		fid, bid, TMP(wrtmp) );
	PRINTOBUF( obuf, outEnd, "ASSERT(IF f%ub%ut%u "
		"THEN f%ub%ut%u[7:0] = 0hex01 "
		"ELSE f%ub%ut%u[7:0] = 0hex00 ENDIF);\n",
		fid, bid, TMP(rdtmp), fid, bid, TMP(wrtmp), fid, bid, TMP(wrtmp) );

	STPPRINT( fs, out );
	#endif
	return 0;
}

int RexSTP::EmitCstrWrTmp_Unop32to16Const(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const uint32_t argconst )
{
	Expr	e_wrtmp;
	#ifdef USE_STP_FILE
	char	*obuf;
	#endif

	e_wrtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	vc_assertFormula( vc, vc_eqExpr( vc,
		vc_bvExtract( vc, e_wrtmp, 15, 0 ),
		vc_bvConstExprFromInt( vc, 16, argconst ) ) );

	#ifdef USE_STP_FILE
	obuf = out;
	obuf[0] = 0x0;

	PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u[15:0] = 0hex%04x );\n",
		fid, bid, TMP(wrtmp), argconst );

	STPPRINT( fs, out );
	#endif
	return 0;
}


int RexSTP::EmitCstrWrTmp_Unop32to16Tmp(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRTemp rdtmp )
{
	Expr	e_wrtmp;
	Expr	e_rdtmp;
	#ifdef USE_STP_FILE
	char	*obuf;
	#endif

	e_wrtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	e_rdtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(rdtmp) );
	vc_assertFormula( vc, vc_eqExpr( vc,
		vc_bvExtract( vc, e_wrtmp, 15, 0 ),
		vc_bvExtract( vc, e_rdtmp, 15, 0 ) ) );

	#ifdef USE_STP_FILE
	obuf = out;
	obuf[0] = 0x0;

	PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u[15:0] = f%ub%ut%u[15:0] );\n",
		fid, bid, TMP(wrtmp), fid, bid, TMP(rdtmp) );

	STPPRINT( fs, out );
	#endif
	return 0;
}

int RexSTP::EmitCstrWrTmp_Unop(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRExpr *const rhs	)
{
	IRExpr *	arg;
	IROp		op;
	IRTemp		rdtmp;
	uint32_t	con;
	int		retval = 0;

	assert( rhs != NULL );
	assert( rhs->tag == Iex_Unop );

	arg = rhs->Iex.Unop.arg;
	op = rhs->Iex.Unop.op;

	switch( op )
	{
	case Iop_Not32:
		#ifdef USE_STP_FILE
		retval = EmitCstrWrTmp_UnopNot32( fs, vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
		#else
		retval = EmitCstrWrTmp_UnopNot32( vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
		#endif
		break;

	case Iop_Not1:
		#ifdef USE_STP_FILE
		retval = EmitCstrWrTmp_UnopNot1( fs, vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
		#else
		retval = EmitCstrWrTmp_UnopNot1( vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
		#endif
		break;

	case Iop_1Uto32:
		#ifdef USE_STP_FILE
		retval = EmitCstrWrTmp_Unop1Uto32( fs, vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
		#else
		retval = EmitCstrWrTmp_Unop1Uto32( vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
		#endif
		break;

	case Iop_32to1:
		#ifdef USE_STP_FILE
		retval = EmitCstrWrTmp_Unop32to1( fs, vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
		#else
		retval = EmitCstrWrTmp_Unop32to1( vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
		#endif
		break;

	case Iop_8Sto32:
		#ifdef USE_STP_FILE
		retval = EmitCstrWrTmp_Unop8Sto32( fs, vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
		#else
		retval = EmitCstrWrTmp_Unop8Sto32( vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
		#endif
		break;

	case Iop_8Uto32:
		#ifdef USE_STP_FILE
		retval = EmitCstrWrTmp_Unop8Uto32( fs, vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
		#else
		retval = EmitCstrWrTmp_Unop8Uto32( vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
		#endif
		break;

	case Iop_16Uto32:
		#ifdef USE_STP_FILE
		retval = EmitCstrWrTmp_Unop16Uto32( fs, vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
		#else
		retval = EmitCstrWrTmp_Unop16Uto32( vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
		#endif
		break;

	case Iop_32to8:

		switch( arg->tag )
		{
		case Iex_RdTmp:
			#ifdef USE_STP_FILE
			retval = EmitCstrWrTmp_Unop32to8Tmp( fs, vc, fid, bid, wrtmp,
				arg->Iex.RdTmp.tmp );
			#else
			retval = EmitCstrWrTmp_Unop32to8Tmp( vc, fid, bid, wrtmp,
				arg->Iex.RdTmp.tmp );
			#endif
			break;

		case Iex_Const:
			if( ConstToUint32(arg->Iex.Const.con, &con ) != 0 )
				return -1;
			#ifdef USE_STP_FILE
			retval = EmitCstrWrTmp_Unop32to8Const( fs, vc, fid, bid, wrtmp, con );
			#else
			retval = EmitCstrWrTmp_Unop32to8Const( vc, fid, bid, wrtmp, con );
			#endif
			break;

		default:
			log->fatal( "FATAL %s %d: Unhandled arg type in 32to8\n", __FILE__, __LINE__ );
			//exit(1);
			return -1;
		}
		break;

	case Iop_64to32:
		#ifdef USE_STP_FILE
		retval = EmitCstrWrTmp_Unop64loto32( fs, vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
		#else
		retval = EmitCstrWrTmp_Unop64loto32( vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
		#endif
		break;

	case Iop_64HIto32:
		#ifdef USE_STP_FILE
		retval = EmitCstrWrTmp_Unop64HIto32( fs, vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
		#else
		retval = EmitCstrWrTmp_Unop64HIto32( vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
		#endif
		break;

	case Iop_1Uto8:
		#ifdef USE_STP_FILE
		retval = EmitCstrWrTmp_Unop1Uto8( fs, vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
		#else
		retval = EmitCstrWrTmp_Unop1Uto8( vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
		#endif
		break;

	case Iop_32to16:
		switch( arg->tag )
		{
		case Iex_RdTmp:
			#ifdef USE_STP_FILE
			retval = EmitCstrWrTmp_Unop32to16Tmp( fs, vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
			#else
			retval = EmitCstrWrTmp_Unop32to16Tmp( vc, fid, bid, wrtmp, arg->Iex.RdTmp.tmp );
			#endif
			break;

		case (Iex_Const):
			if( ConstToUint32(arg->Iex.Const.con, &con) != 0 )
				return -1;
			#ifdef USE_STP_FILE
			retval = EmitCstrWrTmp_Unop32to16Const( fs, vc, fid, bid, wrtmp, con );
			#else
			retval = EmitCstrWrTmp_Unop32to16Const( vc, fid, bid, wrtmp, con );
			#endif
			break;

		default:
			log->fatal( "FATAL %s %d: Unhandled arg type %u in 32to16!",
				__FILE__, __LINE__, arg->tag );
			//exit(1);
			return -1;
		}
		break;

	default:
		log->fatal( "FATAL %s %d: Unhandled Unop 0x%x!\n", __FILE__, __LINE__, op );
		rexutils->RexPPIRExpr( rhs );
		//exit(1);
		return -1;
	}

	return retval;
}

// Just set to 0 for unsupported operations.
int RexSTP::EmitCstrWrTmp_Unsupported(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp wrtmp,
	const IRType wrty )
{
	Expr	e_wrtmp;
	#ifdef USE_STP_FILE
	char	*obuf;
	#endif

	size_t nbits = GetTypeWidth(wrty);

	switch( wrty )
	{
	case Ity_I1: nbits = 1; break;
	case Ity_I8:
	case Ity_I16:
	case Ity_I32:
	case Ity_F32: nbits = 32; break;
	case Ity_F64:
	case Ity_I64: nbits = 64; break;
	default:
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "FATAL %s %d: unhandled IRType!\n", __FILE__, __LINE__ );
		#endif
		return -1;
	}

	e_wrtmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(wrtmp) );
	vc_assertFormula( vc, vc_eqExpr( vc,
		e_wrtmp,
		vc_bvConstExprFromInt( vc, nbits, 0 ) ) );

	#ifdef USE_STP_FILE
	obuf = out;
	obuf[0] = 0x0;

	PRINTOBUF( obuf, outEnd, "ASSERT( f%ub%ut%u = 0hex%0*x );\n",
		fid, bid, TMP(wrtmp), (nbits == 1 ? 1 : (int)(nbits/4)), 0 );

	STPPRINT( fs, out );
	#endif

	return 0;
}

size_t RexSTP::GetTypeWidth( const IRType ty )
{
	#ifdef USE_STP_FILE
	char		*obuf;
	obuf = out;
	obuf[0] = 0x0;
	#endif
	switch( ty )
	{
	case Ity_I1: return 1;
	case Ity_I8: return 8;
	case Ity_I16: return 16;
	case Ity_I32: return 32;
	case Ity_F32: return 32;
	case Ity_F64: return 64;
	case Ity_I64: return 64;
	default:
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "FATAL %s %d: unhandled IRType!\n", __FILE__, __LINE__ );
		#endif
		return -1;
	}
	// TODO: FIX RETURN VALUES
	return 0;
}


/*!
Emit constraints for Ist_WrTmp.
*/
int RexSTP::EmitCstrWrTmp(
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
	RexParam *vars )
{
	int retval = -1;
	IRExpr *	rhs;
	IRTemp		wrtmp;
	IRType		wrty;
	IRExpr *	loadaddr;
	uint32_t	con;

	assert( irsb != NULL );
	assert( stmt != NULL );
	assert( stmt->tag == Ist_WrTmp );

	wrtmp = stmt->Ist.WrTmp.tmp;
	wrty = irsb->tyenv->types[wrtmp];
	rhs = stmt->Ist.WrTmp.data;

	switch( rhs->tag )
	{
	case Iex_Const:
		if( ConstToUint32(rhs->Iex.Const.con, &con) != 0 )
			return -1;
		retval = EmitCstrWrTmp_Const32(
			#ifdef USE_STP_FILE
			fs,
			#endif
			vc, fid, bid, wrtmp, con  );
		break;
	case Iex_Get:
		//if( rhs->Iex.Get.offset == 0x14 && (*ebpStmt) == NULL )
		//{
		//	*ebpStmt = stmt;
		//}

		retval = EmitCstrWrTmp_Get32(
			#ifdef USE_STP_FILE
			fs,
			#endif
			vc, fid, bid, wrtmp, wrty,
			rhs->Iex.Get.offset );
		break;
	case Iex_Load:
		loadaddr = rhs->Iex.Load.addr;
		switch( loadaddr->tag )
		{
		case Iex_RdTmp:
			retval = EmitCstrWrTmp_LoadRdTmp(
				#ifdef USE_STP_FILE
				fs,
				#endif
				vc, fid, bid, wrtmp, wrty,
				loadaddr->Iex.RdTmp.tmp);
			break;
		case Iex_Const:
			if( ConstToUint32(loadaddr->Iex.Const.con, &con) != 0 )
				return -1;	
			retval = EmitCstrWrTmp_LoadConst(
				#ifdef USE_STP_FILE
				fs,
				#endif
				vc, fid, bid, wrtmp, con );
			break;
		default:
			log->fatal("FATAL %s %d: Invalid WrTmp load addr!", __FILE__, __LINE__ );
			//exit(1);	
			return -1;
		}

		break;
	case Iex_RdTmp:
		#ifdef USE_STP_FILE
		retval = EmitCstrWrTmp_RdTmp( fs, vc, fid, bid, wrtmp, wrty, rhs->Iex.RdTmp.tmp );
		#else
		retval = EmitCstrWrTmp_RdTmp( vc, fid, bid, wrtmp, wrty, rhs->Iex.RdTmp.tmp );	
		#endif
		break;
	case Iex_Binop:
		#ifdef USE_STP_FILE
		retval = EmitCstrWrTmp_Binop( fs, vc, irsb, fid, bid, wrty, wrtmp, rhs );
		#else
		retval = EmitCstrWrTmp_Binop( vc, irsb, fid, bid, wrty, wrtmp, rhs );
		#endif
		break;
	case Iex_Unop:
		#ifdef USE_STP_FILE
		retval = EmitCstrWrTmp_Unop( fs, vc, fid, bid, wrtmp, rhs );
		#else
		retval = EmitCstrWrTmp_Unop( vc, fid, bid, wrtmp, rhs );
		#endif
		break;
	case Iex_CCall:
		log->debug( "Not supporting CCall\n" );
		#ifdef USE_STP_FILE
		retval = EmitCstrWrTmp_Unsupported( fs, vc, fid, bid, wrtmp, wrty );	
		#else
		retval = EmitCstrWrTmp_Unsupported( vc, fid, bid, wrtmp, wrty );	
		#endif
		break;
	default:
		log->fatal("FATAL %s %d: Invalid WrTmp data tag %d!\n", __FILE__, __LINE__, rhs->tag );
		rexutils->RexPPIRStmt( stmt );
		log->debug("\n");
		//exit(1);
		return -1;
	}

	return retval;
}

//=======================================
// Ist_Store
//=======================================
int RexSTP::EmitCstrStore_TmpTmpITE(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp lhstmp,
	const IRTemp rhstmp,
	RexOut *outputs,
	const int ebpoffset )
{
	char		varname[VARNAMESIZE];
	Expr		e_oldMemSt;
	Expr		e_lhstmp;
	Expr		e_rhstmp;
	unsigned int	oldMemSt;
	#ifdef USE_STP_FILE
	char		*obuf;

	obuf = out;
	obuf[0] = 0x0;
	#endif
	oldMemSt = memSt++;

	//if( outputs != NULL )
	//{
	//	outputs->AddOut_StoreTmp( memSt, fid, bid, TMP(lhstmp), oldMemSt, ebpoffset );
	//}

	e_lhstmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(lhstmp) );
	e_rhstmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(rhstmp) );
	e_oldMemSt = vc_lookupVarByName( "MemSt%u", useMemSt1 == true ? 1 : oldMemSt );

	snprintf( varname, VARNAMESIZE, "MemSt%u", memSt );
	vc_writeExpr( vc, varname,
		e_oldMemSt, 
		e_lhstmp,
		e_rhstmp );

	#ifdef USE_STP_FILE
	PRINTOBUF( obuf, outEnd, "MemSt%u : ARRAY BITVECTOR(32) OF BITVECTOR(32) = MemSt%u WITH [f%ub%ut%u] := f%ub%ut%u;\n",
		memSt, useMemSt1 == true ? 1 : oldMemSt,
		fid, bid, TMP(lhstmp),
		fid, bid, TMP(rhstmp) );

	STPPRINT( fs, out );
	#endif

	useMemSt1 = false;

	return 0;
}

int RexSTP::EmitCstrStore_TmpConstITE(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp lhstmp,
	const uint32_t rhsconst,
	RexOut *outputs,
	const int ebpoffset )
{
	char		varname[VARNAMESIZE];
	Expr		e_oldMemSt;
	Expr		e_lhstmp;
	Expr		e_rhsconst;
	unsigned int	oldMemSt;
	#ifdef USE_STP_FILE
	char		*obuf;

	obuf = out;
	obuf[0] = 0x0;
	#endif
	oldMemSt = memSt++;

	//if( outputs != NULL )
	//{
	//	outputs->AddOut_StoreTmp( memSt, fid, bid, TMP(lhstmp), oldMemSt, ebpoffset );
	//}

	e_lhstmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(lhstmp) );
	e_rhsconst = vc_bv32ConstExprFromInt( vc, rhsconst );
	e_oldMemSt = vc_lookupVarByName( "MemSt%u", useMemSt1 == true ? 1 : oldMemSt );

	snprintf( varname, VARNAMESIZE, "MemSt%u", memSt );
	vc_writeExpr( vc, varname,
		e_oldMemSt, 
		e_lhstmp,
		e_rhsconst );

	#ifdef USE_STP_FILE
	PRINTOBUF( obuf, outEnd, "MemSt%u : ARRAY BITVECTOR(32) OF BITVECTOR(32) = MemSt%u WITH [f%ub%ut%u] := 0hex%08x;\n",
		memSt, useMemSt1 == true ? 1 : oldMemSt,
		fid, bid, TMP(lhstmp),
		rhsconst );

	STPPRINT( fs, out );
	#endif

	useMemSt1 = false;

	return 0;
}

int RexSTP::EmitCstrStore_ConstTmpITE(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const uint32_t lhsconst,
	const IRTemp rhstmp,
	RexOut *outputs )
{
	char		varname[VARNAMESIZE];
	Expr		e_oldMemSt;
	Expr		e_lhsconst;
	Expr		e_rhstmp;
	unsigned int	oldMemSt;
	#ifdef USE_STP_FILE
	char		*obuf;

	obuf = out;
	obuf[0] = 0x0;
	#endif
	oldMemSt = memSt++;

	//if( outputs != NULL )
	//{
	//	outputs->AddOut_StoreConst( memSt, lhsconst, oldMemSt );
	//}

	e_lhsconst = vc_bv32ConstExprFromInt( vc, lhsconst );
	e_rhstmp = vc_lookupVarByName( "f%ub%ut%u", fid, bid, TMP(rhstmp) );
	e_oldMemSt = vc_lookupVarByName( "MemSt%u", useMemSt1 == true ? 1 : oldMemSt );

	snprintf( varname, VARNAMESIZE, "MemSt%u", memSt );
	vc_writeExpr( vc, varname,
		e_oldMemSt, 
		e_lhsconst,
		e_rhstmp );

	#ifdef USE_STP_FILE
	PRINTOBUF( obuf, outEnd, "MemSt%u : ARRAY BITVECTOR(32) OF BITVECTOR(32) = MemSt%u WITH [0hex%08x] := f%ub%ut%u;\n",
		memSt, useMemSt1 == true ? 1 : oldMemSt,
		lhsconst,
		fid, bid, TMP(rhstmp) );

	STPPRINT( fs, out );
	#endif

	useMemSt1 = false;
	return 0;
}

int RexSTP::EmitCstrStore_ConstConstITE(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const uint32_t lhsconst,
	const uint32_t rhsconst,
	RexOut *outputs )
{
	char		varname[VARNAMESIZE];
	Expr		e_oldMemSt;
	Expr		e_lhsconst;
	Expr		e_rhsconst;
	unsigned int	oldMemSt;
	#ifdef USE_STP_FILE
	char 		*obuf;

	obuf = out;
	obuf[0] = 0x0;
	#endif
	oldMemSt = memSt++;

	//if( outputs != NULL )
	//{
	//	outputs->AddOut_StoreConst( memSt, lhsconst, oldMemSt );
	//}

	e_lhsconst = vc_bv32ConstExprFromInt( vc, lhsconst );
	e_rhsconst = vc_bv32ConstExprFromInt( vc, rhsconst );
	e_oldMemSt = vc_lookupVarByName( "MemSt%u", useMemSt1 == true ? 1 : oldMemSt );

	snprintf( varname, VARNAMESIZE, "MemSt%u", memSt );
	vc_writeExpr( vc, varname,
		e_oldMemSt, 
		e_lhsconst,
		e_rhsconst );

	#ifdef USE_STP_FILE
	PRINTOBUF( obuf, outEnd, "MemSt%u : ARRAY BITVECTOR(32) OF BITVECTOR(32) = MemSt%u WITH [0hex%08x] := 0hex%08x;\n",
		memSt, useMemSt1 == true ? 1 : oldMemSt,
		lhsconst,
		rhsconst );

	STPPRINT( fs, out );
	#endif

	useMemSt1 = false;
	return 0;
}

/*!
Emit constraints for Ist_Store.
*/
int RexSTP::EmitCstrStore(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	IRSB *const irsb,
	IRStmt *const stmt,
	RexOut *outputs,
	const int ebpoffset )
{
	IRExpr *	lhs;
	IRExpr *	rhs;
	IRTemp		ltmp;
	IRTemp		rtmp;
	uint32_t	con1;
	uint32_t	con2;
	int		retval = 0;

	assert( irsb != NULL );
	assert( stmt != NULL );
	assert( stmt->tag == Ist_Store );

	lhs = stmt->Ist.Store.addr;
	rhs = stmt->Ist.Store.data;

	switch( lhs->tag )
	{
	case Iex_RdTmp:
		switch( rhs->tag )
		{
		case Iex_RdTmp:
			retval = EmitCstrStore_TmpTmpITE(
				#ifdef USE_STP_FILE
				fs,
				#endif
				vc, fid, bid, lhs->Iex.RdTmp.tmp,
				rhs->Iex.RdTmp.tmp,
				outputs,
				ebpoffset );
			break;
		case Iex_Const:
			if(ConstToUint32(rhs->Iex.Const.con, &con1) != 0 )
				return -1;
			retval = EmitCstrStore_TmpConstITE(
				#ifdef USE_STP_FILE
				fs,
				#endif
				vc, fid, bid, lhs->Iex.RdTmp.tmp,
				con1, outputs, ebpoffset );
			break;
		default:
			log->fatal( "FATAL %s %d: Unhandled store rhs tag (addr is tmp)!\n",
				__FILE__, __LINE__ );
			rexutils->RexPPIRStmt( stmt );
			//exit(1);
			return -1;
		}
		break;

	case Iex_Const:
		switch( rhs->tag )
		{
		case Iex_RdTmp:
			if( ConstToUint32(lhs->Iex.Const.con, &con1) != 0 )
				return -1;
			retval = EmitCstrStore_ConstTmpITE(
				#ifdef USE_STP_FILE
				fs,
				#endif
				vc, fid, bid, con1,
				rhs->Iex.RdTmp.tmp, outputs );
			break;
		case Iex_Const:
			if( ConstToUint32(lhs->Iex.Const.con, &con1) != 0 )
				return -1;
			if( ConstToUint32(rhs->Iex.Const.con, &con2) != 0 )
				return -1;

			retval = EmitCstrStore_ConstConstITE(
				#ifdef USE_STP_FILE
				fs,
				#endif
				vc, con1, con2, outputs );
			break;
		default:
			log->fatal( "FATAL %s %d: Unhandled store rhs tag (addr is tmp)!\n",
				__FILE__, __LINE__ );
			rexutils->RexPPIRStmt( stmt );
			//exit(1);
			return -1;
		}
		break;
	default:
		log->fatal( "FATAL %s %d: Unhandled addr tag in store statement!\n", __FILE__, __LINE__ );	
		rexutils->RexPPIRStmt( stmt );
		//exit(1);
		return -1;
	}

	return retval;
}

//=======================================
// Tmp declarations
//=======================================

/*!
Declare tmps.
*/
int RexSTP::EmitDeclareTmp(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	const unsigned int fid,
	const unsigned int bid,
	const IRTemp tmp,
	const IRType ty )
{
	char	varname[VARNAMESIZE];
	Expr	e_var;
	#ifdef USE_STP_FILE
	char	*obuf;

	obuf = out;
	obuf[0] = 0x0;
	#endif

	snprintf(varname, VARNAMESIZE, "f%ub%ut%u", fid, bid, TMP(tmp) );

	switch( ty )
	{
	case Ity_I1:
		e_var = vc_varExpr( vc, varname, vc_boolType( vc ) );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "f%ub%ut%u : BOOLEAN;\n", fid, bid, TMP(tmp) );
		#endif
		break;
	case Ity_I8:
	case Ity_I16:
	case Ity_I32:
	case Ity_F32:
	case Ity_F64:
		e_var = vc_varExpr( vc, varname, vc_bv32Type( vc ) );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "f%ub%ut%u : BITVECTOR(32);\n", fid, bid, TMP(tmp) );
		#endif
		break;
	case Ity_I64:
		e_var = vc_varExpr( vc, varname, vc_bvType( vc, 64 ) );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "f%ub%ut%u : BITVECTOR(64);\n", fid, bid, TMP(tmp) );
		#endif
		break;
	default:
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd, "FATAL %s %d: unhandled IRType!\n", __FILE__, __LINE__ );
		#endif
		//exit(1);
		return -1;
	}

	#ifdef USE_STP_FILE
	STPPRINT( fs, out );
	#endif
	return 0;
}

//=======================================

int RexSTP::EmitIRStmt(
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
	const int ebpoffset )
{
	assert( irsb );
	assert( stmt );

	int retval = 0;

	switch( stmt->tag )
	{
	case Ist_NoOp:
		break;
	case Ist_IMark:
		//printf( "\n------ IMark(0x%08x, %d) ------\n",
		//	stmt->Ist.IMark.addr,
		//	stmt->Ist.IMark.len );
		break;
	case Ist_AbiHint:
		break;
	case Ist_Put:
		#ifdef USE_STP_FILE
		retval = EmitCstrPut( fs, vc, fid, bid, irsb, stmt, outs );
		#else
		retval = EmitCstrPut( vc, fid, bid, irsb, stmt, outs );
		#endif
		break;
	case Ist_PutI:
		break;
	case Ist_WrTmp:
		#ifdef USE_STP_FILE
		retval = EmitCstrWrTmp( fs, vc, fid, bid, irsb, stmt, /*ebpStmt, */args, vars );
		#else
		retval = EmitCstrWrTmp( vc, fid, bid, irsb, stmt, /*ebpStmt, */args, vars );
		#endif
		break;
	case Ist_Store:
		#ifdef USE_STP_FILE
		retval = EmitCstrStore( fs, vc, fid, bid, irsb, stmt, outs, ebpoffset );
		#else
		retval = EmitCstrStore( vc, fid, bid, irsb, stmt, outs, ebpoffset );
		#endif
		break;
	case Ist_Dirty:
		break;
	case Ist_MBE:
		break;
	case Ist_Exit:
		break;
	default:
		log->fatal("Unk stmt tag!\n");
		return -1;
	}

	return retval;
}
/*
Two possibilities:
1. First IRSB for func contains function prologue to set up EBP.
2. Non-first IRSB for func which just gets EBP.

Returns stmt for ebp. We have to assume that the first 6 instructions are as follows.

0   ------ IMark(0x80483e6, 1) ------
1   Ist_WrTmp	t0 = GET:I32(20)
2   Ist_WrTmp	t12 = GET:I32(16)
3   Ist_WrTmp	t11 = Sub32(t12,0x4:I32)
4   Ist_Put	PUT(16) = t11
5   Ist_Store	STle(t11) = t0
6   ------ IMark(0x80483e7, 2) ------
7   Ist_Put	PUT(20) = t11
.
.
.

*/
IRStmt* RexSTP::GetEBPStmt( IRSB *const irsb )
{
	if( irsb->stmts_used < 6 ) return NULL;

	if( IS_GET32_R( irsb->stmts[1], 20 ) &&
	IS_GET32_R( irsb->stmts[2], 16 ) &&
	IS_SUB32_T( irsb->stmts[3], irsb->stmts[2]->Ist.WrTmp.tmp ) &&
	IS_PUT32_RT( irsb->stmts[4], 16, irsb->stmts[3]->Ist.WrTmp.tmp ) &&
	IS_STORE32_TT( irsb->stmts[5], irsb->stmts[3]->Ist.WrTmp.tmp,  irsb->stmts[1]->Ist.WrTmp.tmp ) )
		return irsb->stmts[3];

	for( int i = 0; i < irsb->stmts_used; ++i )
	{
		if( IS_GET32_R( irsb->stmts[i], 20 ) )
			return irsb->stmts[i];
	}

	return NULL;
}

/*!
Returns the statement that saves the adjusted ESP. The ESP was adjusted to
make space for the local variables.
*/
IRStmt *RexSTP::GetAdjustedESPStmt( IRSB *const irsb, IRStmt *const ebpStmt )
{
	bool hasSubEBP = false; // has stmt that subtracts from EBP
	bool hasPutESP = false; // has stmt that updates ESP with subtracted value from EBP

	IRStmt *potentialESP = NULL;
	for( int i = 0; i < irsb->stmts_used; ++i )
	{
		if( potentialESP == NULL &&
		IS_SUB32_T( irsb->stmts[i], ebpStmt->Ist.WrTmp.tmp ) )
		{	// Look for the subtraction from EBP.
			potentialESP = irsb->stmts[i];
		}
		else if( potentialESP != NULL &&
		IS_PUT32_RT( irsb->stmts[i], 16, potentialESP->Ist.WrTmp.tmp ) )
		{	// Confirmed with a save to ESP (16).
			return potentialESP;
		}
	}

	return NULL;
}

/*!
Returns the statement that stores the return address before a call.
*/
IRStmt* RexSTP::GetStoreRetnAddr( IRSB *const irsb, IRStmt *const espStmt )
{
	if( irsb == NULL || espStmt == NULL )
		return NULL;

	if( irsb->jumpkind != Ijk_Call )
		return NULL;

	// Sanity check
	if( irsb->stmts_used < 5 )
		return NULL;

	uint32_t c_out;
	if( IS_PUT32_RC( irsb->stmts[ irsb->stmts_used-4], 60) &&
	IS_SUB32_TC( irsb->stmts[ irsb->stmts_used-3], espStmt->Ist.WrTmp.tmp, 0x4, c_out ) &&
	IS_PUT32_RT( irsb->stmts[ irsb->stmts_used-2], 16, irsb->stmts[ irsb->stmts_used-3]->Ist.WrTmp.tmp ) &&
	IS_STORE32_T( irsb->stmts[ irsb->stmts_used-1], irsb->stmts[ irsb->stmts_used-3]->Ist.WrTmp.tmp) &&
	(irsb->stmts[ irsb->stmts_used-1]->Ist.Store.data->tag == Iex_Const) )
		return irsb->stmts[ irsb->stmts_used - 1 ];

	// Didn't match signature.
	return NULL;
}

/*!
Returns stmt for the last value written to EAX, i.e. the function
return value. We assume that the last 13 instructions are as follows. 

.
.
.
PUT(0) = t7
----- IMark(0x80483e4, 1) -----
PUT(60)	= 0x80483e4:I32
PUT(16)	= t6
t2	= LDle:I32(t6)
PUT(20) = t2
t8	= Add32(t6,0x4:I32)
PUT(16)	= t8
----- IMark(0x80483e5, 1) -----
PUT(60)	= 0x80483e5:I32
t4	= LDle:I32(t8)
t9	= Add32(t8,0x4:I32)
PUT(16)	= t9
*/
IRStmt* RexSTP::GetEAXStmt( IRSB *const irsb )
{
	int i;
	uint32_t c_out;

	if( irsb->stmts_used < 13 ) return false;

	i = irsb->stmts_used - 13;
	if( IS_PUT32_R( irsb->stmts[i], 0 ) &&
	IS_IMARK( irsb->stmts[i+1] ) &&
	IS_PUT32_R( irsb->stmts[i+2], 60 ) &&
	IS_PUT32_R( irsb->stmts[i+3], 16 ) &&
	IS_LOAD32( irsb->stmts[i+4] ) &&
	IS_PUT32_RT( irsb->stmts[i+5], 20, irsb->stmts[i+4]->Ist.WrTmp.tmp ) &&
	IS_ADD32_XC( irsb->stmts[i+6], 4, c_out ) &&
	IS_PUT32_RT( irsb->stmts[i+7], 16, irsb->stmts[i+6]->Ist.WrTmp.tmp ) &&
	IS_IMARK( irsb->stmts[i+8] ) &&
	IS_PUT32_R( irsb->stmts[i+9], 60 ) &&
	IS_LOAD32_T( irsb->stmts[i+10], irsb->stmts[i+6]->Ist.WrTmp.tmp ) &&
	IS_ADD32_TC( irsb->stmts[i+11], irsb->stmts[i+6]->Ist.WrTmp.tmp, 4, c_out ) &&
	IS_PUT32_RT( irsb->stmts[i+12], 16, irsb->stmts[i+11]->Ist.WrTmp.tmp ) )
		return irsb->stmts[i];

	return NULL;
}

/*!
Returns the EBP offset if the current statement is a store whose index is
a plus from MachStn[0x14]. Example: 

ASSERT( f1b2t13 = MachSt57[0hex014] );
ASSERT( f1b2t14 = BVPLUS(32,f1b2t13,0hexfffffffc) );
MemSt14 : ARRAY BITVECTOR(32) OF BITVECTOR(32) = MemSt13 WITH [f1b2t14] := f1b2t4;

In the example, the function returns 0xfffffffc.
*/
int RexSTP::GetStoreTmpEBPoffset( IRSB *const irsb, const int i, IRStmt *const ebpStmt )
{
	IRStmt *stmt = irsb->stmts[i];
	int ebpoffset = 0;
	int j;
	IRStmt *plusStmt;

	assert( ebpStmt != NULL );
	assert( ebpStmt->tag == Ist_WrTmp );

	// Must be a store to an address
	// specified by a temporary register.
	if( !IS_STORE32( stmt ) || stmt->Ist.Store.addr->tag != Iex_RdTmp )
		return -1;

	// Look for the PLUS stmt whose first operand is
	// a temporary register and second operand is
	// a constant, which would be the offset we want.
	for( j = i - 1; j >= 0; --j )
	{
		plusStmt =  irsb->stmts[j];
		if( IS_T_ADD32( stmt->Ist.Store.addr->Iex.RdTmp.tmp, plusStmt ) &&
		plusStmt->Ist.WrTmp.data->Iex.Binop.arg1->tag == Iex_RdTmp &&
		plusStmt->Ist.WrTmp.data->Iex.Binop.arg2->tag == Iex_Const )
		{
			if( ConstToUint32( plusStmt->Ist.WrTmp.data->Iex.Binop.arg2->Iex.Const.con, (uint32_t* )&ebpoffset ) != 0 )
				return -1;
			break;
		}
	}
	if( j < 0 ) return -1;	

	// ebpoffset is valid if the first operand of the plusStmt
	// is the temporary register that saves the ebp.
	if( ebpStmt->Ist.WrTmp.tmp ==  plusStmt->Ist.WrTmp.data->Iex.Binop.arg1->Iex.RdTmp.tmp )
		return ebpoffset;

	// Didn't find signature for ebpoffset
	return -1;
}

int RexSTP::ExtractPathCond(
	RexFunc	&func,
	IRSB *irsb,
	const unsigned int bbid )
{
	RexPathCond *pathcond;
	// Done if we're returning, or we cannot decode
	// the next instruction.
	if( irsb->jumpkind == Ijk_Ret ||
	irsb->jumpkind == Ijk_NoDecode )
		return 0;

	if( irsb->jumpkind != Ijk_Boring &&
	irsb->jumpkind != Ijk_Call &&
	irsb->jumpkind != Ijk_Sys_syscall &&
	irsb->jumpkind != Ijk_Sys_int32 &&
	irsb->jumpkind != Ijk_Sys_int128 &&
	irsb->jumpkind != Ijk_Sys_int129 &&
	irsb->jumpkind != Ijk_Sys_int130 &&
	irsb->jumpkind != Ijk_Sys_sysenter )
	{
		log->debug( "In function %u\n", func.GetId() );
		log->debug( "jumpkind: %d", irsb->jumpkind );
		rexutils->RexPPIRJumpKind( irsb->jumpkind );
		assert( irsb->jumpkind == Ijk_Boring ||
		irsb->jumpkind == Ijk_Call ||
		irsb->jumpkind == Ijk_Sys_syscall ||
		irsb->jumpkind == Ijk_Sys_int32 ||
		irsb->jumpkind == Ijk_Sys_int128 ||
		irsb->jumpkind == Ijk_Sys_int129 ||
		irsb->jumpkind == Ijk_Sys_int130 ||
		irsb->jumpkind == Ijk_Sys_sysenter );
	}

	pathcond = func.GetPathCond();
	IRStmt* laststmt = irsb->stmts[irsb->stmts_used - 1];

	// Done if last statement is not a proper exit,
	// i.e. a jump.
	if( laststmt->tag != Ist_Exit ) return 0;


	// There's 2 exit points now, one in the last statement
	// and the second in irsb->next.

	assert( laststmt->Ist.Exit.jk == Ijk_Boring );
	assert( laststmt->Ist.Exit.guard->tag == Iex_RdTmp );

	uint32_t addr1, addr2;

	if( ConstToUint32( laststmt->Ist.Exit.dst, &addr1 ) != 0 )
		return -1;
	if( ConstToUint32( irsb->next->Iex.Const.con, &addr2 ) != 0 )
		return -1;

	// DEBUG
	//if( func.GetId() == 3 )
	//{
	//	log->debug( "DEBUG Func %d addr1 %08x addr2 %08x\n",
	//		func.GetId(), addr1, addr2 );
	//	rexutils->RexPPIRSB( irsb );
	//}
	// GUBED
	bool taken = func.IsNextBBTaken( addr1, addr2 );

	pathcond->AddPC(
		func.GetId(),
		bbid,
		laststmt->Ist.Exit.guard->Iex.RdTmp.tmp,
		taken );

	return 0;
}

void RexSTP::CheckRegRead(
	RexMach *rexMach,
	const unsigned int bbid,
	IRStmt *stmt )
{
	// Test with ECX (4), EDX (8), and EBP (20 ) first...
	if( !IS_GET32_R( stmt, 4 ) &&
	!IS_GET32_R( stmt, 8 ) &&
	!IS_GET32_R( stmt, 16 ) &&
	!IS_GET32_R( stmt, 20 ) )
		return;

	REG r  = REGNONE;
	switch( stmt->Ist.WrTmp.data->Iex.Get.offset  )
	{
		case 4: r = ECX; break;
		case 8: r = EDX; break;
		case 16: r = ESP; break;
		case 20: r = EBP; break;
		default: r = REGNONE;
	}

	rexMach->Read( r, bbid, stmt->Ist.WrTmp.tmp );
}

/*!
Adds stmt to regSub if stmt is a SUBTRACT.
*/
int RexSTP::CheckRegSubtract(
	RexMach *rexMach,
	const unsigned int bbid,
	IRStmt *stmt )
{
	if( !IS_SUB32( stmt ) )
		return 0;

	if( stmt->Ist.WrTmp.data->Iex.Binop.arg1->tag != Iex_RdTmp ||
	stmt->Ist.WrTmp.data->Iex.Binop.arg2->tag != Iex_Const )
		return 0;

	uint32_t con;
	if( ConstToUint32( stmt->Ist.WrTmp.data->Iex.Binop.arg2->Iex.Const.con, &con ) != 0 )
		return -1;
	rexMach->AddSubtract(
		bbid,
		stmt->Ist.WrTmp.tmp,
		bbid,
		stmt->Ist.WrTmp.data->Iex.Binop.arg1->Iex.RdTmp.tmp,
		con );

	return 0;
}

/*!
Adds stmt to regPlus if stmt is a PLUS.
*/
int RexSTP::CheckRegPlus(
	RexMach *rexMach,
	const unsigned int bbid,
	IRStmt *stmt )
{
	if( !IS_ADD32( stmt ) )
		return 0;

	if( stmt->Ist.WrTmp.data->Iex.Binop.arg1->tag != Iex_RdTmp ||
	stmt->Ist.WrTmp.data->Iex.Binop.arg2->tag != Iex_Const )
		return 0;

	uint32_t con;
	if( ConstToUint32( stmt->Ist.WrTmp.data->Iex.Binop.arg2->Iex.Const.con, &con ) != 0 )
		return -1;
	
	rexMach->AddPlus(
		bbid,
		stmt->Ist.WrTmp.tmp,
		bbid,
		stmt->Ist.WrTmp.data->Iex.Binop.arg1->Iex.RdTmp.tmp,
		con );

	return 0;
}

/*!
Adds stmt to regPut if stmt is a PUT whose new value is a temporary
register derived from a member in regLoad.
*/
void RexSTP::CheckRegWrite( RexMach *rexMach, const unsigned int bbid, IRStmt *stmt )
{
	if( !IS_PUT32_R( stmt, 4 ) &&
	!IS_PUT32_R( stmt, 8 ) &&
	!IS_PUT32_R( stmt, 16 ) &&
	!IS_PUT32_R( stmt, 20 ) )
		return;

	if( stmt->Ist.Put.data->tag != Iex_RdTmp )
		return;

	REG r  = REGNONE;
	switch( stmt->Ist.Put.offset  )
	{
		case 4: r = ECX; break;
		case 8: r = EDX; break;
		case 16: r = ESP; break;
		case 20: r = EBP; break;
		default: r = REGNONE;
	}

	rexMach->Write( r, bbid, stmt->Ist.Put.data->Iex.RdTmp.tmp );
}

/*!
Adds stmt to regLoad if stmt is a LOAD whose new value is a temporary
register derived from a member in regPlus.
*/

void RexSTP::CheckRegLoad(
	RexMach *rexMach,
	const unsigned int bbid,
	IRStmt *stmt )
{
	if( !IS_LOAD32( stmt ) ||
	stmt->Ist.WrTmp.data->Iex.Load.addr->tag != Iex_RdTmp )
		return;

	rexMach->SetLoaded(
		bbid, stmt->Ist.WrTmp.tmp,
		bbid, stmt->Ist.WrTmp.data->Iex.Load.addr->Iex.RdTmp.tmp );
}

void RexSTP::CheckRegStore(
	RexMach *rexMach,
	const unsigned int bbid,
	IRStmt *stmt,
	const unsigned int memst )
{

	if( !IS_STORE32( stmt ) ||
	stmt->Ist.Store.addr->tag != Iex_RdTmp )
		return;

	rexMach->SetStored( memst, bbid, stmt->Ist.Store.addr->Iex.RdTmp.tmp );
}

int RexSTP::EmitIRSB(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	RexFunc &func,
	IRSB *const irsb )
{
	int retval = 0;
	RexParam *useargs = NULL;
	RexParam *usevars = NULL;
	RexOut *useouts = NULL;
	RexMach *rexMach = func.GetRexMach();

	assert( irsb );

	for(int i = 0; i < irsb->tyenv->types_used; ++i )
	{
		#ifdef USE_STP_FILE
		retval = EmitDeclareTmp( fs, vc, func.GetId(), bbid, i, irsb->tyenv->types[i] );
		#else
		retval = EmitDeclareTmp( vc, func.GetId(), bbid, i, irsb->tyenv->types[i] );
		#endif
		if( retval != 0 )
			return retval;
	}


	// Insert retn of function
	IRStmt *eaxStmt = GetEAXStmt( irsb );
	if( eaxStmt != NULL )
	{
		assert( eaxStmt->tag == Ist_Put );
		func.GetRetn()->AddOut_Retn(
			func.GetId(),
			bbid,
			eaxStmt->Ist.Put.data->Iex.RdTmp.tmp );
	}

	IRStmt *adjESPStmt = NULL;

	IRStmt *ebpStmt = GetEBPStmt( irsb );
	if( ebpStmt != NULL )
	{
		//rexMach->Read( EBP, bbid, ebpStmt->Ist.WrTmp.tmp );
	
		// Find adjusted ESP
		adjESPStmt = GetAdjustedESPStmt( irsb, ebpStmt );
	}

	IRStmt *storeEBP2ESP = NULL;
	if( irsb->stmts_used > 5 &&
	ebpStmt == irsb->stmts[3] )
		storeEBP2ESP = irsb->stmts[5];

	IRStmt *storeRtnAddrStmt = NULL;
	if( adjESPStmt != NULL )
	{
		storeRtnAddrStmt = GetStoreRetnAddr( irsb, adjESPStmt );
	}

	for( int i = 0; i < irsb->stmts_used; ++i )
	{
		IRStmt *stmt = irsb->stmts[i];
		if( stmt == NULL || stmt->tag == Ist_NoOp ) continue;


		CheckRegRead( rexMach, bbid, stmt );
		if( CheckRegPlus( rexMach, bbid, stmt ) != 0 ) return -1;
		if( CheckRegSubtract( rexMach, bbid, stmt ) != 0 ) return -1;
		CheckRegLoad( rexMach, bbid, stmt );
		CheckRegStore( rexMach, bbid, stmt, memSt+1 );
		CheckRegWrite( rexMach, bbid, stmt );

		int ebpoffset = -1;
		if( ebpStmt != NULL )
		{
			ebpoffset = GetStoreTmpEBPoffset( irsb, i, ebpStmt );
		}


		uint32_t c_out;
		useargs = NULL;
		usevars = NULL;
		if( ebpStmt != NULL &&
		!IS_ADD32_TC( stmt, ebpStmt->Ist.WrTmp.tmp, 4, c_out ) )
		{
			useargs = func.GetArgs();
			usevars = func.GetVars();
		}

		useouts = NULL;
		if( stmt != eaxStmt  &&
		stmt != storeEBP2ESP &&
		stmt != storeRtnAddrStmt )
		{
			if( ebpoffset == -1 )
				useouts = func.GetOutAuxs();
			else if( ebpoffset < 0 )
				useouts = func.GetOutVars();
			else if( ebpoffset > 4 )
				useouts = func.GetOutArgs();
		}
		//log->debug("%d\t", i);
		//func.GetRexUtils()->RexPPIRStmt(stmt);
		//log->debug("\n");
		retval = EmitIRStmt(
			#ifdef USE_STP_FILE
			fs,
			#endif
			vc,
			func.GetId(),
			bbid, irsb, stmt, /*&ebpStmt,*/
			useargs, usevars, useouts,
			ebpoffset );

		if( retval != 0 )
			return retval;
	}

	//rexutils->RexPPIRSB( irsb );
	if( retval == 0 )
	{
		retval = ExtractPathCond( func, irsb, bbid );
		bbid++;
	}

	return retval;
}

int RexSTP::TranslateRexBB(
	RexTranslator *rextrans,
	RexFunc &func,
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc
	)
{
	unsigned int		offset;
	size_t			insnsSize;
	list< unsigned int >	work;
	IRSB *			irsb;
	unsigned char *		insns;
	int			retval = -1;

	insns = func.GetInsnsOnUsePath(work);
	assert( insns != NULL );

	insnsSize = func.GetInsnsSize();
	while( work.size() > 0 )
	{
		offset = work.front();
		work.pop_front();

		//log->debug("DEBUG Translating to IR at %08x insn offset %d\n",
		//	func.GetEAAtInsnOffset( insns, offset ),
		//	offset);
		/*
		for( int i = 0; i < min(10, (int)insnsSize); ++i )
		{
			log->debug("%02x ", insns[offset+i]);
		}
		log->debug("\n");
		*/

		irsb = rextrans->ToIR(
			&insns[offset],
			func.GetEAAtInsnOffset( insns, offset ),
			func.GetRexUtils() );

		func.AddIRSB( irsb );

		// Emit constraints for this irsb
		retval = EmitIRSB(
			#ifdef USE_STP_FILE
			fs,
			#endif
			vc, func, irsb );
		if( retval != 0 ) return retval;

		VexGuestExtents* vge = rextrans->GetGuestExtents();
		for( int i = vge->n_used - 1; i >= 0; --i )
		{
			offset += vge->len[i];
		}

		if( offset < insnsSize )
		{
			if( work.size() == 0 ||
			offset < work.front() )
			{
				work.push_front(offset);
			}
			else
			{
				while( offset > work.front() )
					work.pop_front();
			}
		}
	}

	// In the newer version, we use the RexMach mechanism to extract offsets.
	// So we use that instead of the older RexIO method. But we need to transfer
	// the results.
	TransferStoreTmpToOutput( func );

	return retval;
}


bool RexSTP::DerivedFromESP( RexTemp* rtmp )
{
	if( rtmp->base == 0x0 )
	{
		if( rtmp->reg == ESP )
			return true;
		else
			return false;
	}
	return DerivedFromESP( rtmp->base );
}

void RexSTP::TransferStoreTmpToOutput( RexFunc &func )
{
	RexMach* rexMach = func.GetRexMach();
	RexOut* rexauxs = func.GetOutAuxs();

	RexTemp *storetmp = rexMach->GetFirstStoredRexTemp();
	while( storetmp != NULL )
	{
		if( storetmp->base != 0x0 &&
		( !(storetmp->base->action == SUBTRACT && storetmp->base->offset == 4) ||
		!DerivedFromESP(storetmp) ) )
		{
			rexauxs->AddOut_StoreTmp(
				storetmp->memst,
				func.GetId(),
				storetmp->bbid,
				storetmp->tmp );
		}

		storetmp = rexMach->GetNextStoredRexTemp();
	}
}

int RexSTP::EmitFunc(
	#ifdef USE_STP_FILE
	fstream &fs,
	#endif
	VC &vc,
	RexFunc &func )
{
	char	varname[VARNAMESIZE];
	Expr	e_memSt;
	Expr	e_machSt;
	int	retval = -1;
	#ifdef USE_STP_FILE
	char * obuf;
	obuf = out;
	obuf[0] = 0x0;
	#endif


	//useMemSt1 = ((memSt != 0) || (machSt != 0));
	//useMachSt1 = useMemSt1;
	bbid = 0;
	useMachSt1 = (machSt != 0);
	useMemSt1 = false;

	// Declare MemSt and MachSt
	if( memSt == 0 || useMemSt1 == false )
	{
		snprintf( varname, VARNAMESIZE, "MemSt%u", ++memSt );
		e_memSt = vc_varExpr( vc, varname,
			vc_arrayType( vc, vc_bv32Type( vc ),
			vc_bv32Type( vc ) ) );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd,
			"MemSt%u : ARRAY BITVECTOR(32) OF BITVECTOR(32);\n",
			memSt );
		#endif	
	}
	if( machSt == 0 || useMachSt1 == false )
	{
		snprintf( varname, VARNAMESIZE, "MachSt%u", ++machSt );
		e_machSt = vc_varExpr( vc, varname,
			vc_arrayType( vc, vc_bvType( vc, 12 ),
			vc_bv32Type( vc ) ) );
		#ifdef USE_STP_FILE
		PRINTOBUF( obuf, outEnd,
			"MachSt%u : ARRAY BITVECTOR(12) OF BITVECTOR(32);\n",
			machSt );
		#endif
	}

	func.SetFirstMemSt( useMemSt1 ? 1 : memSt );
	func.SetFirstMachSt( useMachSt1 ? 1 : machSt );

	#ifdef USE_STP_FILE
	STPPRINT( fs, out );
	retval = TranslateRexBB( this->rextrans, func, fs, vc );
	#else
	retval = TranslateRexBB( this->rextrans, func, vc );
	#endif
	if( retval != 0 ) return retval;

	// We can't conclude if there's a function call anywhere,
	// or if the last IRSB does not end with a return.
	//if( func.HasUnsupportedIRSB() == true )
	//	return -1; 

	//log->debug("Printing IRSBs for func%d\n", func.GetId());
	//func.PrintIRSBs();
	//func.PrintRexMach();

	// Reset usepath's iterator in func,
	// so that next call starts from beginning again
	func.ResetUsePathItr();

	// Store the final memory and machine states
	func.SetFinalMemSt( memSt );
	func.SetFinalMachSt( machSt );
	return retval;
}


void RexSTP::SaveStatesSnapshot()
{
	lastMachSt = machSt;
	lastMemSt = memSt;
}

void RexSTP::RevertStatesSnapshot()
{
	machSt = lastMachSt;
	memSt = lastMemSt;
}

void RexSTP::ResetStatesSnapshot()
{
	memSt	= 0;
	machSt	= 0;
}


