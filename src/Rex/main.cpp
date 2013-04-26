#ifdef __cplusplus
extern "C" {
#endif
#include "libvex.h"
#ifdef __cplusplus
}
#endif
#include <stdlib.h>
#include <rex.h>
#include <rexfunc.h>
#include <c_interface.h>
#include <stdio.h>
#include <vector>
#include <execinfo.h>
#include <signal.h>

#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/PatternLayout.hh>

using namespace std;

/*
unsigned char insn[] = { 0x55,		// push ebp
	0x89, 0xe5,			// mov ebp, esp
	0x83, 0xec, 0x08,		// sub esp, 8
	//0xe8, 0x35, 0x00, 0x00, 0x00,	// call call_gmon_start
	//0xe8, 0xac, 0x00, 0x00, 0x00,	// call frame_dummy
	//0x0f, 0x85, 0xc5, 0x01, 0x00, 0x00
	//0xe8, 0x17, 0x03, 0x05, 0x00	// call __do_global_ctors_aux
	0xc9,				// leave
	0xc3				// retn
};
*/

/*
unsigned char insn[] = {
	0x55,				// push	ebp
	0x89, 0xe5,			// mov	ebp, esp
	0x83, 0xec, 0x10,		// sub	esp, 10h
	0x8d, 0x45, 0x10,		// lea	eax, [ebp+arg_8]
	0x89, 0x45, 0xfc,		// mov	[ebp+var_4], eax
	0x89, 0x44, 0x24, 0x08,		// mov	[esp+8], eax
	0x8b, 0x45, 0x0c,		// mov	eax, [ebp+arg_4]
	0x89, 0x44, 0x24, 0x04,		// mov	[esp+4], eax
	0x8b, 0x45, 0x08,		// mov	eax, [ebp+arg_0]
	0x89, 0x04, 0x24,		// mov	[esp], eax
	0xe8, 0x5e, 0x37, 0x00, 0x00,	// call	vdprintf
	0xc9,				// leave
	0xc3				// retn
};
*/

// == atol( arg_0 ) ==

/*
unsigned char insn[] = {
	0x55,				// push	ebp
	0x31, 0xc0,			// xor	eax, eax
	0x89, 0xe5,			// mov	ebp, esp
	0x83, 0xec, 0x10,		// sub	esp, 10h
	0x89, 0x44, 0x24, 0x04,		// mov	[esp+4], eax
	0x8b, 0x45, 0x08,		// mov	eax, [ebp+arg_0]
	0x31, 0xc9,			// xor	ecx, ecx
	0xba, 0x0a, 0x00, 0x00,		// mov	edx, 0Ah
	0x89, 0x4c, 0x24, 0x0c,		// mov	[esp+0Ch], ecx
	0x89, 0x54, 0x24, 0x08,		// mov	[esp+8], edx
	0x89, 0x04, 0x24,		// mov	[esp], eax
	0xe8, 0xda, 0x01, 0x00, 0x00,	// call	__strtol_internal
	0xc9,				// leave
	0xc3				// retn
};
*/

unsigned char insn1[] = {
	0x55,						// push   %ebp
	0x89, 0xe5, 					// mov    %esp,%ebp
	//0x83, 0xec, 0x14,				// sub    $0x14,%esp
	0x8b, 0x45, 0x08,				// mov    0x8(%ebp),%eax
	0x88, 0x45, 0xec,				// mov    %al,-0x14(%ebp)
	0xc7, 0x45, 0xfc, 0x01, 0x00, 0x00, 0x00,	// movl   $0x1,-0x4(%ebp)
	//0x80, 0x7d, 0xec, 0x00,				// cmpb   $0x0,-0x14(%ebp)
	//0x74, 0x04,					// je     80483e1 <_Z2fnb+0x1d>
	0x83, 0x45, 0xfc, 0x01,				// addl   $0x1,-0x4(%ebp)
	0x8b, 0x45, 0xfc,				// mov    -0x4(%ebp),%eax
	0xc9,						// leave
	0xc3						// ret
};

// 34 bytes
unsigned char insn2[] = {
	0x55,						// c4  push   %ebp
	0x89, 0xe5, 					// c5 mov    %esp,%ebp
	0x83, 0xec, 0x14,				// c7 sub    $0x14,%esp
	0x8b, 0x45, 0x08,				// ca mov    0x8(%ebp),%eax
	0x88, 0x45, 0xec,				// cd mov    %al,-0x14(%ebp)
	0xc7, 0x45, 0xfc, 0x01, 0x00, 0x00, 0x00,	// d0 movl   $0x1,-0x4(%ebp)
	0x80, 0x7d, 0xec, 0x00,				// d7 cmpb   $0x0,-0x14(%ebp)
	0x74, 0x04,					// db je     80483e1 <_Z2fnb+0x1d>
	/*0x83, 0x45, 0xfc, 0x01,*/0x90, 0x90, 0x90, 0x90, // dd addl   $0x1,-0x4(%ebp)
	0x8b, 0x45, 0xfc,				// e1 mov    -0x4(%ebp),%eax
	0xc9,						// e4 leave
	0xc3						// e5 ret
};
unsigned char insn2a[] = {
	0x55,						// c4  push   %ebp
	0x89, 0xe5, 					// c5 mov    %esp,%ebp
	0x83, 0xec, 0x14,				// c7 sub    $0x14,%esp
	0x8b, 0x45, 0x08,				// ca mov    0x8(%ebp),%eax
	0x88, 0x45, 0xec,				// cd mov    %al,-0x14(%ebp)
	0xc7, 0x45, 0xfc, 0x01, 0x00, 0x00, 0x00,	// d0 movl   $0x1,-0x4(%ebp)
	0x80, 0x7d, 0xec, 0x00,				// d7 cmpb   $0x0,-0x14(%ebp)
	0x74, 0x04					// db je     80483e1 <_Z2fnb+0x1d>
};
unsigned char insn2b[] = {
	0x83, 0x45, 0xfc, 0x01				// dd addl   $0x1,-0x4(%ebp)
};
unsigned char insn2c[] = {
	0x8b, 0x45, 0xfc,				// e1 mov    -0x4(%ebp),%eax
	0xc9,						// e4 leave
	0xc3						// e5 ret
};

// 34 bytes
unsigned char insn3[] = {
	0x55,						// c4 push   %ebp
	0x89, 0xe5,					// c5 mov    %esp,%ebp
	0x83, 0xec, 0x10,				// c7 sub    $0x10,%esp
	0xc7, 0x45, 0xfc, 0x01, 0x00, 0x00, 0x00,	// ca movl   $0x1,-0x4(%ebp)
	0xeb, 0x08,					// d1 jmp    80483db <_Z2fni+0x17>
	0x83, 0x6d, 0x08, 0x01,				// d3 subl   $0x1,0x8(%ebp)
	0x83, 0x45, 0xfc, 0x01,				// d7 addl   $0x1,-0x4(%ebp)
	0x83, 0x7d, 0x08, 0x00,				// db cmpl   $0x0,0x8(%ebp)
	0x7f, 0xf2,					// df jg     80483d3 <_Z2fni+0xf>
	0x8b, 0x45, 0xfc,				// e1 mov    -0x4(%ebp),%eax
	0xc9,						// e4 leave
	0xc3						// e5 ret
};

// 50 bytes
unsigned char insn4[] = {
	0x55,                   			// c4 push   %ebp
	0x89, 0xe5,                			// c5 mov    %esp,%ebp
	0x83, 0xec, 0x10,             			// c7 sub    $0x10,%esp
	0xc7, 0x45, 0xfc, 0x01, 0x00, 0x00, 0x00, 	// ca movl   $0x1,-0x4(%ebp)
	0xeb, 0x08,                			// d1 jmp    80483db <_Z2fni+0x17>
	0x83, 0x6d, 0x08, 0x01,          		// d3 subl   $0x1,0x8(%ebp)
	0x83, 0x45, 0xfc, 0x01,          		// d7 addl   $0x1,-0x4(%ebp)
	0x83, 0x7d, 0x08, 0x00,          		// db cmpl   $0x0,0x8(%ebp)
	0x7f, 0xf2,                			// df jg     80483d3 <_Z2fni+0xf>
	0xeb, 0x08,                			// e1 jmp    80483eb <_Z2fni+0x27>
	0x83, 0x45, 0x08, 0x01,          		// e3 addl   $0x1,0x8(%ebp)
	0x83, 0x6d, 0xfc, 0x01,          		// e7 subl   $0x1,-0x4(%ebp)
	0x83, 0x7d, 0x08, 0x09,          		// eb cmpl   $0x9,0x8(%ebp)
	0x7e, 0xf2,                			// ef jle    80483e3 <_Z2fni+0x1f>
	0x8b, 0x45, 0xfc,             			// f1 mov    -0x4(%ebp),%eax
	0xc9,                   			// f4 leave
	0xc3                   				// f5 ret
};


// 57 bytes
unsigned char insn5[] = {
	0x55,                   			// c4 push   %ebp
	0x89, 0xe5,                			// c5 mov    %esp,%ebp
	0x83, 0xec, 0x10,             			// c7 sub    $0x10,%esp
	0xc7, 0x45, 0xfc, 0x01, 0x00, 0x00, 0x00, 	// ca movl   $0x1,-0x4(%ebp)
	0xeb, 0x18,                			// d1 jmp    80483eb <_Z2fni+0x27>
	0x83, 0x6d, 0x08, 0x01,          		// d3 subl   $0x1,0x8(%ebp)
	0x83, 0x45, 0xfc, 0x01,          		// d7 addl   $0x1,-0x4(%ebp)
	0xeb, 0x08,                			// db jmp    80483e5 <_Z2fni+0x21>
	0x83, 0x45, 0x08, 0x01,          		// dd addl   $0x1,0x8(%ebp)
	0x83, 0x6d, 0xfc, 0x01,          		// e1 subl   $0x1,-0x4(%ebp)
	0x83, 0x7d, 0x08, 0x09,          		// e5 cmpl   $0x9,0x8(%ebp)
	0x7e, 0xf2,                			// e9 jle    80483dd <_Z2fni+0x19>
	0x83, 0x7d, 0x08, 0x00,          		// eb cmpl   $0x0,0x8(%ebp)
	0x7f, 0xe2,                			// ef jg     80483d3 <_Z2fni+0xf>
	0xc7, 0x45, 0xfc, 0x00, 0x00, 0x00, 0x00, 	// f1 movl   $0x0,-0x4(%ebp)
	0x8b, 0x45, 0xfc,             			// f8 mov    -0x4(%ebp),%eax
	0xc9,                   			// fb leave
	0xc3                   				// fc ret
};


// 15 bytes
unsigned char insn6a[] = {
	0x55,						// c4 push   %ebp
	0x89, 0xe5,					// c5 mov    %esp,%ebp
	0x83, 0xec, 0x10,				// c7 sub    $0x10,%esp
	0xc7, 0x45, 0xfc, 0x01, 0x00, 0x00, 0x00,	// ca movl   $0x1,-0x4(%ebp)
	0xeb, 0x08					// d1 jmp    80483db <_Z2fni+0x17>
};

// 8 bytes
unsigned char insn6b[] = {
	0x83, 0x6d, 0x08, 0x01,				// d3 subl   $0x1,0x8(%ebp)
	0x83, 0x45, 0xfc, 0x01,				// d7 addl   $0x1,-0x4(%ebp)
};

// 6 bytes
unsigned char insn6c[] = {
	0x83, 0x7d, 0x08, 0x00,				// db cmpl   $0x0,0x8(%ebp)
	0x7f, 0xf2,					// df jg     80483d3 <_Z2fni+0xf>
};

// 5 bytes
unsigned char insn6d[] = {
	0x8b, 0x45, 0xfc,				// e1 mov    -0x4(%ebp),%eax
	0xc9,						// e4 leave
	0xc3						// e5 ret
};

// 13 bytes
unsigned char insn7[] = {
	0x55,						// push   %ebp
	0x89, 0xe5,					// mov    %esp,%ebp
	0x8b, 0x55, 0x0c,				// mov    0xc(%ebp),%edx
	0x8b, 0x45, 0x08,				// mov    0x8(%ebp),%eax
	0x29, 0xd0,					// sub    %edx,%eax
	0x5d,						// pop    %ebp
	0xc3						// ret
};

unsigned char insn8[] = {
	0x55,						// push   %ebp
	0x89, 0xe5,					// mov    %esp,%ebp
	0x8b, 0x55, 0x08,				// mov    0x8(%ebp),%edx
	0x8b, 0x45, 0x0c,				// mov    0xc(%ebp),%eax
	0x29, 0xd0,					// sub    %edx,%eax
	0x5d,						// pop    %ebp
	0xc3						// ret
};

// 33 bytes, 0x804bd10
unsigned char insn9a[] = {
	0x55,
	0x89, 0xe5,
	0x83, 0xec, 0x18,
	0x8b, 0x45, 0x08,
	0xc7, 0x44, 0x24, 0x08, 0x01, 0x00, 0x00, 0x00,
	0xc7, 0x44, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00,
	0x89, 0x04, 0x24,
	0xe8, 0x2f, 0xfd, 0xff, 0xff
};

// 2 bytes, 0x804bd31
unsigned char insn9b[] = {
	0xc9,
	0xc3
};

// 15 bytes, 0x080502c0
unsigned char insn10[] =
{
	0x55,
	0x89, 0xe5,
	0x8b, 0x45, 0x0c,
	0x89, 0x45, 0x08,
	0x5d,
	0xe9, 0xcd, 0x84, 0xff, 0xff
};

// 13 bytes, 0x00002100
unsigned char insn11a[] =
{
	0x55,
	0x89, 0xe5,
	0x8b, 0x4d, 0x0c,
	0x8b, 0x55, 0x08,
	0x85, 0xc9,
	0x75, 0x0b
};

// 5 bytes, 0x0000210d
unsigned char insn11b[] =
{
	0xba, 0xff, 0xff, 0xff, 0xff
};

// 4 bytes, 0x00002112
unsigned char insn11c[] =
{
	0x89, 0xd0,
	0x5d,
	0xc3
};

// 6 bytes, 0x00002118
unsigned char insn11d[] =
{
	0x80, 0x79, 0x5c, 0x72,
	0x75, 0xef
};

// 12 bytes, 0x0000211e
unsigned char insn11e[] =
{
	0x83, 0xfa, 0xff,
	0x8d, 0xb4, 0x26, 0x00, 0x00, 0x00, 0x00,
	0x74, 0xe3
};

// 8 bytes, 0x0000212a
unsigned char insn11f[] =
{
	0x83, 0x79, 0x6c, 0xff,
	0x66, 0x90,
	0x75, 0xdb
};

// 23 bytes, 0x00002132
unsigned char insn11g[] =
{
	0x31, 0xc0,
	0x83, 0x69, 0x68, 0x01,
	0x83, 0x79, 0x38, 0x01,
	0x89, 0x51, 0x6c,
	0x0f, 0x94, 0xc0,
	0x85, 0xc0,
	0x89, 0x41, 0x70,
	0x74, 0x07
};

// 7 bytes, 0x00002149
unsigned char insn11h[] =
{
	0xc7, 0x41, 0x38, 0x00, 0x00, 0x00, 0x00
};

// 9 bytes, 0x00002150
unsigned char insn11i[] =
{
	0xc7, 0x41, 0x3c, 0x00, 0x00, 0x00, 0x00,
	0xeb, 0xb9
};

// 20 bytes, 0x804B350
unsigned char insn12[] =
{
	0x55,
	0xb9, 0xff, 0xff, 0xff, 0xff,
	0x89, 0xe5,
	0x8b, 0x45, 0x08,
	0x8b, 0x55, 0x0c,
	0x5d,
	0xe9, 0x2c, 0xfc, 0xff, 0xff
};

// 72 bytes, 0x0804CBC0
unsigned char insn13[] =
{
	0x55,
	0x89, 0xe5,
	0x83, 0xec, 0x28,
	0x8b, 0x45, 0x14,
	0xc7, 0x44, 0x24, 0x14, 0x00, 0x00, 0x00, 0x00,
	0xc7, 0x44, 0x24, 0x10, 0x08, 0x00, 0x00, 0x00,
	0xc7, 0x44, 0x24, 0x0c, 0x0f, 0x00, 0x00, 0x00,
	0x89, 0x44, 0x24, 0x1c,
	0x8b, 0x45, 0x10,
	0xc7, 0x44, 0x24, 0x08, 0x08, 0x00, 0x00, 0x00,
	0x89, 0x44, 0x24, 0x18,
	0x8b, 0x45, 0x0c,
	0x89, 0x44, 0x24, 0x04,
	0x8b, 0x45, 0x08,
	0x89, 0x04, 0x24,
	0xe8, 0x1a, 0xfc, 0xff, 0xff,
	0xc9,
	0xc3
};

// 14 bytes , 0x080729e0 (rsync:deflatePrime)
unsigned char insn14a[] =
{
	0x55,			//push   %ebp
	0x89, 0xe5,		//mov    %esp,%ebp
	0x53,			//push   %ebx
	0x8b, 0x45, 0x08,	//mov    0x8(%ebp),%eax
	0x8b, 0x5d, 0x10,	//mov    0x10(%ebp),%ebx
	0x85, 0xc0,		//test   %eax,%eax
	0x74, 0x07		//je     80729f5 <deflatePrime+0x15>
};

// 7 bytes, 0x080729ee
unsigned char insn14b[] =
{
	0x8b, 0x50, 0x1c,	//mov    0x1c(%eax),%edx
	0x85, 0xd2,		//test   %edx,%edx
	0x75, 0x0b		//jne    8072a00 <deflatePrime+0x20>
};

// 5 bytes, 0x80729f5
unsigned char insn14c[] =
{
	0xb8, 0xfe, 0xff, 0xff, 0xff	//mov    $0xfffffffe,%eax
};

// 3 bytes, 0x80729fa
unsigned char insn14d[] =
{
	0x5b,				//pop    %ebx
	0x5d,				//pop    %ebp
	0xc3				//ret    
};

// 34 bytes, 0x08072a00
unsigned char insn14e[] =
{
	0x0f, 0xb6, 0x4d, 0x0c,				//movzbl 0xc(%ebp),%ecx
	0x8b, 0x45, 0x0c,				//mov    0xc(%ebp),%eax
	0x89, 0x82, 0xbc, 0x16, 0x00, 0x00,		//mov    %eax,0x16bc(%edx)
	0xb8, 0x01, 0x00, 0x00, 0x00,			//mov    $0x1,%eax
	0xd3, 0xe0,					//shl    %cl,%eax
	0x48,						//dec    %eax
	0x21, 0xc3,					//and    %eax,%ebx
	0x31, 0xc0,					//xor    %eax,%eax
	0x66, 0x89, 0x9a, 0xb8, 0x16, 0x00, 0x00,	//mov    %bx,0x16b8(%edx)
	0xeb, 0xd8					//jmp    80729fa <deflatePrime+0x1a>
};

// 10 bytes, 0x08000970 (libz_32.so.1.2.2:deflatePrime)
unsigned char insn15a[] =
{
	0x55,				//push   %ebp
	0x89, 0xe5,			//mov    %esp,%ebp
	0x8b, 0x45, 0x08,		//mov    0x8(%ebp),%eax
	0x85, 0xc0,			//test   %eax,%eax
	0x75, 0x0e			//jne    988 <deflatePrime+0x18>
};

// 7 bytes, 0x0800097a
unsigned char insn15b[] =
{
	0xb8, 0xfe, 0xff, 0xff, 0xff,	//mov    $0xfffffffe,%eax
	0x5d,				//pop    %ebp
	0xc3				//ret    
};

// 7 bytes, 0x08000988
unsigned char insn15c[] =
{
	0x8b, 0x50, 0x1c,				//mov    0x1c(%eax),%edx
	0x85, 0xd2,					//test   %edx,%edx
	0x74, 0xeb					//je     97a <deflatePrime+0xa>
};

// 38 bytes, 0x0800098f
unsigned char insn15d[] =
{
	0x8b, 0x45, 0x0c,				//mov    0xc(%ebp),%eax
	0x0f, 0xb6, 0x4d, 0x0c,				//movzbl 0xc(%ebp),%ecx
	0x89, 0x82, 0xb4, 0x16, 0x00, 0x00,		//mov    %eax,0x16b4(%edx)
	0xb8, 0x01, 0x00, 0x00, 0x00,			//mov    $0x1,%eax
	0xd3, 0xe0,					//shl    %cl,%eax
	0x83, 0xe8, 0x01,				//sub    $0x1,%eax
	0x66, 0x23, 0x45, 0x10,				//and    0x10(%ebp),%ax
	0x66, 0x89, 0x82, 0xb0, 0x16, 0x00, 0x00,	//mov    %ax,0x16b0(%edx)
	0x31, 0xc0,					//xor    %eax,%eax
	0x5d,						//pop    %ebp
	0xc3						//ret    
};

// 22 bytes, 0x080001d0 (libz_32.so.1.2.2:compressBound)
unsigned char insn16[] =
{
	0x55,
	0x89, 0xe5,
	0x8b, 0x55, 0x08,
	0x5d,
	0x89, 0xd0,
	0xc1, 0xe8, 0x0c,
	0x8d, 0x44, 0x02, 0x0b,
	0xc1, 0xea, 0x0e,
	0x01, 0xd0,
	0xc3
};


// 13 bytes, 0x807bc60 (rsync:poptGetArg)
unsigned char insn17a[] =
{
	0x55,
	0x89, 0xe5,
	0x53,
	0x8b, 0x55, 0x08,
	0x31, 0xdb,
	0x85, 0xd2,
	0x74, 0x22
};

// 10 bytes, 0x807bc6d
unsigned char insn17b[] =
{
	0x8b, 0x8a, 0x44, 0x01, 0x00, 0x00,
	0x85, 0xc9,
	0x74, 0x18
};

// 14 bytes, 0x807bc77
unsigned char insn17c[] =
{
	0x8b, 0x82, 0x4c, 0x01, 0x00, 0x00,
	0x3b, 0x82, 0x48, 0x01, 0x00, 0x00,
	0x7d, 0x0a
};

// 10 bytes, 0x807bc85
unsigned char insn17d[] =
{
	0x8b, 0x1c, 0x81,
	0x40,
	0x89, 0x82, 0x4c, 0x01, 0x00, 0x00
};

// 5 bytes, 0x807bc8f
unsigned char insn17e[] =
{
	0x89, 0xd8,
	0x5b,
	0x5d,
	0xc3
};

// 22 bytes, 0x08070d80 (rsync:doliteral)
unsigned char insn18a[] =
{
	0x55,                   	//push   %ebp
	0x89, 0xe5,                	//mov    %esp,%ebp
	0x57,                   	//push   %edi
	0x8b, 0x4d, 0x0c,             	//mov    0xc(%ebp),%ecx
	0x56,                   	//push   %esi
	0x8b, 0x75, 0x08,             	//mov    0x8(%ebp),%esi
	0x53,                   	//push   %ebx
	0x8b, 0x5d, 0x10,             	//mov    0x10(%ebp),%ebx
	0x0f, 0xb6, 0x06,             	//movzbl (%esi),%eax
	0x84, 0xc0,                	//test   %al,%al
	0x74, 0x3a                	//je     8070dd0 <doliteral+0x50>
};

// 7 bytes,  0x08070d96
unsigned char insn18b[] =
{
	0x0f, 0xb6, 0x11,             	//movzbl (%ecx),%edx
	0x84, 0xd2,                	//test   %dl,%dl
	0x75, 0x15                	//jne    8070db2 <doliteral+0x32>
};

// 3 bytes, 0x08070d9d
unsigned char insn18c[] =
{
	0x8d, 0x76, 0x00             	//lea    0x0(%esi),%esi
};

// 11 bytes,  0x08070da0
unsigned char insn18d[] =
{
	0x8b, 0x0b,                	//mov    (%ebx),%ecx
	0x31, 0xff,                	//xor    %edi,%edi
	0x83, 0xc3, 0x04,             	//add    $0x4,%ebx
	0x85, 0xc9,                	//test   %ecx,%ecx
	0x74, 0x38                	//je     8070de3 <doliteral+0x63>
};

// 7 bytes, 0x08070dab
unsigned char insn18e[] =
{
	0x0f, 0xb6, 0x11,             	//movzbl (%ecx),%edx
	0x84, 0xd2,                	//test   %dl,%dl
	0x74, 0xee                	//je     8070da0 <doliteral+0x20>
};

// 6 bytes, 0x08070db2
unsigned char insn18f[] =
{
	0x31, 0xff,                	//xor    %edi,%edi
	0x38, 0xc2,                	//cmp    %al,%dl
	0x75, 0x2b                	//jne    8070de3 <doliteral+0x63>
};

// 9 bytes, 0x08070db8
unsigned char insn18g[] =
{
	0x46,                   	//inc    %esi
	0x41,                   	//inc    %ecx
	0x0f, 0xb6, 0x06,             	//movzbl (%esi),%eax
	0x84, 0xc0,                	//test   %al,%al
	0x75, 0xd5                	//jne    8070d96 <doliteral+0x16>
};

// 2 bytes, 0x08070dc1
unsigned char insn18h[] =
{
	0xeb, 0x0d                	//jmp    8070dd0 <doliteral+0x50>
};

// 5 bytes, 0x08070dd0
unsigned char insn18i[] =
{
	0x80, 0x39, 0x00,             	//cmpb   $0x0,(%ecx)
	0x75, 0x15                	//jne    8070dea <doliteral+0x6a>
};

// 9 bytes, 0x08070dd5
unsigned char insn18j[] =
{
	0x8b, 0x0b,                	//mov    (%ebx),%ecx
	0x83, 0xc3, 0x04,             	//add    $0x4,%ebx
	0x85, 0xc9,                	//test   %ecx,%ecx
	0x75, 0xf2                	//jne    8070dd0 <doliteral+0x50>
};

// 5 bytes,  0x08070dde
unsigned char insn18k[] =
{
	0xbf, 0x01, 0x00, 0x00, 0x00  	//mov    $0x1,%edi
};

// 7 bytes,  0x08070de3
unsigned char insn18l[] =
{
	0x5b,                   	//pop    %ebx
	0x89, 0xf8,                	//mov    %edi,%eax
	0x5e,                   	//pop    %esi
	0x5f,                   	//pop    %edi
	0x5d,                   	//pop    %ebp
	0xc3                    	//ret    
};

// 4 bytes, 0x08070dea
unsigned char insn18m[] =
{
	0x31, 0xff,                	//xor    %edi,%edi
	0xeb, 0xf5                	//jmp    8070de3 <doliteral+0x63>
};
 
// 10 bytes,  0x0807ca10
unsigned char insn19a[] =
{
	0x55,                   	//push   %ebp
	0x89, 0xe5,                	//mov    %esp,%ebp
	0x8b, 0x45, 0x08,             	//mov    0x8(%ebp),%eax
	0x85, 0xc0,                	//test   %eax,%eax
	0x74, 0x22                	//je     807ca3c <getTableTranslationDomain+0x2c>
};

// 6 bytes,  0x0807ca1a
unsigned char insn19b[] =
{
	0x8b, 0x08,                	//mov    (%eax),%ecx
	0x85, 0xc9,                	//test   %ecx,%ecx
	0x74, 0x25                	//je     807ca45 <getTableTranslationDomain+0x35>
};

// 6 bytes,  0x807ca20
unsigned char insn19c[] =
{

	0x83, 0x78, 0x08, 0x06,          	//cmpl   $0x6,0x8(%eax)
	0x74, 0x1a                	//je     807ca40 <getTableTranslationDomain+0x30>
};

// 9 bytes,  0x807ca26
unsigned char insn19d[] =
{

	0x83, 0xc0, 0x1c,             	//add    $0x1c,%eax
	0x8b, 0x08,                	//mov    (%eax),%ecx
	0x85, 0xc9,                	//test   %ecx,%ecx
	0x75, 0xf1                	//jne    807ca20 <getTableTranslationDomain+0x10>
};

// 6  bytes,  0x0807ca2f
unsigned char insn19e[] =
{

	0x80, 0x78, 0x04, 0x00,          	//cmpb   $0x0,0x4(%eax)
	0x75, 0xeb                	//jne    807ca20 <getTableTranslationDomain+0x10>
};

// 7 bytes,  0x0807ca35
unsigned char insn19f[] =
{

	0x8b, 0x50, 0x0c,             	//mov    0xc(%eax),%edx
	0x85, 0xd2,                	//test   %edx,%edx
	0x75, 0xe4                	//jne    807ca20 <getTableTranslationDomain+0x10>
};

// 4 bytes,  0x0807ca3c
unsigned char insn19g[] =
{

	0x31, 0xc0                	//xor    %eax,%eax
};

// 2 bytes, 0x0807ca3e
unsigned char insn19h[] =
{
	0x5d,                   	//pop    %ebp
	0xc3                   	//ret    
};

// 5 bytes,  0x0807ca40
unsigned char insn19i[] =
{

	0x8b, 0x40, 0x0c,             	//mov    0xc(%eax),%eax
	0xeb, 0xf9                	//jmp    807ca3e <getTableTranslationDomain+0x2e>
};

// 6 bytes,  0x0807ca45
unsigned char insn19j[] =
{

	0x80, 0x78, 0x04, 0x00,          	//cmpb   $0x0,0x4(%eax)
	0x75, 0xd5                	//jne    807ca20 <getTableTranslationDomain+0x10>
};

// 7 bytes,  0x0807ca4b
unsigned char insn19k[] =
{

	0x8b, 0x50, 0x0c,             	//mov    0xc(%eax),%edx
	0x85, 0xd2,                	//test   %edx,%edx
	0x75, 0xce                	//jne    807ca20 <getTableTranslationDomain+0x10>
};

// 2 bytes, 0x0807ca52
unsigned char insn19l[] =
{

	0xeb, 0xe8                	//jmp    807ca3c <getTableTranslationDomain+0x2c>
};

// 23 bytes, 0x080797f0
unsigned char insn20[] = 
{
	0x55,                   	//push   %ebp
	0x89, 0xe5,                	//mov    %esp,%ebp
	0x8b, 0x55, 0x08,             	//mov    0x8(%ebp),%edx
	0x5d,                   	//pop    %ebp
	0x89, 0xd0,                	//mov    %edx,%eax
	0xc1, 0xe8, 0x0c,             	//shr    $0xc,%eax
	0x8d, 0x04, 0x10,             	//lea    (%eax,%edx,1),%eax
	0xc1, 0xea, 0x0e,             	//shr    $0xe,%edx
	0x8d, 0x44, 0x02, 0x0b,          //	lea    0xb(%edx,%eax,1),%eax
	0xc3
};

// 32 bytes, 0x00004680 (libz_32.so.1.2.2:gzgetc)
unsigned char insn21a[] =
{
	0x55,                   	//push   %ebp
	0x89, 0xe5,                	//mov    %esp,%ebp
	0x83, 0xec, 0x28,             	//sub    $0x28,%esp
	0x8d, 0x45, 0xff,             	//lea    -0x1(%ebp),%eax
	0x89, 0x44, 0x24, 0x04,          	//mov    %eax,0x4(%esp)
	0x8b, 0x45, 0x08,             	//mov    0x8(%ebp),%eax
	0xc7, 0x44, 0x24, 0x08, 0x01, 0x00, 0x00, 	//movl   $0x1,0x8(%esp)
	0x00, 
	0x89, 0x04, 0x24,             	//mov    %eax,(%esp)
	0xe8, 0xfc, 0xff, 0xff, 0xff       	//call   469c <gzgetc+0x1c>
};

// 10 bytes, 0x000046a0
unsigned char insn21b[] =
{
	0xba, 0xff, 0xff, 0xff, 0xff,       	//mov    $0xffffffff,%edx
	0x83, 0xe8, 0x01,             	//sub    $0x1,%eax
	0x75, 0x04                	//jne    46ae <gzgetc+0x2e>
};

// 4 bytes, 0x000046aa
unsigned char insn21c[] =
{
	0x0f, 0xb6, 0x55, 0xff          	//movzbl -0x1(%ebp),%edx
};

// 4 bytes, 0x000046ae
unsigned char insn21d[] =
{
	0x89, 0xd0,                	//mov    %edx,%eax
	0xc9,                   	//leave  
	0xc3                   	//ret    
};

// 32 bytes,  08064ee0 (rsync:read_byte)
unsigned char insn22a[] =
{
	0x55,                   	//push   %ebp
	0x89, 0xe5,                	//mov    %esp,%ebp
	0x83, 0xec, 0x18,             	//sub    $0x18,%esp
	0xc7, 0x44, 0x24, 0x08, 0x01, 0x00, 0x00, 	//movl   $0x1,0x8(%esp)
	0x00, 
	0x8d, 0x45, 0xff,             	//lea    -0x1(%ebp),%eax
	0x89, 0x44, 0x24, 0x04,          	//mov    %eax,0x4(%esp)
	0x8b, 0x45, 0x08,             	//mov    0x8(%ebp),%eax
	0x89, 0x04, 0x24,             	//mov    %eax,(%esp)
	0xe8, 0x00, 0xfe, 0xff, 0xff       	//call   8064d00 <readfd>
};

// 8 bytes, 08064f00
unsigned char insn22b[] =
{
	0x0f, 0xb6, 0x45, 0xff,          	//movzbl -0x1(%ebp),%eax
	0x89, 0xec,                	//mov    %ebp,%esp
	0x5d,                   	//pop    %ebp
	0xc3                   	//ret    
};

// 10 bytes, 0x8079810 (rsync:get_crc_table)
unsigned char insn23[] = 
{
	0x55,                   	//push   %ebp
	0x89, 0xe5,                	//mov    %esp,%ebp
	0xb8, 0x60, 0xa4, 0x08, 0x08,       	//mov    $0x808a460,%eax
	0x5d,                   	//pop    %ebp
	0xc3                   	//ret    
};


//10 bytes,  0x00000600 (libz_32.so.1.2.3:get_crc_table)
unsigned char insn24[] =
{
	0x55,                   	//push   %ebp
	0xb8, 0x00, 0x00, 0x00, 0x00,       	//mov    $0x0,%eax
	0x89, 0xe5,                	//mov    %esp,%ebp
	0x5d,                   	//pop    %ebp
	0xc3                   	//ret    
};

void InitFuncs( log4cpp::Category *_log, vector<RexFunc*> &rexfuncs, const unsigned int fid )
{

	RexFunc *rexfunc;

	/*
	// #0
	rexfunc = new RexFunc( _log, fid, 0x080483c4 );
	rexfunc->AddBB( insn2a, 25, 0x080483c4, 0x080483e1, 0x080483dd );
	rexfunc->AddBB( insn2b, 4, 0x080483dd, 0x080483e1, BADADDR );
	rexfunc->AddBB( insn2c, 5, 0x080483e1, BADADDR, BADADDR );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back(rexfunc);

	// #1
	rexfunc = new RexFunc( _log, fid, 0x080483c4 );
	rexfunc->AddBB( insn1, 25, 0x080483c4 );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back(rexfunc);

	// #2
	rexfunc = new RexFunc( _log, fid, 0x80483c4 );
	rexfunc->AddBB( insn6a, 15, 0x080483c4, 0x080483db, BADADDR );
	rexfunc->AddBB( insn6b, 8, 0x080483d3, 0x080483db, BADADDR );
	rexfunc->AddBB( insn6c, 6, 0x80483db, 0x080483d3, 0x080483e1 );
	rexfunc->AddBB( insn6d, 5, 0x080483e1, BADADDR, BADADDR );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back(rexfunc);

	// #3
	rexfunc = new RexFunc( _log, fid, 0x080483e6 );
	rexfunc->AddBB( insn7, 13, 0x080483e6 );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back(rexfunc);

	// #4
	rexfunc = new RexFunc( _log, fid, 0x080483e6 );
	rexfunc->AddBB( insn8, 13, 0x080483e6 );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back(rexfunc);

	// #5
	rexfunc = new RexFunc( _log, fid, 0x0804bd10 );
	rexfunc->AddBB( insn9a, 33, 0x0804bd10 );
	rexfunc->AddBB( insn9b, 2, 0x0804bd31 );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back( rexfunc );

	// #6
	rexfunc = new RexFunc( _log, fid, 0x080502c0 );
	rexfunc->AddBB( insn10, 15, 0x080502c0 );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back( rexfunc );

	// #7
	rexfunc = new RexFunc( _log, fid, 0x00002100 );
	rexfunc->AddBB( insn11a, 13, 0x00002100, 0x00002118, 0x0000210d );
	rexfunc->AddBB( insn11b,  5, 0x0000210d, 0x00002112, 0xffffffff );
	rexfunc->AddBB( insn11c,  4, 0x00002112, 0xffffffff, 0xffffffff );
	rexfunc->AddBB( insn11d,  6, 0x00002118, 0x0000210d, 0x0000211e );
	rexfunc->AddBB( insn11e, 12, 0x0000211e, 0x0000210d, 0x0000212a );
	rexfunc->AddBB( insn11f,  8, 0x0000212a, 0x0000210d, 0x00002132 );
	rexfunc->AddBB( insn11g, 23, 0x00002132, 0x00002150, 0x00002149 );
	rexfunc->AddBB( insn11h,  7, 0x00002149, 0x00002150, 0xffffffff );
	rexfunc->AddBB( insn11i,  9, 0x00002150, 0x00002112, 0xffffffff );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back( rexfunc );

	// #8
	rexfunc = new RexFunc( _log, fid, 0x0804B350 );
	rexfunc->AddBB( insn12, 20, 0x0804B350 );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back( rexfunc );

	// #9
	rexfunc = new RexFunc( _log, fid, 0x0804cbc0 );
	rexfunc->AddBB( insn13, 72, 0x0804cbc0 );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back( rexfunc );
	*/

	/*
	// #10
	rexfunc = new RexFunc( _log, fid, 0x080729e0 );
	rexfunc->AddBB( insn14a, 14, 0x080729e0, 0x080729f5, 0x080729ee );
	rexfunc->AddBB( insn14b, 7, 0x080729ee, 0x08072a00, 0x080729f5);
	rexfunc->AddBB( insn14c, 5, 0x080729f5, 0x080729fa );
	rexfunc->AddBB( insn14d, 3, 0x080729fa );
	rexfunc->AddBB( insn14e, 34, 0x08072a00, 0x080729fa );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back( rexfunc );
	
	// #11
	rexfunc = new RexFunc( _log, fid, 0x08000970 );
	rexfunc->AddBB( insn15a, 10, 0x08000970, 0x08000988, 0x0800097a );
	rexfunc->AddBB( insn15b, 7, 0x0800097a );
	rexfunc->AddBB( insn15c, 7, 0x08000988, 0x0800097a, 0x0800098f );
	rexfunc->AddBB( insn15d, 38, 0x0800098f );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back( rexfunc );
	*/

	/*
	// #12
	rexfunc = new RexFunc( _log, fid, 0x080001d0 );
	rexfunc->AddBB( insn16, 22, 0x080001d0 );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back( rexfunc );

	// #14
	rexfunc = new RexFunc( _log, fid, 0x0807bc60 );
	rexfunc->AddBB( insn17a, 13, 0x0807bc60, 0x0807bc8f, 0x0807bc6d );
	rexfunc->AddBB( insn17b, 10, 0x0807bc6d, 0x0807bc8f, 0x0807bc77 );
	rexfunc->AddBB( insn17c, 14, 0x0807bc77, 0x0807bc8f, 0x0807bc85 );
	rexfunc->AddBB( insn17d, 10, 0x0807bc85, 0x0807bc8f );
	rexfunc->AddBB( insn17e, 5, 0x0807bc8f );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back( rexfunc );

	rexfunc = new RexFunc( _log, fid, 0x08070d80 );
	rexfunc->AddBB( insn18a, 22, 0x08070d80, 0x08070dd0, 0x08070d96 );
	rexfunc->AddBB( insn18b, 7, 0x08070d96, 0x08070db2, 0x08070d9d );
	rexfunc->AddBB( insn18c, 3, 0x08070d9d, 0x08070da0 );
	rexfunc->AddBB( insn18d, 11, 0x08070da0, 0x08070de3, 0x08070dab );
	rexfunc->AddBB( insn18e, 7, 0x08070dab, 0x08070da0, 0x08070db2 );
	rexfunc->AddBB( insn18f, 6, 0x08070db2, 0x08070de3, 0x08070db8 );
	rexfunc->AddBB( insn18g, 9, 0x08070db8, 0x08070d96, 0x08070dc1 );
	rexfunc->AddBB( insn18h, 2, 0x08070dc1, 0x08070dd0 );
	rexfunc->AddBB( insn18i, 5, 0x08070dd0, 0x08070dea, 0x08070dd5 );
	rexfunc->AddBB( insn18j, 9, 0x08070dd5, 0x08070dd0, 0x08070dde );
	rexfunc->AddBB( insn18k, 5, 0x08070dde, 0x08070de3 );
	rexfunc->AddBB( insn18l, 7, 0x08070de3 );
	rexfunc->AddBB( insn18m, 4, 0x08070dea, 0x08070de3 );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back( rexfunc );
	// #15
	// Doesn't match. An address used is loaded from memory. We only
	// assert addresses stored on stack to be the same, not that of
	// addresses from memory. Probably function pointer.
	rexfunc = new RexFunc( _log, fid, 0x0807ca10 );
	rexfunc->AddBB( insn19a, 10, 0x0807ca10, 0x0807ca3c, 0x0807ca1a );
	rexfunc->AddBB( insn19b, 6, 0x0807ca1a, 0x0807ca45, 0x0807ca20 );
	rexfunc->AddBB( insn19c, 6, 0x0807ca20, 0x0807ca40, 0x0807ca26 );
	rexfunc->AddBB( insn19d, 9, 0x0807ca26, 0x0807ca20, 0x0807ca2f );
	rexfunc->AddBB( insn19e, 6, 0x0807ca2f, 0x0807ca20, 0x0807ca35 );
	rexfunc->AddBB( insn19f, 7, 0x0807ca35, 0x0807ca20, 0x0807ca3c );
	rexfunc->AddBB( insn19g, 2, 0x0807ca3c, 0x0807ca3e );
	rexfunc->AddBB( insn19h, 2, 0x0807ca3e );
	rexfunc->AddBB( insn19i, 5, 0x0807ca40, 0x0807ca3e );
	rexfunc->AddBB( insn19j, 6, 0x0807ca45, 0x0807ca20, 0x0807ca4b );
	rexfunc->AddBB( insn19k, 7, 0x0807ca4b, 0x0807ca20, 0x0807ca52 );
	rexfunc->AddBB( insn19l, 2, 0x0807ca52, 0x0807ca3c );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back( rexfunc );

	// #16
	rexfunc = new RexFunc( _log, fid, 0x080797f0 );
	rexfunc->AddBB( insn20, 23, 0x080797f0 );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back( rexfunc );
	*/

	/*
	rexfunc = new RexFunc( _log, fid, 0x00004680 );
	rexfunc->AddBB( insn21a, 32, 0x00004680, 0x000046a0 );
	rexfunc->AddBB( insn21b, 10, 0x000046a0, 0x000046ae, 0x000046aa );
	rexfunc->AddBB( insn21c, 4, 0x000046aa, 0x000046ae );
	rexfunc->AddBB( insn21d, 4, 0x000046ae );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back( rexfunc );

	rexfunc = new RexFunc( _log, fid, 0x08064ee0 );
	rexfunc->AddBB( insn22a, 32, 0x08064ee0, 0x08064f00 );
	rexfunc->AddBB( insn22b, 8, 0x08064f00 );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back( rexfunc );
	*/

	rexfunc = new RexFunc( _log, fid, 0x08079810 );
	rexfunc->AddBB( insn23, 10, 0x08079810 );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back( rexfunc );

	rexfunc = new RexFunc( _log, fid, 0x00000600 );
	rexfunc->AddBB( insn24, 10, 0x00000600 );
	rexfunc->FinalizeBBs();
	rexfuncs.push_back( rexfunc );

}

void DelFuncs( vector< RexFunc* > &rexfuncs )
{
	while ( rexfuncs.begin() != rexfuncs.end() )
	{
		delete rexfuncs.back();
		rexfuncs.pop_back();
	}
}


void handler(int sig)
{
	void *array[10];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	// print out all the frames to stderr
	fprintf(stderr, "Error: signal %d:\n", sig);
	backtrace_symbols_fd(array, size, 2);
	exit(1);
}


int main()
{
	signal(SIGSEGV, handler);

	//
	// Setup log4cpp
	//
	char logname[32];
	snprintf(logname, sizeof(logname), "log_rex_%d", getpid());

	log4cpp::PatternLayout *layout = new log4cpp::PatternLayout();
	layout->setConversionPattern("%m");
	log4cpp::FileAppender *appender = new log4cpp::FileAppender("RexFileAppender", logname);
	appender->setLayout(layout);
	log4cpp::Category &log = log4cpp::Category::getInstance("RexCategory");
	log.setAppender(appender);
	log.setPriority(log4cpp::Priority::DEBUG);


	RexUtils *rexutils1 = new RexUtils( &log );
	RexUtils *rexutils2 = new RexUtils( &log );
	RexTranslator *rextrans = new RexTranslator( &log );
	rextrans->InitVex();

	int count = 0;
	while( count < 1 )
	{

	vector< RexFunc *> rexfuncs0;
	vector< RexFunc *> rexfuncs1;
	InitFuncs( &log, rexfuncs0, 0 );
	InitFuncs( &log, rexfuncs1, 1 );

	REXSTATUS status = REX_ERR_UNK;
	vector<RexFunc*>::iterator b0, e0, b1, e1;
	int m, n;
	for ( m = 0, b0 = rexfuncs0.begin(), e0 = rexfuncs0.end(); b0 != e0; ++m, ++b0 )
	{
		RexFunc *f0 = *b0;

		for ( n = 0, b1 = rexfuncs1.begin(), e1 = rexfuncs1.end(); b1 != e1; ++n, ++b1 )
		{
			printf("======================= %d, %d ============================\n", m, n);
			RexFunc *f1 = *b1;

			Rex rex( &log, rexutils1, rexutils2, rextrans );

			#ifdef USE_STP_FILE
			char stpprefix[64];
			snprintf( stpprefix, 64, "test_%d_%d", m, n );
			status = rex.SemanticEquiv( stpprefix, *f0, *f1, 60 );
			#else
			status = rex.SemanticEquiv( *f0, *f1, 60 );
			#endif

			char stat = 'E';
			switch( status )
			{
			case REX_SE_TRUE: stat = '='; break;
			case REX_SE_FALSE: stat = '!'; break;
			case REX_SE_MISMATCH: stat = 'M'; break;
			case REX_SE_TIMEOUT: stat = 'T'; break;
			case REX_SE_ERR: stat = 'E'; break;
			case REX_TOO_MANY: stat = 'Z'; break;
			case REX_ERR_IR: stat = 'I'; break;
			case REX_ERR_UNK: stat = 'U'; break;
			default: stat = '?';
			}
			log.notice( "RESULT: Func %d %c= Func %d\n",
				m, stat, n );
		}
	}

	DelFuncs( rexfuncs0 );
	DelFuncs( rexfuncs1 );
	count++;
	}

	delete rextrans;
	delete rexutils1;
	delete rexutils2;

	// This will free the memory for the appender.
	// However, there is still a 48-byte memory leak
	// in Appender. To be more specific, "_allAppenders"
	// is allocated but never deleted. We'll live
	// with this for now, and deal with it when we have
	// abundant time.
	log.shutdown();
}
