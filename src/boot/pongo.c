#include <boot/pongo.h>

bool compressWithLZ4(char *input, char *output, int inputSize, int *outputSize)
{
    int outputSizeT = *outputSize;
    *outputSize = LZ4_compress_HC(input, output, inputSize, outputSizeT, LZ4HC_CLEVEL_MAX);
    if (*outputSize == 0) {
        return false;
    }
    return true;
}

bool addShellcodeToPongoOS(char *output, int *outputSize)
{
    void *shellcode = malloc(0x200);
    FILE *shellcodeData = fopen("src/boot/payloads/checkra1n/shellcode.bin", "rb");
    if (shellcodeData == NULL)
    {
        LOG(LOG_ERROR, "ERROR: failed to open shellcode file");
    }
    fread(shellcode, 1, 0x200, shellcodeData);
    fclose(shellcodeData);

    FILE *pongoFile = fopen("src/boot/payloads/checkra1n/Pongo.bin", "rb");
    if (pongoFile == NULL)
    {
        LOG(LOG_ERROR, "ERROR: failed to open PongoOS file");
    }
    
    fseek(pongoFile, 0, SEEK_END);
    int pongoSize = ftell(pongoFile);
    char *pongo = malloc(pongoSize);
    fread(pongo, 1, pongoSize, pongoFile);
    fclose(pongoFile);
    output = malloc(pongoSize);

    LOG(LOG_INFO, "Compressing PongoOS...");    
    bool ret = compressWithLZ4(pongo, output, pongoSize, &pongoSize);
    if (!ret) {
        LOG(LOG_ERROR, "ERROR: failed to compress PongoOS");
        return false;
    }

    LOG(LOG_INFO, "Adding shellcode to PongoOS...");
    void *tmp = malloc(pongoSize + 0x200);
    memcpy(tmp, shellcode, 0x200);
    memcpy(tmp + 0x200, output, pongoSize);
    free(output);
    output = tmp;
    pongoSize += 0x200;
    free(shellcode);

    LOG(LOG_INFO, "Adjusting PongoOS size...");
    uint32_t *size = (uint32_t *)(output + 0x1fc);
    *size = pongoSize - 0x200;
    LOG(LOG_INFO, "PongoOS size: %d", pongoSize);
    *outputSize = pongoSize;
    LOG(LOG_INFO, "PongoOS size: %d", *outputSize);

    // Debugging
    FILE *pongoCompressed = fopen("src/boot/payloads/checkra1n/PongoCompressed.bin", "wb+");
    if (pongoCompressed == NULL)
    {
        LOG(LOG_ERROR, "ERROR: failed to open PongoOS file");
    }
    fwrite(output, pongoSize, 1, pongoCompressed);
    fclose(pongoCompressed);

    return true;
}

bool bootPongoOS(device_t *device)
{
    int length = 0;
    int size, pongoLength;
    char *pongo;
    transfer_ret_t ret;
    // if (!addShellcodeToPongoOS(pongo, &pongoLength)) {
    //     LOG(LOG_ERROR, "ERROR: failed to add shellcode to PongoOS");
    //     return false;
    // }
    FILE *pongoFile = fopen("src/boot/payloads/checkra1n/pongoCompressed.bin", "rb");
    if (pongoFile == NULL)
    {
        LOG(LOG_ERROR, "ERROR: failed to open PongoOS file");
    }
    fseek(pongoFile, 0, SEEK_END);
    pongoLength = ftell(pongoFile);
    pongo = malloc(pongoLength);
    fread(pongo, pongoLength, 1, pongoFile);
    fclose(pongoFile);
    char *serial = getDeviceSerialNumberWithTransfer(&device->handle);
    if (serial == NULL) {
        LOG(LOG_ERROR, "ERROR: failed to get device serial number");
        return false;
    }
    LOG(LOG_INFO, "Device serial number: %s", serial);
    LOG(LOG_INFO, "Sending PongoOS...");
    while (length < pongoLength) {
        retry:
            size = ((pongoLength - length) > 0x800) ? 0x800 : (pongoLength - length);
            sendUSBControlRequest(&device->handle, 0x21, DFU_DNLOAD, 0, 0, (unsigned char *)&pongo[length], size, &ret);
            if (ret.sz != size || ret.ret != USB_TRANSFER_OK) {
                LOG(LOG_ERROR, "ERROR: failed to send PongoOS");
                sleep_ms(100);
                goto retry;
            }
            length += size;
    }
    sendUSBControlRequestNoData(&device->handle, 0x21, DFU_CLRSTATUS, 0, 0, 0, NULL);
    LOG(LOG_SUCCESS, "pongoOS sent, should be booting");
    return true;
}