#ifndef __LMOBJECT_H__
#define __LMOBJECT_H__

#include <log4cpp/Category.hh>

#include "lmfunction.h"
//#ifdef VERBOSE
//#include "lmlog.h"
//#endif
#include <ida.hpp>
#include <funcs.hpp>
#include <list>
//#include <gsl/gsl_matrix.h>
#include "../fcg/fcg.h"

class LMObject
{
public:
	int AddFunction(func_t *f);
	void PrintFuncCalls();
	int ComputeAdjacencyMatrix();
	void WriteFCG();

	LMObject(
		//#ifdef VERBOSE
		//LMLog *lmlog
		//#endif
		log4cpp::Category &cat
		);

	~LMObject();
private:
	std::vector<LMFunction*>	funcVec;	// Vector of functions
	//gsl_matrix			*pAdjMatrix;	// Ptr to Adjacency matrix
	FCGraph				*pFCG;		// Ptr to Function Call Graph
	char graphPath[1024];				// Path of Function Call Graph


	log4cpp::Category *log;
	//#ifdef VERBOSE
	//LMLog				*pLMLog;
	//#endif
	bool GetDotOutputFilename(char *const outname,
                                  size_t outname_size);
	bool FormPathInClientDot(
		char *const outpath,
		const size_t outpath_size,
		char *const inpath);

	bool FormPathInInpathFolder(
		char *const outpath,
		const size_t outpath_size,
		const char *const inpath,
		bool isInArchive);

	bool FileExists(string strFilename);

	bool ConvertArchiveInpath(
		char *inpath,
		const size_t inpathlen);
	
	void GetParentFolder(
		const char* path,
		char parentname[1024]);

	void RemoveDoubleSlash(char *const str);

};

#endif
