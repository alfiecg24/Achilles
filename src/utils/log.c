#include <utils/log.h>

extern bool dfu_device_found;
extern struct AchillesArgs args;

void step(int time, int time2, char *text) {
	// TODO: make this use LOG
    for (int i = time2; i < time; i++) {
		if (dfu_device_found) { return; }
		printf(BCYN "\r\033[K%s (%d)" CRESET, text, time - i + time2);
        fflush(stdout);
        sleep(1);
    }
    printf(CYN "\r%s (%d)" CRESET, text, time2);
	if (time2 == 0) puts("");
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
	switch (loglevel) {
	case LOG_ERROR:
		snprintf(type, 0x10, "%s", "Error");
		snprintf(colour, 0x10, "%s", RED);
		snprintf(colour_bold, 0x10, "%s", BRED);
		break;
	case LOG_INFO:
		snprintf(type, 0x10, "%s", "Info");
		snprintf(colour, 0x10, "%s", CYN);
		snprintf(colour_bold, 0x10, "%s", BCYN);
		if (args.quiet) { goto out; }
		break;
	case LOG_SUCCESS:
		snprintf(type, 0x10, "%s", "Success");
		snprintf(colour, 0x10, "%s", GRN);
		snprintf(colour_bold, 0x10, "%s", BGRN);
		if (args.quiet) { goto out; }
		break;
	default: // LOG_VERBOSE
		if (!args.verbose) {
			return 0;
		}
		assert(loglevel >= 0);
		snprintf(type, 0x10, "%s", "Verbose");
		snprintf(colour, 0x10, "%s", WHT);
		snprintf(colour_bold, 0x10, "%s", BWHT);
		break;
	}
	{
		pthread_mutex_lock(&log_mutex);
		char timestring[0x80];
		time_t curtime;
		time(&curtime);
		struct tm *timeinfo = localtime(&curtime);
		snprintf(timestring, 0x80, "%s[%s%02d/%02d/%d %02d:%02d:%02d%s]", CRESET, HBLK, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_year - 100, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, CRESET);
		if (args.verbose && args.debug) {
			printf("%s%s%s <%s> " CRESET "%s" HBLU "%s" CRESET ":" RED "%d" CRESET ":" BGRN "%s()" CRESET ":%s ", colour_bold, timestring, colour_bold, type, WHT, fname, lineno, fxname, colour_bold);
		}
		else if (args.verbose) {
			printf("%s%s%s <%s>" CRESET ":%s ", colour_bold, timestring, colour_bold, type, colour_bold);
		}
		else {
			printf("%s<%s>%s: ", colour_bold, type, CRESET);
		}
		printf("%s", colour);
		ret = vprintf(format, logArgs);
		va_end(logArgs);
		if (newline) {
			printf(CRESET "\n");
		} else {
			printf(CRESET);
		}
		fflush(stdout);
	}
out:
	pthread_mutex_unlock(&log_mutex);
	return ret;
}
