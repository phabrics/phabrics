#ifndef __CHUNK_H__

#define __CHUNK_H__

// Chunks must be constrained to exactly 32 instances on all platforms (check this)
#define CHUNK_SIZE 32

#ifdef INSTANCE_SIZE
#define CHUNK_TOTAL_SIZE (INSTANCE_SIZE * CHUNK_SIZE)
#endif

typedef struct chunk_ref {
  hword_t uid;
  hword_t offset;
} chunk_ref_t;

typedef struct chunk {
  byte_t this_align;   // Alignment exponent of chunk in bytes (amount to shift 1 by to get size)
  byte_t page_align;   // Alignment exponent of page in bytes (amount to shift 1 by to get size)
  hword_t offset;      // Offset of nearest chunk with available instance measured in chunk-size units
  word_t alloc_mask;
  chunk_ref_t next;    // The next chunk in a doubly linked list of chunks
  chunk_ref_t prev;    // The previous chunk in a doubly linked list of chunks
} chunk_t;

int alloc_instance_from_chunk(chunk_t *chunk, instance_t **instance);

#endif // __CHUNK_H__
