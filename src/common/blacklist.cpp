#include <ida.hpp>
#include <idp.hpp>
#include <function_analyzer.h>
#include <blacklist.h>

/*!
 * Returns true if the function at funcStartEA is a blacklisted function,
 * defined as follows.
 *
 * 1. funcStartEA is not in a code segment.
 * 2. funcStartEA is in any of the segments "_init", "_fini" or "_plt".
 * 3. <Add More When Required>
 *
 * */
bool IsBlacklistedSegFN( func_t *pFn )
{
	return IsBlacklistedSegEA( pFn->startEA );
}

bool IsBlacklistedSegEA( const ea_t ea )
{
	// Skip none-code segment
	segment_t *seg = getseg(ea);
	if( seg->type != SEG_CODE ) return true;

	// Skip "_init", "_fini" and "_plt" segments
	char segname[MAXSTR];
	get_segm_name(seg->startEA, segname, sizeof(segname)-1 );
	if( strcmp(segname, "_init")  == 0 ||
	strcmp(segname, "_fini") == 0 ||
	strcmp(segname, "_plt") == 0 )
		return true;

	return false;

}

bool IsBlacklistedFunc( const char *funcname )
{
	if ( strcmp(funcname, "frame_dummy") == 0 ||
	strcmp(funcname, "__do_global_dtors_aux") == 0 ||
	strcmp(funcname, "__do_global_ctors_aux") == 0 ||
	strcmp(funcname, "__do_global_dtors") == 0 ||
	strcmp(funcname, "__do_global_ctors") == 0 ||
	strcmp(funcname, "__i686.get_pc_thunk.bx") == 0 ||
	strcmp(funcname, "__i686.get_pc_thunk.cx") == 0 ||
	strcmp(funcname, "__libc_csu_fini") == 0 ||
	strcmp(funcname, "__libc_csu_init") == 0 ||
	strcmp(funcname, "_start") == 0 ||
	strcmp(funcname, "start") == 0 ||
	strcmp(funcname, "_init_array") == 0 ||
	strcmp(funcname, "_fini_array") == 0 ||
	strcmp(funcname, "__init_array_start") == 0 ||
	strcmp(funcname, "__fini_array_start") == 0 ||
	strcmp(funcname, "__init_array_end") == 0 ||
	strcmp(funcname, "__fini_array_end") == 0 ||
	strcmp(funcname, "main") == 0 )
		return true;

	return false;
}
