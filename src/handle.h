#ifndef __HANDLE_H__

#define __HANDLE_H__

typedef struct handle {
  hword_t cmd;                  // A command field
  hword_t status;               // A return status field
  entry_ref_t entry;            // A reference to the entry point in case it is needed by the handler
  instance_ref_t context;
  instance_ref_t data;
} handle_t;

#endif // __HANDLE_H__
