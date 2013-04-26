#ifndef __REXIO_HEADER__
#define __REXIO_HEADER__

#include <libvex.h>
#include <list>
#include <vector>
#include <stdint.h>
#include <log4cpp/Category.hh>

using namespace std;

typedef enum{ DUMMY, PARAM, IN, PUT, ST_TMP, ST_CONST, RETN, PATHCOND } IOTAG;

typedef struct
{
	unsigned char tag;

	union
	{
		struct
		{
			int		ebpOffset;
		} param;

		struct
		{
			unsigned int	fid;
			unsigned int	bid;
			IRTemp		tmp;
		} in;

		struct
		{
			int offset;
		} put;

		struct
		{
			unsigned int	curMemSt;
			unsigned int	fid;
			unsigned int	bid;
			IRTemp		tmp;
		} store_tmp;

		struct
		{
			unsigned int	curMemSt;
			uint32_t	con;
			unsigned int	arr_rest;
		} store_const;

		struct
		{
			unsigned int	fid;
			unsigned int	bid;
			IRTemp		tmp;
		} retn;

		struct
		{
			unsigned int	fid;
			unsigned int	bid;
			IRTemp		tmp;
			bool		taken;
		} pathcond;
	} io;
} IO;

class RexIO
{
public:
	RexIO( log4cpp::Category *_log );
	~RexIO();

	void		Clear();
	void		AddIO( IO* io );
	void		TransferIO( IO* io, RexIO &dst );
	bool		IsLastIO()	{ return ioitr == iolist.end();	}
	size_t		GetNumIO()	{ return iolist.size();		}

	IO*		GetLastIO();
	IO*		GetFirstIO();
	IO*		GetNextIO();
	IO*		GetIOByIdx( const unsigned int idx );
	list<IO*>*	GetList()	{ return &iolist;		}

	void		ZapDummies();
	void		PadDummies( const unsigned int toSize );
	void		Print();

	int 		FindLastNonDummy( list<IO*> *lst );
	void		Permute();
	list<IO*>*	GetFirstPermutation();
	list<IO*>*	GetNextPermutation();
	size_t		GetNumPermutation()			{ return permutations.size(); }
	void		PrintPermutation();
	void		RemoveSubsetPmtns();
	void		PrintIOList( list<IO*>* lst );

protected:
	list< IO* >			iolist;		/// list of all io's

private:
	log4cpp::Category		*log;
	list< IO* >::iterator		ioitr;		/// io iterator
	vector< list<IO*>* >		permutations;	/// permutations
	vector< list<IO*>* >::iterator	pmtnitr;	/// permutation iterator

	bool		IsSameIO( IO *io1, IO *io2 );	
	bool		IsSubsetHelper( list<IO*> *list1, list<IO*>::iterator itr1, list<IO*> *list2 );
	bool		IsSubset( list<IO*> *list1, list<IO*> *list2 );
	void		PermuteHelper( vector< list<IO*>* > &pmtn, const size_t pmtnSize, list<IO*>::iterator weaveitr );
};

class RexOut : public RexIO
{
public:
	RexOut( log4cpp::Category *_log ) : RexIO( _log ) {}
	void AddOut_PutOffset( const int putos );
	void AddOut_StoreTmp(
		const unsigned int curMemSt,
		const unsigned int fid,
		const unsigned int bid,
		const IRTemp tmp );
	void AddOut_StoreConst(
		const unsigned int curMemSt,
		const unsigned int c,
		const unsigned int arr_rest );
	void AddOut_Retn(
		const unsigned int fid,
		const unsigned int bid,
		const IRTemp tmp );

	IO* FindRetn( const unsigned int fid, const unsigned int bid, const IRTemp tmp );

private:
	log4cpp::Category *log;

	IO* FindPutOffset( const int putos );
	IO* FindStoreTmp( const unsigned int curMemSt,
		const unsigned int fid,
		const unsigned int bid,
		const IRTemp tmp );
	IO* FindStoreConst( const unsigned int curmemSt, const unsigned int c );
};

class RexIn : public RexIO
{
public:
	RexIn( log4cpp::Category *_log ) : RexIO( _log ) {}
	void AddIn( const unsigned int fid, const unsigned int bid, const IRTemp tmp );
	IO* FindIn( const unsigned int fid, const unsigned int bid, const IRTemp tmp );

private:

};

class RexParam : public RexIO
{
public:
	RexParam( log4cpp::Category *_log ) : RexIO( _log ) {}
	void AddParam( const int ebpOffset );
	void AddParamOrdered( const int ebpOffset );
	IO* FindParam( const int ebpOffset );

private:

};

class RexPathCond : public RexIO
{
public:
	RexPathCond( log4cpp::Category *_log ) : RexIO( _log ) {}
	void AddPC( const unsigned int fid, const unsigned int bid, const IRTemp tmp, const bool taken );
	IO* FindPC( const unsigned int fid, const unsigned int bid, const IRTemp tmp, const bool taken );

private:

};


#endif
