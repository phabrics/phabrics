#include "slice.h"
#include "site.h"

// A utility function to generate masks for selecting appropriate subfields of a slice to be shifted
slice_t *gen_slice_masks(slice_info_t *sli, site_t *masks, int num_masks) {
  int i,j,k,l;
  slice_t *m;
 
  l= num_masks;
  masks= alloc_slices(l);
  
  s= si->size;

  for(i=0;i<l;i++) {
    for(j=0;j<(sli->size>>l);j++) {
      for(k=0;k<(s>>i);k++) {
	
      }
    }
  }
  return masks;
}

void mask_and_shift() {
  int i, mno;
  slice_t msk;

  mno = 0;
  for(i=0;i<si->level;i++) {
    LOAD_MASK(mask,mno);
  }
}
