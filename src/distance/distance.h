#ifndef __DISTANCE_HEADER__
#define __DISTANCE_HEADER__

#define PRNTSQLERR( conn )	\
	cout << __FILE__ << " Line " << __LINE__ << ": " << mysql_error(conn) << endl;

#endif
