#ifndef __HANDLE_H__

#define __HANDLE_H__

typedef struct handle {
  short cmd;
  short status;
  driver_t *driver;
  context_t *context;
  void *data;
} handle_t;

#endif // __HANDLE_H__
