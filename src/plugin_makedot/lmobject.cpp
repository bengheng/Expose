#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/SimpleLayout.hh>

#include "lmobject.h"
#include <ida.hpp>
#include <idp.hpp>
#include <nalt.hpp>
#include <funcs.hpp>
#include <ostream>
#include "../utils/utils.h"
#include <boost/graph/graphviz.hpp>
#include "sha1.h"

LMObject::LMObject(
	log4cpp::Category &category
)
{
	log = &category;
	log->info("Created LMObject...");

	// Allocate Function Call Graph
	pFCG = new FCGraph();

	// Checks if we're in an archive folder.
	// If so, read in the dot file if it exists.
	// And we'll need to save it to the same file.
	bool convertedInpath = GetDotOutputFilename(graphPath, sizeof(graphPath));

	// If we need to convert the input path, it means we're
	// in an "archive" folder. So we try to read in the graph
	// if it exists already, since this may be another object
	// in the archive.
	if( convertedInpath == true && FileExists(graphPath) == true)
	{
		msg("Reading graph from %s\n", graphPath);
		pFCG->ReadDOT(graphPath);

		// Delete the file after reading
		remove(graphPath);
	}
}

LMObject::~LMObject()
{
	// Deletes all functions in list
	funcVec.clear();

	//if(pAdjMatrix != NULL)
	//	gsl_matrix_free(pAdjMatrix);

	// Delete Function Call Graph
	delete pFCG;
}

int LMObject::AddFunction(func_t *f)
{
	char fname[1024];
	get_func_name(f->startEA, fname, sizeof(fname));


	if ( strcmp(fname, "frame_dummy") == 0 ||
	        strcmp(fname, "__do_global_dtors_aux") == 0 ||
	        strcmp(fname, "__do_global_ctors_aux") == 0 ||
	        strcmp(fname, "__do_global_dtors") == 0 ||
	        strcmp(fname, "__do_global_ctors") == 0 ||
	        strcmp(fname, "__i686.get_pc_thunk.bx") == 0 ||
	        strcmp(fname, "__libc_csu_fini") == 0 ||
	        strcmp(fname, "__libc_csu_init") == 0 ||
	        strcmp(fname, "_start") == 0 ||
	        strcmp(fname, "start") == 0 ||
	        strcmp(fname, "_init_array") == 0 ||
	        strcmp(fname, "_fini_array") == 0 ||
	        strcmp(fname, "__init_array_start") == 0 ||
	        strcmp(fname, "__fini_array_start") == 0 ||
	        strcmp(fname, "__init_array_end") == 0 ||
	        strcmp(fname, "__fini_array_end") == 0 ||
	        strcmp(fname, "main") == 0 )
	{
		return 0;
	}

	segment_t *seg = getseg(f->startEA);

	// Also don't want addresses that are
	// data references to something in XTRN
	ea_t dref = get_first_dref_from(f->startEA);
	if (dref != BADADDR)
	{
		dref = get_first_dref_from(dref);
		segment_t *seg = getseg(dref);
		if (seg != NULL && seg->type == SEG_XTRN)
		{
			return 0;
		}
	}

	log->info("Adding function \"%s\".", fname);

	LMFunction *pFunc = new LMFunction(f
#ifdef VERBOSE
	                                   , pLMLog
#endif
	                                  );
	pFunc->run_analysis();
	funcVec.push_back(pFunc);

	log->info("%s has %d calls.", fname, pFunc->get_num_calls());

#ifdef FA_DEBUG
	pFunc->PrintCalls();
#endif

	// Add vertex to Function Call Graph
	std::string vname = fname;
#ifdef VERBOSE
	msg("Adding %s to FCG\n", vname.c_str());
#endif
	pFCG->AddVertex(vname, f->startEA, f->endEA, pFunc->get_num_nodes(), pFunc->get_num_edges(), pFunc->get_num_calls(), pFunc->get_num_instructions());

	return 0;
}

void LMObject::PrintFuncCalls()
{
	if (funcVec.size() == 0) return;
	//if(pAdjMatrix != NULL)  gsl_matrix_free(pAdjMatrix);


	//pAdjMatrix = gsl_matrix_calloc(funcVec.size(), funcVec.size());

	// For every function, is there a xref to another function?
	int i;
	int funcVecSize = funcVec.size();
	log->info("Printing function calls for %d functions...", funcVecSize);
	LMFunction *src;
	for (i = 0; i < funcVecSize; ++i)
	{
		src = funcVec[i];
#ifdef FA_DEBUG
		src->PrintCalls();
#endif
	}

	//PrintGSLMatrix(pAdjMatrix, "Adjacency Matrix");
}
int LMObject::ComputeAdjacencyMatrix()
{
	if (funcVec.size() == 0) return 0;
	//if(pAdjMatrix != NULL)  gsl_matrix_free(pAdjMatrix);

#ifdef VERBOSE
	msg( "Updating edge weights for %d nodes.\n ", funcVec.size() );
#endif
	//msg("Computing adjacency matrix...\n");
	//pAdjMatrix = gsl_matrix_calloc(funcVec.size(), funcVec.size());

	// For every function, is there a xref to another function?
	int i, j, totalFuncCalls = 0;
	int funcVecSize = funcVec.size();
	LMFunction *src, *dst;
	for (i = 0; i < funcVecSize; ++i)
	{
		src = funcVec[i];
#ifdef FA_DEBUG
		src->PrintCalls();
#endif
		//totalFuncCalls += src->get_num_calls();
		for (j = 0; j < funcVecSize; ++j)
		{
			dst = funcVec[j];
			if (src->HasXrefTo(dst->get_ea_start()) == true)
			{
				log->info("LMObject: xref from %a to %a", src->get_ea_start(), dst->get_ea_start());

				//gsl_matrix_set(pAdjMatrix, i, j, 1);

				// Add edge to Function Call Graph
				std::string srcname, dstname;
				srcname = src->get_name();
				dstname = dst->get_name();
				pFCG->AddEdgeWeight(srcname, dstname,
				                    abs( (int)src->get_ea_start() - (int)dst->get_ea_start()));

				// !!! This is currently unused. !!!
				pFCG->AddEdgeLoop(srcname, dstname, 0);
				totalFuncCalls++;
			}
		}
	}
	msg("Total Number of Function Calls: %d\n", totalFuncCalls);

	//PrintGSLMatrix(pAdjMatrix, "Adjacency Matrix");
}


/*!
Removes double slash in NULL-terminated string.
*/
void LMObject::RemoveDoubleSlash(char *const str)
{
	char *currchar = str;
	char *shiftchar1, *shiftchar2;

	while( *currchar != '\0' )
	{

		if( *currchar == '/' && *(currchar+1) == '/' )
		{
			// Shift all characters up by 1
			shiftchar1 = currchar;
			shiftchar2 = currchar+1;
			while( *shiftchar1 != '\0' )
			{
				*shiftchar1 = *shiftchar2;
				++shiftchar1;
				++shiftchar2;
			}
		}


		++currchar; // Next char
	}
}

// Forms path in client's DOT folder if the input path is of the form
// <path>/libmatcher/mnt/
bool LMObject::FormPathInClientDot(
    char *const outpath,
    const size_t outpath_size,
    char *const inpath)
{
	char pathsha1[41] = {0};
	char path_hint1[1024];
	char path_hint2[1024];
	char *filepath;
	char *shortmntpath;
	const char *nameptr;
	char clientname[20] = {0};
	int index = 0;
	int hint = 0;

	// Checks for hint if we should form the path in the
	// client's dot folder.
  qstring home;
  qgetenv("HOME", &home);
	qsnprintf(path_hint1, sizeof(path_hint1), "%s/libmatcher/mnt/", home.c_str());
	RemoveDoubleSlash(path_hint1);
	qsnprintf(path_hint2, sizeof(path_hint2), "/backup/libmatcher/mnt/");
	RemoveDoubleSlash(path_hint2);
	if ( strstr(inpath, path_hint1) != NULL )
	{
		shortmntpath = &inpath[strlen(path_hint1)];
		hint = 1;
	}
	else if ( strstr(inpath, path_hint2) != NULL )
	{
		shortmntpath = &inpath[strlen(path_hint2)];
		hint = 2;
	}
	else return false;

	filepath = strchr(shortmntpath, '/');
	if ( filepath == NULL ) return false;

	//
	// Extract the client name
	// <path1>/clientname/<path2>
	//

	nameptr = shortmntpath;
	while (nameptr != filepath)
	{
		clientname[index] = *nameptr;
		nameptr++;
		index++;
	}

	log->info("Client name: \"%s\"\n", clientname);

	msg("Computing SHA1 for \"%s\" strlen = %d\n", filepath, strlen(filepath));

	// Compute SHA1 of path
	SHA1 sha;
	sha.Reset();
	sha.Input(filepath, strlen(filepath));
	unsigned res[5];
	unsigned char *value;
	sha.Result(res);
	index = 0;
	for (int i = 0; i < 5; ++i)
	{
		value = (unsigned char*)&res[i];
		for (int j = 0; j < sizeof(unsigned); ++j)
		{
			qsnprintf(&pathsha1[index],
			          sizeof(pathsha1) - index,
			          "%02x",
			          value[sizeof(unsigned) - j - 1]);
			msg("%02x ", value[sizeof(unsigned) - j - 1]);
			index+=2;
		}
		msg("\n");
	}
	msg("Path SHA1: %s\n", pathsha1);


	// Prepare output path
	if (hint == 1) {
    qstring home;
    qgetenv("HOME", &home);
		qsnprintf(outpath, outpath_size,
		          "%s/libmatcher/dot/%s/%s.dot",
		          home.c_str(), clientname, pathsha1);
  }
	else if (hint == 2)
		qsnprintf(outpath, outpath_size,
		          "/backup/libmatcher/dot/%s/%s.dot",
		          clientname, pathsha1);

	return true;
}

// Forms output filename in input path's folder by appending an index
// in the format "filename.n.dot". If the filename is already used,
// increment the index n and try again.
//
// Note that this function always succeeds.
bool LMObject::FormPathInInpathFolder(
    char *const outpath,
    const size_t outpath_size,
    const char *const inpath, bool isInArchive)
{
	unsigned int index = 0;
	size_t size = 0;
	struct stat st;
	char *filename;

	//filename = strrchr(inpath, '/');	// Get last '/'
	//if(filename == NULL)
	//	filename = inpath;
	//else if(filename[0] == '/')
	//{
	//	filename[0] = '\0';		// NULL the '/'
	//	filename++;			// Point to filename
	//}

	do
	{
		++index;

		// Form the new output filename
		//qsnprintf(outpath, outpath_size, "%s/libmatcher/dot/sbj/%s.%d.dot",
		//	getenv("HOME"), filename, index);
		qsnprintf(outpath, outpath_size, "%s.%d.dot", inpath, index);

		st.st_size = 0;
		stat(outpath, &st);

		// Keep trying until file open fails (doesn't exist) or
		// it has 0 file size. This means the filename is not used,
		// or previously generated DOT file is 0 byte, so we can
		// reuse the name.
	} while ( isInArchive == false &&
		open(outpath, O_CREAT+O_EXCL, S_IRUSR|S_IWUSR) == -1 &&
		st.st_size != 0 );

	return true;
}

/*!
Returns the parent folder name.
*/
void LMObject::GetParentFolder(const char* path, char parentname[1024])
{
	const char *currchar = strrchr(path, '/');
	int idx = 0;

	// Move to second last '/'
	while( --currchar && *currchar != '/' && currchar != path);

	// Move to first character of parent folder name
	++currchar;  

	// Copy parent folder name
	while(*currchar != '/' && *currchar != '\0')
	{
		parentname[idx] = *currchar;
		++idx; ++currchar;
	}

	parentname[idx] = '\0'; // NULL-terminate
}

/*!
Checks if the parent folder of the file is an archive folder, i.e. prefixed
with "archive_". If so, rename the input path to use the archive filename.

For e.g., if inpath is "<path>/archive_xyz.a/file.o", it will be converted to
"<path>/xyz.a".

The function returns true if the path is converted, and false otherwise.
*/
bool LMObject::ConvertArchiveInpath(char *inpath, const size_t inpathlen)
{
	char parentname[1024];
	const char *arprefix = "archive_";

	GetParentFolder(inpath, parentname);
	msg("Parent Folder: %s\n", parentname);

	if( strncmp(parentname, arprefix, strlen(arprefix)) != 0 )
		return false;	// No need to convert

	//
	// Convert inpath
	//

	char *replacedst = strstr(inpath, parentname);
	char *replacesrc = &parentname[strlen(arprefix)];

	// Replace each character
	do
	{
		*replacedst = *replacesrc;
		++replacedst; ++replacesrc;
	} while(*replacesrc != '\0');
	*replacedst = '\0';	// NULL-terminate

	return true;
}

/*!
Gets the output filename for the current file.

If the file is from "$(HOME)/libmatcher/mnt/<nodename>".
We remove this prefix, then compute the SHA1, and append
with ".dot". This will be used as the output filename.

Otherwise, we remove the file extension, if any, and
append an index, followed by ".dot". The index is
incremented if a file using the same index exist.
*/
bool LMObject::GetDotOutputFilename(
    char *const outpath,
    const size_t outpath_size)
{
	char inpath[1024];
	bool convertedInpath;

	// Prepare path prefix
	get_input_file_path(inpath, sizeof(inpath));

	convertedInpath = ConvertArchiveInpath(inpath, sizeof(inpath));
	msg("New inpath: %s\n", inpath);
	log->info("Getting DOT filename for \"%s\"...", inpath);

	if ( FormPathInClientDot(outpath, outpath_size, inpath) == false )
	{
		FormPathInInpathFolder(outpath, outpath_size, inpath, convertedInpath);
	}

	// Remove ugly double slashes in outpath
	RemoveDoubleSlash(outpath);

	return convertedInpath;
}

bool LMObject::FileExists(string strFilename) {
	struct stat stFileInfo;
	bool blnReturn;
	int intStat;

	// Attempt to get the file attributes
	intStat = stat(strFilename.c_str(),&stFileInfo);
	if (intStat == 0) {
		// We were able to get the file attributes
		// so the file obviously exists.
		blnReturn = true;
	} else {
		// We were not able to get the file attributes.
		// This may mean that we don't have permission to
		// access the folder which contains this file. If you
		// need to do that level of checking, lookup the
		// return values of stat which will give you
		// more details on why stat failed.
		blnReturn = false;
	}

	return(blnReturn);
}

void LMObject::WriteFCG()
{

	log->info("Writing DOT file to \"%s\"...", graphPath);

	// If DOT file already exist, delete it.
	//if (FileExists(string(graphPath)) == true)
	//	remove(graphPath);


#ifdef VERBOSE
	msg("Writing DOT file to \"%s\"...\n", graphPath);
#endif
	pFCG->WriteDOT(graphPath);
	chmod(graphPath, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

#ifdef VERBOSE
	msg("%d nodes %d edges\n", num_vertices(*pFCG), num_edges(*pFCG));
#endif
}
