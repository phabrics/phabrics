#ifndef __INSTANCE_H__

#define __INSTANCE_H__

// Instances must be constrained to exactly 16 bytes on all platforms (check this)
#define INSTANCE_SIZE 16

// Instances must always be aligned on 16-byte boundaries
#define INSTANCE_ALIGN 4

#include "resource.h"
#include "chunk.h"
#include "driver.h"
#include "handle.h"

typedef union instance {
  entry_t entry;            // An entry point into an executable instance
  site_t site;              // A site is a location in the virtual address space of bits
  resource_t resource;      // A resource is a type of physical entity that contains a subset of sites
  chunk_t chunk;            // A chunk is a container for instances
  entry_t entry;            // An entry is a vector to a piece of executable code known as a handler
  handle_t handle;          // A handle is an argument passed into a handler
  driver_t driver;          // A driver is a handler that communicates with a system component
} instance_t;

#ifdef CHECK_INSTANCE_SIZE
#define INSTANCE_INVALID (sizeof(instance_t) - INSTANCE_SIZE)
#else
#define INSTANCE_INVALID 0
#endif

#define GET_NEXT_INSTANCE(inst) (inst+= INSTANCE_SIZE)

#define ITERATE_INSTANCE(inst,dir,type) (GET_INSTANCE(inst)->link[dir].type)

#endif // __INSTANCE_H__
