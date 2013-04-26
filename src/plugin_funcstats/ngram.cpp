#include <log4cpp/Category.hh>
#include <ida.hpp>
#include <idp.hpp>
#include <funcs.hpp>
#include <bytes.hpp>
#include <ua.hpp>
#include <auto.hpp>
#include <allins.hpp>
//#include <mysql/mysql.h>
#include <mysql.h>
#include <function.h>

using namespace std;

#define PRNTSQLERR( log, conn )\
	log.fatal( "%s Line %d: %s", __FILE__, __LINE__, mysql_error(conn) )

static size_t GetOpcodeSize(ea_t opcodeAddr,
	char *opcodePrefix,
	size_t opcodePrefixLen)
{
	unsigned char byteVal;
	size_t len;

	// First byte.
	// If the first byte is not 0x0f,
	// then the opcode length is 1.
	byteVal = get_byte( opcodeAddr );
	if( opcodePrefix != NULL )
	{
		len = strlen(opcodePrefix);
		qsnprintf( &opcodePrefix[len], opcodePrefixLen - len, "%02x", byteVal );
	}
	if( byteVal != 0x0f ) return 1;

	
	// Second byte.
	// If the second byte is not 0x38 and 0x3a,
	// then the opcode length is 2.
	byteVal = get_byte( opcodeAddr + 1 );
	if( opcodePrefix != NULL )
	{
		len = strlen(opcodePrefix);
		qsnprintf( &opcodePrefix[len], opcodePrefixLen - len, "%02x", byteVal );
	}
	if( byteVal != 0x38 && byteVal != 0x3a ) return 2;

	// Third byte.
	byteVal = get_byte( opcodeAddr + 2 );
	if( opcodePrefix != NULL )
	{
		len = strlen(opcodePrefix);	
		qsnprintf( &opcodePrefix[len], opcodePrefixLen - len, "%02x", byteVal );
	}

	return 3;
}

/*!
 * Update ngram frequency table.
 * */
static void UpdFreqTable(
	MYSQL * conn,
	const unsigned int func_id,	
	const unsigned int n,
	int *const coincTable, const int coincTableSize,
	int *const ptrTable, const int ptrTableSize,
	unsigned short *const instrTable, const int instrTableSize,
	int *const freqTable, const int freqTableSize,
	log4cpp::Category &log )
{
	int freq;

	const char* preBuf = "INSERT INTO `frequency` (func_id, ngram_id, freq) VALUES ";
	size_t preBufLen = strlen(preBuf);

	const char* postBuf =  "ON DUPLICATE KEY UPDATE freq=VALUES(freq)";
	size_t postBufLen = strlen(postBuf);

	const size_t MAXBUFLEN = 1048576;
	char *buf = NULL;
	if( (buf = new char[MAXBUFLEN]) == NULL )
	{
		log.fatal("%s Line %d: Can't allocate buffer for ngram!", __FILE__, __LINE__);
		return;
	}
	qsnprintf( buf, MAXBUFLEN, "%s", preBuf );
	size_t bufLen = strlen(buf);

	for(int i=0; i < ptrTableSize; ++i)
	{
		//
		// New ngram
		//

		size_t len;
		char ngram[513] = { 0x0 };
		freq = 1;
	
		// Print n opcodes
		int instrTableIdx = ptrTable[i];
		qsnprintf(ngram, sizeof(ngram), "0x");
		int offset = 2;
		for(int j=0; j < n && ( instrTableIdx+j < ptrTableSize); ++j)
		{
			unsigned short opcodeLen;

			unsigned short instr = instrTable[instrTableIdx+j];
			unsigned char *instrPtr = (unsigned char *)&instr;

			qsnprintf( &ngram[offset], sizeof(ngram) - offset, "%02x", instrPtr[1] ); offset+=2;
			qsnprintf( &ngram[offset], sizeof(ngram) - offset, "%02x", instrPtr[0] ); offset+=2;
		}

		// Count number of occurrences where the coincidence number
		// of prefix characters is lesser than n.
		while( (i < ptrTableSize-1) && (coincTable[i] >= n) )
		{
			++freq;
			++i;
		}

		//
		// Insert ngram if it doesn't already exist.
		//
		char ngrambuf[512];
		qsnprintf(ngrambuf, sizeof(ngrambuf),
			"INSERT IGNORE INTO `ngram` (`value`, `n`) VALUES (%s, %u) "\
			"ON DUPLICATE KEY UPDATE ngram_id=LAST_INSERT_ID(ngram_id)",
			ngram, n );
		if( mysql_query(conn, ngrambuf) != 0 )
		{
			log.fatal( "%s", ngrambuf );
			log.fatal("%s Line %d: %s", __FILE__, __LINE__, mysql_error(conn) );
			break;
		}

		unsigned int ngram_id = mysql_insert_id(conn);

		//--------------------------------------------------------
		// Tries to pack as many INSERTs as we can into one query
		//--------------------------------------------------------

		char oneVal[32];
		size_t oneValLen = 0;

		if( bufLen ==  preBufLen )
		{
			// Have not inserted any value
			qsnprintf(oneVal, sizeof(oneVal), "(%u, %u, %u)",
				func_id, ngram_id, freq );
		}
		else
		{	// Not the first, so we need the comma
			qsnprintf(oneVal, sizeof(oneVal), ", (%u, %u, %u)",
				func_id, ngram_id, freq );
		}

		oneValLen = strlen(oneVal);

		// If we can pack in the new value, do it.
		// Otherwise, oneVal is full, so send it.
		if( (bufLen + oneValLen + postBufLen) < MAXBUFLEN )
		{
			qsnprintf( &buf[bufLen], MAXBUFLEN - bufLen, "%s", oneVal );

			// Update new bufLen
			bufLen += oneValLen;
		}
		else
		{
			// Reached max capacity of values.
			// Append the postBuf and send it.
			qsnprintf( &buf[bufLen], MAXBUFLEN - bufLen, "%s", postBuf );
			if( mysql_query(conn, buf) != 0 )
			{
				log.fatal( "%s", buf );
				log.fatal("%s Line %d: %s", __FILE__, __LINE__, mysql_error(conn));

				delete [] buf;
				return;
			}

			// Reset buf with preBuf and the new value
			qsnprintf( buf, MAXBUFLEN, "%s(%u, %u, %u)",
				preBuf, func_id, ngram_id, freq );
			bufLen = strlen(buf);
		}
	}

	// Checks if there's values in buf. If so, send it.
	// This will almost always be true, except for the
	// worst case where there are no functions at all.
	if( bufLen > preBufLen)
	{
		qsnprintf( &buf[bufLen], MAXBUFLEN - bufLen, "%s", postBuf );
		if( mysql_query( conn, buf ) != 0 )
		{
			log.fatal("%s Line %d: %s", __FILE__, __LINE__, mysql_error(conn));

			delete [] buf;
			return;
		}
	}

	delete [] buf;
}

/*!
 * Updates the coincidence table.
 * */
static void UpdCoincTable(
	int *const coincTable, const int coincTableSize,
	int *const ptrTable, const int ptrTableSize,
	unsigned short *const instrTable, const int instrTableSize )
{
	int instrTableIdx1, instrTableIdx2;
	unsigned short instr1, instr2;
	int coincCount;

	// For the (ptrTableSize-1) entries
	for(int i = 0; i < ptrTableSize-1; ++i)
	{
		// Get index of current and next instruction in instrTable
		instrTableIdx1 = ptrTable[i];
		instrTableIdx2 = ptrTable[i+1];

		// while we're not at the end of the two instruction strings...
		coincCount = 0;
		while( instrTableIdx1 < ptrTableSize && instrTableIdx2 < ptrTableSize )	
		{
			// Get current and next instruction
			instr1 = instrTable[instrTableIdx1];
			instr2 = instrTable[instrTableIdx2];

			// End loop when the two instructions are different or we're
			// at the last entry in instrTable
			if( instr1 != instr2 )
			{
				coincTable[i] = coincCount;				
				break;
			}
			else if( instrTableIdx1 == (ptrTableSize-1) || instrTableIdx2 == (ptrTableSize-1) )
			{
				coincTable[i] = coincCount+1;
				break;
			}

			// Advance to next instruction in strings
			++instrTableIdx1;
			++instrTableIdx2;
			++coincCount;
		}
	}
}

/*!
 *  Returns true if the opcode string at addr1 is
 *  larger than that at addr2.
 * */
/*
static bool IsGreaterThan(
	int *const ptrTable,
	unsigned int *const instrTable,
	const int tableSize,
	int A_idx,
	int B_idx )
{
	unsigned int A_opcode_addr, B_opcode_addr;
	int A_opcode_offset, B_opcode_offset;
	int A_mux_idx, B_mux_idx;


	A_mux_idx = ptrTable[A_idx];
	B_mux_idx = ptrTable[B_idx];

	// Repeat for each entry in muxTable until the opcodes
	// can be distinguished.
	while( A_mux_idx < tableSize && B_mux_idx < tableSize )
	{
		A_opcode_addr = muxTable[A_mux_idx++]; A_opcode_offset = 0;
		B_opcode_addr = muxTable[B_mux_idx++]; B_opcode_offset = 0;

		//
		// Move the opcode offsets to the next opcode byte
		// whenever an escape byte is encounted. For 2-bytes
		// opcodes, the escape byte is 0F. For 3-bytes
		// opcodes, the escape bytes are 0F38 and 0F3A.
		//
		A_opcode_offset = GetOpcodeSize(A_opcode_addr, NULL, 0) - 1;
		B_opcode_offset = GetOpcodeSize(B_opcode_addr, NULL, 0) - 1;

		if( A_opcode_offset > B_opcode_offset ) return true;
		if( A_opcode_offset < B_opcode_offset ) return false;
		else // A_opcode_offset == B_opcode_offset
		{
			unsigned char A_opcode_val, B_opcode_val;
			A_opcode_val = get_byte(A_opcode_addr+A_opcode_offset);
			B_opcode_val = get_byte(B_opcode_addr+B_opcode_offset);
			if( A_opcode_val > B_opcode_val ) return true;
			else if ( A_opcode_val < B_opcode_val ) return false;
		}

		//msg("ptrTableNumEntries %d, A_idx %d, B_idx %d\n", ptrTableNumEntries, A_idx, B_idx);
	}

	// B at last entry?
	if( A_mux_idx < tableSize && B_mux_idx == tableSize ) return true;

	return false;
}
*/

/*!
 * Performs CombSort based on the opcodes pointed to by the addresses
 * in the ptrTable.
 * */
static void CombSort(
	int *const ptrTable,
	unsigned short *const instrTable,
	const int tableSize)
{
	int gap = tableSize;

	for(;;)
	{
		// Compute gap
		gap = (gap*10) / 13;
		if( gap == 9 || gap == 10)	gap = 11;
		if( gap < 1)			gap = 1;

		bool swapped = false;
		for(int i = 0; i < tableSize - gap; i++)
		{
			int j = i + gap;
			// if( IsGreaterThan( ptrTable, instrTable, tableSize, i, j ) )
			if( instrTable[ ptrTable[i] ] >  instrTable[ ptrTable[j] ])
			{
				std::swap(ptrTable[i], ptrTable[j]);
				swapped = true;
			}
		}
		if( gap == 1 && !swapped )
			break;
	}
}

/*!
Returns the next address after function prologue.
We define function prologue to be the following
sequence of bytes.
Offset 0	55
Offset 1	89 E5
Offset 3	(End of Prologue)

If there is no prologue, the function simply returns the input address.
*/
static ea_t AddrAfterPrologue( const ea_t ea )
{
	if ( get_byte(ea) == 0x55 &&
	get_byte(ea+1) == 0x89 &&
	get_byte(ea+2) == 0xe5 )
	{
		return ea+3;
	}

	return ea;
}

/*!
If the input address is the beginning of a function
epilogue, the function returns the offset to the
first address after the epilogue. Otherwise it
returns 0.

The following are possible epilogues:

------- 1 -------
5d	push ebp	
c3	retn

------- 2 -------
c9	leave
c3	retn

*/
#ifdef __X86__
static ea_t OffsetEpilogue( ea_t ea )
{
	if( ( get_byte(ea) == 0x5d || get_byte(ea) == 0xc9 ) &&
	get_byte(ea+1) == 0xc3 )
		return 2;

	return 0;
}


static ea_t OffsetGetPCThunk( ea_t ea, ea_t *get_pc_thunk_addr, log4cpp::Category &log )
{

	ea_t totalOffset = 0;

	// Make sure cmd contains values for ea
	decode_insn( ea );

	// Get the call operand. Assume it's a 32-bit address.
	ea_t tgtAddr = cmd.Op1.addr;

	// If get_pc_thunk_addr is BADADDR,
	// check if tgtAddr is __i686_get_pc_thunk_bx
	if( *get_pc_thunk_addr == BADADDR )
	{
		unsigned char buf[4];
		get_many_bytes( tgtAddr, buf, sizeof(4) );

		// Both __i686_get_pc_thunk_bx and __i686_get_pc_thunk_cx has only 4 bytes.
		// They differ only in byte 1.
		if( buf[0] == 0x8b &&
		( buf[1] == 0x1c || buf[1] == 0x0c ) &&
		buf[2] == 0x24 &&
		buf[3] == 0xc3 )
		{
			*get_pc_thunk_addr = tgtAddr;
		}
	}


	// If the target address of the current CALL instruction
	// is __i686_get_pc_thunk_bx, check if the next
	// instruction is an ADD to EBX
	if( *get_pc_thunk_addr == tgtAddr )
	{
		totalOffset += cmd.size;

		// Disassembles the next instruction.
		// This will update cmd.
		decode_insn( ea + cmd.size );

		// If the next instruction is ADD and
		// the 1st operand is an EBX register,
		// then we'll skip
		if( cmd.itype == NN_add &&
		cmd.Op1.type == o_reg &&
		cmd.Op1.reg == 3 )
		{
			totalOffset += cmd.size;

			// Reset cmd to values for input address
			decode_insn( ea );
			return totalOffset;
		}
	}

	// Didn't find __i686_get_pc_thunk_bx followed by
	// an ADD instruction to EBX...
	decode_insn(ea);

	return 0;
}

#endif	// __X86__

/*!
Updates total number of ngrams to MySQL database.
*/
static int UpdNumNgrams( MYSQL *conn,
	const unsigned int func_id,
	const unsigned int numNgrams,
	log4cpp::Category &log )
{
	char buf[512];

	qsnprintf( buf, sizeof(buf),
		"UPDATE `function` SET `num_ngrams`=%u WHERE `func_id` = %u",
		numNgrams, func_id );
	if( mysql_query( conn, buf ) != 0 )
	{
		PRNTSQLERR( log, conn );
		return -1;
	}

	return 0;
}

typedef struct
{
	int			*tableIdx;
	size_t			instrTableMaxElem;
	ea_t			*get_pc_thunk_addr;
	log4cpp::Category	*log;
	unsigned short		*instrTable;
} FILLDATA, *PFILLDATA;

void idaapi FillInstrTable( ea_t ea1, ea_t ea2, void *ud )
{
	PFILLDATA pFillDat = (PFILLDATA) ud;

	// pFillDat->log->info("Chunk %08x - %08x tableIdx %d instrTableMaxElem %d",
	//	ea1, ea2, *pFillDat->tableIdx, *pFillDat->instrTableMaxElem);

	int bytes = 1;
	for(ea_t addr = ea1;
	addr <= ea2 && *(pFillDat->tableIdx) < pFillDat->instrTableMaxElem;
	addr+=bytes)
	{
		if( decode_insn(addr) > 0)
		{
			#ifdef __X86__
			ea_t epiOffset = OffsetEpilogue( addr );
			if( epiOffset > 0 )
			{	// If this is the function epilogue, skip.
				bytes = epiOffset;
				continue;
			}
			else
			if( cmd.itype == NN_call && cmd.Op1.type == o_near &&
			( bytes = OffsetGetPCThunk( addr, pFillDat->get_pc_thunk_addr, *(pFillDat->log) ) ) > 0 )
			{	// If this is a near, relative (to next addr) displacement call,
				// and it is a __i686_get_pc_thunk_bx function, skip.
				continue;
			}
			else
			#endif // __X86__
			{
				pFillDat->instrTable[(*pFillDat->tableIdx)++] = cmd.itype;
				bytes = cmd.size;
			}
		}
		else
		{
			bytes = 1;
		}
	}
}

void ngram(
	MYSQL *const conn,
	Function *const pFn,
	unsigned short *const instrTable,
	const size_t instrTableMaxElem,
	log4cpp::Category &log )
{
	// Gets ngram value from environment
  qstring ngram;
  qgetenv("NGRAM", &ngram);
	unsigned int n = atoi( ngram.c_str() );

	int tableIdx = 0;
	
	//
	// Populate instruction table (for x86, skip prologue,
	// get_pc_thunk stuff, and epilogue).
	//

	ea_t	startEA;
	#ifdef __X86__
	ea_t	get_pc_thunk_addr = BADADDR;
	startEA = AddrAfterPrologue( pFn->GetStartEA() );
	#else
	startEA = pFn->GetStartEA();
	#endif

	FILLDATA fillDat;
	fillDat.tableIdx = &tableIdx;
	fillDat.instrTableMaxElem = instrTableMaxElem;
	fillDat.get_pc_thunk_addr = &get_pc_thunk_addr;
	fillDat.log = &log;
	fillDat.instrTable = instrTable;
	iterate_func_chunks( pFn->GetFuncType(), FillInstrTable, &fillDat );

	// log.info("tableIdx %d", tableIdx);

	if(tableIdx == instrTableMaxElem)
	{
		msg("instrTable too small!\n");
		log.fatal("instrTable too small!");
		return;
	}


	// Update number of ngrams to database
	if( UpdNumNgrams( conn, pFn->GetFuncId(), tableIdx, log ) != 0 )
		return;


	// Reallocate muxTable
	//muxTable = (ea_t *) realloc(muxTable, tableIdx*sizeof(ea_t));
	//if( muxTable == NULL )
	//{
	//	msg("Cannot reallocate muxTable!\n");
	//	log.fatal("Cannot reallocate muxTable!");
	//	return;
	//}

	//
	// Allocate and initialize ptrTable
	//
	int *ptrTable;
	ptrTable = (int*) malloc(tableIdx * sizeof(int));
	if(ptrTable == NULL)
	{
		msg("Cannot allocate pointer table. Out of memory!\n");
		log.fatal("Cannot allocate pointer table. Out of memory!");
		return;
	}
	for(int i = 0; i < tableIdx; i++) ptrTable[i] = i;

	CombSort( ptrTable, instrTable, tableIdx );

	// Allocate coincidence table
	int *coincTable = (int*) calloc((tableIdx), sizeof(int));
	if( coincTable == NULL )
	{
		msg("Cannot allocate coincidence table. Out of memory!\n");
		log.fatal("Cannot allocate coincidence table. Out of memory!");
		free(ptrTable);
		return;
	}

	UpdCoincTable(coincTable, tableIdx, ptrTable, tableIdx, instrTable, tableIdx);

	// Allocate and init ngram frequency table
	int *freqTable = (int *) malloc( tableIdx * sizeof(int) );
	if( freqTable == NULL )
	{
		msg("Cannot allocate frequency table. Out of memory!\n");
		log.fatal("Cannot allocate frequency table. Out of memory!");
		free(ptrTable);
		free(coincTable);
		return;
	}
	for(int i = 0; i < tableIdx; i++) freqTable[i] = -1;

	UpdFreqTable( conn, pFn->GetFuncId(), n,
		coincTable, tableIdx - 1,
		ptrTable, tableIdx,
		instrTable, tableIdx,
		freqTable, tableIdx,
		log );

	if(ptrTable != NULL)	free(ptrTable);
	if(coincTable != NULL)	free(coincTable);
	if(freqTable != NULL)	free(freqTable);
}
