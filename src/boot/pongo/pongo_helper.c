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
	sleep(1);
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
	else {
		return ret;
	}
}

int uploadFileToPongo(usb_handle_t *handle, unsigned char *data, unsigned int dataLength)
{
	bool ret;
	ret = sendUSBControlRequest(handle, 0x21, 1, 0, 0, (unsigned char *)&dataLength, 4, NULL);
	if (ret)
	{
        int bulkRet = sendUSBBulkUpload(handle, data, dataLength);
#ifdef ACHILLES_LIBUSB
		if (bulkRet == dataLength)
		{
			LOG(LOG_DEBUG, "Uploaded 0x%X bytes to PongoOS", dataLength);
		} else {
			LOG(LOG_ERROR, "Failed to upload 0x%X bytes to PongoOS, sent 0x%X bytes", dataLength, bulkRet);
			ret = false;
		}
#else
		if (bulkRet == kIOReturnSuccess)
		{
		    LOG(LOG_DEBUG, "Uploaded 0x%X bytes to PongoOS", dataLength);
		} else {
            LOG(LOG_ERROR, "Failed to upload 0x%X bytes to PongoOS", dataLength);
        }
#endif
	}
	resetUSBHandle(handle);
	closeUSBHandle(handle);
	sleep_ms(100);
	initUSBHandle(handle, 0x5ac, 0x4141);
	waitUSBHandle(handle, NULL, NULL);
	sleep(3);
	return ret;
}

void jailbreakBoot(usb_handle_t *handle) {
	LOG(LOG_VERBOSE, "Setting up jailbroken iOS");

	// Lock fuses
	issuePongoCommand(handle, "fuse lock");
	sleep(1);

	// Let PongoOS choose what to do with SEP
	issuePongoCommand(handle, "sep auto");
	sleep(1);

	// Upload kernel patchfinder to PongoOS
	if (getArgumentByName("Custom kernel patchfinder")->set) {
		LOG(LOG_VERBOSE, "Sending custom kernel patchfinder");

		FILE *kpf = fopen(getArgumentByName("Custom kernel patchfinder")->stringVal, "rb");
		if (kpf == NULL) {
			LOG(LOG_ERROR, "Failed to open kernel patchfinder file - please make sure the file path is correct");
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
		uploadFileToPongo(handle, kpfData, kpfLength);
	} else {
		if (getArgumentByName("Jailbreak")->set) {
			#include <kernel/patchfinder/kpf-palera1n.h>
			LOG(LOG_VERBOSE, "Sending kernel patchfinder");
			uploadFileToPongo(handle, kpf_palera1n, kpf_palera1n_len);
		} else {
			#include <kernel/patchfinder/kpf.h>
			LOG(LOG_VERBOSE, "Sending kernel patchfinder");
			uploadFileToPongo(handle, checkra1n_kpf_pongo, checkra1n_kpf_pongo_len);
		}
	}

	// Load the kernel patchfinder module
	issuePongoCommand(handle, "modload");
	sleep(2);
	
	LOG(LOG_VERBOSE, "Sending ramdisk and overlay");

	// Upload ramdisk to PongoOS
	if (getArgumentByName("Custom ramdisk")->set) {
		LOG(LOG_VERBOSE, "Sending custom ramdisk");

		FILE *ramdisk = fopen(getArgumentByName("Custom ramdisk")->stringVal, "rb");
		if (ramdisk == NULL) {
			LOG(LOG_ERROR, "Failed to open ramdisk file - please make sure the file path is correct");
			return;
		}
		fseek(ramdisk, 0, SEEK_END);
		unsigned int ramdiskLength = ftell(ramdisk);
		fseek(ramdisk, 0, SEEK_SET);
		unsigned char *ramdiskData = malloc(ramdiskLength);
		if (ramdiskData == NULL) {
			LOG(LOG_ERROR, "Failed to allocate memory for kpf");
			return;
		}
		fread(ramdiskData, ramdiskLength, 1, ramdisk);
		fclose(ramdisk);
		uploadFileToPongo(handle, ramdiskData, ramdiskLength);
	} else {
		#include <userland/jbinit/ramdisk.h>
		uploadFileToPongo(handle, ramdisk_dmg, ramdisk_dmg_len);
	}

	// Mount the ramdisk
	issuePongoCommand(handle, "ramdisk");

	// Upload overlay to PongoOS
	if (getArgumentByName("Custom overlay")->set) {
		LOG(LOG_VERBOSE, "Sending custom overlay");

		FILE *overlay = fopen(getArgumentByName("Custom overlay")->stringVal, "rb");
		if (overlay == NULL) {
			LOG(LOG_ERROR, "Failed to open overlay file - please make sure the file path is correct");
			return;
		}
		fseek(overlay, 0, SEEK_END);
		unsigned int overlayLength = ftell(overlay);
		fseek(overlay, 0, SEEK_SET);
		unsigned char *overlayData = malloc(overlayLength);
		if (overlayData == NULL) {
			LOG(LOG_ERROR, "Failed to allocate memory for kpf");
			return;
		}
		fread(overlayData, overlayLength, 1, overlay);
		fclose(overlay);
		uploadFileToPongo(handle, overlayData, overlayLength);
	} else {
		#include <userland/jbinit/binpack.h>
		uploadFileToPongo(handle, binpack_dmg, binpack_dmg_len);
	}

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

	// Only give XNU ownership of the framebuffer if we're booting with verbose output
	// otherwise verbose boot is enabled regardless of boot-args
	if (strstr(args, "-v") != NULL) {
		// Give XNU ownership of the framebuffer for verbose boot
		issuePongoCommand(handle, "xfb");
		sleep(1);
	}


	LOG(LOG_INFO, "Booting jailbroken iOS");

	// Patch and boot XNU
	issuePongoCommand(handle, "bootx");

	// All done
	return;
}