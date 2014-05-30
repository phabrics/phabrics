#ifndef __DRIVER_H__

#define __DRIVER_H__

#include "string.h"
#include "entry.h"

#define DRIVER_TYPE_TYPE 0
#define DRIVER_TYPE_PROC 1
#define DRIVER_TYPE_MEM 2
#define DRIVER_TYPE_SYS 3
#define DRIVER_TYPE_THREAD 4
#define DRIVER_TYPE_DISPLAY 5

typedef struct driver {
  hword_t id;           // An identifier for the driver to be referenced by
  hword_t type;         // Driver classification
  hword_t status;       // A status describing the current state of the driver
  hword_t mask;         // A bit mask
  string_ref_t desc;    // A string describing the driver
  entry_ref_t enter;    // An executable entry point for the driver
} driver_t;

driver_t *find_driver(hword_t type, hword_t id);

#endif // __DRIVER_H__
