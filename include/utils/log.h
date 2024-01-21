#ifndef LOG_H
#define LOG_H

#include <assert.h>
#include <stdarg.h>
#include <pthread.h>
#include <unistd.h>

#include <Achilles.h>
#include <utils/ANSI-color-codes.h>

typedef enum
{
	LOG_ERROR = 1,
	LOG_INFO = 2,
	LOG_SUCCESS = 3,
	LOG_VERBOSE = 4
} log_level_t;

void step(int time, int time2, char *text);

int AchillesLog(log_level_t loglevel, bool newline, const char *fname, int lineno, const char *fxname, const char *__restrict format, ...);

#endif // LOG_H