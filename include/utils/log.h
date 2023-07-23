#ifndef LOG_H
#define LOG_H

#include <AlfieLoader.h>
#include <time.h>
#include <stdarg.h>
#include <stddef.h>
#include <pthread.h>
#include <assert.h>
#include <utils/ANSI-color-codes.h>

typedef enum
{
	LOG_ERROR = 1,
	LOG_WARNING = 2,
	LOG_INFO = 3,
	LOG_SUCCESS = 4,
	LOG_DEBUG = 5
} log_level_t;

extern arg_t args[];

// ******************************************************
// Function: step()
//
// Purpose: Print a message to stdout that has a countdown
//
// Parameters:
//      int time: the number of seconds to count down from
//      char *text: the message to print
// ******************************************************
void step(int time, bool endWithNewline, char *text);

// ******************************************************
// Function: AlfieLoaderLog()
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
int AlfieLoaderLog(log_level_t loglevel, bool newline, const char *fname, int lineno, const char *fxname, const char *__restrict format, ...);

#endif // LOG_H