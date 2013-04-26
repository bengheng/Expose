#include <string.h>
#include <assert.h>
#include <rexbb.h>

RexBB::RexBB(
	const unsigned char *const _insns,
	const size_t _insnsLen,
	const unsigned int _startEA,
	const unsigned int _nextEA1,
	const unsigned int _nextEA2 )
{
	assert( _insns != NULL );
	assert( _insnsLen != 0 );

	insns = NULL;
	insnsLen = _insnsLen;

	// Allocate memory for instructions
	insns = new unsigned char[insnsLen];
	if( insns == NULL ) return;

	// Copy instructions
	memcpy( insns, _insns, insnsLen );

	startEA = _startEA;
	nextEA1 = _nextEA1;
	nextEA2 = _nextEA2;

	pass = 0;
}

RexBB::~RexBB()
{
	// Free instructions
	if( insns != NULL ) delete [] insns;
}
