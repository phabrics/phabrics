#ifdef WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "page.h"

static const page_info_t def_page_info = 
  { PAGE_SIZES,
    { PAGE_SIZE_0, PAGE_SIZE_1 }
  };

void get_page_info(page_info_t *page_info) {
  page_size_t page_size;

  (*page_info)= def_page_info;

#ifdef WINDOWS
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  page_size= si.dwPageSize;
#else
  page_size= sysconf(_SC_PAGESIZE);
#endif

  page_info->sizes[0]= PAGE_SIZE_0;
}

int init_pages() {

}
