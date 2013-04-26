#ifndef __LMFUNCTION_H_
#define __LMFUNCTION_H_

#include "lmcfg.h"
//#include "lmlog.h"
#include <ida.hpp>
#include <funcs.hpp>
#include "function_analyzer.h"

class LMFunction : public function_analyzer
{
public:
	bool HasXrefTo(ea_t refEA);
	void run_analysis();
	void PrintCalls();

	LMFunction(func_t *f
		//#ifdef VERBOSE
		//, LMLog *lmlog
		//#endif
		);
	~LMFunction();
private:
	//func_t *pFunc;
	LMCFG  *pCfg;
};

#endif
