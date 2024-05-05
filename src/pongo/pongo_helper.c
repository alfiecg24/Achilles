#include <pongo/pongo_helper.h>
#include <pongo/shellcode.h>
#include <pongo/pongo.h>
#include <pongo/lz4/lz4hc.h>

extern struct AchillesArgs args;

// All credits go to @mineekdev for their openra1n project,
// which provided the template for this code.

bool prepare_pongo(unsigned char **pongoBuf, size_t *size)
{
    size_t shellcodeSize, pongoSize;
    void *shellcode, *pongo;

    // The shellcode that is appended to the beginning of the
    // LZ4-compressed Pongo is actually an LZ4 decompressor
    // that decompresses the Pongo image into memory.
    // It is, in effect, a self-extracting payload.

    shellcodeSize = decompressor_shellcode_len;
    shellcode = malloc(shellcodeSize);
    memcpy(shellcode, decompressor_shellcode, shellcodeSize);


    // Get PongoOS
    if (args.pongoPath) {
        FILE *pongoFile;
        pongoFile = fopen(args.pongoPath, "rb");
        if (pongoFile == NULL)
        {
            LOG(LOG_ERROR, "Failed to open PongoOS file (%s)", args.pongoPath);
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
        pongoSize = pongo_bin_len;
        pongo = malloc(pongoSize);
        memcpy(pongo, pongo_bin, pongoSize);
    }

    // Compress PongoOS
    char *pongoCompressed = malloc(pongoSize);
    LOG(LOG_VERBOSE, "Compressing PongoOS");
    pongoSize = LZ4_compress_HC(pongo, pongoCompressed, pongoSize, pongoSize, LZ4HC_CLEVEL_MAX);
    if (pongoSize == 0) {
        LOG(LOG_ERROR, "Failed to compress PongoOS");
        return false;
    }

    // Add shellcode to PongoOS
    LOG(LOG_VERBOSE, "Adding shellcode to PongoOS");
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

int issue_pongo_command(usb_handle_t *handle, char *command, char *outBuffer)
{
    bool ret;
    uint8_t inProgress = 1;
    uint32_t outPosition = 0;
    uint32_t outLength = 0;
    transfer_ret_t transferRet;
    char stdoutBuffer[0x2000];
	if (command == NULL) goto fetch_output;
	size_t length = strlen(command);
	char commandBuffer[0x200];
	if (length > (CMD_LENGTH_MAX - 2))
	{
		LOG(LOG_ERROR, "Pongo command %s too long (max %d)", command, CMD_LENGTH_MAX - 2);
		return -1;
	}
    LOG(LOG_VERBOSE, "Executing PongoOS command: '%s'", command);
	snprintf(commandBuffer, 512, "%s\n", command);
	length = strlen(commandBuffer);
	ret = send_usb_control_request_no_data(handle, 0x21, 4, 1, 0, 0, NULL);
	if (!ret)
		goto bad;
	ret = send_usb_control_request_no_timeout(handle, 0x21, 3, 0, 0, commandBuffer, (uint32_t)length, NULL);
fetch_output:
    while (inProgress) {
        ret = send_usb_control_request_no_timeout(handle, 0xA1, 2, 0, 0, &inProgress, (uint32_t)sizeof(inProgress), NULL);
        if (ret) {
            ret = send_usb_control_request_no_timeout(handle, 0xA1, 1, 0, 0, stdoutBuffer + outPosition, 0x1000, &transferRet);
            outLength = transferRet.sz;
            if (transferRet.ret == USB_TRANSFER_OK) {
                outPosition += outLength;
                if (outPosition > 0x1000) {
                    memmove(stdoutBuffer, stdoutBuffer + outPosition - 0x1000, 0x1000);
                    outPosition = 0x1000;
                }
            }
        }
        if (transferRet.ret != USB_TRANSFER_OK) {
            goto bad;
        }
    }
bad:
	if (transferRet.ret != USB_TRANSFER_OK)
	{
        if (!strncmp("boot", command, 4)) {
            return 0;
        } else if (command != NULL) {
            LOG(LOG_ERROR, "USB transfer error: 0x%x, wLength out 0x%x.", transferRet.ret, transferRet.sz);
            return transferRet.ret;
        } else {
            return -1;
        }
	}
	else {
        if (outBuffer) {
            memcpy(outBuffer, stdoutBuffer, outPosition);
        }
		return ret;
	}
}

bool upload_file_to_pongo(usb_handle_t *handle, const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        LOG(LOG_ERROR, "Failed to stat %s.", path);
        return false;
    }
    if (S_ISDIR(st.st_mode)) {
        LOG(LOG_ERROR, "%s is a directory.", path);
        return false;
    }
    
    size_t length = st.st_size;
    // TODO: Size sanity check?

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        LOG(LOG_ERROR, "Failed to open %s.", path);
        return false;
    }
    char *buffer = malloc(length);
    if (buffer == NULL) {
        LOG(LOG_ERROR, "Failed to allocate buffer for %s.", path);
        return false;
    }
    if (read(fd, buffer, length) != length) {
        LOG(LOG_ERROR, "Failed to read %s.", path);
        return false;
    }

    LOG(LOG_VERBOSE, "Uploading %s to PongoOS...", path);

    bool ret = false;
    transfer_ret_t transferRet;
    ret = send_usb_control_request(handle, 0x21, DFU_DNLOAD, 0, 0, (unsigned char *)&length, 4, &transferRet);
    if (transferRet.ret == USB_TRANSFER_OK) {
        ret = send_usb_bulk_upload(handle, buffer, length);
    }

    close(fd);
    free(buffer);
    return ret;
}

char *append_boot_arguments(const char *base, const char *extra) {
    char *newBootArgs = malloc(strlen(base) + strlen(extra) + 1);
    if (newBootArgs == NULL) {
        LOG(LOG_ERROR, "Failed to allocate memory for boot arguments.");
        return NULL;
    }
    sprintf(newBootArgs, "%s %s", base, extra);
    return newBootArgs;
}

bool pongo_jailbreak(usb_handle_t *handle) {
    issue_pongo_command(handle, "fuse lock", NULL);
    issue_pongo_command(handle, "sep auto", NULL);

    // Upload kernel patchfinder
    if (!upload_file_to_pongo(handle, args.kpfPath)) {
        LOG(LOG_ERROR, "Failed to upload kernel patchfinder.");
        return false;
    }
    issue_pongo_command(handle, "modload", NULL);

    // Upload ramdisk
    if (args.ramdiskPath) {
        if (!upload_file_to_pongo(handle, args.ramdiskPath)) {
            LOG(LOG_ERROR, "Failed to upload ramdisk.");
            return false;
        }
        issue_pongo_command(handle, "ramdisk", NULL);
    }

    // Upload overlay
    if (args.overlayPath) {
        if (!upload_file_to_pongo(handle, args.overlayPath)) {
            LOG(LOG_ERROR, "Failed to upload overlay.");
            return false;
        }
        issue_pongo_command(handle, "overlay", NULL);
    }

    // Set boot arguments
    char *bootArgs = "xargs";
    if (args.jailbreak && args.ramdiskPath) {
        char *newBootArgs = append_boot_arguments(bootArgs, "rootdev=md0");
        if (newBootArgs == NULL) return false;
        bootArgs = newBootArgs;
    }
    if (args.verboseBoot) {
        char *newBootArgs = append_boot_arguments(bootArgs, "-v");
        if (newBootArgs == NULL) return false;
        bootArgs = newBootArgs;
    }
    if (args.serialOutput) {
        char *newBootArgs = append_boot_arguments(bootArgs, "serial=3");
        if (newBootArgs == NULL) return false;
        bootArgs = newBootArgs;
    }
    if (args.bootArgs) {
        char *newBootArgs = append_boot_arguments(bootArgs, args.bootArgs);
        if (newBootArgs == NULL) return false;
        bootArgs = newBootArgs;
    }
    if (args.bootArgs || args.ramdiskPath || args.verboseBoot || args.serialOutput) {
        issue_pongo_command(handle, bootArgs, NULL);
    }

    // Extra fix for verbose boot
    if (strstr(bootArgs, "-v")) {
        issue_pongo_command(handle, "xfb", NULL);
    }

    // Boot
    issue_pongo_command(handle, "bootx", NULL);

    // Done
    return true;
}
