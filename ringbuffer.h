#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdlib.h>

/**
 * ring buffer of constant size
 * all elements outside the array should be zero
 * writing outside the size should overwrite elements
    but not change the size
 */

typedef struct ringbuffer {
  double *data;
  int ring_size; //total size
  int current_size; //number of elements
  int offset; //the index of element zero
} ringbuffer_t;

//allocate and initialize buffer of size size
ringbuffer_t* buffer_create(int size);
//dealloc buffer
void buffer_destroy(ringbuffer_t* buffer);

/**
 * add a number to the beginning of the buffer
 * adding when full overwrites last element and doesn't change size
 * @param entry the new entry to insert
 */
void buffer_append_first(ringbuffer_t* buffer, double entry);
/**
 * add a number to the end of the buffer
 * adding when full overwrites first element and leaves size
 * @param entry the new entry
 */
void buffer_append_last(ringbuffer_t* buffer, double entry);
/**
 * look at an index in the buffer
 * anything out of range should be 0
 * @param index the index in the buffer
 */
double buffer_peek(ringbuffer_t* buffer, int index);
/**
 * pop the first element off of the buffer
 * returns zero if there was no first element
 */
double buffer_pop_first(ringbuffer_t *buffer);

/**
 * pop the last element off of the buffer
 * returns zero if there was no last element
 */
double buffer_pop_last(ringbuffer_t *buffer);

#endif //RINGBUFFER_H
