
#include "lmfunction.h"
#ifdef VERBOSE
#include "lmlog.h"
#endif
#include <ida.hpp>
#include <idp.hpp>
#include <funcs.hpp>


LMFunction::LMFunction(func_t *pF
	#ifdef VERBOSE
	, LMLog *lmlog
	#endif
	) : function_analyzer(pF->startEA
	#ifdef VERBOSE
	, lmlog
	#endif
	)
{
	//pFunc = pF;
	pCfg = new LMCFG();
}

LMFunction::~LMFunction()
{
	if(pCfg != NULL)
		delete pCfg;
}

void LMFunction::run_analysis()
{
	function_analyzer::run_analysis();
	pCfg->GenGraph(this);
	// Don't print control flow graph and Idoms yet
	//pCfg->ShowGraph();
	//pCfg->PrintIdoms();
	//pCfg->ComputeLoops();
}

#ifdef VERBOSE
void LMFunction::PrintCalls()
{
	if(get_num_calls() == 0)
		return;

	char funcname1[512];
	char funcname2[512];

	for(int i = 1; i <= get_num_calls(); ++i)
	{
		int dstord = get_call_dst(i);
		if(dstord == -1)
		{
			qsnprintf(pLMLog->GetBuffer(), pLMLog->GetBufferSize(),
				"[%d] Skipping\n", i);
			pLMLog->log();
			continue;
		}

		func_t *pF = getn_func(dstord);		

	
		get_func_name(get_ea_start(), funcname1, sizeof(funcname1));
		get_func_name(get_func(get_call_src(i))->startEA, funcname2, sizeof(funcname2));

		qsnprintf(pLMLog->GetBuffer(), pLMLog->GetBufferSize(), "[%d] %08x, %s: [%08x, %s]%08x calls [%08x]%08x\n",
		    i,
		    get_ea_start(), funcname1, get_func(get_call_src(i))->startEA, funcname2, get_call_src(i), get_func(pF->startEA)->startEA, pF->startEA);
		pLMLog->log();
	}
	qsnprintf(pLMLog->GetBuffer(), pLMLog->GetBufferSize(),
		"----------------------------------------------\n");
	pLMLog->log();

}
#endif

/*!
Returns true if there is a cross reference to the
specified refEA.
*/
bool LMFunction::HasXrefTo(ea_t refEA)
{
	if(refEA == BADADDR)     return false;
	if(get_num_calls() == 0) return false;

	for(int i = 1; i <= get_num_calls(); i++)
	{
		int dstord = get_call_dst(i);
		
		// Skip non-existing function
		if(dstord == -1) continue;
  
		func_t *pF = getn_func(dstord);
		if(pF->startEA == refEA)
			return true;
	}
	
/*	
	xrefblk_t xr;
	for(bool ok = xr.first_from(get_ea_start(), XREF_ALL);
	    ok;
	    ok = xr.next_from())
	{
		if(xr.iscode &&
		   (xr.type == fl_CN || xr.type == fl_CF) &&
		   get_func(refEA)->startEA == xr.to)
			return true;
	}
*/
	return false;
}
