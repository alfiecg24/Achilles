#include "log/log.h"

// TODO: Integrate proper unit testing into source code

int main(void) {
    LOG(LOG_INFO, "Running Achilles tests");

    if (access("binaries", F_OK) == -1 || access("binaries/Achilles-IOKit", F_OK) == -1 || access("binaries/Achilles-libusb", F_OK) == -1) {
        LOG(LOG_ERROR, "Please ensure you have Achilles-IOKit and Achilles-libusb in the binaries/ directory!");
        return 1;
    }

    LOG(LOG_INFO, "Testing regular exploit (IOKit)");
    LOG_NO_NEWLINE(LOG_INFO, "Press enter to continue...");
    getchar();
    system("binaries/Achilles-IOKit -evq");

    LOG(LOG_INFO, "Testing regular exploit (libusb)");
    LOG_NO_NEWLINE(LOG_INFO, "Press enter to continue...");
    getchar();
    system("binaries/Achilles-libusb -evq");

    LOG(LOG_INFO, "Testing PongoOS boot (IOKit)");
    LOG_NO_NEWLINE(LOG_INFO, "Press enter to continue...");
    getchar();
    system("binaries/Achilles-IOKit -pvq");

    LOG(LOG_INFO, "Testing PongoOS boot (libusb)");
    LOG_NO_NEWLINE(LOG_INFO, "Press enter to continue...");
    getchar();
    system("binaries/Achilles-libusb -pvq");

    LOG(LOG_INFO, "Testing jailbreak with PongoOS");
    LOG_NO_NEWLINE(LOG_INFO, "Press enter to continue...");
    getchar();
    system("binaries/Achilles-IOKit -jvq");

    return 0;
}