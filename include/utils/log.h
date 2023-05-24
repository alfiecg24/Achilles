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

int loaderLog(log_level_t loglevel, bool newline, const char *fname, int lineno, const char *fxname, const char *__restrict format, ...);

#endif // LOG_H