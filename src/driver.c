#include "driver.h"

static driver_t type_driver = { DRIVER_TYPE_TYPE, DRIVER_TYPE_TYPE, 0, 0, "Type driver", 0, 0 };
static driver_t proc_driver = { DRIVER_TYPE_TYPE, DRIVER_TYPE_PROC, 0, 0, "Processor driver", 0, &type_driver };
static driver_t mem_driver = { DRIVER_TYPE_TYPE, DRIVER_TYPE_MEM, 0, 0, "Memory driver", 0, &proc_driver };
static driver_t sys_driver = { DRIVER_TYPE_TYPE, DRIVER_TYPE_SYS, 0, 0, "System driver", 0, &mem_driver };
static driver_t thread_driver = { DRIVER_TYPE_TYPE, DRIVER_TYPE_THREAD, 0, 0, "Thread driver", 0, &sys_driver };
static driver_t disp_driver = { DRIVER_TYPE_TYPE, DRIVER_TYPE_DISPLAY, 0, 0, "Display driver", 0, &thread_driver };

static driver_t *drivers= &disp_driver;

driver_t *find_driver(int type, int id) {
  driver_t *driver= drivers;
  
  while(hook &&
	(hook->driver.type == type) &&
	(hook->driver.id == id) )
    driver= driver->next;
  return driver;
}

int add_driver(driver_t *driver) {
  driver->next= drivers;
  drivers= driver;
}
