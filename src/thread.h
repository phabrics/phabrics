#ifndef __THREAD_H__

#define __THREAD_H__

#define THREAD_CREATE 1
#define THREAD_JOIN 2
#define THREAD_END 4
#define THREAD_ERROR 8

typedef struct thread thread_t;

struct thread {
  int reserved0;
  int reserved1;
  int reserved2;
  int reserved3;
};

int create_thread(thread_t *thread);
int join_thread(thread_t *thread);

#endif // __THREAD_H__
