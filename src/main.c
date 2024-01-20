#include <Achilles.h>
#include <exploit/exploit.h>

char *get_argument_value(int argc, char *argv[], const char *flag)
{
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], flag)) {
            if (i+1 < argc) {
                return argv[i+1];
            }
        }
    }
    return NULL;
}

bool argument_exists(int argc, char *argv[], const char *flag)
{
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], flag)) {
            if (!strcmp(flag, "-v") && (i - 1 > 0) && !strcmp(argv[i - 1], "-b")) { // -v is a valid boot argument
                return false;
            }
            return true;
        }
    }
    return false;
}

void print_usage(char *executablePath) {
    printf("Options:\n");
    printf("\t-d - enable debug logging\n");
    printf("\t-v - enable verbose logging\n");
    printf("\t-q - enable quiet logging (removes all logging except for errors)\n");
    printf("\t-s - remove signature checks\n");
    printf("\t-p - boot to PongoOS and exit\n");
    printf("\t-j - jailbreak the device (requires -R and -O)\n");
    printf("\t-V - enable verbose boot\n");
    printf("\t-S - enable serial output\n\n");
    printf("\t-b - additional boot arguments\n");
    printf("\t-k <Pongo.bin> - override PongoOS image\n");
    printf("\t-K <kpf> - override kernel patchfinder\n");
    printf("\t-R <ramdisk.dmg> - ramdisk to boot\n");
    printf("\t-O <overlay.dmg> - overlay to boot\n\n");
    printf("\t-h - print this help message\n");
    printf("Examples:\n");
    printf("\t%s -p\n", executablePath);
    printf("\t%s -j -R ramdisk.dmg -O overlay.dmg\n", executablePath);
    printf("\t%s -s\n", executablePath);
    exit(-1);
}

bool check_for_argument_conflicts(struct AchillesArgs args, char *argv0) {
    if (args.jailbreak && !args.kpfPath) {
        LOG(LOG_ERROR, "-j requires -K.");
        print_usage(argv0);
        return false;
    }

    if (args.bootToPongo && args.jailbreak) {
        LOG(LOG_ERROR, "-p and -j are mutually exclusive.");
        print_usage(argv0);
        return false;
    }

    if (!args.disableSignatureChecks && !args.bootToPongo && !args.jailbreak) {
        LOG(LOG_ERROR, "You must specify either -s, -p, or -j.");
        print_usage(argv0);
        return false;
    }

    if (args.disableSignatureChecks && (args.bootToPongo || args.jailbreak)) {
        LOG(LOG_ERROR, "-s is incompatible with -p and -j.");
        print_usage(argv0);
        return false;
    }
    return true;
}

bool check_custom_paths(struct AchillesArgs args) {
    struct stat st;
    if (args.pongoPath) {
        if (stat(args.pongoPath, &st) != 0) {
            LOG(LOG_ERROR, "PongoOS image at %s does not exist.", args.pongoPath);
            return false;
        }
    }

    if (args.kpfPath) {
        if (stat(args.kpfPath, &st) != 0) {
            LOG(LOG_ERROR, "kernel patchfinder at %s does not exist", args.kpfPath);
            return false;
        }
    }

    if (args.ramdiskPath) {
        if (stat(args.ramdiskPath, &st) != 0) {
            LOG(LOG_ERROR, "ramdisk at %s does not exist.", args.ramdiskPath);
            return false;
        }
    }

    if (args.overlayPath) {
        if (stat(args.overlayPath, &st) != 0) {
            LOG(LOG_ERROR, "overlay at %s does not exist.", args.overlayPath);
            return false;
        }
    }
    return true;
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        print_usage(argv[0]);
    }
    
    args.debug = argument_exists(argc, argv, "-d");
    args.verbose = argument_exists(argc, argv, "-v");
    args.quiet = argument_exists(argc, argv, "-q");
    args.disableSignatureChecks = argument_exists(argc, argv, "-s");
    args.bootToPongo = argument_exists(argc, argv, "-p");
    args.jailbreak = argument_exists(argc, argv, "-j");
    args.verboseBoot = argument_exists(argc, argv, "-V");
    args.serialOutput = argument_exists(argc, argv, "-S");
    args.bootArgs = get_argument_value(argc, argv, "-b");

    args.pongoPath = get_argument_value(argc, argv, "-k");
    args.kpfPath = get_argument_value(argc, argv, "-K");
    args.ramdiskPath = get_argument_value(argc, argv, "-R");
    args.overlayPath = get_argument_value(argc, argv, "-O");

    if (argument_exists(argc, argv, "-h")) {
        print_usage(argv[0]);
    }

    if (!check_for_argument_conflicts(args, argv[0])) {
        return -1;
    }
    
    if (!check_custom_paths(args)) {
        return -1;
    }

    LOG(LOG_SUCCESS, "Achilles v2 we out here!!");

    usb_handle_t handle;
    init_usb_handle(&handle, 0x5AC, 0x1227);
    checkm8(&handle, (args.bootToPongo || args.jailbreak) ? MODE_PONGOOS : MODE_CHECKM8);

    return 0;
}