#ifndef __ARGS_H__

#define __ARGS_H__

typedef struct args_4b {
  byte_t arg0;
  byte_t arg1;
  byte_t arg2;
  byte_t arg3;
} args_4b;

typedef struct args_2h {
  hword_t arg0;
  hword_t arg1;
} args_2h;

typedef struct args_2b1h {
  byte_t arg0;
  byte_t arg1;
  hword_t arg0;
} args_2b1h;

#endif // __ARGS_H__
