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

	auto srcfoo = reinterpret_cast<const t * >(src);
	auto dstfoo = reinterpret_cast<t *>(dst);
	const auto end = reinterpret_cast<t *>(((char*)dst) + num);


	for (; dstfoo < end; dstfoo+=1, srcfoo+=1) {
		*dstfoo  = *srcfoo;
	}

	return dst;
}

void * naive_read(void * dst, const void * src, size_t num) {
	typedef uint64_t t;

	auto srcfoo = reinterpret_cast<const t * >(src);
	auto dstfoo = reinterpret_cast<t *>(dst);
	const auto end = reinterpret_cast<t *>(((char*)dst) + num);

	for (; srcfoo < end; srcfoo+=1) {
		*dstfoo  += *srcfoo;
	}

	return dst;
}


void * pf_memcpy(void * dst, const void * src, size_t num) {
	typedef uint64_t t;
	assert( num % sizeof (t) == 0); // only deal with nice numbers

	auto srcfoo = reinterpret_cast<const t * >(src);
	auto dstfoo = reinterpret_cast<t *>(dst);
	const auto numt = num / sizeof(t);

	for (int i = 0; i < numt; i+=8) {
		// tried + 8, + 24, but +16 did better.
		// tried 0,1,2,3 for temporality, 3 did better.
		__builtin_prefetch(srcfoo + 16, 0, 3); //next word, read, non-temporal
		dstfoo[i] = srcfoo[i];
		dstfoo[i+1] = srcfoo[i+1];
		dstfoo[i+2] = srcfoo[i+2];
		dstfoo[i+3] = srcfoo[i+3];
		dstfoo[i+4] = srcfoo[i+4];
		dstfoo[i+5] = srcfoo[i+5];
		dstfoo[i+6] = srcfoo[i+6];
		dstfoo[i+7] = srcfoo[i+7];
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

	for (long unsigned int i = 0; i < len/sizeof(int); i+=sizeof(int)) {
		src[i] = i+1;
	}

	assert(0 == posix_memalign(&dst, linesize, len));

	cpyfun(dst, src, len);
//// make them fail
//	src[len - 1] = 0x1;
//	dst[len - 1] = 0x0;
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

  const uint64_t* src = nullptr;
  {
	  uint64_t * tmp = (uint64_t*)malloc(num);

	  if (test) {
		#pragma omp parallel for
		  for (uint64_t i = 0; i < num/sizeof(uint64_t); ++i) {
			  (tmp)[i] = i;
		  }
	  }

	  src = tmp;
  }

  uint64_t * dst = nullptr;

  assert(0 == posix_memalign((void**)&dst, linesize, num)); // aligning is helpful for some impls.

  struct timeval before, after;

  void *(*fun)(void *, const void *, size_t) = nullptr;

  if (strcmp(algo,"memmove") == 0 ) {
	fun = memmove;
  } else if (strcmp(algo, "glibc" ) == 0) {
	fun = memcpy;
  } else if (strcmp(algo, "builtin") == 0) {
	fun = __builtin_memcpy;
  } else if (strcmp(algo, "naive") == 0 ) {
	 fun = naive_memcpy;
  } else if (strcmp(algo, "read") == 0) {
	  fun = naive_read; //will fail test
  } else if (strcmp(algo, "mmx") == 0) {
	  fun = nt_memcpy;
  } else if (strcmp(algo, "pf") == 0) {
	  fun = pf_memcpy;
  } else {
	  assert(("invalid algo", 0));
  }


  gettimeofday(&before, NULL);
  fun(dst, src, num);
  gettimeofday(&after, NULL);

//  ((unsigned char*)dst)[0] = 0xff; // test corruption
  if (test) {
	#pragma omp parallel for
	  for (uint64_t i = 0; i < num/sizeof(uint64_t); ++i) {
		  dst[i] == i || (abort(), true);
	  }
  }

  long diff = timediff(before, after);
  printf("{\"sizemb\": %ld, \"wallclockmilli\": %ld}\n", sizemb, diff/1000);
}
