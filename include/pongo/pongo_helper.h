#ifndef PONGO_HELPER_H
#define PONGO_HELPER_H

#include <Achilles.h>
#include <usb/usb.h>
#include <exploit/dfu.h>
#include <sys/stat.h>

#define USB_RET_SUCCESS         KERN_SUCCESS
#define USB_RET_NOT_RESPONDING  kIOReturnNotResponding
#define USB_RET_IO              kIOReturnNotReady
#define USB_RET_NO_DEVICE       kIOReturnNoDevice

#define CMD_LENGTH_MAX 512

bool prepare_pongo(unsigned char **pongoBuf, size_t *size);
int issue_pongo_command(usb_handle_t *handle, char *command, char *outBuffer);
bool upload_file_to_pongo(usb_handle_t *handle, const char *path);
bool pongo_jailbreak(usb_handle_t *handle);

#endif // PONGO_HELPER_H