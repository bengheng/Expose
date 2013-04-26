#ifndef __BLACKLIST_HEADER__
#define __BLACKLIST_HEADER__

#include <ida.hpp>
#include <idp.hpp>
#include <function_analyzer.h>

bool IsBlacklistedSegEA(const ea_t ea);
bool IsBlacklistedSegFN(func_t *pFn);
bool IsBlacklistedFunc(const char *funcname);

#endif
