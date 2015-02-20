#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <sys/time.h>
#include <getopt.h>
#include <algorithm>
#include "emmintrin.h"

using namespace std;

long timediff(struct timeval before, struct timeval after){
  return (after.tv_usec - before.tv_usec) + (after.tv_sec-before.tv_sec)*1000000;
}

void * naive_memcpy(void * dst, const void * src, size_t num) {
	assert( num % sizeof(uint64_t) == 0); // only deal with nice numbers

	for (int i = 0; i < num; i+=sizeof(uint64_t)) {
		*reinterpret_cast<uint64_t *>((char*)dst + i) = *reinterpret_cast<const uint64_t *>((const char*)src + i);
	}

	return dst;
}

void * nt_memcpy(void *dst, const void *src, size_t num) {
	assert( num % 16 == 0); // only deal with nice numbers

	auto *a = reinterpret_cast<const char * >(src);

	for ( int i = 0; i < num; i+=16) {
		__m128i v = _mm_set_epi8(a[i+0], a[i+1], a[i+2], a[i+3],
								a[i + 4+0], a[i + 4+1], a[i + 4+2], a[i + 4+3],
								a[i + 4 + 4+0], a[i + 4 + 4+1], a[i + 4 + 4+2], a[i + 4 + 4+3],
								a[i+4 + 4 + 4+0], a[i+4 + 4 + 4+1], a[i+4 + 4 + 4+2], a[i+4 + 4 + 4+3]);

		_mm_stream_si128 ((__m128i*)(reinterpret_cast<char*>(dst) + i), v);
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
  bool test = false;

  while ((c = getopt (argc, argv, "s:a:t")) != -1) {
    switch (c) {
    case 's':
      sizemb = atoi(optarg);
      break;
    case 'a':
      algo = optarg;
      break;
    case 't':
    	test = true;
    	break;
    default:
      assert(("input error", 0));

    }
  }

  assert(sizemb > 0);

  size_t num = ((size_t)sizemb) * 1024 * 1024 / sizeof(char);
  const char * src = (char*)malloc(num);
  char * dst = (char*)malloc(num);

  struct timeval before, after;

  void *(*fun)(void *, const void *, size_t) = nullptr;

  if (strcmp(algo,"memmove") == 0 ) {
	fun = memmove;
  } else if (strcmp(algo, "memcpy" ) == 0) {
	fun = memcpy;
  } else if (strcmp(algo, "builtin") == 0) {
	fun = __builtin_memcpy;
  } else if (strcmp(algo, "naive") == 0 ) {
	 fun = naive_memcpy;
  } else if (strcmp(algo, "nt") == 0) {
	  fun = nt_memcpy;
  } else {
	  assert(("invalid algo", 0));

  }

  if (test) {
	  memcpy_test(fun);
  }

  gettimeofday(&before, NULL);
  fun(dst, src, num);
  gettimeofday(&after, NULL);

  long diff = timediff(before, after);
  printf("{\"sizemb\": %ld, \"timemilli\": %ld}\n", sizemb, diff/1000);
}
