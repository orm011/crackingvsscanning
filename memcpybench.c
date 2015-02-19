#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <sys/time.h>
#include <getopt.h>

long timediff(struct timeval before, struct timeval after){
  return (after.tv_usec - before.tv_usec) + (after.tv_sec-before.tv_sec)*1000000;
}


int main( int argc, char ** argv) {

  long sizemb = -1;
  const char * algo;
  char c = -1;

  while ((c = getopt (argc, argv, "s:a:")) != -1) {
    switch (c) {
    case 's':
      sizemb = atoi(optarg);
      break;
    case 'a':
      algo = optarg;
      break;
    default:
      assert(("input error", 0));
    }  }

  assert(sizemb > 0);

  size_t num = ((size_t)sizemb) * 1024 * 1024 / sizeof(char);
  const char * src = (char*)malloc(num);
  char * dst = (char*)malloc(num);

  struct timeval before, after;
  gettimeofday(&before, NULL);

  if (strcmp(algo,"memmove") == 0 ) {
    memmove(dst, src, num);
  } else if (strcmp(algo, "memcpy" ) == 0) {
    memcpy(dst, src, num);
  } else if (strcmp(algo, "builtin") == 0) {
    __builtin_memcpy(dst, src, num); 
  } else {
    assert(("invalid algo", 0));    
  }

  gettimeofday(&after, NULL);

  long diff = timediff(before, after);
  printf("{\"sizemb\": %ld, \"timemilli\": %ld}\n", sizemb, diff/1000);
}
