#include "instance.h"

// Manage a fixed, variable-sized pool of instances from which to allocate instances from and free to
static instance_t *pool;

// Add a vector of instances to be used in allocating individual instances or instance collections
int add_inst_pool(instance_t *instances, int num_instances) {
  // Instances should point to a set of contiguous, aligned, num_instances instances

  instances->vector.size= num_instances;
  instances->links[0]= pool->links[1];
  instances->links[1]= instances + 1;
  
  pool->links[1]= instances;

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
