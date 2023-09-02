#include <boot/pongo/pongo.h>

bool isInPongoOS(char *serial) {
    if (serial == NULL) {
        LOG(LOG_ERROR, "Failed to get device serial number");
        return false;
    }
    if (strstr(serial, "SRTG:[PongoOS") != NULL) {
        return true;
    }
    return false;
}

void awaitPongoOS(usb_handle_t *handle) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (!isInPongoOS(getDeviceSerialNumber(handle))) {
        clock_gettime(CLOCK_MONOTONIC, &end);
        double timeTaken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        if (timeTaken > 10) {
            LOG(LOG_ERROR, "Timed out waiting for PongoOS to boot");
            return;
        }
        sleep_ms(100);
    }
    return;
}

// FOR COMPILATION //

#include <boot/payloads/checkra1n/headers/shellcode.h>
#include <boot/payloads/checkra1n/headers/Pongo.h>

// // // // // // //

// HUGE thanks to @mineekdev for their openra1n project,
// which was the template for this code.

bool preparePongoOS(void **pongoBuf, size_t *size)
{
    size_t shellcodeSize, pongoSize;
    void *shellcode, *pongo;

    // The shellcode that is appended to the beginning of the
    // LZ4-compressed Pongo is actually an LZ4 decompressor
    // that decompresses the Pongo image into memory.
    // It is, in effect, a self-extracting payload.

    shellcodeSize = shellcode_bin_len;
    shellcode = malloc(shellcodeSize);
    memcpy(shellcode, shellcode_bin, shellcodeSize);


    // Get PongoOS
    char *pongoPath;
    if (getArgumentByName("Override Pongo") != NULL
    && getArgumentByName("Override Pongo")->set) {
        FILE *pongoFile;
        pongoPath = malloc(strlen(getArgumentByName("Override Pongo")->stringVal) + 1);
        strcpy(pongoPath, getArgumentByName("Override Pongo")->stringVal);
        pongoFile = fopen(pongoPath, "rb");
        if (pongoFile == NULL)
        {
            LOG(LOG_ERROR, "Failed to open PongoOS file (%s)", pongoPath);
            return false;
        }
        fseek(pongoFile, 0, SEEK_END);
        pongoSize = ftell(pongoFile);
        rewind(pongoFile);
        if (pongoSize >= 0x7fe00) {
            LOG(LOG_ERROR, "PongoOS is too large, must be less than 0x7fe00 bytes but is 0x%X bytes", pongoSize);
            return false;
        }
        pongo = malloc(pongoSize);
        fread(pongo, pongoSize, 1, pongoFile);
        fclose(pongoFile);
    } else {
        pongoSize = build_Pongo_bin_len;
        pongo = malloc(pongoSize);
        memcpy(pongo, build_Pongo_bin, pongoSize);
    }

    // Compress PongoOS
    char *pongoCompressed = malloc(pongoSize);
    LOG(LOG_DEBUG, "Compressing PongoOS");
    pongoSize = LZ4_compress_HC(pongo, pongoCompressed, pongoSize, pongoSize, LZ4HC_CLEVEL_MAX);
    if (pongoSize == 0) {
        LOG(LOG_ERROR, "Failed to compress PongoOS");
        return false;
    }

    // Add shellcode to PongoOS
    LOG(LOG_DEBUG, "Adding shellcode to PongoOS");
    void *tmp = malloc(pongoSize + shellcodeSize);
    memcpy(tmp, shellcode, shellcodeSize);
    memcpy(tmp + shellcodeSize, pongoCompressed, pongoSize);
    free(pongo);
    pongo = tmp;
    pongoSize += shellcodeSize;
    free(shellcode);

    // Write size of compressed Pongo into data for decompressor
    uint32_t *pongoSizeInData = (uint32_t *)(pongo + 0x1fc);
    *pongoSizeInData = pongoSize - shellcodeSize;

    // Update parameters
    *pongoBuf = pongo;
    *size = pongoSize;

    return true;
}

bool bootPongoOS(device_t *device)
{
    void *PongoOS;
    size_t pongoSize;
    transfer_ret_t ret;
    if (!preparePongoOS(&PongoOS, &pongoSize)) { return false; }
    if (PongoOS == NULL) {
        LOG(LOG_ERROR, "Failed to get PongoOS");
        return false;
    }

    LOG(LOG_DEBUG, "Sending PongoOS of size 0x%X", pongoSize);
    {
        size_t lengthSent = 0, size;
        while (lengthSent < pongoSize) 
        {
            retry:
                size = ((pongoSize - lengthSent) > 0x800) ? 0x800 : (pongoSize - lengthSent);
                sendUSBControlRequest(&device->handle, 0x21, DFU_DNLOAD, 0, 0, (unsigned char *)&PongoOS[lengthSent], size, &ret);
                if (ret.sz != size || ret.ret != USB_TRANSFER_OK) {
                    LOG(LOG_DEBUG, "Retrying at length 0x%X", lengthSent);
                    sleep_ms(100);
                    goto retry;
                }
                lengthSent += size;
        }
    }
    sendUSBControlRequestNoData(&device->handle, 0x21, DFU_CLRSTATUS, 0, 0, 0, NULL);
    resetUSBHandle(&device->handle);
    closeUSBHandle(&device->handle);
    initUSBHandle(&device->handle, 0x05ac, 0x4141);
    LOG(LOG_INFO, "Waiting for PongoOS to boot");
    waitUSBHandle(&device->handle, NULL, NULL);
    awaitPongoOS(&device->handle);
    return true;
}