/*!
\file	LMException.hpp

Exception class for Libmatcher.
*/

#ifndef __LMEXCEPTION_H__
#define __LMEXCEPTION_H__

#include <string>
#include <exception>

class LMException: public std::exception
{
public:
	LMException(const char *msg) {message.assign(msg); };
	~LMException() throw() {};

	inline std::string getMessage() { return message; };
private:
	std::string message;
};

#endif
