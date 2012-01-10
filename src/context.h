#ifndef __CONTEXT_H__

#define __CONTEXT_H__

typedef struct context context_t;
typedef struct driver driver_t;
typedef struct thread thread_t;

struct context {
  driver_t *thread_driver;
  int num_threads;
  thread_t *threads;
};

int create_threads(context_t *context);
int join_threads(context_t *context);

#endif // __CONTEXT_H__
