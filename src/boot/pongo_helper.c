#include <boot/pongo_helper.h>

// Heavily based on pongo_helper.c from palera1n

#define USB_RET_SUCCESS         KERN_SUCCESS
#define USB_RET_NOT_RESPONDING  kIOReturnNotResponding
#define USB_RET_IO              kIOReturnNotReady
#define USB_RET_NO_DEVICE       kIOReturnNoDevice

#define CMD_LEN_MAX 0x200

int issuePongoCommand(usb_handle_t *handle, char *command)
{
	bool ret;
	if (command == NULL) return EINVAL;
	size_t len = strlen(command);
	char command_buf[512];
	if (len > (CMD_LEN_MAX - 2))
	{
		LOG(LOG_ERROR, "Pongo command %s too long (max %d)", command, CMD_LEN_MAX - 2);
		return EINVAL;
	}
    LOG(LOG_DEBUG, "Executing PongoOS command: '%s'", command);
	snprintf(command_buf, 512, "%s\n", command);
	len = strlen(command_buf);
	ret = sendUSBControlRequestNoData(handle, 0x21, 4, 1, 0, 0, NULL);
	if (!ret)
		goto bad;
	ret = sendUSBControlRequest(handle, 0x21, 3, 0, 0, command_buf, (uint32_t)len, NULL);
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

int uploadFileToPongo(usb_handle_t *handle, unsigned char *buf, unsigned int buf_len)
{
	bool ret;
    LOG(LOG_DEBUG, "Uploading %llu bytes to PongoOS", (unsigned long long)buf_len);
	ret = sendUSBControlRequest(handle, 0x21, 1, 0, 0, (unsigned char *)&buf_len, 4, NULL);
	if (ret)
	{
        LOG(LOG_DEBUG, "Starting transfer");
        int bulkRet = sendUSBBulkUpload(handle, buf, buf_len);
        LOG(LOG_DEBUG, "Transfer done");
		if (bulkRet == kIOReturnSuccess)
		{
		    LOG(LOG_DEBUG, "Uploaded %llu bytes to PongoOS", (unsigned long long)buf_len);
		} else {
            LOG(LOG_ERROR, "Failed to upload %llu bytes to PongoOS", (unsigned long long)buf_len);
        }
	}
	return ret;
}