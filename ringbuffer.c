#include "ringbuffer.h"

ringbuffer_t* buffer_create(int size)
{
  ringbuffer_t* created = malloc(sizeof(ringbuffer_t));
  created->ring_size = size;
  created->current_size = 0;
  created->offset = 0;
  created->data = malloc(size*sizeof(double));
}

void inc_offset(ringbuffer_t* buffer)
{
  buffer->offset++;
  if(buffer->offset == buffer->ring_size){
    buffer->offset = 0;
  }
}

void dec_offset(ringbuffer_t* buffer)
{
  buffer->offset--;
  if(buffer->offset < 0){
    buffer->offset = buffer->ring_size - 1;
  }
}

void buffer_destroy(ringbuffer_t* buffer)
{
  free(buffer->data);
  free(buffer);
}

void buffer_append_first(ringbuffer_t* buffer, double entry)
{
  dec_offset(buffer);
  buffer->data[buffer->offset] = entry;
  buffer->current_size++;
  if(buffer->current_size == buffer->ring_size + 1){
    buffer->current_size = buffer->ring_size;
  }
}

void buffer_append_last(ringbuffer_t* buffer, double entry)
{
  buffer->data[(buffer->offset + buffer->current_size) % buffer->ring_size] = entry;
  buffer->current_size++;
  if(buffer->current_size == buffer->ring_size + 1){
    buffer->current_size = buffer->ring_size;
    inc_offset(buffer);
  }
}

double buffer_peek(ringbuffer_t* buffer, int index)
{
  if(index > buffer->current_size || index < 0){
    return 0;
  }else{
    return buffer->data[(index + buffer->offset) % buffer->ring_size];
  }
}

double buffer_pop_first(ringbuffer_t *buffer)
{
  if(buffer->current_size == 0){
    return 0;
  }
  int temp = buffer->data[buffer->offset];
  inc_offset(buffer);
  buffer->current_size--;
  return temp;
}

double buffer_pop_last(ringbuffer_t *buffer)
{
  if(buffer->current_size == 0){
    return 0;
  } 
  int temp = buffer->data[(buffer->offset + buffer->current_size - 1) % buffer->ring_size];
  buffer->current_size--;
  return temp;
}
