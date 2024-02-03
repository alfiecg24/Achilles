#ifndef ACHILLES_H
#define ACHILLES_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <utils/log.h>

#define LOG(logLevel, ...) AchillesLog(logLevel, true, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_NO_NEWLINE(logLevel, ...) AchillesLog(logLevel, false, __FILE__, __LINE__, __func__, __VA_ARGS__)

struct AchillesArgs {
    char *deviceUDID;
    bool debug;
    bool verbose;
    bool quiet;
    bool disableSignatureChecks;
    bool bootToPongo;
    bool jailbreak;
    bool verboseBoot;
    bool serialOutput;
    bool pongoterm;
    char *bootArgs;
    char *pongoPath;
    char *kpfPath;
    char *ramdiskPath;
    char *overlayPath;
};

#endif // ACHILLES_H