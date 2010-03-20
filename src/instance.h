#ifndef __INSTANCE_H__

#define __INSTANCE_H__

// Instances must be constrained to exactly 16 bytes on all 32-bit platforms (check this)
#define INSTANCE_SIZE 16

#define INSTANCE_ALIGN 4

#include "page.h"
#include "chunk.h"
#include "driver.h"
#include "handle.h"

typedef union instance {
  page_t page;
  chunk_t chunk;
  driver_t driver;
  handle_t handle;
  layer_t layer;
  site_t site;
  kick_t kick;
  punch_t punch;
} instance_t;

#ifdef CHECK_INSTANCE_SIZE
#define INSTANCE_INVALID (sizeof(instance_t) - INSTANCE_SIZE)
#else
#define INSTANCE_INVALID 0
#endif

#define GET_NEXT_INSTANCE(inst) (inst+= INSTANCE_SIZE)

#define ITERATE_INSTANCE(inst,dir,type) (GET_INSTANCE(inst)->link[dir].type)

#endif // __INSTANCE_H__
