#include <pongo/pongo_helper.h>

int issue_pongo_command(usb_handle_t *handle, char *command) {
    uint32_t outPosition = 0;
    uint32_t outLength = 0;
    transfer_ret_t transferRet;
    bool ret;
    uint8_t inProgress = 1;
    if (command == NULL) goto fetch_output;
    size_t length = strlen(command);
    char commandBuffer[CMD_LENGTH_MAX];
    char stdoutBuffer[0x2000];

    if (length > (CMD_LENGTH_MAX - 2)) {
        LOG(LOG_ERROR, "PongoOS command %s is too long (max %d, got %d).", command, CMD_LENGTH_MAX - 2, length);
    }

    LOG(LOG_VERBOSE, "Sending command to PongoOS: %s.", command);

    snprintf(commandBuffer, 512, "%s\n", command);
    length = strlen(commandBuffer);
    ret = send_usb_control_request_no_data(handle, 0x21, 4, 1, 0, 0, NULL);
    if (!ret) goto bad;
    ret = send_usb_control_request(handle, 0x21, 3, 0, 0, commandBuffer, (uint32_t)length, NULL);
fetch_output:
    while (inProgress) {
        ret = send_usb_control_request(handle, 0xA1, 2, 0, 0, &inProgress, (uint32_t)sizeof(inProgress), &transferRet);
        if (transferRet.ret == USB_TRANSFER_OK) {
            ret = send_usb_control_request(handle, 0xA1, 1, 0, 0, stdoutBuffer + outPosition, 0x1000, &transferRet);
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
    if (transferRet.ret != USB_TRANSFER_OK) {
        if (!strncmp("boot", command, 4)) {
            return 0;
        } else if (command != NULL) {
            LOG(LOG_ERROR, "USB transfer error: 0x%x, wLength out 0x%x.", transferRet.ret, transferRet.sz);
            return transferRet.ret;
        } else {
            return -1;
        }
    }
    return transferRet.ret;
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

bool pongo_jailbreak(usb_handle_t *handle) {
    issue_pongo_command(handle, "fuse lock");
    issue_pongo_command(handle, "sep auto");

    // Upload kernel patchfinder
    if (!upload_file_to_pongo(handle, args.kpfPath)) {
        LOG(LOG_ERROR, "Failed to upload kernel patchfinder.");
        return false;
    }
    sleep(1);
    issue_pongo_command(handle, "modload");
    sleep(2);

    // // Upload ramdisk
    // if (!upload_file_to_pongo(handle, args.ramdiskPath)) {
    //     LOG(LOG_ERROR, "Failed to upload ramdisk.");
    //     return false;
    // }
    // issue_pongo_command(handle, "ramdisk");
    // sleep(2);

    // // Upload overlay
    // if (!upload_file_to_pongo(handle, args.overlayPath)) {
    //     LOG(LOG_ERROR, "Failed to upload overlay.");
    //     return false;
    // }
    // issue_pongo_command(handle, "overlay");
    // sleep(2);

    // // Set boot arguments
    // char *bootArgs = "xargs rootdev=md0";
    // if (args.bootArgs) {
    //     char *newBootArgs = malloc(strlen(args.bootArgs) + strlen(bootArgs) + 2);
    //     if (newBootArgs == NULL) {
    //         LOG(LOG_ERROR, "Failed to allocate memory for boot arguments.");
    //         return false;
    //     }
    //     snprintf(newBootArgs, strlen(args.bootArgs) + strlen(bootArgs) + 2, "%s %s", bootArgs, args.bootArgs);
    //     bootArgs = newBootArgs;
    // }
    // issue_pongo_command(handle, bootArgs);
    // sleep(2);

    // // Extra fix for verbose boot
    // if (strstr(bootArgs, "-v")) {
    //     issue_pongo_command(handle, "xfb");
    //     sleep(2);
    // }

    // Boot arguments
    issue_pongo_command(handle, "xargs -v");

    // Framebuffer ownership to XNU
    issue_pongo_command(handle, "xfb");

    // Boot
    issue_pongo_command(handle, "bootx");

    // Done
    return true;
}