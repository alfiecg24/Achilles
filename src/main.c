#include <AlfieLoader.h>
#include <string.h>
#include <exploit/exploit.h>

arg_t args[] = {
    // Name, short option, long option, description, examples, type, value
    {"Verbosity", "-v", "--verbosity", "Set verbosity level", "-vvv, --verbosity 3", FLAG_INT, 0},
    {"Debug", "-d", "--debug", "Enable debug mode", NULL, FLAG_BOOL, false},
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
    for (int i = 0; i < sizeof(args) / sizeof(arg_t); i++)
    {
        // Check if the argument is a boolean
        if (args[i].type == FLAG_BOOL)
        {
            for (int j = 0; j < argc; j++)
            {
                if (strcmp(argv[j], args[i].shortOpt) == 0 || strcmp(argv[j], args[i].longOpt) == 0)
                {
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
                            args[i].intVal++;
                        }
                    }
                }
                else if (strcmp(argv[j], args[i].longOpt) == 0)
                {
                    if (j + 1 < argc)
                    {
                        args[i].intVal = atoi(argv[j + 1]); // Set the value of the integer to the next argument
                    }
                }
            }
        }
    }

    arg_t *verbosityArg = getArgByName("Verbosity");
    if (verbosityArg->intVal == 0)
    {
        LOG(LOG_DEBUG, "Verbosity not set, defaulting to 1");
        verbosityArg->intVal = 1;
    }
    else if (verbosityArg->intVal > 3)
    {
        LOG(LOG_DEBUG, "Verbosity set to %d, max is 3", verbosityArg->intVal);
        verbosityArg->intVal = 3;
    }

    for (int i = 0; i < sizeof(args) / sizeof(arg_t); i++)
    {
        if (args[i].type == FLAG_BOOL)
        {
            LOG(LOG_DEBUG, "%s: %s", args[i].name, args[i].boolVal ? "true" : "false");
        }
        else if (args[i].type == FLAG_INT)
        {
            LOG(LOG_DEBUG, "%s: %d", args[i].name, args[i].intVal);
        }
    }

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

    exploit();

    return 0;
}