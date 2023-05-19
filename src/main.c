#include <AlfieLoader.h>
#include <string.h>
#include <exploit/exploit.h>

arg_t args[] = {
    {"Verbosity", "-v", "--verbosity", "Set verbosity level", "-vvv, --verbosity 3", FLAG_INT, 0},
    {"Debug", "-d", "--debug", "Enable debug mode", NULL, FLAG_BOOL, false},
    {"Help", "-h", "--help", "Show this help message", NULL, FLAG_BOOL, false},
    {"Version", "-V", "--version", "Show version information", NULL, FLAG_BOOL, false}
};

void printHelp() {
    printf("Usage: %s [options]\n", NAME);
    printf("Options:\n");
    for (int i = 0; i < sizeof(args) / sizeof(arg_t); i++) {
        if (args[i].examples != NULL) {
             printf("\t%s, %s: %s (e.g. %s)\n", args[i].shortOpt, args[i].longOpt, args[i].description, args[i].examples);
        } else {
            printf("\t%s, %s: %s\n", args[i].shortOpt, args[i].longOpt, args[i].description);
        }
    }
}

void printVersion() {
    printf("%s %s - %s\n", NAME, VERSION, RELEASE_TYPE);
}

int main(int argc, char *argv[]) {
    // Parse args
    for (int i = 0; i < sizeof(args) / sizeof(arg_t); i++) {
        if (args[i].type == FLAG_BOOL) {
            for (int j = 0; j < argc; j++) {
                if (strcmp(argv[j], args[i].shortOpt) == 0 || strcmp(argv[j], args[i].longOpt) == 0) {
                    args[i].boolVal = true;
                }
            }
        } else if (args[i].type == FLAG_INT) {
            for (int j = 0; j < argc; j++) {
                if (strncmp(argv[j], args[i].shortOpt, 2) == 0) {
                    int argLen = strlen(argv[j]);
                    for (int k = 1; k < argLen; k++) {
                        if (argv[j][k] == args[i].shortOpt[1]) {
                            args[i].intVal++;
                        }
                    }
                } else if (strcmp(argv[j], args[i].longOpt) == 0) {
                    if (j + 1 < argc) {
                        args[i].intVal = atoi(argv[j + 1]);
                    }
                }
            }
        }
    }

    for (int i = 0; i < sizeof(args) / sizeof(arg_t); i++) {
        if (args[i].type == FLAG_BOOL) {
            LOG(LOG_DEBUG, "%s: %s", args[i].name, args[i].boolVal ? "true" : "false");
        } else if (args[i].type == FLAG_INT) {
            if (args[0].intVal == 0) {
                LOG(LOG_DEBUG, "Verbosity not set, defaulting to 1");
                args[i].intVal = 1;
            } else if (args[0].intVal > 3) {
                LOG(LOG_DEBUG, "Verbosity set to %d, max is 3", args[0].intVal);
                args[i].intVal = 3;
            }
            LOG(LOG_DEBUG, "%s: %d", args[i].name, args[i].intVal);
        }
    }

    if (args[2].boolVal) {
        printHelp();
        return 0;
    }

    if (args[3].boolVal) {
        printVersion();
        return 0;
    }

    exploit();

    return 0;
}