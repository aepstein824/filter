#include "ringbuffer.h"

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void print_compare(char* name, int expected, int actual)
{
  printf("%s expected:%d actual:%d passed:%d\n", name, expected, actual, expected == actual);
}

int main (int argc, char *argv[])
{
  ringbuffer_t* test = buffer_create(3);

  //test empty
  print_compare("peek 1", 0, buffer_peek(test, 0));
  print_compare("peek 5", 0, buffer_peek(test, 5));
  print_compare("peek -1", 0, buffer_peek(test, -1));
  print_compare("pop first empty", 0, buffer_pop_first(test));
  print_compare("pop last empty", 0, buffer_pop_last(test));
  print_compare("pop empty size", 0, test-> current_size);
 
  //append two things, still not full
  buffer_append_last(test, 10);
  buffer_append_first(test, -10);
  print_compare("offset", 2, test->offset);
  print_compare("peek(1)", 10, buffer_peek(test, 1));
  print_compare("peek(0)", -10, buffer_peek(test, 0));
  //pop first not full
  print_compare("pop first {-10, 10}", -10, buffer_pop_first(test));
  print_compare("peek(0) {10)", 10, buffer_peek(test, 0));
  print_compare("peek -1", 0, buffer_peek(test, -1));
  //overfill on append first
  buffer_append_last(test, 1);
  buffer_append_last(test, 2);
  buffer_append_first(test, -1);
  printf("should be {10, 1, start -1}\n");
  print_compare("peek 0", -1, buffer_peek(test, 0));
  print_compare("peek 1", 10, buffer_peek(test, 1));
  print_compare("peek 2",  1, buffer_peek(test, 2));
  //overfill on append last
  buffer_append_last(test, 50);
  printf("should be {start 10, 1, 50}\n");
  print_compare("peek 0", 10, buffer_peek(test, 0));
  print_compare("peek 1", 1, buffer_peek(test, 1));
  print_compare("peek 2", 50, buffer_peek(test, 2));
  //check popping from front and back
  print_compare("pop first", 10, buffer_pop_first(test));
  print_compare("pop last", 50, buffer_pop_last(test));
  print_compare("final size check", 1, test->current_size);
  print_compare("final offset check", 1, test->offset);
}

