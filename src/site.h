#ifndef __SITE_H__

#define __SITE_H__

// site size should be represented by an integral, scalar type of the host machine that can be used
// as an address offset into a virtual lookup table

#define SITE_TYPE unsigned

#define SITE_LENGTH sizeof(SITE_TYPE)

#define SITE_LEVEL 5

typedef SITE_TYPE site_t;

typedef struct site_info {
  unsigned num_states, num_layers;
  unsigned num_slices, slice_layers;
  site_t slices[SITE_LEVEL];
  //  site_t *layers[SITE_LENGTH];
  int state_type;
  void *state_info;
} site_info_t;

// Utility function to compute the number of bits needed to encode a given number of values
inline unsigned bit_count(unsigned num) {
  int cnt= 0;

  if(num) num--;
  while(num>0) {
    num>>=1;
    cnt++;
  }
  return cnt;
}

int init_site_info(site_info_t *site_info, unsigned num_states);

inline int init_si_layers(site_info_t *site_info, unsigned num_layers) {
  return init_site_info(site_info, (1<<num_layers));
}

#endif // __SITE_H__
