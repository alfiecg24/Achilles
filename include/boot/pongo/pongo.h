#ifndef PONGO_H
#define PONGO_H

#include <Achilles.h>
#include <boot/lz4/lz4hc.h>
#include <usb/usb.h>
#include <exploit/dfu.h>
#include <time.h>

// ******************************************************
// Function: bootPongoOS()
//
// Purpose: Boot PongoOS on a device in YoloDFU mode
//
// Parameters:
//      device_t *device: the device to boot PongoOS on
//
// Returns:
//      bool: true if PongoOS was booted successfully, false otherwise
// ******************************************************
bool bootPongoOS(device_t *device);

// ******************************************************
// Function: isInPongoOS()
//
// Purpose: Checks if a USB serial number indicates a device that is in PongoOS
//
// Parameters:
//      char *serial: the serial number of the device
//
// Returns:
//      bool: true if the device is in PongoOS, false otherwise
// ******************************************************
bool isInPongoOS(char *serial);

#endif // PONGO_H