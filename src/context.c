#include "context.h"

int create_threads(context_t *context, int num_threads) {
  int i, error;
  thread_t *thread;

  for(i=0;i<context->num_threads;i++) {
    thread->context= context;
    error= create_thread(thread);
    if(error) break;
    thread+= sizeof(thread_t);
  }

  context->num_threads= i;
  context->threads= thread;
  
  return error;
}

int join_threads(context_t *context) {
  int i, error, num_threads= context->num_threads;
  thread_t *thread= context->threads;
  
  for(i=0;i<num_threads;i++) {
    error= join_thread(thread);
    if(error) break;
    thread->status= THREAD_STOPPED;
    thread+= sizeof(thread_t);
  }
  return error;
}
