// A site defines a basic unit of space as a bit-wise cross-section of layers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "site.h"
#include "layer.h"
#include "context.h"
#include "page.h"

static const site_info_t def_site_info = 
  { -1,
    SITE_LENGTH,
    SITE_LEVEL, SITE_LEVEL, 
    { 0x0000ffff, 0x00ff00ff, 0x0f0f0f0f, 0x33333333, 0x55555555 },
    0, 0
  };

int alloc_sites(site_t **sites, int num_sites) {
  *sites= (site_t *)alloc_layer((site_t)num_sites/sizeof(char *));
}

// Allocate a specified number of bit slices of a given length
int alloc_site_slices(site_t **sites, int slice_length, int num_slices) {
  int total_length, num_sites;

  total_length= slice_length * num_slices;
  num_sites= (total_length + SITE_LENGTH - 1)/SITE_LENGTH;
  
  return alloc_sites(sites, num_sites);
}

// initialize the site info based on the number of states needed
// Generate slices for use in the binary transpose used to tranform sites

int init_site_info(site_info_t *site_info, unsigned num_states) {
  site_t *slices,slice;
  unsigned num_slices;
  int i,j;

  slices= site_info->slices;

  site_info->num_states= num_states;
  // The number of layers is strictly determnined by the number of states
  site_info->num_layers= bit_count(num_states);

  // Calculate the slices to use in transforming from sites to slices
  site_info->num_slices= num_slices= bit_count(site_info->num_layers);
  site_info->slice_layers= 1<<num_slices;
  
  for(i=num_slices-1;i>=0;--i) {
    slice= (1<<(1<<i))-1;
    for(j=i+1;j<SITE_LEVEL;++j) slice|= (slice<<(1<<j));
    *(slices++)= slice;
  }

  return 0;
}

static void *thread_fn(void *arg) {
  thread_info_t *t= (thread_info_t *)arg;
  
  *(int *)(t->data)+= 2;
  return 0;
}

static int data[2]= { 1, 2 };
static thread_info_t threads[2];
static context_info_t c = { thread_fn, 0, 2, threads };

main(int argc, char **argv) {
  site_info_t s;
  site_t *slices;
  int i, num_states, num_dims;

  num_dims= 3;

  num_states= 0xffff;

  init_site_info(&s,num_states);

  printf("%d %d %d %d %p %p\n",s.num_states,s.num_layers,s.num_slices,s.slice_layers,s.slices,s.state_info);

  slices= s.slices;
  for(i=0;i<s.num_slices;i++) printf("%8.8x\n",*(slices++));

  //  init_space(num_dims,s);
  
  for(i=0;i<2;i++) threads[i].data= &data[i];
  set_thread_handler(&c,0);
  create_threads(&c);

  printf("%d %d",data[0],data[1]);
  //  slices= alloc_layer(1024);
  //  free(slices);
  return 0;
}
