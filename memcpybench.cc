#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <sys/time.h>
#include <getopt.h>
#include <algorithm>

using namespace std;

long timediff(struct timeval before, struct timeval after){
  return (after.tv_usec - before.tv_usec) + (after.tv_sec-before.tv_sec)*1000000;
}

void * naive_memcpy(void * dst, const void * src, size_t num) {
	assert( num % sizeof(uint64_t) == 0); // only deal with nice numbers

	for (int i = 0; i < num; i++) {
		*reinterpret_cast<uint64_t *>(dst) = *reinterpret_cast<const uint64_t *>(src);
	}

	return dst;
}


void memcpy_test(void * (*cpyfun)(void *, const void *, size_t)){
	const size_t len = 1024;
	char * dst = new char[len];
	char * src = new char[len];

	cpyfun(dst, src, len);
	assert(equal(src, src + len, dst));
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
  } else if (strcmp(algo, "naive") == 0 ) {
    naive_memcpy(dst, src, num);
  } else {
    assert(("invalid algo", 0));    
  }

  gettimeofday(&after, NULL);

  long diff = timediff(before, after);
  printf("{\"sizemb\": %ld, \"timemilli\": %ld}\n", sizemb, diff/1000);
}
