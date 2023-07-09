#ifndef LOG_H
#define LOG_H

#include <AlfieLoader.h>
#include <time.h>
#include <stdarg.h>
#include <stddef.h>
#include <pthread.h>
#include <assert.h>

typedef enum
{
	LOG_FATAL = 0,
	LOG_ERROR = 1,
	LOG_WARNING = 2,
	LOG_INFO = 3,
	LOG_SUCCESS = 4,
	LOG_DEBUG = 5
} log_level_t;

extern arg_t args[];

// ******************************************************
// Function: loaderLog()
//
// Purpose: Log a message to stdout formatted with ANSI colour codes
//
// Parameters:
//      log_level_t loglevel: the log level of the message
//      bool newline: whether or not to print a newline
//      const char *fname: the name of the file the message is from
//      int lineno: the line number the message is from
//      const char *fxname: the name of the function the message is from
//      const char *__restrict format: the format string of the message
//      ...: the arguments to the format string
//
// mostly taken from https://github.com/palera1n/palera1n-c - thanks!
// ******************************************************
int loaderLog(log_level_t loglevel, bool newline, const char *fname, int lineno, const char *fxname, const char *__restrict format, ...);

#endif // LOG_H