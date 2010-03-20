#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include "layer.h"
#include "page.h"

int use_heap = 0;
char mem_file[100];
char *mem_addr = NULL;
int buffer_id = 0;

/* Allocate a cacheline aligned memory buffer either from large pages or the 
 * malloc heap on the specified node.
 */
char *alloc_layer(size_t size)
{
  void *addr;
  int fmem = -1;
  size_t huge_size;

  if (!use_heap) {
    sprintf(mem_file, "/huge/site_layer_%d_%d.bin", getpid(), buffer_id++);

    if ((fmem = open (mem_file, O_CREAT | O_RDWR, 0755)) == -1) {
      printf("WARNING: unable to open file %s (errno=%d). Using malloc heap.\n", mem_file, errno);
    }
  }
  
  if (fmem == -1) {
    if (posix_memalign(&addr, (size_t)128, size)) {
      printf("ERROR: unable to malloc %d byte buffer.\n", size);
      exit(1);
    }
  } else {
    /* Delete file so that huge pages will get freed on program
     * termination.
     */
    remove(mem_file);
    huge_size = (size + PAGE_SIZE_1-1) & ~(PAGE_SIZE_1-1);

    //    assert(HUGE_PAGE_SIZE > 32*1024);
    addr = (char *) mmap (0, huge_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fmem, 0);
    if (addr == MAP_FAILED) {
      printf("ERROR: unable to mmap file %s (errno=%d).\n", mem_file, errno);
      close (fmem);
      exit (1);
    }
  }
  /* Perform a memset to ensure the memory binding.
   */
  (void *)memset(addr, 0, size);

  return addr;
}
