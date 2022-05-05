#ifndef _UTIL_H_
#define _UTIL_H_

#include "main.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIN32
#include <unistd.h>
#endif
#ifdef WIN32
#define stat _stat
#endif

constexpr inline void _checkp(const char*, const int, const void*);
#define checkp(p) _checkp(__FILE__, __LINE__, static_cast<void*>((p)))

constexpr inline void _check (const char*, const int, const int);
#define check(e) _check(__FILE__, __LINE__, (e))

uint64_t get_last_modified_time(const char*);
char* slurp_file(const char*, size_t*);

#endif // _UTIL_H_
