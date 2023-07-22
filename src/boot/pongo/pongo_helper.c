#include <boot/pongo/pongo_helper.h>

// for DIR, opendir() etc
// Based on pongo_helper.c from palera1n, heavily simplified

#define USB_RET_SUCCESS         KERN_SUCCESS
#define USB_RET_NOT_RESPONDING  kIOReturnNotResponding
#define USB_RET_IO              kIOReturnNotReady
#define USB_RET_NO_DEVICE       kIOReturnNoDevice

#define CMD_LEN_MAX 0x200

bool issuePongoCommand(usb_handle_t *handle, char *command)
{
	bool ret;
	if (command == NULL) return EINVAL;
	size_t len = strlen(command);
	char commandBuffer[0x200];
	if (len > (CMD_LEN_MAX - 2))
	{
		LOG(LOG_ERROR, "Pongo command %s too long (max %d)", command, CMD_LEN_MAX - 2);
		return EINVAL;
	}
    LOG(LOG_DEBUG, "Executing PongoOS command: '%s'", command);
	snprintf(commandBuffer, 512, "%s\n", command);
	len = strlen(commandBuffer);
	ret = sendUSBControlRequestNoData(handle, 0x21, 4, 1, 0, 0, NULL);
	if (!ret)
		goto bad;
	ret = sendUSBControlRequest(handle, 0x21, 3, 0, 0, commandBuffer, (uint32_t)len, NULL);
bad:
	if (!ret)
	{
        if (command != NULL && (!strncmp("boot", command, 4))) {
			LOG(LOG_DEBUG, "Ignoring transfer error for boot command");
			return 0;
		}
		LOG(LOG_ERROR, "USB error: %d", ret);
		return ret;
	}
	else
		return ret;
}

int uploadFileToPongo(usb_handle_t *handle, unsigned char *data, unsigned int dataLength)
{
	bool ret;
	ret = sendUSBControlRequest(handle, 0x21, 1, 0, 0, (unsigned char *)&dataLength, 4, NULL);
	if (ret)
	{
        int bulkRet = sendUSBBulkUpload(handle, data, dataLength);
		if (bulkRet == kIOReturnSuccess)
		{
		    LOG(LOG_DEBUG, "Uploaded 0x%X bytes to PongoOS", dataLength);
		} else {
            LOG(LOG_ERROR, "Failed to upload 0x%X bytes to PongoOS", dataLength);
        }
	}
	resetUSBHandle(handle);
	closeUSBHandle(handle);
	sleep_ms(100);
	initUSBHandle(handle, 0x5ac, 0x4141);
	waitUSBHandle(handle, 0, 0, NULL, NULL);
	sleep(3);
	return ret;
}

void jailbreakBoot(usb_handle_t *handle) {
	LOG(LOG_INFO, "Setting up jailbroken iOS");

	// Lock fuses
	issuePongoCommand(handle, "fuse lock");
	sleep(1);

	// Let PongoOS choose what to do with SEP
	issuePongoCommand(handle, "sep auto");
	sleep(1);

	// Open kernel patchfinder and upload it to PongoOS
	LOG(LOG_INFO, "Sending kernel patchfinder");
	FILE *kpf = fopen("src/kernel/patchfinder/kpf", "rb");
	if (kpf == NULL) {
		LOG(LOG_ERROR, "Failed to open kernel patchfinder file - please make sure src/kernel/patchfinder/kpf exists");
		return;
	}
	fseek(kpf, 0, SEEK_END);
	unsigned int kpfLength = ftell(kpf);
	fseek(kpf, 0, SEEK_SET);
	unsigned char *kpfData = malloc(kpfLength);
	if (kpfData == NULL) {
		LOG(LOG_ERROR, "Failed to allocate memory for kpf");
		return;
	}
	fread(kpfData, kpfLength, 1, kpf);
	fclose(kpf);
	uploadFileToPongo(handle, (unsigned char *)kpfData, kpfLength);
	free(kpfData);

	// Load the kernel patchfinder module
	issuePongoCommand(handle, "modload");
	sleep(2);
	
	LOG(LOG_INFO, "Sending ramdisk and overlay");

	// Open ramdisk.dmg and upload it to PongoOS
	FILE *ramdisk = fopen("src/userland/ramdisk/ramdisk.dmg", "rb");
	if (ramdisk == NULL) {
		LOG(LOG_ERROR, "Failed to open ramdisk file - please make sure src/userland/ramdisk/ramdisk.dmg exists");
		return;
	}
	fseek(ramdisk, 0, SEEK_END);
	unsigned int ramdiskLength = ftell(ramdisk);
	fseek(ramdisk, 0, SEEK_SET);
	unsigned char *ramdiskData = malloc(ramdiskLength);
	if (ramdiskData == NULL) {
		LOG(LOG_ERROR, "Failed to allocate memory for ramdisk");
		return;
	}
	fread(ramdiskData, ramdiskLength, 1, ramdisk);
	fclose(ramdisk);
	uploadFileToPongo(handle, (unsigned char *)ramdiskData, ramdiskLength);
	free(ramdiskData);

	// Mount the ramdisk
	issuePongoCommand(handle, "ramdisk");

	// Open binpack.dmg and upload it to PongoOS
	FILE *overlay = fopen("src/userland/binpack/binpack.dmg", "rb");
	if (overlay == NULL) {
		LOG(LOG_ERROR, "Failed to open binpack file - please make sure src/userland/binpack/binpack.dmg exists");
		return;
	}
	fseek(overlay, 0, SEEK_END);
	unsigned int overlayLength = ftell(overlay);
	fseek(overlay, 0, SEEK_SET);
	unsigned char *overlayData = malloc(overlayLength);
	if (overlayData == NULL) {
		LOG(LOG_ERROR, "Failed to allocate memory for binpack");
		return;
	}
	fread(overlayData, overlayLength, 1, overlay);
	fclose(overlay);
	uploadFileToPongo(handle, (unsigned char *)overlayData, overlayLength);
	free(overlayData);

	// Mount the binpack
	issuePongoCommand(handle, "overlay");

	// Set boot arguments to boot from ramdisk
	char *args = "xargs rootdev=md0";

	// This is very messy but it will have to do for now
	if (getArgumentByName("Boot arguments")->set || getArgumentByName("Verbose boot")->set || getArgumentByName("Serial output")->set) {
		size_t extra = 0;
		if (getArgumentByName("Boot arguments")->set) {
			extra += strlen(getArgumentByName("Boot arguments")->stringVal) + 1; // + 1 for space
		}
		if (getArgumentByName("Verbose boot")->boolVal) {
			if ((getArgumentByName("Boot arguments")->set && strstr(getArgumentByName("Boot arguments")->stringVal, "-v") != NULL)) {
			} else {
				extra += strlen(" -v");
			}
		}
		if (getArgumentByName("Serial output")->boolVal) {
			if ((getArgumentByName("Boot arguments")->set && strstr(getArgumentByName("Boot arguments")->stringVal, "serial=") != NULL)) {
			} else {
				extra += strlen(" serial=3");
			}
		}
		args = malloc(strlen(args) + extra);
		strcpy(args, "xargs rootdev=md0");
		if (getArgumentByName("Boot arguments")->set) {
			strcat(args, " ");
			strcat(args, getArgumentByName("Boot arguments")->stringVal);
		}
		if (getArgumentByName("Verbose boot")->boolVal && strstr(args, "-v") == NULL) {
			strcat(args, " -v");
		}
		if (getArgumentByName("Serial output")->boolVal && strstr(args, "serial=") == NULL) {
			strcat(args, " serial=3");
		}
	}
	issuePongoCommand(handle, args);
	sleep(1);

	if (strstr(args, "-v") != NULL) {
		
		// Give XNU ownership of the framebuffer for verbose boot
		issuePongoCommand(handle, "xfb");
		sleep(1);
	}
	free(args);


	LOG(LOG_INFO, "Booting iOS");

	// Patch and boot XNU
	issuePongoCommand(handle, "bootx");

	// All done
	return;
}