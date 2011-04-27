#ifndef __MAP_H__

#define __MAP_H__

/* A map defines a hierarchical partitioning of a coordinate space into dimensional sections which are virtual groups of enumerated bits .   */

typedef struct map {
  hword_t resource_id;   // An id for a resource that maps here
  hword_t level;
} map_t;

#endif // __MAP_H__
