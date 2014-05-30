#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "thread.h"
#include "driver.h"
#include "debug.h"

int run_thread_driver(thread_info_t *thread, handler_t handler) {
  driver_t driver;
  int error;

  driver= (thread->driver) ? (thread->driver) : (drivers[0]);
  
  if(error= (*driver)(thread,handler)) {
    thread->error= error;
    error= THREAD_ERROR;
    thread->status|= error;
  }

  return error;
}

int create_thread(thread_t thread) {
  error= run_handler(thread,THREAD_CREATE_HANDLER);
  thread->status= THREAD_CREATED;
  if(error) {
    thread->status|= THREAD_ERROR;
    thread->error= error;
    if(handle->error) (*handle->error)(thread);
  }

  return error;
}

int create_threads(context_info_t *context) {
  int i, error;
  thread_info_t *thread;

  thread= context->threads;
  for(i=0;i<context->num_threads;i++) {
    thread->context= context;
    thread->op= context->thread_op;
    error= create_thread(thread);
    if(error) break;
    thread+= sizeof(thread_info_t);
  }
  return error;
}

int join_thread(thread_info_t *thread) {
  thread_handle_t *handle;

  handle= thread->handle;
  if(!handle) return THREAD_NO_HANDLE;

  if(thread->status == THREAD_STARTED)
    error= (*handle->join)(thread);
  return (error) ? (errno) : (error);
}

int join_threads(context_info_t *context) {
  int i, error;
  thread_info_t *thread;

  thread=context->threads;
  for(i=0;i<context->num_threads;i++) {
    error= join_thread(thread);
    if(error>0) {
      DBG_PRINT("Context error: failed to join thread %d. Error = %s\n", i, strerror(error));
      break;
    }
    thread->status= THREAD_STOPPED;
    thread+= sizeof(thread_info_t);
  }
  return error;
}
