#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <exploit/exploit.h>

int main(int argc, char *argv[]) {
    int verbosity = 1;
    if (argc > 1 && strncmp(argv[1], "-v", 2) == 0) {
        int argLen = strlen(argv[1]);
        verbosity = argLen - 1;
        if (verbosity > 3) {
            verbosity = 3;
        }
        LOG(LOG_DEBUG, "Verbosity set to %d", verbosity);
    }


    if (argc > 1 && strcmp(argv[1], "--verbosity") == 0) {
        if (argc > 2) {
            // cast verbosity to int
            verbosity = atoi(argv[2]);
        }
    }
    
    exploit(verbosity);

    return 0;
}