#ifndef PONGO_H
#define PONGO_H

#include <AlfieLoader.h>
#include <boot/lz4/lz4hc.h>
#include <usb/usb.h>
#include <exploit/dfu.h>

bool bootPongoOS(device_t *device);

#endif // PONGO_H