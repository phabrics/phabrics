#ifndef __RESOURCE_H__

#define __RESOURCE_H__

#include "stream.h"

#define RESOURCE_TYPE_CPU 0
#define RESOURCE_TYPE_MEM 1
#define RESOURCE_TYPE_SHMEM 2
#define RESOURCE_TYPE_CACHE 3

// A resource is an abstraction of an addressable unit of physical storage that can is used to hold the actual bits

typedef struct resource {
  hword_t id;        // A unique identifier for the resource
  hword_t type;      // The type of the resource
  hword_t driver_id; // Id of the driver for the given resource
  hword_t next;      // The next resource in a list of resources
  word_t size;       // The size of the resource in bytes
  entry_t alloc;     // A entry point for allocating of sites
} resource_t;

// Given a stream, load the resources from it.  xo
status_t load_resources(stream_t res_conf);
word_t get_total_resources();
word_t get_num_resources(hword_t type);
resource_t get_resource_by_id(hword_t id);

#endif // __RESOURCE_H__
