#ifndef __LAYER_H__

#define __LAYER_H__

#include "site.h"

typedef struct layer_info {
  site_info_t *site_info;
  site_t *layer[SITE_LENGTH];
} layer_info_t;

char * alloc_layer(size_t);

#endif
