#ifndef PONGO_HELPER_H
#define PONGO_HELPER_H

#include <AlfieLoader.h>

#include <usb/usb.h>

#include <errno.h>
#include <fcntl.h>              // open
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>             // exit, strtoull
#include <string.h>             // strlen, strerror, memcpy, memmove
#include <unistd.h>             // close
#include <sys/mman.h>           // mmap, munmap
#include <sys/stat.h>           // fstst
#include <dirent.h>             // opendir, readdir, closedir

#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOCFPlugIn.h>

// ******************************************************
// Function: issuePongoCommand()
//
// Purpose: Send a command to PongoOS
//
// Parameters:
//      usb_handle_t *handle: the USB handle
//      char *command: the command to send
//
// Returns:
//      bool: true if the command was sent successfully, false otherwise
// ******************************************************
bool issuePongoCommand(usb_handle_t *handle, char *command);

// ******************************************************
// Function: uploadFileToPongo()
//
// Purpose: Upload a file to PongoOS
//
// Parameters:
//      usb_handle_t *handle: the USB handle
//      unsigned char *buf: the buffer to upload
//      unsigned int buf_len: the length of the buffer
//
// Returns:
//      int: the IOReturn code from the upload
// ******************************************************
int uploadFileToPongo(usb_handle_t *handle, unsigned char *buf, unsigned int buf_len);

#endif // PONGO_HELPER_H