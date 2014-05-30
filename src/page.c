#include "instance.h"

/* A utility function to convert raw buffers from memory into page buffers */

int alloc_pages_from_buffer(void *buf, size_t buf_size, page_t **page, size_t page_align) {
  size_t this_align, page_size;
  instance_t *instance;
  
  this_align = page_align;
  page_align += bit_count(sizeof(instance_t));
  // Align the buffer on the next page boundary to start out
  page_size= (1 << page_align) - 1;
  instance= (instance_t *)((buf + page_size) & ~page_size);
  buf_size-= (void *)instance - buf;
  page_size++;
  if( buf_size >= page_size )
    *page= instance->page;
  else
    return -1;

  while( buf_size >= page_size ) {
    instance->page.this_align= this_align;
    instance->page.chunk_align= cur_chunk_align;
    instance->page.offset= 0;
    instance->page.uid= (instance >> page_align) & 0xffff;
    instance->page.avail= 1;
    (char *)instance+= page_size;
    buf_size-= page_size;
  }

  return buf_size;
}jh

/* A utility function to allocate full-size chunks from a page */

int alloc_chunks_from_page(page_t *page, chunk_t **chunk) {
  size_t chunk_size, log_chunks;
  instance_t *instance;
  int i, num_chunks;

  instance= (instance_t *)page + ( page->avail << page->chunk_align );
  log_chunks= page->this_align - page->chunk_align;
  if(log_chunks >= 0) {
    num_chunks= (1 << log_chunks) - page->avail;
    *chunk= instance->chunk;
  } else {
    return -1;
  }

  chunk_size= (1 << page->chunk_align);
  for(i=0;i<num_chunks;i++) {
    instance+= chunk_size;
  }
}
