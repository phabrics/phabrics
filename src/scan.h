#ifndef __SCAN_H__

#define __SCAN_H__

#define SCAN_TYPE unsigned

#define NUM_DIMS 3

typedef SCAN_TYPE scan_t;

typedef struct scan_info {
  scan_t idx, dcm;
  unsigned num_dims;
  scan_t dims[NUM_DIMS];
} scan_info_t;

#endif
