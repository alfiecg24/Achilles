#ifndef PONGO_H
#define PONGO_H

#include <AlfieLoader.h>
#include <boot/lz4/lz4hc.h>
#include <usb/usb.h>
#include <exploit/dfu.h>
#include <time.h>

bool bootPongoOS(device_t *device);
bool pongoOSHasBooted(char *serial);

#endif // PONGO_H