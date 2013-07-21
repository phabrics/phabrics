#ifndef __PAGE_H__

#define __PAGE_H__

#define PAGE_TYPE char *

#define PAGE_SIZE unsigned long

#define PAGE_SIZES 2

#define PAGE_SIZE_0 (4*1024)

#define PAGE_SIZE_1 (4*1024*1024)

typedef PAGE_SIZE page_size_t;

typedef struct page_info {
  unsigned num_page_sizes;
  page_size_t sizes[PAGE_SIZES];
} page_info_t;

#endif // __PAGE_H__
