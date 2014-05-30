#ifndef __PAGE_H__

#define __PAGE_H__

#define PAGE_CHUNK_ALIGN 5

#define PAGE_TYPE char *

#define PAGE_SIZE unsigned long

#define PAGE_SIZES 2

#define PAGE_SIZE_0 (4*1024)

#define PAGE_SIZE_1 (4*1024*1024)

typedef PAGE_SIZE page_size_t;

/* Descriptor for a page, which is a unit of memory aligned on a boundary equal to its size which 
   must be a power of 2.  On architectures that support virtual memory management, these pages 
   should probably correspond to the virtual pages used to manage memory, but this is not a strict 
   requirement.  The page described here is independent of the common hardware concept or any
   device or platform-specific implementation.
 */

typedef struct page {
  byte_t this_align;  // Alignment exponent of page in bytes (amount to shift 1 by to get size)
  byte_t chunk_align; // Alignment exponent of chunk in units of instances (log of instances/chunk)
  hword_t offset;     // Offset of nearest chunk with available instance measured in chunk-size units
  hword_t uid;        // Every page has a unique identifier (usually from the address)
  hword_t empty;      // Offset of nearest chunk available as a whole chunk unit
} page_t;

int alloc_pages_from_buffer(void *buf, size_t buf_size, page_t **page, size_t page_align);
int alloc_chunks_from_page(page_t *page, chunk_t **chunk);

#endif // __PAGE_H__
