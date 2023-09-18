#include "log.h"

void step(int time, bool endWithNewline, char *text) {
	for (int i = 0; i <= time; i++) {
		printf(BCYN "\r\033[K%s (%d)" CRESET, text, time - i);
		fflush(stdout);
		sleep(1);
	}
	if (endWithNewline) {
		printf(CYN "\r%s (%d)\n" CRESET, text, 0);
	} else {
		printf(CYN "\r%s (%d)" CRESET, text, 0);
	}
}

int AchillesLog(log_level_t loglevel, bool newline, const char *fname, int lineno, const char *fxname, const char *__restrict format, ...)
{
	int ret = 0;
	pthread_mutex_t log_mutex;
	pthread_mutex_init(&log_mutex, NULL);
	char type[0x10];
	char colour[0x10];
	char colour_bold[0x10];
	va_list logArgs;
	va_start(logArgs, format);
	switch (loglevel)
	{
	case LOG_ERROR:
		snprintf(type, 0x10, "%s", "Error");
		snprintf(colour, 0x10, "%s", RED);
		snprintf(colour_bold, 0x10, "%s", BRED);
		break;
	case LOG_WARNING:
		snprintf(type, 0x10, "%s", "Warning");
		snprintf(colour, 0x10, "%s", YEL);
		snprintf(colour_bold, 0x10, "%s", BYEL);
		break;
	case LOG_INFO:
		snprintf(type, 0x10, "%s", "Info");
		snprintf(colour, 0x10, "%s", CYN);
		snprintf(colour_bold, 0x10, "%s", BCYN);
		break;
	case LOG_SUCCESS:
		snprintf(type, 0x10, "%s", "Success");
		snprintf(colour, 0x10, "%s", GRN);
		snprintf(colour_bold, 0x10, "%s", BGRN);
		break;
	}
	{
		pthread_mutex_lock(&log_mutex);
		char timestring[0x80];
		time_t curtime;
		time(&curtime);
		struct tm *timeinfo = localtime(&curtime);
		snprintf(timestring, 0x80, "%s[%s%02d/%02d/%d %02d:%02d:%02d%s]", CRESET, HBLK, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_year - 100, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, CRESET);
		printf("%s%s%s <%s>" CRESET ":%s ", colour_bold, timestring, colour_bold, type, colour_bold);
		printf("%s", colour);
		ret = vprintf(format, logArgs);
		va_end(logArgs);
		if (newline)
		{
			printf(CRESET "\n");
		} else {
			printf(CRESET);
		}
		fflush(stdout);
	}
	pthread_mutex_unlock(&log_mutex);
	return ret;
}
