#include <Achilles.h>
#include <utils/log.h>
#include <usb/device.h>
#include <exploit/exploit.h>

int tests(void) {

    arg_t *verbosityArg = getArgumentByName("Verbosity");
    verbosityArg->set = true;
    verbosityArg->intVal = 1;

    #ifdef ACHILLES_LIBUSB
    LOG(LOG_INFO, "Running Achilles tests for libusb");
    #else
    LOG(LOG_INFO, "Running Achilles tests for IOKit");
    #endif

    LOG(LOG_INFO, "Test 1: regular exploit");
    arg_t *exploitArg = getArgumentByName("Exploit");
    exploitArg->set = true;
    exploitArg->boolVal = true;
    arg_t *quickArg = getArgumentByName("Quick mode");
    quickArg->set = true;
    quickArg->boolVal = true;

    int i = 0;
    device_t device;
    while (findDevice(&device, false) == -1)
    {
        sleep(1);
        i++;
        if (i == 10)
        {
            LOG(LOG_ERROR, "No iOS device found after 10 seconds - please connect a device.");
            return -1;
        }
    }

    checkm8();


    while (findDevice(&device, false) == -1)
    {
        sleep(1);
        i++;
        if (i == 10)
        {
            LOG(LOG_ERROR, "No iOS device found after 10 seconds - please connect a device.");
            return -1;
        }
    }

    if (isSerialNumberPwned(device.serialNumber)) {
        LOG(LOG_SUCCESS, "✅ Test 1 passed");
    } else {
        LOG(LOG_ERROR, "❌ Test 1 failed");
        return -1;
    }


    LOG(LOG_INFO, "Test 2: boot PongoOS");

    LOG_NO_NEWLINE(LOG_INFO, "Press enter when you're ready to continue...");
    getchar();

    while (findDevice(&device, false) == -1)
    {
        sleep(1);
        i++;
        if (i == 10)
        {
            LOG(LOG_ERROR, "No iOS device found after 10 seconds - please connect a device.");
            return -1;
        }
    }

    exploitArg->set = false;
    exploitArg->boolVal = false;
    arg_t *pongoArg = getArgumentByName("PongoOS");
    pongoArg->set = true;
    pongoArg->boolVal = true;
    checkm8();

    while (findDevice(&device, false) == -1)
    {
        sleep(1);
        i++;
        if (i == 10)
        {
            LOG(LOG_ERROR, "No iOS device found after 10 seconds - please connect a device.");
            return -1;
        }
    }

    if (isInPongoOS(device.serialNumber)) {
        LOG(LOG_SUCCESS, "✅ Test 2 passed");
    } else {
        LOG(LOG_ERROR, "❌ Test 2 failed");
        return -1;
    }

    LOG(LOG_INFO, "Test 3: jailbreak");

    LOG_NO_NEWLINE(LOG_INFO, "Press enter when you're ready to continue...");
    getchar();

    while (findDevice(&device, false) == -1)
    {
        sleep(1);
        i++;
        if (i == 10)
        {
            LOG(LOG_ERROR, "No iOS device found after 10 seconds - please connect a device.");
            return -1;
        }
    }

    pongoArg->set = false;
    pongoArg->boolVal = false;
    arg_t *jailbreakArg = getArgumentByName("Jailbreak");
    jailbreakArg->set = true;
    jailbreakArg->boolVal = true;
    if (checkm8() == 0) {
        LOG(LOG_SUCCESS, "✅ Test 3 passed");
    } else {
        LOG(LOG_ERROR, "❌ Test 3 failed");
        return -1;
    }

    LOG(LOG_INFO, "🎉 All tests passed!");

    return 0;
}