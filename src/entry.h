#ifndef __ENTRY_H__

#define __ENTRY_H__

#include "generic.h"

// An entry descriptor that is used to describe where the entry is located, what type it is, and how to get to it
typedef struct entry {
  byte_t table;          // Defines the entry table to use to look up the entry
  byte_t type;           // The actual type of the entry
  hword_t id;            // An index into a table of executable entries
  union {
    gen_func_t func;
    gen_args_t args;
  } info;
} entry_t;

// The interface to be implemented by a particular platform
// Adds a new global table of entries.  The table number is used if given and not already in use, and the type is used to
// determine the table lookup type for each entry
entry_t new_entry_table(entry_t entry);

// Installs an entry point of the given type in the given table and returns the reference
entry_t install_entry(entry_t entry);

// Vector to an entry given by the entry descriptor
status_t vector_entry(entry_t entry);

#endif // __ENTRY_H__
