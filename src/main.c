#include <AlfieLoader.h>
#include <string.h>
#include <exploit/exploit.h>
#include <exploit/payloads/helpers.h>

bool isARM64Host = false;

arg_t args[] = {
    // Name, short option, long option, description, examples, type, value
    {"Verbosity", "-v", "--verbosity", "Verbosity level, maximum of 2", "-vv, --verbosity 2", FLAG_INT, 0},
    {"Debug", "-d", "--debug", "Enable debug logging", NULL, FLAG_BOOL, false},
    {"Help", "-h", "--help", "Show this help message", NULL, FLAG_BOOL, false},
    {"Version", "-V", "--version", "Show version information", NULL, FLAG_BOOL, false}};

arg_t *getArgByName(char *name)
{
    // Get an argument by its name
    for (int i = 0; i < sizeof(args) / sizeof(arg_t); i++)
    {
        if (strcmp(args[i].name, name) == 0)
        {
            return &args[i];
        }
    }
    return (arg_t *){NULL};
}

void printHelp()
{
    // Print help information
    printf("Usage: %s [options]\n", NAME);
    printf("Options:\n");
    for (int i = 0; i < sizeof(args) / sizeof(arg_t); i++)
    {
        if (args[i].examples != NULL)
        {
            printf("\t%s, %s: %s (e.g. %s)\n", args[i].shortOpt, args[i].longOpt, args[i].description, args[i].examples);
        }
        else
        {
            printf("\t%s, %s: %s\n", args[i].shortOpt, args[i].longOpt, args[i].description);
        }
    }
}

void printVersion()
{
    // Print version information
    printf("%s %s - %s\n", NAME, VERSION, RELEASE_TYPE);
}

int main(int argc, char *argv[])
{
    bool hasUsedUnrecognisedArg = false;
    bool matchFound = false;
    // Iterate over each defined argument
    for (int i = 0; i < sizeof(args) / sizeof(arg_t); i++) {
        // Iterate over each argument passed from CLI
        for (int v = 0; v < argc; v++) {
            if ((strncmp(argv[v], "-", 1) == 0) && // Prevent the file being executing from being flagged
            (strcmp(argv[v], args[i].longOpt) != 0) && // Check if it matches the long argument option
            (strncmp(argv[v], args[i].shortOpt, 2) != 0)) { // Check if it matches the short argument option
                hasUsedUnrecognisedArg = true;
                continue;
            } else {
                matchFound = true;
                break;
            }
        }
    }

    if (hasUsedUnrecognisedArg) {
        // Print the help menu
        printHelp();
        return 0;
    }

    for (int i = 0; i < sizeof(args) / sizeof(arg_t); i++)
    {
        if (args[i].type == FLAG_BOOL)
        { // Check if the argument is a boolean
            for (int j = 0; j < argc; j++)
            {
                if (strcmp(argv[j], args[i].shortOpt) == 0 || strcmp(argv[j], args[i].longOpt) == 0)
                {
                    // Set the boolean value to true if the argument is found
                    args[i].boolVal = true;
                }
            }
        }
        else if (args[i].type == FLAG_INT)
        { // Check if the argument is an integer
            for (int j = 0; j < argc; j++)
            {
                if (strncmp(argv[j], args[i].shortOpt, 2) == 0)
                { // Check if the argument is a short option
                    int argLen = strlen(argv[j]);
                    for (int k = 1; k < argLen; k++)
                    { // Loop through the short option (e.g. -vvv)
                        if (argv[j][k] == args[i].shortOpt[1])
                        {
                            // Increment the integer value for each time the short option is repeated
                            args[i].intVal++;
                        }
                    }
                }
                else if (strcmp(argv[j], args[i].longOpt) == 0)
                {
                    if (j + 1 < argc)
                    {
                        // Set the value of the integer to the next argument
                        args[i].intVal = atoi(argv[j + 1]);
                    }
                }
            }
        }
    }

    arg_t *verbosityArg = getArgByName("Verbosity");
    if (verbosityArg->intVal > 2)
    {
        LOG(LOG_DEBUG, "Verbosity set to %d, max is 2 - automatically updating to 2", verbosityArg->intVal);
        verbosityArg->intVal = 3;
    }
    
    // TODO: make this char *argList to prevent possible overflowing?
    char argList[1024];
    memset(argList, 0, sizeof(argList));
    for (int i = 0; i < sizeof(args) / sizeof(arg_t); i++)
    {
        if (args[i].type == FLAG_BOOL)
        {
            sprintf(argList, "%s%s: %s, ", argList, args[i].name, args[i].boolVal ? "true" : "false");
        }
        else if (args[i].type == FLAG_INT)
        {
            sprintf(argList, "%s%s: %d, ", argList, args[i].name, args[i].intVal);
        }
    }
    // Remove the trailing comma
    argList[strlen(argList) - 2] = '\0';

    LOG(LOG_DEBUG, "Arguments - %s", argList);

    if (getArgByName("Help")->boolVal)
    {
        printHelp();
        return 0;
    }

    if (getArgByName("Version")->boolVal)
    {
        printVersion();
        return 0;
    }

    struct utsname buffer;
    uname(&buffer);
    LOG(LOG_DEBUG, "Host OS: %s", buffer.sysname);
    LOG(LOG_DEBUG, "Host architecture: %s", buffer.machine);
    if (strcmp(buffer.sysname, "Darwin") != 0)
    {
        LOG(LOG_FATAL, "ERROR: This tool is only supported on macOS");
        return -1;
    }
    if (strcmp(buffer.machine, "arm64") == 0)
    {
        isARM64Host = true;
    }

    char **devices;
    int deviceCount;
    int i = 0;
    device_t device;
    if (findDevice(&device, false) == -1) {
        while (i < 5)
        {
            idevice_error_t ideviceError = idevice_get_device_list(&devices, &deviceCount);
            if (ideviceError != IDEVICE_E_SUCCESS)
            {
                LOG(LOG_ERROR, "Failed to get device list: %s", ideviceError);
                return 1;
            }
            if (deviceCount > 1)
            {
                LOG(LOG_FATAL, "ERROR: At the moment, only one device at a time is supported. Please disconnect any additional iOS devices and try again.");
                return -1;
            }
            if (deviceCount == 0)
            {
                sleep(1);
                i++;
            }
            else
            {
                break;
            }
        }
        if (deviceCount == 0)
        {
            LOG(LOG_FATAL, "ERROR: No iOS device found after 5 seconds - please connect a device.");
            return -1;
        }
    }

    if (isSerialNumberPwned(getDeviceSerialNumber(&device.handle)))
    {
        LOG(LOG_DEBUG, "Serial number: %s", getDeviceSerialNumber(&device.handle));
        LOG(LOG_FATAL, "ERROR: This device is already pwned.");
        return -1;
    }

    checkm8();

    // LOG(LOG_DEBUG, "0x%2X", ((3U << 8U) | device_descriptor.i_serial_number));


    // uint8_t branch[16];
    // asm_arm64_x7_trampoline(0x10000E08C, branch);

    // def usb_req_leak(device): libusb1_no_error_ctrl_transfer(device, 0x80, 6, 0x304, 0x40A, 0x40, 1)

    // reverseControlRequest(0x80);
    // LOG(LOG_DEBUG, "0x%2X", (0x80 & 0x80));

    return 0;
}
