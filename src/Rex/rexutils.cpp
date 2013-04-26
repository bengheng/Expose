#ifdef __cplusplus
extern "C" {
#endif
#include <libvex.h>
#ifdef __cplusplus
}
#endif

#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <rexutils.h>
#include <assert.h>
#include <log4cpp/Category.hh>
//#include <log4cpp/FileAppender.hh>
//#include <log4cpp/PatternLayout.hh>
//#include <log4cpp/SimpleLayout.hh>

RexUtils::RexUtils( log4cpp::Category *_log )
{
	mempool = NULL;
	memfree = 0;
	memsize = 0;
	nextmem = NULL;

	log = _log;
}

RexUtils::~RexUtils()
{
	if( mempool != NULL )
		delete[] mempool;
}

int RexUtils::AllocMemPool( const size_t nbytes )
{
	// The memory pool must not have been allocated.
	if( memsize != memfree ) return -1;

	// Same memsize requested.
	if ( memsize == nbytes )
	{
		return 0;
	}

	// The new requested size is different,
	// so we re-allocate.
	if( mempool != NULL )
	{
		delete [] mempool;
		mempool = NULL;
		memfree = 0;
		memsize = 0;
		nextmem = NULL;
	}

	if( nbytes == 0 )
	{
		return 0;
	}

	mempool = new unsigned char[nbytes];
	if ( mempool == NULL ) return -1;
	memset( mempool, 0x0, nbytes );

	memsize = nbytes;
	memfree = nbytes;
	nextmem = mempool;
	return 0;
}

void RexUtils::FreeMemPool()
{
	memfree = memsize;
	nextmem = mempool;
}

void* RexUtils::Alloc( const size_t nbytes )
{
	unsigned char* newmem = NULL;
	if ( memfree < nbytes )
	{
		log->fatal( "RexUtils: OUT OF MEMORY!\n");
		assert( memfree >= nbytes );
		return NULL;
	}
	newmem = nextmem;
	nextmem += nbytes;
	memfree -= nbytes;
	return newmem;
}

void RexUtils::RexPrint( const char *_msg, ... )
{
	char		msg[512];
	va_list		args;
	va_start( args, _msg );
	vsnprintf( msg, 512, _msg, args );
	va_end( args );
	log->debug( "%s", msg );
}

//================================================================
// Adapted from valgrind/VEX/priv/ir_defs.c
//================================================================
/*---------------------------------------------------------------*/
/*--- Printing the IR                                         ---*/
/*---------------------------------------------------------------*/

void RexUtils::RexPPIRType ( const IRType ty )
{
	switch (ty) {
	case Ity_INVALID:
		RexPrint("Ity_INVALID");
		break;
	case Ity_I1:
		RexPrint( "I1");
		break;
	case Ity_I8:
		RexPrint( "I8");
		break;
	case Ity_I16:
		RexPrint( "I16");
		break;
	case Ity_I32:
		RexPrint( "I32");
		break;
	case Ity_I64:
		RexPrint( "I64");
		break;
	case Ity_I128:
		RexPrint( "I128");
		break;
	case Ity_F32:
		RexPrint( "F32");
		break;
	case Ity_F64:
		RexPrint( "F64");
		break;
	case Ity_V128:
		RexPrint( "V128");
		break;
	default:
		RexPrint("ty = 0x%x\n", (Int)ty);
		throw "RexPPIRType";
	}
}

void RexUtils::RexPPIRConst ( const IRConst *const con )
{
	union { ULong i64;
		Double f64;
	} u;
	assert(sizeof(ULong) == sizeof(Double));
	switch (con->tag) {
	case Ico_U1:
		RexPrint( "%d:I1",        con->Ico.U1 ? 1 : 0);
		break;
	case Ico_U8:
		RexPrint( "0x%x:I8",      (UInt)(con->Ico.U8));
		break;
	case Ico_U16:
		RexPrint( "0x%x:I16",     (UInt)(con->Ico.U16));
		break;
	case Ico_U32:
		RexPrint( "0x%x:I32",     (UInt)(con->Ico.U32));
		break;
	case Ico_U64:
		RexPrint( "0x%llx:I64",   (ULong)(con->Ico.U64));
		break;
	case Ico_F64:
		u.f64 = con->Ico.F64;
		RexPrint( "F64{0x%llx}",  u.i64);
		break;
	case Ico_F64i:
		RexPrint( "F64i{0x%llx}", con->Ico.F64i);
		break;
	case Ico_V128:
		RexPrint( "V128{0x%04x}", (UInt)(con->Ico.V128));
		break;
	default:
		throw "RexPPIRConst";
	}
}

void RexUtils::RexPPIRCallee ( const IRCallee *const ce )
{
	RexPrint("%s", ce->name);
	if (ce->regparms > 0)
		RexPrint("[rp=%d]", ce->regparms);
	if (ce->mcx_mask > 0)
		RexPrint("[mcx=0x%x]", ce->mcx_mask);
	RexPrint("{%p}", (void*)ce->addr);
}

void RexUtils::RexPPIRRegArray ( const IRRegArray *const arr )
{
	RexPrint("(%d:%dx", arr->base, arr->nElems);
	RexPPIRType(arr->elemTy);
	RexPrint(")");
}

void RexUtils::RexPPIRTemp ( const IRTemp tmp )
{
	if (tmp == IRTemp_INVALID)
		RexPrint("IRTemp_INVALID");
	else
		RexPrint( "t%d", (Int)tmp);
}

void RexUtils::RexPPIROp ( const IROp op )
{
	const char* str = NULL;
	IROp   base;
	switch (op) {
	case Iop_Add8 ... Iop_Add64:
		str = "Add";
		base = Iop_Add8;
		break;
	case Iop_Sub8 ... Iop_Sub64:
		str = "Sub";
		base = Iop_Sub8;
		break;
	case Iop_Mul8 ... Iop_Mul64:
		str = "Mul";
		base = Iop_Mul8;
		break;
	case Iop_Or8 ... Iop_Or64:
		str = "Or";
		base = Iop_Or8;
		break;
	case Iop_And8 ... Iop_And64:
		str = "And";
		base = Iop_And8;
		break;
	case Iop_Xor8 ... Iop_Xor64:
		str = "Xor";
		base = Iop_Xor8;
		break;
	case Iop_Shl8 ... Iop_Shl64:
		str = "Shl";
		base = Iop_Shl8;
		break;
	case Iop_Shr8 ... Iop_Shr64:
		str = "Shr";
		base = Iop_Shr8;
		break;
	case Iop_Sar8 ... Iop_Sar64:
		str = "Sar";
		base = Iop_Sar8;
		break;
	case Iop_CmpEQ8 ... Iop_CmpEQ64:
		str = "CmpEQ";
		base = Iop_CmpEQ8;
		break;
	case Iop_CmpNE8 ... Iop_CmpNE64:
		str = "CmpNE";
		base = Iop_CmpNE8;
		break;
	case Iop_CasCmpEQ8 ... Iop_CasCmpEQ64:
		str = "CasCmpEQ";
		base = Iop_CasCmpEQ8;
		break;
	case Iop_CasCmpNE8 ... Iop_CasCmpNE64:
		str = "CasCmpNE";
		base = Iop_CasCmpNE8;
		break;
	case Iop_Not8 ... Iop_Not64:
		str = "Not";
		base = Iop_Not8;
		break;
		/* other cases must explicitly "return;" */
	case Iop_8Uto16:
		RexPrint("8Uto16");
		return;
	case Iop_8Uto32:
		RexPrint("8Uto32");
		return;
	case Iop_16Uto32:
		RexPrint("16Uto32");
		return;
	case Iop_8Sto16:
		RexPrint("8Sto16");
		return;
	case Iop_8Sto32:
		RexPrint("8Sto32");
		return;
	case Iop_16Sto32:
		RexPrint("16Sto32");
		return;
	case Iop_32Sto64:
		RexPrint("32Sto64");
		return;
	case Iop_32Uto64:
		RexPrint("32Uto64");
		return;
	case Iop_32to8:
		RexPrint("32to8");
		return;
	case Iop_16Uto64:
		RexPrint("16Uto64");
		return;
	case Iop_16Sto64:
		RexPrint("16Sto64");
		return;
	case Iop_8Uto64:
		RexPrint("8Uto64");
		return;
	case Iop_8Sto64:
		RexPrint("8Sto64");
		return;
	case Iop_64to16:
		RexPrint("64to16");
		return;
	case Iop_64to8:
		RexPrint("64to8");
		return;

	case Iop_Not1:
		RexPrint("Not1");
		return;
	case Iop_32to1:
		RexPrint("32to1");
		return;
	case Iop_64to1:
		RexPrint("64to1");
		return;
	case Iop_1Uto8:
		RexPrint("1Uto8");
		return;
	case Iop_1Uto32:
		RexPrint("1Uto32");
		return;
	case Iop_1Uto64:
		RexPrint("1Uto64");
		return;
	case Iop_1Sto8:
		RexPrint("1Sto8");
		return;
	case Iop_1Sto16:
		RexPrint("1Sto16");
		return;
	case Iop_1Sto32:
		RexPrint("1Sto32");
		return;
	case Iop_1Sto64:
		RexPrint("1Sto64");
		return;

	case Iop_MullS8:
		RexPrint("MullS8");
		return;
	case Iop_MullS16:
		RexPrint("MullS16");
		return;
	case Iop_MullS32:
		RexPrint("MullS32");
		return;
	case Iop_MullS64:
		RexPrint("MullS64");
		return;
	case Iop_MullU8:
		RexPrint("MullU8");
		return;
	case Iop_MullU16:
		RexPrint("MullU16");
		return;
	case Iop_MullU32:
		RexPrint("MullU32");
		return;
	case Iop_MullU64:
		RexPrint("MullU64");
		return;

	case Iop_Clz64:
		RexPrint("Clz64");
		return;
	case Iop_Clz32:
		RexPrint("Clz32");
		return;
	case Iop_Ctz64:
		RexPrint("Ctz64");
		return;
	case Iop_Ctz32:
		RexPrint("Ctz32");
		return;

	case Iop_CmpLT32S:
		RexPrint("CmpLT32S");
		return;
	case Iop_CmpLE32S:
		RexPrint("CmpLE32S");
		return;
	case Iop_CmpLT32U:
		RexPrint("CmpLT32U");
		return;
	case Iop_CmpLE32U:
		RexPrint("CmpLE32U");
		return;

	case Iop_CmpLT64S:
		RexPrint("CmpLT64S");
		return;
	case Iop_CmpLE64S:
		RexPrint("CmpLE64S");
		return;
	case Iop_CmpLT64U:
		RexPrint("CmpLT64U");
		return;
	case Iop_CmpLE64U:
		RexPrint("CmpLE64U");
		return;

	case Iop_CmpNEZ8:
		RexPrint("CmpNEZ8");
		return;
	case Iop_CmpNEZ16:
		RexPrint("CmpNEZ16");
		return;
	case Iop_CmpNEZ32:
		RexPrint("CmpNEZ32");
		return;
	case Iop_CmpNEZ64:
		RexPrint("CmpNEZ64");
		return;

	case Iop_CmpwNEZ32:
		RexPrint("CmpwNEZ32");
		return;
	case Iop_CmpwNEZ64:
		RexPrint("CmpwNEZ64");
		return;

	case Iop_Left8:
		RexPrint("Left8");
		return;
	case Iop_Left16:
		RexPrint("Left16");
		return;
	case Iop_Left32:
		RexPrint("Left32");
		return;
	case Iop_Left64:
		RexPrint("Left64");
		return;
	case Iop_Max32U:
		RexPrint("Max32U");
		return;

	case Iop_CmpORD32U:
		RexPrint("CmpORD32U");
		return;
	case Iop_CmpORD32S:
		RexPrint("CmpORD32S");
		return;

	case Iop_CmpORD64U:
		RexPrint("CmpORD64U");
		return;
	case Iop_CmpORD64S:
		RexPrint("CmpORD64S");
		return;

	case Iop_DivU32:
		RexPrint("DivU32");
		return;
	case Iop_DivS32:
		RexPrint("DivS32");
		return;
	case Iop_DivU64:
		RexPrint("DivU64");
		return;
	case Iop_DivS64:
		RexPrint("DivS64");
		return;

	case Iop_DivModU64to32:
		RexPrint("DivModU64to32");
		return;
	case Iop_DivModS64to32:
		RexPrint("DivModS64to32");
		return;

	case Iop_DivModU128to64:
		RexPrint("DivModU128to64");
		return;
	case Iop_DivModS128to64:
		RexPrint("DivModS128to64");
		return;

	case Iop_16HIto8:
		RexPrint("16HIto8");
		return;
	case Iop_16to8:
		RexPrint("16to8");
		return;
	case Iop_8HLto16:
		RexPrint("8HLto16");
		return;

	case Iop_32HIto16:
		RexPrint("32HIto16");
		return;
	case Iop_32to16:
		RexPrint("32to16");
		return;
	case Iop_16HLto32:
		RexPrint("16HLto32");
		return;

	case Iop_64HIto32:
		RexPrint("64HIto32");
		return;
	case Iop_64to32:
		RexPrint("64to32");
		return;
	case Iop_32HLto64:
		RexPrint("32HLto64");
		return;

	case Iop_128HIto64:
		RexPrint("128HIto64");
		return;
	case Iop_128to64:
		RexPrint("128to64");
		return;
	case Iop_64HLto128:
		RexPrint("64HLto128");
		return;

	case Iop_AddF64:
		RexPrint("AddF64");
		return;
	case Iop_SubF64:
		RexPrint("SubF64");
		return;
	case Iop_MulF64:
		RexPrint("MulF64");
		return;
	case Iop_DivF64:
		RexPrint("DivF64");
		return;
	case Iop_AddF64r32:
		RexPrint("AddF64r32");
		return;
	case Iop_SubF64r32:
		RexPrint("SubF64r32");
		return;
	case Iop_MulF64r32:
		RexPrint("MulF64r32");
		return;
	case Iop_DivF64r32:
		RexPrint("DivF64r32");
		return;
	case Iop_AddF32:
		RexPrint("AddF32");
		return;
	case Iop_SubF32:
		RexPrint("SubF32");
		return;
	case Iop_MulF32:
		RexPrint("MulF32");
		return;
	case Iop_DivF32:
		RexPrint("DivF32");
		return;

	case Iop_ScaleF64:
		RexPrint("ScaleF64");
		return;
	case Iop_AtanF64:
		RexPrint("AtanF64");
		return;
	case Iop_Yl2xF64:
		RexPrint("Yl2xF64");
		return;
	case Iop_Yl2xp1F64:
		RexPrint("Yl2xp1F64");
		return;
	case Iop_PRemF64:
		RexPrint("PRemF64");
		return;
	case Iop_PRemC3210F64:
		RexPrint("PRemC3210F64");
		return;
	case Iop_PRem1F64:
		RexPrint("PRem1F64");
		return;
	case Iop_PRem1C3210F64:
		RexPrint("PRem1C3210F64");
		return;
	case Iop_NegF64:
		RexPrint("NegF64");
		return;
	case Iop_AbsF64:
		RexPrint("AbsF64");
		return;
	case Iop_NegF32:
		RexPrint("NegF32");
		return;
	case Iop_AbsF32:
		RexPrint("AbsF32");
		return;
	case Iop_SqrtF64:
		RexPrint("SqrtF64");
		return;
	case Iop_SqrtF32:
		RexPrint("SqrtF32");
		return;
	case Iop_SinF64:
		RexPrint("SinF64");
		return;
	case Iop_CosF64:
		RexPrint("CosF64");
		return;
	case Iop_TanF64:
		RexPrint("TanF64");
		return;
	case Iop_2xm1F64:
		RexPrint("2xm1F64");
		return;

	case Iop_MAddF64:
		RexPrint("MAddF64");
		return;
	case Iop_MSubF64:
		RexPrint("MSubF64");
		return;
	case Iop_MAddF64r32:
		RexPrint("MAddF64r32");
		return;
	case Iop_MSubF64r32:
		RexPrint("MSubF64r32");
		return;

	case Iop_Est5FRSqrt:
		RexPrint("Est5FRSqrt");
		return;
	case Iop_RoundF64toF64_NEAREST:
		RexPrint("RoundF64toF64_NEAREST");
		return;
	case Iop_RoundF64toF64_NegINF:
		RexPrint("RoundF64toF64_NegINF");
		return;
	case Iop_RoundF64toF64_PosINF:
		RexPrint("RoundF64toF64_PosINF");
		return;
	case Iop_RoundF64toF64_ZERO:
		RexPrint("RoundF64toF64_ZERO");
		return;

	case Iop_TruncF64asF32:
		RexPrint("TruncF64asF32");
		return;
	case Iop_CalcFPRF:
		RexPrint("CalcFPRF");
		return;

	case Iop_Add16x2:
		RexPrint("Add16x2");
		return;
	case Iop_Sub16x2:
		RexPrint("Sub16x2");
		return;
	case Iop_QAdd16Sx2:
		RexPrint("QAdd16Sx2");
		return;
	case Iop_QAdd16Ux2:
		RexPrint("QAdd16Ux2");
		return;
	case Iop_QSub16Sx2:
		RexPrint("QSub16Sx2");
		return;
	case Iop_QSub16Ux2:
		RexPrint("QSub16Ux2");
		return;
	case Iop_HAdd16Ux2:
		RexPrint("HAdd16Ux2");
		return;
	case Iop_HAdd16Sx2:
		RexPrint("HAdd16Sx2");
		return;
	case Iop_HSub16Ux2:
		RexPrint("HSub16Ux2");
		return;
	case Iop_HSub16Sx2:
		RexPrint("HSub16Sx2");
		return;

	case Iop_Add8x4:
		RexPrint("Add8x4");
		return;
	case Iop_Sub8x4:
		RexPrint("Sub8x4");
		return;
	case Iop_QAdd8Sx4:
		RexPrint("QAdd8Sx4");
		return;
	case Iop_QAdd8Ux4:
		RexPrint("QAdd8Ux4");
		return;
	case Iop_QSub8Sx4:
		RexPrint("QSub8Sx4");
		return;
	case Iop_QSub8Ux4:
		RexPrint("QSub8Ux4");
		return;
	case Iop_HAdd8Ux4:
		RexPrint("HAdd8Ux4");
		return;
	case Iop_HAdd8Sx4:
		RexPrint("HAdd8Sx4");
		return;
	case Iop_HSub8Ux4:
		RexPrint("HSub8Ux4");
		return;
	case Iop_HSub8Sx4:
		RexPrint("HSub8Sx4");
		return;
	case Iop_Sad8Ux4:
		RexPrint("Sad8Ux4");
		return;

	case Iop_CmpNEZ16x2:
		RexPrint("CmpNEZ16x2");
		return;
	case Iop_CmpNEZ8x4:
		RexPrint("CmpNEZ8x4");
		return;

	case Iop_CmpF64:
		RexPrint("CmpF64");
		return;

	case Iop_F64toI16S:
		RexPrint("F64toI16S");
		return;
	case Iop_F64toI32S:
		RexPrint("F64toI32S");
		return;
	case Iop_F64toI64S:
		RexPrint("F64toI64S");
		return;

	case Iop_F64toI32U:
		RexPrint("F64toI32U");
		return;

	case Iop_I16StoF64:
		RexPrint("I16StoF64");
		return;
	case Iop_I32StoF64:
		RexPrint("I32StoF64");
		return;
	case Iop_I64StoF64:
		RexPrint("I64StoF64");
		return;

	case Iop_I32UtoF64:
		RexPrint("I32UtoF64");
		return;

	case Iop_F32toF64:
		RexPrint("F32toF64");
		return;
	case Iop_F64toF32:
		RexPrint("F64toF32");
		return;

	case Iop_RoundF64toInt:
		RexPrint("RoundF64toInt");
		return;
	case Iop_RoundF32toInt:
		RexPrint("RoundF32toInt");
		return;
	case Iop_RoundF64toF32:
		RexPrint("RoundF64toF32");
		return;

	case Iop_ReinterpF64asI64:
		RexPrint("ReinterpF64asI64");
		return;
	case Iop_ReinterpI64asF64:
		RexPrint("ReinterpI64asF64");
		return;
	case Iop_ReinterpF32asI32:
		RexPrint("ReinterpF32asI32");
		return;
	case Iop_ReinterpI32asF32:
		RexPrint("ReinterpI32asF32");
		return;

	case Iop_I32UtoFx4:
		RexPrint("I32UtoFx4");
		return;
	case Iop_I32StoFx4:
		RexPrint("I32StoFx4");
		return;

	case Iop_F32toF16x4:
		RexPrint("F32toF16x4");
		return;
	case Iop_F16toF32x4:
		RexPrint("F16toF32x4");
		return;

	case Iop_Rsqrte32Fx4:
		RexPrint("VRsqrte32Fx4");
		return;
	case Iop_Rsqrte32x4:
		RexPrint("VRsqrte32x4");
		return;
	case Iop_Rsqrte32Fx2:
		RexPrint("VRsqrte32Fx2");
		return;
	case Iop_Rsqrte32x2:
		RexPrint("VRsqrte32x2");
		return;

	case Iop_QFtoI32Ux4_RZ:
		RexPrint("QFtoI32Ux4_RZ");
		return;
	case Iop_QFtoI32Sx4_RZ:
		RexPrint("QFtoI32Sx4_RZ");
		return;

	case Iop_FtoI32Ux4_RZ:
		RexPrint("FtoI32Ux4_RZ");
		return;
	case Iop_FtoI32Sx4_RZ:
		RexPrint("FtoI32Sx4_RZ");
		return;

	case Iop_I32UtoFx2:
		RexPrint("I32UtoFx2");
		return;
	case Iop_I32StoFx2:
		RexPrint("I32StoFx2");
		return;

	case Iop_FtoI32Ux2_RZ:
		RexPrint("FtoI32Ux2_RZ");
		return;
	case Iop_FtoI32Sx2_RZ:
		RexPrint("FtoI32Sx2_RZ");
		return;

	case Iop_RoundF32x4_RM:
		RexPrint("RoundF32x4_RM");
		return;
	case Iop_RoundF32x4_RP:
		RexPrint("RoundF32x4_RP");
		return;
	case Iop_RoundF32x4_RN:
		RexPrint("RoundF32x4_RN");
		return;
	case Iop_RoundF32x4_RZ:
		RexPrint("RoundF32x4_RZ");
		return;

	case Iop_Abs8x8:
		RexPrint("Abs8x8");
		return;
	case Iop_Abs16x4:
		RexPrint("Abs16x4");
		return;
	case Iop_Abs32x2:
		RexPrint("Abs32x2");
		return;
	case Iop_Add8x8:
		RexPrint("Add8x8");
		return;
	case Iop_Add16x4:
		RexPrint("Add16x4");
		return;
	case Iop_Add32x2:
		RexPrint("Add32x2");
		return;
	case Iop_QAdd8Ux8:
		RexPrint("QAdd8Ux8");
		return;
	case Iop_QAdd16Ux4:
		RexPrint("QAdd16Ux4");
		return;
	case Iop_QAdd32Ux2:
		RexPrint("QAdd32Ux2");
		return;
	case Iop_QAdd64Ux1:
		RexPrint("QAdd64Ux1");
		return;
	case Iop_QAdd8Sx8:
		RexPrint("QAdd8Sx8");
		return;
	case Iop_QAdd16Sx4:
		RexPrint("QAdd16Sx4");
		return;
	case Iop_QAdd32Sx2:
		RexPrint("QAdd32Sx2");
		return;
	case Iop_QAdd64Sx1:
		RexPrint("QAdd64Sx1");
		return;
	case Iop_PwAdd8x8:
		RexPrint("PwAdd8x8");
		return;
	case Iop_PwAdd16x4:
		RexPrint("PwAdd16x4");
		return;
	case Iop_PwAdd32x2:
		RexPrint("PwAdd32x2");
		return;
	case Iop_PwAdd32Fx2:
		RexPrint("PwAdd32Fx2");
		return;
	case Iop_PwAddL8Ux8:
		RexPrint("PwAddL8Ux8");
		return;
	case Iop_PwAddL16Ux4:
		RexPrint("PwAddL16Ux4");
		return;
	case Iop_PwAddL32Ux2:
		RexPrint("PwAddL32Ux2");
		return;
	case Iop_PwAddL8Sx8:
		RexPrint("PwAddL8Sx8");
		return;
	case Iop_PwAddL16Sx4:
		RexPrint("PwAddL16Sx4");
		return;
	case Iop_PwAddL32Sx2:
		RexPrint("PwAddL32Sx2");
		return;
	case Iop_Sub8x8:
		RexPrint("Sub8x8");
		return;
	case Iop_Sub16x4:
		RexPrint("Sub16x4");
		return;
	case Iop_Sub32x2:
		RexPrint("Sub32x2");
		return;
	case Iop_QSub8Ux8:
		RexPrint("QSub8Ux8");
		return;
	case Iop_QSub16Ux4:
		RexPrint("QSub16Ux4");
		return;
	case Iop_QSub32Ux2:
		RexPrint("QSub32Ux2");
		return;
	case Iop_QSub64Ux1:
		RexPrint("QSub64Ux1");
		return;
	case Iop_QSub8Sx8:
		RexPrint("QSub8Sx8");
		return;
	case Iop_QSub16Sx4:
		RexPrint("QSub16Sx4");
		return;
	case Iop_QSub32Sx2:
		RexPrint("QSub32Sx2");
		return;
	case Iop_QSub64Sx1:
		RexPrint("QSub64Sx1");
		return;
	case Iop_Mul8x8:
		RexPrint("Mul8x8");
		return;
	case Iop_Mul16x4:
		RexPrint("Mul16x4");
		return;
	case Iop_Mul32x2:
		RexPrint("Mul32x2");
		return;
	case Iop_Mul32Fx2:
		RexPrint("Mul32Fx2");
		return;
	case Iop_PolynomialMul8x8:
		RexPrint("PolynomialMul8x8");
		return;
	case Iop_MulHi16Ux4:
		RexPrint("MulHi16Ux4");
		return;
	case Iop_MulHi16Sx4:
		RexPrint("MulHi16Sx4");
		return;
	case Iop_QDMulHi16Sx4:
		RexPrint("QDMulHi16Sx4");
		return;
	case Iop_QDMulHi32Sx2:
		RexPrint("QDMulHi32Sx2");
		return;
	case Iop_QRDMulHi16Sx4:
		RexPrint("QRDMulHi16Sx4");
		return;
	case Iop_QRDMulHi32Sx2:
		RexPrint("QRDMulHi32Sx2");
		return;
	case Iop_QDMulLong16Sx4:
		RexPrint("QDMulLong16Sx4");
		return;
	case Iop_QDMulLong32Sx2:
		RexPrint("QDMulLong32Sx2");
		return;
	case Iop_Avg8Ux8:
		RexPrint("Avg8Ux8");
		return;
	case Iop_Avg16Ux4:
		RexPrint("Avg16Ux4");
		return;
	case Iop_Max8Sx8:
		RexPrint("Max8Sx8");
		return;
	case Iop_Max16Sx4:
		RexPrint("Max16Sx4");
		return;
	case Iop_Max32Sx2:
		RexPrint("Max32Sx2");
		return;
	case Iop_Max8Ux8:
		RexPrint("Max8Ux8");
		return;
	case Iop_Max16Ux4:
		RexPrint("Max16Ux4");
		return;
	case Iop_Max32Ux2:
		RexPrint("Max32Ux2");
		return;
	case Iop_Min8Sx8:
		RexPrint("Min8Sx8");
		return;
	case Iop_Min16Sx4:
		RexPrint("Min16Sx4");
		return;
	case Iop_Min32Sx2:
		RexPrint("Min32Sx2");
		return;
	case Iop_Min8Ux8:
		RexPrint("Min8Ux8");
		return;
	case Iop_Min16Ux4:
		RexPrint("Min16Ux4");
		return;
	case Iop_Min32Ux2:
		RexPrint("Min32Ux2");
		return;
	case Iop_PwMax8Sx8:
		RexPrint("PwMax8Sx8");
		return;
	case Iop_PwMax16Sx4:
		RexPrint("PwMax16Sx4");
		return;
	case Iop_PwMax32Sx2:
		RexPrint("PwMax32Sx2");
		return;
	case Iop_PwMax8Ux8:
		RexPrint("PwMax8Ux8");
		return;
	case Iop_PwMax16Ux4:
		RexPrint("PwMax16Ux4");
		return;
	case Iop_PwMax32Ux2:
		RexPrint("PwMax32Ux2");
		return;
	case Iop_PwMin8Sx8:
		RexPrint("PwMin8Sx8");
		return;
	case Iop_PwMin16Sx4:
		RexPrint("PwMin16Sx4");
		return;
	case Iop_PwMin32Sx2:
		RexPrint("PwMin32Sx2");
		return;
	case Iop_PwMin8Ux8:
		RexPrint("PwMin8Ux8");
		return;
	case Iop_PwMin16Ux4:
		RexPrint("PwMin16Ux4");
		return;
	case Iop_PwMin32Ux2:
		RexPrint("PwMin32Ux2");
		return;
	case Iop_CmpEQ8x8:
		RexPrint("CmpEQ8x8");
		return;
	case Iop_CmpEQ16x4:
		RexPrint("CmpEQ16x4");
		return;
	case Iop_CmpEQ32x2:
		RexPrint("CmpEQ32x2");
		return;
	case Iop_CmpGT8Ux8:
		RexPrint("CmpGT8Ux8");
		return;
	case Iop_CmpGT16Ux4:
		RexPrint("CmpGT16Ux4");
		return;
	case Iop_CmpGT32Ux2:
		RexPrint("CmpGT32Ux2");
		return;
	case Iop_CmpGT8Sx8:
		RexPrint("CmpGT8Sx8");
		return;
	case Iop_CmpGT16Sx4:
		RexPrint("CmpGT16Sx4");
		return;
	case Iop_CmpGT32Sx2:
		RexPrint("CmpGT32Sx2");
		return;
	case Iop_Cnt8x8:
		RexPrint("Cnt8x8");
		return;
	case Iop_Clz8Sx8:
		RexPrint("Clz8Sx8");
		return;
	case Iop_Clz16Sx4:
		RexPrint("Clz16Sx4");
		return;
	case Iop_Clz32Sx2:
		RexPrint("Clz32Sx2");
		return;
	case Iop_Cls8Sx8:
		RexPrint("Cls8Sx8");
		return;
	case Iop_Cls16Sx4:
		RexPrint("Cls16Sx4");
		return;
	case Iop_Cls32Sx2:
		RexPrint("Cls32Sx2");
		return;
	case Iop_ShlN8x8:
		RexPrint("ShlN8x8");
		return;
	case Iop_ShlN16x4:
		RexPrint("ShlN16x4");
		return;
	case Iop_ShlN32x2:
		RexPrint("ShlN32x2");
		return;
	case Iop_ShrN8x8:
		RexPrint("ShrN8x8");
		return;
	case Iop_ShrN16x4:
		RexPrint("ShrN16x4");
		return;
	case Iop_ShrN32x2:
		RexPrint("ShrN32x2");
		return;
	case Iop_SarN8x8:
		RexPrint("SarN8x8");
		return;
	case Iop_SarN16x4:
		RexPrint("SarN16x4");
		return;
	case Iop_SarN32x2:
		RexPrint("SarN32x2");
		return;
	case Iop_QNarrow16Ux4:
		RexPrint("QNarrow16Ux4");
		return;
	case Iop_QNarrow16Sx4:
		RexPrint("QNarrow16Sx4");
		return;
	case Iop_QNarrow32Sx2:
		RexPrint("QNarrow32Sx2");
		return;
	case Iop_InterleaveHI8x8:
		RexPrint("InterleaveHI8x8");
		return;
	case Iop_InterleaveHI16x4:
		RexPrint("InterleaveHI16x4");
		return;
	case Iop_InterleaveHI32x2:
		RexPrint("InterleaveHI32x2");
		return;
	case Iop_InterleaveLO8x8:
		RexPrint("InterleaveLO8x8");
		return;
	case Iop_InterleaveLO16x4:
		RexPrint("InterleaveLO16x4");
		return;
	case Iop_InterleaveLO32x2:
		RexPrint("InterleaveLO32x2");
		return;
	case Iop_CatOddLanes8x8:
		RexPrint("CatOddLanes8x8");
		return;
	case Iop_CatOddLanes16x4:
		RexPrint("CatOddLanes16x4");
		return;
	case Iop_CatEvenLanes8x8:
		RexPrint("CatEvenLanes8x8");
		return;
	case Iop_CatEvenLanes16x4:
		RexPrint("CatEvenLanes16x4");
		return;
	case Iop_InterleaveOddLanes8x8:
		RexPrint("InterleaveOddLanes8x8");
		return;
	case Iop_InterleaveOddLanes16x4:
		RexPrint("InterleaveOddLanes16x4");
		return;
	case Iop_InterleaveEvenLanes8x8:
		RexPrint("InterleaveEvenLanes8x8");
		return;
	case Iop_InterleaveEvenLanes16x4:
		RexPrint("InterleaveEvenLanes16x4");
		return;
	case Iop_Shl8x8:
		RexPrint("Shl8x8");
		return;
	case Iop_Shl16x4:
		RexPrint("Shl16x4");
		return;
	case Iop_Shl32x2:
		RexPrint("Shl32x2");
		return;
	case Iop_Shr8x8:
		RexPrint("Shr8x8");
		return;
	case Iop_Shr16x4:
		RexPrint("Shr16x4");
		return;
	case Iop_Shr32x2:
		RexPrint("Shr32x2");
		return;
	case Iop_QShl8x8:
		RexPrint("QShl8x8");
		return;
	case Iop_QShl16x4:
		RexPrint("QShl16x4");
		return;
	case Iop_QShl32x2:
		RexPrint("QShl32x2");
		return;
	case Iop_QShl64x1:
		RexPrint("QShl64x1");
		return;
	case Iop_QSal8x8:
		RexPrint("QSal8x8");
		return;
	case Iop_QSal16x4:
		RexPrint("QSal16x4");
		return;
	case Iop_QSal32x2:
		RexPrint("QSal32x2");
		return;
	case Iop_QSal64x1:
		RexPrint("QSal64x1");
		return;
	case Iop_QShlN8x8:
		RexPrint("QShlN8x8");
		return;
	case Iop_QShlN16x4:
		RexPrint("QShlN16x4");
		return;
	case Iop_QShlN32x2:
		RexPrint("QShlN32x2");
		return;
	case Iop_QShlN64x1:
		RexPrint("QShlN64x1");
		return;
	case Iop_QShlN8Sx8:
		RexPrint("QShlN8Sx8");
		return;
	case Iop_QShlN16Sx4:
		RexPrint("QShlN16Sx4");
		return;
	case Iop_QShlN32Sx2:
		RexPrint("QShlN32Sx2");
		return;
	case Iop_QShlN64Sx1:
		RexPrint("QShlN64Sx1");
		return;
	case Iop_QSalN8x8:
		RexPrint("QSalN8x8");
		return;
	case Iop_QSalN16x4:
		RexPrint("QSalN16x4");
		return;
	case Iop_QSalN32x2:
		RexPrint("QSalN32x2");
		return;
	case Iop_QSalN64x1:
		RexPrint("QSalN64x1");
		return;
	case Iop_Sar8x8:
		RexPrint("Sar8x8");
		return;
	case Iop_Sar16x4:
		RexPrint("Sar16x4");
		return;
	case Iop_Sar32x2:
		RexPrint("Sar32x2");
		return;
	case Iop_Sal8x8:
		RexPrint("Sal8x8");
		return;
	case Iop_Sal16x4:
		RexPrint("Sal16x4");
		return;
	case Iop_Sal32x2:
		RexPrint("Sal32x2");
		return;
	case Iop_Sal64x1:
		RexPrint("Sal64x1");
		return;
	case Iop_Perm8x8:
		RexPrint("Perm8x8");
		return;
	case Iop_Reverse16_8x8:
		RexPrint("Reverse16_8x8");
		return;
	case Iop_Reverse32_8x8:
		RexPrint("Reverse32_8x8");
		return;
	case Iop_Reverse32_16x4:
		RexPrint("Reverse32_16x4");
		return;
	case Iop_Reverse64_8x8:
		RexPrint("Reverse64_8x8");
		return;
	case Iop_Reverse64_16x4:
		RexPrint("Reverse64_16x4");
		return;
	case Iop_Reverse64_32x2:
		RexPrint("Reverse64_32x2");
		return;
	case Iop_Abs32Fx2:
		RexPrint("Abs32Fx2");
		return;

	case Iop_CmpNEZ32x2:
		RexPrint("CmpNEZ32x2");
		return;
	case Iop_CmpNEZ16x4:
		RexPrint("CmpNEZ16x4");
		return;
	case Iop_CmpNEZ8x8:
		RexPrint("CmpNEZ8x8");
		return;

	case Iop_Add32Fx4:
		RexPrint("Add32Fx4");
		return;
	case Iop_Add32Fx2:
		RexPrint("Add32Fx2");
		return;
	case Iop_Add32F0x4:
		RexPrint("Add32F0x4");
		return;
	case Iop_Add64Fx2:
		RexPrint("Add64Fx2");
		return;
	case Iop_Add64F0x2:
		RexPrint("Add64F0x2");
		return;

	case Iop_Div32Fx4:
		RexPrint("Div32Fx4");
		return;
	case Iop_Div32F0x4:
		RexPrint("Div32F0x4");
		return;
	case Iop_Div64Fx2:
		RexPrint("Div64Fx2");
		return;
	case Iop_Div64F0x2:
		RexPrint("Div64F0x2");
		return;

	case Iop_Max32Fx4:
		RexPrint("Max32Fx4");
		return;
	case Iop_Max32Fx2:
		RexPrint("Max32Fx2");
		return;
	case Iop_PwMax32Fx4:
		RexPrint("PwMax32Fx4");
		return;
	case Iop_PwMax32Fx2:
		RexPrint("PwMax32Fx2");
		return;
	case Iop_Max32F0x4:
		RexPrint("Max32F0x4");
		return;
	case Iop_Max64Fx2:
		RexPrint("Max64Fx2");
		return;
	case Iop_Max64F0x2:
		RexPrint("Max64F0x2");
		return;

	case Iop_Min32Fx4:
		RexPrint("Min32Fx4");
		return;
	case Iop_Min32Fx2:
		RexPrint("Min32Fx2");
		return;
	case Iop_PwMin32Fx4:
		RexPrint("PwMin32Fx4");
		return;
	case Iop_PwMin32Fx2:
		RexPrint("PwMin32Fx2");
		return;
	case Iop_Min32F0x4:
		RexPrint("Min32F0x4");
		return;
	case Iop_Min64Fx2:
		RexPrint("Min64Fx2");
		return;
	case Iop_Min64F0x2:
		RexPrint("Min64F0x2");
		return;

	case Iop_Mul32Fx4:
		RexPrint("Mul32Fx4");
		return;
	case Iop_Mul32F0x4:
		RexPrint("Mul32F0x4");
		return;
	case Iop_Mul64Fx2:
		RexPrint("Mul64Fx2");
		return;
	case Iop_Mul64F0x2:
		RexPrint("Mul64F0x2");
		return;

	case Iop_Recip32x2:
		RexPrint("Recip32x2");
		return;
	case Iop_Recip32Fx2:
		RexPrint("Recip32Fx2");
		return;
	case Iop_Recip32Fx4:
		RexPrint("Recip32Fx4");
		return;
	case Iop_Recip32x4:
		RexPrint("Recip32x4");
		return;
	case Iop_Recip32F0x4:
		RexPrint("Recip32F0x4");
		return;
	case Iop_Recip64Fx2:
		RexPrint("Recip64Fx2");
		return;
	case Iop_Recip64F0x2:
		RexPrint("Recip64F0x2");
		return;
	case Iop_Recps32Fx2:
		RexPrint("VRecps32Fx2");
		return;
	case Iop_Recps32Fx4:
		RexPrint("VRecps32Fx4");
		return;
	case Iop_Abs32Fx4:
		RexPrint("Abs32Fx4");
		return;
	case Iop_Rsqrts32Fx4:
		RexPrint("VRsqrts32Fx4");
		return;
	case Iop_Rsqrts32Fx2:
		RexPrint("VRsqrts32Fx2");
		return;

	case Iop_RSqrt32Fx4:
		RexPrint("RSqrt32Fx4");
		return;
	case Iop_RSqrt32F0x4:
		RexPrint("RSqrt32F0x4");
		return;
	case Iop_RSqrt64Fx2:
		RexPrint("RSqrt64Fx2");
		return;
	case Iop_RSqrt64F0x2:
		RexPrint("RSqrt64F0x2");
		return;

	case Iop_Sqrt32Fx4:
		RexPrint("Sqrt32Fx4");
		return;
	case Iop_Sqrt32F0x4:
		RexPrint("Sqrt32F0x4");
		return;
	case Iop_Sqrt64Fx2:
		RexPrint("Sqrt64Fx2");
		return;
	case Iop_Sqrt64F0x2:
		RexPrint("Sqrt64F0x2");
		return;

	case Iop_Sub32Fx4:
		RexPrint("Sub32Fx4");
		return;
	case Iop_Sub32Fx2:
		RexPrint("Sub32Fx2");
		return;
	case Iop_Sub32F0x4:
		RexPrint("Sub32F0x4");
		return;
	case Iop_Sub64Fx2:
		RexPrint("Sub64Fx2");
		return;
	case Iop_Sub64F0x2:
		RexPrint("Sub64F0x2");
		return;

	case Iop_CmpEQ32Fx4:
		RexPrint("CmpEQ32Fx4");
		return;
	case Iop_CmpLT32Fx4:
		RexPrint("CmpLT32Fx4");
		return;
	case Iop_CmpLE32Fx4:
		RexPrint("CmpLE32Fx4");
		return;
	case Iop_CmpGT32Fx4:
		RexPrint("CmpGT32Fx4");
		return;
	case Iop_CmpGE32Fx4:
		RexPrint("CmpGE32Fx4");
		return;
	case Iop_CmpUN32Fx4:
		RexPrint("CmpUN32Fx4");
		return;
	case Iop_CmpEQ64Fx2:
		RexPrint("CmpEQ64Fx2");
		return;
	case Iop_CmpLT64Fx2:
		RexPrint("CmpLT64Fx2");
		return;
	case Iop_CmpLE64Fx2:
		RexPrint("CmpLE64Fx2");
		return;
	case Iop_CmpUN64Fx2:
		RexPrint("CmpUN64Fx2");
		return;
	case Iop_CmpGT32Fx2:
		RexPrint("CmpGT32Fx2");
		return;
	case Iop_CmpEQ32Fx2:
		RexPrint("CmpEQ32Fx2");
		return;
	case Iop_CmpGE32Fx2:
		RexPrint("CmpGE32Fx2");
		return;

	case Iop_CmpEQ32F0x4:
		RexPrint("CmpEQ32F0x4");
		return;
	case Iop_CmpLT32F0x4:
		RexPrint("CmpLT32F0x4");
		return;
	case Iop_CmpLE32F0x4:
		RexPrint("CmpLE32F0x4");
		return;
	case Iop_CmpUN32F0x4:
		RexPrint("CmpUN32F0x4");
		return;
	case Iop_CmpEQ64F0x2:
		RexPrint("CmpEQ64F0x2");
		return;
	case Iop_CmpLT64F0x2:
		RexPrint("CmpLT64F0x2");
		return;
	case Iop_CmpLE64F0x2:
		RexPrint("CmpLE64F0x2");
		return;
	case Iop_CmpUN64F0x2:
		RexPrint("CmpUN64F0x2");
		return;

	case Iop_Neg32Fx4:
		RexPrint("Neg32Fx4");
		return;
	case Iop_Neg32Fx2:
		RexPrint("Neg32Fx2");
		return;

	case Iop_V128to64:
		RexPrint("V128to64");
		return;
	case Iop_V128HIto64:
		RexPrint("V128HIto64");
		return;
	case Iop_64HLtoV128:
		RexPrint("64HLtoV128");
		return;

	case Iop_64UtoV128:
		RexPrint("64UtoV128");
		return;
	case Iop_SetV128lo64:
		RexPrint("SetV128lo64");
		return;

	case Iop_32UtoV128:
		RexPrint("32UtoV128");
		return;
	case Iop_V128to32:
		RexPrint("V128to32");
		return;
	case Iop_SetV128lo32:
		RexPrint("SetV128lo32");
		return;

	case Iop_Dup8x16:
		RexPrint("Dup8x16");
		return;
	case Iop_Dup16x8:
		RexPrint("Dup16x8");
		return;
	case Iop_Dup32x4:
		RexPrint("Dup32x4");
		return;
	case Iop_Dup8x8:
		RexPrint("Dup8x8");
		return;
	case Iop_Dup16x4:
		RexPrint("Dup16x4");
		return;
	case Iop_Dup32x2:
		RexPrint("Dup32x2");
		return;

	case Iop_NotV128:
		RexPrint("NotV128");
		return;
	case Iop_AndV128:
		RexPrint("AndV128");
		return;
	case Iop_OrV128:
		RexPrint("OrV128");
		return;
	case Iop_XorV128:
		RexPrint("XorV128");
		return;

	case Iop_CmpNEZ8x16:
		RexPrint("CmpNEZ8x16");
		return;
	case Iop_CmpNEZ16x8:
		RexPrint("CmpNEZ16x8");
		return;
	case Iop_CmpNEZ32x4:
		RexPrint("CmpNEZ32x4");
		return;
	case Iop_CmpNEZ64x2:
		RexPrint("CmpNEZ64x2");
		return;

	case Iop_Abs8x16:
		RexPrint("Abs8x16");
		return;
	case Iop_Abs16x8:
		RexPrint("Abs16x8");
		return;
	case Iop_Abs32x4:
		RexPrint("Abs32x4");
		return;

	case Iop_Add8x16:
		RexPrint("Add8x16");
		return;
	case Iop_Add16x8:
		RexPrint("Add16x8");
		return;
	case Iop_Add32x4:
		RexPrint("Add32x4");
		return;
	case Iop_Add64x2:
		RexPrint("Add64x2");
		return;
	case Iop_QAdd8Ux16:
		RexPrint("QAdd8Ux16");
		return;
	case Iop_QAdd16Ux8:
		RexPrint("QAdd16Ux8");
		return;
	case Iop_QAdd32Ux4:
		RexPrint("QAdd32Ux4");
		return;
	case Iop_QAdd8Sx16:
		RexPrint("QAdd8Sx16");
		return;
	case Iop_QAdd16Sx8:
		RexPrint("QAdd16Sx8");
		return;
	case Iop_QAdd32Sx4:
		RexPrint("QAdd32Sx4");
		return;
	case Iop_QAdd64Ux2:
		RexPrint("QAdd64Ux2");
		return;
	case Iop_QAdd64Sx2:
		RexPrint("QAdd64Sx2");
		return;
	case Iop_PwAdd8x16:
		RexPrint("PwAdd8x16");
		return;
	case Iop_PwAdd16x8:
		RexPrint("PwAdd16x8");
		return;
	case Iop_PwAdd32x4:
		RexPrint("PwAdd32x4");
		return;
	case Iop_PwAddL8Ux16:
		RexPrint("PwAddL8Ux16");
		return;
	case Iop_PwAddL16Ux8:
		RexPrint("PwAddL16Ux8");
		return;
	case Iop_PwAddL32Ux4:
		RexPrint("PwAddL32Ux4");
		return;
	case Iop_PwAddL8Sx16:
		RexPrint("PwAddL8Sx16");
		return;
	case Iop_PwAddL16Sx8:
		RexPrint("PwAddL16Sx8");
		return;
	case Iop_PwAddL32Sx4:
		RexPrint("PwAddL32Sx4");
		return;

	case Iop_Sub8x16:
		RexPrint("Sub8x16");
		return;
	case Iop_Sub16x8:
		RexPrint("Sub16x8");
		return;
	case Iop_Sub32x4:
		RexPrint("Sub32x4");
		return;
	case Iop_Sub64x2:
		RexPrint("Sub64x2");
		return;
	case Iop_QSub8Ux16:
		RexPrint("QSub8Ux16");
		return;
	case Iop_QSub16Ux8:
		RexPrint("QSub16Ux8");
		return;
	case Iop_QSub32Ux4:
		RexPrint("QSub32Ux4");
		return;
	case Iop_QSub8Sx16:
		RexPrint("QSub8Sx16");
		return;
	case Iop_QSub16Sx8:
		RexPrint("QSub16Sx8");
		return;
	case Iop_QSub32Sx4:
		RexPrint("QSub32Sx4");
		return;
	case Iop_QSub64Ux2:
		RexPrint("QSub64Ux2");
		return;
	case Iop_QSub64Sx2:
		RexPrint("QSub64Sx2");
		return;

	case Iop_Mul8x16:
		RexPrint("Mul8x16");
		return;
	case Iop_Mul16x8:
		RexPrint("Mul16x8");
		return;
	case Iop_Mul32x4:
		RexPrint("Mul32x4");
		return;
	case Iop_Mull8Ux8:
		RexPrint("Mull8Ux8");
		return;
	case Iop_Mull8Sx8:
		RexPrint("Mull8Sx8");
		return;
	case Iop_Mull16Ux4:
		RexPrint("Mull16Ux4");
		return;
	case Iop_Mull16Sx4:
		RexPrint("Mull16Sx4");
		return;
	case Iop_Mull32Ux2:
		RexPrint("Mull32Ux2");
		return;
	case Iop_Mull32Sx2:
		RexPrint("Mull32Sx2");
		return;
	case Iop_PolynomialMul8x16:
		RexPrint("PolynomialMul8x16");
		return;
	case Iop_PolynomialMull8x8:
		RexPrint("PolynomialMull8x8");
		return;
	case Iop_MulHi16Ux8:
		RexPrint("MulHi16Ux8");
		return;
	case Iop_MulHi32Ux4:
		RexPrint("MulHi32Ux4");
		return;
	case Iop_MulHi16Sx8:
		RexPrint("MulHi16Sx8");
		return;
	case Iop_MulHi32Sx4:
		RexPrint("MulHi32Sx4");
		return;
	case Iop_QDMulHi16Sx8:
		RexPrint("QDMulHi16Sx8");
		return;
	case Iop_QDMulHi32Sx4:
		RexPrint("QDMulHi32Sx4");
		return;
	case Iop_QRDMulHi16Sx8:
		RexPrint("QRDMulHi16Sx8");
		return;
	case Iop_QRDMulHi32Sx4:
		RexPrint("QRDMulHi32Sx4");
		return;

	case Iop_MullEven8Ux16:
		RexPrint("MullEven8Ux16");
		return;
	case Iop_MullEven16Ux8:
		RexPrint("MullEven16Ux8");
		return;
	case Iop_MullEven8Sx16:
		RexPrint("MullEven8Sx16");
		return;
	case Iop_MullEven16Sx8:
		RexPrint("MullEven16Sx8");
		return;

	case Iop_Avg8Ux16:
		RexPrint("Avg8Ux16");
		return;
	case Iop_Avg16Ux8:
		RexPrint("Avg16Ux8");
		return;
	case Iop_Avg32Ux4:
		RexPrint("Avg32Ux4");
		return;
	case Iop_Avg8Sx16:
		RexPrint("Avg8Sx16");
		return;
	case Iop_Avg16Sx8:
		RexPrint("Avg16Sx8");
		return;
	case Iop_Avg32Sx4:
		RexPrint("Avg32Sx4");
		return;

	case Iop_Max8Sx16:
		RexPrint("Max8Sx16");
		return;
	case Iop_Max16Sx8:
		RexPrint("Max16Sx8");
		return;
	case Iop_Max32Sx4:
		RexPrint("Max32Sx4");
		return;
	case Iop_Max8Ux16:
		RexPrint("Max8Ux16");
		return;
	case Iop_Max16Ux8:
		RexPrint("Max16Ux8");
		return;
	case Iop_Max32Ux4:
		RexPrint("Max32Ux4");
		return;

	case Iop_Min8Sx16:
		RexPrint("Min8Sx16");
		return;
	case Iop_Min16Sx8:
		RexPrint("Min16Sx8");
		return;
	case Iop_Min32Sx4:
		RexPrint("Min32Sx4");
		return;
	case Iop_Min8Ux16:
		RexPrint("Min8Ux16");
		return;
	case Iop_Min16Ux8:
		RexPrint("Min16Ux8");
		return;
	case Iop_Min32Ux4:
		RexPrint("Min32Ux4");
		return;

	case Iop_CmpEQ8x16:
		RexPrint("CmpEQ8x16");
		return;
	case Iop_CmpEQ16x8:
		RexPrint("CmpEQ16x8");
		return;
	case Iop_CmpEQ32x4:
		RexPrint("CmpEQ32x4");
		return;
	case Iop_CmpGT8Sx16:
		RexPrint("CmpGT8Sx16");
		return;
	case Iop_CmpGT16Sx8:
		RexPrint("CmpGT16Sx8");
		return;
	case Iop_CmpGT32Sx4:
		RexPrint("CmpGT32Sx4");
		return;
	case Iop_CmpGT64Sx2:
		RexPrint("CmpGT64Sx2");
		return;
	case Iop_CmpGT8Ux16:
		RexPrint("CmpGT8Ux16");
		return;
	case Iop_CmpGT16Ux8:
		RexPrint("CmpGT16Ux8");
		return;
	case Iop_CmpGT32Ux4:
		RexPrint("CmpGT32Ux4");
		return;

	case Iop_Cnt8x16:
		RexPrint("Cnt8x16");
		return;
	case Iop_Clz8Sx16:
		RexPrint("Clz8Sx16");
		return;
	case Iop_Clz16Sx8:
		RexPrint("Clz16Sx8");
		return;
	case Iop_Clz32Sx4:
		RexPrint("Clz32Sx4");
		return;
	case Iop_Cls8Sx16:
		RexPrint("Cls8Sx16");
		return;
	case Iop_Cls16Sx8:
		RexPrint("Cls16Sx8");
		return;
	case Iop_Cls32Sx4:
		RexPrint("Cls32Sx4");
		return;

	case Iop_ShlV128:
		RexPrint("ShlV128");
		return;
	case Iop_ShrV128:
		RexPrint("ShrV128");
		return;

	case Iop_ShlN8x16:
		RexPrint("ShlN8x16");
		return;
	case Iop_ShlN16x8:
		RexPrint("ShlN16x8");
		return;
	case Iop_ShlN32x4:
		RexPrint("ShlN32x4");
		return;
	case Iop_ShlN64x2:
		RexPrint("ShlN64x2");
		return;
	case Iop_ShrN8x16:
		RexPrint("ShrN8x16");
		return;
	case Iop_ShrN16x8:
		RexPrint("ShrN16x8");
		return;
	case Iop_ShrN32x4:
		RexPrint("ShrN32x4");
		return;
	case Iop_ShrN64x2:
		RexPrint("ShrN64x2");
		return;
	case Iop_SarN8x16:
		RexPrint("SarN8x16");
		return;
	case Iop_SarN16x8:
		RexPrint("SarN16x8");
		return;
	case Iop_SarN32x4:
		RexPrint("SarN32x4");
		return;
	case Iop_SarN64x2:
		RexPrint("SarN64x2");
		return;

	case Iop_Shl8x16:
		RexPrint("Shl8x16");
		return;
	case Iop_Shl16x8:
		RexPrint("Shl16x8");
		return;
	case Iop_Shl32x4:
		RexPrint("Shl32x4");
		return;
	case Iop_Shl64x2:
		RexPrint("Shl64x2");
		return;
	case Iop_QSal8x16:
		RexPrint("QSal8x16");
		return;
	case Iop_QSal16x8:
		RexPrint("QSal16x8");
		return;
	case Iop_QSal32x4:
		RexPrint("QSal32x4");
		return;
	case Iop_QSal64x2:
		RexPrint("QSal64x2");
		return;
	case Iop_QShl8x16:
		RexPrint("QShl8x16");
		return;
	case Iop_QShl16x8:
		RexPrint("QShl16x8");
		return;
	case Iop_QShl32x4:
		RexPrint("QShl32x4");
		return;
	case Iop_QShl64x2:
		RexPrint("QShl64x2");
		return;
	case Iop_QSalN8x16:
		RexPrint("QSalN8x16");
		return;
	case Iop_QSalN16x8:
		RexPrint("QSalN16x8");
		return;
	case Iop_QSalN32x4:
		RexPrint("QSalN32x4");
		return;
	case Iop_QSalN64x2:
		RexPrint("QSalN64x2");
		return;
	case Iop_QShlN8x16:
		RexPrint("QShlN8x16");
		return;
	case Iop_QShlN16x8:
		RexPrint("QShlN16x8");
		return;
	case Iop_QShlN32x4:
		RexPrint("QShlN32x4");
		return;
	case Iop_QShlN64x2:
		RexPrint("QShlN64x2");
		return;
	case Iop_QShlN8Sx16:
		RexPrint("QShlN8Sx16");
		return;
	case Iop_QShlN16Sx8:
		RexPrint("QShlN16Sx8");
		return;
	case Iop_QShlN32Sx4:
		RexPrint("QShlN32Sx4");
		return;
	case Iop_QShlN64Sx2:
		RexPrint("QShlN64Sx2");
		return;
	case Iop_Shr8x16:
		RexPrint("Shr8x16");
		return;
	case Iop_Shr16x8:
		RexPrint("Shr16x8");
		return;
	case Iop_Shr32x4:
		RexPrint("Shr32x4");
		return;
	case Iop_Shr64x2:
		RexPrint("Shr64x2");
		return;
	case Iop_Sar8x16:
		RexPrint("Sar8x16");
		return;
	case Iop_Sar16x8:
		RexPrint("Sar16x8");
		return;
	case Iop_Sar32x4:
		RexPrint("Sar32x4");
		return;
	case Iop_Sar64x2:
		RexPrint("Sar64x2");
		return;
	case Iop_Sal8x16:
		RexPrint("Sal8x16");
		return;
	case Iop_Sal16x8:
		RexPrint("Sal16x8");
		return;
	case Iop_Sal32x4:
		RexPrint("Sal32x4");
		return;
	case Iop_Sal64x2:
		RexPrint("Sal64x2");
		return;
	case Iop_Rol8x16:
		RexPrint("Rol8x16");
		return;
	case Iop_Rol16x8:
		RexPrint("Rol16x8");
		return;
	case Iop_Rol32x4:
		RexPrint("Rol32x4");
		return;

	case Iop_Narrow16x8:
		RexPrint("Narrow16x8");
		return;
	case Iop_Narrow32x4:
		RexPrint("Narrow32x4");
		return;
	case Iop_QNarrow16Ux8:
		RexPrint("QNarrow16Ux8");
		return;
	case Iop_QNarrow32Ux4:
		RexPrint("QNarrow32Ux4");
		return;
	case Iop_QNarrow16Sx8:
		RexPrint("QNarrow16Sx8");
		return;
	case Iop_QNarrow32Sx4:
		RexPrint("QNarrow32Sx4");
		return;
	case Iop_Shorten16x8:
		RexPrint("Shorten16x8");
		return;
	case Iop_Shorten32x4:
		RexPrint("Shorten32x4");
		return;
	case Iop_Shorten64x2:
		RexPrint("Shorten64x2");
		return;
	case Iop_QShortenU16Ux8:
		RexPrint("QShortenU16Ux8");
		return;
	case Iop_QShortenU32Ux4:
		RexPrint("QShortenU32Ux4");
		return;
	case Iop_QShortenU64Ux2:
		RexPrint("QShortenU64Ux2");
		return;
	case Iop_QShortenS16Sx8:
		RexPrint("QShortenS16Sx8");
		return;
	case Iop_QShortenS32Sx4:
		RexPrint("QShortenS32Sx4");
		return;
	case Iop_QShortenS64Sx2:
		RexPrint("QShortenS64Sx2");
		return;
	case Iop_QShortenU16Sx8:
		RexPrint("QShortenU16Sx8");
		return;
	case Iop_QShortenU32Sx4:
		RexPrint("QShortenU32Sx4");
		return;
	case Iop_QShortenU64Sx2:
		RexPrint("QShortenU64Sx2");
		return;
	case Iop_Longen8Ux8:
		RexPrint("Longen8Ux8");
		return;
	case Iop_Longen16Ux4:
		RexPrint("Longen16Ux4");
		return;
	case Iop_Longen32Ux2:
		RexPrint("Longen32Ux2");
		return;
	case Iop_Longen8Sx8:
		RexPrint("Longen8Sx8");
		return;
	case Iop_Longen16Sx4:
		RexPrint("Longen16Sx4");
		return;
	case Iop_Longen32Sx2:
		RexPrint("Longen32Sx2");
		return;

	case Iop_InterleaveHI8x16:
		RexPrint("InterleaveHI8x16");
		return;
	case Iop_InterleaveHI16x8:
		RexPrint("InterleaveHI16x8");
		return;
	case Iop_InterleaveHI32x4:
		RexPrint("InterleaveHI32x4");
		return;
	case Iop_InterleaveHI64x2:
		RexPrint("InterleaveHI64x2");
		return;
	case Iop_InterleaveLO8x16:
		RexPrint("InterleaveLO8x16");
		return;
	case Iop_InterleaveLO16x8:
		RexPrint("InterleaveLO16x8");
		return;
	case Iop_InterleaveLO32x4:
		RexPrint("InterleaveLO32x4");
		return;
	case Iop_InterleaveLO64x2:
		RexPrint("InterleaveLO64x2");
		return;

	case Iop_CatOddLanes8x16:
		RexPrint("CatOddLanes8x16");
		return;
	case Iop_CatOddLanes16x8:
		RexPrint("CatOddLanes16x8");
		return;
	case Iop_CatOddLanes32x4:
		RexPrint("CatOddLanes32x4");
		return;
	case Iop_CatEvenLanes8x16:
		RexPrint("CatEvenLanes8x16");
		return;
	case Iop_CatEvenLanes16x8:
		RexPrint("CatEvenLanes16x8");
		return;
	case Iop_CatEvenLanes32x4:
		RexPrint("CatEvenLanes32x4");
		return;

	case Iop_InterleaveOddLanes8x16:
		RexPrint("InterleaveOddLanes8x16");
		return;
	case Iop_InterleaveOddLanes16x8:
		RexPrint("InterleaveOddLanes16x8");
		return;
	case Iop_InterleaveOddLanes32x4:
		RexPrint("InterleaveOddLanes32x4");
		return;
	case Iop_InterleaveEvenLanes8x16:
		RexPrint("InterleaveEvenLanes8x16");
		return;
	case Iop_InterleaveEvenLanes16x8:
		RexPrint("InterleaveEvenLanes16x8");
		return;
	case Iop_InterleaveEvenLanes32x4:
		RexPrint("InterleaveEvenLanes32x4");
		return;

	case Iop_GetElem8x16:
		RexPrint("GetElem8x16");
		return;
	case Iop_GetElem16x8:
		RexPrint("GetElem16x8");
		return;
	case Iop_GetElem32x4:
		RexPrint("GetElem32x4");
		return;
	case Iop_GetElem64x2:
		RexPrint("GetElem64x2");
		return;

	case Iop_GetElem8x8:
		RexPrint("GetElem8x8");
		return;
	case Iop_GetElem16x4:
		RexPrint("GetElem16x4");
		return;
	case Iop_GetElem32x2:
		RexPrint("GetElem32x2");
		return;
	case Iop_SetElem8x8:
		RexPrint("SetElem8x8");
		return;
	case Iop_SetElem16x4:
		RexPrint("SetElem16x4");
		return;
	case Iop_SetElem32x2:
		RexPrint("SetElem32x2");
		return;

	case Iop_Extract64:
		RexPrint("Extract64");
		return;
	case Iop_ExtractV128:
		RexPrint("ExtractV128");
		return;

	case Iop_Perm8x16:
		RexPrint("Perm8x16");
		return;
	case Iop_Reverse16_8x16:
		RexPrint("Reverse16_8x16");
		return;
	case Iop_Reverse32_8x16:
		RexPrint("Reverse32_8x16");
		return;
	case Iop_Reverse32_16x8:
		RexPrint("Reverse32_16x8");
		return;
	case Iop_Reverse64_8x16:
		RexPrint("Reverse64_8x16");
		return;
	case Iop_Reverse64_16x8:
		RexPrint("Reverse64_16x8");
		return;
	case Iop_Reverse64_32x4:
		RexPrint("Reverse64_32x4");
		return;

	case Iop_F32ToFixed32Ux4_RZ:
		RexPrint("F32ToFixed32Ux4_RZ");
		return;
	case Iop_F32ToFixed32Sx4_RZ:
		RexPrint("F32ToFixed32Sx4_RZ");
		return;
	case Iop_Fixed32UToF32x4_RN:
		RexPrint("Fixed32UToF32x4_RN");
		return;
	case Iop_Fixed32SToF32x4_RN:
		RexPrint("Fixed32SToF32x4_RN");
		return;
	case Iop_F32ToFixed32Ux2_RZ:
		RexPrint("F32ToFixed32Ux2_RZ");
		return;
	case Iop_F32ToFixed32Sx2_RZ:
		RexPrint("F32ToFixed32Sx2_RZ");
		return;
	case Iop_Fixed32UToF32x2_RN:
		RexPrint("Fixed32UToF32x2_RN");
		return;
	case Iop_Fixed32SToF32x2_RN:
		RexPrint("Fixed32SToF32x2_RN");
		return;

	default:
		throw "RexPPIROp(1)";
	}

	assert(str);
	switch (op - base) {
	case 0:
		RexPrint("%s",str);
		RexPrint("8");
		break;
	case 1:
		RexPrint("%s",str);
		RexPrint("16");
		break;
	case 2:
		RexPrint("%s",str);
		RexPrint("32");
		break;
	case 3:
		RexPrint("%s",str);
		RexPrint("64");
		break;
	default:
		throw "RexPPIROp(2)";
	}
}

void RexUtils::RexPPIRExpr ( const IRExpr *const e )
{
	Int i;
	switch (e->tag) {
	case Iex_Binder:
		RexPrint("BIND-%d", e->Iex.Binder.binder);
		break;
	case Iex_Get:
		RexPrint( "GET:" );
		RexPPIRType(e->Iex.Get.ty);
		RexPrint("(%d)", e->Iex.Get.offset);
		break;
	case Iex_GetI:
		RexPrint( "GETI" );
		RexPPIRRegArray(e->Iex.GetI.descr);
		RexPrint("[");
		RexPPIRExpr(e->Iex.GetI.ix);
		RexPrint(",%d]", e->Iex.GetI.bias);
		break;
	case Iex_RdTmp:
		RexPPIRTemp(e->Iex.RdTmp.tmp);
		break;
	case Iex_Qop:
		RexPPIROp(e->Iex.Qop.op);
		RexPrint( "(" );
		RexPPIRExpr(e->Iex.Qop.arg1);
		RexPrint( "," );
		RexPPIRExpr(e->Iex.Qop.arg2);
		RexPrint( "," );
		RexPPIRExpr(e->Iex.Qop.arg3);
		RexPrint( "," );
		RexPPIRExpr(e->Iex.Qop.arg4);
		RexPrint( ")" );
		break;
	case Iex_Triop:
		RexPPIROp(e->Iex.Triop.op);
		RexPrint( "(" );
		RexPPIRExpr(e->Iex.Triop.arg1);
		RexPrint( "," );
		RexPPIRExpr(e->Iex.Triop.arg2);
		RexPrint( "," );
		RexPPIRExpr(e->Iex.Triop.arg3);
		RexPrint( ")" );
		break;
	case Iex_Binop:
		RexPPIROp(e->Iex.Binop.op);
		RexPrint( "(" );
		RexPPIRExpr(e->Iex.Binop.arg1);
		RexPrint( "," );
		RexPPIRExpr(e->Iex.Binop.arg2);
		RexPrint( ")" );
		break;
	case Iex_Unop:
		RexPPIROp(e->Iex.Unop.op);
		RexPrint( "(" );
		RexPPIRExpr(e->Iex.Unop.arg);
		RexPrint( ")" );
		break;
	case Iex_Load:
		RexPrint( "LD%s:", e->Iex.Load.end==Iend_LE ? "le" : "be" );
		RexPPIRType(e->Iex.Load.ty);
		RexPrint( "(" );
		RexPPIRExpr(e->Iex.Load.addr);
		RexPrint( ")" );
		break;
	case Iex_Const:
		RexPPIRConst(e->Iex.Const.con);
		break;
	case Iex_CCall:
		RexPPIRCallee(e->Iex.CCall.cee);
		RexPrint("(");
		for (i = 0; e->Iex.CCall.args[i] != NULL; i++) {
			RexPPIRExpr(e->Iex.CCall.args[i]);
			if (e->Iex.CCall.args[i+1] != NULL)
				RexPrint(",");
		}
		RexPrint("):");
		RexPPIRType(e->Iex.CCall.retty);
		break;
	case Iex_Mux0X:
		RexPrint("Mux0X(");
		RexPPIRExpr(e->Iex.Mux0X.cond);
		RexPrint(",");
		RexPPIRExpr(e->Iex.Mux0X.expr0);
		RexPrint(",");
		RexPPIRExpr(e->Iex.Mux0X.exprX);
		RexPrint(")");
		break;
	default:
		throw "RexPPIRExpr";
	}
}

void RexUtils::RexPPIREffect ( const IREffect fx )
{
	switch (fx) {
	case Ifx_None:
		RexPrint("noFX");
		return;
	case Ifx_Read:
		RexPrint("RdFX");
		return;
	case Ifx_Write:
		RexPrint("WrFX");
		return;
	case Ifx_Modify:
		RexPrint("MoFX");
		return;
	default:
		throw "RexPPIREffect";
	}
}

void RexUtils::RexPPIRDirty ( const IRDirty *const d )
{
	Int i;
	if (d->tmp != IRTemp_INVALID) {
		RexPPIRTemp(d->tmp);
		RexPrint(" = ");
	}
	RexPrint("DIRTY ");
	RexPPIRExpr(d->guard);
	if (d->needsBBP)
		RexPrint(" NeedsBBP");
	if (d->mFx != Ifx_None) {
		RexPrint(" ");
		RexPPIREffect(d->mFx);
		RexPrint("-mem(");
		RexPPIRExpr(d->mAddr);
		RexPrint(",%d)", d->mSize);
	}
	for (i = 0; i < d->nFxState; i++) {
		RexPrint(" ");
		RexPPIREffect(d->fxState[i].fx);
		RexPrint("-gst(%d,%d)", d->fxState[i].offset, d->fxState[i].size);
	}
	RexPrint(" ::: ");
	RexPPIRCallee(d->cee);
	RexPrint("(");
	for (i = 0; d->args[i] != NULL; i++) {
		RexPPIRExpr(d->args[i]);
		if (d->args[i+1] != NULL) {
			RexPrint(",");
		}
	}
	RexPrint(")");
}

void RexUtils::RexPPIRCAS ( const IRCAS *const cas )
{
	/* Print even structurally invalid constructions, as an aid to
	   debugging. */
	if (cas->oldHi != IRTemp_INVALID) {
		RexPPIRTemp(cas->oldHi);
		RexPrint(",");
	}
	RexPPIRTemp(cas->oldLo);
	RexPrint(" = CAS%s(", cas->end==Iend_LE ? "le" : "be" );
	RexPPIRExpr(cas->addr);
	RexPrint("::");
	if (cas->expdHi) {
		RexPPIRExpr(cas->expdHi);
		RexPrint(",");
	}
	RexPPIRExpr(cas->expdLo);
	RexPrint("->");
	if (cas->dataHi) {
		RexPPIRExpr(cas->dataHi);
		RexPrint(",");
	}
	RexPPIRExpr(cas->dataLo);
	RexPrint(")");
}

void RexUtils::RexPPIRJumpKind ( const IRJumpKind kind )
{
	switch (kind) {
	case Ijk_Boring:
		RexPrint("Boring");
		break;
	case Ijk_Call:
		RexPrint("Call");
		break;
	case Ijk_Ret:
		RexPrint("Return");
		break;
	case Ijk_ClientReq:
		RexPrint("ClientReq");
		break;
	case Ijk_Yield:
		RexPrint("Yield");
		break;
	case Ijk_EmWarn:
		RexPrint("EmWarn");
		break;
	case Ijk_EmFail:
		RexPrint("EmFail");
		break;
	case Ijk_NoDecode:
		RexPrint("NoDecode");
		break;
	case Ijk_MapFail:
		RexPrint("MapFail");
		break;
	case Ijk_TInval:
		RexPrint("Invalidate");
		break;
	case Ijk_NoRedir:
		RexPrint("NoRedir");
		break;
	case Ijk_SigTRAP:
		RexPrint("SigTRAP");
		break;
	case Ijk_SigSEGV:
		RexPrint("SigSEGV");
		break;
	case Ijk_SigBUS:
		RexPrint("SigBUS");
		break;
	case Ijk_Sys_syscall:
		RexPrint("Sys_syscall");
		break;
	case Ijk_Sys_int32:
		RexPrint("Sys_int32");
		break;
	case Ijk_Sys_int128:
		RexPrint("Sys_int128");
		break;
	case Ijk_Sys_int129:
		RexPrint("Sys_int129");
		break;
	case Ijk_Sys_int130:
		RexPrint("Sys_int130");
		break;
	case Ijk_Sys_sysenter:
		RexPrint("Sys_sysenter");
		break;
	default:
		throw "RexPPIRJumpKind";
	}
}

void RexUtils::RexPPIRMBusEvent ( const IRMBusEvent event )
{
	switch (event) {
	case Imbe_Fence:
		RexPrint("Fence");
		break;
	default:
		throw "RexPPIRMBusEvent";
	}
}

void RexUtils::RexPPIRStmt ( const IRStmt *const s )
{
	if (!s) {
		RexPrint("!!! IRStmt* which is NULL !!!");
		return;
	}
	switch (s->tag) {
	case Ist_NoOp:
		RexPrint("IR-NoOp");
		break;
	case Ist_IMark:
		RexPrint( "------ IMark(0x%llx, %d) ------",
		            s->Ist.IMark.addr, s->Ist.IMark.len);
		break;
	case Ist_AbiHint:
		RexPrint("====== AbiHint(");
		RexPPIRExpr(s->Ist.AbiHint.base);
		RexPrint(", %d, ", s->Ist.AbiHint.len);
		RexPPIRExpr(s->Ist.AbiHint.nia);
		RexPrint(") ======");
		break;
	case Ist_Put:
		RexPrint("Ist_Put\t");
		RexPrint( "PUT(%d) = ", s->Ist.Put.offset);
		RexPPIRExpr(s->Ist.Put.data);
		break;
	case Ist_PutI:
		RexPrint("Ist_PutI\t");
		RexPrint( "PUTI" );
		RexPPIRRegArray(s->Ist.PutI.descr);
		RexPrint("[");
		RexPPIRExpr(s->Ist.PutI.ix);
		RexPrint(",%d] = ", s->Ist.PutI.bias);
		RexPPIRExpr(s->Ist.PutI.data);
		break;
	case Ist_WrTmp:
		RexPrint("Ist_WrTmp\t");
		RexPPIRTemp(s->Ist.WrTmp.tmp);
		RexPrint( " = " );
		RexPPIRExpr(s->Ist.WrTmp.data);
		break;
	case Ist_Store:
		RexPrint("Ist_Store\t");
		RexPrint( "ST%s(", s->Ist.Store.end==Iend_LE ? "le" : "be" );
		RexPPIRExpr(s->Ist.Store.addr);
		RexPrint( ") = ");
		RexPPIRExpr(s->Ist.Store.data);
		break;
	case Ist_CAS:
		RexPrint("Ist_CAS\t");
		RexPPIRCAS(s->Ist.CAS.details);
		break;
	case Ist_LLSC:
		// Load-link and store-conditional are a pair of instructions that
		// together implement a lock-free atomic read-modify-write operation.
		//
		// Load-link returns the current value of a memory location. A
		// subsequent store-conditional to the same memory location will
		// store a new value only if no updates have occurred to that
		// location since the load-link.
		RexPrint("IST_LLSC\t");
		if (s->Ist.LLSC.storedata == NULL) {
			RexPPIRTemp(s->Ist.LLSC.result);
			RexPrint(" = LD%s-Linked(",
			           s->Ist.LLSC.end==Iend_LE ? "le" : "be");
			RexPPIRExpr(s->Ist.LLSC.addr);
			RexPrint(")");
		} else {
			RexPPIRTemp(s->Ist.LLSC.result);
			RexPrint(" = ( ST%s-Cond(",
			           s->Ist.LLSC.end==Iend_LE ? "le" : "be");
			RexPPIRExpr(s->Ist.LLSC.addr);
			RexPrint(") = ");
			RexPPIRExpr(s->Ist.LLSC.storedata);
			RexPrint(" )");
		}
		break;
	case Ist_Dirty:
		RexPrint("Ist_Dirty\t");
		RexPPIRDirty(s->Ist.Dirty.details);
		break;
	case Ist_MBE:
		RexPrint("Ist_MBE\t");
		RexPrint("IR-");
		RexPPIRMBusEvent(s->Ist.MBE.event);
		break;
	case Ist_Exit:
		RexPrint("Ist_Exit\t");
		RexPrint( "if (" );
		RexPPIRExpr(s->Ist.Exit.guard);
		RexPrint( ") goto {");
		RexPPIRJumpKind(s->Ist.Exit.jk);
		RexPrint("} ");
		RexPPIRConst(s->Ist.Exit.dst);
		break;
	default:
		throw "RexPPIRStmt";
	}
}

void RexUtils::RexPPIRTypeEnv ( const IRTypeEnv *const env ) {
	UInt i;
	for (i = 0; i < env->types_used; i++) {
		if (i % 8 == 0)
			RexPrint( "   ");
		RexPPIRTemp(i);
		RexPrint( ":");
		RexPPIRType(env->types[i]);
		if (i % 8 == 7)
			RexPrint( "\n");
		else
			RexPrint( "   ");
	}
	if (env->types_used > 0 && env->types_used % 8 != 7)
		RexPrint( "\n");
}

void RexUtils::RexPPIRSB ( const IRSB *const bb )
{
	Int i;
	RexPrint("IRSB {\n");
	RexPPIRTypeEnv(bb->tyenv);
	RexPrint("\n");
	for (i = 0; i < bb->stmts_used; i++) {
		RexPrint( "%d   ", i);
		RexPPIRStmt(bb->stmts[i]);
		RexPrint( "\n");
	}
	RexPrint( "   goto {");
	RexPPIRJumpKind(bb->jumpkind);
	RexPrint( "} ");
	RexPPIRExpr( bb->next );
	RexPrint( "\n}\n");
}


/*---------------------------------------------------------------*/
/*--- Constructors                                            ---*/
/*---------------------------------------------------------------*/


/* Constructors -- IRConst */

IRConst* RexUtils::RexIRConst_U1 ( Bool bit )
{
	IRConst* c = (IRConst*) Alloc(sizeof(IRConst));
	c->tag     = Ico_U1;
	c->Ico.U1  = bit;
	/* call me paranoid; I don't care :-) */
	assert(bit == False || bit == True);
	return c;
}
IRConst* RexUtils::RexIRConst_U8 ( UChar u8 )
{
	IRConst* c = (IRConst*) Alloc(sizeof(IRConst));
	c->tag     = Ico_U8;
	c->Ico.U8  = u8;
	return c;
}
IRConst* RexUtils::RexIRConst_U16 ( UShort u16 )
{
	IRConst* c = (IRConst*) Alloc(sizeof(IRConst));
	c->tag     = Ico_U16;
	c->Ico.U16 = u16;
	return c;
}
IRConst* RexUtils::RexIRConst_U32 ( UInt u32 )
{
	IRConst* c = (IRConst*) Alloc(sizeof(IRConst));
	c->tag     = Ico_U32;
	c->Ico.U32 = u32;
	return c;
}
IRConst* RexUtils::RexIRConst_U64 ( ULong u64 )
{
	IRConst* c = (IRConst*)  Alloc(sizeof(IRConst));
	c->tag     = Ico_U64;
	c->Ico.U64 = u64;
	return c;
}
IRConst* RexUtils::RexIRConst_F64 ( Double f64 )
{
	IRConst* c = (IRConst*) Alloc(sizeof(IRConst));
	c->tag     = Ico_F64;
	c->Ico.F64 = f64;
	return c;
}
IRConst* RexUtils::RexIRConst_F64i ( ULong f64i )
{
	IRConst* c  = (IRConst*) Alloc(sizeof(IRConst));
	c->tag      = Ico_F64i;
	c->Ico.F64i = f64i;
	return c;
}
IRConst* RexUtils::RexIRConst_V128 ( UShort con )
{
	IRConst* c  = (IRConst*) Alloc(sizeof(IRConst));
	c->tag      = Ico_V128;
	c->Ico.V128 = con;
	return c;
}

/* Constructors -- IRCallee */

IRCallee* RexUtils::RexMkIRCallee ( Int regparms, HChar* name, void* addr )
{
	IRCallee* ce = (IRCallee*) Alloc(sizeof(IRCallee));
	ce->regparms = regparms;
	ce->name     = name;
	ce->addr     = addr;
	ce->mcx_mask = 0;
	assert(regparms >= 0 && regparms <= 3);
	assert(name != NULL);
	assert(addr != 0);
	return ce;
}

/* Constructors -- IRRegArray */

IRRegArray* RexUtils::RexMkIRRegArray ( Int base, IRType elemTy, Int nElems )
{
	IRRegArray* arr = (IRRegArray*) Alloc(sizeof(IRRegArray));
	arr->base       = base;
	arr->elemTy     = elemTy;
	arr->nElems     = nElems;
	assert(!(arr->base < 0 || arr->base > 10000 /* somewhat arbitrary */));
	assert(!(arr->elemTy == Ity_I1));
	assert(!(arr->nElems <= 0 || arr->nElems > 500 /* somewhat arbitrary */));
	return arr;
}

/* Constructors -- IRExpr */

IRExpr* RexUtils::RexIRExpr_Binder ( Int binder ) {
	IRExpr* e            = (IRExpr*) Alloc(sizeof(IRExpr));
	e->tag               = Iex_Binder;
	e->Iex.Binder.binder = binder;
	return e;
}
IRExpr* RexUtils::RexIRExpr_Get ( Int off, IRType ty ) {
	IRExpr* e         = (IRExpr*) Alloc(sizeof(IRExpr));
	e->tag            = Iex_Get;
	e->Iex.Get.offset = off;
	e->Iex.Get.ty     = ty;
	return e;
}
IRExpr* RexUtils::RexIRExpr_GetI ( IRRegArray* descr, IRExpr* ix, Int bias ) {
	IRExpr* e         = (IRExpr*) Alloc(sizeof(IRExpr));
	e->tag            = Iex_GetI;
	e->Iex.GetI.descr = descr;
	e->Iex.GetI.ix    = ix;
	e->Iex.GetI.bias  = bias;
	return e;
}
IRExpr* RexUtils::RexIRExpr_RdTmp ( IRTemp tmp ) {
	IRExpr* e        = (IRExpr*) Alloc(sizeof(IRExpr));
	e->tag           = Iex_RdTmp;
	e->Iex.RdTmp.tmp = tmp;
	return e;
}
IRExpr* RexUtils::RexIRExpr_Qop ( IROp op, IRExpr* arg1, IRExpr* arg2,
                                  IRExpr* arg3, IRExpr* arg4 ) {
	IRExpr* e       = (IRExpr*) Alloc(sizeof(IRExpr));
	e->tag          = Iex_Qop;
	e->Iex.Qop.op   = op;
	e->Iex.Qop.arg1 = arg1;
	e->Iex.Qop.arg2 = arg2;
	e->Iex.Qop.arg3 = arg3;
	e->Iex.Qop.arg4 = arg4;
	return e;
}
IRExpr* RexUtils::RexIRExpr_Triop  ( IROp op, IRExpr* arg1,
                                     IRExpr* arg2, IRExpr* arg3 ) {
	IRExpr* e         = (IRExpr*) Alloc(sizeof(IRExpr));
	e->tag            = Iex_Triop;
	e->Iex.Triop.op   = op;
	e->Iex.Triop.arg1 = arg1;
	e->Iex.Triop.arg2 = arg2;
	e->Iex.Triop.arg3 = arg3;
	return e;
}
IRExpr* RexUtils::RexIRExpr_Binop ( IROp op, IRExpr* arg1, IRExpr* arg2 ) {
	IRExpr* e         = (IRExpr*) Alloc(sizeof(IRExpr));
	e->tag            = Iex_Binop;
	e->Iex.Binop.op   = op;
	e->Iex.Binop.arg1 = arg1;
	e->Iex.Binop.arg2 = arg2;
	return e;
}
IRExpr* RexUtils::RexIRExpr_Unop ( IROp op, IRExpr* arg ) {
	IRExpr* e       = (IRExpr*) Alloc(sizeof(IRExpr));
	e->tag          = Iex_Unop;
	e->Iex.Unop.op  = op;
	e->Iex.Unop.arg = arg;
	return e;
}
IRExpr* RexUtils::RexIRExpr_Load ( IREndness end, IRType ty, IRExpr* addr ) {
	IRExpr* e        = (IRExpr*) Alloc(sizeof(IRExpr));
	e->tag           = Iex_Load;
	e->Iex.Load.end  = end;
	e->Iex.Load.ty   = ty;
	e->Iex.Load.addr = addr;
	assert(end == Iend_LE || end == Iend_BE);
	return e;
}
IRExpr* RexUtils::RexIRExpr_Const ( IRConst* con ) {
	IRExpr* e        = (IRExpr*) Alloc(sizeof(IRExpr));
	e->tag           = Iex_Const;
	e->Iex.Const.con = con;
	return e;
}
IRExpr* RexUtils::RexIRExpr_CCall ( IRCallee* cee, IRType retty, IRExpr** args ) {
	IRExpr* e          = (IRExpr*) Alloc(sizeof(IRExpr));
	e->tag             = Iex_CCall;
	e->Iex.CCall.cee   = cee;
	e->Iex.CCall.retty = retty;
	e->Iex.CCall.args  = args;
	return e;
}
IRExpr* RexUtils::RexIRExpr_Mux0X ( IRExpr* cond, IRExpr* expr0, IRExpr* exprX ) {
	IRExpr* e          = (IRExpr*) Alloc(sizeof(IRExpr));
	e->tag             = Iex_Mux0X;
	e->Iex.Mux0X.cond  = cond;
	e->Iex.Mux0X.expr0 = expr0;
	e->Iex.Mux0X.exprX = exprX;
	return e;
}

/* Constructors for NULL-terminated IRExpr expression vectors,
   suitable for use as arg lists in clean/dirty helper calls. */

IRExpr** RexUtils::RexMkIRExprVec_0 ( void ) {
	IRExpr** vec = (IRExpr**) Alloc(1 * sizeof(IRExpr*));
	vec[0] = NULL;
	return vec;
}
IRExpr** RexUtils::RexMkIRExprVec_1 ( IRExpr* arg1 ) {
	IRExpr** vec = (IRExpr**) Alloc(2 * sizeof(IRExpr*));
	vec[0] = arg1;
	vec[1] = NULL;
	return vec;
}
IRExpr** RexUtils::RexMkIRExprVec_2 ( IRExpr* arg1, IRExpr* arg2 ) {
	IRExpr** vec = (IRExpr**) Alloc(3 * sizeof(IRExpr*));
	vec[0] = arg1;
	vec[1] = arg2;
	vec[2] = NULL;
	return vec;
}
IRExpr** RexUtils::RexMkIRExprVec_3 ( IRExpr* arg1, IRExpr* arg2, IRExpr* arg3 ) {
	IRExpr** vec = (IRExpr**) Alloc(4 * sizeof(IRExpr*));
	vec[0] = arg1;
	vec[1] = arg2;
	vec[2] = arg3;
	vec[3] = NULL;
	return vec;
}
IRExpr** RexUtils::RexMkIRExprVec_4 ( IRExpr* arg1, IRExpr* arg2, IRExpr* arg3,
                                      IRExpr* arg4 ) {
	IRExpr** vec = (IRExpr**) Alloc(5 * sizeof(IRExpr*));
	vec[0] = arg1;
	vec[1] = arg2;
	vec[2] = arg3;
	vec[3] = arg4;
	vec[4] = NULL;
	return vec;
}
IRExpr** RexUtils::RexMkIRExprVec_5 ( IRExpr* arg1, IRExpr* arg2, IRExpr* arg3,
                                      IRExpr* arg4, IRExpr* arg5 ) {
	IRExpr** vec = (IRExpr**) Alloc(6 * sizeof(IRExpr*));
	vec[0] = arg1;
	vec[1] = arg2;
	vec[2] = arg3;
	vec[3] = arg4;
	vec[4] = arg5;
	vec[5] = NULL;
	return vec;
}
IRExpr** RexUtils::RexMkIRExprVec_6 ( IRExpr* arg1, IRExpr* arg2, IRExpr* arg3,
                                      IRExpr* arg4, IRExpr* arg5, IRExpr* arg6 ) {
	IRExpr** vec = (IRExpr**) Alloc(7 * sizeof(IRExpr*));
	vec[0] = arg1;
	vec[1] = arg2;
	vec[2] = arg3;
	vec[3] = arg4;
	vec[4] = arg5;
	vec[5] = arg6;
	vec[6] = NULL;
	return vec;
}
IRExpr** RexUtils::RexMkIRExprVec_7 ( IRExpr* arg1, IRExpr* arg2, IRExpr* arg3,
                                      IRExpr* arg4, IRExpr* arg5, IRExpr* arg6,
                                      IRExpr* arg7 ) {
	IRExpr** vec = (IRExpr**) Alloc(8 * sizeof(IRExpr*));
	vec[0] = arg1;
	vec[1] = arg2;
	vec[2] = arg3;
	vec[3] = arg4;
	vec[4] = arg5;
	vec[5] = arg6;
	vec[6] = arg7;
	vec[7] = NULL;
	return vec;
}
IRExpr** RexUtils::RexMkIRExprVec_8 ( IRExpr* arg1, IRExpr* arg2, IRExpr* arg3,
                                      IRExpr* arg4, IRExpr* arg5, IRExpr* arg6,
                                      IRExpr* arg7, IRExpr* arg8 ) {
	IRExpr** vec = (IRExpr**) Alloc(9 * sizeof(IRExpr*));
	vec[0] = arg1;
	vec[1] = arg2;
	vec[2] = arg3;
	vec[3] = arg4;
	vec[4] = arg5;
	vec[5] = arg6;
	vec[6] = arg7;
	vec[7] = arg8;
	vec[8] = NULL;
	return vec;
}

/* Constructors -- IRDirty */

IRDirty* RexUtils::RexEmptyIRDirty ( void ) {
	IRDirty* d = (IRDirty*) Alloc(sizeof(IRDirty));
	d->cee      = NULL;
	d->guard    = NULL;
	d->args     = NULL;
	d->tmp      = IRTemp_INVALID;
	d->mFx      = Ifx_None;
	d->mAddr    = NULL;
	d->mSize    = 0;
	d->needsBBP = False;
	d->nFxState = 0;
	return d;
}

/* Constructors -- IRCAS */

IRCAS* RexUtils::RexMkIRCAS ( IRTemp oldHi, IRTemp oldLo,
                              IREndness end, IRExpr* addr,
                              IRExpr* expdHi, IRExpr* expdLo,
                              IRExpr* dataHi, IRExpr* dataLo ) {
	IRCAS* cas = (IRCAS*) Alloc(sizeof(IRCAS));
	cas->oldHi  = oldHi;
	cas->oldLo  = oldLo;
	cas->end    = end;
	cas->addr   = addr;
	cas->expdHi = expdHi;
	cas->expdLo = expdLo;
	cas->dataHi = dataHi;
	cas->dataLo = dataLo;
	return cas;
}

/* Constructors -- IRStmt */

IRStmt* RexUtils::RexIRStmt_NoOp ( void )
{
	/* Just use a single static closure. */
	static IRStmt static_closure;
	static_closure.tag = Ist_NoOp;
	return &static_closure;
}
IRStmt* RexUtils::RexUtils::RexIRStmt_IMark ( Addr64 addr, Int len ) {
	IRStmt* s         = (IRStmt*) (IRStmt*) Alloc(sizeof(IRStmt));
	s->tag            = Ist_IMark;
	s->Ist.IMark.addr = addr;
	s->Ist.IMark.len  = len;
	return s;
}
IRStmt* RexUtils::RexIRStmt_AbiHint ( IRExpr* base, Int len, IRExpr* nia ) {
	IRStmt* s           = (IRStmt*) Alloc(sizeof(IRStmt));
	s->tag              = Ist_AbiHint;
	s->Ist.AbiHint.base = base;
	s->Ist.AbiHint.len  = len;
	s->Ist.AbiHint.nia  = nia;
	return s;
}
IRStmt* RexUtils::RexIRStmt_Put ( Int off, IRExpr* data ) {
	IRStmt* s         = (IRStmt*) Alloc(sizeof(IRStmt));
	s->tag            = Ist_Put;
	s->Ist.Put.offset = off;
	s->Ist.Put.data   = data;
	return s;
}
IRStmt* RexUtils::RexIRStmt_PutI ( IRRegArray* descr, IRExpr* ix,
                                   Int bias, IRExpr* data ) {
	IRStmt* s         = (IRStmt*) Alloc(sizeof(IRStmt));
	s->tag            = Ist_PutI;
	s->Ist.PutI.descr = descr;
	s->Ist.PutI.ix    = ix;
	s->Ist.PutI.bias  = bias;
	s->Ist.PutI.data  = data;
	return s;
}
IRStmt* RexUtils::RexIRStmt_WrTmp ( IRTemp tmp, IRExpr* data ) {
	IRStmt* s         = (IRStmt*) Alloc(sizeof(IRStmt));
	s->tag            = Ist_WrTmp;
	s->Ist.WrTmp.tmp  = tmp;
	s->Ist.WrTmp.data = data;
	return s;
}
IRStmt* RexUtils::RexIRStmt_Store ( IREndness end, IRExpr* addr, IRExpr* data ) {
	IRStmt* s         = (IRStmt*) Alloc(sizeof(IRStmt));
	s->tag            = Ist_Store;
	s->Ist.Store.end  = end;
	s->Ist.Store.addr = addr;
	s->Ist.Store.data = data;
	assert(end == Iend_LE || end == Iend_BE);
	return s;
}
IRStmt* RexUtils::RexIRStmt_CAS ( IRCAS* cas ) {
	IRStmt* s          = (IRStmt*) Alloc(sizeof(IRStmt));
	s->tag             = Ist_CAS;
	s->Ist.CAS.details = cas;
	return s;
}
IRStmt* RexUtils::RexIRStmt_LLSC ( IREndness end,
                                   IRTemp result, IRExpr* addr, IRExpr* storedata ) {
	IRStmt* s = (IRStmt*) Alloc(sizeof(IRStmt));
	s->tag                = Ist_LLSC;
	s->Ist.LLSC.end       = end;
	s->Ist.LLSC.result    = result;
	s->Ist.LLSC.addr      = addr;
	s->Ist.LLSC.storedata = storedata;
	return s;
}
IRStmt* RexUtils::RexIRStmt_Dirty ( IRDirty* d )
{
	IRStmt* s            = (IRStmt*) Alloc(sizeof(IRStmt));
	s->tag               = Ist_Dirty;
	s->Ist.Dirty.details = d;
	return s;
}
IRStmt* RexUtils::RexIRStmt_MBE ( IRMBusEvent event )
//IRStmt* RexUtils::RexIRStmt_MBE ( void )
{
	IRStmt* s        = (IRStmt*) Alloc(sizeof(IRStmt));
	s->tag           = Ist_MBE;
	s->Ist.MBE.event = event;
	return s;
	/*
	static IRStmt static_closure;
	static_closure.tag = Ist_MBE;
	return &static_closure;
	*/
}
IRStmt* RexUtils::RexIRStmt_Exit ( IRExpr* guard, IRJumpKind jk, IRConst* dst ) {
	IRStmt* s         = (IRStmt*) Alloc(sizeof(IRStmt));
	s->tag            = Ist_Exit;
	s->Ist.Exit.guard = guard;
	s->Ist.Exit.jk    = jk;
	s->Ist.Exit.dst   = dst;
	return s;
}



/* Constructors -- IRTypeEnv */

IRTypeEnv* RexUtils::RexEmptyIRTypeEnv ( void )
{
	IRTypeEnv* env   = (IRTypeEnv*) Alloc(sizeof(IRTypeEnv));
	env->types       = (IRType*) Alloc(8 * sizeof(IRType));
	env->types_size  = 8;
	env->types_used  = 0;
	return env;
}


/* Constructors -- IRSB */

IRSB* RexUtils::RexEmptyIRSB ( void )
{
	IRSB* bb       = (IRSB*) Alloc(sizeof(IRSB));
	// bb->tyenv      = RexEmptyIRTypeEnv();
	bb->tyenv      = NULL;
	bb->stmts_used = 0;
	//bb->stmts_size = 8;
	bb->stmts_size = 0;
	// bb->stmts      = (IRStmt**) Alloc(bb->stmts_size * sizeof(IRStmt*));
	bb->stmts      = NULL;
	bb->next       = NULL;
	bb->jumpkind   = Ijk_Boring;
	return bb;
}




/*---------------------------------------------------------------*/
/*--- (Deep) copy constructors.  These make complete copies   ---*/
/*--- the original, which can be modified without affecting   ---*/
/*--- the original.                                           ---*/
/*---------------------------------------------------------------*/

/* Copying IR Expr vectors (for call args). */

/* Shallow copy of an IRExpr vector */

IRExpr** RexUtils::RexShallowCopyIRExprVec ( IRExpr** vec )
{
	Int      i;
	IRExpr** newvec;
	for (i = 0; vec[i]; i++)
		;
	newvec = (IRExpr**) Alloc((i+1)*sizeof(IRExpr*));
	for (i = 0; vec[i]; i++)
		newvec[i] = vec[i];
	newvec[i] = NULL;
	return newvec;
}

/* Deep copy of an IRExpr vector */

IRExpr** RexUtils::RexDeepCopyIRExprVec ( IRExpr** vec )
{
	Int      i;
	IRExpr** newvec = RexShallowCopyIRExprVec( vec );
	for (i = 0; newvec[i]; i++)
		newvec[i] = RexDeepCopyIRExpr(newvec[i]);
	return newvec;
}

/* Deep copy constructors for all heap-allocated IR types follow. */

IRConst* RexUtils::RexDeepCopyIRConst ( IRConst* c )
{
	switch (c->tag) {
	case Ico_U1:
		return RexIRConst_U1(c->Ico.U1);
	case Ico_U8:
		return RexIRConst_U8(c->Ico.U8);
	case Ico_U16:
		return RexIRConst_U16(c->Ico.U16);
	case Ico_U32:
		return RexIRConst_U32(c->Ico.U32);
	case Ico_U64:
		return RexIRConst_U64(c->Ico.U64);
	case Ico_F64:
		return RexIRConst_F64(c->Ico.F64);
	case Ico_F64i:
		return RexIRConst_F64i(c->Ico.F64i);
	case Ico_V128:
		return RexIRConst_V128(c->Ico.V128);
	default:
		throw "RexDeepCopyIRConst";
	}

	/*
	IRConst *c2;
	if( (c2 = (IRConst*) Alloc(sizeof(IRConst))) )
		std::memcpy( c2, c, sizeof(IRConst) );
	return c2;
	*/
}

IRCallee* RexUtils::RexDeepCopyIRCallee ( IRCallee* ce )
{
	IRCallee* ce2 = RexMkIRCallee(ce->regparms, ce->name, ce->addr);
	ce2->mcx_mask = ce->mcx_mask;
	return ce2;
}

IRRegArray* RexUtils::RexDeepCopyIRRegArray ( IRRegArray* d )
{
	return RexMkIRRegArray(d->base, d->elemTy, d->nElems);
}

IRExpr* RexUtils::RexDeepCopyIRExpr ( IRExpr* e )
{
	switch (e->tag) {
	case Iex_Get:
		return RexIRExpr_Get(e->Iex.Get.offset, e->Iex.Get.ty);
	case Iex_GetI:
		return RexIRExpr_GetI(RexDeepCopyIRRegArray(e->Iex.GetI.descr),
		                      RexDeepCopyIRExpr(e->Iex.GetI.ix),
		                      e->Iex.GetI.bias);
	case Iex_RdTmp:
		return RexIRExpr_RdTmp(e->Iex.RdTmp.tmp);
	case Iex_Qop:
		return RexIRExpr_Qop(e->Iex.Qop.op,
		                     RexDeepCopyIRExpr(e->Iex.Qop.arg1),
		                     RexDeepCopyIRExpr(e->Iex.Qop.arg2),
		                     RexDeepCopyIRExpr(e->Iex.Qop.arg3),
		                     RexDeepCopyIRExpr(e->Iex.Qop.arg4));
	case Iex_Triop:
		return RexIRExpr_Triop(e->Iex.Triop.op,
		                       RexDeepCopyIRExpr(e->Iex.Triop.arg1),
		                       RexDeepCopyIRExpr(e->Iex.Triop.arg2),
		                       RexDeepCopyIRExpr(e->Iex.Triop.arg3));
	case Iex_Binop:
		return RexIRExpr_Binop(e->Iex.Binop.op,
		                       RexDeepCopyIRExpr(e->Iex.Binop.arg1),
		                       RexDeepCopyIRExpr(e->Iex.Binop.arg2));
	case Iex_Unop:
		return RexIRExpr_Unop(e->Iex.Unop.op,
		                      RexDeepCopyIRExpr(e->Iex.Unop.arg));
	case Iex_Load:
		return RexIRExpr_Load(e->Iex.Load.end,
		                      e->Iex.Load.ty,
		                      RexDeepCopyIRExpr(e->Iex.Load.addr));
	case Iex_Const:
		return RexIRExpr_Const(RexDeepCopyIRConst(e->Iex.Const.con));
	case Iex_CCall:
		return RexIRExpr_CCall(RexDeepCopyIRCallee(e->Iex.CCall.cee),
		                       e->Iex.CCall.retty,
		                       RexDeepCopyIRExprVec(e->Iex.CCall.args));

	case Iex_Mux0X:
		return RexIRExpr_Mux0X(RexDeepCopyIRExpr(e->Iex.Mux0X.cond),
		                       RexDeepCopyIRExpr(e->Iex.Mux0X.expr0),
		                       RexDeepCopyIRExpr(e->Iex.Mux0X.exprX));
	default:
		throw "RexDeepCopyIRExpr";
	}
}

IRDirty* RexUtils::RexDeepCopyIRDirty ( IRDirty* d )
{
	Int      i;
	IRDirty* d2 = RexEmptyIRDirty();
	d2->cee   = RexDeepCopyIRCallee(d->cee);
	d2->guard = RexDeepCopyIRExpr(d->guard);
	d2->args  = RexDeepCopyIRExprVec(d->args);
	d2->tmp   = d->tmp;
	d2->mFx   = d->mFx;
	d2->mAddr = d->mAddr==NULL ? NULL : RexDeepCopyIRExpr(d->mAddr);
	d2->mSize = d->mSize;
	d2->needsBBP = d->needsBBP;
	d2->nFxState = d->nFxState;
	for (i = 0; i < d2->nFxState; i++)
		d2->fxState[i] = d->fxState[i];
	return d2;
}

IRCAS* RexUtils::RexDeepCopyIRCAS ( IRCAS* cas )
{
	return RexMkIRCAS( cas->oldHi, cas->oldLo, cas->end,
	                   RexDeepCopyIRExpr(cas->addr),
	                   cas->expdHi==NULL ? NULL : RexDeepCopyIRExpr(cas->expdHi),
	                   RexDeepCopyIRExpr(cas->expdLo),
	                   cas->dataHi==NULL ? NULL : RexDeepCopyIRExpr(cas->dataHi),
	                   RexDeepCopyIRExpr(cas->dataLo) );
}

IRStmt* RexUtils::RexDeepCopyIRStmt ( IRStmt* s )
{
	switch (s->tag) {
	case Ist_NoOp:
		return RexIRStmt_NoOp();
	case Ist_AbiHint:
		return RexIRStmt_AbiHint(RexDeepCopyIRExpr(s->Ist.AbiHint.base),
		                         s->Ist.AbiHint.len,
		                         RexDeepCopyIRExpr(s->Ist.AbiHint.nia));
	case Ist_IMark:
		return RexIRStmt_IMark(s->Ist.IMark.addr, s->Ist.IMark.len);
	case Ist_Put:
		return RexIRStmt_Put(s->Ist.Put.offset,
		                     RexDeepCopyIRExpr(s->Ist.Put.data));
	case Ist_PutI:
		return RexIRStmt_PutI(RexDeepCopyIRRegArray(s->Ist.PutI.descr),
		                      RexDeepCopyIRExpr(s->Ist.PutI.ix),
		                      s->Ist.PutI.bias,
		                      RexDeepCopyIRExpr(s->Ist.PutI.data));
	case Ist_WrTmp:
		return RexIRStmt_WrTmp(s->Ist.WrTmp.tmp,
		                       RexDeepCopyIRExpr(s->Ist.WrTmp.data));
	case Ist_Store:
		return RexIRStmt_Store(s->Ist.Store.end,
		                       RexDeepCopyIRExpr(s->Ist.Store.addr),
		                       RexDeepCopyIRExpr(s->Ist.Store.data));
	case Ist_CAS:
		return RexIRStmt_CAS(RexDeepCopyIRCAS(s->Ist.CAS.details));
	case Ist_LLSC:
		return RexIRStmt_LLSC(s->Ist.LLSC.end,
		                      s->Ist.LLSC.result,
		                      RexDeepCopyIRExpr(s->Ist.LLSC.addr),
		                      s->Ist.LLSC.storedata
		                      ? RexDeepCopyIRExpr(s->Ist.LLSC.storedata)
		                      : NULL);
	case Ist_Dirty:
		return RexIRStmt_Dirty(RexDeepCopyIRDirty(s->Ist.Dirty.details));
	case Ist_MBE:
		return RexIRStmt_MBE(s->Ist.MBE.event);
		//return RexIRStmt_MBE();
	case Ist_Exit:
		return RexIRStmt_Exit(RexDeepCopyIRExpr(s->Ist.Exit.guard),
		                      s->Ist.Exit.jk,
		                      RexDeepCopyIRConst(s->Ist.Exit.dst));
	default:
		throw "RexDeepCopyIRStmt";
	}
}

IRTypeEnv* RexUtils::RexDeepCopyIRTypeEnv ( IRTypeEnv* src )
{
	Int        i;
	IRTypeEnv* dst = (IRTypeEnv*) Alloc(sizeof(IRTypeEnv));
	dst->types_size = src->types_size;
	dst->types_used = src->types_used;
	dst->types = (IRType*) Alloc(dst->types_size * sizeof(IRType));
	for (i = 0; i < src->types_used; i++)
		dst->types[i] = src->types[i];
	return dst;
}

IRSB* RexUtils::RexDeepCopyIRSB ( IRSB* bb )
{
	Int      i;
	IRStmt** sts2;
	IRSB* bb2 = RexDeepCopyIRSBExceptStmts(bb);
	bb2->stmts_used = bb2->stmts_size = bb->stmts_used;
	sts2 = (IRStmt**) Alloc(bb2->stmts_used * sizeof(IRStmt*));
	for (i = 0; i < bb2->stmts_used; i++)
		sts2[i] = RexDeepCopyIRStmt(bb->stmts[i]);
	bb2->stmts    = sts2;
	return bb2;
}

IRSB* RexUtils::RexDeepCopyIRSBExceptStmts ( IRSB* bb )
{
	/*
	IRSB* bb2     = RexEmptyIRSB();
	bb2->tyenv    = RexDeepCopyIRTypeEnv(bb->tyenv);
	bb2->next     = RexDeepCopyIRExpr(bb->next);
	bb2->jumpkind = bb->jumpkind;
	return bb2;
	*/

   Int      i;
   IRStmt** sts2;
   IRSB* bb2 = RexEmptyIRSB();
   bb2->tyenv = RexDeepCopyIRTypeEnv(bb->tyenv);
   bb2->stmts_used = bb2->stmts_size = bb->stmts_used;
   sts2 = (IRStmt **)Alloc(bb2->stmts_used * sizeof(IRStmt*));
   for (i = 0; i < bb2->stmts_used; i++)
      sts2[i] = RexDeepCopyIRStmt(bb->stmts[i]);
   bb2->stmts    = sts2;
   bb2->next     = RexDeepCopyIRExpr(bb->next);
   bb2->jumpkind = bb->jumpkind;
   return bb2;

}


