#ifndef __DRIVER_H__

#define __DRIVER_H__

#define DRIVER_TYPE_TYPE 0
#define DRIVER_TYPE_PROC 1
#define DRIVER_TYPE_MEM 2
#define DRIVER_TYPE_SYS 3
#define DRIVER_TYPE_THREAD 4
#define DRIVER_TYPE_DISPLAY 5

typedef struct handle handle_t;
typedef int (*entry_t)(handle_t *);

typedef struct driver {
  short type;
  short id;
  short status;
  short mask;
  char *desc;
  entry_t enter;
} driver_t;

driver_t *find_driver(short type, short id);

#endif // __DRIVER_H__
