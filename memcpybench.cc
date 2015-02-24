#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <sys/time.h>
#include <getopt.h>
#include <algorithm>
#include "emmintrin.h"

using namespace std;

static const unsigned long int linesize = 64;


long timediff(struct timeval before, struct timeval after){
  return (after.tv_usec - before.tv_usec) + (after.tv_sec-before.tv_sec)*1000000;
}

void * naive_memcpy(void * dst, const void * src, size_t num) {
	typedef uint64_t t;
	assert( num % sizeof (t) == 0); // only deal with nice numbers

	auto srcfoo = reinterpret_cast<const t * >(src);
	auto dstfoo = reinterpret_cast<t *>(dst);
	const auto end = reinterpret_cast<t *>(((char*)dst) + num);

	for (int i = 0; i < num; i+=sizeof(uint64_t)) {
		*reinterpret_cast<uint64_t *>((char*)dst + i) = *reinterpret_cast<const uint64_t *>((const char*)src + i);
	}
//	for (; dstfoo < end; dstfoo+=1, srcfoo+=1) {
//		*dstfoo  = *srcfoo;
//	}

	return dst;
}


void * pf_memcpy(void * dst, const void * src, size_t num) {
	typedef uint64_t t;
	assert( num % sizeof (t) == 0); // only deal with nice numbers

	auto srcfoo = reinterpret_cast<const t * >(src);
	auto dstfoo = reinterpret_cast<t *>(dst);
	const auto end = reinterpret_cast<t *>(((char*)dst) + num);

	for (; dstfoo < end; dstfoo+=8, srcfoo+=8) {
		// tried + 8, + 24, but +16 did better.
		// tried 0,1,2,3 for temporality, 3 did better.
		__builtin_prefetch(srcfoo + 16, 0, 3); //next word, read, non-temporal
		dstfoo[0] = srcfoo[0];
		dstfoo[1] = srcfoo[1];
		dstfoo[2] = srcfoo[2];
		dstfoo[3] = srcfoo[3];
		dstfoo[4] = srcfoo[4];
		dstfoo[5] = srcfoo[5];
		dstfoo[6] = srcfoo[6];
		dstfoo[7] = srcfoo[7];
	}

	return dst;
}

void * nt_memcpy(void *dst, const void *src, size_t num) {
	assert( num % 64 == 0); // only deal with nice numbers

	auto *s = reinterpret_cast<const __m128i* >(src);
	auto *d = reinterpret_cast<__m128i * >(dst);
	size_t dqs = num / 16;

	for ( int i = 0; i < dqs; i+=4 ) {
		__m128i v1 = _mm_load_si128(s + i + 0);
		__m128i v2 = _mm_load_si128(s + i + 1);
		__m128i v3 = _mm_load_si128(s + i + 2);
		__m128i v4 = _mm_load_si128(s + i + 3);

		_mm_stream_si128(d + i + 0, v1);
		_mm_stream_si128(d + i + 1, v2);
		_mm_stream_si128(d + i + 2, v3);
		_mm_stream_si128(d + i + 3, v4);
	}

	return dst;

}

void memcpy_test(void * (*cpyfun)(void *, const void *, size_t)) {
	const size_t len = 1024;
	char * src = new char[len];
	void *dst = nullptr;

	assert(0 == posix_memalign(&dst, linesize, len));

	cpyfun(dst, src, len);
	assert(equal(src, src + len, (char*)dst));
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
  void * dst = nullptr;

  assert(0 == posix_memalign(&dst, linesize, num)); // aligning is helpful for some impls.

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
  } else if (strcmp(algo, "mmx") == 0) {
	  fun = nt_memcpy;
  } else if (strcmp(algo, "pf") == 0) {
	  fun = pf_memcpy;
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
  printf("{\"sizemb\": %ld, \"wallclockmilli\": %ld}\n", sizemb, diff/1000);
}
