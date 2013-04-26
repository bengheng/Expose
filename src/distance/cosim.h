#ifndef __COSIM_H__
#define __COSIM_H__

#include <log4cpp/Category.hh>
#include <mysql/mysql.h>
#include <map>

using namespace std;

typedef unsigned int NGRAMID;
typedef unsigned int FREQ;


int GetCosim(
	map<NGRAMID, FREQ> &func1_freqs,
	map<NGRAMID, FREQ> &func2_freqs,
	float *cosim,
	log4cpp::Category &log);

#endif
