#include "instance.h"

// The pool is a table of pointers to memory locations that hold collections of one or more instances
// These variables should be statically allocated in and linked in from platform-specific objects
extern instance_t *pool[];
extern int pool_size;


// Add a vector of instances to be used in allocating individual instances or instance collections
int add_inst_pool(instance_t *instances, int num_instances) {
  // Instances should point to a set of contiguous, aligned, num_instances instances

  instances->vector.size= num_instances;
  instances->links[DIR_PREV]= pool->links[DIR_NEXT];
  instances->links[DIR_NEXT]= instances + 1;
  
  pool->links[DIR_NEXT]= instances;

  // Returns a unique id for this set of instances to be used to possibly specify a specific set to use within the pool
  return id;
}

// Return a pointer to an instance if possible from the instances pointed to by instances
int get_new_instance(instance_t *instances, instance_t **instance) {
  instance_t *inst;
  
  if(instances->status != POOL_EMPTY) {
    
  }
}

void free_instance(instance_t *instance) {
  
}
